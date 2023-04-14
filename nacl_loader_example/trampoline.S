.text
.global trampoline

trampoline:
pushq   %rbp
movq    %rsp,%rbp
.rept 24
    nop
.endr
callq   *16(%rsp)
popq    %rbp
ret
