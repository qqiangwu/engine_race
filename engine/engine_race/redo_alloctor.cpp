#include "redo_alloctor.h"
#include "db_meta.h"

using namespace zero_switch;

Redo_allocator::Redo_allocator(DBMeta& meta)
    : meta_(meta)
{
}

Redo_log_ptr Redo_allocator::allocate()
{
    if (!future_.valid()) {
        future_ = do_allocate_();
    }

    auto p = future_.get();

    try {
        future_ = do_allocate_();
    } catch (...) {
    }

    return p;
}

boost::future<Redo_log_ptr> Redo_allocator::do_allocate_()
{
    boost::packaged_task<Redo_log_ptr> task([this]{
        return std::make_shared<Redo_log>(meta_.db(), meta_.next_redo_id());
    });

    auto f = task.get_future();
    executor_.submit(std::move(task));

    return f;
}
