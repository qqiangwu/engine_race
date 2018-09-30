#ifndef ENGINE_RACE_ENGINE_BATCH_COMMITER_H_
#define ENGINE_RACE_ENGINE_BATCH_COMMITER_H_

#include <deque>
#include <mutex>
#include <string_view>
#include <boost/thread/future.hpp>

namespace zero_switch {

enum class Task_state {
    done,
    leader,
    canceled
};

struct Task {
    std::string_view key;
    std::string_view value;
    boost::promise<Task_state> result;
};

class Kv_updater {
public:
    virtual ~Kv_updater() = default;

    virtual void write(const std::vector<std::pair<const std::string_view, const std::string_view>>& batch) = 0;
};

class Batch_commiter {
public:
    explicit Batch_commiter(Kv_updater& updater);

    void submit(const std::string_view key, const std::string_view value);

private:
    void plug_(Task&& task);
    void unplug_(const int finished);
    void commit_();

private:
    Kv_updater& updater_;

    std::mutex mutex_;
    std::deque<Task> tasks_;
};

}

#endif
