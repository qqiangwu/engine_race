#include <cassert>
#include <fmt/format.h>
#include "sst/table_cache.h"

using namespace zero_switch;
using namespace std;

using fmt::format;

Table_cache_provider::Table_cache_provider(const std::string& dir, Block_cache& block_cache)
    : dir_(dir),
      block_cache_(block_cache)
{
}

Table_cache_provider::value_type* Table_cache_provider::read(const key_type& key, Storage* storage)
{
    assert(storage);

    const auto path = format("{}/{}.sst", dir_, key);
    return new(storage) Table(path, key, block_cache_);
}