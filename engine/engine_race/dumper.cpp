#include <chrono>
#include <vector>
#include <memory>
#include <system_error>
#include <cerrno>
#include "dumper.h"

using namespace zero_switch;
using namespace boost;

Dumper::Dumper(const std::string& db)
    : db_(db), executor_(1)
{
}

Dumper::~Dumper()
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

    const auto start = std::chrono::system_clock::now();
    const std::string filename = db_ + '/' + std::to_string(l0file) + ".db";
    char buffer[1 * 1024 * 1024];
    FILE* file = std::fopen(filename.c_str(), "w");
    if (!file) {
        throw std::system_error(errno, std::system_category());
    }

    std::setvbuf(file, buffer, _IOFBF, sizeof(buffer));
    for (auto& x: memfile.values()) {
        const auto k = x.first;
        const auto v = x.second;
        const unsigned ksize = k.size();
        const unsigned vsize = v.size();
        const auto rc = std::fprintf(file, "%u%.*s%u%.*s", ksize, ksize, k.data(), vsize, vsize, v.data());
        if (rc < 0) {
            std::fclose(file);
            throw std::system_error(errno, std::system_category());
        }
    }

    const auto rc = std::fclose(file);
    if (rc != 0) {
        throw std::system_error(errno, std::system_category());
    }

    using namespace std::chrono;
    const auto elapsed = duration_cast<milliseconds>(system_clock::now() - start);
    kvlog.info("dump finished: mem={}, elapse={}ms", l0file, elapsed.count());
}
