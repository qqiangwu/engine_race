#include "core.h"
#include "replayer.h"

using namespace zero_switch;

Replayer::Replayer(DBMeta& meta, DBFile_manager& mgr, Dumper& dumper)
    : meta_(meta),
      dbfileMgr_(mgr),
      dumper_(dumper)
{
}

void Replayer::replay()
{
}
