#include <typeinfo>
#include "frame/test_macros.h"

#include "Lock/SpinLock.h"
#include "Lock/LockHolder.h"

CASE_TEST(LockTest, SpinLock)
{
    util::lock::SpinLock lock;
    CASE_EXPECT_FALSE(lock.IsLocked());
    
    lock.Lock();
    CASE_EXPECT_TRUE(lock.IsLocked());
    
    CASE_EXPECT_FALSE(lock.TryLock());
    
    lock.Unlock();
    CASE_EXPECT_FALSE(lock.IsLocked());
    
    CASE_EXPECT_TRUE(lock.TryLock());
    CASE_EXPECT_TRUE(lock.TryUnlock());
}

CASE_TEST(LockTest, LockHolder)
{
    util::lock::SpinLock lock;
    CASE_EXPECT_FALSE(lock.IsLocked());
    
    {
        util::lock::LockHolder<util::lock::SpinLock> holder1(lock);
        
        CASE_EXPECT_TRUE(lock.IsLocked());
        CASE_EXPECT_TRUE(holder1.IsAvailable());
        
        util::lock::LockHolder<
            util::lock::SpinLock,
            util::lock::detail::DefaultTryLockAction<util::lock::SpinLock>
        > holder2(lock);
        
        CASE_EXPECT_FALSE(holder2.IsAvailable());
    }
    
    CASE_EXPECT_FALSE(lock.IsLocked());
}
