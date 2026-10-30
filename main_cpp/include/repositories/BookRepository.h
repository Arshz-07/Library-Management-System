#pragma once

#include <string>
#include <vector>
#include <optional>
#include "core/DatabaseManager.h"
#include "models/Book.h"
#include "models/Category.h"

namespace library {

class BookRepository {
public:
    explicit BookRepository(DatabaseManager& db);

    int64_t create(const Book& book);
    bool update(const Book& book);
    bool deleteBook(int64_t id);

    std::optional<Book> findById(int64_t id);
    std::optional<Book> findByIsbn(const std::string& isbn);
    std::vector<Book> getAll(const std::string& query = "", 
                             int64_t categoryId = 0, 
                             bool onlyAvailable = false, 
                             const std::string& sortBy = "title");

    // Inventory adjustments
    bool decrementAvailableCopies(int64_t bookId);
    bool incrementAvailableCopies(int64_t bookId);

    // Categories
    std::vector<Category> getCategories();
    int64_t createCategory(const Category& category);

    // Metrics
    int countTotalTitles();
    int countTotalCopies();
    int countAvailableCopies();

private:
    DatabaseManager& m_db;
    Book mapRow(const SQLiteStatement& stmt);
};

} // namespace library
