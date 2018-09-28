
extern c_interrupt_shim

; error code
; interrupt number
; all registers

section .text

interrupt_shim:
    push eax
    push ecx
    push ebx
    push edx
    push esi
    push edi
    push ebp

    mov ebp, ds
    push ebp     ; push data segment

    mov ebp, 0x20
    mov ds, ebp  ; set kernel data segment

    push esp
    mov eax, c_interrupt_shim
    call eax

global return_from_interrupt
return_from_interrupt:
    add esp, 4

    pop ebp
    mov ds, ebp ; restore data segment

    pop ebp
    pop edi
    pop esi
    pop edx
    pop ebx
    pop ecx
    pop eax
    add esp, 8
    iret

%macro isrnoerr 1
    push 0
    push %1
    jmp interrupt_shim
%endmacro

%macro isrerr 1
    push %1
    jmp interrupt_shim
%endmacro

global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31
global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15
global isr_syscall
global isr_yield
global isr_panic

isr0: isrnoerr 0
isr1: isrnoerr 1
isr2: isrnoerr 2
isr3: isrnoerr 3
isr4: isrnoerr 4
isr5: isrnoerr 5
isr6: isrnoerr 6
isr7: isrnoerr 7
isr8: isrerr 8
isr9: isrnoerr 9
isr10: isrerr 10
isr11: isrerr 11
isr12: isrerr 12
isr13: isrerr 13
isr14: isrerr 14
isr15: isrnoerr 15
isr16: isrnoerr 16
isr17: isrerr 17
isr18: isrnoerr 18
isr19: isrnoerr 19
isr20: isrnoerr 20
isr21: isrnoerr 21
isr22: isrnoerr 22
isr23: isrnoerr 23
isr24: isrnoerr 24
isr25: isrnoerr 25
isr26: isrnoerr 26
isr27: isrnoerr 27
isr28: isrnoerr 28
isr29: isrnoerr 29
isr30: isrerr 30
isr31: isrnoerr 31
irq0: isrnoerr 32
irq1: isrnoerr 33
irq2: isrnoerr 34
irq3: isrnoerr 35
irq4: isrnoerr 36
irq5: isrnoerr 37
irq6: isrnoerr 38
irq7: isrnoerr 39
irq8: isrnoerr 40
irq9: isrnoerr 41
irq10: isrnoerr 42
irq11: isrnoerr 43
irq12: isrnoerr 44
irq13: isrnoerr 45
irq14: isrnoerr 46
irq15: isrnoerr 47
isr_syscall: isrnoerr 128
isr_yield: isrnoerr 129
isr_panic: isrnoerr 130

section .data
global idt
idt:
    resq 256
idt_end:


section .data
global idt_ptr
idt_ptr:
    dw idt_end - idt - 1
    dd idt
