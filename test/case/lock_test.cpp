#include "frame/test_macros.h"

#include "config/compiler_features.h"

#include "lock/lock_holder.h"
#include "lock/spin_lock.h"
#include "lock/spin_rw_lock.h"

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

    util::lock::lock_holder<util::lock::spin_lock, util::lock::detail::default_try_lock_action<util::lock::spin_lock> >
        holder2(lock);

    CASE_EXPECT_FALSE(holder2.is_available());
  }

  CASE_EXPECT_FALSE(lock.is_locked());
}

CASE_TEST(lock_test, spin_rw_lock) {
  util::lock::spin_rw_lock lock;

  CASE_EXPECT_FALSE(lock.is_read_locked());
  CASE_EXPECT_FALSE(lock.is_write_locked());

  CASE_EXPECT_TRUE(lock.try_read_lock());
  CASE_EXPECT_TRUE(lock.is_read_locked());

  CASE_EXPECT_TRUE(lock.try_read_lock());
  CASE_EXPECT_TRUE(lock.try_read_lock());

  CASE_EXPECT_TRUE(lock.is_read_locked());
  CASE_EXPECT_FALSE(lock.is_write_locked());

  CASE_EXPECT_FALSE(lock.try_write_lock());
  CASE_EXPECT_TRUE(lock.try_read_unlock());
  CASE_EXPECT_TRUE(lock.is_read_locked());
  CASE_EXPECT_FALSE(lock.is_write_locked());

  CASE_EXPECT_FALSE(lock.try_write_lock());
  CASE_EXPECT_TRUE(lock.try_read_unlock());
  CASE_EXPECT_TRUE(lock.is_read_locked());
  CASE_EXPECT_FALSE(lock.is_write_locked());

  CASE_EXPECT_FALSE(lock.try_write_lock());
  CASE_EXPECT_TRUE(lock.try_read_unlock());
  CASE_EXPECT_FALSE(lock.is_read_locked());
  CASE_EXPECT_FALSE(lock.is_write_locked());

  CASE_EXPECT_TRUE(lock.try_write_lock());
  CASE_EXPECT_FALSE(lock.try_write_lock());
  CASE_EXPECT_TRUE(lock.is_write_locked());

  CASE_EXPECT_FALSE(lock.try_read_lock());
  CASE_EXPECT_FALSE(lock.try_read_lock());
  CASE_EXPECT_FALSE(lock.try_read_lock());
  CASE_EXPECT_FALSE(lock.is_read_locked());
  CASE_EXPECT_TRUE(lock.is_write_locked());

  CASE_EXPECT_TRUE(lock.try_write_unlock());
  CASE_EXPECT_FALSE(lock.is_write_locked());
  CASE_EXPECT_FALSE(lock.try_write_unlock());
  CASE_EXPECT_FALSE(lock.is_write_locked());
  CASE_EXPECT_FALSE(lock.is_read_locked());

  CASE_EXPECT_TRUE(lock.try_read_lock());
  CASE_EXPECT_TRUE(lock.try_read_lock());
  CASE_EXPECT_TRUE(lock.try_read_lock());

  CASE_EXPECT_TRUE(lock.is_read_locked());
}

CASE_TEST(lock_test, spin_rw_lock_holder) {
  util::lock::spin_rw_lock lock;

  {
    util::lock::read_lock_holder<util::lock::spin_rw_lock> holder(lock);
    CASE_EXPECT_TRUE(holder.is_available());

    CASE_EXPECT_TRUE(lock.is_read_locked());
    CASE_EXPECT_FALSE(lock.is_write_locked());
  }

  CASE_EXPECT_FALSE(lock.is_read_locked());
  CASE_EXPECT_FALSE(lock.is_write_locked());

  {
    util::lock::write_lock_holder<util::lock::spin_rw_lock> holder(lock);
    CASE_EXPECT_TRUE(holder.is_available());

    CASE_EXPECT_FALSE(lock.is_read_locked());
    CASE_EXPECT_TRUE(lock.is_write_locked());
  }

  CASE_EXPECT_FALSE(lock.is_read_locked());
  CASE_EXPECT_FALSE(lock.is_write_locked());
}

#if defined(UTIL_CONFIG_COMPILER_CXX_THREAD_LOCAL) && defined(UTIL_CONFIG_COMPILER_CXX_LAMBDAS) && \
    UTIL_CONFIG_COMPILER_CXX_LAMBDAS

#  include <std/smart_ptr.h>
#  include <thread>

CASE_TEST(lock_test, spin_rw_lock_mt) {
  util::lock::spin_rw_lock lock;
  ::util::lock::atomic_int_type<size_t> write_lock_count;
  write_lock_count.store(0);

  CASE_EXPECT_TRUE(lock.try_read_lock());
  CASE_EXPECT_TRUE(lock.try_read_lock());
  CASE_EXPECT_TRUE(lock.try_read_lock());
  lock.read_lock();
  CASE_MSG_INFO() << "Wait another thread to write_lock" << std::endl;

  std::thread *write_lock_thd[32];
  for (int i = 0; i < 32; ++i) {
    write_lock_thd[i] = new std::thread([&lock, &write_lock_count]() {
      CASE_MSG_INFO() << "Thread: " << std::this_thread::get_id() << " before write_lock" << std::endl;
      lock.write_lock();
      ++write_lock_count;
      CASE_MSG_INFO() << "Thread: " << std::this_thread::get_id() << " after write_lock" << std::endl;

      __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD();
      __UTIL_LOCK_SPIN_LOCK_THREAD_SLEEP();
      CASE_EXPECT_EQ(1, write_lock_count.load());

      CASE_MSG_INFO() << "Thread: " << std::this_thread::get_id() << " before write_unlock" << std::endl;
      --write_lock_count;
      lock.write_unlock();
      CASE_MSG_INFO() << "Thread: " << std::this_thread::get_id() << " after write_unlock" << std::endl;
    });
  }

  while (!lock.is_write_locked()) {
    __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD();
  }

  CASE_EXPECT_FALSE(lock.try_read_lock());
  CASE_MSG_INFO() << "Start to unlock read lock" << std::endl;
  lock.read_unlock();
  lock.read_unlock();
  lock.read_unlock();
  lock.read_unlock();

  for (int i = 0; i < 32; ++i) {
    if (write_lock_thd[i]->joinable()) {
      write_lock_thd[i]->join();
    }

    delete write_lock_thd[i];
  }

  CASE_EXPECT_FALSE(lock.is_write_locked());
  CASE_EXPECT_FALSE(lock.is_read_locked());
}

#endif