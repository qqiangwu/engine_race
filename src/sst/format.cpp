#include <cassert>
#include <stdexcept>
#include "sst/error.h"
#include "sst/codec.h"
#include "sst/format.h"

using namespace zero_switch;

void Block_handle::encode(std::string& buffer) const
{
    encode_variant64(buffer, offset_);
    encode_variant64(buffer, size_);
}

std::string Block_handle::encode() const
{
    std::string buffer;
    encode(buffer);
    return buffer;
}

Block_handle Block_handle::decode(std::string_view& buffer)
try {
    const auto offset = decode_variant64(buffer);
    const auto size = decode_variant64(buffer);

    return Block_handle(offset, size);
} catch (std::invalid_argument& e) {
    throw Data_corruption_error{ "bad block handle" };
}

void Footer::encode(std::string& buffer) const
{
    buffer.reserve(buffer.size() + footer_size);

    const char* p = buffer.data() + buffer.size();
    const auto size_before = buffer.size();
    block_index_handle_.encode(buffer);

    const auto bh_size = buffer.size() - size_before;
    const auto padding_size = footer_size - bh_size - crc_size;
    buffer.append('\0', padding_size);

    const auto crc = crc32({p, bh_size + padding_size});
    encode_fix32(buffer, crc);
}

std::string Footer::encode() const
{
    std::string buffer;
    encode(buffer);
    return buffer;
}

Footer Footer::decode(std::string_view& buffer)
{
    assert(buffer.size() == footer_size);

    auto footer_buf = std::string_view(buffer.data(), footer_size);
    auto payload_buf = std::string_view(footer_buf.data(), footer_size - crc_size);
    auto crc_buf = std::string_view(buffer.data() + payload_buf.size(), crc_size);
    const auto crc = decode_fix32(crc_buf);
    if (!check_crc32(payload_buf, crc)) {
        throw Data_corruption_error{"footer corrupted"};
    }

    const auto block_handle = Block_handle::decode(footer_buf);

    // safe point
    buffer.remove_prefix(footer_size);
    return Footer(block_handle);
}
