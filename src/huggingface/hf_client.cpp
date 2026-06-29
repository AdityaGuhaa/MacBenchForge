#include "hf_client.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>

using json = nlohmann::json;

namespace macbenchforge {

namespace {
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
    std::string format_size(uint64_t bytes) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1);
        if (bytes > 1024ULL * 1024 * 1024) {
            oss << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
        } else if (bytes > 1024ULL * 1024) {
            oss << (bytes / (1024.0 * 1024.0)) << " MB";
        } else {
            oss << bytes << " B";
        }
        return oss.str();
    }
}

HFClient::HFClient(const HuggingFaceConfig& config, uint64_t system_memory_bytes)
    : config_(config), system_memory_bytes_(system_memory_bytes) {}

std::string HFClient::curl_get(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        struct curl_slist* headers = NULL;
        if (!config_.api_token.empty()) {
            std::string auth = "Authorization: Bearer " + config_.api_token;
            headers = curl_slist_append(headers, auth.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        if (headers) curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

std::vector<HFSearchResult> HFClient::search(const std::string& query, int page, int page_size) {
    std::vector<HFSearchResult> results;
    
    // Build query URL
    // e.g. https://huggingface.co/api/models?search=llama&filter=gguf&sort=downloads&direction=-1&limit=20
    std::string url = config_.api_search + "?filter=gguf&sort=downloads&direction=-1";
    if (!query.empty()) {
        char* escaped = curl_easy_escape(NULL, query.c_str(), query.length());
        if (escaped) {
            url += "&search=" + std::string(escaped);
            curl_free(escaped);
        }
    }
    url += "&limit=" + std::to_string(page_size);
    url += "&offset=" + std::to_string(page * page_size);

    std::string response = curl_get(url);
    if (response.empty()) return results;

    try {
        json j = json::parse(response);
        if (j.is_array()) {
            for (const auto& item : j) {
                HFSearchResult r;
                r.repo_id = item.value("id", "");
                auto pos = r.repo_id.find('/');
                if (pos != std::string::npos) {
                    r.author = r.repo_id.substr(0, pos);
                    r.model_name = r.repo_id.substr(pos + 1);
                } else {
                    r.model_name = r.repo_id;
                }
                r.downloads = item.value("downloads", 0);
                r.likes = item.value("likes", 0);
                r.last_modified = item.value("lastModified", "");
                
                // Check if curated
                r.is_curated = false;
                for (const auto& c : config_.curated) {
                    if (c.repo_id == r.repo_id) {
                        r.is_curated = true;
                        break;
                    }
                }
                
                results.push_back(r);
            }
        }
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error in HF search: " << e.what() << std::endl;
    }
    
    return results;
}

std::vector<HFModelFile> HFClient::get_model_files(const std::string& repo_id) {
    std::vector<HFModelFile> files;
    
    std::string url = config_.api_search + "/" + repo_id;
    std::string response = curl_get(url);
    if (response.empty()) return files;

    try {
        json j = json::parse(response);
        
        // Find if this is a curated model to get recommended quant
        std::string recommended_quant = "";
        for (const auto& c : config_.curated) {
            if (c.repo_id == repo_id) {
                recommended_quant = c.recommended_quant;
                break;
            }
        }

        if (j.contains("siblings") && j["siblings"].is_array()) {
            for (const auto& sibling : j["siblings"]) {
                std::string rfilename = sibling.value("rfilename", "");
                if (rfilename.length() > 5 && rfilename.substr(rfilename.length() - 5) == ".gguf") {
                    HFModelFile f;
                    f.filename = rfilename;
                    // Note: size might not be directly in siblings for all models in this API format
                    // Often it needs a tree API call, but we might get it if available, or fetch from info
                    // The standard API doesn't always provide file size in siblings.
                    // For MacBenchForge, we will try to get size from tree API if needed, or assume it's there.
                    
                    // Actually, a better way to get file sizes is the tree API:
                    // https://huggingface.co/api/models/{repo_id}/tree/main
                    // Let's do a secondary call if needed, or just do the tree call instead.
                }
            }
        }
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error in get_model_files: " << e.what() << std::endl;
    }

    // Let's use the tree API to get file sizes properly.
    std::string tree_url = config_.api_base + "/api/models/" + repo_id + "/tree/main";
    std::string tree_resp = curl_get(tree_url);
    if (!tree_resp.empty()) {
        try {
            json tj = json::parse(tree_resp);
            if (tj.is_array()) {
                std::string recommended_quant = "";
                for (const auto& c : config_.curated) {
                    if (c.repo_id == repo_id) {
                        recommended_quant = c.recommended_quant;
                        break;
                    }
                }

                for (const auto& item : tj) {
                    if (item.value("type", "") == "file") {
                        std::string path = item.value("path", "");
                        if (path.length() > 5 && path.substr(path.length() - 5) == ".gguf") {
                            HFModelFile f;
                            f.filename = path;
                            f.size_bytes = item.value("size", 0LL);
                            f.size_label = format_size(f.size_bytes);
                            
                            // Extract quant
                            f.quant_label = "Unknown";
                            auto dash_pos = path.rfind('-');
                            if (dash_pos != std::string::npos) {
                                f.quant_label = path.substr(dash_pos + 1, path.length() - dash_pos - 6);
                            }
                            
                            // 80% threshold for fits_in_memory
                            f.fits_in_memory = f.size_bytes < (system_memory_bytes_ * 0.8);
                            f.is_recommended = (f.quant_label == recommended_quant);
                            f.download_url = config_.api_base + "/" + repo_id + "/resolve/main/" + f.filename;
                            
                            files.push_back(f);
                        }
                    }
                }
            }
        } catch (const json::parse_error& e) {
             std::cerr << "JSON parse error in tree API: " << e.what() << std::endl;
        }
    }
    
    return files;
}

std::vector<HFSearchResult> HFClient::get_curated() {
    std::vector<HFSearchResult> results;
    for (const auto& c : config_.curated) {
        HFSearchResult r;
        r.repo_id = c.repo_id;
        auto pos = r.repo_id.find('/');
        if (pos != std::string::npos) {
            r.author = r.repo_id.substr(0, pos);
            r.model_name = r.repo_id.substr(pos + 1);
        } else {
            r.model_name = r.repo_id;
        }
        r.is_curated = true;
        // In a real app we might fetch the latest stats, but for curated list we just return what we have
        results.push_back(r);
    }
    return results;
}

} // namespace macbenchforge
