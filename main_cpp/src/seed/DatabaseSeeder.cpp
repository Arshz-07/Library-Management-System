#include "seed/DatabaseSeeder.h"
#include "core/Config.h"
#include "core/Security.h"
#include <iostream>

namespace library {

void DatabaseSeeder::seedIfEmpty(DatabaseManager& db) {
    // Check if books are already seeded
    {
        auto stmt = db.prepare("SELECT COUNT(*) FROM books;");
        if (stmt.step() && stmt.getInt(0) > 0) {
            std::cout << "[Seeder] Database already populated. Skipping seed." << std::endl;
            return;
        }
    }

    std::cout << "[Seeder] Seeding comprehensive library catalog and accounts..." << std::endl;
    auto tx = db.beginTransaction();

    // 1. Seed Categories
    struct CatDef { const char* name; const char* desc; const char* icon; };
    CatDef categories[] = {
        {"Computer Science & Tech", "Software engineering, algorithms, AI, cloud computing, and hardware systems", "💻"},
        {"Literature & Fiction", "Classic literature, modern masterpieces, dystopian novels, and science fiction", "📖"},
        {"Science & Mathematics", "Theoretical physics, astronomy, genetics, calculus, and natural sciences", "🔬"},
        {"History & Biography", "World history, military conflicts, and memoirs of visionary historical leaders", "🏛️"},
        {"Business & Economics", "Entrepreneurship, venture capital, personal finance, and market strategy", "📈"},
        {"Philosophy & Psychology", "Cognitive psychology, stoicism, behavioral economics, and human nature", "🧠"}
    };

    for (const auto& c : categories) {
        auto stmt = db.prepare("INSERT INTO categories (name, description, icon) VALUES (?, ?, ?);");
        stmt.bindString(1, c.name);
        stmt.bindString(2, c.desc);
        stmt.bindString(3, c.icon);
        stmt.step();
    }

    // 2. Seed Default Accounts
    struct UserDef {
        const char* code;
        const char* name;
        const char* email;
        const char* phone;
        const char* password;
        const char* role;
    };

    UserDef users[] = {
        {"ADM-001", "Chief Librarian & Admin", "admin@librosphere.org", "+1-555-0100", "admin123", "admin"},
        {"LIB-101", "Sarah Jenkins", "sarah@librosphere.org", "+1-555-0145", "lib123", "librarian"},
        {"MEM-201", "Alice Reader", "alice@university.edu", "+1-555-0188", "alice123", "member"},
        {"MEM-202", "John Doe", "john@university.edu", "+1-555-0192", "john123", "member"}
    };

    for (const auto& u : users) {
        std::string salt = Security::generateSalt(16);
        std::string hash = Security::hashPassword(u.password, salt);

        auto stmt = db.prepare(R"(
            INSERT INTO users (member_code, name, email, phone, password_hash, salt, role, status)
            VALUES (?, ?, ?, ?, ?, ?, ?, 'active');
        )");
        stmt.bindString(1, u.code);
        stmt.bindString(2, u.name);
        stmt.bindString(3, u.email);
        stmt.bindString(4, u.phone);
        stmt.bindString(5, hash);
        stmt.bindString(6, salt);
        stmt.bindString(7, u.role);
        stmt.step();
    }

    // 3. Seed Realistic Books Catalog
    struct BookDef {
        const char* isbn;
        const char* title;
        const char* author;
        int categoryId;
        const char* publisher;
        int year;
        int copies;
        const char* shelf;
        const char* cover;
        const char* desc;
    };

    BookDef books[] = {
        {
            "9780132350884",
            "Clean Code: A Handbook of Agile Software Craftsmanship",
            "Robert C. Martin",
            1, "Prentice Hall", 2008, 5, "Stack CS-01",
            "https://images.unsplash.com/photo-1516259762381-22954d7d3ad2?w=500&auto=format&fit=crop&q=80",
            "Even bad code can function. But if code isn't clean, it can bring a development organization to its knees. A timeless guide to writing readable, maintainable, and elegant software."
        },
        {
            "9780135957059",
            "The Pragmatic Programmer: Your Journey To Mastery (20th Anniv)",
            "David Thomas, Andrew Hunt",
            1, "Addison-Wesley", 2019, 4, "Stack CS-02",
            "https://images.unsplash.com/photo-1555066931-4365d14bab8c?w=500&auto=format&fit=crop&q=80",
            "The Pragmatic Programmer cuts through the increasing specialization and technicalities of modern software development to examine the core process: taking a requirement and producing working, maintainable code."
        },
        {
            "9780201633610",
            "Design Patterns: Elements of Reusable Object-Oriented Software",
            "Erich Gamma, Richard Helm, Ralph Johnson, John Vlissides",
            1, "Addison-Wesley", 1994, 4, "Stack CS-03",
            "https://images.unsplash.com/photo-1526374965328-7f61d4dc18c5?w=500&auto=format&fit=crop&q=80",
            "Capturing a wealth of experience about the design of object-oriented software, four top-notch designers present a catalog of simple and succinct solutions to commonly occurring design problems."
        },
        {
            "9780262046305",
            "Introduction to Algorithms (CLRS 4th Edition)",
            "Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, Clifford Stein",
            1, "MIT Press", 2022, 3, "Stack CS-04",
            "https://images.unsplash.com/photo-1509228468518-180dd4864904?w=500&auto=format&fit=crop&q=80",
            "The leading algorithms text worldwide, comprehensively covering data structures, dynamic programming, graph algorithms, NP-completeness, and modern parallel algorithms."
        },
        {
            "9781119800361",
            "Operating System Concepts (10th Edition)",
            "Abraham Silberschatz, Peter B. Galvin, Greg Gagne",
            1, "Wiley", 2018, 4, "Stack CS-05",
            "https://images.unsplash.com/photo-1544197150-b99a580bb7a8?w=500&auto=format&fit=crop&q=80",
            "The dinosaur book provides a clear description of the concepts that underlie operating systems: processes, threads, virtual memory, file systems, and concurrency."
        },
        {
            "9780136681557",
            "Computer Networking: A Top-Down Approach (8th Edition)",
            "James F. Kurose, Keith W. Ross",
            1, "Pearson", 2020, 3, "Stack CS-06",
            "https://images.unsplash.com/photo-1544716278-ca5e3f4abd8c?w=500&auto=format&fit=crop&q=80",
            "Focuses on the Internet and fundamental networking principles through a layered top-down approach, exploring HTTP, DNS, TCP/IP, and wireless protocols."
        },
        {
            "9780441013593",
            "Dune",
            "Frank Herbert",
            2, "Chilton Books", 1965, 4, "Aisle LIT-10",
            "https://images.unsplash.com/photo-1506466010722-395aa2bef877?w=500&auto=format&fit=crop&q=80",
            "Set on the desert planet Arrakis, Dune is the story of the boy Paul Atreides, heir to a noble family in a sweeping epic of politics, religion, ecology, and human destiny."
        },
        {
            "9780451524935",
            "1984",
            "George Orwell",
            2, "Secker & Warburg", 1949, 6, "Aisle LIT-11",
            "https://images.unsplash.com/photo-1497633762265-9d179a990aa6?w=500&auto=format&fit=crop&q=80",
            "Winston Smith toes the Party line, rewriting history to satisfy the Ministry of Truth. With every lie he writes, he grows to secretly despise the omnipresent Party that seeks power for its own sake."
        },
        {
            "9780061120084",
            "To Kill a Mockingbird",
            "Harper Lee",
            2, "J. B. Lippincott & Co.", 1960, 5, "Aisle LIT-12",
            "https://images.unsplash.com/photo-1476275466078-4007374efbbe?w=500&auto=format&fit=crop&q=80",
            "A gripping, heart-wrenching, and wholly remarkable tale of coming-of-age in a South poisoned by virulent prejudice, viewed through the eyes of young Scout Finch."
        },
        {
            "9780743273565",
            "The Great Gatsby",
            "F. Scott Fitzgerald",
            2, "Charles Scribner's Sons", 1925, 5, "Aisle LIT-13",
            "https://images.unsplash.com/photo-1495446815901-a7297e633e8d?w=500&auto=format&fit=crop&q=80",
            "The exemplary novel of the Jazz Age, recounting the tragic romance and hollow illusion of the mysterious Jay Gatsby in West Egg, Long Island."
        },
        {
            "9780553380163",
            "A Brief History of Time",
            "Stephen Hawking",
            3, "Bantam Books", 1988, 4, "Hall SCI-01",
            "https://images.unsplash.com/photo-1451187580459-43490279c0fa?w=500&auto=format&fit=crop&q=80",
            "A landmark volume in science writing by one of the greatest minds of our time, exploring black holes, gravitational singularities, the Big Bang, and the nature of time."
        },
        {
            "9780345539434",
            "Cosmos",
            "Carl Sagan",
            3, "Random House", 1980, 4, "Hall SCI-02",
            "https://images.unsplash.com/photo-1446776811953-b23d57bd21aa?w=500&auto=format&fit=crop&q=80",
            "The magnificent mystery of the Universe, tracing fifteen billion years of cosmic evolution that transformed matter into consciousness and curiosity."
        },
        {
            "9780199291151",
            "The Selfish Gene (40th Anniversary Edition)",
            "Richard Dawkins",
            3, "Oxford University Press", 2016, 3, "Hall SCI-03",
            "https://images.unsplash.com/photo-1532094349884-543bc11b234d?w=500&auto=format&fit=crop&q=80",
            "Dawkins' brilliant reformulation of the theory of natural selection from the gene's point of view, introducing the revolutionary concept of memes."
        },
        {
            "9780062316097",
            "Sapiens: A Brief History of Humankind",
            "Yuval Noah Harari",
            4, "Harper", 2015, 6, "Wing HIST-01",
            "https://images.unsplash.com/photo-1461360370896-922624d12aa1?w=500&auto=format&fit=crop&q=80",
            "From a renowned historian comes a groundbreaking narrative of humanity's creation and evolution: how biology and history have defined us."
        },
        {
            "9780393354324",
            "Guns, Germs, and Steel: The Fates of Human Societies",
            "Jared Diamond",
            4, "W. W. Norton & Company", 1997, 3, "Wing HIST-02",
            "https://images.unsplash.com/photo-1447069387593-a5de0862481e?w=500&auto=format&fit=crop&q=80",
            "A major work explaining why Eurasian and North African civilizations have survived and conquered others, refuting racist biological theories."
        },
        {
            "9780735211292",
            "Atomic Habits: An Easy & Proven Way to Build Good Habits",
            "James Clear",
            5, "Avery", 2018, 7, "Tower BIZ-01",
            "https://images.unsplash.com/photo-1434030216411-0b793f4b4173?w=500&auto=format&fit=crop&q=80",
            "No matter your goals, Atomic Habits offers a proven framework for improving every day: how tiny changes in system architecture lead to remarkable compounding results."
        },
        {
            "9780374533557",
            "Thinking, Fast and Slow",
            "Daniel Kahneman",
            6, "Farrar, Straus and Giroux", 2011, 5, "Alcove PSY-01",
            "https://images.unsplash.com/photo-1507679799987-c73779587ccf?w=500&auto=format&fit=crop&q=80",
            "Nobel laureate Daniel Kahneman takes us on an unprecedented tour of the mind, explaining the two systems that drive the way we think: fast intuitive thinking, and slow deliberate logic."
        },
        {
            "9780804139298",
            "Zero to One: Notes on Startups, or How to Build the Future",
            "Peter Thiel, Blake Masters",
            5, "Crown Business", 2014, 4, "Tower BIZ-02",
            "https://images.unsplash.com/photo-1559526324-4b87b5e36e44?w=500&auto=format&fit=crop&q=80",
            "The great secret of our time is that there are still uncharted frontiers to explore and new inventions to create. A masterclass in contrarian thinking and monopoly building."
        },
        {
            "9780857197689",
            "The Psychology of Money: Timeless Lessons on Wealth and Greed",
            "Morgan Housel",
            5, "Harriman House", 2020, 5, "Tower BIZ-03",
            "https://images.unsplash.com/photo-1526304640581-d334cdbbf45e?w=500&auto=format&fit=crop&q=80",
            "Doing well with money isn't necessarily about what you know. It's about how you behave. 19 short stories exploring the strange ways people think about wealth."
        },
        {
            "9780140449334",
            "Meditations",
            "Marcus Aurelius",
            6, "Penguin Classics", 2006, 5, "Alcove PSY-02",
            "https://images.unsplash.com/photo-1457369804613-52c61a468e7d?w=500&auto=format&fit=crop&q=80",
            "The private reflections of the Roman Emperor Marcus Aurelius on Stoic philosophy, personal resilience, emotional duty, and cosmic purpose."
        }
    };

    for (const auto& b : books) {
        auto stmt = db.prepare(R"(
            INSERT INTO books (isbn, title, author, category_id, publisher, publish_year,
                               total_copies, available_copies, shelf_location, cover_url, description)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
        )");
        stmt.bindString(1, b.isbn);
        stmt.bindString(2, b.title);
        stmt.bindString(3, b.author);
        stmt.bindInt(4, b.categoryId);
        stmt.bindString(5, b.publisher);
        stmt.bindInt(6, b.year);
        stmt.bindInt(7, b.copies);
        stmt.bindInt(8, b.copies); // initially all available, loans below will decrement
        stmt.bindString(9, b.shelf);
        stmt.bindString(10, b.cover);
        stmt.bindString(11, b.desc);
        stmt.step();
    }

    // 4. Seed Initial Interactive Loans
    // Loan 1: Alice has "Clean Code" (Book ID 1, User ID 3), active, borrowed 3 days ago, due in 11 days
    {
        db.execute(R"(
            INSERT INTO loans (book_id, user_id, borrow_date, due_date, status, notes)
            VALUES (1, 3, datetime('now', '-3 days'), datetime('now', '+11 days'), 'borrowed', 'Requested for BCA OOP Project study');
            UPDATE books SET available_copies = available_copies - 1 WHERE id = 1;
        )");
    }

    // Loan 2: John has "Dune" (Book ID 7, User ID 4), OVERDUE! Borrowed 25 days ago, due 11 days ago. Fine = 11 * 0.50 = $5.50
    {
        db.execute(R"(
            INSERT INTO loans (book_id, user_id, borrow_date, due_date, fine_amount, fine_paid, status, notes)
            VALUES (7, 4, datetime('now', '-25 days'), datetime('now', '-11 days'), 5.50, 0, 'overdue', 'Overdue notice sent by email');
            UPDATE books SET available_copies = available_copies - 1 WHERE id = 7;
        )");
    }

    // Loan 3: Alice previously borrowed "1984" (Book ID 8, User ID 3) and returned it on time!
    {
        db.execute(R"(
            INSERT INTO loans (book_id, user_id, borrow_date, due_date, return_date, fine_amount, fine_paid, status, notes)
            VALUES (8, 3, datetime('now', '-30 days'), datetime('now', '-16 days'), datetime('now', '-17 days'), 0.0, 1, 'returned', 'Returned in excellent condition');
        )");
    }

    // 5. Seed Initial Activity Logs
    db.execute(R"(
        INSERT INTO activity_logs (user_id, action, details) VALUES
        (1, 'SYSTEM_INIT', 'LibroSphere database initialized with 20 curated volumes across 6 disciplines.'),
        (3, 'LOAN_ISSUED', 'Alice Reader borrowed Clean Code: A Handbook of Agile Software Craftsmanship.'),
        (4, 'OVERDUE_ALERT', 'Loan for Dune (John Doe) passed due date by 11 days.'),
        (3, 'LOAN_RETURNED', 'Alice Reader returned 1984 on time.');
    )");

    tx.commit();
    std::cout << "[Seeder] Database seeded successfully with books, users, categories, and circulation loans!" << std::endl;
}

} // namespace library
