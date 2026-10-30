#pragma once

#include <string>
#include <crow/json.h>

namespace library {

struct Loan {
    int64_t id{0};
    int64_t bookId{0};
    std::string bookTitle;
    std::string bookAuthor;
    std::string bookIsbn;
    std::string bookCoverUrl;

    int64_t userId{0};
    std::string userName;
    std::string userEmail;
    std::string memberCode;

    std::string borrowDate;
    std::string dueDate;
    std::string returnDate; // empty if still active
    double fineAmount{0.0};
    bool finePaid{false};
    std::string status;     // "borrowed", "returned", "overdue"
    std::string notes;
    std::string createdAt;

    // Computed attributes
    int daysLeft{0};        // negative if overdue
    bool isOverdue{false};

    crow::json::wvalue toJson() const {
        crow::json::wvalue json;
        json["id"] = id;
        json["bookId"] = bookId;
        json["bookTitle"] = bookTitle;
        json["bookAuthor"] = bookAuthor;
        json["bookIsbn"] = bookIsbn;
        json["bookCoverUrl"] = bookCoverUrl;

        json["userId"] = userId;
        json["userName"] = userName;
        json["userEmail"] = userEmail;
        json["memberCode"] = memberCode;

        json["borrowDate"] = borrowDate;
        json["dueDate"] = dueDate;
        json["returnDate"] = returnDate;
        json["fineAmount"] = fineAmount;
        json["finePaid"] = finePaid;
        json["status"] = status;
        json["notes"] = notes;
        json["createdAt"] = createdAt;
        json["daysLeft"] = daysLeft;
        json["isOverdue"] = isOverdue;
        return json;
    }
};

} // namespace library
