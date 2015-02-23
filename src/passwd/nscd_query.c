#include <sys/socket.h>
#include <byteswap.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "nscd.h"

static const struct {
	short sun_family;
	char sun_path[21];
} addr = {
	AF_UNIX,
	"/var/run/nscd/socket"
};

FILE *__nscd_query(int32_t req, const char *key, int32_t *buf, size_t len, int *swap)
{
	size_t i;
	int fd;
	FILE *f = 0;
	int32_t req_buf[REQ_LEN] = {
		NSCDVERSION,
		req,
		strlen(key)+1
	};
	struct msghdr msg = {
		.msg_iov = (struct iovec[]){
			{&req_buf, sizeof(req_buf)},
			{(char*)key, strlen(key)+1}
		},
		.msg_iovlen = 2
	};

	if (strlen(key) > INT32_MAX - 1) {
		return (FILE*)-1;
	}

	*swap = 0;
retry:

	fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0) return NULL;

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		/* If there isn't a running nscd we return -1 to indicate that
		 * that is precisely what happened
		 */
		if (errno == EACCES || errno == ECONNREFUSED || errno == ENOENT) {
			close(fd);
			return (FILE *)-1;
		}
		goto error;
	}

	if (sendmsg(fd, &msg, MSG_NOSIGNAL) < 0)
		goto error;

	if(!(f = fdopen(fd, "r"))) goto error;

	if (!fread(buf, len, 1, f)) {
		/* If the VERSION entry mismatches nscd will disconnect. The
		 * most likely cause is that the endianness mismatched. So, we
		 * byteswap and try once more. (if we already swapped, just
		 * fail out)
		 */
		if (ferror(f)) goto error;
		if (!*swap) {
			fclose(f);
			for (i = 0; i < sizeof(req_buf)/sizeof(req_buf[0]); i++) {
				req_buf[i] = bswap_32(req_buf[i]);
			}
			*swap = 1;
			goto retry;
		} else {
			errno = EIO;
			goto error;
		}
	}

	if (*swap) {
		for (i = 0; i < len/sizeof(buf[0]); i++) {
			buf[i] = bswap_32(buf[i]);
		}
	}

	/* The first entry in every nscd response is the version number. This
	 * really shouldn't happen, and is evidence of some form of malformed
	 * response.
	 */
	if(buf[0] != NSCDVERSION) {
		errno = EIO;
		goto error;
	}

	return f;
error:
	if (f) fclose(f); else close(fd);
	return 0;
}
