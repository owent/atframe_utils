#include <typeinfo>
#include "frame/test_macros.h"

#include "lock/spin_lock.h"
#include "lock/lock_holder.h"

CASE_TEST(lock_test, spin_lock) {
    util::lock::spin_lock lock;
    CASE_EXPECT_FALSE(lock.is_locked());

    lock.lock();
    CASE_EXPECT_TRUE(lock.is_locked());

    CASE_EXPECT_FALSE(lock.try_lock());

    lock.unlock();
    CASE_EXPECT_FALSE(lock.is_locked());

    CASE_EXPECT_TRUE(lock.try_lock());
    CASE_EXPECT_TRUE(lock.try_unlock());
}

CASE_TEST(lock_test, lock_holder) {
    util::lock::spin_lock lock;
    CASE_EXPECT_FALSE(lock.is_locked());

    {
        util::lock::lock_holder<util::lock::spin_lock> holder1(lock);

        CASE_EXPECT_TRUE(lock.is_locked());
        CASE_EXPECT_TRUE(holder1.is_available());

        util::lock::lock_holder<util::lock::spin_lock,
                                util::lock::detail::default_try_lock_action<util::lock::spin_lock> > holder2(lock);

        CASE_EXPECT_FALSE(holder2.is_available());
    }

    CASE_EXPECT_FALSE(lock.is_locked());
}
