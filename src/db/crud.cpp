#include "crud.hpp"
#include <stdexcept>

namespace macbenchforge {

Crud::Crud(Database& db)
    : db_(db.connection())
{}

// ── Hardware ──────────────────────────────────────────────────────────────────

int Crud::insert_hardware(const HardwareInfo& hw) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO hardware_snapshots
            (chip_name, perf_cores, efficiency_cores, total_cores,
             gpu_cores, memory_bytes, memory_label, macos_version, cpu_arch, vram_limit_bytes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    stmt.bind(1, hw.chip_name);
    stmt.bind(2, hw.perf_cores);
    stmt.bind(3, hw.efficiency_cores);
    stmt.bind(4, hw.total_cores);
    stmt.bind(5, hw.gpu_cores);
    stmt.bind(6, (int64_t)hw.memory_bytes);
    stmt.bind(7, hw.memory_label);
    stmt.bind(8, hw.macos_version);
    stmt.bind(9, hw.cpu_arch);
    stmt.bind(10, (int64_t)hw.vram_limit_bytes);
    stmt.exec();
    return (int)db_.getLastInsertRowid();
}

std::optional<HardwareInfo> Crud::get_latest_hardware() {
    SQLite::Statement stmt(db_, R"(
        SELECT chip_name, perf_cores, efficiency_cores, total_cores,
               gpu_cores, memory_bytes, memory_label, macos_version, cpu_arch, vram_limit_bytes
        FROM hardware_snapshots
        ORDER BY id DESC LIMIT 1
    )");
    if (stmt.executeStep())
        return row_to_hardware(stmt);
    return std::nullopt;
}

// ── Models ────────────────────────────────────────────────────────────────────

int Crud::insert_model(const Model& model) {
    SQLite::Statement stmt(db_, R"(
        INSERT OR IGNORE INTO models
            (name, path, size_label, file_size,
             is_active, hf_repo_id, hf_filename, quant_label)
        VALUES (?, ?, ?, ?, 1, ?, ?, ?)
    )");
    stmt.bind(1, model.name);
    stmt.bind(2, model.path);
    stmt.bind(3, model.size_label);
    stmt.bind(4, (int64_t)model.file_size);
    stmt.bind(5, model.hf_repo_id);
    stmt.bind(6, model.hf_filename);
    stmt.bind(7, model.quant_label);
    stmt.exec();
    return (int)db_.getLastInsertRowid();
}

std::vector<Model> Crud::get_all_models() {
    SQLite::Statement stmt(db_, R"(
        SELECT id, name, path, size_label, file_size,
               added_at, is_active, hf_repo_id, hf_filename, quant_label
        FROM models
        WHERE is_active = 1
        ORDER BY name ASC
    )");
    std::vector<Model> models;
    while (stmt.executeStep())
        models.push_back(row_to_model(stmt));
    return models;
}

std::optional<Model> Crud::get_model_by_id(int id) {
    SQLite::Statement stmt(db_, R"(
        SELECT id, name, path, size_label, file_size,
               added_at, is_active, hf_repo_id, hf_filename, quant_label
        FROM models WHERE id = ?
    )");
    stmt.bind(1, id);
    if (stmt.executeStep())
        return row_to_model(stmt);
    return std::nullopt;
}

std::optional<Model> Crud::get_model_by_path(const std::string& path) {
    SQLite::Statement stmt(db_, R"(
        SELECT id, name, path, size_label, file_size,
               added_at, is_active, hf_repo_id, hf_filename, quant_label
        FROM models WHERE path = ?
    )");
    stmt.bind(1, path);
    if (stmt.executeStep())
        return row_to_model(stmt);
    return std::nullopt;
}

void Crud::deactivate_model(int id) {
    SQLite::Statement stmt(db_,
        "UPDATE models SET is_active = 0 WHERE id = ?");
    stmt.bind(1, id);
    stmt.exec();
}

// ── Download Jobs ─────────────────────────────────────────────────────────────

int Crud::insert_download_job(const DownloadJob& job) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO download_jobs
            (repo_id, filename, dest_path, status, total_bytes)
        VALUES (?, ?, ?, 'pending', ?)
    )");
    stmt.bind(1, job.repo_id);
    stmt.bind(2, job.filename);
    stmt.bind(3, job.dest_path);
    stmt.bind(4, (int64_t)job.total_bytes);
    stmt.exec();
    return (int)db_.getLastInsertRowid();
}

void Crud::update_download_progress(int     job_id,
                                     int64_t downloaded_bytes,
                                     int64_t total_bytes) {
    SQLite::Statement stmt(db_, R"(
        UPDATE download_jobs
        SET downloaded_bytes = ?, total_bytes = ?
        WHERE id = ?
    )");
    stmt.bind(1, downloaded_bytes);
    stmt.bind(2, total_bytes);
    stmt.bind(3, job_id);
    stmt.exec();
}

void Crud::update_download_status(int                job_id,
                                   const std::string& status,
                                   const std::string& error) {
    SQLite::Statement stmt(db_, R"(
        UPDATE download_jobs
        SET status = ?, error = ?
        WHERE id = ?
    )");
    stmt.bind(1, status);
    stmt.bind(2, error);
    stmt.bind(3, job_id);
    stmt.exec();
}

void Crud::mark_download_started(int job_id) {
    SQLite::Statement stmt(db_, R"(
        UPDATE download_jobs
        SET started_at = datetime('now'), status = 'downloading'
        WHERE id = ?
    )");
    stmt.bind(1, job_id);
    stmt.exec();
}

void Crud::mark_download_finished(int job_id) {
    SQLite::Statement stmt(db_, R"(
        UPDATE download_jobs
        SET finished_at = datetime('now')
        WHERE id = ?
    )");
    stmt.bind(1, job_id);
    stmt.exec();
}

std::optional<DownloadJob> Crud::get_download_job(int job_id) {
    SQLite::Statement stmt(db_, R"(
        SELECT id, repo_id, filename, dest_path, status,
               total_bytes, downloaded_bytes, started_at, finished_at, error
        FROM download_jobs WHERE id = ?
    )");
    stmt.bind(1, job_id);
    if (stmt.executeStep())
        return row_to_download_job(stmt);
    return std::nullopt;
}

std::vector<DownloadJob> Crud::get_all_download_jobs() {
    SQLite::Statement stmt(db_, R"(
        SELECT id, repo_id, filename, dest_path, status,
               total_bytes, downloaded_bytes, started_at, finished_at, error
        FROM download_jobs
        ORDER BY id DESC
    )");
    std::vector<DownloadJob> jobs;
    while (stmt.executeStep())
        jobs.push_back(row_to_download_job(stmt));
    return jobs;
}

std::optional<DownloadJob> Crud::get_download_job_by_path(
    const std::string& dest_path)
{
    SQLite::Statement stmt(db_, R"(
        SELECT id, repo_id, filename, dest_path, status,
               total_bytes, downloaded_bytes, started_at, finished_at, error
        FROM download_jobs
        WHERE dest_path = ?
        ORDER BY id DESC LIMIT 1
    )");
    stmt.bind(1, dest_path);
    if (stmt.executeStep())
        return row_to_download_job(stmt);
    return std::nullopt;
}

// ── BenchmarkConfig ───────────────────────────────────────────────────────────

int Crud::insert_config(const BenchmarkConfig& cfg) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO benchmark_configs
            (preset_name, prompt_type, prompt_tokens,
             generation_tokens, repetitions, threads, gpu_layers)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    stmt.bind(1, cfg.preset_name);
    stmt.bind(2, cfg.prompt_type);
    stmt.bind(3, cfg.prompt_tokens);
    stmt.bind(4, cfg.generation_tokens);
    stmt.bind(5, cfg.repetitions);
    stmt.bind(6, cfg.threads);
    stmt.bind(7, cfg.gpu_layers);
    stmt.exec();
    return (int)db_.getLastInsertRowid();
}

// ── BenchmarkRun ──────────────────────────────────────────────────────────────

int Crud::insert_run(int model_id, int config_id, int hardware_id) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO benchmark_runs
            (model_id, config_id, hardware_id, status)
        VALUES (?, ?, ?, 'pending')
    )");
    stmt.bind(1, model_id);
    stmt.bind(2, config_id);
    stmt.bind(3, hardware_id);
    stmt.exec();
    return (int)db_.getLastInsertRowid();
}

void Crud::update_run_status(int                run_id,
                              const std::string& status,
                              const std::string& error_message) {
    SQLite::Statement stmt(db_, R"(
        UPDATE benchmark_runs
        SET status = ?, error_message = ?
        WHERE id = ?
    )");
    stmt.bind(1, status);
    stmt.bind(2, error_message);
    stmt.bind(3, run_id);
    stmt.exec();
}

void Crud::mark_run_started(int run_id) {
    SQLite::Statement stmt(db_, R"(
        UPDATE benchmark_runs
        SET started_at = datetime('now'), status = 'running'
        WHERE id = ?
    )");
    stmt.bind(1, run_id);
    stmt.exec();
}

void Crud::mark_run_finished(int run_id) {
    SQLite::Statement stmt(db_, R"(
        UPDATE benchmark_runs
        SET finished_at = datetime('now')
        WHERE id = ?
    )");
    stmt.bind(1, run_id);
    stmt.exec();
}

// ── BenchmarkResult ───────────────────────────────────────────────────────────

void Crud::insert_result(const BenchmarkResult& r) {
    SQLite::Statement stmt(db_, R"(
        INSERT INTO benchmark_results
            (run_id, tokens_per_second, prompt_tokens_per_sec,
             gen_tokens_per_sec, time_to_first_token_ms,
             ram_usage_mb, raw_output)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");
    stmt.bind(1, r.run_id);
    stmt.bind(2, r.tokens_per_second);
    stmt.bind(3, r.prompt_tokens_per_sec);
    stmt.bind(4, r.gen_tokens_per_sec);
    stmt.bind(5, r.time_to_first_token_ms);
    stmt.bind(6, r.ram_usage_mb);
    stmt.bind(7, r.raw_output);
    stmt.exec();
}

// ── Queries ───────────────────────────────────────────────────────────────────

std::vector<RunSummary> Crud::get_all_run_summaries() {
    SQLite::Statement stmt(db_, R"(
        SELECT
            r.id,
            m.name, m.path, m.quant_label, m.hf_repo_id,
            c.prompt_type, c.preset_name,
            r.status, r.started_at,
            res.tokens_per_second, res.prompt_tokens_per_sec,
            res.gen_tokens_per_sec, res.time_to_first_token_ms,
            res.ram_usage_mb,
            hw.chip_name, hw.memory_label, hw.gpu_cores
        FROM benchmark_runs r
        JOIN models              m   ON r.model_id   = m.id
        JOIN benchmark_configs   c   ON r.config_id  = c.id
        JOIN hardware_snapshots  hw  ON r.hardware_id = hw.id
        LEFT JOIN benchmark_results res ON res.run_id = r.id
        ORDER BY r.id DESC
    )");
    std::vector<RunSummary> summaries;
    while (stmt.executeStep())
        summaries.push_back(row_to_summary(stmt));
    return summaries;
}

std::vector<RunSummary> Crud::get_run_summaries_by_ids(
    const std::vector<int>& run_ids)
{
    std::vector<RunSummary> summaries;
    for (int rid : run_ids) {
        SQLite::Statement stmt(db_, R"(
            SELECT
                r.id,
                m.name, m.path, m.quant_label, m.hf_repo_id,
                c.prompt_type, c.preset_name,
                r.status, r.started_at,
                res.tokens_per_second, res.prompt_tokens_per_sec,
                res.gen_tokens_per_sec, res.time_to_first_token_ms,
                res.ram_usage_mb,
                hw.chip_name, hw.memory_label, hw.gpu_cores
            FROM benchmark_runs r
            JOIN models              m   ON r.model_id   = m.id
            JOIN benchmark_configs   c   ON r.config_id  = c.id
            JOIN hardware_snapshots  hw  ON r.hardware_id = hw.id
            LEFT JOIN benchmark_results res ON res.run_id = r.id
            WHERE r.id = ?
        )");
        stmt.bind(1, rid);
        if (stmt.executeStep())
            summaries.push_back(row_to_summary(stmt));
    }
    return summaries;
}

std::vector<RunSummary> Crud::get_runs_for_model(int model_id) {
    SQLite::Statement stmt(db_, R"(
        SELECT
            r.id,
            m.name, m.path, m.quant_label, m.hf_repo_id,
            c.prompt_type, c.preset_name,
            r.status, r.started_at,
            res.tokens_per_second, res.prompt_tokens_per_sec,
            res.gen_tokens_per_sec, res.time_to_first_token_ms,
            res.ram_usage_mb,
            hw.chip_name, hw.memory_label, hw.gpu_cores
        FROM benchmark_runs r
        JOIN models              m   ON r.model_id   = m.id
        JOIN benchmark_configs   c   ON r.config_id  = c.id
        JOIN hardware_snapshots  hw  ON r.hardware_id = hw.id
        LEFT JOIN benchmark_results res ON res.run_id = r.id
        WHERE r.model_id = ?
        ORDER BY r.id DESC
    )");
    stmt.bind(1, model_id);
    std::vector<RunSummary> summaries;
    while (stmt.executeStep())
        summaries.push_back(row_to_summary(stmt));
    return summaries;
}

void Crud::delete_run(int run_id) {
    SQLite::Statement del_result(db_,
        "DELETE FROM benchmark_results WHERE run_id = ?");
    del_result.bind(1, run_id);
    del_result.exec();

    SQLite::Statement del_run(db_,
        "DELETE FROM benchmark_runs WHERE id = ?");
    del_run.bind(1, run_id);
    del_run.exec();
}

// ── Row Helpers ───────────────────────────────────────────────────────────────

HardwareInfo Crud::row_to_hardware(SQLite::Statement& stmt) {
    HardwareInfo hw;
    hw.chip_name         = stmt.getColumn(0).getText();
    hw.perf_cores        = stmt.getColumn(1).getInt();
    hw.efficiency_cores  = stmt.getColumn(2).getInt();
    hw.total_cores       = stmt.getColumn(3).getInt();
    hw.gpu_cores         = stmt.getColumn(4).getInt();
    hw.memory_bytes      = (uint64_t)stmt.getColumn(5).getInt64();
    hw.memory_label      = stmt.getColumn(6).getText();
    hw.macos_version     = stmt.getColumn(7).getText();
    hw.cpu_arch          = stmt.getColumn(8).getText();
    hw.vram_limit_bytes  = (uint64_t)stmt.getColumn(9).getInt64();
    return hw;
}

Model Crud::row_to_model(SQLite::Statement& stmt) {
    Model m;
    m.id          = stmt.getColumn(0).getInt();
    m.name        = stmt.getColumn(1).getText();
    m.path        = stmt.getColumn(2).getText();
    m.size_label  = stmt.getColumn(3).getText();
    m.file_size   = stmt.getColumn(4).getInt64();
    m.added_at    = stmt.getColumn(5).getText();
    m.is_active   = stmt.getColumn(6).getInt() == 1;
    m.hf_repo_id  = stmt.getColumn(7).getText();
    m.hf_filename = stmt.getColumn(8).getText();
    m.quant_label = stmt.getColumn(9).getText();
    return m;
}

DownloadJob Crud::row_to_download_job(SQLite::Statement& stmt) {
    DownloadJob j;
    j.id               = stmt.getColumn(0).getInt();
    j.repo_id          = stmt.getColumn(1).getText();
    j.filename         = stmt.getColumn(2).getText();
    j.dest_path        = stmt.getColumn(3).getText();
    j.status           = stmt.getColumn(4).getText();
    j.total_bytes      = stmt.getColumn(5).getInt64();
    j.downloaded_bytes = stmt.getColumn(6).getInt64();
    j.started_at       = stmt.getColumn(7).isNull() ? "" : stmt.getColumn(7).getText();
    j.finished_at      = stmt.getColumn(8).isNull() ? "" : stmt.getColumn(8).getText();
    j.error            = stmt.getColumn(9).getText();
    return j;
}

RunSummary Crud::row_to_summary(SQLite::Statement& stmt) {
    RunSummary s;
    s.run_id                 = stmt.getColumn(0).getInt();
    s.model_name             = stmt.getColumn(1).getText();
    s.model_path             = stmt.getColumn(2).getText();
    s.quant_label            = stmt.getColumn(3).getText();
    s.hf_repo_id             = stmt.getColumn(4).getText();
    s.prompt_type            = stmt.getColumn(5).getText();
    s.preset_name            = stmt.getColumn(6).getText();
    s.status                 = stmt.getColumn(7).getText();
    s.started_at             = stmt.getColumn(8).getText();
    s.tokens_per_second      = stmt.getColumn(9).isNull()  ? 0.0 : stmt.getColumn(9).getDouble();
    s.prompt_tokens_per_sec  = stmt.getColumn(10).isNull() ? 0.0 : stmt.getColumn(10).getDouble();
    s.gen_tokens_per_sec     = stmt.getColumn(11).isNull() ? 0.0 : stmt.getColumn(11).getDouble();
    s.time_to_first_token_ms = stmt.getColumn(12).isNull() ? 0.0 : stmt.getColumn(12).getDouble();
    s.ram_usage_mb           = stmt.getColumn(13).isNull() ? 0.0 : stmt.getColumn(13).getDouble();
    s.chip_name              = stmt.getColumn(14).getText();
    s.memory_label           = stmt.getColumn(15).getText();
    s.gpu_cores              = stmt.getColumn(16).getInt();
    return s;
}

} // namespace macbenchforge