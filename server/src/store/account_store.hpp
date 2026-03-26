#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <atomic>

#include "model/account.hpp"

class AccountStore {
public:
    uint64_t create(const std::string& name, AccountType type);
    std::shared_ptr<Account> find(uint64_t account_id);

private:
    std::unordered_map<uint64_t, std::shared_ptr<Account>> accounts_;
    mutable std::shared_mutex                               mtx_;
    std::atomic<uint64_t>                                   next_id_{1};
};
