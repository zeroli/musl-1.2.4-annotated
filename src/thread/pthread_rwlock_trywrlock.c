#include "pthread_impl.h"

int __pthread_rwlock_trywrlock(pthread_rwlock_t *rw)
{
	// 有人已经锁住了吗？不管是读还是写
	// 如果是，修改失败，返回BUSY
	// 如果没有，则修改为特别值，代表写锁住
	if (a_cas(&rw->_rw_lock, 0, 0x7fffffff)) return EBUSY;
	return 0;
}

weak_alias(__pthread_rwlock_trywrlock, pthread_rwlock_trywrlock);
