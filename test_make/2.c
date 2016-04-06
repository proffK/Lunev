#include "1.h"

int hello_world()
{
        volatile char* msg = "Hello, World!";
        long msi = (long) msg;

        asm ("movq 1, %%rax\n\t"
            "movq 13, %%rdx\n\t"
            "movq %0 , %%rsi\n\t"
            "movq 1 , %%rdi\n\t"
            "syscall" :: "r" (msi) : "rax", "rdx", "rsi", "rdi");

        return 0;
}
