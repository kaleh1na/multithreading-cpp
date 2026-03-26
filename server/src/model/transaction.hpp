#pragma once
#include <cstdint>
#include <string>
#include <chrono>

enum class TransactionStatus { Pending, Completed, Failed };

struct Transaction {
    uint64_t          transaction_id;
    uint64_t          from_account_id;
    uint64_t          to_account_id;
    int64_t           amount;
    TransactionStatus status;
    std::string       fail_reason;
    std::chrono::system_clock::time_point created_at;
};
