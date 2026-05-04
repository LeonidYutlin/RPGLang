; --------- RPGLang Standard Library --------- 

default rel

; -------------- GLOBAL HEADER ---------------

global out
global rout
global random
global exit

; -------------- ------ ------ ---------------

;TODO: move buffer to be local to every call of the function

; ------------ CALLING CONVENTIONS -----------
; Return value: rax
; First 6 args: rdi, rsi, rdx, rcx, r8, r9
; CALLEE-saved: r12, r13, r14, r15, rbx, rsp, rbp
; Rest are CALLER-saved
; ------------ ------- ----------- -----------

; -------------------- BSS -------------------

section .bss

BUF_SIZE equ 32
buf: resb BUF_SIZE

; -------------------- --- -------------------

; ------------------- TEXT -------------------

section .text

REG_SIZE equ 8
SIGN_BIT_MASK equ 0x80000000
NEWLINE equ 0xA
DECIMAL_RADIX equ 10
STDOUT_FD equ 1

EXIT_SYSCALL  equ 0x3c
WRITE_SYSCALL equ 0x1

; ------------- GLOBAL FUNCTIONS -------------

;--------------
; exit - exits the program with a given exit code
; Input:  rdi = exit code
; Destr:  rax, but does that matter if your program died lol
;--------------
exit:
		mov rax, EXIT_SYSCALL
		syscall
    ret

;--------------
; rout - prints unsigned 64-bit number to stdout, 
;        with a newline at the end, in a given radix
; Input:  rdi = number to print
;         rsi = radix
; Destr:  rdi, rdx, rcx, r10, rsi
;--------------
rout:
    push rax
    push rbx
    test rsi, rsi
    jz .exit
    cmp rsi, alpha_len
    jg .exit
    mov rax, rdi
    mov rbx, rsi
    lea rdi, [buf]
    jmp num_common
.exit:
    pop rbx
    pop rax
    ret

;--------------
; out - prints signed 64-bit number to stdout, with a newline at the end
; Input:  rdi = number to print
; Destr:  rdi, rdx, rcx, r10, rsi
;--------------
out:
    push rax
    push rbx

    mov rax, rdi
    lea rdi, [buf]
    test rax, SIGN_BIT_MASK
    mov rbx, DECIMAL_RADIX
    jz num_common

    push rax
    mov al, '-'
    stosb
    pop rax
    not rax
    inc rax

num_common:
    lea r10, [alpha]
    call num2str
    mov al, NEWLINE
    stosb

.flush:
    mov rdx, rdi
    lea rsi, [buf]
    sub rdx, rsi
    mov rax, WRITE_SYSCALL
    mov rdi, STDOUT_FD
    syscall
    lea rdi, [buf]

    pop rbx
    pop rax
    ret

CLOSE_SYSCALL equ 0x3
OPEN_SYSCALL  equ 0x2
READ_SYSCALL  equ 0x0
READ_ONLY equ 0

RAND_INT_SIZE equ 1
RAND_MODULO equ 20

; TODO: documentation for this function
; TODO: make it so it doesnt open and close the file every single time
; TODO: this violates the calling conventions. Yep.
random:
    push rbp
    mov rbp, rsp
    sub rsp, REG_SIZE * 2

    mov rax, OPEN_SYSCALL
    lea rdi, [DEV_RANDOM]
    mov rsi, READ_ONLY
    syscall 

    mov qword [rbp - REG_SIZE], rax
    
    mov rdi, rax
    lea rax, [rbp - REG_SIZE * 2]
    mov rsi, rax
    mov rax, READ_SYSCALL
    mov rdx, RAND_INT_SIZE
    syscall


    mov rax, CLOSE_SYSCALL
    mov rdi, [rbp - REG_SIZE]
    syscall 

    xor ax, ax
    mov al, byte [rbp - REG_SIZE * 2]
    mov bl, RAND_MODULO
    div bl
    mov byte [rbp - REG_SIZE * 2], ah

    ;xor edi, edi
    ;mov dil, byte [rbp - REG_SIZE * 2]
    ;call out

    mov al, byte [rbp - REG_SIZE * 2]
    test al, al
    jnz .success
.failure:
    lea rsi, [failure]
    mov rdx, failure_len
    mov rax, WRITE_SYSCALL
    mov rdi, STDOUT_FD
    syscall

    xor edi, edi
    call exit
    ret
.success:

    mov rsp, rbp
    pop rbp
    ret

; ------------ INTERNAL FUNCTIONS ------------

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

; ------------------- ---- -------------------

; ------------------ RODATA -------------------

section .rodata

alpha equ alpha_lower
alpha_lower: db "0123456789abcdefghijklmnopqrstuvwxyz"
alpha_upper: db "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
alpha_len equ $ - alpha_upper
failure: db "Critical Failure! GAME OVER", NEWLINE, 0
failure_len equ $ - failure
DEV_RANDOM: db "/dev/random", 0

; ------------------ ------ -------------------
