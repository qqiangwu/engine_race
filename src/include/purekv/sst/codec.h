#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace zero_switch {

void encode_variant32(std::string& buffer, std::uint32_t val);
void encode_fix32(std::string& buffer, std::uint32_t val);

void encode_variant64(std::string& buffer, std::uint64_t val);
void encode_fix64(std::string& buffer, std::uint64_t val);

// @pre assert buffer.size() == 4
std::uint32_t decode_fix32(std::string_view& buffer);

// @throws std::invalid_argument if buffer doesn't contains a std::uint64_t
std::uint64_t decode_variant64(std::string_view& buffer);

std::uint32_t crc32(const std::string_view buffer);

inline bool check_crc32(const std::string_view buffer, const std::uint32_t crc)
{
    const auto c0 = crc32(buffer);
    return c0 == crc;
}

}
