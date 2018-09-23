#ifndef ENGINE_RACE_CORE_H_
#define ENGINE_RACE_CORE_H_

#include <cstdint>
#include <memory>
#include <unordered_map>
#include "db_meta.h"
#include "db_file_manager.h"
#include "redo_log.h"

namespace zero_switch {

using Memfile = std::unordered_map<std::string, std::string>;
using Memfile_ptr = std::shared_ptr<Memfile>;

}

#endif
