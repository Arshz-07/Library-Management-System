#pragma once

#include <string>

namespace library {

struct Config {
    static constexpr int PORT = 8080;
    static inline const std::string DB_PATH = "library.db";
    static inline const std::string STATIC_DIR = "./public";
    
    // Business rules
    static constexpr int DEFAULT_LOAN_DAYS = 14;
    static constexpr double DAILY_FINE_RATE = 0.50; // $0.50 per day overdue
    static constexpr int MAX_ACTIVE_LOANS_PER_MEMBER = 5;

    // Seed defaults
    static inline const std::string ADMIN_DEFAULT_USER = "admin";
    static inline const std::string ADMIN_DEFAULT_PASS = "admin123";
    static inline const std::string ADMIN_DEFAULT_EMAIL = "admin@librosphere.org";
    
    static inline const std::string LIBRARIAN_DEFAULT_USER = "sarah_lib";
    static inline const std::string LIBRARIAN_DEFAULT_PASS = "lib123";
    static inline const std::string LIBRARIAN_DEFAULT_EMAIL = "sarah@librosphere.org";

    static inline const std::string DEMO_STUDENT_USER = "alice_reader";
    static inline const std::string DEMO_STUDENT_PASS = "alice123";
    static inline const std::string DEMO_STUDENT_EMAIL = "alice@university.edu";
};

} // namespace library
