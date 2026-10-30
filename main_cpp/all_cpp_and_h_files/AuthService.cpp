#include "services/AuthService.h"
#include "core/Security.h"
#include <algorithm>

namespace library {

AuthService::AuthService(UserRepository& userRepo, StatsRepository& statsRepo)
    : m_userRepo(userRepo), m_statsRepo(statsRepo) {}

std::string AuthService::extractToken(const crow::request& req) {
    // 1. Check Authorization header: Bearer <token>
    auto authHeader = req.get_header_value("Authorization");
    if (!authHeader.empty() && authHeader.rfind("Bearer ", 0) == 0) {
        return authHeader.substr(7);
    }

    // 2. Check X-Session-Token header
    auto sessionHeader = req.get_header_value("X-Session-Token");
    if (!sessionHeader.empty()) {
        return sessionHeader;
    }

    // 3. Check Cookie header: session_token=<token>
    auto cookieHeader = req.get_header_value("Cookie");
    if (!cookieHeader.empty()) {
        std::string target = "session_token=";
        size_t pos = cookieHeader.find(target);
        if (pos != std::string::npos) {
            size_t start = pos + target.length();
            size_t end = cookieHeader.find(';', start);
            return cookieHeader.substr(start, (end == std::string::npos) ? std::string::npos : (end - start));
        }
    }

    return "";
}

AuthResult AuthService::registerUser(const std::string& name, 
                                    const std::string& email, 
                                    const std::string& password, 
                                    const std::string& phone, 
                                    const std::string& role) {
    AuthResult res;

    if (name.empty() || email.empty() || password.empty()) {
        res.errorMessage = "Name, email, and password are required.";
        return res;
    }

    if (password.length() < 6) {
        res.errorMessage = "Password must be at least 6 characters long.";
        return res;
    }

    if (m_userRepo.findByEmail(email).has_value()) {
        res.errorMessage = "An account with this email already exists.";
        return res;
    }

    std::string prefix = "MEM";
    if (role == "admin") prefix = "ADM";
    else if (role == "librarian") prefix = "LIB";

    std::string memberCode = Security::generateMemberCode(prefix);
    std::string salt = Security::generateSalt(16);
    std::string hash = Security::hashPassword(password, salt);

    User newUser;
    newUser.memberCode = memberCode;
    newUser.name = name;
    newUser.email = email;
    newUser.phone = phone;
    newUser.passwordHash = hash;
    newUser.salt = salt;
    newUser.role = role.empty() ? "member" : role;
    newUser.status = "active";

    int64_t newId = m_userRepo.create(newUser);
    newUser.id = newId;

    std::string token = Security::generateSessionToken();
    m_userRepo.createSession(token, newId);

    m_statsRepo.logActivity(newId, "MEMBER_REGISTERED", "Registered new account: " + name + " (" + memberCode + ")");

    res.success = true;
    res.token = token;
    res.user = newUser;
    return res;
}

AuthResult AuthService::login(const std::string& identifier, const std::string& password) {
    AuthResult res;

    if (identifier.empty() || password.empty()) {
        res.errorMessage = "Email/Member ID and password are required.";
        return res;
    }

    // Try finding by email first, then member code
    std::optional<User> userOpt = m_userRepo.findByEmail(identifier);
    if (!userOpt) {
        userOpt = m_userRepo.findByMemberCode(identifier);
    }

    if (!userOpt) {
        res.errorMessage = "Invalid credentials. Account not found.";
        return res;
    }

    const auto& user = *userOpt;
    if (user.status == "suspended") {
        res.errorMessage = "Account has been suspended. Please contact the library administration.";
        return res;
    }

    if (!Security::verifyPassword(password, user.salt, user.passwordHash)) {
        res.errorMessage = "Invalid credentials. Incorrect password.";
        return res;
    }

    std::string token = Security::generateSessionToken();
    m_userRepo.createSession(token, user.id);

    m_statsRepo.logActivity(user.id, "USER_LOGIN", user.name + " signed into the system.");

    res.success = true;
    res.token = token;
    res.user = user;
    return res;
}

bool AuthService::logout(const std::string& token) {
    if (token.empty()) return false;
    return m_userRepo.deleteSession(token);
}

std::optional<User> AuthService::authenticate(const std::string& token) {
    if (token.empty()) return std::nullopt;
    return m_userRepo.findUserBySession(token);
}

std::optional<User> AuthService::authenticateRequest(const crow::request& req) {
    std::string token = extractToken(req);
    return authenticate(token);
}

bool AuthService::hasRole(const User& user, const std::vector<std::string>& allowedRoles) {
    for (const auto& r : allowedRoles) {
        if (user.role == r) return true;
    }
    return false;
}

} // namespace library
