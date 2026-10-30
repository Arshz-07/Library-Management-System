#include "repositories/StatsRepository.h"
#include "repositories/BookRepository.h"
#include "repositories/UserRepository.h"
#include "repositories/LoanRepository.h"

namespace library {

StatsRepository::StatsRepository(DatabaseManager& db) : m_db(db) {}

void StatsRepository::logActivity(int64_t userId, const std::string& action, const std::string& details) {
    const std::string sql = "INSERT INTO activity_logs (user_id, action, details) VALUES (?, ?, ?);";
    auto stmt = m_db.prepare(sql);
    if (userId > 0) {
        stmt.bindInt64(1, userId);
    } else {
        stmt.bindNull(1);
    }
    stmt.bindString(2, action);
    stmt.bindString(3, details);
    stmt.step();
}

std::vector<ActivityLog> StatsRepository::getRecentActivities(int limit) {
    const std::string sql = R"(
        SELECT a.id, a.user_id, COALESCE(u.name, 'System'), a.action, a.details, a.created_at
        FROM activity_logs a
        LEFT JOIN users u ON a.user_id = u.id
        ORDER BY a.id DESC LIMIT ?;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt(1, limit > 0 ? limit : 10);

    std::vector<ActivityLog> logs;
    while (stmt.step()) {
        ActivityLog log;
        log.id = stmt.getInt64(0);
        log.userId = stmt.getInt64(1);
        log.userName = stmt.getString(2);
        log.action = stmt.getString(3);
        log.details = stmt.getString(4);
        log.createdAt = stmt.getString(5);
        logs.push_back(log);
    }
    return logs;
}

crow::json::wvalue StatsRepository::getDashboardSummary(BookRepository& bookRepo, UserRepository& userRepo, LoanRepository& loanRepo) {
    crow::json::wvalue out;
    out["totalTitles"] = bookRepo.countTotalTitles();
    out["totalCopies"] = bookRepo.countTotalCopies();
    out["availableCopies"] = bookRepo.countAvailableCopies();
    out["borrowedCopies"] = bookRepo.countTotalCopies() - bookRepo.countAvailableCopies();
    out["totalMembers"] = userRepo.countMembers();
    out["activeLoans"] = loanRepo.countActiveLoans();
    out["overdueLoans"] = loanRepo.countOverdueLoans();
    out["returnedLoans"] = loanRepo.countReturnedLoans();
    out["totalPendingFines"] = loanRepo.getTotalPendingFines();
    out["totalCollectedFines"] = loanRepo.getTotalCollectedFines();

    auto activities = getRecentActivities(8);
    std::vector<crow::json::wvalue> actJson;
    for (const auto& a : activities) {
        actJson.push_back(a.toJson());
    }
    out["recentActivities"] = std::move(actJson);

    return out;
}

} // namespace library
