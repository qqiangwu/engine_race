#ifndef ENGINE_RACE_ENGINE_BATCH_COMMITER_H_
#define ENGINE_RACE_ENGINE_BATCH_COMMITER_H_

#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <string_view>
#include <boost/thread/future.hpp>

namespace zero_switch {

struct Task {
    std::string_view key;
    std::string_view value;
    boost::promise<void> async_result;
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

    void submit(std::string_view key, std::string_view value);

private:
    void submit_task_(Task&& task);
    void commit_();

private:
    void run_() noexcept;
    void run_impl_();
    std::vector<Task> fetch_batch_();

private:
    Kv_updater& updater_;

    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::deque<Task> task_queue_;

    std::atomic<bool> stopped_;
    std::thread commiter_;
};

}

#endif
