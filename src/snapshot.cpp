#include "snapshot.h"

using namespace zero_switch;

Snapshot::Snapshot(uint64_t seq, std::vector<Memfile_ptr> memfiles, std::vector<SSTable_ptr> sstables)
    : seq_(seq),
      memfiles_(std::move(memfiles)),
      sstables_(std::move(sstables))
{
}

optional<std::string> Snapshot::read(const string_view key)
{
    for (auto& memfile: memfiles_) {
        auto r = memfile->read(key);
        if (r) {
            return { std::string(*r) };
        }
    }

    for (auto& file: sstables_) {
        auto r = file->read(key);
        if (r) {
            return { std::string(*r) };
        }
    }

    return std::nullopt;
}
