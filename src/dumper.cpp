#include <chrono>
#include <vector>
#include <memory>
#include <system_error>
#include <cerrno>
#include <fmt/format.h>
#include "fs/default_fs.h"
#include "sst/table_builder.h"
#include "util/stop_watch.h"
#include "dumper.h"

using namespace zero_switch;
using namespace boost;

using fmt::format;

Dumper::Dumper(const std::string& db)
    : db_(db), executor_(1)
{
}

Dumper::~Dumper() noexcept
{
    kvlog.info("dumper destroyed");
}

future<void> Dumper::submit(const Memfile& memfile, const std::uint64_t l0file)
{
    packaged_task<void> task([this, &memfile, l0file]{
        this->run_(memfile, l0file);
    });
    auto r = task.get_future();

    executor_.submit(std::move(task));

    return r;
}

void Dumper::run_(const Memfile& memfile, const uint64_t l0file)
{
    kvlog.info("dump start: mem={}", l0file);

    const auto file_buffer_size = 1 * 1024 * 1024;
    const std::string filename = format("{}/{}.db", db_, l0file);

    Stop_watch watch;
    Table_builder::Options options;
    options.index_block_options.block_restart_interval = 1;

    Default_file_appender appender(filename, file_buffer_size);
    Table_builder table_builder(options, appender);

    for (auto [k, v]: memfile.values()) {
        table_builder.add(k, v);
    }

    appender.sync();
    appender.close();

    // commit
    kvlog.info("dump finished: mem={}, elapse={}ms", l0file, watch.lap().count());
}
