#include <cstdio>
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
    std::vector<std::pair<const std::string*, const std::string*>> table;
    table.reserve(memfile.size());

    for (auto& kv: memfile) {
        table.emplace_back(&kv.first, &kv.second);
    }

    std::sort(table.begin(), table.end(), [](const auto a, const auto b){
        return *a.first < *b.first;
    });

    const std::string filename = db_ + '/' + std::to_string(l0file) + ".db";
    char buffer[1 * 1024 * 1024];
    FILE* file = std::fopen(filename.c_str(), "w");
    if (!file) {
        throw std::system_error(errno, std::system_category());
    }

    std::setvbuf(file, buffer, _IOFBF, sizeof(buffer));
    for (auto& x: table) {
        const auto k = x.first;
        const auto v = x.second;
        const auto rc = std::fprintf(file, "%u%s%u%s", unsigned(k->size()), k->c_str(), unsigned(v->size()), v->c_str());
        if (rc < 0) {
            std::fclose(file);
            throw std::system_error(errno, std::system_category());
        }
    }

    const auto rc = std::fclose(file);
    if (rc != 0) {
        throw std::system_error(errno, std::system_category());
    }
}
