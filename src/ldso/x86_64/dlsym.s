.text
.global dlsym
.hidden __dlsym
.type dlsym,@function
dlsym:
	mov (%rsp),%rdx
	jmp __dlsym	; 跳转到真正的dlsym c实现: ldso\dynlink.c
