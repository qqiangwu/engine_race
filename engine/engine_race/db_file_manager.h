#ifndef ENGINE_RACE_DB_FILE_MANAGER_H_
#define ENGINE_RACE_DB_FILE_MANAGER_H_

#include <cstdint>
#include <vector>
#include <unordered_set>
#include <shared_mutex>
#include "config.h"
#include "sstable.h"

namespace zero_switch {

class DBMeta;

// strong gurantee
class DBFile_manager {
public:
    explicit DBFile_manager(DBMeta& meta);

public:
    std::vector<SSTable_ptr> snapshot() const;
    std::vector<std::string> reachable_files() const;

public:
    std::vector<SSTable_ptr> build_new_state(std::uint64_t file_id);
    void apply(std::vector<SSTable_ptr>&& files) noexcept;

private:
    void on_sstable_removed_(SSTable* sstable) noexcept;

private:
    DBMeta& meta_;

    mutable std::shared_mutex rwmutex_;
    std::vector<SSTable_ptr> sstables_;
    std::unordered_set<std::string> zombies_;
};

}

#endif
