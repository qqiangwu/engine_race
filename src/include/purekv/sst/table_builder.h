#pragma once

#include <string_view>
#include "../fs/fs.h"
#include "block_builder.h"
#include "format.h"

namespace zero_switch {

// basic guarrantee
class Table_builder {
public:
    struct Options {
        Block_builder::Options data_block_options;
        Block_builder::Options index_block_options;
    };

public:
    Table_builder(const Options& options, File_appender& appender);

    Table_builder(const Table_builder&) = delete;
    Table_builder& operator=(const Table_builder&) = delete;

public:
    // @pre key is larger than previous
    void add(std::string_view key, std::string_view value);

    void commit();

    std::uint64_t num_entries() const;
    std::uint64_t file_size() const;

private:
    void flush_();
    Block_handle flush_block_(Block_builder& builder);

private:
    Options options_;
    File_appender& appender_;

    Block_builder data_block_;
    Block_builder index_block_;

    std::uint64_t offset_ {};
    int num_of_entries_ {};
    std::string last_key_;

    bool closed_ = false;
};

}
