#pragma once
#include <cstdint>
#include <string>
#include <mutex>

enum class AccountType { Payer, Merchant };

struct Account {
    uint64_t    account_id;
    std::string name;
    AccountType type;
    int64_t     balance;
    std::mutex  mtx;

    Account(uint64_t id, std::string n, AccountType t)
        : account_id(id), name(std::move(n)), type(t), balance(0) {}
};
