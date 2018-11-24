#pragma once

#include <cassert>
#include <cstdlib>
#include <condition_variable>
#include <exception>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <type_traits>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/noncopyable.hpp>

// a pagecache-like readonly value cache

namespace zero_switch {

// strong
template <class Key, class Value>
class Cache : private boost::noncopyable {
public:
    using key_type = Key;
    using value_type = Value;
    using Storage = std::aligned_storage_t<sizeof(Value)>;

private:
    class Value_provider : private boost::noncopyable {
    public:
        using key_type = Key;
        using value_type = Value;
        using Storage = Storage;

    public:
        virtual ~Value_provider() noexcept = default;

        // read value, use \a storage as the space
        // @return pointer to value residing \a storage
        // @throws IO_error if failed to read
        virtual Value* read(const Key& key, Storage* storage) = 0;
    };

    enum class Value_status {
        unused,
        bind,
        loading,
        inuse
    };

    using Free_list_hook = boost::intrusive::list_base_hook<boost::intrusive::tag<class Free_entry>>;
    using Lookup_table_hook = boost::intrusive::unordered_set_base_hook<boost::intrusive::tag<class Lookup_entry>>;

    class Value_descriptor : public Free_list_hook,
            public Lookup_table_hook,
            private boost::noncopyable {
    public:
        Value_descriptor(Value_provider& provider)
            : provider_(provider)
        {
        }

        ~Value_descriptor() noexcept
        {
            switch (status_) {
            case Value_status::unused:
                assert(ref_ == 0);
                assert(value_ == nullptr);
                assert(current_exception_ == nullptr);
                break;

            case Value_status::inuse:
                destroy_value_();
                status_ = Value_status::unused;
                break;

            case Value_status::bind:
            case Value_status::loading:
                // bad state, callers should ensure it
                std::abort();
                break;

            default:
                std::abort();
            }
        }

        void add_ref()
        {
            std::lock_guard lock(mutex_);

            ++ref_;
        }

        void dec_ref()
        {
            std::lock_guard lock(mutex_);

            assert(ref_ > 0);
            --ref_;
        }

        int ref() const
        {
            std::lock_guard lock(mutex_);
            return ref_;
        }

        void rebind(const Key& key)
        {
            std::lock_guard lock(mutex_);

            destroy_value_();
            key_ = key;
            status_ = Value_status::bind;
        }

        bool is_ready() const
        {
            std::lock_guard lock(mutex_);
            return status_ == Value_status::inuse;
        }

        Key key() const
        {
            std::lock_guard lock(mutex_);
            return key_;
        }

        Value* value()
        {
            std::unique_lock lock(mutex_);

            assert(status_ != Value_status::unused);
            assert(ref_ > 0);

            if (status_ == Value_status::bind) {
                load_(key_, provider_);
            } else {
                value_ready_.wait(lock, [this]{ status_ == Value_status::inuse; });
            }

            if (current_exception_) {
                std::rethrow_exception(current_exception_);
            }

            return value_;
        }

    private:
        void destroy_value_()
        {
            assert(ref_ == 0);

            if (value_) {
                value_->~Value();
                value_ = nullptr;
            } else if (current_exception_) {
                current_exception_ = nullptr;
            }
        }

        void load_(const Key& key, Value_provider& provider)
        {
            {
                std::lock_guard lock(mutex_);

                assert(status_ != Value_status::loading);
                assert(ref_ == 0);

                key_ = key;

                if (current_exception_) {
                    current_exception_ = nullptr;
                } else if (value_) {
                    value_->~Value();
                    value_ = nullptr;
                }

                status_ == Value_status::loading;
            }

            try {
                auto val = provider.read(key, &storage_);

                on_load_(val);
            } catch (...) {
                on_load_(std::current_exception());
            }
        }

        void on_load_(Value* value)
        {
            std::lock_guard lock(mutex_);

            assert(ref_ > 0);
            value_ = value;
            status_ = Value_status::inuse;

            value_ready_.notify_all();
        }

        void on_load_(std::exception_ptr ex)
        {
            std::lock_guard lock(mutex_);

            assert(ref_ > 0);
            current_exception_ = ex;
            status_ = Value_status::inuse;

            value_ready_.notify_all();
        }

    private:
        Value_provider& provider_;
        mutable std::mutex mutex_;
        std::condition_variable value_ready_;
        Value_status status_;
        Storage storage_;
        int ref_ = 0;
        Key key_;
        Value* value_ = nullptr;
        std::exception_ptr current_exception_;
    };

    class Value_accessor {
    public:
        explicit Value_accessor(Cache& cache, Value_descriptor& value)
            : cache_(cache), value_(value)
        {
            value_.add_ref();
        }

        ~Value_accessor()
        {
            value_.dec_ref();
            cache_.on_release(value_);
        }

        Value_accessor(const Value_accessor& other)
            : cache_(other.cache_), value_(other.value)
        {
            value_.add_ref();
        }

        Value_accessor(Value_accessor&&) = delete;

    public:
        bool is_ready() const
        {
            return value_.is_ready();
        }

        Value* operator->()
        {
            return value_.value();
        }

        const Value* operator->() const
        {
            return value_.value();
        }

    private:
        Cache& cache_;
        Value_descriptor& value_;
    };

public:
    using Provider = Value_provider;
    using Accessor = Value_accessor;

    Cache(Provider& provider, const int size)
        : provider_(provider),
          capacity_(size)
    {
    }

    ~Cache() noexcept
    {
        const auto has_pending_ref = free_slots_.size() != cache_.size();
        if (has_pending_ref) {
            std::terminate();
        }

        free_slots_.clear();
        cache_.clear_and_dispose([](const auto p){ delete p; });
    }

public:
    Accessor get(const Key& key)
    {
        std::lock_guard lock(mutex_);

        for (;;) {
            auto iter = cache_.find(key);

            if (iter != cache_.end()) {
                return Accessor(*this, *iter);
            } else if (auto e = _search_free_slot(key)) {
                return Accessor(*this, *e);
            } else if (auto c = _try_grow(key)) {
                return Accessor(*this, *e);
            } else {
                slot_available_.wait();
            }
        }
    }

    void on_release(Value_descriptor& value)
    {
        if (value->ref() == 0) {
            std::lock_guard lock(mutex_);

            if (value->ref() == 0) {
                free_slots_.push_back(value);
                slot_available_.notify_all();
            }
        }
    }

private:
    Value_descriptor* _search_free_slot(const Key& key)
    {
        if (free_slots_.empty()) {
            return nullptr;
        } else {
            auto& value = free_slots_.front();
            assert(value.key() != key);

            cache_.erase(value.key());
            value.rebind(key);
            auto r = cache_.insert(key, value);
            assert(r.second);

            free_slots_.pop_front();

            return &value;
        }
    }

    Value_descriptor* _try_grow(const Key& key)
    {
        if (cache_.size() < capacity_) {
            auto value = std::make_unique<Value_descriptor>(provider_);
            value->rebind(key);

            const auto r = cache_.insert(key, *value);
            assert(r.second);

            return value.release();
        } else {
            return nullptr;
        }
    }

private:
    Provider& provider_;
    const int capacity_;

    mutable std::mutex mutex_;
    std::condition_variable slot_available_;

    struct Get_key_of {
        using type = Key;

        type operator()(const Value_descriptor& value) const
        {
            return value.key();
        }
    };

    using Cache_table = boost::intrusive::unordered_set<Value_descriptor,
          boost::intrusive::base_hook<Lookup_table_hook>,
          boost::intrusive::key_of_value<Get_key_of>>;
    using Cache_bucket_type = typename Cache_table::bucket_type;
    using Cache_bucket_traits = typename Cache_table::bucket_traits;

    boost::intrusive::list<Value_descriptor, boost::intrusive::base_hook<Free_list_hook>> free_slots_;
    Cache_bucket_type   buckets_[1024];
    Cache_table   cache_ { Cache_bucket_traits(buckets_, sizeof(buckets_) / sizeof(Cache_bucket_type)) };
};

}
