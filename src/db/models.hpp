#pragma once

#include <string>
#include <vector>

namespace macbenchforge {

// ── HardwareInfo ──────────────────────────────────────────────────────────────
// Detected at startup via sysctl + IOKit.
// Stored with each benchmark run so results are always traceable
// to the exact hardware they were produced on.
struct HardwareInfo {
    std::string chip_name;          // e.g. "Apple M1 Pro"
    int         perf_cores;         // performance CPU cores (e.g. 8)
    int         efficiency_cores;   // efficiency CPU cores (e.g. 2)
    int         total_cores;        // perf + efficiency
    int         gpu_cores;          // Metal GPU cores (e.g. 16)
    uint64_t    memory_bytes;       // total unified memory in bytes
    std::string memory_label;       // human-readable e.g. "16 GB"
    std::string macos_version;      // e.g. "14.5"
    std::string cpu_arch;           // always "arm64" for this project
    uint64_t    vram_limit_bytes;   // Dynamic VRAM limit from iogpu sysctl
};

// ── Model ─────────────────────────────────────────────────────────────────────
// Represents a discovered or manually registered .gguf model file.
struct Model {
    int         id;
    std::string name;               // derived from filename
    std::string path;               // absolute path to .gguf file
    std::string size_label;         // human-readable e.g. "4.1 GB"
    int64_t     file_size;          // bytes
    std::string added_at;           // ISO 8601 timestamp
    bool        is_active;          // false = file moved/deleted

    // HuggingFace metadata (empty if locally discovered)
    std::string hf_repo_id;         // e.g. "bartowski/Llama-3.2-3B-Instruct-GGUF"
    std::string hf_filename;        // e.g. "Llama-3.2-3B-Instruct-Q4_K_M.gguf"
    std::string quant_label;        // e.g. "Q4_K_M"
};

// ── DownloadJob ───────────────────────────────────────────────────────────────
// Tracks an in-progress or completed HuggingFace model download.
// Persisted to DB so interrupted downloads can be resumed.
struct DownloadJob {
    int         id;
    std::string repo_id;            // HF repo e.g. "bartowski/Llama-3.2-3B-GGUF"
    std::string filename;           // specific file e.g. "model-Q4_K_M.gguf"
    std::string dest_path;          // absolute destination path
    std::string status;             // "pending" | "downloading" | "done" | "failed"
    int64_t     total_bytes;        // total file size (-1 if unknown)
    int64_t     downloaded_bytes;   // bytes received so far
    std::string started_at;         // ISO 8601
    std::string finished_at;        // ISO 8601
    std::string error;              // populated if status == "failed"
};

// ── BenchmarkConfig ───────────────────────────────────────────────────────────
// Exact parameters used for a run. Stored for reproducibility.
struct BenchmarkConfig {
    int         id;
    std::string preset_name;        // "quick" | "thorough" | "custom"
    std::string prompt_type;        // e.g. "short_qa" | "long_programming"
    int         prompt_tokens;
    int         generation_tokens;
    int         repetitions;
    int         threads;
    int         gpu_layers;
};

// ── BenchmarkRun ──────────────────────────────────────────────────────────────
// One complete benchmark execution for a single model + config.
struct BenchmarkRun {
    int         id;
    int         model_id;           // FK -> models.id
    int         config_id;          // FK -> benchmark_configs.id
    std::string status;             // "pending" | "running" | "done" | "failed"
    std::string started_at;         // ISO 8601
    std::string finished_at;        // ISO 8601
    std::string error_message;
};

// ── BenchmarkResult ───────────────────────────────────────────────────────────
// Measured metrics from a completed BenchmarkRun.
struct BenchmarkResult {
    int    id;
    int    run_id;                  // FK -> benchmark_runs.id

    // Throughput
    double tokens_per_second;       // generation t/s (avg across reps)
    double prompt_tokens_per_sec;   // prompt processing t/s
    double gen_tokens_per_sec;      // generation t/s

    // Latency
    double time_to_first_token_ms;  // estimated TTFT

    // Memory (Apple Silicon unified memory)
    double ram_usage_mb;            // peak unified memory during run

    // Raw output
    std::string raw_output;         // full llama-bench JSON for re-parsing
};

// ── RunSummary ────────────────────────────────────────────────────────────────
// Flattened JOIN view of Model + BenchmarkRun + BenchmarkResult + HardwareInfo.
// Used by the API layer to send comparison data to the frontend.
// Not stored in DB -- assembled by crud.cpp query.
struct RunSummary {
    int         run_id;
    std::string model_name;
    std::string model_path;
    std::string quant_label;
    std::string hf_repo_id;
    std::string prompt_type;
    std::string preset_name;
    std::string status;
    std::string started_at;

    // Metrics
    double tokens_per_second;
    double prompt_tokens_per_sec;
    double gen_tokens_per_sec;
    double time_to_first_token_ms;
    double ram_usage_mb;

    // Hardware context (stored with run for accurate result attribution)
    std::string chip_name;
    std::string memory_label;
    int         gpu_cores;
};

// ── HFSearchResult ────────────────────────────────────────────────────────────
// One result from a HuggingFace model search.
// Not persisted -- only lives in memory during a search session.
struct HFSearchResult {
    std::string repo_id;            // e.g. "bartowski/Llama-3.2-3B-Instruct-GGUF"
    std::string author;             // e.g. "bartowski"
    std::string model_name;         // e.g. "Llama-3.2-3B-Instruct-GGUF"
    int64_t     downloads;          // total download count
    int64_t     likes;
    std::string last_modified;      // ISO 8601
    bool        is_curated;         // true if in curated list
};

// ── HFModelFile ───────────────────────────────────────────────────────────────
// One .gguf file inside a HuggingFace repo.
// Shown to user when they click a search result to pick a quant.
struct HFModelFile {
    std::string filename;           // e.g. "Llama-3.2-3B-Q4_K_M.gguf"
    int64_t     size_bytes;         // file size in bytes
    std::string size_label;         // human-readable e.g. "2.0 GB"
    std::string quant_label;        // extracted e.g. "Q4_K_M"
    bool        fits_in_memory;     // true if size_bytes < available unified memory
    bool        is_recommended;     // true if matches recommended_quant from curated list
    std::string download_url;       // full HF download URL
};

} // namespace macbenchforge