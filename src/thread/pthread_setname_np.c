#define _GNU_SOURCE
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "pthread_impl.h"

int pthread_setname_np(pthread_t thread, const char *name)
{
	int fd, cs, status = 0;
	// 这种方式求解字符串长度，在编译期第一次看到，有点意思
	char f[sizeof "/proc/self/task//comm" + 3*sizeof(int)];
	size_t len;

	// strnlen，第二个参数代表最大的长度
	if ((len = strnlen(name, 16)) > 15) return ERANGE;

	if (thread == pthread_self())
		return prctl(PR_SET_NAME, (unsigned long)name, 0UL, 0UL, 0UL) ? errno : 0;

	// 这个虚拟文件对应的是线程信息
	snprintf(f, sizeof f, "/proc/self/task/%d/comm", thread->tid);
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cs);
	if ((fd = open(f, O_WRONLY|O_CLOEXEC)) < 0 || write(fd, name, len) < 0) status = errno;
	if (fd >= 0) close(fd);
	pthread_setcancelstate(cs, 0);
	return status;
}
