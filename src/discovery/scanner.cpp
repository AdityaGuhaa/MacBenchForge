#include "scanner.hpp"
#include <filesystem>
#include <regex>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

namespace macbenchforge {

namespace {

std::string format_size(uint64_t bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    if (bytes > 1024ULL * 1024 * 1024) {
        oss << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
    } else if (bytes > 1024ULL * 1024) {
        oss << (bytes / (1024.0 * 1024.0)) << " MB";
    } else {
        oss << bytes << " B";
    }
    return oss.str();
}

std::string extract_quant(const std::string& filename) {
    // Look for common quant patterns like Q4_K_M, Q8_0, IQ2_S, etc.
    std::regex quant_regex(R"(-([IQq0-9_]+)\.gguf$)");
    std::smatch match;
    if (std::regex_search(filename, match, quant_regex) && match.size() > 1) {
        std::string q = match[1].str();
        // Basic uppercase just in case
        for (char& c : q) c = std::toupper(c);
        return q;
    }
    return "Unknown";
}

} // namespace

std::vector<Model> Scanner::scan(const std::vector<std::string>& dirs, bool recursive) {
    std::vector<Model> models;

    for (const auto& dir : dirs) {
        fs::path p(dir);
        if (!fs::exists(p) || !fs::is_directory(p)) {
            std::cerr << "Scanner warning: directory not found or invalid: " << dir << std::endl;
            continue;
        }

        try {
            if (recursive) {
                for (const auto& entry : fs::recursive_directory_iterator(p, fs::directory_options::skip_permission_denied)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".gguf") {
                        Model m;
                        m.id = 0;
                        m.name = entry.path().stem().string();
                        m.path = entry.path().string();
                        m.file_size = fs::file_size(entry);
                        m.size_label = format_size(m.file_size);
                        m.quant_label = extract_quant(entry.path().filename().string());
                        m.is_active = true;
                        models.push_back(m);
                    }
                }
            } else {
                for (const auto& entry : fs::directory_iterator(p, fs::directory_options::skip_permission_denied)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".gguf") {
                        Model m;
                        m.id = 0;
                        m.name = entry.path().stem().string();
                        m.path = entry.path().string();
                        m.file_size = fs::file_size(entry);
                        m.size_label = format_size(m.file_size);
                        m.quant_label = extract_quant(entry.path().filename().string());
                        m.is_active = true;
                        models.push_back(m);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Scanner error in dir " << dir << ": " << e.what() << std::endl;
        }
    }

    return models;
}

std::optional<Model> Scanner::scan_file(const std::string& path) {
    fs::path p(path);
    if (!fs::exists(p) || !fs::is_regular_file(p)) return std::nullopt;
    
    Model m;
    m.id = 0;
    m.name = p.stem().string();
    m.path = p.string();
    m.file_size = fs::file_size(p);
    m.size_label = format_size(m.file_size);
    m.quant_label = extract_quant(p.filename().string());
    m.is_active = true;
    return m;
}

} // namespace macbenchforge
