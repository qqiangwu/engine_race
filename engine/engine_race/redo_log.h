#ifndef ENGINE_RACE_REDO_LOG_H_
#define ENGINE_RACE_REDO_LOG_H_

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <memory>

namespace zero_switch {

class Redo_log {
public:
    Redo_log(const std::string& dir, std::uint64_t id);
    ~Redo_log();

    Redo_log(const Redo_log&) = delete;
    Redo_log& operator=(const Redo_log&) = delete;

    std::uint64_t id() const
    {
        return id_;
    }

    void append(std::string_view key, std::string_view value);

private:
    const std::string  dir_;
    const std::uint64_t id_;
    const std::string  path_;
    FILE* out_;
};

using Redo_log_ptr = std::shared_ptr<Redo_log>;

}

#endif