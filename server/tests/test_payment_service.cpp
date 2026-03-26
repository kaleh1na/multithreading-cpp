#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <future>

#include "store/account_store.hpp"
#include "store/transaction_store.hpp"
#include "service/payment_service.hpp"
#include "error.hpp"

struct PaymentServiceFixture : ::testing::Test {
    AccountStore     accounts;
    TransactionStore transactions;
    PaymentService   service{accounts, transactions};
};

TEST_F(PaymentServiceFixture, DepositIncreasesBalance) {
    const uint64_t id = service.create_account("Alice", AccountType::Payer);
    service.deposit(id, 500);
    EXPECT_EQ(service.balance(id), 500);
}

TEST_F(PaymentServiceFixture, DepositInvalidAmountThrows) {
    const uint64_t id = service.create_account("Alice", AccountType::Payer);
    EXPECT_THROW(service.deposit(id, 0), InvalidAmount);
    EXPECT_THROW(service.deposit(id, -10), InvalidAmount);
}

TEST_F(PaymentServiceFixture, PayReturnsPendingImmediately) {
    const uint64_t payer    = service.create_account("Payer",    AccountType::Payer);
    const uint64_t merchant = service.create_account("Merchant", AccountType::Merchant);
    service.deposit(payer, 1000);

    auto [tx_id, fut] = service.pay(payer, merchant, 300);

    EXPECT_GT(tx_id, 0u);
    EXPECT_TRUE(fut.valid());
}

TEST_F(PaymentServiceFixture, PaySuccessful) {
    const uint64_t payer    = service.create_account("Payer",    AccountType::Payer);
    const uint64_t merchant = service.create_account("Merchant", AccountType::Merchant);
    service.deposit(payer, 1000);

    auto [tx_id, fut] = service.pay(payer, merchant, 300);
    const Transaction tx = fut.get();

    EXPECT_GT(tx_id, 0u);
    EXPECT_EQ(tx.status, TransactionStatus::Completed);
    EXPECT_EQ(service.balance(payer),    700);
    EXPECT_EQ(service.balance(merchant), 300);
}

TEST_F(PaymentServiceFixture, PayInsufficientFunds) {
    const uint64_t payer    = service.create_account("Payer",    AccountType::Payer);
    const uint64_t merchant = service.create_account("Merchant", AccountType::Merchant);
    service.deposit(payer, 50);

    auto [tx_id, fut] = service.pay(payer, merchant, 100);
    const Transaction tx = fut.get();

    EXPECT_GT(tx_id, 0u);
    EXPECT_EQ(tx.status, TransactionStatus::Failed);
    EXPECT_EQ(tx.fail_reason, "insufficient_funds");
    EXPECT_EQ(service.balance(payer),    50);
    EXPECT_EQ(service.balance(merchant), 0);
}

TEST_F(PaymentServiceFixture, PayAccountNotFound) {
    const uint64_t payer = service.create_account("Payer", AccountType::Payer);
    service.deposit(payer, 100);
    EXPECT_THROW(service.pay(payer, 9999, 50), AccountNotFound);
    EXPECT_THROW(service.pay(9999, payer, 50), AccountNotFound);
}

TEST_F(PaymentServiceFixture, GetTransactionNotFound) {
    EXPECT_THROW(service.get_transaction(9999), TransactionNotFound);
}

TEST_F(PaymentServiceFixture, ConcurrentPayOneSucceeds) {
    const uint64_t payer    = service.create_account("Payer",    AccountType::Payer);
    const uint64_t merchant = service.create_account("Merchant", AccountType::Merchant);
    service.deposit(payer, 100);

    std::future<Transaction> fut1;
    std::future<Transaction> fut2;

    std::thread t1([&] {
        auto [tx_id, f] = service.pay(payer, merchant, 100);
        fut1 = std::move(f);
    });
    std::thread t2([&] {
        auto [tx_id, f] = service.pay(payer, merchant, 100);
        fut2 = std::move(f);
    });
    t1.join();
    t2.join();

    const Transaction final1 = fut1.get();
    const Transaction final2 = fut2.get();

    const int completed = (final1.status == TransactionStatus::Completed ? 1 : 0)
                        + (final2.status == TransactionStatus::Completed ? 1 : 0);
    const int failed    = (final1.status == TransactionStatus::Failed    ? 1 : 0)
                        + (final2.status == TransactionStatus::Failed    ? 1 : 0);

    EXPECT_EQ(completed, 1);
    EXPECT_EQ(failed,    1);
    EXPECT_EQ(service.balance(payer),    0);
    EXPECT_EQ(service.balance(merchant), 100);
}

TEST_F(PaymentServiceFixture, DeadlockFreeCounterTransfers) {
    const uint64_t a = service.create_account("A", AccountType::Payer);
    const uint64_t b = service.create_account("B", AccountType::Payer);
    service.deposit(a, 1000);
    service.deposit(b, 1000);

    std::future<Transaction> fut1;
    std::future<Transaction> fut2;

    std::thread t1([&] {
        auto [tx_id, f] = service.pay(a, b, 100);
        fut1 = std::move(f);
    });
    std::thread t2([&] {
        auto [tx_id, f] = service.pay(b, a, 100);
        fut2 = std::move(f);
    });
    t1.join();
    t2.join();

    fut1.get();
    fut2.get();

    EXPECT_EQ(service.balance(a) + service.balance(b), 2000);
}

TEST_F(PaymentServiceFixture, BalanceReturnsCorrectValue) {
    const uint64_t id = service.create_account("Bob", AccountType::Payer);
    service.deposit(id, 250);
    EXPECT_EQ(service.balance(id), 250);
}

TEST_F(PaymentServiceFixture, BalanceAccountNotFound) {
    EXPECT_THROW(service.balance(9999), AccountNotFound);
}

TEST_F(PaymentServiceFixture, HistoryContainsAllTransactions) {
    const uint64_t payer    = service.create_account("Payer",    AccountType::Payer);
    const uint64_t merchant = service.create_account("Merchant", AccountType::Merchant);
    service.deposit(payer, 500);

    auto [tx_id1, fut1] = service.pay(payer, merchant, 100);
    auto [tx_id2, fut2] = service.pay(payer, merchant, 200);

    fut1.get();
    fut2.get();

    const auto payer_history    = service.history(payer);
    const auto merchant_history = service.history(merchant);

    EXPECT_EQ(payer_history.size(),    2u);
    EXPECT_EQ(merchant_history.size(), 2u);
}

TEST_F(PaymentServiceFixture, HistoryAccountNotFound) {
    EXPECT_THROW(service.history(9999), AccountNotFound);
}

TEST_F(PaymentServiceFixture, DepositToMerchantThrows) {
    const uint64_t merchant = service.create_account("Shop", AccountType::Merchant);
    EXPECT_THROW(service.deposit(merchant, 1000), InvalidAccountType);
}

TEST_F(PaymentServiceFixture, PayFromMerchantThrows) {
    const uint64_t merchant = service.create_account("Shop", AccountType::Merchant);
    const uint64_t payer    = service.create_account("Alice", AccountType::Payer);
    EXPECT_THROW(service.pay(merchant, payer, 100), InvalidAccountType);
}

TEST_F(PaymentServiceFixture, PayFromPayerToMerchantSucceeds) {
    const uint64_t payer    = service.create_account("Alice", AccountType::Payer);
    const uint64_t merchant = service.create_account("Shop",  AccountType::Merchant);
    service.deposit(payer, 500);

    auto [tx_id, fut] = service.pay(payer, merchant, 200);
    const Transaction tx = fut.get();

    EXPECT_EQ(tx.status, TransactionStatus::Completed);
    EXPECT_EQ(service.balance(payer),    300);
    EXPECT_EQ(service.balance(merchant), 200);
}
