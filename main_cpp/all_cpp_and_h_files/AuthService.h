#pragma once

#include <string>
#include <optional>
#include <vector>
#include <crow/http_request.h>
#include "repositories/UserRepository.h"
#include "repositories/StatsRepository.h"

namespace library {

struct AuthResult {
    bool success{false};
    std::string token;
    std::optional<User> user;
    std::string errorMessage;
};

class AuthService {
public:
    explicit AuthService(UserRepository& userRepo, StatsRepository& statsRepo);

    AuthResult registerUser(const std::string& name, 
                            const std::string& email, 
                            const std::string& password, 
                            const std::string& phone = "", 
                            const std::string& role = "member");

    AuthResult login(const std::string& identifier, const std::string& password);
    bool logout(const std::string& token);
    std::optional<User> authenticate(const std::string& token);
    std::optional<User> authenticateRequest(const crow::request& req);

    bool hasRole(const User& user, const std::vector<std::string>& allowedRoles);

private:
    UserRepository& m_userRepo;
    StatsRepository& m_statsRepo;
    std::string extractToken(const crow::request& req);
};

} // namespace library
