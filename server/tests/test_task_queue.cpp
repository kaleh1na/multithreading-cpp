#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

#include "worker/task_queue.hpp"

TEST(TaskQueue, TaskIsExecuted) {
    psp::TaskQueue queue;
    std::atomic<bool> executed{false};

    queue.enqueue([&] { executed.store(true, std::memory_order_relaxed); });

    std::thread worker([&] {
        if (auto task = queue.dequeue()) {
            (*task)();
        }
    });

    queue.stop();
    worker.join();

    EXPECT_TRUE(executed.load());
}

TEST(TaskQueue, StopReturnsNullopt) {
    psp::TaskQueue queue;
    queue.stop();
    EXPECT_EQ(queue.dequeue(), std::nullopt);
}

TEST(TaskQueue, FourWorkers1000Tasks) {
    psp::TaskQueue queue;
    std::atomic<int> counter{0};
    constexpr int total = 1000;

    for (int i = 0; i < total; ++i) {
        queue.enqueue([&] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    constexpr int num_workers = 4;
    std::vector<std::thread> workers;
    workers.reserve(num_workers);

    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back([&] {
            while (auto task = queue.dequeue()) {
                (*task)();
            }
        });
    }

    queue.stop();
    for (auto& w : workers) w.join();

    EXPECT_EQ(counter.load(), total);
}
