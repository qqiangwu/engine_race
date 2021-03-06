#ifndef ENGINE_RACE_DUMPER_H_
#define ENGINE_RACE_DUMPER_H_

#include <stdexcept>
#include <unordered_map>
#include <atomic>
#include <boost/thread/future.hpp>
#include <boost/thread/executors/basic_thread_pool.hpp>
#include "core.h"

namespace zero_switch {

class Dumper {
public:
    explicit Dumper(const std::string& db);
    ~Dumper() noexcept;

    boost::future<void> submit(const Memfile& memfile, std::uint64_t file_id);

private:
    void run_(const Memfile& memfile, std::uint64_t l0file);

private:
    const std::string db_;
    boost::basic_thread_pool executor_;
};

}

#endif
