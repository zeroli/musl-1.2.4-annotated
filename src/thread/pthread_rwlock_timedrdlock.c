#include "pthread_impl.h"

int __pthread_rwlock_timedrdlock(pthread_rwlock_t *restrict rw, const struct timespec *restrict at)
{
	int r, t;

	// 先尝试锁住读锁？成功，就OK
	r = pthread_rwlock_tryrdlock(rw);
	// 这里也可能返回EAGAIN，代表达到读锁上限了
	// 也直接返回了，不过客户端需要判断返回值，之后继续尝试
	if (r != EBUSY) return r;

	// 在用户空间spin一段时间
	// 注意这里用了volatle类型的变量_rw_lock，每次都要从内存读取
	// 有waiters，代表已经有读线程在等待了，则没必要spin了，直接也进入等待
	int spins = 100;
	while (spins-- && rw->_rw_lock && !rw->_rw_waiters) a_spin();

	// 读锁的过程，主要是判断是否有人锁住了写锁
	// trylock返回EBUSY，代表有人锁住了写锁
	// 没有人锁写锁的话，可以直接锁住共享的读锁
	while ((r=__pthread_rwlock_tryrdlock(rw))==EBUSY) {
		// lock变为0，或者又读线程锁住读锁了，那么可以尝试直接trylock
		// 注意这里判断是无所状态，或者是非原子读写变量
		if (!(r=rw->_rw_lock) || (r&0x7fffffff)!=0x7fffffff) continue;
		t = r | 0x80000000;  // TODO：设置这个标志位代表啥意思？？？？？
		// 读线程waiters递增
		a_inc(&rw->_rw_waiters);
		a_cas(&rw->_rw_lock, r, t);  // 尝试原子的修改rw_lock，这里有可能修改失败，r != rw_lock
		// 如果rw_lock变量在等待期间 != t了， 则代表有人修改了这个值，没有timeout，很有可能返回0
		// 那么可以再次trylock
		// shared ^ 128，就是第7位求反变为private参数
		r = __timedwait(&rw->_rw_lock, t, CLOCK_REALTIME, at, rw->_rw_shared^128);
		a_dec(&rw->_rw_waiters);
		// 如果r非零而且不是信号中断了，直接返回这个值给客户端，lock失败，发生错误
		// 否则r=0，则等待时间已过，则继续trylock下，看能否锁住
		if (r && r != EINTR) return r;
	}
	return r;
}

weak_alias(__pthread_rwlock_timedrdlock, pthread_rwlock_timedrdlock);
