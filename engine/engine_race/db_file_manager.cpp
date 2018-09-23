#include "db_file_manager.h"

using namespace zero_switch;

DBFile_manager::DBFile_manager(DBMeta& meta)
{
}

bool DBFile_manager::read(const std::string_view key, std::string* value)
{
    return false;
}

void DBFile_manager::on_dump_complete(const std::uint64_t file_id)
{
}
