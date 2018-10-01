// Copyright [2018] Alibaba Cloud All rights reserved
#ifndef ENGINE_RACE_ENGINE_RACE_H_
#define ENGINE_RACE_ENGINE_RACE_H_

#include <string>
#include <mutex>
#include <condition_variable>
#include "include/engine.h"
#include "core.h"
#include "batch_commiter.h"
#include "dumper.h"
#include "redo_alloctor.h"

namespace polar_race {

using namespace zero_switch;

class EngineRace: public Engine, public Kv_updater {
public:
    static RetCode Open(const std::string& name, Engine** eptr);

    explicit EngineRace(const std::string& dir);

    ~EngineRace();

    RetCode Write(const PolarString& key, const PolarString& value) override;

    RetCode Read(const PolarString& key, std::string* value) override;

    RetCode Range(const PolarString& lower, const PolarString& upper,
            Visitor &visitor) override;

    void write(const std::vector<std::pair<std::string_view, std::string_view>>& batch) override;

private:
    void replay_();
    void roll_new_memfile_();

    void wait_for_room_();
    void submit_memfile_(const Memfile_ptr& memfile);

    void append_log_(const PolarString& key, const PolarString& value);
    void apply_(const PolarString& key, const PolarString& value);

    // called by dumper
    void on_dump_completed_(uint64_t redo_id, uint64_t file_id);
    void on_dump_failed_();
    void gc_();

private:
    std::mutex mutex_;
    std::condition_variable dump_done_;

    Memfile_ptr memfile_;
    Memfile_ptr immutable_memfile_;
    Redo_log_ptr redolog_;

    DBMeta meta_;
    DBFile_manager dbfileMgr_;

    Batch_commiter commiter_;
    Dumper dumper_;
    Redo_allocator redo_alloctor_;
};

}  // namespace polar_race

#endif  // ENGINE_RACE_ENGINE_RACE_H_
