#include <iostream>
#include <filesystem>
#include <crow.h>

#include "core/Config.h"
#include "core/DatabaseManager.h"
#include "seed/DatabaseSeeder.h"
#include "repositories/UserRepository.h"
#include "repositories/BookRepository.h"
#include "repositories/LoanRepository.h"
#include "repositories/StatsRepository.h"
#include "services/AuthService.h"
#include "services/BookService.h"
#include "services/LoanService.h"
#include "services/MemberService.h"
#include "controllers/Controllers.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n";
    std::cout << "       LibroSphere - Modern Library Management System   \n";
    std::cout << "       C++ REST Backend + SQLite + HTML5 / CSS3 / JS    \n";
    std::cout << "========================================================\n\n";

    try {
        // 1. Locate public static folder
        std::string publicDir = library::Config::STATIC_DIR;
        if (!fs::exists(publicDir)) {
            if (fs::exists("../public")) {
                publicDir = "../public";
            } else if (argc > 0) {
                fs::path exePath = fs::canonical(argv[0]).parent_path();
                if (fs::exists(exePath / "public")) {
                    publicDir = (exePath / "public").string();
                }
            }
        }
        std::cout << "[Server] Serving static frontend from: " << fs::absolute(publicDir) << "\n";

        // 2. Initialize Persistent SQLite Database & Schema (RAII)
        std::cout << "[Server] Connecting to database: " << library::Config::DB_PATH << "...\n";
        library::DatabaseManager db(library::Config::DB_PATH);
        db.initializeSchema();

        // 3. Seed Database with Initial Curated Catalog and Accounts
        library::DatabaseSeeder::seedIfEmpty(db);

        // 4. Instantiate Repositories
        library::UserRepository userRepo(db);
        library::BookRepository bookRepo(db);
        library::LoanRepository loanRepo(db, library::Config::DAILY_FINE_RATE);
        library::StatsRepository statsRepo(db);

        // 5. Instantiate Business Services
        library::AuthService authService(userRepo, statsRepo);
        library::BookService bookService(bookRepo, statsRepo);
        library::LoanService loanService(loanRepo, bookRepo, userRepo, statsRepo);
        library::MemberService memberService(userRepo, loanRepo, statsRepo);

        // 6. Initialize Crow Web Application
        crow::SimpleApp app;

        // Register REST API and Static File Routes
        library::registerAuthRoutes(app, authService);
        library::registerBookRoutes(app, bookService, authService);
        library::registerLoanRoutes(app, loanService, authService);
        library::registerMemberRoutes(app, memberService, authService);
        library::registerDashboardRoutes(app, statsRepo, bookRepo, userRepo, loanRepo, authService);
        library::registerStaticRoutes(app, publicDir);

        std::cout << "\n>>> LibroSphere Backend running on http://localhost:" << library::Config::PORT << " <<<\n";
        std::cout << "Default Admin Account:     " << library::Config::ADMIN_DEFAULT_USER 
                  << " / " << library::Config::ADMIN_DEFAULT_PASS << "\n";
        std::cout << "Default Librarian Account: " << library::Config::LIBRARIAN_DEFAULT_USER 
                  << " / " << library::Config::LIBRARIAN_DEFAULT_PASS << "\n";
        std::cout << "Default Student Account:   " << library::Config::DEMO_STUDENT_USER 
                  << " / " << library::Config::DEMO_STUDENT_PASS << "\n\n";

        app.port(library::Config::PORT)
           .multithreaded()
           .run();

    } catch (const std::exception& ex) {
        std::cerr << "[CRITICAL ERROR] " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
