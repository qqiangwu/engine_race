#include "memfile.h"

using namespace zero_switch;

std::uint64_t Memfile::estimated_size() const
{
    return size_.load();
}

std::uint64_t Memfile::count() const
{
    std::lock_guard lock(mutex_);
    return map_.size();
}

void Memfile::add(const string_view key, const string_view value)
{
    const auto size = key.size() + value.size();
    size_ += size;

    std::lock_guard lock(mutex_);
    auto iter = map_.find(std::string(key));
    if (iter == map_.end()) {
        map_.emplace(std::string(key), std::string(value));
    } else {
        iter->second = std::string(value);
    }
}

optional<std::string> Memfile::read(const string_view key)
{
    std::shared_lock lock(mutex_);
    auto iter = map_.find(std::string(key));
    if (iter == map_.end()) {
        return nullopt;
    } else {
        return { iter->second };
    }
}
