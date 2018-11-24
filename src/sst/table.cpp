#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include "engine_error.h"
#include "fs/default_fs.h"
#include "sst/table.h"
#include "sst/format.h"
#include "sst/error.h"

using namespace std;
using namespace zero_switch;
using fmt::format;

namespace fs = boost::filesystem;

namespace {

std::string allocate_fix_size_buffer(const std::uint64_t size)
{
    return std::string(size, ' ');
}

Footer load_footer(Random_access_file& reader, const std::string& table_path, const std::uint64_t id)
{
    const auto path = fs::path(table_path);
    const auto file_size = fs::file_size(path);
    if (file_size < Footer::footer_size) {
        throw Data_corruption_error{ format("sst table {} at {} has too little size", id, table_path) };
    }

    const auto footer_offset = file_size - Footer::footer_size;
    auto buffer = allocate_fix_size_buffer(Footer::footer_size);
    const auto bytes_read = reader.read(footer_offset, buffer);
    if (bytes_read != Footer::footer_size) {
        throw_sys_error<IO_error>(format("read footer failed: file={}, offset={}, bytes_read={}, expect={}",
                table_path,
                footer_offset,
                bytes_read,
                Footer::footer_size));
    }

    std::string_view buffer_view = buffer;
    return Footer::decode(buffer_view);
}

Block load_index_block(Random_access_file& reader,
        const std::string& table_path, const std::uint64_t id, const Footer footer)
{
    const auto index_block_handle = footer.bock_index_handle();
    auto buffer = allocate_fix_size_buffer(index_block_handle.size());
    const auto bytes_read = reader.read(index_block_handle.offset(), buffer);
    if (bytes_read != index_block_handle.size()) {
        throw_sys_error<IO_error>(format("read index block failed: file={}, offset={}, bytes_read={}, expect={}",
                table_path,
                index_block_handle.offset(),
                bytes_read,
                index_block_handle.size()));
    }
    return Block(index_block_handle, std::move(buffer));
}

Block load_table(Random_access_file& reader, const std::string& table_path, const std::uint64_t id)
{
    const auto footer = load_footer(reader, table_path, id);
    return load_index_block(reader, table_path, id, footer);
}

}

Table::Table(const std::string& path, const std::uint64_t id, Block_cache& block_cache)
    : path_(path),
      id_(id),
      file_reader_(std::make_unique<Default_random_access_file>(path)),
      block_cache_(block_cache),
      index_block_(load_table(*file_reader_, path, id))
{
}