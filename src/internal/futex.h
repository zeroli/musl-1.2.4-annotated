#ifndef _INTERNAL_FUTEX_H
#define _INTERNAL_FUTEX_H

#define FUTEX_WAIT		0
#define FUTEX_WAKE		1
#define FUTEX_FD		2
#define FUTEX_REQUEUE		3
#define FUTEX_CMP_REQUEUE	4
#define FUTEX_WAKE_OP		5
#define FUTEX_LOCK_PI		6
#define FUTEX_UNLOCK_PI		7
#define FUTEX_TRYLOCK_PI	8
#define FUTEX_WAIT_BITSET	9

// 00000000 10000000
// 将第7位设置1，代表private
// 所以客户端API传入shared标志0/1
// pshared = 1 => inter-process sharing
// shared = pshared * 128 => pshared << 7，将第7位设为1
// 然后底层API需要private参数时，shared ^ 128
// 异或操作带来的效果就是第7位求反，其它位不变
// 因为异或0，不变，异或1，求反
#define FUTEX_PRIVATE 128

#define FUTEX_CLOCK_REALTIME 256

#endif
