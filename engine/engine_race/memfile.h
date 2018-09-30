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
    std::uint64_t estimated_size() const;
    std::uint64_t count() const;

    void add(std::string_view key, std::string_view value);

    // not thread safe
    const std::unordered_map<std::string, std::string>& values() const
    {
        return map_;
    }

private:
    std::atomic<std::uint64_t> size_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> map_;
};

using Memfile_ptr = std::shared_ptr<Memfile>;

}

#endif
