#include "store/transaction_store.hpp"

uint64_t TransactionStore::save(Transaction tx) {
    tx.transaction_id = next_id_.fetch_add(1);
    tx.created_at = std::chrono::system_clock::now();
    std::lock_guard lock(mtx_);
    transactions_.push_back(std::move(tx));
    return transactions_.back().transaction_id;
}

void TransactionStore::update(uint64_t tx_id, TransactionStatus status, const std::string& fail_reason) {
    std::lock_guard lock(mtx_);
    for (auto& tx : transactions_) {
        if (tx.transaction_id == tx_id) {
            tx.status      = status;
            tx.fail_reason = fail_reason;
            return;
        }
    }
}

std::optional<Transaction> TransactionStore::find(uint64_t tx_id) const {
    std::lock_guard lock(mtx_);
    for (const auto& tx : transactions_) {
        if (tx.transaction_id == tx_id) {
            return tx;
        }
    }
    return std::nullopt;
}

std::vector<Transaction> TransactionStore::history(uint64_t account_id) const {
    std::lock_guard lock(mtx_);
    std::vector<Transaction> result;
    for (const auto& tx : transactions_) {
        if (tx.from_account_id == account_id || tx.to_account_id == account_id) {
            result.push_back(tx);
        }
    }
    return result;
}
