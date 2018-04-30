#include <chrono>
#include <vector>

#include <cassert>

using Clock = std::chrono::system_clock;

namespace timesync
{
uint64_t sqrtsd_loop_(double* buffer, uint64_t elems, uint64_t repeat)
{
    unsigned long long passes, length, addr;
    unsigned long long a, b, c, d;
    uint64_t ret = 0;
    assert(elems >= 256 / sizeof(*buffer));

    passes = elems / 64; // 32 128-Bit accesses in inner loop
    length = passes * 32 * repeat;
    addr = (unsigned long long)buffer;

    if (!passes)
        return ret;
    /*
     * Input:  RAX: addr (pointer to the buffer)
     *         RBX: passes (number of iterations)
     *         RCX: length (total number of accesses)
     */
    __asm__ __volatile__("mfence;"
                         "mov %%rax,%%r9;"  // addr
                         "mov %%rbx,%%r10;" // passes
                         "mov %%rcx,%%r15;" // length
                         "mov %%r9,%%r14;"  // store addr
                         "mov %%r10,%%r8;"  // store passes
                         "mov %%r15,%%r13;" // store length

                         // initialize registers
                         "movapd 0(%%r9), %%xmm0;"
                         "movapd 0(%%r9), %%xmm8;"
                         "movapd 16(%%r9), %%xmm9;"
                         "movapd 32(%%r9), %%xmm10;"
                         "movapd 48(%%r9), %%xmm11;"
                         "movapd 64(%%r9), %%xmm12;"
                         "movapd 80(%%r9), %%xmm13;"
                         "movapd 96(%%r9), %%xmm14;"
                         "movapd 112(%%r9), %%xmm15;"

                         ".align 64;"
                         "_work_loop_sqrt_sd:"
                         "sqrtsd %%xmm8, %%xmm0;"
                         "sqrtsd %%xmm9, %%xmm0;"
                         "sqrtsd %%xmm10, %%xmm0;"
                         "sqrtsd %%xmm11, %%xmm0;"
                         "sqrtsd %%xmm12, %%xmm0;"
                         "sqrtsd %%xmm13, %%xmm0;"
                         "sqrtsd %%xmm14, %%xmm0;"
                         "sqrtsd %%xmm15, %%xmm0;"
                         "sqrtsd %%xmm8, %%xmm0;"
                         "sqrtsd %%xmm9, %%xmm0;"
                         "sqrtsd %%xmm10, %%xmm0;"
                         "sqrtsd %%xmm11, %%xmm0;"
                         "sqrtsd %%xmm12, %%xmm0;"
                         "sqrtsd %%xmm13, %%xmm0;"
                         "sqrtsd %%xmm14, %%xmm0;"
                         "sqrtsd %%xmm15, %%xmm0;"
                         "sqrtsd %%xmm8, %%xmm0;"
                         "sqrtsd %%xmm9, %%xmm0;"
                         "sqrtsd %%xmm10, %%xmm0;"
                         "sqrtsd %%xmm11, %%xmm0;"
                         "sqrtsd %%xmm12, %%xmm0;"
                         "sqrtsd %%xmm13, %%xmm0;"
                         "sqrtsd %%xmm14, %%xmm0;"
                         "sqrtsd %%xmm15, %%xmm0;"
                         "sqrtsd %%xmm8, %%xmm0;"
                         "sqrtsd %%xmm9, %%xmm0;"
                         "sqrtsd %%xmm10, %%xmm0;"
                         "sqrtsd %%xmm11, %%xmm0;"
                         "sqrtsd %%xmm12, %%xmm0;"
                         "sqrtsd %%xmm13, %%xmm0;"
                         "sqrtsd %%xmm14, %%xmm0;"
                         "sqrtsd %%xmm15, %%xmm0;"
                         "add $512,%%r9;"
                         "sub $1,%%r10;"
                         "jnz _skip_reset_sqrt_sd;" // reset buffer if the end is reached
                         "mov %%r14,%%r9;"          // restore addr
                         "mov %%r8,%%r10;"          // restore passes
                         "_skip_reset_sqrt_sd:"
                         "sub $32,%%r15;"
                         "jnz _work_loop_sqrt_sd;"

                         "mov %%r13,%%rcx;" // restore length
                         : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                         : "a"(addr), "b"(passes), "c"(length)
                         : "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "xmm0",
                           "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "xmm8", "xmm9",
                           "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15"

    );
    ret = c;

    return ret;
}
} // namespace timesync::footprint
