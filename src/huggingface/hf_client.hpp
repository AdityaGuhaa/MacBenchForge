#pragma once

#include "config/config.hpp"
#include "db/models.hpp"
#include <string>
#include <vector>

namespace macbenchforge {

// ── HFClient ──────────────────────────────────────────────────────────────────
// Client for HuggingFace API.
class HFClient {
public:
    HFClient(const HuggingFaceConfig& config, uint64_t system_memory_bytes);

    // Search for models on HF hub.
    std::vector<HFSearchResult> search(const std::string& query, int page, int page_size);

    // Get .gguf files for a specific repo.
    std::vector<HFModelFile> get_model_files(const std::string& repo_id);

    // Returns curated models as HFSearchResults.
    std::vector<HFSearchResult> get_curated();

private:
    HuggingFaceConfig config_;
    uint64_t          system_memory_bytes_;

    std::string curl_get(const std::string& url);
};

} // namespace macbenchforge
