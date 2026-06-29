#pragma once

#include <httplib.h>
#include "config/config.hpp"
#include "db/database.hpp"
#include "db/crud.hpp"

namespace macbenchforge {

// Setup all API routes on the provided server.
void setup_routes(httplib::Server& svr, Config& config, Database& db, Crud& crud, int hardware_id);

} // namespace macbenchforge
