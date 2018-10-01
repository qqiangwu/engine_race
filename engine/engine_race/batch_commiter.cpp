#include <iostream>
#include <vector>
#include <algorithm>
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

    for (auto& t: task_queue_) {
        t.async_result.set_exception(Task_canceled{"stopped"});
    }
} catch (...) {
}

void Batch_commiter::run_() noexcept
{
    while (!stopped_) {
        try {
            run_impl_();
        } catch (const std::runtime_error& e) {
            std::cerr << "error: " << e.what() << std::endl;
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

void Batch_commiter::run_impl_()
{
    auto tasks = fetch_batch_();
    auto batch = build_batch(tasks);

    updater_.write(batch);

    for (auto& t: tasks) {
        t.async_result.set_value();
    }
}

std::vector<Task> Batch_commiter::fetch_batch_()
{
    std::unique_lock<std::mutex> lock(mutex_);

    while (!stopped_ && task_queue_.empty()) {
        not_empty_.wait(lock);
    }

    std::vector<Task> batch;
    batch.reserve(task_queue_.size());
    batch.assign(std::make_move_iterator(task_queue_.begin()),
                 std::make_move_iterator(task_queue_.end()));
    task_queue_.clear();

    return batch;
}

void Batch_commiter::submit(const std::string_view key, const std::string_view value)
{
    Task task { key, value };
    auto future = task.async_result.get_future();

    submit_task_(std::move(task));

    future.get();
}

void Batch_commiter::submit_task_(Task&& task)
{
    std::lock_guard<std::mutex> lock(mutex_);
    task_queue_.push_back(std::move(task));
    not_empty_.notify_one();
}
