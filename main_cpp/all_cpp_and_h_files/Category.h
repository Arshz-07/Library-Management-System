#pragma once

#include <string>
#include <crow/json.h>

namespace library {

struct Category {
    int64_t id{0};
    std::string name;
    std::string description;
    std::string icon;
    int bookCount{0};

    crow::json::wvalue toJson() const {
        crow::json::wvalue json;
        json["id"] = id;
        json["name"] = name;
        json["description"] = description;
        json["icon"] = icon;
        json["bookCount"] = bookCount;
        return json;
    }
};

} // namespace library
