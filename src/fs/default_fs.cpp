#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <fmt/format.h>
#include "purekv/engine_error.h"
#include "fs/default_fs.h"

using namespace zero_switch;
using fmt::format;

Default_file_appender::Default_file_appender(const std::string& filename, const std::int64_t buffer_size)
    : filename_(filename),
      buffer_(buffer_size),
      file_(std::fopen(filename.c_str(), "a"))
{
    assert(buffer_size > 0 && "buffer size should be positive");

    if (!file_) {
        throw_sys_error(format("create file appender failed: {}", filename));
    }

    const auto rc = std::setvbuf(file_, buffer_.data(), _IOFBF, buffer_.size());
    if (rc != 0) {
        throw_sys_error<IO_error>(format("setvbuf failed: ", filename_));
    }
}

Default_file_appender::~Default_file_appender() noexcept
try {
    if (file_) {
        close();
    }
} catch (...) {
    // ignored
}

void Default_file_appender::append(const std::string_view buffer)
{
    assert(file_);

    const auto bytes_written = std::fwrite(buffer.data(), 1, buffer.size(), file_);
    if (bytes_written < buffer.size()) {
        throw_sys_error("fwrite failed:" + filename_);
    }
}

void Default_file_appender::flush()
{
    assert(file_);

    const int rc = std::fflush(file_);
    if (rc != 0) {
        throw_sys_error<IO_error>("fflush failed: " + filename_);
    }
}

void Default_file_appender::sync()
{
    assert(file_);

    flush();

    const auto fd = ::fileno(file_);
    assert(fd >= 0 && "fatal error");

    const auto rc = ::fsync(fd);
    if (rc != 0) {
        throw_sys_error<IO_error>("fsync failed: " + filename_);
    }
}

void Default_file_appender::close()
{
    assert(file_);

    const auto rc = std::fclose(file_);
    file_ = nullptr;

    if (rc < 0) {
        throw_sys_error<IO_error>("fclose failed: " + filename_);
    }
}

Default_random_access_file::Default_random_access_file(const std::string& filename)
    : filename_(filename),
      fd_(::open(filename_.c_str(), O_RDONLY))
{
    if (fd_ < 0) {
        throw_sys_error<IO_error>(format("open {} failed", filename));
    }
}

Default_random_access_file::~Default_random_access_file() noexcept
{
    assert(fd_ >= 0);

    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

std::uintmax_t Default_random_access_file::read(const std::uintmax_t offset, std::string& buffer)
{
    assert(fd_ >= 0);

    const auto bytes_read = ::pread(fd_, buffer.data(), buffer.size(), static_cast<off_t>(offset));
    if (bytes_read < 0) {
        throw_sys_error<IO_error>(format("pread failed: file={}, offset={}", filename_, offset));
    }

    return static_cast<std::uintmax_t>(bytes_read);
}