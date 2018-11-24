#include <fcntl.h>
#include <stdio.h>
#include <system_error>
#include <cerrno>
#include "engine_error.h"
#include "redo_log.h"

using namespace zero_switch;

Redo_log::Redo_log(const std::string& dir, const std::uint64_t id)
    : dir_(dir),
      id_(id),
      path_(dir + '/' + std::to_string(id) + ".redo"),
      out_(std::fopen(path_.c_str(), "w"))
{
    if (!out_) {
        throw_sys_error<IO_error>("open redo failed:" + path_);
    }

    int r = std::setvbuf(out_, buffer_.data(), _IOFBF, buffer_.size());
    if (r != 0) {
        throw_sys_error<IO_error>("setvbuf failed:" + path_);
    }

#ifdef LINUX
    r = fallocate(fileno(out_), FALLOC_FL_KEEP_SIZE, 0, 2 * 1024 * 1024);
    if (r != 0) {
        kvlog.warn("fallocate failed: file={}", path_);
    }
#endif
}

Redo_log::~Redo_log()
{
    if (out_) {
        std::fclose(out_);
        out_ = nullptr;
    }
}

void Redo_log::append(const std::string_view key, const std::string_view value)
{
    const auto r = std::fprintf(out_, "%u%.*s%u%.*s",
            unsigned(key.size()), int(key.size()), key.data(),
            unsigned(value.size()), int(value.size()), value.data());
    if (r < 0) {
        throw_sys_error<IO_error>("write redo failed" + path_);
    }

    if (std::fflush(out_) != 0) {
        throw_sys_error<IO_error>("flush redo failed:" + path_);
    }
}

void Redo_log::append(const std::vector<std::pair<std::string_view, std::string_view>>& batch)
{
    for (auto [key, value]: batch) {
        const auto r = std::fprintf(out_, "%u%.*s%u%.*s",
                unsigned(key.size()), int(key.size()), key.data(),
                unsigned(value.size()), int(value.size()), value.data());
        if (r < 0) {
            throw_sys_error<IO_error>("write redo failed" + path_);
        }
    }

    if (std::fflush(out_) != 0) {
        throw_sys_error<IO_error>("flush redo failed:" + path_);
    }
}
