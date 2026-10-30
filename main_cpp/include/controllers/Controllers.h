#pragma once

#include <crow.h>
#include <string>
#include "services/AuthService.h"
#include "services/BookService.h"
#include "services/LoanService.h"
#include "services/MemberService.h"
#include "repositories/StatsRepository.h"
#include "repositories/BookRepository.h"
#include "repositories/UserRepository.h"
#include "repositories/LoanRepository.h"

namespace library {

// JSON extraction helpers to prevent type conversion issues
inline std::string jsonString(const crow::json::rvalue& j, const char* key, const std::string& defVal = "") {
    if (j.has(key)) {
        return std::string(j[key].s());
    }
    return defVal;
}

inline int jsonInt(const crow::json::rvalue& j, const char* key, int defVal = 0) {
    if (j.has(key)) {
        return static_cast<int>(j[key].i());
    }
    return defVal;
}

inline int64_t jsonInt64(const crow::json::rvalue& j, const char* key, int64_t defVal = 0) {
    if (j.has(key)) {
        return static_cast<int64_t>(j[key].i());
    }
    return defVal;
}

inline double jsonDouble(const crow::json::rvalue& j, const char* key, double defVal = 0.0) {
    if (j.has(key)) {
        return j[key].d();
    }
    return defVal;
}

// Helpers
inline crow::response jsonResponse(int statusCode, crow::json::wvalue body) {
    crow::response res(statusCode, std::move(body));
    res.add_header("Content-Type", "application/json; charset=utf-8");
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Session-Token");
    return res;
}

inline crow::response errorResponse(int statusCode, const std::string& error) {
    crow::json::wvalue json;
    json["success"] = false;
    json["error"] = error;
    return jsonResponse(statusCode, std::move(json));
}

inline crow::response successResponse(const std::string& message = "Success", crow::json::wvalue data = crow::json::wvalue()) {
    crow::json::wvalue json;
    json["success"] = true;
    json["message"] = message;
    if (data.t() != crow::json::type::Null) {
        json["data"] = std::move(data);
    }
    return jsonResponse(200, std::move(json));
}

void registerAuthRoutes(crow::SimpleApp& app, AuthService& authService);
void registerBookRoutes(crow::SimpleApp& app, BookService& bookService, AuthService& authService);
void registerLoanRoutes(crow::SimpleApp& app, LoanService& loanService, AuthService& authService);
void registerMemberRoutes(crow::SimpleApp& app, MemberService& memberService, AuthService& authService);
void registerDashboardRoutes(crow::SimpleApp& app, StatsRepository& statsRepo, BookRepository& bookRepo, UserRepository& userRepo, LoanRepository& loanRepo, AuthService& authService);
void registerStaticRoutes(crow::SimpleApp& app, const std::string& publicDir);

} // namespace library
