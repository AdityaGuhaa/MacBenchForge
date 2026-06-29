#include "downloader.hpp"
#include <curl/curl.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace macbenchforge {

struct ProgressData {
    int job_id;
    Crud* crud;
    std::shared_ptr<std::atomic_bool> active;
    int64_t last_update_time;
};

static size_t file_write_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

static int progress_cb(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    ProgressData* p = static_cast<ProgressData*>(clientp);
    if (p && p->active && !(*p->active)) {
        return 1; // abort
    }
    
    // Update DB, maybe throttle updates to avoid thrashing
    if (p && p->crud) {
        // Very basic throttling: only update if some time has passed or download is complete
        // For simplicity, we just update. In a real app, use a timer to limit to ~2Hz.
        p->crud->update_download_progress(p->job_id, dlnow, dltotal);
    }
    
    return 0;
}

Downloader::Downloader(const HuggingFaceConfig& config, Crud& crud)
    : config_(config), crud_(crud) {}

int Downloader::start_download(const std::string& repo_id, const std::string& filename, const std::string& dest_dir) {
    // 1. Create directory if not exists
    fs::path dir(dest_dir);
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    // 2. Build paths
    std::string dest_path = (dir / filename).string();
    std::string part_path = dest_path + ".part";
    std::string url = config_.api_base + "/" + repo_id + "/resolve/main/" + filename;

    // 3. Create Job Record
    DownloadJob job;
    job.repo_id = repo_id;
    job.filename = filename;
    job.dest_path = dest_path;
    job.total_bytes = -1; // Unknown until download starts
    
    int job_id = crud_.insert_download_job(job);

    auto active_flag = std::make_shared<std::atomic_bool>(true);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_downloads_[job_id] = active_flag;
    }

    // 4. Launch Thread
    std::thread([this, url, dest_path, part_path, job_id, active_flag]() {
        crud_.mark_download_started(job_id);

        FILE *fp = fopen(part_path.c_str(), "wb");
        if (!fp) {
            crud_.update_download_status(job_id, "failed", "Could not open file for writing.");
            return;
        }

        CURL *curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_cb);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            
            ProgressData pdata;
            pdata.job_id = job_id;
            pdata.crud = &crud_;
            pdata.active = active_flag;
            
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_cb);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &pdata);

            struct curl_slist* headers = NULL;
            if (!config_.api_token.empty()) {
                std::string auth = "Authorization: Bearer " + config_.api_token;
                headers = curl_slist_append(headers, auth.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }

            CURLcode res = curl_easy_perform(curl);
            fclose(fp);

            if (headers) curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res == CURLE_OK) {
                // Rename part to final
                fs::rename(part_path, dest_path);
                crud_.update_download_status(job_id, "done");
                crud_.mark_download_finished(job_id);
            } else if (res == CURLE_ABORTED_BY_CALLBACK) {
                crud_.update_download_status(job_id, "failed", "Cancelled by user");
            } else {
                crud_.update_download_status(job_id, "failed", curl_easy_strerror(res));
            }
        } else {
            fclose(fp);
            crud_.update_download_status(job_id, "failed", "Failed to init curl");
        }
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_downloads_.erase(job_id);
        }
    }).detach();

    return job_id;
}

void Downloader::cancel_download(int job_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_downloads_.find(job_id);
    if (it != active_downloads_.end()) {
        *(it->second) = false;
    }
}

} // namespace macbenchforge
