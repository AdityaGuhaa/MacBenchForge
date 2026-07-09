#include "routes.hpp"
#include "hardware/hw_info.hpp"
#include "discovery/scanner.hpp"
#include "huggingface/hf_client.hpp"
#include "huggingface/downloader.hpp"
#include "benchmark/runner.hpp"
#include "export/exporter.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <filesystem>

using json = nlohmann::json;

namespace macbenchforge {

void to_json(json& j, const Model& m) {
    j = json{
        {"id", m.id},
        {"name", m.name},
        {"path", m.path},
        {"size_label", m.size_label},
        {"file_size", m.file_size},
        {"added_at", m.added_at},
        {"is_active", m.is_active},
        {"hf_repo_id", m.hf_repo_id},
        {"hf_filename", m.hf_filename},
        {"quant_label", m.quant_label}
    };
}

void to_json(json& j, const DownloadJob& d) {
    j = json{
        {"id", d.id},
        {"repo_id", d.repo_id},
        {"filename", d.filename},
        {"dest_path", d.dest_path},
        {"status", d.status},
        {"total_bytes", d.total_bytes},
        {"downloaded_bytes", d.downloaded_bytes},
        {"started_at", d.started_at},
        {"finished_at", d.finished_at},
        {"error", d.error}
    };
}

void to_json(json& j, const RunSummary& r) {
    j = json{
        {"run_id", r.run_id},
        {"model_name", r.model_name},
        {"model_path", r.model_path},
        {"quant_label", r.quant_label},
        {"hf_repo_id", r.hf_repo_id},
        {"prompt_type", r.prompt_type},
        {"preset_name", r.preset_name},
        {"status", r.status},
        {"started_at", r.started_at},
        {"tokens_per_second", r.tokens_per_second},
        {"prompt_tokens_per_sec", r.prompt_tokens_per_sec},
        {"gen_tokens_per_sec", r.gen_tokens_per_sec},
        {"time_to_first_token_ms", r.time_to_first_token_ms},
        {"ram_usage_mb", r.ram_usage_mb},
        {"chip_name", r.chip_name},
        {"memory_label", r.memory_label},
        {"gpu_cores", r.gpu_cores}
    };
}

void to_json(json& j, const HFSearchResult& r) {
    j = json{
        {"repo_id", r.repo_id},
        {"author", r.author},
        {"model_name", r.model_name},
        {"downloads", r.downloads},
        {"likes", r.likes},
        {"last_modified", r.last_modified},
        {"is_curated", r.is_curated}
    };
}

void to_json(json& j, const HFModelFile& f) {
    j = json{
        {"filename", f.filename},
        {"size_bytes", f.size_bytes},
        {"size_label", f.size_label},
        {"quant_label", f.quant_label},
        {"fits_in_memory", f.fits_in_memory},
        {"is_recommended", f.is_recommended},
        {"download_url", f.download_url}
    };
}

void setup_routes(httplib::Server& svr, Config& config, Database& db, Crud& crud, int hardware_id) {

    // Helper for JSON responses
    auto send_json = [](httplib::Response& res, const json& j) {
        res.set_header("Content-Type", "application/json");
        res.set_header("Cache-Control", "no-cache, no-store, must-revalidate");
        res.body = j.dump();
    };

    auto get_run_summary = [&crud](int id) -> std::optional<RunSummary> {
        auto vec = crud.get_run_summaries_by_ids({id});
        if (vec.empty()) return std::nullopt;
        return vec.front();
    };

    // --- HARDWARE ---
    svr.Get("/api/hardware", [send_json](const httplib::Request&, httplib::Response& res) {
        auto hw = HardwareDetector::detect();
        json j = {
            {"chip_name", hw.chip_name},
            {"perf_cores", hw.perf_cores},
            {"efficiency_cores", hw.efficiency_cores},
            {"total_cores", hw.total_cores},
            {"gpu_cores", hw.gpu_cores},
            {"memory_bytes", hw.memory_bytes},
            {"memory_label", hw.memory_label},
            {"macos_version", hw.macos_version},
            {"cpu_arch", hw.cpu_arch},
            {"vram_limit_bytes", hw.vram_limit_bytes}
        };
        send_json(res, j);
    });

    // --- MODELS ---
    svr.Get("/api/models", [&crud, send_json](const httplib::Request&, httplib::Response& res) {
        auto models = crud.get_all_models();
        json j = models;
        send_json(res, j);
    });

    svr.Post("/api/models/scan", [&crud, &config, send_json](const httplib::Request&, httplib::Response& res) {
        Scanner scanner;

        // Scan specific common user-accessible directories on macOS (avoid full ~ recursion which is too slow)
        std::vector<std::string> scan_dirs;

        const char* home = getenv("HOME");
        if (home) {
            std::string h(home);
            scan_dirs.push_back(h + "/Downloads");
            scan_dirs.push_back(h + "/Desktop");
            scan_dirs.push_back(h + "/Documents");
            scan_dirs.push_back(h + "/.lmstudio/models");
            scan_dirs.push_back(h + "/.cache/lm-studio/models");
            scan_dirs.push_back(h + "/llama.cpp/models");
        }

        // External volumes (USB drives, external disks)
        if (std::filesystem::exists("/Volumes")) {
            for (const auto& entry : std::filesystem::directory_iterator("/Volumes",
                    std::filesystem::directory_options::skip_permission_denied)) {
                if (entry.is_directory()) {
                    scan_dirs.push_back(entry.path().string());
                }
            }
        }

        // Also scan /tmp and /opt in case models are stored there
        if (std::filesystem::exists("/tmp")) scan_dirs.push_back("/tmp");
        if (std::filesystem::exists("/opt")) scan_dirs.push_back("/opt");

        // Add any config-defined directories that aren't already covered
        for (const auto& d : config.scan_dirs) {
            std::string expanded = expand_path(d);
            bool already = false;
            for (const auto& s : scan_dirs) {
                if (expanded.find(s) == 0 || s.find(expanded) == 0) {
                    already = true;
                    break;
                }
            }
            if (!already) scan_dirs.push_back(expanded);
        }

        auto found = scanner.scan(scan_dirs, true); // always recursive
        int added = 0;

        for (auto& m : found) {
            crud.insert_model(m);
            added++;
        }

        // Deactivate models whose files no longer exist
        auto db_models = crud.get_all_models();
        int deactivated = 0;
        for (const auto& m : db_models) {
            if (!std::filesystem::exists(m.path)) {
                crud.deactivate_model(m.id);
                deactivated++;
            }
        }

        // Return fresh model list
        auto all_models = crud.get_all_models();
        json models_json = json::array();
        for (const auto& m : all_models) {
            models_json.push_back({
                {"id", m.id},
                {"name", m.name},
                {"path", m.path},
                {"file_size", m.file_size},
                {"size_label", m.size_label},
                {"quant_label", m.quant_label},
                {"is_active", m.is_active}
            });
        }

        json j = {
            {"count", (int)all_models.size()},
            {"scanned_dirs", (int)scan_dirs.size()},
            {"new_found", added},
            {"deactivated", deactivated},
            {"models", models_json}
        };
        send_json(res, j);
    });

    svr.Post("/api/models/pick", [&crud, send_json](const httplib::Request&, httplib::Response& res) {
        std::string cmd = "osascript -e 'set frontApp to (path to frontmost application as text)' "
                          "-e 'tell application \"Finder\"' -e 'activate' "
                          "-e 'set myFile to choose file with prompt \"Select a GGUF Model\"' "
                          "-e 'end tell' -e 'tell application frontApp to activate' -e 'POSIX path of myFile'";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            res.status = 500;
            send_json(res, {{"error", "Failed to open dialog"}});
            return;
        }
        char buffer[1024];
        std::string path = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            path += buffer;
        }
        pclose(pipe);

        if (!path.empty() && path.back() == '\n') path.pop_back();

        if (path.empty()) {
            res.status = 400;
            send_json(res, {{"error", "Cancelled"}});
            return;
        }

        auto m_opt = Scanner::scan_file(path);
        if (!m_opt) {
            res.status = 400;
            send_json(res, {{"error", "Invalid file or not a regular file"}});
            return;
        }

        int id = crud.insert_model(*m_opt);
        auto m = crud.get_model_by_id(id);
        if (m) {
            json j = {{"ok", true}, {"model", *m}};
            send_json(res, j);
        } else {
            res.status = 500;
            send_json(res, {{"error", "Failed to insert model"}});
        }
    });

    // --- HUGGINGFACE ---
    svr.Get("/api/hf/search", [&config, send_json](const httplib::Request& req, httplib::Response& res) {
        std::string query = req.get_param_value("q");
        int page = 0;
        if (req.has_param("page")) page = std::stoi(req.get_param_value("page"));
        
        HFClient client(config.huggingface, HardwareDetector::memory_bytes());
        if (query.empty()) {
            json j = client.get_curated();
            send_json(res, j);
        } else {
            json j = client.search(query, page, config.huggingface.page_size);
            send_json(res, j);
        }
    });

    svr.Get("/api/hf/files", [&config, send_json](const httplib::Request& req, httplib::Response& res) {
        std::string repo_id = req.get_param_value("repo_id");
        HFClient client(config.huggingface, HardwareDetector::memory_bytes());
        json j = client.get_model_files(repo_id);
        send_json(res, j);
    });

    // Make downloader static so it persists across requests
    static Downloader* downloader = new Downloader(config.huggingface, crud);

    svr.Post("/api/hf/download", [&config, send_json](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            std::string repo_id = body["repo_id"];
            std::string filename = body["filename"];
            
            int job_id = downloader->start_download(repo_id, filename, config.download_dir);
            json j = {{"job_id", job_id}};
            send_json(res, j);
        } catch (...) {
            res.status = 400;
            send_json(res, {{"error", "Bad Request"}});
        }
    });

    // --- DOWNLOADS ---
    svr.Get("/api/downloads", [&crud, send_json](const httplib::Request&, httplib::Response& res) {
        auto jobs = crud.get_all_download_jobs();
        json j = jobs;
        send_json(res, j);
    });

    svr.Get(R"(/api/downloads/(\d+))", [&crud, send_json](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto job = crud.get_download_job(id);
        if (job) {
            json j = *job;
            send_json(res, j);
        } else {
            res.status = 404;
            send_json(res, {{"error", "Not Found"}});
        }
    });

    // --- CONFIG ---
    svr.Get("/api/config", [&config, send_json](const httplib::Request&, httplib::Response& res) {
        json presets_j = {
            {"quick", {
                {"prompt_tokens", config.preset_quick.prompt_tokens},
                {"generation_tokens", config.preset_quick.generation_tokens},
                {"repetitions", config.preset_quick.repetitions}
            }},
            {"thorough", {
                {"prompt_tokens", config.preset_thorough.prompt_tokens},
                {"generation_tokens", config.preset_thorough.generation_tokens},
                {"repetitions", config.preset_thorough.repetitions}
            }}
        };
        
        json prompt_presets_j = json::array();
        for (const auto& p : config.prompt_presets) {
            prompt_presets_j.push_back({
                {"id", p.id},
                {"label", p.label},
                {"description", p.description},
                {"prompt_tokens", p.prompt_tokens},
                {"generation_tokens", p.generation_tokens}
            });
        }
        
        json curated_j = json::array();
        for (const auto& c : config.huggingface.curated) {
            curated_j.push_back({
                {"repo_id", c.repo_id},
                {"description", c.description},
                {"recommended_quant", c.recommended_quant}
            });
        }
        
        json j = {
            {"presets", presets_j},
            {"prompt_presets", prompt_presets_j},
            {"curated", curated_j}
        };
        send_json(res, j);
    });

    // --- BENCHMARK ---
    svr.Post("/api/benchmark/run", [&config, &crud, hardware_id, send_json](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            int model_id = body["model_id"];
            std::string preset_name = body["preset_name"];
            std::string prompt_type = body["prompt_type"];

            // Validate model exists before spawning thread
            auto model_opt = crud.get_model_by_id(model_id);
            if (!model_opt) {
                res.status = 404;
                send_json(res, {{"error", "Model not found"}});
                return;
            }

            // Spawn benchmark on a detached thread.
            // BenchmarkRunner creates the config + run records itself.
            // Client polls /api/runs to find the latest run.
            std::thread([&crud, &config, model_id, preset_name, prompt_type, hardware_id]() {
                BenchmarkRunner runner(config, crud);
                runner.run_benchmark(model_id, preset_name, prompt_type, hardware_id);
            }).detach();

            json j = {{"status", "started"}};
            send_json(res, j);
            
        } catch (...) {
            res.status = 400;
            send_json(res, {{"error", "Bad Request"}});
        }
    });

    svr.Get(R"(/api/benchmark/status/(\d+))", [get_run_summary, send_json](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto run = get_run_summary(id);
        if (run) {
            json j = *run;
            send_json(res, j);
        } else {
            res.status = 404;
            send_json(res, {{"error", "Not Found"}});
        }
    });

    // --- RUNS ---
    svr.Get("/api/runs", [&crud, send_json](const httplib::Request&, httplib::Response& res) {
        auto runs = crud.get_all_run_summaries();
        json j = runs;
        send_json(res, j);
    });

    svr.Get(R"(/api/runs/(\d+))", [get_run_summary, send_json](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto run = get_run_summary(id);
        if (run) {
            json j = *run;
            send_json(res, j);
        } else {
            res.status = 404;
            send_json(res, {{"error", "Not Found"}});
        }
    });

    svr.Delete(R"(/api/runs/(\d+))", [&crud, send_json](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        crud.delete_run(id);
        send_json(res, {{"ok", true}});
    });

    // --- EXPORT ---
    svr.Post("/api/export/json", [&config, get_run_summary, send_json](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            std::vector<int> run_ids = body["run_ids"];
            std::vector<RunSummary> summaries;
            for (int id : run_ids) {
                auto s = get_run_summary(id);
                if (s) summaries.push_back(*s);
            }
            
            Exporter exp(config.export_dir);
            std::string path = exp.export_json(summaries);
            send_json(res, {{"path", path}});
        } catch (...) {
            res.status = 400;
            send_json(res, {{"error", "Bad Request"}});
        }
    });

    svr.Post("/api/export/csv", [&config, get_run_summary, send_json](const httplib::Request& req, httplib::Response& res) {
        try {
            json body = json::parse(req.body);
            std::vector<int> run_ids = body["run_ids"];
            std::vector<RunSummary> summaries;
            for (int id : run_ids) {
                auto s = get_run_summary(id);
                if (s) summaries.push_back(*s);
            }
            
            Exporter exp(config.export_dir);
            std::string path = exp.export_csv(summaries);
            send_json(res, {{"path", path}});
        } catch (...) {
            res.status = 400;
            send_json(res, {{"error", "Bad Request"}});
        }
    });
}

} // namespace macbenchforge
