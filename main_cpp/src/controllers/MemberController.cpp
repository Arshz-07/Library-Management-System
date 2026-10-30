#include "controllers/Controllers.h"
#include <iostream>

namespace library {

void registerMemberRoutes(crow::SimpleApp& app, MemberService& memberService, AuthService& authService) {
    // List members
    CROW_ROUTE(app, "/api/members").methods(crow::HTTPMethod::GET)
    ([&memberService, &authService](const crow::request& req) {
        auto user = authService.authenticateRequest(req);
        if (!user || !authService.hasRole(*user, {"admin", "librarian"})) {
            return errorResponse(403, "Access denied. Librarian or Admin privileges required.");
        }

        std::string search = req.url_params.get("q") ? req.url_params.get("q") : "";
        auto members = memberService.listMembers(search);

        std::vector<crow::json::wvalue> list;
        for (const auto& m : members) {
            list.push_back(m.toJson());
        }

        crow::json::wvalue out;
        out["total"] = static_cast<int>(members.size());
        out["members"] = std::move(list);
        return successResponse("Members retrieved.", std::move(out));
    });

    // Get single member details
    CROW_ROUTE(app, "/api/members/<int>").methods(crow::HTTPMethod::GET)
    ([&memberService, &authService](const crow::request& req, int id) {
        auto user = authService.authenticateRequest(req);
        if (!user) {
            return errorResponse(401, "Authentication required.");
        }

        if (user->id != id && !authService.hasRole(*user, {"admin", "librarian"})) {
            return errorResponse(403, "Access denied.");
        }

        auto member = memberService.getMember(id);
        if (!member) {
            return errorResponse(404, "Member not found.");
        }

        crow::json::wvalue out;
        out["member"] = member->toJson();
        return successResponse("Member found.", std::move(out));
    });

    // Create member by staff
    CROW_ROUTE(app, "/api/members").methods(crow::HTTPMethod::POST)
    ([&authService](const crow::request& req) {
        auto caller = authService.authenticateRequest(req);
        if (!caller || !authService.hasRole(*caller, {"admin", "librarian"})) {
            return errorResponse(403, "Access denied. Librarian or Admin privileges required.");
        }

        auto json = crow::json::load(req.body);
        if (!json) {
            return errorResponse(400, "Invalid JSON payload.");
        }

        std::string name = jsonString(json, "name");
        std::string email = jsonString(json, "email");
        std::string password = jsonString(json, "password", "welcome123");
        std::string phone = jsonString(json, "phone");
        std::string role = jsonString(json, "role", "member");

        // Only admin can create librarians or admins
        if ((role == "admin" || role == "librarian") && caller->role != "admin") {
            return errorResponse(403, "Only Administrators can create staff accounts.");
        }

        auto res = authService.registerUser(name, email, password, phone, role);
        if (!res.success) {
            return errorResponse(400, res.errorMessage);
        }

        crow::json::wvalue out;
        out["member"] = res.user->toJson();
        return successResponse("Member account created successfully.", std::move(out));
    });

    // Update status (active / suspended)
    CROW_ROUTE(app, "/api/members/<int>/status").methods(crow::HTTPMethod::PUT)
    ([&memberService, &authService](const crow::request& req, int id) {
        auto caller = authService.authenticateRequest(req);
        if (!caller || !authService.hasRole(*caller, {"admin", "librarian"})) {
            return errorResponse(403, "Access denied. Librarian or Admin privileges required.");
        }

        auto json = crow::json::load(req.body);
        if (!json || !json.has("status")) {
            return errorResponse(400, "Field 'status' (active/suspended) is required.");
        }

        std::string status = jsonString(json, "status");
        if (status != "active" && status != "suspended") {
            return errorResponse(400, "Status must be 'active' or 'suspended'.");
        }

        try {
            bool ok = memberService.updateStatus(id, status, caller->id);
            if (!ok) return errorResponse(404, "Member not found.");
            return successResponse("Member status updated to " + status + ".");
        } catch (const std::exception& ex) {
            return errorResponse(400, ex.what());
        }
    });

    // Delete member
    CROW_ROUTE(app, "/api/members/<int>").methods(crow::HTTPMethod::Delete)
    ([&memberService, &authService](const crow::request& req, int id) {
        auto caller = authService.authenticateRequest(req);
        if (!caller || !authService.hasRole(*caller, {"admin"})) {
            return errorResponse(403, "Only Administrators can delete member accounts.");
        }

        try {
            bool ok = memberService.deleteMember(id, caller->id);
            if (!ok) return errorResponse(404, "Member not found.");
            return successResponse("Member account deleted successfully.");
        } catch (const std::exception& ex) {
            return errorResponse(400, ex.what());
        }
    });
}

} // namespace library
