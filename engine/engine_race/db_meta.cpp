#include <sys/stat.h>
#include <cassert>
#include <cerrno>
#include <system_error>
#include "db_meta.h"

using namespace zero_switch;

DBMeta::DBMeta(const std::string& db)
    : db_(db),
      redo_id_(0),
      next_redo_id_(1),
      next_file_id_(1)
{
    const auto r = ::mkdir(db.c_str(), 0755);
    if (r != 0) {
        if (errno != EEXIST) {
            throw std::system_error(errno, std::system_category(), "init db failed: " + db);
        }
    }
}

std::uint64_t DBMeta::current_redo_id() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    return redo_id_;
}

std::uint64_t DBMeta::next_redo_id()
{
    std::lock_guard<std::mutex> lock(mutex_);

    return next_redo_id_++;
}

std::uint64_t DBMeta::next_id()
{
    std::lock_guard<std::mutex> lock(mutex_);

    return next_file_id_++;
}

std::vector<std::uint64_t> DBMeta::sorted_tables() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    return sorted_tables_;
}

void DBMeta::on_dump_complete(const std::uint64_t redo_id, const std::uint64_t file_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    assert(redo_id > redo_id_);
    assert(redo_id < next_redo_id_);

    redo_id_ = redo_id;
    sorted_tables_.push_back(file_id);

    checkpoint_();
}

void DBMeta::checkpoint_()
{
    // TODO checkpoint
}
