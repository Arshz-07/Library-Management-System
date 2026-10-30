#pragma once

#include <string>
#include <vector>
#include <optional>
#include "core/DatabaseManager.h"
#include "models/Loan.h"

namespace library {

class LoanRepository {
public:
    explicit LoanRepository(DatabaseManager& db, double dailyFineRate = 0.50);

    // Issue a book
    int64_t issueLoan(int64_t bookId, int64_t userId, int loanDays = 14, const std::string& notes = "");

    // Return a book
    bool returnLoan(int64_t loanId, double customFine = -1.0);

    // Find loan by ID
    std::optional<Loan> findById(int64_t id);

    // Find active loan by book and user
    std::optional<Loan> findActiveLoan(int64_t bookId, int64_t userId);

    // Lists
    std::vector<Loan> getActiveLoans(int64_t userId = 0);
    std::vector<Loan> getOverdueLoans();
    std::vector<Loan> getAllLoans(const std::string& statusFilter = "", int64_t userId = 0, int limit = 100);

    // Fine management
    bool markFinePaid(int64_t loanId);
    double getTotalPendingFines(int64_t userId = 0);
    double getTotalCollectedFines();

    // Counts
    int countActiveLoans(int64_t userId = 0);
    int countOverdueLoans();
    int countReturnedLoans();

    // Auto update overdue status & fines
    void refreshOverdueStatuses();

private:
    DatabaseManager& m_db;
    double m_fineRate;
    Loan mapRow(const SQLiteStatement& stmt);
};

} // namespace library
