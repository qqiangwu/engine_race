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
      commiter_(*this),
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
    auto k = std::string_view(key.data(), key.size());
    auto v = std::string_view(value.data(), value.size());
    commiter_.submit(k, v);

    return kSucc;
} catch (const std::exception& e) {
    std::cerr << __func__ << e.what() << std::endl;

    return kIOError;
}

// serialized by batch_commiter
void EngineRace::write(const std::vector<std::pair<std::string_view, std::string_view>>& batch)
{
    wait_for_room_();

    redolog_->append(batch);

    for (auto [key, value]: batch) {
        memfile_->add(key, value);
    }
}

// 4. Read value of a key
RetCode EngineRace::Read(const PolarString& key, std::string* value)
try {
    // TODO
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
RetCode EngineRace::Range(const PolarString& lower, const PolarString& upper, Visitor &visitor)
{
  return kSucc;
}

void EngineRace::replay_()
{
    Replayer replayer(meta_, dbfileMgr_, dumper_);

    replayer.replay();
}

void EngineRace::roll_new_memfile_()
{
    assert(!memfile_);
    assert(!redolog_);

    const auto next_redo_id = meta_.next_redo_id();
    memfile_ = std::make_shared<Memfile>(next_redo_id);
    redolog_ = std::make_shared<Redo_log>(meta_.db(), next_redo_id);
}

// @pre locked
void EngineRace::wait_for_room_()
{
    const auto _2MB = 2 * 1024 * 1024;

    if (memfile_->estimated_size() >= _2MB) {
        std::unique_lock<std::mutex> lock(mutex_);

        // FIXME: make configurable
        while (immutable_memfiles_.size() >= 4) {
            dump_done_.wait(lock);
        }

        submit_memfile_(memfile_);
        roll_new_memfile_();
    }
}

// @pre locked
void EngineRace::submit_memfile_(const Memfile_ptr& memfile)
{
    immutable_memfiles_.push_back(memfile_);
    memfile_ = nullptr;
    redolog_ = nullptr;

    if (immutable_memfiles_.size() == 1) {
        try_dump_();
    }
}

void EngineRace::try_dump_()
{
    if (immutable_memfiles_.empty()) {
        return;
    }

    const auto file_id = meta_.next_id();
    auto imm = immutable_memfiles_.front();
    dumper_.submit(*imm, file_id)
        .then([this, redo_id = imm->redo_id(), file_id](auto r) {
            try {
                r.get();
                this->on_dump_completed_(redo_id, file_id);
            } catch (const std::exception& e) {
                std::cout << "dump failed: " << e.what() << std::endl;
                this->on_dump_failed_();
            }
        });
}

// @pre not locked
void EngineRace::on_dump_completed_(const uint64_t redo_id, const uint64_t file_id)
{
    // transition serialized by dumper
    meta_.on_dump_complete(redo_id, file_id);
    dbfileMgr_.on_dump_complete(file_id);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        immutable_memfiles_.pop_front();
        try_dump_();
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
