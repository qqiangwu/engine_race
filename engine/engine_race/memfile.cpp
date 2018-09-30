#include "memfile.h"

using namespace zero_switch;

std::uint64_t Memfile::estimated_size() const
{
    return size_.load();
}

std::uint64_t Memfile::count() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return map_.size();
}

void Memfile::add(const std::string_view key, const std::string_view value)
{
    const auto size = key.size() + value.size();
    size_ += size;

    std::lock_guard<std::mutex> lock(mutex_);
    map_[std::string(key)] = std::string(value);
}
