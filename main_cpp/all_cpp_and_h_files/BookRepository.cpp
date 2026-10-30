#include "repositories/BookRepository.h"
#include <iostream>

namespace library {

BookRepository::BookRepository(DatabaseManager& db) : m_db(db) {}

Book BookRepository::mapRow(const SQLiteStatement& stmt) {
    Book b;
    b.id = stmt.getInt64(0);
    b.isbn = stmt.getString(1);
    b.title = stmt.getString(2);
    b.author = stmt.getString(3);
    b.categoryId = stmt.getInt64(4);
    b.categoryName = stmt.getString(5);
    b.publisher = stmt.getString(6);
    b.publishYear = stmt.getInt(7);
    b.totalCopies = stmt.getInt(8);
    b.availableCopies = stmt.getInt(9);
    b.shelfLocation = stmt.getString(10);
    b.coverUrl = stmt.getString(11);
    b.description = stmt.getString(12);
    b.createdAt = stmt.getString(13);
    return b;
}

int64_t BookRepository::create(const Book& book) {
    const std::string sql = R"(
        INSERT INTO books (isbn, title, author, category_id, publisher, publish_year, 
                           total_copies, available_copies, shelf_location, cover_url, description)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, book.isbn);
    stmt.bindString(2, book.title);
    stmt.bindString(3, book.author);
    if (book.categoryId > 0) {
        stmt.bindInt64(4, book.categoryId);
    } else {
        stmt.bindNull(4);
    }
    stmt.bindString(5, book.publisher);
    stmt.bindInt(6, book.publishYear);
    stmt.bindInt(7, book.totalCopies);
    stmt.bindInt(8, book.availableCopies > 0 ? book.availableCopies : book.totalCopies);
    stmt.bindString(9, book.shelfLocation);
    stmt.bindString(10, book.coverUrl);
    stmt.bindString(11, book.description);
    stmt.step();
    return m_db.getLastInsertId();
}

bool BookRepository::update(const Book& book) {
    const std::string sql = R"(
        UPDATE books 
        SET isbn = ?, title = ?, author = ?, category_id = ?, publisher = ?, 
            publish_year = ?, total_copies = ?, available_copies = ?, 
            shelf_location = ?, cover_url = ?, description = ?
        WHERE id = ?;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, book.isbn);
    stmt.bindString(2, book.title);
    stmt.bindString(3, book.author);
    if (book.categoryId > 0) {
        stmt.bindInt64(4, book.categoryId);
    } else {
        stmt.bindNull(4);
    }
    stmt.bindString(5, book.publisher);
    stmt.bindInt(6, book.publishYear);
    stmt.bindInt(7, book.totalCopies);
    stmt.bindInt(8, book.availableCopies);
    stmt.bindString(9, book.shelfLocation);
    stmt.bindString(10, book.coverUrl);
    stmt.bindString(11, book.description);
    stmt.bindInt64(12, book.id);
    stmt.step();
    return m_db.getChangesCount() > 0;
}

bool BookRepository::deleteBook(int64_t id) {
    const std::string sql = "DELETE FROM books WHERE id = ?;";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, id);
    stmt.step();
    return m_db.getChangesCount() > 0;
}

std::optional<Book> BookRepository::findById(int64_t id) {
    const std::string sql = R"(
        SELECT b.id, b.isbn, b.title, b.author, b.category_id, COALESCE(c.name, 'Uncategorized'),
               b.publisher, b.publish_year, b.total_copies, b.available_copies,
               b.shelf_location, b.cover_url, b.description, b.created_at
        FROM books b
        LEFT JOIN categories c ON b.category_id = c.id
        WHERE b.id = ?;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, id);
    if (stmt.step()) {
        return mapRow(stmt);
    }
    return std::nullopt;
}

std::optional<Book> BookRepository::findByIsbn(const std::string& isbn) {
    const std::string sql = R"(
        SELECT b.id, b.isbn, b.title, b.author, b.category_id, COALESCE(c.name, 'Uncategorized'),
               b.publisher, b.publish_year, b.total_copies, b.available_copies,
               b.shelf_location, b.cover_url, b.description, b.created_at
        FROM books b
        LEFT JOIN categories c ON b.category_id = c.id
        WHERE b.isbn = ?;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, isbn);
    if (stmt.step()) {
        return mapRow(stmt);
    }
    return std::nullopt;
}

std::vector<Book> BookRepository::getAll(const std::string& query, 
                                        int64_t categoryId, 
                                        bool onlyAvailable, 
                                        const std::string& sortBy) {
    std::string sql = R"(
        SELECT b.id, b.isbn, b.title, b.author, b.category_id, COALESCE(c.name, 'Uncategorized'),
               b.publisher, b.publish_year, b.total_copies, b.available_copies,
               b.shelf_location, b.cover_url, b.description, b.created_at
        FROM books b
        LEFT JOIN categories c ON b.category_id = c.id
        WHERE 1=1
    )";

    if (!query.empty()) {
        sql += " AND (b.title LIKE ? OR b.author LIKE ? OR b.isbn LIKE ? OR b.description LIKE ?) ";
    }
    if (categoryId > 0) {
        sql += " AND b.category_id = ? ";
    }
    if (onlyAvailable) {
        sql += " AND b.available_copies > 0 ";
    }

    if (sortBy == "author") {
        sql += " ORDER BY b.author ASC, b.title ASC";
    } else if (sortBy == "year") {
        sql += " ORDER BY b.publish_year DESC, b.title ASC";
    } else if (sortBy == "newest") {
        sql += " ORDER BY b.id DESC";
    } else if (sortBy == "popular") {
        sql += " ORDER BY (b.total_copies - b.available_copies) DESC, b.title ASC";
    } else {
        sql += " ORDER BY b.title ASC";
    }

    auto stmt = m_db.prepare(sql);
    int bindIndex = 1;
    if (!query.empty()) {
        std::string pat = "%" + query + "%";
        stmt.bindString(bindIndex++, pat);
        stmt.bindString(bindIndex++, pat);
        stmt.bindString(bindIndex++, pat);
        stmt.bindString(bindIndex++, pat);
    }
    if (categoryId > 0) {
        stmt.bindInt64(bindIndex++, categoryId);
    }

    std::vector<Book> books;
    while (stmt.step()) {
        books.push_back(mapRow(stmt));
    }
    return books;
}

bool BookRepository::decrementAvailableCopies(int64_t bookId) {
    const std::string sql = R"(
        UPDATE books 
        SET available_copies = available_copies - 1 
        WHERE id = ? AND available_copies > 0;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, bookId);
    stmt.step();
    return m_db.getChangesCount() > 0;
}

bool BookRepository::incrementAvailableCopies(int64_t bookId) {
    const std::string sql = R"(
        UPDATE books 
        SET available_copies = available_copies + 1 
        WHERE id = ? AND available_copies < total_copies;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, bookId);
    stmt.step();
    return m_db.getChangesCount() > 0;
}

std::vector<Category> BookRepository::getCategories() {
    const std::string sql = R"(
        SELECT c.id, c.name, c.description, c.icon,
               (SELECT COUNT(*) FROM books b WHERE b.category_id = c.id) AS book_count
        FROM categories c
        ORDER BY c.name ASC;
    )";
    auto stmt = m_db.prepare(sql);
    std::vector<Category> categories;
    while (stmt.step()) {
        Category c;
        c.id = stmt.getInt64(0);
        c.name = stmt.getString(1);
        c.description = stmt.getString(2);
        c.icon = stmt.getString(3);
        c.bookCount = stmt.getInt(4);
        categories.push_back(c);
    }
    return categories;
}

int64_t BookRepository::createCategory(const Category& category) {
    const std::string sql = "INSERT OR IGNORE INTO categories (name, description, icon) VALUES (?, ?, ?);";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, category.name);
    stmt.bindString(2, category.description);
    stmt.bindString(3, category.icon);
    stmt.step();
    return m_db.getLastInsertId();
}

int BookRepository::countTotalTitles() {
    const std::string sql = "SELECT COUNT(*) FROM books;";
    auto stmt = m_db.prepare(sql);
    if (stmt.step()) {
        return stmt.getInt(0);
    }
    return 0;
}

int BookRepository::countTotalCopies() {
    const std::string sql = "SELECT COALESCE(SUM(total_copies), 0) FROM books;";
    auto stmt = m_db.prepare(sql);
    if (stmt.step()) {
        return stmt.getInt(0);
    }
    return 0;
}

int BookRepository::countAvailableCopies() {
    const std::string sql = "SELECT COALESCE(SUM(available_copies), 0) FROM books;";
    auto stmt = m_db.prepare(sql);
    if (stmt.step()) {
        return stmt.getInt(0);
    }
    return 0;
}

} // namespace library
