#ifndef ENGINE_RACE_ENGINE_BATCH_COMMITER_H_
#define ENGINE_RACE_ENGINE_BATCH_COMMITER_H_

#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <atomic>
#include <string_view>

namespace zero_switch {

struct Task {
    std::string_view key;
    std::string_view value;
    std::promise<void> async_result;
};

class Kv_updater {
public:
    virtual ~Kv_updater() = default;

    virtual void write(const std::vector<std::pair<std::string_view, std::string_view>>& batch) = 0;
};

class Batch_commiter {
public:
    explicit Batch_commiter(Kv_updater& updater);
    ~Batch_commiter() noexcept;

    std::future<void> submit(std::string_view key, std::string_view value);

private:
    void plug_(Task&& task);
    void try_unplug_();

private:
    void run_() noexcept;
    void wait_unplug_();
    void unplug_();
    std::vector<Task> fetch_batch_();

private:
    Kv_updater& updater_;

    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::deque<Task> task_queue_;

    std::atomic<bool> stopped_ { false };
    std::thread commiter_;
};

}

#endif
