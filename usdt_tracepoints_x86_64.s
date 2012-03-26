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
        xorq    %rax, %rax
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
        popq %r11
        popq %rbx
        popq %r12
        addq $0x18,%rsp
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
        subq    $0x18,%rsp
        pushq   %r12
        pushq   %rbx
        pushq   %r11
        movq    %rdi,%r12
        movq    %rsi,%rbx
        movq    %rdx,%r11
        test    %rbx,%rbx
        je      fire
        movq    (%r11),%rdi
        dec     %rbx
        test    %rbx,%rbx
        je      fire
        movq    8(%r11),%rsi
        dec     %rbx
        test    %rbx,%rbx
        je      fire
        movq    16(%r11),%rdx
        dec     %rbx
        test    %rbx,%rbx
        je      fire
        movq    24(%r11),%rcx
        dec     %rbx
        test    %rbx,%rbx
        je      fire
        movq    32(%r11),%r8
        dec     %rbx
        test    %rbx,%rbx
        je      fire
        movq    40(%r11),%r9
fire:   jmp     *%r12
