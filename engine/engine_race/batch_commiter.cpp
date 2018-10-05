#include <chrono>
#include <vector>
#include <algorithm>
#include "config.h"
#include "engine_error.h"
#include "batch_commiter.h"

using namespace zero_switch;

const auto buffer_size = 64;

Batch_commiter::Batch_commiter(Kv_updater& updater)
    : updater_(updater),
      commiter_([this]{ this->run_(); })
{
    task_queue_.reserve(buffer_size);
    task_in_process_.reserve(buffer_size);
    batch_.reserve(buffer_size);
}

Batch_commiter::~Batch_commiter() noexcept
try {
    stopped_ = true;
    not_empty_.notify_one();
    commiter_.join();

    for (auto& t: task_queue_) {
        t.async_result.set_value(Task_status::server_busy);
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
            kvlog.error("[commit] failed: {}", e.what());
        }
    }
}

void Batch_commiter::wait_unplug_()
{
    std::unique_lock lock(mutex_);

    using namespace std::chrono_literals;

    while (!stopped_ && task_queue_.empty()) {
        not_empty_.wait_for(lock, 3ms);
    }
}

void Batch_commiter::build_batch_()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);

        swap(task_queue_, task_in_process_);
    }

    for (auto& t: task_in_process_) {
        batch_.emplace_back(t.key, t.value);
    }
}

void Batch_commiter::unplug_()
{
    try {
        build_batch_();

        if (!batch_.empty()) {
            const auto done = updater_.write(batch_);

            for (auto& t: task_in_process_) {
                t.async_result.set_value(done? Task_status::done: Task_status::server_busy);
            }
        }
    } catch (const std::exception& e) {
        kvlog.error("unplug failed: error={}, size={}", e.what(), task_in_process_.size());

        for (auto& t: task_in_process_) {
            t.async_result.set_exception(std::current_exception());
        }
    }

    task_in_process_.clear();
    batch_.clear();
}

std::future<Task_status> Batch_commiter::submit(const std::string_view key, const std::string_view value)
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
