#pragma once

#include "config/config.hpp"
#include "db/crud.hpp"
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <atomic>

namespace macbenchforge {

// ── Downloader ────────────────────────────────────────────────────────────────
// Background downloader for HuggingFace models.
class Downloader {
public:
    Downloader(const HuggingFaceConfig& config, Crud& crud);

    // Starts download on a detached background thread.
    // Returns job ID.
    int start_download(const std::string& repo_id, const std::string& filename, const std::string& dest_dir);

    // Cancel a running download.
    void cancel_download(int job_id);

private:
    HuggingFaceConfig config_;
    Crud&             crud_;
    std::mutex        mutex_;
    std::map<int, std::shared_ptr<std::atomic_bool>> active_downloads_;
};

} // namespace macbenchforge
