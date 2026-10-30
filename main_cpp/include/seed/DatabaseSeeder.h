#pragma once

#include "core/DatabaseManager.h"

namespace library {

class DatabaseSeeder {
public:
    static void seedIfEmpty(DatabaseManager& db);
};

} // namespace library
