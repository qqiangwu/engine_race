#pragma once

#include "cache/cache.h"
#include "block.h"

namespace zero_switch {

struct Block_id {
    std::uint64_t table;
    std::uint64_t block;
};

using Block_cache = Cache<Block_id, Block>;

class Block_cache_provider : public Block_cache::Provider {
public:
    virtual value_type* read(const key_type& key, Storage* storage) override;
};

}