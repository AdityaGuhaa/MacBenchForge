#pragma once

#include "db/models.hpp"
#include <string>
#include <vector>

namespace macbenchforge {

// ── Exporter ──────────────────────────────────────────────────────────────────
// Exports benchmark summaries to files.
class Exporter {
public:
    explicit Exporter(const std::string& export_dir);

    // Export selected runs to a JSON file. Returns full path.
    std::string export_json(const std::vector<RunSummary>& summaries);

    // Export selected runs to a CSV file. Returns full path.
    std::string export_csv(const std::vector<RunSummary>& summaries);

private:
    std::string export_dir_;
};

} // namespace macbenchforge
