#ifndef ENGINE_RACE_ENGINE_RACE_ERROR_H_
#define ENGINE_RACE_ENGINE_RACE_ERROR_H_

#include <cerrno>
#include <stdexcept>
#include <system_error>

namespace zero_switch {

// underlying io errors
class IO_error: public std::system_error {
public:
    using std::system_error::system_error;
};

// server busy
class Server_busy: public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Server_internal_error: public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

template <class SysError, class String>
inline void throw_sys_error(const String& str)
{
    throw SysError(errno, std::system_category(), str);
}

}

#endif
