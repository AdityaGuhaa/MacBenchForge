#include "runner.hpp"
#include "metrics.hpp"
#include "hardware/hw_info.hpp"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <memory>
#include <array>

namespace macbenchforge {

BenchmarkRunner::BenchmarkRunner(const Config& config, Crud& crud)
    : config_(config), crud_(crud) {}

int BenchmarkRunner::run_benchmark(int model_id, const std::string& preset_name, const std::string& prompt_type, int hardware_id) {
    
    // 1. Resolve model
    auto model_opt = crud_.get_model_by_id(model_id);
    if (!model_opt) {
        std::cerr << "BenchmarkRunner: Model ID " << model_id << " not found." << std::endl;
        return -1;
    }
    Model model = *model_opt;

    // 2. Resolve Config
    BenchmarkConfig bc;
    bc.preset_name = preset_name;
    bc.prompt_type = prompt_type;
    bc.threads = config_.threads;
    bc.gpu_layers = config_.gpu_layers;

    if (preset_name == "quick") {
        bc.prompt_tokens = config_.preset_quick.prompt_tokens;
        bc.generation_tokens = config_.preset_quick.generation_tokens;
        bc.repetitions = config_.preset_quick.repetitions;
    } else if (preset_name == "thorough") {
        bc.prompt_tokens = config_.preset_thorough.prompt_tokens;
        bc.generation_tokens = config_.preset_thorough.generation_tokens;
        bc.repetitions = config_.preset_thorough.repetitions;
    } else {
        // Fallback or custom logic, try to find prompt preset
        bool found = false;
        for (const auto& p : config_.prompt_presets) {
            if (p.id == prompt_type) {
                bc.prompt_tokens = p.prompt_tokens;
                bc.generation_tokens = p.generation_tokens;
                bc.repetitions = config_.repetitions; // default
                found = true;
                break;
            }
        }
        if (!found) {
            bc.prompt_tokens = 128;
            bc.generation_tokens = 128;
            bc.repetitions = 1;
        }
    }

    int config_id = crud_.insert_config(bc);

    // 3. Create Run Record
    int run_id = crud_.insert_run(model_id, config_id, hardware_id);
    crud_.mark_run_started(run_id);

    // 4. Build Command
    std::ostringstream cmd;
    cmd << config_.llama_bench_path
        << " -m \"" << model.path << "\""
        << " -t " << bc.threads
        << " -ngl " << bc.gpu_layers
        << " -p " << bc.prompt_tokens
        << " -n " << bc.generation_tokens
        << " -r " << bc.repetitions
        << " -o json"
        << " 2>&1"; // capture stderr as well

    std::string command = cmd.str();
    std::string output = "";

    // 5. Start Memory Poller
    MemoryPoller poller;
    poller.start();

    // 6. Execute Subprocess
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        crud_.update_run_status(run_id, "failed", "popen() failed");
        crud_.mark_run_finished(run_id);
        return run_id;
    }

    std::array<char, 128> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    int return_code = pclose(pipe);

    // 7. Stop Memory Poller
    poller.stop();
    double peak_ram = poller.peak_ram_mb();

    // 8. Handle Result
    if (return_code == 0) {
        // We capture stderr into output too, so we might need to find the JSON array part
        // llama-bench JSON array usually starts with [ and ends with ]
        std::string json_str = output;
        size_t start_idx = output.find('[');
        size_t end_idx = output.rfind(']');
        if (start_idx != std::string::npos && end_idx != std::string::npos && end_idx > start_idx) {
            json_str = output.substr(start_idx, end_idx - start_idx + 1);
        }

        BenchmarkResult res = MetricsParser::parse(json_str, peak_ram);
        res.run_id = run_id;
        
        crud_.insert_result(res);
        crud_.update_run_status(run_id, "done");
    } else {
        crud_.update_run_status(run_id, "failed", "llama-bench exited with code " + std::to_string(return_code) + "\n" + output);
    }

    crud_.mark_run_finished(run_id);

    return run_id;
}

} // namespace macbenchforge
