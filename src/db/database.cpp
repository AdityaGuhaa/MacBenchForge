#include "database.hpp"

#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

namespace macbenchforge {

Database::Database(const std::string& db_path)
    : db_(db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    // Ensure parent directory exists
    fs::path p(db_path);
    if (p.has_parent_path())
        fs::create_directories(p.parent_path());

    // WAL mode -- better concurrent read performance
    // Allows frontend to read results while a benchmark is writing
    db_.exec("PRAGMA journal_mode=WAL;");

    // Busy timeout -- wait up to 5 seconds if another thread holds a write lock
    // Prevents SQLITE_BUSY errors during concurrent benchmark/download operations
    db_.exec("PRAGMA busy_timeout=5000;");

    // Enforce foreign key constraints
    db_.exec("PRAGMA foreign_keys=ON;");

    create_tables();
}

SQLite::Database& Database::connection() {
    return db_;
}

void Database::create_tables() {
    create_hardware_table();       // no FK deps
    create_models_table();         // no FK deps
    create_download_jobs_table();  // no FK deps
    create_configs_table();        // no FK deps
    create_runs_table();           // FK -> models, configs, hardware
    create_results_table();        // FK -> runs
}

void Database::create_hardware_table() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS hardware_snapshots (
            id                INTEGER PRIMARY KEY AUTOINCREMENT,
            chip_name         TEXT    NOT NULL DEFAULT '',
            perf_cores        INTEGER NOT NULL DEFAULT 0,
            efficiency_cores  INTEGER NOT NULL DEFAULT 0,
            total_cores       INTEGER NOT NULL DEFAULT 0,
            gpu_cores         INTEGER NOT NULL DEFAULT 0,
            memory_bytes      INTEGER NOT NULL DEFAULT 0,
            memory_label      TEXT    NOT NULL DEFAULT '',
            macos_version     TEXT    NOT NULL DEFAULT '',
            cpu_arch          TEXT    NOT NULL DEFAULT 'arm64',
            vram_limit_bytes  INTEGER NOT NULL DEFAULT 0,
            recorded_at       TEXT    NOT NULL DEFAULT (datetime('now'))
        );
    )");
}

void Database::create_models_table() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS models (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            name          TEXT    NOT NULL,
            path          TEXT    NOT NULL UNIQUE,
            size_label    TEXT    NOT NULL DEFAULT '',
            file_size     INTEGER NOT NULL DEFAULT 0,
            added_at      TEXT    NOT NULL DEFAULT (datetime('now')),
            is_active     INTEGER NOT NULL DEFAULT 1,
            hf_repo_id    TEXT    NOT NULL DEFAULT '',
            hf_filename   TEXT    NOT NULL DEFAULT '',
            quant_label   TEXT    NOT NULL DEFAULT ''
        );
    )");
}

void Database::create_download_jobs_table() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS download_jobs (
            id                INTEGER PRIMARY KEY AUTOINCREMENT,
            repo_id           TEXT    NOT NULL,
            filename          TEXT    NOT NULL,
            dest_path         TEXT    NOT NULL,
            status            TEXT    NOT NULL DEFAULT 'pending',
            total_bytes       INTEGER NOT NULL DEFAULT -1,
            downloaded_bytes  INTEGER NOT NULL DEFAULT 0,
            started_at        TEXT,
            finished_at       TEXT,
            error             TEXT    NOT NULL DEFAULT ''
        );
    )");
}

void Database::create_configs_table() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS benchmark_configs (
            id                INTEGER PRIMARY KEY AUTOINCREMENT,
            preset_name       TEXT    NOT NULL DEFAULT 'custom',
            prompt_type       TEXT    NOT NULL DEFAULT 'custom',
            prompt_tokens     INTEGER NOT NULL,
            generation_tokens INTEGER NOT NULL,
            repetitions       INTEGER NOT NULL DEFAULT 1,
            threads           INTEGER NOT NULL DEFAULT 8,
            gpu_layers        INTEGER NOT NULL DEFAULT 99
        );
    )");
}

void Database::create_runs_table() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS benchmark_runs (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            model_id        INTEGER NOT NULL REFERENCES models(id),
            config_id       INTEGER NOT NULL REFERENCES benchmark_configs(id),
            hardware_id     INTEGER NOT NULL REFERENCES hardware_snapshots(id),
            status          TEXT    NOT NULL DEFAULT 'pending',
            started_at      TEXT,
            finished_at     TEXT,
            error_message   TEXT    NOT NULL DEFAULT ''
        );
    )");
}

void Database::create_results_table() {
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS benchmark_results (
            id                      INTEGER PRIMARY KEY AUTOINCREMENT,
            run_id                  INTEGER NOT NULL REFERENCES benchmark_runs(id),
            tokens_per_second       REAL    NOT NULL DEFAULT 0.0,
            prompt_tokens_per_sec   REAL    NOT NULL DEFAULT 0.0,
            gen_tokens_per_sec      REAL    NOT NULL DEFAULT 0.0,
            time_to_first_token_ms  REAL    NOT NULL DEFAULT 0.0,
            ram_usage_mb            REAL    NOT NULL DEFAULT 0.0,
            raw_output              TEXT    NOT NULL DEFAULT ''
        );
    )");
}

} // namespace macbenchforge
