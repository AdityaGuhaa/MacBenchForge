#pragma once

#include "config/config.hpp"
#include "db/crud.hpp"
#include <string>

namespace macbenchforge {

// ── BenchmarkRunner ───────────────────────────────────────────────────────────
// Executes a llama-bench run.
class BenchmarkRunner {
public:
    BenchmarkRunner(const Config& config, Crud& crud);

    // Runs a benchmark.
    // Returns the newly created run_id.
    // This is synchronous (blocking) and should be called on a background thread.
    int run_benchmark(int model_id, const std::string& preset_name, const std::string& prompt_type, int hardware_id);

private:
    Config config_;
    Crud&  crud_;
};

} // namespace macbenchforge
