#pragma once
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include <optional>
#include <string>

#include "model/transaction.hpp"

class TransactionStore {
public:
    [[nodiscard]] uint64_t save(Transaction tx);
    void update(uint64_t tx_id, TransactionStatus status, const std::string& fail_reason = "");
    std::optional<Transaction> find(uint64_t tx_id) const;
    std::vector<Transaction> history(uint64_t account_id) const;

private:
    std::vector<Transaction>  transactions_;
    mutable std::mutex        mtx_;
    std::atomic<uint64_t>     next_id_{1};
};
