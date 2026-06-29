#include "config/config.hpp"
#include "db/database.hpp"
#include "db/crud.hpp"
#include "hardware/hw_info.hpp"
#include "discovery/scanner.hpp"
#include "server/server.hpp"
#include <iostream>
#include <csignal>
#include <filesystem>

namespace fs = std::filesystem;
using namespace macbenchforge;

Server* g_server = nullptr;

void signal_handler(int signal) {
    if (g_server) {
        std::cout << "\nCaught signal " << signal << ", shutting down server gracefully..." << std::endl;
        g_server->stop();
    }
}

int main(int argc, char** argv) {
    std::cout << "🔥 MacBenchForge v1.1.0" << std::endl;
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

    // 5. Start Server
    Server server(config, db, crud, hw_id);
    g_server = &server;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    server.start(); // Blocks until stopped

    std::cout << "Shutdown complete." << std::endl;
    return 0;
}
