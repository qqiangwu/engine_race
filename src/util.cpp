#include <pthread.h>
#include "util.h"

bool zero_switch::bind_to_core(const int core_id)
{
    return bind_to_core(pthread_self(), core_id);
}

bool zero_switch::bind_to_core(std::thread::native_handle_type handle, const int core_id)
{
#ifdef LINUX
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    return pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset) == 0;
#else
    return true;
#endif
}

bool zero_switch::exclude_core(const int core_id)
{
#ifdef LINUX
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (int i = 0, size = std::thread::hardware_concurrency(); i < size; ++i) {
        if (i != core_id) {
            CPU_SET(i, &cpuset);
        }
    }

    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
#else
    return true;
#endif
}
