// Copyright [2018] Alibaba Cloud All rights reserved
#include <leveldb/cache.h>
#include "engine_race.h"

namespace polar_race {

struct Leveldb_error : public std::runtime_error {
    Leveldb_error(leveldb::Status s)
        : std::runtime_error("leveldb error: " + s.ToString()),
          status_(s)
    {
    }

    leveldb::Status status() const
    {
        return status_;
    }

private:
    leveldb::Status status_;
};

RetCode translate(leveldb::Status s)
{
    if (s.ok()) {
        return kSucc;
    } else if (s.IsCorruption()) {
        return kCorruption;
    } else if (s.IsNotFound()) {
        return kNotFound;
    } else {
        return kIOError;
    }
}

RetCode Engine::Open(const std::string& name, Engine** eptr) {
  return EngineRace::Open(name, eptr);
}

Engine::~Engine() {
}

/*
 * Complete the functions below to implement you own engine
 */

// 1. Open engine
RetCode EngineRace::Open(const std::string& name, Engine** eptr)
try {
    *eptr = NULL;
    EngineRace *engine_race = new EngineRace(name);

    *eptr = engine_race;
    return kSucc;
} catch (const Leveldb_error& e) {
    return translate(e.status());
}

EngineRace::EngineRace(const std::string& name)
{
    leveldb::Options options;
    options.create_if_missing = true;
    options.block_cache = leveldb::NewLRUCache(1 * 1024 * 1024 * 1024);

    leveldb::DB* db = nullptr;
    auto status = leveldb::DB::Open(options, name, &db);
    if (!status.ok()) {
        throw Leveldb_error(status);
    }

    db_.reset(db);
}

// 2. Close engine
EngineRace::~EngineRace()
{
}

// 3. Write a key-value pair into engine
RetCode EngineRace::Write(const PolarString& key, const PolarString& value)
{
    auto k = leveldb::Slice(key.data(), key.size());
    auto v = leveldb::Slice(value.data(), value.size());
    auto status = db_->Put(leveldb::WriteOptions(), k, v);
    return translate(status);
}

// 4. Read value of a key
RetCode EngineRace::Read(const PolarString& key, std::string* value)
{
    auto k = leveldb::Slice(key.data(), key.size());
    auto s = db_->Get(leveldb::ReadOptions(), k, value);
    return translate(s);
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

}  // namespace polar_race
