#include <cassert>
#include "sst/error.h"
#include "sst/table_builder.h"
#include "sst/format.h"

using namespace std;
using namespace zero_switch;

Table_builder::Table_builder(const Options& options, File_appender& appender)
    : options_(options),
      appender_(appender),
      data_block_(options_.data_block_options, Block_type::data_block),
      index_block_(options_.index_block_options, Block_type::index_block)
{
}

void Table_builder::add(const string_view key, const string_view value)
{
    assert(!closed_);
    assert(!key.empty());

    last_key_.reserve(key.size());

    data_block_.add(key, value);
    ++num_of_entries_;
    last_key_ = key;

    if (data_block_.is_full()) {
        flush_();
    }
}

void Table_builder::flush_()
{
    assert(!data_block_.empty());

    const auto handle = flush_block_(data_block_);
    index_block_.add(last_key_, handle.encode());
}

Block_handle Table_builder::flush_block_(Block_builder& builder)
{
    const auto handle_offset = offset_;
    const auto raw = builder.build();

    appender_.append(raw);
    offset_ += raw.size();

    return Block_handle(handle_offset, raw.size());
}

void Table_builder::commit()
{
    assert(!closed_);
    assert(num_of_entries_ > 0);

    closed_ = true;

    if (!data_block_.empty()) {
        flush_();
    }

    const auto index_block_handle = flush_block_(index_block_);

    Footer footer(index_block_handle);
    const auto raw = footer.encode();
    appender_.append(raw);

    offset_ += raw.size();
    closed_ = true;
}

std::uint64_t Table_builder::num_entries() const
{
    return num_of_entries_;
}

std::uint64_t Table_builder::file_size() const
{
    return offset_;
}
