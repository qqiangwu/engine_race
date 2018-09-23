// Copyright [2018] Alibaba Cloud All rights reserved
#include <iostream>
#include <string_view>
#include "engine_race.h"
#include "replayer.h"

using namespace zero_switch;
using namespace polar_race;

RetCode Engine::Open(const std::string& name, Engine** eptr)
{
    return EngineRace::Open(name, eptr);
}

Engine::~Engine()
{
}

/*
 * Complete the functions below to implement you own engine
 */

// 1. Open engine
RetCode EngineRace::Open(const std::string& name, Engine** eptr)
try {
    *eptr = new EngineRace(name);

    return kSucc;
} catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return kIOError;
}

EngineRace::EngineRace(const std::string& name)
    : meta_(name),
      dbfileMgr_(meta_),
      dumper_(meta_.db())
{
    replay_();
    roll_new_memfile_();
}

// 2. Close engine
// TODO elegant exit
EngineRace::~EngineRace()
{
}

// 3. Write a key-value pair into engine
RetCode EngineRace::Write(const PolarString& key, const PolarString& value)
try {
    std::unique_lock<std::mutex> lock(mutex_);

    wait_for_room_(lock);
    append_log_(key, value);
    apply_(key, value);

    return kSucc;
} catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;

    return kIOError;
}

// 4. Read value of a key
RetCode EngineRace::Read(const PolarString& key, std::string* value)
try {
    if (read_memfile_(memfile_, key, value)) {
        return kSucc;
    }
    if (dbfileMgr_.read(std::string_view(key.data(), key.size()), value)) {
        return kSucc;
    }
    return kNotFound;
} catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;

    return kIOError;
}

// 5. Applies the given Vistor::Visit function to the result
// of every key-value pair in the key range [first, last),
// in order
// lower=="" is treated as a key before all keys in the database.
// upper=="" is treated as a key after all keys in the database.
// Therefore the following call will traverse the entire database:
//   Range("", "", visitor)
RetCode EngineRace::Range(const PolarString& lower, const PolarString& upper,
    Visitor &visitor) {
  return kSucc;
}

void EngineRace::replay_()
{
    Replayer replayer(meta_, dbfileMgr_, dumper_);

    replayer.replay();
}

void EngineRace::roll_new_memfile_()
{
    memfile_ = std::make_shared<Memfile>();
    redolog_ = std::make_shared<Redo_log>(meta_.db(), meta_.next_redo_id());
}

// @pre locked
void EngineRace::wait_for_room_(std::unique_lock<std::mutex>& lock)
{
    if (memfile_->size() >= 1024) {
        while (immutable_memfile_ && memfile_->size() >= 1024) {
            dump_done_.wait(lock);
        }

        if (memfile_->size() >= 1024) {
            submit_memfile_(memfile_);
            roll_new_memfile_();
        }
    }
}

// @pre locked
void EngineRace::submit_memfile_(const Memfile_ptr& memfile)
{
    const auto file_id = meta_.next_id();
    const auto redo_id = redolog_->id();
    dumper_.submit(*memfile_, file_id)
        .then([this, redo_id, file_id](auto r) {
            try {
                r.get();
                this->on_dump_completed_(redo_id, file_id);
            } catch (const std::exception& e) {
                std::cout << "dump failed: " << e.what() << std::endl;
                this->on_dump_failed_();
            }
        });

    immutable_memfile_ = memfile_;
}

// @pre not locked
void EngineRace::on_dump_completed_(const uint64_t redo_id, const uint64_t file_id)
{
    // transition serialized by dumper
    meta_.on_dump_complete(redo_id, file_id);
    dbfileMgr_.on_dump_complete(file_id);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        immutable_memfile_ = nullptr;
    }

    dump_done_.notify_all();
    //gc_();
}

// @pre not locked
void EngineRace::on_dump_failed_()
{
    std::terminate();
}

// serialized by dumper
// TODO
void EngineRace::gc_()
{
}

void EngineRace::append_log_(const PolarString& key, const PolarString& value)
{
    auto k = std::string_view(key.data(), key.size());
    auto v = std::string_view(value.data(), value.size());
    redolog_->append(k, v);
}

void EngineRace::apply_(const PolarString& key, const PolarString& value)
{
    memfile_->emplace(key.ToString(), value.ToString());
}

bool EngineRace::read_memfile_(const Memfile_ptr& memfile, const PolarString& key, std::string* value)
{
    auto iter = memfile_->find(key.ToString());
    if (iter == memfile_->end()) {
        return false;
    }

    *value = iter->second;
    return true;
}
