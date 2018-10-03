// Copyright [2018] Alibaba Cloud All rights reserved
#include <string_view>
#include "engine_race.h"
#include "replayer.h"

using namespace zero_switch;
using namespace polar_race;

/*
 * Complete the functions below to implement you own engine
 */

// 1. Open engine
RetCode EngineRace::Open(const std::string& name, Engine** eptr)
try {
    *eptr = new EngineRace(name);

    return kSucc;
} catch (const std::exception& e) {
    kvlog.error("Open %s failed: %s", name.c_str(), e.what());
    return kIOError;
}

EngineRace::EngineRace(const std::string& name)
    : meta_(name),
      dbfileMgr_(meta_),
      redo_alloctor_(meta_),
      commiter_(std::make_unique<Batch_commiter>(*this)),
      dumper_(std::make_unique<Dumper>(meta_.db()))
{
    replay_();
    roll_new_memfile_();
}

// 2. Close engine
// TODO elegant exit
EngineRace::~EngineRace() noexcept
{
    kvlog.info("destroy engine");

    dumper_.reset();
    commiter_.reset();
}

// 3. Write a key-value pair into engine
RetCode EngineRace::Write(const PolarString& key, const PolarString& value)
try {
    auto k = std::string_view(key.data(), key.size());
    auto v = std::string_view(value.data(), value.size());
    commiter_->submit(k, v);

    return kSucc;
} catch (const Server_busy&) {
    return kTimedOut;
} catch (const std::exception& e) {
    kvlog.error("EngineRace::Write failed: %s", e.what());
    return kIOError;
}

// serialized by batch_commiter
void EngineRace::write(const std::vector<std::pair<string_view, string_view>>& batch)
{
    wait_for_room_();
    redolog_->append(batch);

    try {
        for (auto [key, value]: batch) {
            memfile_->add(key, value);
        }
    } catch (...) {
        kvlog.error("failed in wal and memfile, db is in unknown state");
        health_ = false;
        throw;
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
    kvlog.error("EngineRace::Read failed: %s", e.what());

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

void EngineRace::roll_new_memfile_()
{
    assert(!memfile_);
    assert(!redolog_);

    auto memfile = std::make_shared<Memfile>();
    auto redolog = redo_alloctor_.allocate();

    swap(memfile_, memfile);
    swap(redolog_, redolog);
    health_ = true;
}

// @pre locked
void EngineRace::wait_for_room_()
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
    } else if (!memfile_) {
        kvlog.warn("memfile is null: mem={}, redo={}", !!memfile_.get(), !!redolog_.get());
        roll_new_memfile_();
    } else if (!health_ || memfile_->estimated_size() >= _2MB) {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        const auto deadline = std::chrono::system_clock::now() + 10ms;

        while (immutable_memfile_) {
            const auto status = dump_done_.wait_until(lock, deadline);
            if (status == std::cv_status::timeout) {
                throw Server_busy{"background dump is ongoing for a long time"};
            }
        }

        submit_memfile_(memfile_);
    }
}

// @pre locked
void EngineRace::submit_memfile_(const Memfile_ptr& memfile)
{
    assert(memfile_);
    assert(redolog_);

    const auto file_id = meta_.next_id();
    dumper_->submit(*memfile_, file_id)
        .then([this, redo = std::move(redolog_), file_id](auto r) {
            try {
                r.get();
                this->on_dump_completed_(redo->id(), file_id);
            } catch (const std::exception& e) {
                kvlog.error("dump failed: %s", e.what());
                this->on_dump_failed_();
            }
        });

    swap(memfile_, immutable_memfile_);
    memfile_ = nullptr;
    redolog_ = nullptr;

    roll_new_memfile_();
}

// @pre not locked
// serialized by the dumper
// if we failed to do checkpoint and failed in the half way, we are in fatal error state
void EngineRace::on_dump_completed_(const uint64_t redo_id, const uint64_t file_id) noexcept
try {
    auto files = dbfileMgr_.build_new_state(file_id);
    meta_.checkpoint(redo_id, file_id);
    dbfileMgr_.apply(std::move(files));

    Memfile_ptr trash {};
    {
        std::unique_lock lock(mutex_);
        swap(immutable_memfile_, trash);
    }

    dump_done_.notify_all();

    // TODO: add gc
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
    std::terminate();
}

// serialized by dumper
// TODO
void EngineRace::gc_()
{
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
        if (immutable_memfile_) {
            memfiles.push_back(immutable_memfile_);
        }
    }

    sstables = dbfileMgr_.snapshot();

    return { 0, std::move(memfiles), std::move(sstables) };
}
