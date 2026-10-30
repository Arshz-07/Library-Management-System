#pragma once

#include <string>

namespace library {

class Security {
public:
    // Generate random hex salt
    static std::string generateSalt(size_t length = 16);

    // Generate session token (64-character hex)
    static std::string generateSessionToken();

    // Generate unique member code (e.g. MEM-7492, LIB-3821)
    static std::string generateMemberCode(const std::string& prefix = "MEM");

    // Salted SHA-256 hash with iterations
    static std::string hashPassword(const std::string& password, const std::string& salt, int iterations = 10000);

    // Verify password against stored hash and salt
    static bool verifyPassword(const std::string& password, const std::string& storedSalt, const std::string& storedHash);

    // Constant time string comparison
    static bool constantTimeEquals(const std::string& a, const std::string& b);

    // Single SHA-256 hash
    static std::string sha256(const std::string& input);
};

} // namespace library
