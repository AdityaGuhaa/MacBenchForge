#include "config.hpp"

#include <stdexcept>
#include <cstdlib>
#include <toml++/toml.hpp>

namespace macbenchforge {

// ── Helpers ───────────────────────────────────────────────────────────────────

std::string expand_path(const std::string& path) {
    if (path.empty() || path[0] != '~') return path;
    const char* home = std::getenv("HOME");
    if (!home) return path;
    return std::string(home) + path.substr(1);
}

// ── Defaults ──────────────────────────────────────────────────────────────────

Config default_config() {
    Config cfg;

    // [general]
    cfg.port             = 7860;
    cfg.llama_bench_path = "/opt/homebrew/bin/llama-bench";
    cfg.db_path          = expand_path("~/.macbenchforge/macbenchforge.db");
    cfg.open_browser     = true;
    cfg.theme            = "light";

    // [discovery]
    cfg.scan_dirs = {
        expand_path("~/Downloads"),
        expand_path("~/.cache/huggingface/hub")
    };
    cfg.recursive_scan = true;
    cfg.download_dir   = expand_path("~/Downloads/MacBenchForge/models");

    // [benchmark]
    cfg.default_preset = "quick";
    cfg.threads        = 8;
    cfg.gpu_layers     = 99;
    cfg.repetitions    = 3;

    // [presets]
    cfg.preset_quick    = { 128, 128, 1 };
    cfg.preset_thorough = { 512, 512, 5 };

    // [prompts]
    cfg.prompt_presets = {
        { "short_qa",           "Short QA",           "Short factual question-answer",        64,   64   },
        { "short_programming",  "Short Programming",  "Short code completion",                128,  128  },
        { "medium_reasoning",   "Medium Reasoning",   "Multi-step reasoning task",            256,  256  },
        { "medium_programming", "Medium Programming", "Medium complexity coding task",        512,  256  },
        { "long_architecture",  "Long Architecture",  "Large context architectural analysis", 1024, 512  },
        { "long_programming",   "Long Programming",   "Long codebase context task",           1024, 1024 },
    };

    // [huggingface]
    cfg.huggingface.api_base   = "https://huggingface.co";
    cfg.huggingface.api_search = "https://huggingface.co/api/models";
    cfg.huggingface.api_token  = "";
    cfg.huggingface.page_size  = 20;
    cfg.huggingface.curated    = {
        { "bartowski/Llama-3.2-3B-Instruct-GGUF",    "Meta Llama 3.2 3B -- fast, great for everyday tasks", "Q4_K_M" },
        { "bartowski/Llama-3.2-1B-Instruct-GGUF",    "Meta Llama 3.2 1B -- extremely fast, low memory",     "Q4_K_M" },
        { "bartowski/gemma-3-4b-it-GGUF",            "Google Gemma 3 4B -- strong reasoning",               "Q4_K_M" },
        { "bartowski/gemma-3-1b-it-GGUF",            "Google Gemma 3 1B -- ultra fast",                     "Q4_K_M" },
        { "bartowski/Qwen2.5-7B-Instruct-GGUF",      "Alibaba Qwen 2.5 7B -- excellent multilingual",       "Q4_K_M" },
        { "bartowski/Phi-4-mini-instruct-GGUF",       "Microsoft Phi-4 Mini -- strong reasoning, small",     "Q4_K_M" },
        { "bartowski/Mistral-7B-Instruct-v0.3-GGUF", "Mistral 7B v0.3 -- reliable all-rounder",             "Q4_K_M" },
        { "bartowski/SmolLM2-1.7B-Instruct-GGUF",    "HuggingFace SmolLM2 1.7B -- tiny and capable",        "Q4_K_M" },
    };

    // [export]
    cfg.export_dir = expand_path("~/Downloads");

    return cfg;
}

// ── Loader ────────────────────────────────────────────────────────────────────

Config load_config(const std::string& path) {
    Config cfg = default_config();

    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw std::runtime_error(
            std::string("Failed to parse config.toml: ") + e.description().data()
        );
    }

    // [general]
    if (auto v = tbl["general"]["port"].value<int>())
        cfg.port = *v;
    if (auto v = tbl["general"]["llama_bench_path"].value<std::string>())
        cfg.llama_bench_path = *v;
    if (auto v = tbl["general"]["db_path"].value<std::string>())
        cfg.db_path = expand_path(*v);
    if (auto v = tbl["general"]["open_browser"].value<bool>())
        cfg.open_browser = *v;
    if (auto v = tbl["general"]["theme"].value<std::string>())
        cfg.theme = *v;

    // [discovery]
    if (auto* arr = tbl["discovery"]["scan_dirs"].as_array()) {
        cfg.scan_dirs.clear();
        arr->for_each([&](auto&& el) {
            if constexpr (toml::is_string<decltype(el)>)
                cfg.scan_dirs.push_back(expand_path(std::string(*el)));
        });
    }
    if (auto v = tbl["discovery"]["recursive_scan"].value<bool>())
        cfg.recursive_scan = *v;
    if (auto v = tbl["discovery"]["download_dir"].value<std::string>())
        cfg.download_dir = expand_path(*v);

    // [benchmark]
    if (auto v = tbl["benchmark"]["default_preset"].value<std::string>())
        cfg.default_preset = *v;
    if (auto v = tbl["benchmark"]["threads"].value<int>())
        cfg.threads = *v;
    if (auto v = tbl["benchmark"]["gpu_layers"].value<int>())
        cfg.gpu_layers = *v;
    if (auto v = tbl["benchmark"]["repetitions"].value<int>())
        cfg.repetitions = *v;

    // [presets.quick]
    if (auto v = tbl["presets"]["quick"]["prompt_tokens"].value<int>())
        cfg.preset_quick.prompt_tokens = *v;
    if (auto v = tbl["presets"]["quick"]["generation_tokens"].value<int>())
        cfg.preset_quick.generation_tokens = *v;
    if (auto v = tbl["presets"]["quick"]["repetitions"].value<int>())
        cfg.preset_quick.repetitions = *v;

    // [presets.thorough]
    if (auto v = tbl["presets"]["thorough"]["prompt_tokens"].value<int>())
        cfg.preset_thorough.prompt_tokens = *v;
    if (auto v = tbl["presets"]["thorough"]["generation_tokens"].value<int>())
        cfg.preset_thorough.generation_tokens = *v;
    if (auto v = tbl["presets"]["thorough"]["repetitions"].value<int>())
        cfg.preset_thorough.repetitions = *v;

    // [prompts] -- override built-in presets if defined in toml
    for (auto& preset : cfg.prompt_presets) {
        auto node = tbl["prompts"][preset.id];
        if (auto v = node["label"].value<std::string>())
            preset.label = *v;
        if (auto v = node["description"].value<std::string>())
            preset.description = *v;
        if (auto v = node["prompt_tokens"].value<int>())
            preset.prompt_tokens = *v;
        if (auto v = node["generation_tokens"].value<int>())
            preset.generation_tokens = *v;
    }

    // [huggingface]
    if (auto v = tbl["huggingface"]["api_base"].value<std::string>())
        cfg.huggingface.api_base = *v;
    if (auto v = tbl["huggingface"]["api_search"].value<std::string>())
        cfg.huggingface.api_search = *v;
    if (auto v = tbl["huggingface"]["api_token"].value<std::string>())
        cfg.huggingface.api_token = *v;
    if (auto v = tbl["huggingface"]["page_size"].value<int>())
        cfg.huggingface.page_size = *v;

    // [[huggingface.curated]] -- array of tables
    if (auto* arr = tbl["huggingface"]["curated"].as_array()) {
        cfg.huggingface.curated.clear();
        arr->for_each([&](auto&& el) {
            if constexpr (toml::is_table<decltype(el)>) {
                CuratedModel m;
                if (auto v = el["repo_id"].template value<std::string>())
                    m.repo_id = *v;
                if (auto v = el["description"].template value<std::string>())
                    m.description = *v;
                if (auto v = el["recommended_quant"].template value<std::string>())
                    m.recommended_quant = *v;
                if (!m.repo_id.empty())
                    cfg.huggingface.curated.push_back(m);
            }
        });
    }

    // [export]
    if (auto v = tbl["export"]["export_dir"].value<std::string>())
        cfg.export_dir = expand_path(*v);

    return cfg;
}

} // namespace macbenchforge