#pragma once

#include <string>
#include <vector>
#include <optional>
#include "core/DatabaseManager.h"
#include "models/User.h"

namespace library {

class UserRepository {
public:
    explicit UserRepository(DatabaseManager& db);

    int64_t create(const User& user);
    std::optional<User> findById(int64_t id);
    std::optional<User> findByEmail(const std::string& email);
    std::optional<User> findByMemberCode(const std::string& code);
    std::vector<User> getAll(const std::string& search = "");
    bool updateStatus(int64_t id, const std::string& status);
    bool updateProfile(int64_t id, const std::string& name, const std::string& phone);
    bool deleteUser(int64_t id);

    // Session authentication
    bool createSession(const std::string& token, int64_t userId, int expireHours = 72);
    std::optional<User> findUserBySession(const std::string& token);
    bool deleteSession(const std::string& token);

    int countMembers();

private:
    DatabaseManager& m_db;
    User mapRow(const SQLiteStatement& stmt, bool includeStats = false);
};

} // namespace library
