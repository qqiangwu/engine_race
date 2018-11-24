#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include "fs.h"

namespace zero_switch {

constexpr const auto default_buffer_size = 1024 * 64;

// basic
class Default_file_appender : public File_appender {
public:
    explicit Default_file_appender(const std::string& filename, std::int64_t buffer_size = default_buffer_size);
    virtual ~Default_file_appender() noexcept override;

    virtual void append(std::string_view buffer) override;

    virtual void flush() override;

    virtual void sync() override;

    virtual void close() override;

private:
    const std::string filename_;
    std::vector<char> buffer_;

    FILE* file_;
};

// basic
class Default_random_access_file : public Random_access_file {
public:
    explicit Default_random_access_file(const std::string& filename);

    virtual ~Default_random_access_file() noexcept override;

    virtual std::uintmax_t read(std::uintmax_t offset, std::string& buffer) override;

private:
    const std::string filename_;
    int fd_;
};

}
