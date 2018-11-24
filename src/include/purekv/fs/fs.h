#pragma once

#include <cstdint>
#include <string_view>
#include <boost/noncopyable.hpp>

namespace zero_switch {

// basic guarantee: if any method throws, the appender should not be used anymore
class File_appender : private boost::noncopyable {
public:
    virtual ~File_appender() noexcept = default;

    virtual void append(std::string_view buffer) = 0;
    virtual void flush() = 0;
    virtual void sync() = 0;
    virtual void close() = 0;
};

// basic guarantee: if any method throws, the appender should not be used anymore
class Random_access_file : private boost::noncopyable {
public:
    virtual ~Random_access_file() noexcept = default;

    virtual std::uintmax_t read(std::uintmax_t offset, std::string& buffer) = 0;
};

}
