#pragma once

#include "config/config.hpp"
#include "db/database.hpp"
#include "db/crud.hpp"
#include <httplib.h>
#include <thread>

namespace macbenchforge {

// ── Server ────────────────────────────────────────────────────────────────────
// Web server for the dashboard.
class Server {
public:
    Server(Config& config, Database& db, Crud& crud, int hardware_id);

    void start();
    void stop();

private:
    Config&      config_;
    Database&    db_;
    Crud&        crud_;
    int          hardware_id_;
    httplib::Server svr_;
};

} // namespace macbenchforge
