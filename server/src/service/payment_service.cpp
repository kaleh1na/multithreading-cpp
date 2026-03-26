#include "service/payment_service.hpp"
#include "error.hpp"

#include <algorithm>
#include <future>
#include <memory>

PaymentService::PaymentService(AccountStore& accounts, TransactionStore& transactions)
    : accounts_(accounts), transactions_(transactions) {
    const unsigned int n = std::max(2u, std::thread::hardware_concurrency());
    workers_.reserve(n);
    for (unsigned int i = 0; i < n; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

PaymentService::~PaymentService() {
    queue_.stop();
    for (auto& t : workers_) {
        t.join();
    }
}

void PaymentService::worker_loop() {
    while (auto task = queue_.dequeue()) {
        (*task)();
    }
}

uint64_t PaymentService::create_account(const std::string& name, AccountType type) {
    return accounts_.create(name, type);
}

void PaymentService::deposit(uint64_t account_id, int64_t amount) {
    if (amount <= 0) {
        throw InvalidAmount{};
    }
    auto acc = accounts_.find(account_id);
    if (!acc) {
        throw AccountNotFound{};
    }
    if (acc->type != AccountType::Payer) {
        throw InvalidAccountType{};
    }
    std::lock_guard lock(acc->mtx);
    acc->balance += amount;
}

std::pair<uint64_t, std::future<Transaction>> PaymentService::pay(uint64_t from_id, uint64_t to_id, int64_t amount) {
    if (amount <= 0) {
        throw InvalidAmount{};
    }
    auto from = accounts_.find(from_id);
    auto to   = accounts_.find(to_id);
    if (!from || !to) {
        throw AccountNotFound{};
    }
    if (from->type != AccountType::Payer) {
        throw InvalidAccountType{};
    }

    Transaction tx{};
    tx.from_account_id = from_id;
    tx.to_account_id   = to_id;
    tx.amount          = amount;
    tx.status          = TransactionStatus::Pending;

    const uint64_t tx_id = transactions_.save(tx);

    auto promise = std::make_shared<std::promise<Transaction>>();
    auto future  = promise->get_future();

    queue_.enqueue([this, from, to, tx_id, amount, promise = std::move(promise)] {
        std::scoped_lock lock(from->mtx, to->mtx);
        if (from->balance < amount) {
            transactions_.update(tx_id, TransactionStatus::Failed, "insufficient_funds");
        } else {
            from->balance -= amount;
            to->balance   += amount;
            transactions_.update(tx_id, TransactionStatus::Completed);
        }
        auto opt = transactions_.find(tx_id);
        promise->set_value(*opt);
    });

    return std::make_pair(tx_id, std::move(future));
}

Transaction PaymentService::get_transaction(uint64_t tx_id) {
    auto opt = transactions_.find(tx_id);
    if (!opt) {
        throw TransactionNotFound{};
    }
    return *opt;
}

int64_t PaymentService::balance(uint64_t account_id) {
    auto acc = accounts_.find(account_id);
    if (!acc) {
        throw AccountNotFound{};
    }
    std::lock_guard lock(acc->mtx);
    return acc->balance;
}

std::vector<Transaction> PaymentService::history(uint64_t account_id) {
    if (!accounts_.find(account_id)) {
        throw AccountNotFound{};
    }
    return transactions_.history(account_id);
}
