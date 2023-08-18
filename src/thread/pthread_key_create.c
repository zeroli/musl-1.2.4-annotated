#include "pthread_impl.h"
#include "fork_impl.h"

// 系统全局最多1M的线程局部存储空间
volatile size_t __pthread_tsd_size = sizeof(void *) * PTHREAD_KEYS_MAX;
// 每个线程结构中的一个成员变量指向这个全局的
void *__pthread_tsd_main[PTHREAD_KEYS_MAX] = { 0 };
// 这里定义一个函数数组，每个元素就是函数指针：void (void*)
// 这也是系统全局的，所有线程共享的
// 保存tls数据的销毁函数
static void (*keys[PTHREAD_KEYS_MAX])(void *);

static pthread_rwlock_t key_lock = PTHREAD_RWLOCK_INITIALIZER;

static pthread_key_t next_key;

static void nodtor(void *dummy)
{
}

static void dummy_0(void)
{
}

weak_alias(dummy_0, __tl_lock);
weak_alias(dummy_0, __tl_unlock);

void __pthread_key_atfork(int who)
{
	if (who<0) __pthread_rwlock_rdlock(&key_lock);
	else if (!who) __pthread_rwlock_unlock(&key_lock);
	else key_lock = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;
}

int __pthread_key_create(pthread_key_t *k, void (*dtor)(void *))
{
	pthread_t self = __pthread_self();

	/* This can only happen in the main thread before
	 * pthread_create has been called. */
	// main thread tsd没有赋值？
	// pthread_create会帮忙设置这个
	if (!self->tsd) self->tsd = __pthread_tsd_main;

	/* Purely a sentinel value since null means slot is free. */
	if (!dtor) dtor = nodtor;

	__pthread_rwlock_wrlock(&key_lock);
	pthread_key_t j = next_key;
	do {
		if (!keys[j]) {
			keys[next_key = *k = j] = dtor;
			__pthread_rwlock_unlock(&key_lock);
			return 0;
		}
		// 循环数组，从头开始继续查找free slot
		// 循环一遍之后，还是没找到空闲的，错误退出
	} while ((j=(j+1)%PTHREAD_KEYS_MAX) != next_key);

	__pthread_rwlock_unlock(&key_lock);
	return EAGAIN;
}

int __pthread_key_delete(pthread_key_t k)
{
	sigset_t set;
	pthread_t self = __pthread_self(), td=self;

	__block_app_sigs(&set);
	__pthread_rwlock_wrlock(&key_lock);

	__tl_lock();
	// 遍历所有的线程？？没必要吧
	// 每个线程tsd要么指向全局的__pthread_tsd_main（main thread）
	// 要么指向自己的一片mmap出来的内存空间
	do td->tsd[k] = 0;
	while ((td=td->next)!=self);
	__tl_unlock();

 	// 重置销毁函数为空，代表free了
	// 重置之前，不调用销毁函数销毁数据么？
	// 线程退出时会自动调用那个线程所有的tls key对应的销毁函数
	// __pthread_tsd_run_dtors
	keys[k] = 0;

	__pthread_rwlock_unlock(&key_lock);
	__restore_sigs(&set);

	return 0;
}

// 这个销毁tls数据的函数会在thread exit的时候自动调用
// 查看__pthread_exit
void __pthread_tsd_run_dtors()
{
	pthread_t self = __pthread_self();
	int i, j;
	// 在线程退出的时候，有人又添加了tls key？导致tsd_used != 0，所以要loop一下？
	for (j=0; self->tsd_used && j<PTHREAD_DESTRUCTOR_ITERATIONS; j++) {
		__pthread_rwlock_rdlock(&key_lock);
		self->tsd_used = 0;
		for (i=0; i<PTHREAD_KEYS_MAX; i++) {
			void *val = self->tsd[i];
			void (*dtor)(void *) = keys[i];
			self->tsd[i] = 0;
			if (val && dtor && dtor != nodtor) {
				__pthread_rwlock_unlock(&key_lock);  // UNLOCK了，在调用用户代码之前
				dtor(val);   // 调用用户的销毁函数
				__pthread_rwlock_rdlock(&key_lock);
			}
		}
		__pthread_rwlock_unlock(&key_lock);
	}
}

weak_alias(__pthread_key_create, pthread_key_create);
weak_alias(__pthread_key_delete, pthread_key_delete);
