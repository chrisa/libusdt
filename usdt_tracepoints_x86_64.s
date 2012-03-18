/*
 * Stub functions containing DTrace tracepoints for probes and
 * is-enabled probes. These functions are copied for each probe
 * dynamically created.
 *
 */   
        .text

        .align 4, 0x90
        .globl usdt_tracepoint_isenabled
        .globl _usdt_tracepoint_isenabled
        .globl usdt_tracepoint_probe
        .globl _usdt_tracepoint_probe
        .globl usdt_probe_args
        .globl _usdt_probe_args

usdt_tracepoint_isenabled:
_usdt_tracepoint_isenabled:
        pushq   %rbp
        movq    %rsp, %rbp
        xorl    %eax, %eax
        nop
        nop
        leave
        ret
usdt_tracepoint_probe:
_usdt_tracepoint_probe:
        nop
        nop
        nop
        nop
        nop
        leave
        ret

/*
 * Probe argument marshalling, x86_64 style
 *
 */

usdt_probe_args:
_usdt_probe_args:
        pushq   %rbp
        movq    %rsp,%rbp
        movq    %rdi,%r12        // addr  -> %r12
        movl    %esi,%ebx        // argc  -> %ebx
        movq    %rdx,%r11        // nargv -> %r11
        test    %ebx,%ebx
        je      fire
        movq    (%r11),%rdi
        dec     %ebx
        test    %ebx,%ebx
        je      fire
        movq    8(%r11),%rsi
        dec     %ebx
        test    %ebx,%ebx
        je      fire
        movq    16(%r11),%rdx
        dec     %ebx
        test    %ebx,%ebx
        je      fire
        movq    24(%r11),%rcx
        dec     %ebx
        test    %ebx,%ebx
        je      fire
        movq    32(%r11),%r8
        dec     %ebx
        test    %ebx,%ebx
        je      fire
        movq    40(%r11),%r9
fire:   jmp     *%r12
