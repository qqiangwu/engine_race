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

namespace detail {

struct Fast_string_cmp {
    using is_transparent = void;

    template <class SV1, class SV2>
    bool operator()(const SV1& a, const SV2& b) const
    {
        return string_view(a.data(), a.size()) < string_view(b.data(), b.size());
    }
};

}

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

    void add(string_view key, string_view value);
    void add(const std::vector<std::pair<string_view, string_view>>& batch);

    optional<std::string> read(string_view key);

    // not thread safe
    const std::map<std::string, std::string, detail::Fast_string_cmp>& values() const
    {
        return map_;
    }

private:

    const std::uint64_t redo_id_;
    std::atomic<std::uint64_t> size_ {};

    mutable std::shared_mutex mutex_;
    std::map<std::string, std::string, detail::Fast_string_cmp> map_;
};

using Memfile_ptr = std::shared_ptr<Memfile>;

}

#endif
