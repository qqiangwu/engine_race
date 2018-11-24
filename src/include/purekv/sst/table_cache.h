#pragma once

#include "cache/cache.h"
#include "table.h"
#include "block_cache.h"

namespace zero_switch {

using Table_cache = Cache<std::uint64_t, Table>;

// strong
class Table_cache_provider : public Table_cache::Provider {
public:
    Table_cache_provider(const std::string& dir, Block_cache& block_cache_);

    virtual value_type* read(const key_type& key, Storage* storage) override;

private:
    const std::string& dir_;
    Block_cache& block_cache_;
};

}