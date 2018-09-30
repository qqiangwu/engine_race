#include <vector>
#include <algorithm>
#include "engine_error.h"
#include "batch_commiter.h"

using namespace zero_switch;

Batch_commiter::Batch_commiter(Kv_updater& updater)
    : updater_(updater)
{
}

void Batch_commiter::submit(const std::string_view key, const std::string_view value)
{
    Task task { key, value };
    auto future = task.result.get_future();

    plug_(std::move(task));

    auto state = future.get();
    switch (state) {
    case Task_state::done:
        return;

    case Task_state::canceled:
        throw Task_canceled{"submit failed"};

    case Task_state::leader:
        commit_();
        return;

    default:
        throw std::runtime_error("unknown error in submit");
    }
}

// TODO: flow control
void Batch_commiter::plug_(Task&& task)
{
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push_back(std::move(task));
    if (tasks_.size() == 1) {
        tasks_.front().result.set_value(Task_state::leader);
    }
}

static inline auto build_batch(const std::vector<Task>& tasks)
{
    std::vector<std::pair<const std::string_view, const std::string_view>> batch;
    batch.reserve(tasks.size());

    for (auto& task: tasks) {
        batch.push_back(std::make_pair(task.key, task.value));
    }

    return batch;
}

static void wakeup(std::vector<Task>& tasks)
{
    std::for_each(tasks.begin() + 1, tasks.end(), [](auto& t){
        t.result.set_value(Task_state::done);
    });
}

void Batch_commiter::commit_()
{
    std::vector<Task> tasks;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.assign(std::make_move_iterator(tasks_.begin()),
                     std::make_move_iterator(tasks_.end()));
    }

    auto batch = build_batch(tasks);
    updater_.write(batch);

    unplug_(tasks.size());
    wakeup(tasks);
}

void Batch_commiter::unplug_(const int finished)
{
    assert(finished >= 0);

    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.erase(tasks_.begin(), tasks_.begin() + finished);
    if (!tasks_.empty()) {
        tasks_.front().result.set_value(Task_state::leader);
    }
}
