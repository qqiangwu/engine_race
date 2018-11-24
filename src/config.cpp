#include "config.h"

void zero_switch::detail::init_logger()
{
    spdlog::init_thread_pool(8192, 1);
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
}
