#include "repositories/UserRepository.h"
#include <iostream>

namespace library {

UserRepository::UserRepository(DatabaseManager& db) : m_db(db) {}

User UserRepository::mapRow(const SQLiteStatement& stmt, bool includeStats) {
    User u;
    u.id = stmt.getInt64(0);
    u.memberCode = stmt.getString(1);
    u.name = stmt.getString(2);
    u.email = stmt.getString(3);
    u.phone = stmt.getString(4);
    u.passwordHash = stmt.getString(5);
    u.salt = stmt.getString(6);
    u.role = stmt.getString(7);
    u.status = stmt.getString(8);
    u.createdAt = stmt.getString(9);

    if (includeStats && stmt.getColumnCount() >= 12) {
        u.activeLoansCount = stmt.getInt(10);
        u.pendingFines = stmt.getDouble(11);
    }
    return u;
}

int64_t UserRepository::create(const User& user) {
    const std::string sql = R"(
        INSERT INTO users (member_code, name, email, phone, password_hash, salt, role, status)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, user.memberCode);
    stmt.bindString(2, user.name);
    stmt.bindString(3, user.email);
    stmt.bindString(4, user.phone);
    stmt.bindString(5, user.passwordHash);
    stmt.bindString(6, user.salt);
    stmt.bindString(7, user.role.empty() ? "member" : user.role);
    stmt.bindString(8, user.status.empty() ? "active" : user.status);
    stmt.step();
    return m_db.getLastInsertId();
}

std::optional<User> UserRepository::findById(int64_t id) {
    const std::string sql = R"(
        SELECT u.id, u.member_code, u.name, u.email, u.phone, u.password_hash, u.salt, u.role, u.status, u.created_at,
               (SELECT COUNT(*) FROM loans l WHERE l.user_id = u.id AND l.status IN ('borrowed', 'overdue')) AS active_loans,
               (SELECT COALESCE(SUM(l.fine_amount), 0.0) FROM loans l WHERE l.user_id = u.id AND l.fine_paid = 0) AS pending_fines
        FROM users u
        WHERE u.id = ?;
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, id);
    if (stmt.step()) {
        return mapRow(stmt, true);
    }
    return std::nullopt;
}

std::optional<User> UserRepository::findByEmail(const std::string& email) {
    const std::string sql = R"(
        SELECT u.id, u.member_code, u.name, u.email, u.phone, u.password_hash, u.salt, u.role, u.status, u.created_at,
               (SELECT COUNT(*) FROM loans l WHERE l.user_id = u.id AND l.status IN ('borrowed', 'overdue')) AS active_loans,
               (SELECT COALESCE(SUM(l.fine_amount), 0.0) FROM loans l WHERE l.user_id = u.id AND l.fine_paid = 0) AS pending_fines
        FROM users u
        WHERE LOWER(u.email) = LOWER(?);
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, email);
    if (stmt.step()) {
        return mapRow(stmt, true);
    }
    return std::nullopt;
}

std::optional<User> UserRepository::findByMemberCode(const std::string& code) {
    const std::string sql = R"(
        SELECT u.id, u.member_code, u.name, u.email, u.phone, u.password_hash, u.salt, u.role, u.status, u.created_at,
               (SELECT COUNT(*) FROM loans l WHERE l.user_id = u.id AND l.status IN ('borrowed', 'overdue')) AS active_loans,
               (SELECT COALESCE(SUM(l.fine_amount), 0.0) FROM loans l WHERE l.user_id = u.id AND l.fine_paid = 0) AS pending_fines
        FROM users u
        WHERE UPPER(u.member_code) = UPPER(?);
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, code);
    if (stmt.step()) {
        return mapRow(stmt, true);
    }
    return std::nullopt;
}

std::vector<User> UserRepository::getAll(const std::string& search) {
    std::string sql = R"(
        SELECT u.id, u.member_code, u.name, u.email, u.phone, u.password_hash, u.salt, u.role, u.status, u.created_at,
               (SELECT COUNT(*) FROM loans l WHERE l.user_id = u.id AND l.status IN ('borrowed', 'overdue')) AS active_loans,
               (SELECT COALESCE(SUM(l.fine_amount), 0.0) FROM loans l WHERE l.user_id = u.id AND l.fine_paid = 0) AS pending_fines
        FROM users u
    )";

    if (!search.empty()) {
        sql += " WHERE u.name LIKE ? OR u.email LIKE ? OR u.member_code LIKE ? ";
    }
    sql += " ORDER BY u.created_at DESC;";

    auto stmt = m_db.prepare(sql);
    if (!search.empty()) {
        std::string pattern = "%" + search + "%";
        stmt.bindString(1, pattern);
        stmt.bindString(2, pattern);
        stmt.bindString(3, pattern);
    }

    std::vector<User> users;
    while (stmt.step()) {
        users.push_back(mapRow(stmt, true));
    }
    return users;
}

bool UserRepository::updateStatus(int64_t id, const std::string& status) {
    const std::string sql = "UPDATE users SET status = ? WHERE id = ?;";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, status);
    stmt.bindInt64(2, id);
    stmt.step();
    return m_db.getChangesCount() > 0;
}

bool UserRepository::updateProfile(int64_t id, const std::string& name, const std::string& phone) {
    const std::string sql = "UPDATE users SET name = ?, phone = ? WHERE id = ?;";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, name);
    stmt.bindString(2, phone);
    stmt.bindInt64(3, id);
    stmt.step();
    return m_db.getChangesCount() > 0;
}

bool UserRepository::deleteUser(int64_t id) {
    const std::string sql = "DELETE FROM users WHERE id = ?;";
    auto stmt = m_db.prepare(sql);
    stmt.bindInt64(1, id);
    stmt.step();
    return m_db.getChangesCount() > 0;
}

bool UserRepository::createSession(const std::string& token, int64_t userId, int expireHours) {
    const std::string sql = R"(
        INSERT OR REPLACE INTO sessions (token, user_id, expires_at)
        VALUES (?, ?, datetime('now', '+' || ? || ' hours'));
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, token);
    stmt.bindInt64(2, userId);
    stmt.bindInt(3, expireHours);
    stmt.step();
    return true;
}

std::optional<User> UserRepository::findUserBySession(const std::string& token) {
    const std::string sql = R"(
        SELECT u.id, u.member_code, u.name, u.email, u.phone, u.password_hash, u.salt, u.role, u.status, u.created_at,
               (SELECT COUNT(*) FROM loans l WHERE l.user_id = u.id AND l.status IN ('borrowed', 'overdue')) AS active_loans,
               (SELECT COALESCE(SUM(l.fine_amount), 0.0) FROM loans l WHERE l.user_id = u.id AND l.fine_paid = 0) AS pending_fines
        FROM sessions s
        JOIN users u ON s.user_id = u.id
        WHERE s.token = ? AND datetime(s.expires_at) > datetime('now');
    )";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, token);
    if (stmt.step()) {
        return mapRow(stmt, true);
    }
    return std::nullopt;
}

bool UserRepository::deleteSession(const std::string& token) {
    const std::string sql = "DELETE FROM sessions WHERE token = ?;";
    auto stmt = m_db.prepare(sql);
    stmt.bindString(1, token);
    stmt.step();
    return m_db.getChangesCount() > 0;
}

int UserRepository::countMembers() {
    const std::string sql = "SELECT COUNT(*) FROM users WHERE role = 'member';";
    auto stmt = m_db.prepare(sql);
    if (stmt.step()) {
        return stmt.getInt(0);
    }
    return 0;
}

} // namespace library
