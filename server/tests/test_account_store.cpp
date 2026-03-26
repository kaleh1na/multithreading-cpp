#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <set>
#include <mutex>

#include "store/account_store.hpp"

TEST(AccountStore, CreateAndFind) {
    AccountStore store;
    const uint64_t id = store.create("Alice", AccountType::Payer);
    auto acc = store.find(id);
    ASSERT_NE(acc, nullptr);
    EXPECT_EQ(acc->account_id, id);
    EXPECT_EQ(acc->name, "Alice");
    EXPECT_EQ(acc->type, AccountType::Payer);
    EXPECT_EQ(acc->balance, 0);
}

TEST(AccountStore, FindMissing) {
    AccountStore store;
    EXPECT_EQ(store.find(9999), nullptr);
}

TEST(AccountStore, ConcurrentCreate) {
    AccountStore store;
    constexpr int threads = 4;
    constexpr int per_thread = 25;

    std::vector<std::thread> workers;
    std::vector<uint64_t> ids(threads * per_thread);
    std::mutex ids_mtx;

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t] {
            for (int i = 0; i < per_thread; ++i) {
                uint64_t id = store.create("user", AccountType::Merchant);
                std::lock_guard lock(ids_mtx);
                ids[t * per_thread + i] = id;
            }
        });
    }
    for (auto& w : workers) w.join();

    std::set<uint64_t> unique_ids(ids.begin(), ids.end());
    EXPECT_EQ(unique_ids.size(), static_cast<size_t>(threads * per_thread));

    for (uint64_t id : ids) {
        EXPECT_NE(store.find(id), nullptr);
    }
}
