#include "controllers/Controllers.h"
#include <iostream>

namespace library {

void registerDashboardRoutes(crow::SimpleApp& app, 
                             StatsRepository& statsRepo, 
                             BookRepository& bookRepo, 
                             UserRepository& userRepo, 
                             LoanRepository& loanRepo, 
                             AuthService& authService) {
    // Dashboard overall statistics
    CROW_ROUTE(app, "/api/dashboard/stats").methods(crow::HTTPMethod::GET)
    ([&statsRepo, &bookRepo, &userRepo, &loanRepo]() {
        crow::json::wvalue stats = statsRepo.getDashboardSummary(bookRepo, userRepo, loanRepo);
        return successResponse("Dashboard metrics retrieved.", std::move(stats));
    });

    // Recent activity logs
    CROW_ROUTE(app, "/api/dashboard/activity").methods(crow::HTTPMethod::GET)
    ([&statsRepo, &authService](const crow::request& req) {
        auto user = authService.authenticateRequest(req);
        if (!user || !authService.hasRole(*user, {"admin", "librarian"})) {
            return errorResponse(403, "Access denied. Librarian or Admin privileges required.");
        }

        int limit = 15;
        if (req.url_params.get("limit")) {
            try { limit = std::stoi(req.url_params.get("limit")); } catch (...) {}
        }

        auto logs = statsRepo.getRecentActivities(limit);
        std::vector<crow::json::wvalue> list;
        for (const auto& a : logs) {
            list.push_back(a.toJson());
        }

        crow::json::wvalue out;
        out["activities"] = std::move(list);
        return successResponse("Activity logs retrieved.", std::move(out));
    });
}

} // namespace library
