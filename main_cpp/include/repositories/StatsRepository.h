#pragma once

#include <string>
#include <vector>
#include <crow/json.h>
#include "core/DatabaseManager.h"
#include "models/ActivityLog.h"

namespace library {

class BookRepository;
class UserRepository;
class LoanRepository;

class StatsRepository {
public:
    explicit StatsRepository(DatabaseManager& db);

    void logActivity(int64_t userId, const std::string& action, const std::string& details);
    std::vector<ActivityLog> getRecentActivities(int limit = 10);
    crow::json::wvalue getDashboardSummary(BookRepository& bookRepo, UserRepository& userRepo, LoanRepository& loanRepo);

private:
    DatabaseManager& m_db;
};

} // namespace library
