__asm__(
".text \n"
".global " START " \n"
START ": \n"
"	xor %rbp,%rbp \n"
"	mov %rsp,%rdi \n" // rdi保存第一个参数，保存着传入参数的个数，整型
".weak _DYNAMIC \n"
".hidden _DYNAMIC \n"
"	lea _DYNAMIC(%rip),%rsi \n" // rsi保存第二个参数，_DYNAMIC数组的地址，argv数组
"	andq $-16,%rsp \n"  // align down到16字节边界
"	call " START "_c \n"  // 调用START_c函数
);

// 这个头文件被包含在了crt1.c文件中
// 那里宏定义START = _start
// 所以call _start_c，就是程序的启动函数
// _DYNAMIC定义在哪里了？
// _start_c需要唯一的long*参数p，p[0]就是argc，从p[1]开始就是argv字符串数组
// 字符串数组就是指针的数组，数组最后一元素为nullptr
// linux kernel在执行exec函数时，会将argc/argv/env放在程序stack的顶层，也就是rsp指向的地方
