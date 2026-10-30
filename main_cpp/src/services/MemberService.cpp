#include "services/MemberService.h"
#include <stdexcept>

namespace library {

MemberService::MemberService(UserRepository& userRepo, LoanRepository& loanRepo, StatsRepository& statsRepo)
    : m_userRepo(userRepo), m_loanRepo(loanRepo), m_statsRepo(statsRepo) {}

std::vector<User> MemberService::listMembers(const std::string& search) {
    return m_userRepo.getAll(search);
}

std::optional<User> MemberService::getMember(int64_t id) {
    return m_userRepo.findById(id);
}

bool MemberService::updateStatus(int64_t id, const std::string& status, int64_t actingUserId) {
    auto user = m_userRepo.findById(id);
    if (!user) return false;

    if (user->role == "admin" && status == "suspended") {
        throw std::invalid_argument("Cannot suspend an administrator account.");
    }

    bool ok = m_userRepo.updateStatus(id, status);
    if (ok) {
        m_statsRepo.logActivity(actingUserId, "MEMBER_STATUS_CHANGED", 
                               "Changed status of " + user->name + " to " + status);
    }
    return ok;
}

bool MemberService::updateProfile(int64_t id, const std::string& name, const std::string& phone) {
    if (name.empty()) {
        throw std::invalid_argument("Member name cannot be empty.");
    }
    return m_userRepo.updateProfile(id, name, phone);
}

bool MemberService::deleteMember(int64_t id, int64_t actingUserId) {
    auto user = m_userRepo.findById(id);
    if (!user) return false;

    if (user->role == "admin") {
        throw std::invalid_argument("Cannot delete administrator account.");
    }

    int activeLoans = m_loanRepo.countActiveLoans(id);
    if (activeLoans > 0) {
        throw std::invalid_argument("Cannot delete member who has " + std::to_string(activeLoans) + " active book loans.");
    }

    bool ok = m_userRepo.deleteUser(id);
    if (ok) {
        m_statsRepo.logActivity(actingUserId, "MEMBER_DELETED", "Deleted member: " + user->name);
    }
    return ok;
}

} // namespace library
