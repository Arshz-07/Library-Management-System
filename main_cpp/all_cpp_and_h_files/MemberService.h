#pragma once

#include <string>
#include <vector>
#include <optional>
#include "repositories/UserRepository.h"
#include "repositories/LoanRepository.h"
#include "repositories/StatsRepository.h"
#include "models/User.h"

namespace library {

class MemberService {
public:
    MemberService(UserRepository& userRepo, LoanRepository& loanRepo, StatsRepository& statsRepo);

    std::vector<User> listMembers(const std::string& search = "");
    std::optional<User> getMember(int64_t id);
    bool updateStatus(int64_t id, const std::string& status, int64_t actingUserId = 0);
    bool updateProfile(int64_t id, const std::string& name, const std::string& phone);
    bool deleteMember(int64_t id, int64_t actingUserId = 0);

private:
    UserRepository& m_userRepo;
    LoanRepository& m_loanRepo;
    StatsRepository& m_statsRepo;
};

} // namespace library
