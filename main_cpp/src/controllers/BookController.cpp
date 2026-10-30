#include "controllers/Controllers.h"
#include <iostream>

namespace library {

void registerBookRoutes(crow::SimpleApp& app, BookService& bookService, AuthService& authService) {
    // List books with search & filters
    CROW_ROUTE(app, "/api/books").methods(crow::HTTPMethod::GET)
    ([&bookService](const crow::request& req) {
        std::string query = req.url_params.get("q") ? req.url_params.get("q") : "";
        
        int64_t categoryId = 0;
        if (req.url_params.get("category")) {
            try { categoryId = std::stoll(req.url_params.get("category")); } catch (...) {}
        }

        bool onlyAvailable = false;
        if (req.url_params.get("available") && std::string(req.url_params.get("available")) == "true") {
            onlyAvailable = true;
        }

        std::string sort = req.url_params.get("sort") ? req.url_params.get("sort") : "title";

        auto books = bookService.listBooks(query, categoryId, onlyAvailable, sort);
        std::vector<crow::json::wvalue> bookJsonList;
        for (const auto& b : books) {
            bookJsonList.push_back(b.toJson());
        }

        crow::json::wvalue out;
        out["total"] = static_cast<int>(books.size());
        out["books"] = std::move(bookJsonList);
        return successResponse("Books retrieved.", std::move(out));
    });

    // Get single book
    CROW_ROUTE(app, "/api/books/<int>").methods(crow::HTTPMethod::GET)
    ([&bookService](int id) {
        auto book = bookService.getBook(id);
        if (!book) {
            return errorResponse(404, "Book not found.");
        }
        crow::json::wvalue out;
        out["book"] = book->toJson();
        return successResponse("Book found.", std::move(out));
    });

    // Create book (Admin / Librarian)
    CROW_ROUTE(app, "/api/books").methods(crow::HTTPMethod::POST)
    ([&bookService, &authService](const crow::request& req) {
        auto user = authService.authenticateRequest(req);
        if (!user || !authService.hasRole(*user, {"admin", "librarian"})) {
            return errorResponse(403, "Access denied. Librarian or Admin privileges required.");
        }

        auto json = crow::json::load(req.body);
        if (!json) {
            return errorResponse(400, "Invalid JSON payload.");
        }

        Book b;
        b.isbn = jsonString(json, "isbn");
        b.title = jsonString(json, "title");
        b.author = jsonString(json, "author");
        b.categoryId = jsonInt64(json, "categoryId", 0);
        b.publisher = jsonString(json, "publisher");
        b.publishYear = jsonInt(json, "publishYear", 2024);
        b.totalCopies = jsonInt(json, "totalCopies", 1);
        b.availableCopies = jsonInt(json, "availableCopies", b.totalCopies);
        b.shelfLocation = jsonString(json, "shelfLocation", "General");
        b.coverUrl = jsonString(json, "coverUrl");
        b.description = jsonString(json, "description");

        try {
            int64_t id = bookService.addBook(b, user->id);
            crow::json::wvalue out;
            out["id"] = id;
            return successResponse("Book added to library catalog successfully.", std::move(out));
        } catch (const std::exception& ex) {
            return errorResponse(400, ex.what());
        }
    });

    // Update book (Admin / Librarian)
    CROW_ROUTE(app, "/api/books/<int>").methods(crow::HTTPMethod::PUT)
    ([&bookService, &authService](const crow::request& req, int id) {
        auto user = authService.authenticateRequest(req);
        if (!user || !authService.hasRole(*user, {"admin", "librarian"})) {
            return errorResponse(403, "Access denied. Librarian or Admin privileges required.");
        }

        auto json = crow::json::load(req.body);
        if (!json) {
            return errorResponse(400, "Invalid JSON payload.");
        }

        Book b;
        b.id = id;
        b.isbn = jsonString(json, "isbn");
        b.title = jsonString(json, "title");
        b.author = jsonString(json, "author");
        b.categoryId = jsonInt64(json, "categoryId", 0);
        b.publisher = jsonString(json, "publisher");
        b.publishYear = jsonInt(json, "publishYear", 2024);
        b.totalCopies = jsonInt(json, "totalCopies", 1);
        b.shelfLocation = jsonString(json, "shelfLocation");
        b.coverUrl = jsonString(json, "coverUrl");
        b.description = jsonString(json, "description");

        try {
            bool ok = bookService.updateBook(b, user->id);
            if (!ok) return errorResponse(404, "Book not found or failed to update.");
            return successResponse("Book updated successfully.");
        } catch (const std::exception& ex) {
            return errorResponse(400, ex.what());
        }
    });

    // Delete book (Admin only)
    CROW_ROUTE(app, "/api/books/<int>").methods(crow::HTTPMethod::Delete)
    ([&bookService, &authService](const crow::request& req, int id) {
        auto user = authService.authenticateRequest(req);
        if (!user || !authService.hasRole(*user, {"admin"})) {
            return errorResponse(403, "Access denied. Admin privileges required to delete books.");
        }

        try {
            bool ok = bookService.deleteBook(id, user->id);
            if (!ok) return errorResponse(404, "Book not found.");
            return successResponse("Book deleted from library.");
        } catch (const std::exception& ex) {
            return errorResponse(400, ex.what());
        }
    });

    // Get categories
    CROW_ROUTE(app, "/api/categories").methods(crow::HTTPMethod::GET)
    ([&bookService]() {
        auto categories = bookService.getCategories();
        std::vector<crow::json::wvalue> list;
        for (const auto& c : categories) {
            list.push_back(c.toJson());
        }
        crow::json::wvalue out;
        out["categories"] = std::move(list);
        return successResponse("Categories retrieved.", std::move(out));
    });
}

} // namespace library
