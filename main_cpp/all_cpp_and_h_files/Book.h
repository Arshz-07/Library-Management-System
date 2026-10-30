#pragma once

#include <string>
#include <crow/json.h>

namespace library {

struct Book {
    int64_t id{0};
    std::string isbn;
    std::string title;
    std::string author;
    int64_t categoryId{0};
    std::string categoryName;
    std::string publisher;
    int publishYear{2024};
    int totalCopies{1};
    int availableCopies{1};
    std::string shelfLocation;
    std::string coverUrl;
    std::string description;
    std::string createdAt;

    crow::json::wvalue toJson() const {
        crow::json::wvalue json;
        json["id"] = id;
        json["isbn"] = isbn;
        json["title"] = title;
        json["author"] = author;
        json["categoryId"] = categoryId;
        json["categoryName"] = categoryName;
        json["publisher"] = publisher;
        json["publishYear"] = publishYear;
        json["totalCopies"] = totalCopies;
        json["availableCopies"] = availableCopies;
        json["borrowedCopies"] = (totalCopies - availableCopies);
        json["shelfLocation"] = shelfLocation;
        json["coverUrl"] = coverUrl;
        json["description"] = description;
        json["createdAt"] = createdAt;
        json["isAvailable"] = (availableCopies > 0);
        return json;
    }
};

} // namespace library
