#pragma once

#include "db/models.hpp"
#include <string>
#include <vector>

namespace macbenchforge {

// ── Scanner ───────────────────────────────────────────────────────────────────
// Scans directories for .gguf model files.
class Scanner {
public:
    Scanner() = default;

    // Scan the given directories for .gguf files.
    // If recursive is true, traverses subdirectories.
    // Returns a list of Models with size and quant_label populated.
    std::vector<Model> scan(const std::vector<std::string>& dirs, bool recursive);

    // Scan a specific file and return a Model if valid.
    static std::optional<Model> scan_file(const std::string& path);
};

} // namespace macbenchforge
