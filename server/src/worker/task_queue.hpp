#pragma once
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace psp {

using Task = std::function<void()>;

class TaskQueue {
public:
    void enqueue(Task task);
    std::optional<Task> dequeue();
    void stop();

private:
    std::queue<Task>        queue_;
    std::mutex              mtx_;
    std::condition_variable cv_;
    bool                    stopped_{false};
};

} // namespace psp
