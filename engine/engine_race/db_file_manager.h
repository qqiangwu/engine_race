#ifndef ENGINE_RACE_DB_FILE_MANAGER_H_
#define ENGINE_RACE_DB_FILE_MANAGER_H_

#include <cstdint>
#include <string_view>

namespace zero_switch {

class DBMeta;

class DBFile_manager {
public:
    explicit DBFile_manager(DBMeta& meta);

    bool read(std::string_view key, std::string* value);

public:
    void on_dump_complete(std::uint64_t file_id);
};

}

#endif
