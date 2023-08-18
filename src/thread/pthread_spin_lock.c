#include "pthread_impl.h"
#include <errno.h>

int pthread_spin_lock(pthread_spinlock_t *s)
{
	// 这个实现相当简洁
	//  spin_lock其实就是int类型，非零或者无法修改位EBUSY，则spin
	while (*(volatile int *)s || a_cas(s, 0, EBUSY)) a_spin();
	return 0;
}
