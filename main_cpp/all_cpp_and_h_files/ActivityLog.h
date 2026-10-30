#pragma once

#include <string>
#include <crow/json.h>

namespace library {

struct ActivityLog {
    int64_t id{0};
    int64_t userId{0};
    std::string userName;
    std::string action;
    std::string details;
    std::string createdAt;

    crow::json::wvalue toJson() const {
        crow::json::wvalue json;
        json["id"] = id;
        json["userId"] = userId;
        json["userName"] = userName;
        json["action"] = action;
        json["details"] = details;
        json["createdAt"] = createdAt;
        return json;
    }
};

} // namespace library
