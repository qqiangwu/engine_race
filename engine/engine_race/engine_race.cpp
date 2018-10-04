// Copyright [2018] Alibaba Cloud All rights reserved
#include <string_view>
#include <boost/filesystem.hpp>
#include "engine_race.h"
#include "replayer.h"

using namespace zero_switch;
using namespace polar_race;
using namespace std::chrono;
using namespace std::chrono_literals;

/*
 * Complete the functions below to implement you own engine
 */

// 1. Open engine
RetCode EngineRace::Open(const std::string& name, Engine** eptr)
try {
    *eptr = new EngineRace(name);

    return kSucc;
} catch (const std::exception& e) {
    kvlog.error("Open {} failed: {}", name.c_str(), e.what());
    return kIOError;
}

EngineRace::EngineRace(const std::string& name)
    : meta_(name),
      dbfileMgr_(meta_),
      redo_alloctor_(meta_),
      commiter_(std::make_unique<Batch_commiter>(*this)),
      dumper_(std::make_unique<Dumper>(meta_.db())),
      executor_(std::make_unique<boost::basic_thread_pool>(1))
{
    replay_();
    switch_memfile_();
}

// 2. Close engine
// TODO elegant exit
EngineRace::~EngineRace() noexcept
{
    kvlog.info("destroy engine");

    executor_.reset();
    dumper_.reset();
    commiter_.reset();
}

// 3. Write a key-value pair into engine
RetCode EngineRace::Write(const PolarString& key, const PolarString& value)
try {
    auto k = std::string_view(key.data(), key.size());
    auto v = std::string_view(value.data(), value.size());
    auto fut = commiter_->submit(k, v);

    switch (fut.wait_for(10ms)) {
    case std::future_status::timeout:
        return kTimedOut;

    case std::future_status::ready:
        return fut.get() == Task_status::done? kSucc: kTimedOut;

    default:
        return kIOError;
    }
} catch (const std::exception& e) {
    kvlog.error("EngineRace::Write failed: {}", e.what());
    return kIOError;
}

// serialized by batch_commiter
bool EngineRace::write(const std::vector<std::pair<string_view, string_view>>& batch)
{
    if (!wait_for_room_()) {
        return false;
    }

    redolog_->append(batch);

    try {
        memfile_->add(batch);
        return true;
    } catch (...) {
        // we can never recover in this case, terminate is the only choice
        kvlog.critical("failed in wal and memfile, db is in unknown state");
        kvlog.flush();
        std::terminate();
    }
}

// 4. Read value of a key
RetCode EngineRace::Read(const PolarString& key, std::string* value)
try {
    auto snapshot = snapshot_();
    auto v = snapshot.read(string_view(key.data(), key.size()));
    if (v && value) {
        *value = v.value();
    }
    return v? kSucc: kNotFound;
} catch (const std::exception& e) {
    kvlog.error("EngineRace::Read failed: {}", e.what());

    return kIOError;
}

// 5. Applies the given Vistor::Visit function to the result
// of every key-value pair in the key range [first, last),
// in order
// lower=="" is treated as a key before all keys in the database.
// upper=="" is treated as a key after all keys in the database.
// Therefore the following call will traverse the entire database:
//   Range("", "", visitor)
RetCode EngineRace::Range(const PolarString& lower, const PolarString& upper, Visitor &visitor)
{
  return kSucc;
}

void EngineRace::replay_()
{
    Replayer replayer(meta_, dbfileMgr_, *dumper_);

    replayer.replay();
}

void EngineRace::switch_memfile_()
{
    auto redolog = redo_alloctor_.allocate();
    auto memfile = std::make_shared<Memfile>(redolog->id());

    if (memfile_) {
        // all or nothing
        immutable_memfiles_.emplace_back(memfile_);
    }

    swap(memfile_, memfile);
    swap(redolog_, redolog);
}

// @pre locked
bool EngineRace::wait_for_room_()
{
    const auto _2MB = 2 * 1024 * 1024;

    std::unique_lock lock(mutex_);
    if (fatal_error_) {
        if (fatal_error_fix_) {
            kvlog.critical("fatal_error: try fix");
            fatal_error_fix_();
        }
        if (fatal_error_) {
            throw Server_internal_error{"fatal_error state"};
        }
    } else if (memfile_->estimated_size() >= _2MB) {
        const auto deadline = std::chrono::system_clock::now() + 10ms;

        while (immutable_memfiles_.size() >= 32) {
            const auto status = dump_done_.wait_until(lock, deadline);
            if (status == std::cv_status::timeout) {
                return false;
            }
        }

        switch_memfile_();
        if (immutable_memfiles_.size() == 1) {
            schedule_dump_();
        }
    }

    return true;
}

// @pre locked
void EngineRace::schedule_dump_() noexcept
try {
    if (immutable_memfiles_.empty()) {
        return;
    }

    const auto file_id = meta_.next_id();
    auto memfile = immutable_memfiles_.front();

    kvlog.info("schedule dump: memfile={}, immutable_size={}", memfile->redo_id(), immutable_memfiles_.size());
    dumper_->submit(*memfile, file_id)
        .then([this, redo_id = memfile->redo_id(), file_id](auto r) {
            try {
                r.get();
                this->on_dump_completed_(redo_id, file_id);
            } catch (const std::exception& e) {
                kvlog.error("dump failed: {}", e.what());
                this->on_dump_failed_();
            }
        });
} catch (const std::exception& e) {
    kvlog.critical("schedule dump failed: {}", e.what());
} catch (...) {
    kvlog.critical("schedule dump failed: unknown error");
}

// @pre not locked
// serialized by the dumper
// if we failed to do checkpoint and failed in the half way, we are in fatal error state
void EngineRace::on_dump_completed_(const uint64_t redo_id, const uint64_t file_id) noexcept
try {
    auto files = dbfileMgr_.build_new_state(file_id);
    meta_.checkpoint(redo_id, file_id);
    dbfileMgr_.apply(std::move(files));

    {
        std::unique_lock lock(mutex_);
        immutable_memfiles_.pop_front();
        schedule_dump_();
    }

    dump_done_.notify_all();
    //executor_->submit([this]{ gc_(); });
    gc_();
} catch (...) {
    try {
        std::lock_guard lock(mutex_);
        fatal_error_ = true;

        std::function<void()> fix = [this, redo_id, file_id]{
            std::lock_guard lock(mutex_);
            fatal_error_ = false;
            fatal_error_fix_ = nullptr;
            this->on_dump_completed_(redo_id, file_id);
        };

        swap(fatal_error_fix_, fix);
    } catch (...) {
        kvlog.critical("dump callback fatal error, never recovers");
    }
}

// @pre not locked
void EngineRace::on_dump_failed_() noexcept
{
    std::lock_guard lock(mutex_);
    schedule_dump_();
}

// serialized by dumper
void EngineRace::gc_() noexcept
try {
    namespace fs = boost::filesystem;

    const auto current_redo_id = meta_.current_redo_id();
    const auto maxfile_to_gc = 8;
    std::vector<fs::path> to_be_deleted;
    to_be_deleted.reserve(maxfile_to_gc);

    for (auto& p: fs::directory_iterator(meta_.db())) {
        try {
            if (p.path().extension() == ".redo") {
                const auto stem = p.path().stem().string();
                const auto redo_id = std::stoull(stem);
                if (redo_id < current_redo_id) {
                    to_be_deleted.push_back(p.path());
                    if (to_be_deleted.size() == maxfile_to_gc) {
                        break;
                    }
                }
            }
        } catch (const std::logic_error& e) {
            kvlog.error("bad path: path={}", p.path().string());
        }
    }

    for (auto& p: to_be_deleted) {
        const auto r = fs::remove(p);
        if (r) {
            kvlog.info("redo deleted: redo={}", p.string());
        }
    }
} catch (const std::exception& e) {
    kvlog.error("gc error: {}", e.what());
} catch (...) {
    kvlog.error("gc unknown error");
}

Snapshot EngineRace::snapshot_() const
{
    std::vector<Memfile_ptr> memfiles;
    std::vector<SSTable_ptr> sstables;
    memfiles.reserve(2);

    {
        std::lock_guard lock(mutex_);
        if (memfile_) {
            memfiles.push_back(memfile_);
        }
        std::for_each(immutable_memfiles_.rbegin(), immutable_memfiles_.rend(), [&memfiles](auto& p){
            memfiles.push_back(p);
        });
    }

    sstables = dbfileMgr_.snapshot();

    return { 0, std::move(memfiles), std::move(sstables) };
}
