#ifndef TRACE_SYNC_MOCK_H
#define TRACE_SYNC_MOCK_H

#include <mock_com_util.h>
#include <cstdint>

inline void set_trace_sync_mock_defaults(Mock_com_util &mock_com_util)
{
    using testing::_;
    using testing::Return;

    ON_CALL(mock_com_util, com_util_call_once(_, _))
        .WillByDefault(
            [](com_util_once_flag *flag, com_util_once_fn fn)
            {
                if (flag->state == 0)
                {
                    flag->state = 1;
                    fn();
                    flag->state = 2;
                }
            });
    ON_CALL(mock_com_util, com_util_local_lock_create(_))
        .WillByDefault(
            [](com_util_local_lock **lock)
            {
                *lock = reinterpret_cast<com_util_local_lock *>(static_cast<uintptr_t>(0x3100));
                return COM_UTIL_OK;
            });
    ON_CALL(mock_com_util, com_util_local_lock_lock(_, _)).WillByDefault(Return(COM_UTIL_OK));
    ON_CALL(mock_com_util, com_util_local_lock_unlock(_)).WillByDefault(Return(COM_UTIL_OK));
    ON_CALL(mock_com_util, com_util_local_lock_destroy(_)).WillByDefault(Return());
    ON_CALL(mock_com_util, com_util_local_rwlock_create(_))
        .WillByDefault(
            [](com_util_local_rwlock **lock)
            {
                *lock = reinterpret_cast<com_util_local_rwlock *>(static_cast<uintptr_t>(0x3200));
                return COM_UTIL_OK;
            });
    ON_CALL(mock_com_util, com_util_local_rwlock_lock_shared(_, _)).WillByDefault(Return(COM_UTIL_OK));
    ON_CALL(mock_com_util, com_util_local_rwlock_lock_exclusive(_, _)).WillByDefault(Return(COM_UTIL_OK));
    ON_CALL(mock_com_util, com_util_local_rwlock_unlock_shared(_)).WillByDefault(Return(COM_UTIL_OK));
    ON_CALL(mock_com_util, com_util_local_rwlock_unlock_exclusive(_)).WillByDefault(Return(COM_UTIL_OK));
    ON_CALL(mock_com_util, com_util_local_rwlock_destroy(_)).WillByDefault(Return());
}

#endif /* TRACE_SYNC_MOCK_H */
