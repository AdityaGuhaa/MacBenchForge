#pragma once

#include "database.hpp"
#include "models.hpp"
#include <vector>
#include <optional>

namespace macbenchforge {

class Crud {
public:
    explicit Crud(Database& db);

    // ── Hardware ──────────────────────────────────────────────────────────────
    // Insert a hardware snapshot. Returns the new row id.
    int  insert_hardware(const HardwareInfo& hw);

    // Returns the most recently recorded hardware snapshot.
    std::optional<HardwareInfo> get_latest_hardware();

    // ── Models ────────────────────────────────────────────────────────────────
    // Insert a new model. Returns the new row id.
    // Uses INSERT OR IGNORE -- safe to call repeatedly for same path.
    int  insert_model(const Model& model);

    // Returns all active models.
    std::vector<Model> get_all_models();

    // Returns a single model by id.
    std::optional<Model> get_model_by_id(int id);

    // Returns a model by its file path.
    std::optional<Model> get_model_by_path(const std::string& path);

    // Marks a model as inactive (file moved/deleted).
    // Historical run data is preserved.
    void deactivate_model(int id);

    // ── Download Jobs ─────────────────────────────────────────────────────────
    // Insert a new download job. Returns the new row id.
    int  insert_download_job(const DownloadJob& job);

    // Update bytes downloaded so far (called frequently during download).
    void update_download_progress(int job_id,
                                  int64_t downloaded_bytes,
                                  int64_t total_bytes);

    // Update job status ("downloading", "done", "failed").
    void update_download_status(int                job_id,
                                const std::string& status,
                                const std::string& error = "");

    // Set started_at to current UTC time.
    void mark_download_started(int job_id);

    // Set finished_at to current UTC time.
    void mark_download_finished(int job_id);

    // Returns a download job by id.
    std::optional<DownloadJob> get_download_job(int job_id);

    // Returns all download jobs (newest first).
    std::vector<DownloadJob> get_all_download_jobs();

    // Returns active download job for a given dest_path (if any).
    std::optional<DownloadJob> get_download_job_by_path(
        const std::string& dest_path);

    // ── BenchmarkConfig ───────────────────────────────────────────────────────
    // Insert a benchmark config. Returns the new row id.
    int insert_config(const BenchmarkConfig& config);

    // ── BenchmarkRun ──────────────────────────────────────────────────────────
    // Insert a new run record with status "pending". Returns the new row id.
    int  insert_run(int model_id, int config_id, int hardware_id);

    // Update run status.
    void update_run_status(int                run_id,
                           const std::string& status,
                           const std::string& error_message = "");

    // Set started_at and status = "running".
    void mark_run_started(int run_id);

    // Set finished_at.
    void mark_run_finished(int run_id);

    // ── BenchmarkResult ───────────────────────────────────────────────────────
    // Insert measured metrics for a completed run.
    void insert_result(const BenchmarkResult& result);

    // ── Queries ───────────────────────────────────────────────────────────────
    // Returns all RunSummary rows (joined view), newest first.
    std::vector<RunSummary> get_all_run_summaries();

    // Returns RunSummary rows filtered by a list of run ids.
    std::vector<RunSummary> get_run_summaries_by_ids(
        const std::vector<int>& run_ids);

    // Returns all runs for a specific model.
    std::vector<RunSummary> get_runs_for_model(int model_id);

    // Deletes a run and its result. Model row is preserved.
    void delete_run(int run_id);

private:
    SQLite::Database& db_;

    // Helpers: map query rows to structs.
    HardwareInfo row_to_hardware(SQLite::Statement& stmt);
    Model        row_to_model(SQLite::Statement& stmt);
    DownloadJob  row_to_download_job(SQLite::Statement& stmt);
    RunSummary   row_to_summary(SQLite::Statement& stmt);
};

} // namespace macbenchforge