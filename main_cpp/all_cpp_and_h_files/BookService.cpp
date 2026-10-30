#include "services/BookService.h"
#include <stdexcept>

namespace library {

BookService::BookService(BookRepository& bookRepo, StatsRepository& statsRepo)
    : m_bookRepo(bookRepo), m_statsRepo(statsRepo) {}

int64_t BookService::addBook(Book book, int64_t actingUserId) {
    if (book.title.empty() || book.author.empty() || book.isbn.empty()) {
        throw std::invalid_argument("Title, Author, and ISBN are required.");
    }

    if (m_bookRepo.findByIsbn(book.isbn).has_value()) {
        throw std::invalid_argument("A book with ISBN '" + book.isbn + "' already exists.");
    }

    if (book.totalCopies <= 0) book.totalCopies = 1;
    if (book.availableCopies <= 0) book.availableCopies = book.totalCopies;

    int64_t id = m_bookRepo.create(book);
    m_statsRepo.logActivity(actingUserId, "BOOK_ADDED", "Added book: " + book.title + " (ISBN: " + book.isbn + ")");
    return id;
}

bool BookService::updateBook(Book book, int64_t actingUserId) {
    if (book.id <= 0) throw std::invalid_argument("Invalid book ID");
    if (book.title.empty() || book.author.empty() || book.isbn.empty()) {
        throw std::invalid_argument("Title, Author, and ISBN are required.");
    }

    auto existing = m_bookRepo.findById(book.id);
    if (!existing) {
        throw std::invalid_argument("Book not found.");
    }

    auto byIsbn = m_bookRepo.findByIsbn(book.isbn);
    if (byIsbn && byIsbn->id != book.id) {
        throw std::invalid_argument("ISBN is already assigned to another book.");
    }

    // Maintain loaned copies count invariant
    int borrowedCount = existing->totalCopies - existing->availableCopies;
    if (book.totalCopies < borrowedCount) {
        throw std::invalid_argument("Total copies cannot be less than currently borrowed copies (" + 
                                    std::to_string(borrowedCount) + ").");
    }
    book.availableCopies = book.totalCopies - borrowedCount;

    bool ok = m_bookRepo.update(book);
    if (ok) {
        m_statsRepo.logActivity(actingUserId, "BOOK_UPDATED", "Updated book: " + book.title + " (ID: " + std::to_string(book.id) + ")");
    }
    return ok;
}

bool BookService::deleteBook(int64_t id, int64_t actingUserId) {
    auto book = m_bookRepo.findById(id);
    if (!book) return false;

    if (book->availableCopies < book->totalCopies) {
        throw std::invalid_argument("Cannot delete book while copies are currently on loan.");
    }

    bool ok = m_bookRepo.deleteBook(id);
    if (ok) {
        m_statsRepo.logActivity(actingUserId, "BOOK_DELETED", "Deleted book: " + book->title);
    }
    return ok;
}

std::optional<Book> BookService::getBook(int64_t id) {
    return m_bookRepo.findById(id);
}

std::vector<Book> BookService::listBooks(const std::string& query, 
                                        int64_t categoryId, 
                                        bool onlyAvailable, 
                                        const std::string& sort) {
    return m_bookRepo.getAll(query, categoryId, onlyAvailable, sort);
}

std::vector<Category> BookService::getCategories() {
    return m_bookRepo.getCategories();
}

} // namespace library
