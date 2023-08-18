#include "pthread_impl.h"
#include <threads.h>

static void *__pthread_getspecific(pthread_key_t k)
{
	struct pthread *self = __pthread_self();
	return self->tsd[k];  // thread storage就是简单的一个数组，key就是数组索引
}

weak_alias(__pthread_getspecific, pthread_getspecific);
weak_alias(__pthread_getspecific, tss_get);
