#include "core/DatabaseManager.h"
#include <sstream>

namespace library {

// ==================== SQLiteStatement ====================

SQLiteStatement::SQLiteStatement(sqlite3_stmt* stmt) : m_stmt(stmt) {}

SQLiteStatement::~SQLiteStatement() {
    if (m_stmt) {
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }
}

SQLiteStatement::SQLiteStatement(SQLiteStatement&& other) noexcept : m_stmt(other.m_stmt) {
    other.m_stmt = nullptr;
}

SQLiteStatement& SQLiteStatement::operator=(SQLiteStatement&& other) noexcept {
    if (this != &other) {
        if (m_stmt) {
            sqlite3_finalize(m_stmt);
        }
        m_stmt = other.m_stmt;
        other.m_stmt = nullptr;
    }
    return *this;
}

void SQLiteStatement::bindInt(int index, int value) {
    if (sqlite3_bind_int(m_stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error("Failed to bind integer at index " + std::to_string(index));
    }
}

void SQLiteStatement::bindInt64(int index, int64_t value) {
    if (sqlite3_bind_int64(m_stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error("Failed to bind int64 at index " + std::to_string(index));
    }
}

void SQLiteStatement::bindDouble(int index, double value) {
    if (sqlite3_bind_double(m_stmt, index, value) != SQLITE_OK) {
        throw std::runtime_error("Failed to bind double at index " + std::to_string(index));
    }
}

void SQLiteStatement::bindString(int index, const std::string& value) {
    if (sqlite3_bind_text(m_stmt, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        throw std::runtime_error("Failed to bind string at index " + std::to_string(index));
    }
}

void SQLiteStatement::bindNull(int index) {
    if (sqlite3_bind_null(m_stmt, index) != SQLITE_OK) {
        throw std::runtime_error("Failed to bind null at index " + std::to_string(index));
    }
}

bool SQLiteStatement::step() {
    int rc = sqlite3_step(m_stmt);
    if (rc == SQLITE_ROW) {
        return true;
    }
    if (rc == SQLITE_DONE) {
        return false;
    }
    throw std::runtime_error(std::string("SQLite step error: ") + sqlite3_errstr(rc));
}

void SQLiteStatement::reset() {
    sqlite3_reset(m_stmt);
    sqlite3_clear_bindings(m_stmt);
}

int SQLiteStatement::getInt(int column) const {
    return sqlite3_column_int(m_stmt, column);
}

int64_t SQLiteStatement::getInt64(int column) const {
    return sqlite3_column_int64(m_stmt, column);
}

double SQLiteStatement::getDouble(int column) const {
    return sqlite3_column_double(m_stmt, column);
}

std::string SQLiteStatement::getString(int column) const {
    const unsigned char* text = sqlite3_column_text(m_stmt, column);
    return text ? reinterpret_cast<const char*>(text) : std::string();
}

bool SQLiteStatement::isNull(int column) const {
    return sqlite3_column_type(m_stmt, column) == SQLITE_NULL;
}

int SQLiteStatement::getColumnCount() const {
    return sqlite3_column_count(m_stmt);
}

std::string SQLiteStatement::getColumnName(int column) const {
    const char* name = sqlite3_column_name(m_stmt, column);
    return name ? std::string(name) : std::string();
}

// ==================== SQLiteTransaction ====================

SQLiteTransaction::SQLiteTransaction(sqlite3* db) : m_db(db) {
    char* errMsg = nullptr;
    if (sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to begin transaction: " + err);
    }
}

SQLiteTransaction::~SQLiteTransaction() {
    if (m_active && !m_committed && m_db) {
        char* errMsg = nullptr;
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, &errMsg);
        if (errMsg) sqlite3_free(errMsg);
    }
}

void SQLiteTransaction::commit() {
    if (!m_active || m_committed) return;
    char* errMsg = nullptr;
    if (sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to commit transaction: " + err);
    }
    m_committed = true;
    m_active = false;
}

void SQLiteTransaction::rollback() {
    if (!m_active || m_committed) return;
    char* errMsg = nullptr;
    sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    m_active = false;
}

// ==================== DatabaseManager ====================

DatabaseManager::DatabaseManager(const std::string& dbPath) : m_path(dbPath) {
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    int rc = sqlite3_open_v2(m_path.c_str(), &m_db, flags, nullptr);
    if (rc != SQLITE_OK) {
        std::string err = m_db ? sqlite3_errmsg(m_db) : "Unknown open error";
        if (m_db) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        throw std::runtime_error("Cannot open SQLite database at '" + m_path + "': " + err);
    }

    // Enable Foreign Keys and WAL journal mode
    execute("PRAGMA foreign_keys = ON;");
    execute("PRAGMA journal_mode = WAL;");
    execute("PRAGMA synchronous = NORMAL;");
}

DatabaseManager::~DatabaseManager() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

void DatabaseManager::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(m_mutex);
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string err = errMsg ? errMsg : sqlite3_errmsg(m_db);
        sqlite3_free(errMsg);
        throw std::runtime_error("Database execute error: " + err + " (SQL: " + sql + ")");
    }
}

SQLiteStatement DatabaseManager::prepare(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string err = m_db ? sqlite3_errmsg(m_db) : "Unknown prepare error";
        throw std::runtime_error("Prepare failed: " + err + " (SQL: " + sql + ")");
    }
    return SQLiteStatement(stmt);
}

SQLiteTransaction DatabaseManager::beginTransaction() {
    return SQLiteTransaction(m_db);
}

int64_t DatabaseManager::getLastInsertId() const {
    return sqlite3_last_insert_rowid(m_db);
}

int DatabaseManager::getChangesCount() const {
    return sqlite3_changes(m_db);
}

void DatabaseManager::initializeSchema() {
    std::cout << "[Database] Initializing relational schema..." << std::endl;

    const std::string ddl = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            member_code TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL,
            phone TEXT,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            role TEXT NOT NULL CHECK(role IN ('admin', 'librarian', 'member')),
            status TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active', 'suspended')),
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            expires_at DATETIME NOT NULL
        );

        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            description TEXT,
            icon TEXT
        );

        CREATE TABLE IF NOT EXISTS books (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            isbn TEXT UNIQUE NOT NULL,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            category_id INTEGER REFERENCES categories(id) ON DELETE SET NULL,
            publisher TEXT,
            publish_year INTEGER,
            total_copies INTEGER NOT NULL DEFAULT 1,
            available_copies INTEGER NOT NULL DEFAULT 1,
            shelf_location TEXT,
            cover_url TEXT,
            description TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS loans (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            book_id INTEGER NOT NULL REFERENCES books(id) ON DELETE CASCADE,
            user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
            borrow_date TEXT NOT NULL,
            due_date TEXT NOT NULL,
            return_date TEXT,
            fine_amount REAL DEFAULT 0.0,
            fine_paid INTEGER DEFAULT 0,
            status TEXT NOT NULL DEFAULT 'borrowed' CHECK(status IN ('borrowed', 'returned', 'overdue')),
            notes TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS activity_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER REFERENCES users(id) ON DELETE SET NULL,
            action TEXT NOT NULL,
            details TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE INDEX IF NOT EXISTS idx_books_isbn ON books(isbn);
        CREATE INDEX IF NOT EXISTS idx_books_title ON books(title);
        CREATE INDEX IF NOT EXISTS idx_books_author ON books(author);
        CREATE INDEX IF NOT EXISTS idx_books_category ON books(category_id);
        CREATE INDEX IF NOT EXISTS idx_loans_user ON loans(user_id);
        CREATE INDEX IF NOT EXISTS idx_loans_book ON loans(book_id);
        CREATE INDEX IF NOT EXISTS idx_loans_status ON loans(status);
        CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
        CREATE INDEX IF NOT EXISTS idx_users_code ON users(member_code);
    )";

    execute(ddl);
    std::cout << "[Database] Relational schema initialized successfully." << std::endl;
}

} // namespace library
