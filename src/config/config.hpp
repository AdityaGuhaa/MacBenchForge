#pragma once

#include <string>
#include <vector>

namespace macbenchforge {

// ── PresetParams ──────────────────────────────────────────────────────────────
// Parameters for a benchmark preset (quick / thorough).
struct PresetParams {
    int prompt_tokens;
    int generation_tokens;
    int repetitions;
};

// ── PromptPreset ──────────────────────────────────────────────────────────────
// A named prompt scenario with token counts.
struct PromptPreset {
    std::string id;               // e.g. "short_qa"
    std::string label;            // e.g. "Short QA"
    std::string description;      // e.g. "Short factual question-answer"
    int         prompt_tokens;
    int         generation_tokens;
};

// ── CuratedModel ─────────────────────────────────────────────────────────────
// A recommended model from the curated list in config.toml.
struct CuratedModel {
    std::string repo_id;           // e.g. "bartowski/Llama-3.2-3B-Instruct-GGUF"
    std::string description;       // e.g. "Meta Llama 3.2 3B -- fast"
    std::string recommended_quant; // e.g. "Q4_K_M"
};

// ── HuggingFaceConfig ────────────────────────────────────────────────────────
// HuggingFace API settings.
struct HuggingFaceConfig {
    std::string api_base;          // "https://huggingface.co"
    std::string api_search;        // "https://huggingface.co/api/models"
    std::string api_token;         // optional API token
    int         page_size;         // results per page

    std::vector<CuratedModel> curated;
};

// ── Config ────────────────────────────────────────────────────────────────────
// Root configuration loaded from config.toml.
struct Config {
    // [general]
    int         port;
    std::string llama_bench_path;
    std::string db_path;
    bool        open_browser;
    std::string theme;

    // [discovery]
    std::vector<std::string> scan_dirs;
    bool        recursive_scan;
    std::string download_dir;

    // [benchmark]
    std::string default_preset;
    int         threads;
    int         gpu_layers;
    int         repetitions;

    // [presets]
    PresetParams preset_quick;
    PresetParams preset_thorough;

    // [prompts]
    std::vector<PromptPreset> prompt_presets;

    // [huggingface]
    HuggingFaceConfig huggingface;

    // [export]
    std::string export_dir;
};

// ── Functions ─────────────────────────────────────────────────────────────────

// Expand ~ to $HOME in a path string.
std::string expand_path(const std::string& path);

// Load configuration from a TOML file. Falls back to defaults for
// any missing keys. Throws std::runtime_error on parse failure.
Config load_config(const std::string& path);

// Returns a Config populated with all default values.
Config default_config();

} // namespace macbenchforge
