#include "worker/task_queue.hpp"

namespace psp {

void TaskQueue::enqueue(Task task) {
    {
        std::lock_guard lock(mtx_);
        queue_.push(std::move(task));
    }
    cv_.notify_one();
}

std::optional<Task> TaskQueue::dequeue() {
    std::unique_lock lock(mtx_);
    cv_.wait(lock, [this] { return !queue_.empty() || stopped_; });
    if (queue_.empty()) {
        return std::nullopt;
    }
    Task task = std::move(queue_.front());
    queue_.pop();
    return task;
}

void TaskQueue::stop() {
    {
        std::lock_guard lock(mtx_);
        stopped_ = true;
    }
    cv_.notify_all();
}

} // namespace psp
