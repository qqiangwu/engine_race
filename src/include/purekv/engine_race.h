// Copyright [2018] Alibaba Cloud All rights reserved
#ifndef ENGINE_RACE_ENGINE_RACE_H_
#define ENGINE_RACE_ENGINE_RACE_H_

#include <string>
#include <condition_variable>
#include <shared_mutex>
#include <functional>
#include <deque>
#include "engine.h"
#include "core.h"
#include "purekv/batch_commiter.h"
#include "dumper.h"
#include "redo_alloctor.h"
#include "snapshot.h"

namespace polar_race {

using namespace zero_switch;

class EngineRace: public Engine, public Kv_updater {
public:
    static RetCode Open(const std::string& name, Engine** eptr);

    explicit EngineRace(const std::string& dir);

    ~EngineRace() noexcept;

    RetCode Write(const PolarString& key, const PolarString& value) override;

    RetCode Read(const PolarString& key, std::string* value) override;

    RetCode Range(const PolarString& lower, const PolarString& upper,
            Visitor &visitor) override;

    bool write(const std::vector<std::pair<std::string_view, std::string_view>>& batch) override final;

private:
    void replay_();
    void switch_memfile_();

    bool wait_for_room_(const std::vector<std::pair<string_view, string_view>>& batch);
    void schedule_dump_() noexcept;

    void append_log_(const PolarString& key, const PolarString& value);
    void apply_(const PolarString& key, const PolarString& value);

    Snapshot snapshot_() const;

    // called by dumper
    void on_dump_completed_(uint64_t redo_id, uint64_t file_id) noexcept;
    void on_dump_failed_() noexcept;
    void gc_() noexcept;

private:
    mutable std::mutex mutex_;
    std::condition_variable dump_done_;

    Memfile_ptr memfile_;
    std::deque<Memfile_ptr> immutable_memfiles_;
    Redo_log_ptr redolog_;

    DBMeta meta_;
    DBFile_manager dbfileMgr_;

    std::unique_ptr<boost::basic_thread_pool> executor_;

    Redo_allocator redo_alloctor_;
    std::unique_ptr<Batch_commiter> commiter_;
    std::unique_ptr<Dumper> dumper_;
};

}  // namespace polar_race

#endif  // ENGINE_RACE_ENGINE_RACE_H_
