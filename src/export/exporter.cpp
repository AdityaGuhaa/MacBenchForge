#include "exporter.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace macbenchforge {

namespace {

std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string escape_csv(const std::string& s) {
    if (s.find(',') == std::string::npos && s.find('\"') == std::string::npos && s.find('\n') == std::string::npos) {
        return s;
    }
    std::string result = "\"";
    for (char c : s) {
        if (c == '\"') {
            result += "\"\"";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

} // namespace

Exporter::Exporter(const std::string& export_dir) : export_dir_(export_dir) {}

std::string Exporter::export_json(const std::vector<RunSummary>& summaries) {
    fs::path dir(export_dir_);
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    std::string filename = "macbenchforge_export_" + get_timestamp() + ".json";
    fs::path file_path = dir / filename;

    json j = json::array();
    for (const auto& s : summaries) {
        j.push_back({
            {"run_id", s.run_id},
            {"model_name", s.model_name},
            {"model_path", s.model_path},
            {"quant_label", s.quant_label},
            {"hf_repo_id", s.hf_repo_id},
            {"prompt_type", s.prompt_type},
            {"preset_name", s.preset_name},
            {"status", s.status},
            {"started_at", s.started_at},
            {"tokens_per_second", s.tokens_per_second},
            {"prompt_tokens_per_sec", s.prompt_tokens_per_sec},
            {"gen_tokens_per_sec", s.gen_tokens_per_sec},
            {"time_to_first_token_ms", s.time_to_first_token_ms},
            {"ram_usage_mb", s.ram_usage_mb},
            {"chip_name", s.chip_name},
            {"memory_label", s.memory_label},
            {"gpu_cores", s.gpu_cores}
        });
    }

    std::ofstream ofs(file_path);
    ofs << j.dump(4);
    ofs.close();

    return file_path.string();
}

std::string Exporter::export_csv(const std::vector<RunSummary>& summaries) {
    fs::path dir(export_dir_);
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    }

    std::string filename = "macbenchforge_export_" + get_timestamp() + ".csv";
    fs::path file_path = dir / filename;

    std::ofstream ofs(file_path);
    
    // Header
    ofs << "run_id,model_name,model_path,quant_label,hf_repo_id,prompt_type,preset_name,status,started_at,tokens_per_second,prompt_tokens_per_sec,gen_tokens_per_sec,time_to_first_token_ms,ram_usage_mb,chip_name,memory_label,gpu_cores\n";

    // Rows
    for (const auto& s : summaries) {
        ofs << s.run_id << ","
            << escape_csv(s.model_name) << ","
            << escape_csv(s.model_path) << ","
            << escape_csv(s.quant_label) << ","
            << escape_csv(s.hf_repo_id) << ","
            << escape_csv(s.prompt_type) << ","
            << escape_csv(s.preset_name) << ","
            << escape_csv(s.status) << ","
            << escape_csv(s.started_at) << ","
            << s.tokens_per_second << ","
            << s.prompt_tokens_per_sec << ","
            << s.gen_tokens_per_sec << ","
            << s.time_to_first_token_ms << ","
            << s.ram_usage_mb << ","
            << escape_csv(s.chip_name) << ","
            << escape_csv(s.memory_label) << ","
            << s.gpu_cores << "\n";
    }

    ofs.close();

    return file_path.string();
}

} // namespace macbenchforge
