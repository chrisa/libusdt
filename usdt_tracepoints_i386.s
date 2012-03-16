/*
 * Stub functions containing DTrace tracepoints for probes and
 * is-enabled probes. These functions are copied for each probe
 * dynamically created.
 *
 */   
        .text

        .align  4, 0x90
        .globl usdt_tracepoint_isenabled
        .globl _usdt_tracepoint_isenabled
usdt_tracepoint_isenabled:
_usdt_tracepoint_isenabled:
        pushl   %ebp
        movl    %esp, %ebp
        subl    $8, %esp
        xorl    %eax, %eax
        nop
        nop
        leave
        ret
probe_tracepoint:
_probe_tracepoint:
        nop
        nop
        nop
        nop
        nop
        addl    $0x20,%esp
        leave
        ret

        .align  4, 0x90
        .globl usdt_tracepoint_probe
        .globl _usdt_tracepoint_probe
        .globl probe_tracepoint
        .globl _probe_tracepoint
usdt_tracepoint_probe:
_usdt_tracepoint_probe:
        pushl   %ebp
        movl    %esp,%ebp
        subl    $8,%esp
        subl    $8,%esp
        movl    8(%ebp),%ebx    // addr -> %ebx
        movl    0xc(%ebp),%ecx  // argc -> %ecx
        test    %ecx,%ecx
        jne     args
        jmp     *%ebx
args:   movl    %ecx,%eax
        sal     $2,%eax
        subl    $4,%eax
        addl    0x10(%ebp),%eax
        pushl   (%eax)
        dec     %ecx
        jne     args
        jmp     *%ebx

