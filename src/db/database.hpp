#pragma once

#include <string>
#include <SQLiteCpp/SQLiteCpp.h>

namespace macbenchforge {

// ── Database ──────────────────────────────────────────────────────────────────
// Manages the SQLite connection and schema creation.
// Uses WAL mode for concurrent read/write (frontend reads while benchmark writes).
class Database {
public:
    // Opens (or creates) the database at db_path.
    // Creates parent directories if needed.
    // Initialises schema on first run.
    explicit Database(const std::string& db_path);

    // Returns a reference to the underlying SQLiteCpp connection.
    SQLite::Database& connection();

private:
    SQLite::Database db_;

    // Creates all tables if they don't already exist.
    void create_tables();

    // Individual table creation methods.
    void create_hardware_table();
    void create_models_table();
    void create_download_jobs_table();
    void create_configs_table();
    void create_runs_table();
    void create_results_table();
};

} // namespace macbenchforge