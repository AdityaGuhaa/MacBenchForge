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
            continue;
        }

        try {
            if (recursive) {
                std::error_code ec;
                auto it = fs::recursive_directory_iterator(p, fs::directory_options::skip_permission_denied, ec);
                auto end = fs::recursive_directory_iterator();
                while (it != end) {
                    if (ec) {
                        it.increment(ec);
                        continue;
                    }
                    try {
                        const auto& entry = *it;
                        if (entry.is_regular_file() && entry.path().extension() == ".gguf") {
                            uint64_t size = fs::file_size(entry);
                            // Ignore files < 100 MB (e.g., vocabularies)
                            if (size >= 100 * 1024 * 1024) {
                                Model m;
                                m.id = 0;
                                m.name = entry.path().stem().string();
                                m.path = entry.path().string();
                                m.file_size = size;
                                m.size_label = format_size(m.file_size);
                                m.quant_label = extract_quant(entry.path().filename().string());
                                m.is_active = true;
                                models.push_back(m);
                            }
                        }
                    } catch (...) {}
                    it.increment(ec);
                }
            } else {
                std::error_code ec;
                for (const auto& entry : fs::directory_iterator(p, fs::directory_options::skip_permission_denied, ec)) {
                    if (ec) continue;
                    try {
                        if (entry.is_regular_file() && entry.path().extension() == ".gguf") {
                            uint64_t size = fs::file_size(entry);
                            if (size >= 100 * 1024 * 1024) {
                                Model m;
                                m.id = 0;
                                m.name = entry.path().stem().string();
                                m.path = entry.path().string();
                                m.file_size = size;
                                m.size_label = format_size(m.file_size);
                                m.quant_label = extract_quant(entry.path().filename().string());
                                m.is_active = true;
                                models.push_back(m);
                            }
                        }
                    } catch (...) {}
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
