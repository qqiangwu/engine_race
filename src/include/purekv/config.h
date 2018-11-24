#ifndef ENGINE_RACE_CONFIG_H_
#define ENGINE_RACE_CONFIG_H_

#include <string_view>
#include <optional>
#include <cstdint>
#include <memory>
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace zero_switch {

using std::string_view;
using std::optional;
using std::nullopt;

namespace detail {

void init_logger();

}

}

inline auto get_logger()
{
    zero_switch::detail::init_logger();
    return spdlog::basic_logger_mt<spdlog::async_factory>("kvlog", "engine.log");
}

inline auto _log_ptr = get_logger();
inline auto& kvlog = *_log_ptr;

#endif
