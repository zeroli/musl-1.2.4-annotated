#include "pthread_impl.h"

int pthread_attr_init(pthread_attr_t *a)
{
	*a = (pthread_attr_t){0};
	// 这里为啥要获取读写锁呢？难道是其它线程有可能更改
	// 下面的2个全局变量？是的！
	// API: pthread_setattr_default_np，就可以做到这个
	__acquire_ptc();
	a->_a_stacksize = __default_stacksize;
	a->_a_guardsize = __default_guardsize;
	__release_ptc();
	return 0;
}
