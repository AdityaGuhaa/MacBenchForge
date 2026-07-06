#include "server.hpp"
#include "routes.hpp"
#include <iostream>
#include <cstdlib>

namespace macbenchforge {

Server::Server(Config& config, Database& db, Crud& crud, int hardware_id)
    : config_(config), db_(db), crud_(crud), hardware_id_(hardware_id) {}

void Server::start() {
    setup_routes(svr_, config_, db_, crud_, hardware_id_);

    // Serve static files from frontend directory
    if (!svr_.set_mount_point("/", "frontend")) {
        std::cerr << "Warning: Could not mount 'frontend' directory. Web UI will not be available." << std::endl;
    }

    // Default headers
    svr_.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    if (config_.open_browser) {
        std::string cmd = "open http://localhost:" + std::to_string(config_.port);
        int ret = system(cmd.c_str());
        (void)ret; // ignore
    }

    std::cout << "Starting Server on http://localhost:" << config_.port << std::endl;
    svr_.listen("127.0.0.1", config_.port);
}

void Server::stop() {
    svr_.stop();
}

} // namespace macbenchforge
