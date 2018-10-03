#include <chrono>
#include <vector>
#include <algorithm>
#include "config.h"
#include "engine_error.h"
#include "batch_commiter.h"

using namespace zero_switch;

Batch_commiter::Batch_commiter(Kv_updater& updater)
    : updater_(updater),
      commiter_([this]{ this->run_(); })
{
}

Batch_commiter::~Batch_commiter() noexcept
try {
    stopped_ = true;
    not_empty_.notify_one();
    commiter_.join();

    try {
        throw Server_busy{"stopped"};
    } catch (...) {
        for (auto& t: task_queue_) {
            t.async_result.set_exception(std::current_exception());
        }
    }

    task_queue_.clear();

    kvlog.info("batch_commiter destroyed");
} catch (...) {
    kvlog.critical("batch_commiter destroyed");
}

void Batch_commiter::run_() noexcept
{
    while (!stopped_) {
        try {
            wait_unplug_();

            if (!stopped_) {
                unplug_();
            }
        } catch (const std::exception& e) {
            kvlog.error("[commit] failed: %s", e.what());
        }
    }
}

static auto build_batch(const std::vector<Task>& tasks)
{
    std::vector<std::pair<std::string_view, std::string_view>> batch;
    batch.reserve(tasks.size());

    for (auto& t: tasks) {
        batch.emplace_back(t.key, t.value);
    }

    return batch;
}

void Batch_commiter::wait_unplug_()
{
    std::unique_lock lock(mutex_);

    using namespace std::chrono_literals;

    while (!stopped_ && task_queue_.empty()) {
        not_empty_.wait_for(lock, 3ms);
    }
}

void Batch_commiter::unplug_()
{
    auto tasks = fetch_batch_();

    try {
        auto batch = build_batch(tasks);

        if (!batch.empty()) {
            updater_.write(batch);

            for (auto& t: tasks) {
                t.async_result.set_value();
            }
        }
    } catch (const std::exception& e) {
        kvlog.error("unplug failed: error={}, size={}", e.what(), tasks.size());

        for (auto& t: tasks) {
            t.async_result.set_exception(std::current_exception());
        }
    }
}

std::vector<Task> Batch_commiter::fetch_batch_()
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::vector<Task> batch;
    batch.reserve(task_queue_.size());
    batch.assign(std::make_move_iterator(task_queue_.begin()),
                 std::make_move_iterator(task_queue_.end()));
    task_queue_.clear();

    return batch;
}

std::future<void> Batch_commiter::submit(const std::string_view key, const std::string_view value)
{
    Task task { key, value };
    auto future = task.async_result.get_future();

    plug_(std::move(task));

    return future;
}

void Batch_commiter::plug_(Task&& task)
{
    std::lock_guard<std::mutex> lock(mutex_);
    task_queue_.push_back(std::move(task));
    try_unplug_();
}

// @pre locked
void Batch_commiter::try_unplug_()
{
    if (task_queue_.size() >= 32) {
        not_empty_.notify_one();
    }
}
