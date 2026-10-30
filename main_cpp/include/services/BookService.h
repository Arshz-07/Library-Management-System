#pragma once

#include <string>
#include <vector>
#include <optional>
#include "repositories/BookRepository.h"
#include "repositories/StatsRepository.h"
#include "models/Book.h"

namespace library {

class BookService {
public:
    explicit BookService(BookRepository& bookRepo, StatsRepository& statsRepo);

    int64_t addBook(Book book, int64_t actingUserId = 0);
    bool updateBook(Book book, int64_t actingUserId = 0);
    bool deleteBook(int64_t id, int64_t actingUserId = 0);

    std::optional<Book> getBook(int64_t id);
    std::vector<Book> listBooks(const std::string& query = "", 
                                int64_t categoryId = 0, 
                                bool onlyAvailable = false, 
                                const std::string& sort = "title");

    std::vector<Category> getCategories();

private:
    BookRepository& m_bookRepo;
    StatsRepository& m_statsRepo;
};

} // namespace library
