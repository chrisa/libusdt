/*
 * Stub functions containing DTrace tracepoints for probes and
 * is-enabled probes. These functions are copied for each probe
 * dynamically created.
 *
 */   
        .text

        .align  4, 0x90
        .globl _usdt_tracepoint_isenabled
_usdt_tracepoint_isenabled:
        pushl   %ebp
        movl    %esp, %ebp
        subl    $8, %esp
        xorl    %eax, %eax
        nop
        nop
        leave
        ret

        .align  4, 0x90
        .globl _usdt_tracepoint_probe
_usdt_tracepoint_probe:
        pushl   %ebp
        movl    %esp, %ebp
        subl    $8, %esp
        nop
        nop
        nop
        leave
        ret
