#pragma once

#include <string>
#include <crow/json.h>

namespace library {

struct User {
    int64_t id{0};
    std::string memberCode;
    std::string name;
    std::string email;
    std::string phone;
    std::string passwordHash;
    std::string salt;
    std::string role;   // "admin", "librarian", "member"
    std::string status; // "active", "suspended"
    std::string createdAt;
    
    // Non-persisted computed fields
    int activeLoansCount{0};
    double pendingFines{0.0};

    crow::json::wvalue toJson(bool includePrivate = false) const {
        crow::json::wvalue json;
        json["id"] = id;
        json["memberCode"] = memberCode;
        json["name"] = name;
        json["email"] = email;
        json["phone"] = phone;
        json["role"] = role;
        json["status"] = status;
        json["createdAt"] = createdAt;
        json["activeLoansCount"] = activeLoansCount;
        json["pendingFines"] = pendingFines;

        if (includePrivate) {
            json["passwordHash"] = passwordHash;
            json["salt"] = salt;
        }
        return json;
    }
};

} // namespace library
