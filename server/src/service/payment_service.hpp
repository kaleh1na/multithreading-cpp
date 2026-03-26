#pragma once
#include <cstdint>
#include <future>
#include <string>
#include <utility>
#include <vector>
#include <thread>

#include "model/account.hpp"
#include "model/transaction.hpp"
#include "store/account_store.hpp"
#include "store/transaction_store.hpp"
#include "worker/task_queue.hpp"

class PaymentService {
public:
    PaymentService(AccountStore& accounts, TransactionStore& transactions);
    ~PaymentService();

    uint64_t                 create_account(const std::string& name, AccountType type);
    void                     deposit(uint64_t account_id, int64_t amount);
    [[nodiscard]] std::pair<uint64_t, std::future<Transaction>> pay(uint64_t from_id, uint64_t to_id, int64_t amount);
    Transaction              get_transaction(uint64_t tx_id);
    int64_t                  balance(uint64_t account_id);
    std::vector<Transaction> history(uint64_t account_id);

private:
    AccountStore&     accounts_;
    TransactionStore& transactions_;
    psp::TaskQueue    queue_;
    std::vector<std::thread> workers_;

    void worker_loop();
};
