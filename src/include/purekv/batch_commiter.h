#ifndef ENGINE_RACE_ENGINE_BATCH_COMMITER_H_
#define ENGINE_RACE_ENGINE_BATCH_COMMITER_H_

#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <atomic>
#include <string_view>
#include "util/daemon.h"
#include "config.h"

namespace zero_switch {

enum class Task_status {
    done,
    server_busy
};

struct Task {
    std::string_view key;
    std::string_view value;
    std::promise<Task_status> async_result;
};

class Kv_updater {
public:
    virtual ~Kv_updater() = default;

    virtual bool write(const std::vector<std::pair<std::string_view, std::string_view>>& batch) = 0;
};

class Batch_commiter {
public:
    explicit Batch_commiter(Kv_updater& updater);
    ~Batch_commiter() noexcept;

    std::future<Task_status> submit(std::string_view key, std::string_view value);

private:
    void plug_(Task&& task);
    void try_unplug_() noexcept;

private:
    void run_() noexcept;
    void wait_unplug_();
    void unplug_();
    void fetch_tasks_();
    void build_batch_();

private:
    Kv_updater& updater_;

    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::vector<Task> task_queue_;
    std::vector<Task> task_in_process_;
    std::vector<std::pair<string_view, string_view>> batch_;

    std::atomic<bool> stopped_ { false };
    std::thread commiter_;
};

}

#endif
