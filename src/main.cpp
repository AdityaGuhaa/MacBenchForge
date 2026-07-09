#include "config/config.hpp"
#include "db/database.hpp"
#include "db/crud.hpp"
#include "hardware/hw_info.hpp"
#include "discovery/scanner.hpp"
#include "server/server.hpp"
#include "app/native_window.h"
#include <iostream>
#include <csignal>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;
using namespace macbenchforge;

Server* g_server = nullptr;

void signal_handler(int signal) {
    if (g_server) {
        std::cout << "\nCaught signal " << signal << ", shutting down server gracefully..." << std::endl;
        g_server->stop();
    }
}

// Called by the native window when the user closes it
extern "C" void on_window_close(void) {
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char** argv) {
    std::cout << "🔥 MacBenchForge v2.1.0" << std::endl;
    std::cout << "=======================" << std::endl;

    std::string config_path = "config.toml";
    if (!fs::exists(config_path)) {
        // Fallback: check if we are in an app bundle (Contents/MacOS)
        fs::path exe_path = fs::canonical(fs::path(argv[0]));
        fs::path bin_path = exe_path.parent_path();
        
        fs::path bundle_res_path = bin_path.parent_path() / "Resources" / "config.toml";
        if (fs::exists(bundle_res_path)) {
            config_path = bundle_res_path.string();
            fs::current_path(bundle_res_path.parent_path());
        } else {
            // Default fallback
            config_path = (bin_path / "config.toml").string();
            fs::current_path(bin_path);
        }
    }
    
    Config config;
    try {
        config = load_config(config_path);
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Loaded config from " << config_path << std::endl;

    // 2. Setup DB
    fs::path db_p(config.db_path);
    if (!fs::exists(db_p.parent_path())) {
        fs::create_directories(db_p.parent_path());
    }
    Database db(config.db_path);
    Crud crud(db);

    // 3. Hardware
    std::cout << "Detecting hardware..." << std::endl;
    auto hw = HardwareDetector::detect();
    int hw_id = crud.insert_hardware(hw);
    
    std::cout << "  Chip: " << hw.chip_name << std::endl;
    std::cout << "  CPU Cores: " << hw.total_cores << " (" << hw.perf_cores << "P/" << hw.efficiency_cores << "E)" << std::endl;
    std::cout << "  GPU Cores: " << hw.gpu_cores << std::endl;
    std::cout << "  Memory: " << hw.memory_label << std::endl;

    // 4. Initial Scan
    std::cout << "Scanning for models in " << config.scan_dirs.size() << " directorie(s)..." << std::endl;
    Scanner scanner;
    auto models = scanner.scan(config.scan_dirs, config.recursive_scan);
    int added = 0;
    for (const auto& m : models) {
        crud.insert_model(m);
        added++;
    }
    std::cout << "Found " << added << " model(s)." << std::endl;

    // 4b. Cleanup: deactivate models whose files no longer exist on disk or are too small
    auto db_models = crud.get_all_models();
    int deactivated = 0;
    for (const auto& m : db_models) {
        bool should_deactivate = false;
        if (!fs::exists(m.path)) {
            should_deactivate = true;
        } else if (fs::file_size(m.path) < 100 * 1024 * 1024) {
            should_deactivate = true;
        } else if (fs::path(m.path).filename().string().find("mmproj-") == 0) {
            should_deactivate = true;
        }

        if (should_deactivate) {
            crud.deactivate_model(m.id);
            deactivated++;
        }
    }
    if (deactivated > 0) {
        std::cout << "Deactivated " << deactivated << " stale or small model(s)." << std::endl;
    }

    // 5. Start Server
    Server server(config, db, crud, hw_id);
    g_server = &server;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    if (config.open_browser) {
        // Native window mode: run the HTTP server on a background thread
        // and the macOS GUI on the main thread (required by AppKit).
        std::thread server_thread([&server]() {
            server.start(); // Blocks until server.stop() is called
        });
        server_thread.detach();

        std::cout << "Launching native window..." << std::endl;
        launch_native_window(config.port, on_window_close); // Blocks until window closes
    } else {
        // Headless mode: run the server on the main thread (no GUI)
        std::cout << "Running in headless mode (no native window)." << std::endl;
        server.start(); // Blocks until stopped
    }

    std::cout << "Shutdown complete." << std::endl;
    return 0;
}
