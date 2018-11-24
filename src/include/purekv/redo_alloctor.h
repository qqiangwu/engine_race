#ifndef ENGINE_REDO_ALLOCATOR_H_
#define ENGINE_REDO_ALLOCATOR_H_

#include <boost/thread/future.hpp>
#include <boost/thread/executors/basic_thread_pool.hpp>
#include "redo_log.h"

namespace zero_switch {

class DBMeta;

// strong guarantee
class Redo_allocator {
public:
    explicit Redo_allocator(DBMeta& meta);

    Redo_log_ptr allocate();

private:
    boost::future<Redo_log_ptr> do_allocate_();

private:
    DBMeta& meta_;
    boost::basic_thread_pool executor_;
    boost::future<Redo_log_ptr> future_;
};

}

#endif
