#include "controllers/Controllers.h"
#include <iostream>

namespace library {

void registerAuthRoutes(crow::SimpleApp& app, AuthService& authService) {
    // Register
    CROW_ROUTE(app, "/api/auth/register").methods(crow::HTTPMethod::POST)
    ([&authService](const crow::request& req) {
        auto json = crow::json::load(req.body);
        if (!json) {
            return errorResponse(400, "Invalid JSON payload.");
        }

        std::string name = jsonString(json, "name");
        std::string email = jsonString(json, "email");
        std::string password = jsonString(json, "password");
        std::string phone = jsonString(json, "phone");
        std::string role = jsonString(json, "role", "member");

        auto res = authService.registerUser(name, email, password, phone, role);
        if (!res.success) {
            return errorResponse(400, res.errorMessage);
        }

        crow::json::wvalue out;
        out["token"] = res.token;
        out["user"] = res.user->toJson();
        return successResponse("User registered successfully.", std::move(out));
    });

    // Login
    CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::POST)
    ([&authService](const crow::request& req) {
        auto json = crow::json::load(req.body);
        if (!json) {
            return errorResponse(400, "Invalid JSON payload.");
        }

        std::string identifier = jsonString(json, "identifier");
        if (identifier.empty()) {
            identifier = jsonString(json, "email");
        }
        std::string password = jsonString(json, "password");

        auto res = authService.login(identifier, password);
        if (!res.success) {
            return errorResponse(401, res.errorMessage);
        }

        crow::json::wvalue out;
        out["token"] = res.token;
        out["user"] = res.user->toJson();

        auto resp = successResponse("Login successful.", std::move(out));
        resp.add_header("Set-Cookie", "session_token=" + res.token + "; Path=/; HttpOnly; SameSite=Lax");
        return resp;
    });

    // Get current user profile (Me)
    CROW_ROUTE(app, "/api/auth/me").methods(crow::HTTPMethod::GET)
    ([&authService](const crow::request& req) {
        auto user = authService.authenticateRequest(req);
        if (!user) {
            return errorResponse(401, "Not authenticated or session expired.");
        }

        crow::json::wvalue out;
        out["user"] = user->toJson();
        return successResponse("Profile retrieved.", std::move(out));
    });

    // Logout
    CROW_ROUTE(app, "/api/auth/logout").methods(crow::HTTPMethod::POST)
    ([&authService](const crow::request& req) {
        auto authHeader = req.get_header_value("Authorization");
        std::string token = "";
        if (!authHeader.empty() && authHeader.rfind("Bearer ", 0) == 0) {
            token = authHeader.substr(7);
        } else {
            token = req.get_header_value("X-Session-Token");
        }

        if (!token.empty()) {
            authService.logout(token);
        }

        auto resp = successResponse("Logged out successfully.");
        resp.add_header("Set-Cookie", "session_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
        return resp;
    });
}

} // namespace library
