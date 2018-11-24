#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace zero_switch {

// basic: can be reuse after reset()
class Block_builder {
public:
    struct Options {
        std::uint32_t block_size = 64 * 1024;
        std::uint32_t block_restart_interval = 16;
    };

public:
    explicit Block_builder(const Options& options, std::uint8_t type);

    Block_builder(const Block_builder&) = delete;
    Block_builder& operator=(const Block_builder&) = delete;

    // @pre key.size() > 0
    // @pre key is larger than previous keys
    void add(std::string_view key, std::string_view value);

    std::string_view build();

    void reset() noexcept;

    bool is_full() const;

    bool empty() const
    {
        return buffer_.empty();
    }

private:
    std::uint32_t size_() const;

private:
    const Options options_;
    const std::uint8_t type_;
    std::string buffer_;
    std::vector<std::uint32_t> restart_points_;
    int counter_since_restart_ = 0;
    std::string_view last_key_;
};

}
