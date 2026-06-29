#include "metrics.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace macbenchforge {

BenchmarkResult MetricsParser::parse(const std::string& raw_output, double peak_ram_mb) {
    BenchmarkResult res;
    res.id = 0;
    res.run_id = 0;
    res.tokens_per_second = 0.0;
    res.prompt_tokens_per_sec = 0.0;
    res.gen_tokens_per_sec = 0.0;
    res.time_to_first_token_ms = 0.0;
    res.ram_usage_mb = peak_ram_mb;
    res.raw_output = raw_output;

    if (raw_output.empty()) return res;

    try {
        json j = json::parse(raw_output);
        
        // llama-bench -o json outputs an array of objects
        if (j.is_array()) {
            double total_pp = 0.0;
            double total_tg = 0.0;
            int count_pp = 0;
            int count_tg = 0;
            int n_prompt = 0;

            for (const auto& item : j) {
                int np = item.value("n_prompt", 0);
                int ng = item.value("n_gen", 0);
                double ts = item.value("avg_ts", 0.0);
                
                if (np > 0 && ng == 0) {
                    total_pp += ts;
                    count_pp++;
                    n_prompt = np;
                } else if (ng > 0 && np == 0) {
                    total_tg += ts;
                    count_tg++;
                } else if (np > 0 && ng > 0) {
                    // Sometimes it outputs both in one run depending on options
                    total_pp += item.value("avg_ts", 0.0); // Might be mixed, but tg is usually separate
                    count_pp++;
                    n_prompt = np;
                }
            }

            if (count_pp > 0) {
                res.prompt_tokens_per_sec = total_pp / count_pp;
            }
            if (count_tg > 0) {
                res.gen_tokens_per_sec = total_tg / count_tg;
                res.tokens_per_second = res.gen_tokens_per_sec; // primary metric
            }

            // Estimate TTFT: time to process prompt
            if (res.prompt_tokens_per_sec > 0.0 && n_prompt > 0) {
                res.time_to_first_token_ms = (n_prompt / res.prompt_tokens_per_sec) * 1000.0;
            }
        }
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error in MetricsParser: " << e.what() << std::endl;
        std::cerr << "Raw output was: " << raw_output << std::endl;
    }

    return res;
}

} // namespace macbenchforge
