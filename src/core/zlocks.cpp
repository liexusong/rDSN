# include <rdsn/internal/zlocks.h>
# include "service_engine.h"
# include <rdsn/internal/factory_store.h>

using namespace rdsn::utils;

namespace rdsn { namespace service {

    namespace lock_checker 
    {
        __thread int zlock_exclusive_count = 0;
        __thread int zlock_shared_count = 0;

        void check_wait_safety()
        {
            if (zlock_exclusive_count + zlock_shared_count > 0)
            {
                rassert(false, "wait inside locks may lead to deadlocks - current thread owns %u exclusive locks and %u shared locks now.",
                    zlock_exclusive_count, zlock_shared_count
                    );
            }
        }

        void check_dangling_lock()
        {
            if (zlock_exclusive_count + zlock_shared_count > 0)
            {
                rassert(false, "locks should not be hold at this point - current thread owns %u exclusive locks and %u shared locks now.",
                    zlock_exclusive_count, zlock_shared_count
                    );
            }
        }
    }

zlock::zlock(void)
{
    lock_provider* last = factory_store<lock_provider>::create(service_engine::instance().spec().lock_factory_name.c_str(), PROVIDER_TYPE_MAIN, this, nullptr);

    // TODO: perf opt by saving the func ptrs somewhere
    for (auto it = service_engine::instance().spec().lock_aspects.begin();
        it != service_engine::instance().spec().lock_aspects.end();
        it++)
    {
        last = factory_store<lock_provider>::create(it->c_str(), PROVIDER_TYPE_ASPECT, this, last);
    }

    _provider = last;
}

zlock::~zlock(void)
{
    delete _provider;
}


zrwlock::zrwlock(void)
{
    rwlock_provider* last = factory_store<rwlock_provider>::create(service_engine::instance().spec().rwlock_factory_name.c_str(), PROVIDER_TYPE_MAIN, this, nullptr);

    // TODO: perf opt by saving the func ptrs somewhere
    for (auto it = service_engine::instance().spec().rwlock_aspects.begin();
        it != service_engine::instance().spec().rwlock_aspects.end();
        it++)
    {
        last = factory_store<rwlock_provider>::create(it->c_str(), PROVIDER_TYPE_ASPECT, this, last);
    }

    _provider = last;
}

zrwlock::~zrwlock(void)
{
    delete _provider;
}

zsemaphore::zsemaphore(int initialCount)
{
    semaphore_provider* last = factory_store<semaphore_provider>::create(service_engine::instance().spec().semaphore_factory_name.c_str(), PROVIDER_TYPE_MAIN, this, initialCount, nullptr);

    // TODO: perf opt by saving the func ptrs somewhere
    for (auto it = service_engine::instance().spec().semaphore_aspects.begin();
        it != service_engine::instance().spec().semaphore_aspects.end();
        it++)
    {
        last = factory_store<semaphore_provider>::create(it->c_str(), PROVIDER_TYPE_ASPECT, this, initialCount, last);
    }

    _provider = last;
}

zsemaphore::~zsemaphore()
{
    delete _provider;
}

//------------------------------- event ----------------------------------

zevent::zevent(bool manualReset, bool initState/* = false*/)
{
    _manualReset = manualReset;
    _signaled = initState;
    if (_signaled)
    {
        _sema.signal();
    }
}

zevent::~zevent()
{
}

void zevent::set()
{
    bool nonsignaled = false;
    if (std::atomic_compare_exchange_strong(&_signaled, &nonsignaled, true))
    {
        _sema.signal();
    }
}

void zevent::reset()
{
    if (_manualReset)
    {
        bool signaled = true;
        if (std::atomic_compare_exchange_strong(&_signaled, &signaled, false))
        {
        }
    }
}

bool zevent::wait(int timeout_milliseconds)
{
    lock_checker::check_wait_safety();

    if (_manualReset)
    {
        if (std::atomic_load(&_signaled))
            return true;

        _sema.wait(timeout_milliseconds);
        return std::atomic_load(&_signaled);
    }

    else
    {
        bool signaled = true;
        if (std::atomic_compare_exchange_strong(&_signaled, &signaled, false))
            return true;

        _sema.wait(timeout_milliseconds);
        return std::atomic_compare_exchange_strong(&_signaled, &signaled, false);
    }
}

}} // end namespace rdsn::service
