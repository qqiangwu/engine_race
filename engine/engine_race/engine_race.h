// Copyright [2018] Alibaba Cloud All rights reserved
#ifndef ENGINE_RACE_ENGINE_RACE_H_
#define ENGINE_RACE_ENGINE_RACE_H_

#include <string>
#include <mutex>
#include <deque>
#include <condition_variable>
#include "include/engine.h"
#include "core.h"
#include "batch_commiter.h"
#include "dumper.h"

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

    void on_dump_completed_(uint64_t redo_id, uint64_t file_id);
    void on_dump_failed_();
    void gc_();

    void try_dump_();

private:
    // protecte memfiles and redolog
    std::mutex mutex_;
    std::condition_variable dump_done_;

    Memfile_ptr memfile_;
    std::deque<Memfile_ptr> immutable_memfiles_;
    Redo_log_ptr redolog_;

    DBMeta meta_;
    DBFile_manager dbfileMgr_;

    Batch_commiter commiter_;
    Dumper dumper_;
};

}  // namespace polar_race

#endif  // ENGINE_RACE_ENGINE_RACE_H_
