#ifndef ENGINE_RACE_MEMFILE_H_
#define ENGINE_RACE_MEMFILE_H_

#include <cstdint>
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string_view>
#include <map>
#include "config.h"

namespace zero_switch {

// strong guarantee
class Memfile {
public:
    std::uint64_t estimated_size() const;
    std::uint64_t count() const;

    void add(std::string_view key, std::string_view value);

    optional<std::string> read(string_view key);

    // not thread safe
    const std::map<std::string, std::string>& values() const
    {
        return map_;
    }

private:
    std::atomic<std::uint64_t> size_ {};

    mutable std::shared_mutex mutex_;
    std::map<std::string, std::string> map_;
};

using Memfile_ptr = std::shared_ptr<Memfile>;

}

#endif
