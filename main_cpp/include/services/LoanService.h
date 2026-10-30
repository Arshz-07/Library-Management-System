#pragma once

#include <string>
#include <vector>
#include <optional>
#include "repositories/LoanRepository.h"
#include "repositories/BookRepository.h"
#include "repositories/UserRepository.h"
#include "repositories/StatsRepository.h"
#include "models/Loan.h"

namespace library {

class LoanService {
public:
    LoanService(LoanRepository& loanRepo, 
                BookRepository& bookRepo, 
                UserRepository& userRepo, 
                StatsRepository& statsRepo);

    int64_t issueBook(int64_t bookId, int64_t userId, int loanDays = 14, const std::string& notes = "", int64_t actingUserId = 0);
    bool returnBook(int64_t loanId, double customFine = -1.0, int64_t actingUserId = 0);
    bool payFine(int64_t loanId, int64_t actingUserId = 0);

    std::optional<Loan> getLoan(int64_t id);
    std::vector<Loan> getUserActiveLoans(int64_t userId);
    std::vector<Loan> getUserLoanHistory(int64_t userId);
    std::vector<Loan> getOverdueLoans();
    std::vector<Loan> getAllLoans(const std::string& statusFilter = "", int64_t userId = 0, int limit = 100);

private:
    LoanRepository& m_loanRepo;
    BookRepository& m_bookRepo;
    UserRepository& m_userRepo;
    StatsRepository& m_statsRepo;
};

} // namespace library
