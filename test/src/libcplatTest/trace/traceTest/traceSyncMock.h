#ifndef TRACE_SYNC_MOCK_H
#define TRACE_SYNC_MOCK_H

#include <mock_cplat.h>
#include <cstdint>

inline void set_trace_sync_mock_defaults(Mock_cplat &mock_cplat)
{
    using testing::_;
    using testing::Return;

    ON_CALL(mock_cplat, cplat_call_once(_, _))
        .WillByDefault(
            [](cplat_once_flag *flag, cplat_once_fn fn)
            {
                if (flag->state == 0)
                {
                    flag->state = 1;
                    fn();
                    flag->state = 2;
                }
            });
    ON_CALL(mock_cplat, cplat_local_lock_create(_))
        .WillByDefault(
            [](cplat_local_lock **lock)
            {
                *lock = reinterpret_cast<cplat_local_lock *>(static_cast<uintptr_t>(0x3100));
                return CPLAT_OK;
            });
    ON_CALL(mock_cplat, cplat_local_lock_lock(_, _)).WillByDefault(Return(CPLAT_OK));
    ON_CALL(mock_cplat, cplat_local_lock_unlock(_)).WillByDefault(Return(CPLAT_OK));
    ON_CALL(mock_cplat, cplat_local_lock_dispose(_)).WillByDefault(Return());
    ON_CALL(mock_cplat, cplat_local_rwlock_create(_))
        .WillByDefault(
            [](cplat_local_rwlock **lock)
            {
                *lock = reinterpret_cast<cplat_local_rwlock *>(static_cast<uintptr_t>(0x3200));
                return CPLAT_OK;
            });
    ON_CALL(mock_cplat, cplat_local_rwlock_lock_shared(_, _)).WillByDefault(Return(CPLAT_OK));
    ON_CALL(mock_cplat, cplat_local_rwlock_lock_exclusive(_, _)).WillByDefault(Return(CPLAT_OK));
    ON_CALL(mock_cplat, cplat_local_rwlock_unlock_shared(_)).WillByDefault(Return(CPLAT_OK));
    ON_CALL(mock_cplat, cplat_local_rwlock_unlock_exclusive(_)).WillByDefault(Return(CPLAT_OK));
    ON_CALL(mock_cplat, cplat_local_rwlock_dispose(_)).WillByDefault(Return());
}

#endif /* TRACE_SYNC_MOCK_H */
