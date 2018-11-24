#ifndef ENGINE_RACE_DB_META_H_
#define ENGINE_RACE_DB_META_H_

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

namespace zero_switch {

class DBMeta {
public:
    explicit DBMeta(const std::string& db);

    const std::string& db() const
    {
        return db_;
    }

    std::uint64_t current_redo_id() const;
    std::uint64_t next_redo_id();

    std::uint64_t next_id();

    std::vector<std::uint64_t> sorted_tables() const;

public:
    void checkpoint(std::uint64_t redo_id, std::uint64_t file_id);

private:
    void checkpoint_(const uint64_t redo_id, const std::vector<uint64_t>& sstables);

private:
    const std::string db_;

    mutable std::mutex    mutex_;
    std::uint64_t redo_id_;
    std::uint64_t next_redo_id_;
    std::uint64_t next_file_id_;

    std::vector<std::uint64_t> sorted_tables_;
};

}

#endif
