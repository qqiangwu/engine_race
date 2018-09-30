#include <system_error>
#include <cerrno>
#include "redo_log.h"

using namespace zero_switch;

Redo_log::Redo_log(const std::string& dir, const std::uint64_t id)
    : dir_(dir),
      id_(id),
      path_(dir + '/' + std::to_string(id) + ".redo"),
      out_(std::fopen(path_.c_str(), "w"))
{
    if (!out_) {
        throw std::system_error(errno, std::system_category(), "open redo failed:" + path_);
    }

    const auto r = std::setvbuf(out_, buffer_.data(), _IOFBF, buffer_.size());
    if (r != 0) {
        throw std::system_error(errno, std::system_category(), "setvbuf failed:" + path_);
    }
}

Redo_log::~Redo_log()
{
    std::fclose(out_);
    out_ = nullptr;
}

void Redo_log::append(const std::string_view key, const std::string_view value)
{
    const auto r = std::fprintf(out_, "%u%.*s%u%.*s",
            unsigned(key.size()), int(key.size()), key.data(),
            unsigned(value.size()), int(value.size()), value.data());
    if (r < 0) {
        throw std::system_error(errno, std::system_category(), "write redo failed");
    }

    if (std::fflush(out_) != 0) {
        throw std::system_error(errno, std::system_category(), "flush redo failed");
    }
}

void Redo_log::append(const std::vector<std::pair<const std::string_view, const std::string_view>>& batch)
{
    for (auto [key, value]: batch) {
        const auto r = std::fprintf(out_, "%u%.*s%u%.*s",
                unsigned(key.size()), int(key.size()), key.data(),
                unsigned(value.size()), int(value.size()), value.data());
        if (r < 0) {
            throw std::system_error(errno, std::system_category(), "write redo failed");
        }
    }

    if (std::fflush(out_) != 0) {
        throw std::system_error(errno, std::system_category(), "flush redo failed");
    }
}
