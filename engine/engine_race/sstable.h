#ifndef ENGINE_SSTABLE_H_
#define ENGINE_SSTABLE_H_

#include "config.h"

namespace zero_switch {

class DBMeta;

// strong guarantee
class SSTable {
public:
    SSTable(DBMeta& meta, std::uint64_t file_id);

    const std::string& path() const;

    string_view start() const;
    string_view end() const;

public:
    optional<std::string> read(string_view) const;

private:
    DBMeta& meta_;
    const std::uint64_t file_id_;
    const std::string   path_;
};

using SSTable_ptr = std::shared_ptr<SSTable>;

}

#endif
