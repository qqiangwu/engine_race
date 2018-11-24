#pragma once

#include <memory>
#include "fs/fs.h"
#include "block_cache.h"

namespace zero_switch {

// strong
class Table {
public:
    Table(const std::string& path, std::uint64_t id, Block_cache& block_cache);

private:
    const std::string path_;
    const std::uint64_t id_;
    std::unique_ptr<Random_access_file> file_reader_;
    Block_cache& block_cache_;
    Block index_block_;
};

}