#include <numeric>
#include "memfile.h"

using namespace zero_switch;

Memfile::Memfile(const std::uint64_t redo_id)
    : redo_id_(redo_id)
{
    buffer_.reserve(2 * 1024 * 1024);
}

std::uint64_t Memfile::estimated_size() const
{
    std::lock_guard lock(mutex_);
    return buffer_.size();
}

std::uint64_t Memfile::count() const
{
    std::lock_guard lock(mutex_);
    return map_.size();
}

bool Memfile::has_room_for(const std::vector<std::pair<string_view, string_view>>& batch) const
{
    const auto size = std::accumulate(batch.begin(), batch.end(), 0ULL, [](auto acc, auto& p){
        return acc + p.first.size() + p.second.size();
    });

    std::lock_guard lock(mutex_);
    return buffer_.size() + size <= buffer_.capacity();
}

void Memfile::add(const string_view key, const string_view value)
{
    std::vector<std::pair<string_view, string_view>> batch{ std::make_pair(key, value) };
    add(batch);
}

void Memfile::add(const std::vector<std::pair<string_view, string_view>>& batch)
{
    std::lock_guard lock(mutex_);
    for (auto [k, v]: batch) {
        auto iter = map_.find(k);
        if (iter == map_.end()) {
            auto kk = clone_(k);
            auto vv = clone_(v);
            map_.emplace(kk, vv);
        } else {
            iter->second = clone_(v);
        }
    }
}

optional<std::string> Memfile::read(const string_view key)
{
    std::shared_lock lock(mutex_);
    auto iter = map_.find(key);
    if (iter == map_.end()) {
        return nullopt;
    } else {
        return { std::string(iter->second) };
    }
}

string_view Memfile::clone_(const string_view v)
{
    const auto size = buffer_.size();

    buffer_.insert(buffer_.end(), v.begin(), v.end());
    return string_view(buffer_.data() + size, v.size());
}
