#ifndef ENGINE_RACE_MEMFILE_H_
#define ENGINE_RACE_MEMFILE_H_

#include <cstdint>
#include <atomic>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <string_view>

namespace zero_switch {

class Memfile {
public:
    explicit Memfile(const uint64_t redo_id)
        : redo_id_(redo_id)
    {
    }

    std::uint64_t estimated_size() const;
    std::uint64_t count() const;
    std::uint64_t redo_id() const
    {
        return redo_id_;
    }

    void add(std::string_view key, std::string_view value);

    // not thread safe
    const std::unordered_map<std::string, std::string>& values() const
    {
        return map_;
    }

private:
    const std::uint64_t redo_id_;

    std::atomic<std::uint64_t> size_ {};

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> map_;
};

using Memfile_ptr = std::shared_ptr<Memfile>;

}

#endif
