#include "repositories/LoanRepository.h"
#include <iostream>
#include <cmath>

namespace library {

LoanRepository::LoanRepository(DatabaseManager& db, double dailyFineRate) 
    : m_db(db), m_fineRate(dailyFineRate) {}

Loan LoanRepository::mapRow(const SQLiteStatement& stmt) {
    Loan l;
    l.id = stmt.getInt64(0);
    l.bookId = stmt.getInt64(1);
    l.bookTitle = stmt.getString(2);
    l.bookAuthor = stmt.getString(3);
    l.bookIsbn = stmt.getString(4);
    l.bookCoverUrl = stmt.getString(5);
    l.userId = stmt.getInt64(6);
    l.userName = stmt.getString(7);
    l.userEmail = stmt.getString(8);
    l.memberCode = stmt.getString(9);
    l.borrowDate = stmt.getString(10);
    l.dueDate = stmt.getString(11);
    l.returnDate = stmt.isNull(12) ? "" : stmt.getString(12);
    l.fineAmount = stmt.getDouble(13);
    l.finePaid = (stmt.getInt(14) == 1);
    l.status = stmt.getString(15);
    l.notes = stmt.isNull(16) ? "" : stmt.getString(16);
    l.createdAt = stmt.getString(17);
    l.daysLeft = stmt.getInt(18);
    l.isOverdue = (l.status == "overdue" || (l.status == "borrowed" && l.daysLeft < 0));
    return l;
}

int64_t LoanRepository::issueLoan(int64_t bookId, int64_t userId, int loanDays, const std::string& notes) {
    auto tx = m_db.beginTransaction();

    const std::string sql = R"(
        INSERT INTO loans (book_id, user_id, borrow_date, due_date, status, notes)
        VALUES (?, ?, datetime('now'), datetime('now', '+' || ? || ' days'), 'borrowed', ?);
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, bookId);
    stmt.bindInt64(2, userId);
    stmt.bindInt(3, loanDays);
    stmt.bindString(4, notes);
    stmt.step();

    int64_t loanId = m_db.getLastInsertId();

    // Decrement available copies
    const std::string decSql = "UPDATE books SET available_copies = available_copies - 1 WHERE id = ? AND available_copies > 0;";
    auto decStmt = m_db.prepare(decSql);
    decStmt.bindInt64(1, bookId);
    decStmt.step();

    if (m_db.getChangesCount() == 0) {
        tx.rollback();
        throw std::runtime_error("Book has no available copies to borrow.");
    }

    tx.commit();
    return loanId;
}

bool LoanRepository::returnLoan(int64_t loanId, double customFine) {
    auto tx = m_db.beginTransaction();

    // Fetch loan details first
    auto loanOpt = findById(loanId);
    if (!loanOpt || loanOpt->status == "returned") {
        return false;
    }

    double fineToApply = loanOpt->fineAmount;
    if (customFine >= 0.0) {
        fineToApply = customFine;
    } else {
        // Calculate fine automatically based on overdue days
        if (loanOpt->daysLeft < 0) {
            int overdueDays = std::abs(loanOpt->daysLeft);
            fineToApply = overdueDays * m_fineRate;
        }
    }

    const std::string sql = R"(
        UPDATE loans 
        SET return_date = datetime('now'), 
            status = 'returned', 
            fine_amount = ?,
            fine_paid = (CASE WHEN ? <= 0.0 THEN 1 ELSE fine_paid END)
        WHERE id = ?;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindDouble(1, fineToApply);
    stmt.bindDouble(2, fineToApply);
    stmt.bindInt64(3, loanId);
    stmt.step();

    // Increment available copies
    const std::string incSql = "UPDATE books SET available_copies = available_copies + 1 WHERE id = ?;";
    auto incStmt = m_db.prepare(incSql);
    incStmt.bindInt64(1, loanOpt->bookId);
    incStmt.step();

    tx.commit();
    return true;
}

std::optional<Loan> LoanRepository::findById(int64_t id) {
    refreshOverdueStatuses();
    const std::string sql = R"(
        SELECT l.id, l.book_id, b.title, b.author, b.isbn, b.cover_url,
               l.user_id, u.name, u.email, u.member_code,
               l.borrow_date, l.due_date, l.return_date, l.fine_amount, l.fine_paid,
               l.status, l.notes, l.created_at,
               CAST(julianday(l.due_date) - julianday('now') AS INTEGER) AS days_left
        FROM loans l
        JOIN books b ON l.book_id = b.id
        JOIN users u ON l.user_id = u.id
        WHERE l.id = ?;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, id);
    if (stmt.step()) {
        return mapRow(stmt);
    }
    return std::nullopt;
}

std::optional<Loan> LoanRepository::findActiveLoan(int64_t bookId, int64_t userId) {
    const std::string sql = R"(
        SELECT l.id, l.book_id, b.title, b.author, b.isbn, b.cover_url,
               l.user_id, u.name, u.email, u.member_code,
               l.borrow_date, l.due_date, l.return_date, l.fine_amount, l.fine_paid,
               l.status, l.notes, l.created_at,
               CAST(julianday(l.due_date) - julianday('now') AS INTEGER) AS days_left
        FROM loans l
        JOIN books b ON l.book_id = b.id
        JOIN users u ON l.user_id = u.id
        WHERE l.book_id = ? AND l.user_id = ? AND l.status IN ('borrowed', 'overdue')
        LIMIT 1;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, bookId);
    stmt.bindInt64(2, userId);
    if (stmt.step()) {
        return mapRow(stmt);
    }
    return std::nullopt;
}

std::vector<Loan> LoanRepository::getActiveLoans(int64_t userId) {
    refreshOverdueStatuses();
    std::string sql = R"(
        SELECT l.id, l.book_id, b.title, b.author, b.isbn, b.cover_url,
               l.user_id, u.name, u.email, u.member_code,
               l.borrow_date, l.due_date, l.return_date, l.fine_amount, l.fine_paid,
               l.status, l.notes, l.created_at,
               CAST(julianday(l.due_date) - julianday('now') AS INTEGER) AS days_left
        FROM loans l
        JOIN books b ON l.book_id = b.id
        JOIN users u ON l.user_id = u.id
        WHERE l.status IN ('borrowed', 'overdue')
    )";

    if (userId > 0) {
        sql += " AND l.user_id = ? ";
    }
    sql += " ORDER BY l.due_date ASC;";

    auto stmt = m_db.prepare(sql);
    if (userId > 0) {
        stmt.bindInt64(1, userId);
    }

    std::vector<Loan> list;
    while (stmt.step()) {
        list.push_back(mapRow(stmt));
    }
    return list;
}

std::vector<Loan> LoanRepository::getOverdueLoans() {
    refreshOverdueStatuses();
    const std::string sql = R"(
        SELECT l.id, l.book_id, b.title, b.author, b.isbn, b.cover_url,
               l.user_id, u.name, u.email, u.member_code,
               l.borrow_date, l.due_date, l.return_date, l.fine_amount, l.fine_paid,
               l.status, l.notes, l.created_at,
               CAST(julianday(l.due_date) - julianday('now') AS INTEGER) AS days_left
        FROM loans l
        JOIN books b ON l.book_id = b.id
        JOIN users u ON l.user_id = u.id
        WHERE l.status = 'overdue'
        ORDER BY l.due_date ASC;
    )";
    auto stmt = m_db.prepare(sql);
    std::vector<Loan> list;
    while (stmt.step()) {
        list.push_back(mapRow(stmt));
    }
    return list;
}

std::vector<Loan> LoanRepository::getAllLoans(const std::string& statusFilter, int64_t userId, int limit) {
    refreshOverdueStatuses();
    std::string sql = R"(
        SELECT l.id, l.book_id, b.title, b.author, b.isbn, b.cover_url,
               l.user_id, u.name, u.email, u.member_code,
               l.borrow_date, l.due_date, l.return_date, l.fine_amount, l.fine_paid,
               l.status, l.notes, l.created_at,
               CAST(julianday(l.due_date) - julianday('now') AS INTEGER) AS days_left
        FROM loans l
        JOIN books b ON l.book_id = b.id
        JOIN users u ON l.user_id = u.id
        WHERE 1=1
    )";

    if (!statusFilter.empty()) {
        sql += " AND l.status = ? ";
    }
    if (userId > 0) {
        sql += " AND l.user_id = ? ";
    }
    sql += " ORDER BY l.id DESC LIMIT ?;";

    auto stmt = m_db.prepare(sql);
    int idx = 1;
    if (!statusFilter.empty()) {
        stmt.bindString(idx++, statusFilter);
    }
    if (userId > 0) {
        stmt.bindInt64(idx++, userId);
    }
    stmt.bindInt(idx++, limit > 0 ? limit : 100);

    std::vector<Loan> list;
    while (stmt.step()) {
        list.push_back(mapRow(stmt));
    }
    return list;
}

bool LoanRepository::markFinePaid(int64_t loanId) {
    const std::string sql = "UPDATE loans SET fine_paid = 1 WHERE id = ?;";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, loanId);
    stmt.step();
    return m_db.getChangesCount() > 0;
}

double LoanRepository::getTotalPendingFines(int64_t userId) {
    std::string sql = "SELECT COALESCE(SUM(fine_amount), 0.0) FROM loans WHERE fine_paid = 0";
    if (userId > 0) {
        sql += " AND user_id = ?;";
    } else {
        sql += ";";
    }
    auto stmt = m_db.prepare(sql);
    if (userId > 0) {
        stmt.bindInt64(1, userId);
    }
    if (stmt.step()) {
        return stmt.getDouble(0);
    }
    return 0.0;
}

double LoanRepository::getTotalCollectedFines() {
    const std::string sql = "SELECT COALESCE(SUM(fine_amount), 0.0) FROM loans WHERE fine_paid = 1 AND fine_amount > 0;";
    auto stmt = m_db.prepare(sql);
    if (stmt.step()) {
        return stmt.getDouble(0);
    }
    return 0.0;
}

int LoanRepository::countActiveLoans(int64_t userId) {
    std::string sql = "SELECT COUNT(*) FROM loans WHERE status IN ('borrowed', 'overdue')";
    if (userId > 0) {
        sql += " AND user_id = ?;";
    } else {
        sql += ";";
    }
    auto stmt = m_db.prepare(sql);
    if (userId > 0) {
        stmt.bindInt64(1, userId);
    }
    if (stmt.step()) {
        return stmt.getInt(0);
    }
    return 0;
}

int LoanRepository::countOverdueLoans() {
    refreshOverdueStatuses();
    const std::string sql = "SELECT COUNT(*) FROM loans WHERE status = 'overdue';";
    auto stmt = m_db.prepare(sql);
    if (stmt.step()) {
        return stmt.getInt(0);
    }
    return 0;
}

int LoanRepository::countReturnedLoans() {
    const std::string sql = "SELECT COUNT(*) FROM loans WHERE status = 'returned';";
    auto stmt = m_db.prepare(sql);
    if (stmt.step()) {
        return stmt.getInt(0);
    }
    return 0;
}

void LoanRepository::refreshOverdueStatuses() {
    // Automatically transition past due loans to 'overdue' and update dynamic fines
    const std::string sql = R"(
        UPDATE loans 
        SET status = 'overdue',
            fine_amount = MAX(fine_amount, CAST(julianday('now') - julianday(due_date) AS INTEGER) * ?)
        WHERE status = 'borrowed' AND julianday('now') > julianday(due_date);
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindDouble(1, m_fineRate);
    stmt.step();
}

} // namespace library
