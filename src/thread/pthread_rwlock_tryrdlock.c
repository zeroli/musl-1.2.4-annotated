#include "pthread_impl.h"

int __pthread_rwlock_tryrdlock(pthread_rwlock_t *rw)
{
	int val, cnt;
	// 获取锁的个数，原子的递增1
	do {
		val = rw->_rw_lock;
		cnt = val & 0x7fffffff;
		if (cnt == 0x7fffffff) return EBUSY;  // 特别值，代表有人在写
		if (cnt == 0x7ffffffe) return EAGAIN;  // 达到读锁的最大上限，不能上锁了
	} while (a_cas(&rw->_rw_lock, val, val+1) != val);
	// 否则尝试CAS修改，成功？返回0，失败？有其它人先于我们加锁了，继续尝试
	return 0;
}

weak_alias(__pthread_rwlock_tryrdlock, pthread_rwlock_tryrdlock);
