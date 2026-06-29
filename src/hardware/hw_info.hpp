#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstdint>

#include "db/models.hpp"

namespace macbenchforge {

// ── HardwareDetector ──────────────────────────────────────────────────────────
// Detects Apple Silicon hardware specs at startup via sysctl and IOKit.
// All methods are static -- no instance needed.
struct HardwareDetector {
    // Detect full hardware info. Throws std::runtime_error if critical
    // detection fails (e.g. not running on Apple Silicon).
    static HardwareInfo detect();

    // ── Individual Detectors ─────────────────────────────────────────────────
    // Returns chip name e.g. "Apple M1 Pro"
    static std::string chip_name();

    // Returns number of performance CPU cores (e.g. 8 on M1 Pro)
    static int perf_cores();

    // Returns number of efficiency CPU cores (e.g. 2 on M1 Pro)
    static int efficiency_cores();

    // Returns total logical CPU cores
    static int total_cores();

    // Returns Metal GPU core count via IOKit AGXAccelerator (e.g. 16)
    // Returns 0 if IOKit query fails
    static int gpu_cores();

    // Returns total unified memory in bytes
    static uint64_t memory_bytes();

    // Returns human-readable memory string e.g. "16 GB"
    static std::string memory_label(uint64_t bytes);

    // Returns macOS version string e.g. "14.5"
    static std::string macos_version();
};

// ── MemoryPoller ──────────────────────────────────────────────────────────────
// Polls unified memory usage of the current process on a background thread
// during a benchmark run using mach task info.
// Call start() before launching llama-bench, stop() after it exits.
class MemoryPoller {
public:
    explicit MemoryPoller(int poll_interval_ms = 250);
    ~MemoryPoller();

    // Non-copyable
    MemoryPoller(const MemoryPoller&)            = delete;
    MemoryPoller& operator=(const MemoryPoller&) = delete;

    // Start polling on a background thread.
    void start();

    // Stop polling and join the background thread.
    void stop();

    // Returns peak resident memory observed since start() in MB.
    double peak_ram_mb() const;

    // Reset peak value (call before reuse).
    void reset();

private:
    int              poll_interval_ms_;
    std::thread      thread_;
    std::atomic_bool running_;
    mutable std::mutex mutex_;
    double           peak_ram_mb_;

    void poll_loop();

    // Returns current process resident memory in bytes via mach task info.
    static uint64_t current_resident_bytes();
};

} // namespace macbenchforge