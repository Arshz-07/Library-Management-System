#include "services/LoanService.h"
#include "core/Config.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace library {

LoanService::LoanService(LoanRepository& loanRepo, 
                         BookRepository& bookRepo, 
                         UserRepository& userRepo, 
                         StatsRepository& statsRepo)
    : m_loanRepo(loanRepo), m_bookRepo(bookRepo), m_userRepo(userRepo), m_statsRepo(statsRepo) {}

int64_t LoanService::issueBook(int64_t bookId, int64_t userId, int loanDays, const std::string& notes, int64_t actingUserId) {
    auto book = m_bookRepo.findById(bookId);
    if (!book) {
        throw std::invalid_argument("Book not found.");
    }

    if (book->availableCopies <= 0) {
        throw std::invalid_argument("No copies available for '" + book->title + "'.");
    }

    auto user = m_userRepo.findById(userId);
    if (!user) {
        throw std::invalid_argument("Member not found.");
    }

    if (user->status == "suspended") {
        throw std::invalid_argument("Member account is suspended and cannot borrow books.");
    }

    // Check active loan limit for members
    if (user->role == "member") {
        int currentLoans = m_loanRepo.countActiveLoans(userId);
        if (currentLoans >= Config::MAX_ACTIVE_LOANS_PER_MEMBER) {
            throw std::invalid_argument("Borrowing limit reached. Member already has " + 
                                        std::to_string(currentLoans) + " active loans.");
        }
    }

    // Check if user already holds a copy of this book
    auto active = m_loanRepo.findActiveLoan(bookId, userId);
    if (active) {
        throw std::invalid_argument("Member already has a copy of this book on loan.");
    }

    int days = (loanDays > 0 && loanDays <= 60) ? loanDays : Config::DEFAULT_LOAN_DAYS;
    int64_t loanId = m_loanRepo.issueLoan(bookId, userId, days, notes);

    m_statsRepo.logActivity(actingUserId > 0 ? actingUserId : userId, "LOAN_ISSUED", 
                           "Issued '" + book->title + "' to " + user->name + " (" + user->memberCode + ")");

    return loanId;
}

bool LoanService::returnBook(int64_t loanId, double customFine, int64_t actingUserId) {
    auto loan = m_loanRepo.findById(loanId);
    if (!loan) {
        throw std::invalid_argument("Loan record not found.");
    }

    if (loan->status == "returned") {
        throw std::invalid_argument("This book has already been returned.");
    }

    bool ok = m_loanRepo.returnLoan(loanId, customFine);
    if (ok) {
        std::ostringstream msg;
        msg << "Returned '" << loan->bookTitle << "' from " << loan->userName;
        if (loan->isOverdue) {
            msg << " (Overdue. Fine calculated)";
        }
        m_statsRepo.logActivity(actingUserId > 0 ? actingUserId : loan->userId, "LOAN_RETURNED", msg.str());
    }
    return ok;
}

bool LoanService::payFine(int64_t loanId, int64_t actingUserId) {
    auto loan = m_loanRepo.findById(loanId);
    if (!loan) return false;

    bool ok = m_loanRepo.markFinePaid(loanId);
    if (ok) {
        std::ostringstream msg;
        msg << "Fine of $" << std::fixed << std::setprecision(2) << loan->fineAmount 
            << " settled for " << loan->userName << " (" << loan->bookTitle << ")";
        m_statsRepo.logActivity(actingUserId, "FINE_PAID", msg.str());
    }
    return ok;
}

std::optional<Loan> LoanService::getLoan(int64_t id) {
    return m_loanRepo.findById(id);
}

std::vector<Loan> LoanService::getUserActiveLoans(int64_t userId) {
    return m_loanRepo.getActiveLoans(userId);
}

std::vector<Loan> LoanService::getUserLoanHistory(int64_t userId) {
    return m_loanRepo.getAllLoans("", userId, 50);
}

std::vector<Loan> LoanService::getOverdueLoans() {
    return m_loanRepo.getOverdueLoans();
}

std::vector<Loan> LoanService::getAllLoans(const std::string& statusFilter, int64_t userId, int limit) {
    return m_loanRepo.getAllLoans(statusFilter, userId, limit);
}

} // namespace library
