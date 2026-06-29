#pragma once

#include "db/models.hpp"
#include <string>

namespace macbenchforge {

// ── MetricsParser ─────────────────────────────────────────────────────────────
// Utility to parse llama-bench JSON output.
struct MetricsParser {
    // Parse the raw JSON output array from llama-bench.
    // Extracts prompt t/s, gen t/s, TTFT.
    // Populates a BenchmarkResult (with run_id = 0, to be set by caller).
    static BenchmarkResult parse(const std::string& raw_output, double peak_ram_mb);
};

} // namespace macbenchforge
