#include "memfile.h"

using namespace zero_switch;

Memfile::Memfile(const std::uint64_t redo_id)
    : redo_id_(redo_id)
{
}

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
    auto iter = map_.find(key);
    if (iter == map_.end()) {
        map_.emplace(std::string(key), std::string(value));
    } else {
        iter->second = std::string(value);
    }
}

void Memfile::add(const std::vector<std::pair<string_view, string_view>>& batch)
{
    std::uint64_t size = 0;

    std::lock_guard lock(mutex_);
    for (auto [k, v]: batch) {
        size += k.size() + v.size();

        auto iter = map_.find(k);
        if (iter == map_.end()) {
            map_.emplace(std::string(k), std::string(v));
        } else {
            iter->second = std::string(v);
        }
    }

    size_ += size;
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
