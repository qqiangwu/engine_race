#ifndef ENGINE_CORE_UTIL_H_
#define ENGINE_CORE_UTIL_H_

#include <thread>

namespace zero_switch {

bool bind_to_core(int core_id);
bool bind_to_core(std::thread::native_handle_type handle, int core_id);

bool exclude_core(int core_id);

}

#endif
