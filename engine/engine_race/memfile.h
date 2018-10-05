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
    explicit Memfile(std::uint64_t redo_id);

    std::uint64_t estimated_size() const;
    std::uint64_t count() const;
    std::uint64_t redo_id() const
    {
        return redo_id_;
    }

    bool has_room_for(const std::vector<std::pair<string_view, string_view>>& batch) const;

    void add(string_view key, string_view value);
    void add(const std::vector<std::pair<string_view, string_view>>& batch);

    optional<std::string> read(string_view key);

    // not thread safe
    const std::map<string_view, string_view>& values() const
    {
        return map_;
    }

private:
    string_view clone_(string_view v);

private:
    const std::uint64_t redo_id_;

    mutable std::shared_mutex mutex_;
    std::vector<char> buffer_;
    std::map<string_view, string_view> map_;
};

using Memfile_ptr = std::shared_ptr<Memfile>;

}

#endif
