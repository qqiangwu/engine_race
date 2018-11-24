#pragma once

#include <cstdint>
#include <vector>
#include "config.h"
#include "memfile.h"
#include "sstable.h"

namespace zero_switch {

// strong guarantee
class Snapshot {
public:
    Snapshot(uint64_t seq, std::vector<Memfile_ptr> memfiles, std::vector<SSTable_ptr> sstables);

    optional<std::string> read(string_view key);

private:
    const uint64_t seq_;
    const std::vector<Memfile_ptr> memfiles_;
    const std::vector<SSTable_ptr> sstables_;
};

}