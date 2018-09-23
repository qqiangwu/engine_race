#ifndef ENGINE_RACE_REPLAYER_H_
#define ENGINE_RACE_REPLAYER_H_

namespace zero_switch {

class DBMeta;
class DBFile_manager;
class Dumper;

class Replayer {
public:
    Replayer(DBMeta& meta, DBFile_manager& mgr, Dumper& dumper);

    void replay();

private:
    DBMeta& meta_;
    DBFile_manager& dbfileMgr_;
    Dumper& dumper_;
};

}

#endif
