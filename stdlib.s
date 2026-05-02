default rel

global out

;TODO: document asm functions more clearly
;TODO: check if you can add more constant macros
;TODO: asm codestyle check
;TODO: move buffer to be local to every call of the function
;TODO: ensure calling conventions are met

; Return value: rax
; First 6 args: rdi, rsi, rdx, rcx, r8, r9
; CALLEE-saved: r12, r13, r14, r15, rbx, rsp, rbp
; Rest are CALLER-saved

BUF_SIZE equ 32
REG_SIZE equ 8

section .bss

buf: resb BUF_SIZE

section .text

SIGN_BIT_MASK equ 0x80000000
NEWLINE equ 0xA

;--------------
; out - prints signed 64-bit number to stdout, with a newline at the end
; Input:  rdi = number to print
; Destr:  TODO: fill this out 
;--------------
out:
  mov rax, rdi
  lea rdi, [buf]
  test rax, SIGN_BIT_MASK
  jz .decimal_common
  push rax
  mov al, '-'
  stosb
  pop rax
  not rax
  inc rax
.decimal_common:
  mov rbx, 10
  lea r10, [alpha]
  call num2str
  mov al, NEWLINE
  stosb
  call buf_flush
  ret

STDOUT_FD equ 1
WRITE_SYSCALL equ 0x1

buf_flush:
  mov rdx, rdi
  lea rsi, [buf]
  sub rdx, rsi
  mov rax, WRITE_SYSCALL
  mov rdi, STDOUT_FD
  syscall
  lea rdi, [buf]
  ret

;--------------
; num2str - converts number to string of bytes (uses stack to reverse the order)
; Input:  rax    = number to convert
;         rbx    = radix
;         r10 -> alphabet string
;         rdi -> destination to convert into
; Destr:  rax, rdi, rdx, rcx 
;--------------
num2str:
  test rax, rax
  jz num_zero
  cmp rbx, 1
  je base1
  xor ecx, ecx
.windup:
	test rax, rax
	jz num_unwind
	xor edx, edx
	div rbx ; rax = rax / rbx, rdx = rax % rbx
  mov dl, [r10 + rdx]
  dec rsp
  mov byte [rsp], dl
  inc rcx
  jmp .windup
  jmp num_unwind

base1:
  mov rcx, rax
  mov al, '0'
  rep stosb
  ret

num_zero:
  mov al, '0'
  stosb
  ret

num_unwind:
.loop:
  test rcx, rcx
  jz .exit
  mov byte al, [rsp]
  inc rsp
  stosb
  loop .loop
.exit:
  ret

section .rodata

alpha equ alpha_lower
alpha_lower: db "0123456789abcdefghijklmnopqrstuvwxyz"
alpha_upper: db "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
alpha_len equ $ - alpha_upper
