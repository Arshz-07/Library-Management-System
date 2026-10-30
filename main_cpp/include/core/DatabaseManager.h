#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <mutex>
#include "sqlite3.h"

namespace library {

class SQLiteStatement {
public:
    explicit SQLiteStatement(sqlite3_stmt* stmt);
    ~SQLiteStatement();

    SQLiteStatement(const SQLiteStatement&) = delete;
    SQLiteStatement& operator=(const SQLiteStatement&) = delete;

    SQLiteStatement(SQLiteStatement&& other) noexcept;
    SQLiteStatement& operator=(SQLiteStatement&& other) noexcept;

    void bindInt(int index, int value);
    void bindInt64(int index, int64_t value);
    void bindDouble(int index, double value);
    void bindString(int index, const std::string& value);
    void bindNull(int index);

    bool step(); // true if SQLITE_ROW, false if SQLITE_DONE
    void reset();

    int getInt(int column) const;
    int64_t getInt64(int column) const;
    double getDouble(int column) const;
    std::string getString(int column) const;
    bool isNull(int column) const;
    int getColumnCount() const;
    std::string getColumnName(int column) const;

    sqlite3_stmt* get() const { return m_stmt; }

private:
    sqlite3_stmt* m_stmt{nullptr};
};

class SQLiteTransaction {
public:
    explicit SQLiteTransaction(sqlite3* db);
    ~SQLiteTransaction();

    SQLiteTransaction(const SQLiteTransaction&) = delete;
    SQLiteTransaction& operator=(const SQLiteTransaction&) = delete;

    void commit();
    void rollback();

private:
    sqlite3* m_db{nullptr};
    bool m_committed{false};
    bool m_active{true};
};

class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& dbPath);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    void initializeSchema();
    void execute(const std::string& sql);
    SQLiteStatement prepare(const std::string& sql);
    SQLiteTransaction beginTransaction();

    int64_t getLastInsertId() const;
    int getChangesCount() const;
    sqlite3* getRawHandle() const { return m_db; }

    std::mutex& getMutex() { return m_mutex; }

private:
    sqlite3* m_db{nullptr};
    std::string m_path;
    std::mutex m_mutex;
};

} // namespace library
