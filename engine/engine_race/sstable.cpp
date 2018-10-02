#include "db_meta.h"
#include "sstable.h"

using namespace zero_switch;

SSTable::SSTable(DBMeta& meta, const std::uint64_t file_id)
    : meta_(meta),
      file_id_(file_id),
      path_(meta_.db() + "/" + std::to_string(file_id_) + ".db")
{
}

const std::string& SSTable::path() const
{
    return path_;
}

string_view SSTable::start() const
{
    return {};
}

string_view SSTable::end() const
{
    return {};
}


optional<std::string> SSTable::read(string_view) const
{
    return nullptr;
}
