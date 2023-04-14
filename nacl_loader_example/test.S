.section .text
.global f
f:
push   %rbp
mov    %rsp,%rbp
mov    $0x2a,%eax
pop    %r11
mov    %r11d,%ebp
add    %r15,%rbp
pop    %r11
and    $0xffffffe0,%r11d
add    %r15,%r11
jmpq   *%r11