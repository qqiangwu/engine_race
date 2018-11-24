#include "db_meta.h"
#include "db_file_manager.h"

using namespace zero_switch;

DBFile_manager::DBFile_manager(DBMeta& meta)
    : meta_(meta)
{
}

std::vector<SSTable_ptr> DBFile_manager::snapshot() const
{
    std::shared_lock lock(rwmutex_);
    return sstables_;
}

std::vector<std::string> DBFile_manager::reachable_files() const
{
    std::vector<std::string> files;
    files.reserve(sstables_.size() + zombies_.size());

    std::shared_lock lock(rwmutex_);
    files.insert(files.end(), zombies_.begin(), zombies_.end());
    for (auto& p: sstables_) {
        files.push_back(p->path());
    }

    return files;
}

std::vector<SSTable_ptr> DBFile_manager::build_new_state(const uint64_t file_id)
{
    auto tmp = std::make_unique<SSTable>(meta_, file_id);
    auto file = SSTable_ptr(tmp.release(), [this](auto p){
        this->on_sstable_removed_(p);
        delete p;
    });
    auto files = snapshot();

    files.push_back(file);

    return files;
}

void DBFile_manager::apply(std::vector<SSTable_ptr>&& files) noexcept
{
    swap(sstables_, files);
}

void DBFile_manager::on_sstable_removed_(SSTable* table) noexcept
{
    auto& path = table->path();
    zombies_.erase(path);
}
