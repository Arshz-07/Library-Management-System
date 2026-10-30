#include "controllers/Controllers.h"
#include <iostream>

namespace library {

void registerLoanRoutes(crow::SimpleApp& app, LoanService& loanService, AuthService& authService) {
    // Get all loans (Admin / Librarian)
    CROW_ROUTE(app, "/api/loans").methods(crow::HTTPMethod::GET)
    ([&loanService, &authService](const crow::request& req) {
        auto user = authService.authenticateRequest(req);
        if (!user || !authService.hasRole(*user, {"admin", "librarian"})) {
            return errorResponse(403, "Access denied. Librarian or Admin privileges required.");
        }

        std::string status = req.url_params.get("status") ? req.url_params.get("status") : "";
        int limit = 100;
        if (req.url_params.get("limit")) {
            try { limit = std::stoi(req.url_params.get("limit")); } catch (...) {}
        }

        int64_t userIdFilter = 0;
        if (req.url_params.get("userId")) {
            try { userIdFilter = std::stoll(req.url_params.get("userId")); } catch (...) {}
        }

        auto loans = loanService.getAllLoans(status, userIdFilter, limit);
        std::vector<crow::json::wvalue> list;
        for (const auto& l : loans) {
            list.push_back(l.toJson());
        }

        crow::json::wvalue out;
        out["total"] = static_cast<int>(loans.size());
        out["loans"] = std::move(list);
        return successResponse("Loans retrieved.", std::move(out));
    });

    // My loans (Current authenticated member)
    CROW_ROUTE(app, "/api/loans/my-loans").methods(crow::HTTPMethod::GET)
    ([&loanService, &authService](const crow::request& req) {
        auto user = authService.authenticateRequest(req);
        if (!user) {
            return errorResponse(401, "Please log in to view your loans.");
        }

        auto activeLoans = loanService.getUserActiveLoans(user->id);
        auto historyLoans = loanService.getUserLoanHistory(user->id);

        std::vector<crow::json::wvalue> activeList;
        for (const auto& l : activeLoans) {
            activeList.push_back(l.toJson());
        }

        std::vector<crow::json::wvalue> historyList;
        for (const auto& l : historyLoans) {
            historyList.push_back(l.toJson());
        }

        crow::json::wvalue out;
        out["user"] = user->toJson();
        out["activeLoans"] = std::move(activeList);
        out["history"] = std::move(historyList);
        return successResponse("Member loans retrieved.", std::move(out));
    });

    // Issue a loan
    CROW_ROUTE(app, "/api/loans/issue").methods(crow::HTTPMethod::POST)
    ([&loanService, &authService](const crow::request& req) {
        auto caller = authService.authenticateRequest(req);
        if (!caller) {
            return errorResponse(401, "Authentication required.");
        }

        auto json = crow::json::load(req.body);
        if (!json) {
            return errorResponse(400, "Invalid JSON payload.");
        }

        if (!json.has("bookId")) {
            return errorResponse(400, "Field 'bookId' is required.");
        }
        int64_t bookId = jsonInt64(json, "bookId");

        int64_t targetUserId = caller->id;
        if (json.has("userId") && authService.hasRole(*caller, {"admin", "librarian"})) {
            targetUserId = jsonInt64(json, "userId");
        }

        int loanDays = jsonInt(json, "loanDays", 14);
        std::string notes = jsonString(json, "notes");

        try {
            int64_t loanId = loanService.issueBook(bookId, targetUserId, loanDays, notes, caller->id);
            crow::json::wvalue out;
            out["loanId"] = loanId;
            return successResponse("Book issued successfully.", std::move(out));
        } catch (const std::exception& ex) {
            return errorResponse(400, ex.what());
        }
    });

    // Return a loan
    CROW_ROUTE(app, "/api/loans/return/<int>").methods(crow::HTTPMethod::POST)
    ([&loanService, &authService](const crow::request& req, int loanId) {
        auto caller = authService.authenticateRequest(req);
        if (!caller) {
            return errorResponse(401, "Authentication required.");
        }

        auto loan = loanService.getLoan(loanId);
        if (!loan) {
            return errorResponse(404, "Loan record not found.");
        }

        if (caller->id != loan->userId && !authService.hasRole(*caller, {"admin", "librarian"})) {
            return errorResponse(403, "You do not have permission to return this loan.");
        }

        double customFine = -1.0;
        auto json = crow::json::load(req.body);
        if (json && json.has("customFine") && authService.hasRole(*caller, {"admin", "librarian"})) {
            customFine = jsonDouble(json, "customFine", -1.0);
        }

        try {
            bool ok = loanService.returnBook(loanId, customFine, caller->id);
            if (!ok) return errorResponse(400, "Could not process book return.");
            return successResponse("Book returned successfully.");
        } catch (const std::exception& ex) {
            return errorResponse(400, ex.what());
        }
    });

    // Pay fine
    CROW_ROUTE(app, "/api/loans/pay-fine/<int>").methods(crow::HTTPMethod::POST)
    ([&loanService, &authService](const crow::request& req, int loanId) {
        auto caller = authService.authenticateRequest(req);
        if (!caller) {
            return errorResponse(401, "Authentication required.");
        }

        auto loan = loanService.getLoan(loanId);
        if (!loan) {
            return errorResponse(404, "Loan record not found.");
        }

        if (caller->id != loan->userId && !authService.hasRole(*caller, {"admin", "librarian"})) {
            return errorResponse(403, "Permission denied.");
        }

        bool ok = loanService.payFine(loanId, caller->id);
        if (!ok) return errorResponse(400, "Failed to settle fine.");
        return successResponse("Fine paid and settled successfully.");
    });
}

} // namespace library
