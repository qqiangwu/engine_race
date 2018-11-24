#pragma once

#include <cstdint>
#include <string>

// sst format
//  body:
//      block1
//      block2
//      block3
//      ...
//      block_index
//  footer(32 bytes)
//      block_index_handle(16)
//      padding
//      crc32(4)
namespace zero_switch {

enum Block_type : std::uint8_t {
    data_block,
    index_block
};

// strong
class Block_handle {
public:
    Block_handle(const std::uint64_t offset, const std::uint64_t size)
        : offset_(offset), size_(size)
    {
    }

    std::uint64_t offset() const
    {
        return offset_;
    }

    std::uint64_t size() const
    {
        return size_;
    }

    // buffer may be change on error
    void encode(std::string& buffer) const;
    std::string encode() const;

    // @throws Data_corruption_error if cannot parse
    static Block_handle decode(std::string_view& buffer);

private:
    const std::uint64_t offset_;
    const std::uint64_t size_;
};

// strong
class Footer {
public:
    inline static const std::uint32_t footer_size = 32;
    inline static const std::uint32_t crc_size = 4;

public:
    Footer(Block_handle block_index_handle)
        : block_index_handle_(block_index_handle)
    {
    }

    Block_handle bock_index_handle() const
    {
        return block_index_handle_;
    }

    // buffer may be change on error
    void encode(std::string& buffer) const;
    std::string encode() const;

    // @pre buffer.size() == footer_size
    // @throws Data_corruption_error
    static Footer decode(std::string_view& buffer);

private:
    Block_handle block_index_handle_;
};

}
