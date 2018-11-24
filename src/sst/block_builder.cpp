#include <cassert>
#include "sst/block_builder.h"
#include "sst/codec.h"

using namespace zero_switch;
using std::string_view;

Block_builder::Block_builder(const Options& options, const std::uint8_t type)
    : options_(options), type_(type)
{
    assert(options.block_size > 0);
    assert(options.block_restart_interval > 0);

    buffer_.reserve(options.block_size);
    restart_points_.push_back(0);
}

void Block_builder::add(const string_view key, const string_view value)
{
    assert(!key.empty());
    assert(key > last_key_);
    assert(counter_since_restart_ <= options_.block_restart_interval);

    if (counter_since_restart_ == options_.block_restart_interval) {
        // safe point
        restart_points_.push_back(buffer_.size());
        counter_since_restart_ = 0;
    }

    encode_variant32(buffer_, key.size());
    encode_variant32(buffer_, value.size());

    const auto key_start = buffer_.size();
    buffer_.append(key.data(), key.size());
    buffer_.append(value.data(), value.size());

    last_key_ = { buffer_.data() + key_start, key.size() };
    ++counter_since_restart_;
}

string_view Block_builder::build()
{
    for (const auto p: restart_points_) {
        encode_fix32(buffer_, p);
    }

    encode_fix32(buffer_, restart_points_.size());
    buffer_.push_back(type_);
    encode_fix32(buffer_, crc32(buffer_));

    assert(buffer_.size() <= options_.block_size);
    return buffer_;
}

void Block_builder::reset() noexcept
{
    buffer_.clear();
    restart_points_.resize(1);
    counter_since_restart_ = 0;
    last_key_ = {};
}

bool Block_builder::is_full() const
{
    const auto current_size = size_();
    const auto trailer = sizeof(type_) + sizeof(std::uint32_t);
    return current_size + trailer >= options_.block_size;
}

std::uint32_t Block_builder::size_() const
{
    const std::uint32_t raw_buffer_size = buffer_.size();
    const std::uint32_t restart_vec_size = restart_points_.size() * sizeof(std::uint32_t);
    const std::uint32_t restart_vec_len_size = sizeof(std::uint32_t);
    return raw_buffer_size + restart_vec_size + restart_vec_len_size;
}
