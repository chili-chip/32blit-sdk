/* The gate between the loader and a sandboxed .blit game (32blit-sdk #91).
 *
 * sandbox_enter drops to unprivileged Thread mode on the process stack (PSP)
 * and calls the game. The game reaches the loader only through `svc`:
 * - each APIConst slot the game sees is a trampoline `svc #slot; bx lr`; the
 *   SVC handler sends it to sandbox_api_dispatch, privileged, which checks the
 *   call in sandbox.cpp, makes it and drops privilege again;
 * - the game returns to sandbox_return, whose `svc #255` gives privilege back
 *   only at that address, and sandbox_exit returns to the loader.
 * Faults in the game end at sandbox_exit too (sandbox_fault_c).
 * Armv8-M: CONTROL.nPRIV/SPSEL, SVC and exception return are in the Armv8-M
 * Architecture Reference Manual (DDI0553), B3.
 */
#if defined(BLIT_SANDBOX) && defined(BUILD_LOADER)
#include "sandbox_abi.h"

.syntax unified
.cpu cortex-m33
.thumb

.section .text.sandbox_gate, "ax"
.align 5

/* One trampoline per APIConst slot: 4 bytes each, in slot order. */
.macro trampoline n
.global sandbox_api_\n
.type sandbox_api_\n, %function
.thumb_func
sandbox_api_\n:
    svc #\n
    bx lr
.endm

.altmacro
.set slot, 0
.rept SANDBOX_API_SLOTS
    trampoline %slot
    .set slot, slot + 1
.endr
.noaltmacro

/* uint32_t sandbox_enter(const void *function, uint32_t arg0, uint32_t arg1, uint32_t stack_top)
 * CONTROL[1:0] for the game is in sandbox_thread_control. */
.global sandbox_enter
.type sandbox_enter, %function
.thumb_func
sandbox_enter:
    push {r4-r11, ip, lr}
    msr psp, r3
    mov ip, r0
    mov r0, r1
    mov r1, r2
    movs r2, #0
    movs r3, #0
    ldr lr, =sandbox_return
    ldr r4, =sandbox_thread_control
    ldr r4, [r4]
    mrs r5, control
    bic r5, r5, #3
    orr r5, r5, r4
    /* the game gets no loader values in its registers */
    movs r4, #0
    msr control, r5
    isb
    movs r5, #0
    mov r6, r5
    mov r7, r5
    mov r8, r5
    mov r9, r5
    mov r10, r5
    mov r11, r5
    bx ip

/* The game returns here. */
.global sandbox_return
.type sandbox_return, %function
.thumb_func
sandbox_return:
    svc #SANDBOX_SVC_RETURN

/* Privileged again (the SVC or fault handler cleared nPRIV); still on PSP. */
.global sandbox_exit
.type sandbox_exit, %function
.thumb_func
sandbox_exit:
    mrs r2, control
    bic r2, r2, #3
    msr control, r2
    isb
    pop {r4-r11, ip, pc}

/* Entered from the SVC handler: privileged Thread mode on the game's stack,
 * r0-r3 the arguments, ip the slot, lr where the game called from. */
.global sandbox_api_dispatch
.type sandbox_api_dispatch, %function
.thumb_func
sandbox_api_dispatch:
    push {r0-r4, lr}
    mov r0, sp
    mov r1, ip
    bl sandbox_check_call       /* 0: refuse, 1: stop the game, else the function to call */
    mov ip, r0
    pop {r0-r3}
    cmp ip, #1
    beq 3f
    cmp ip, #0
    beq 1f
    blx ip
    b 2f
1:
    movs r0, #0
    movs r1, #0
2:
    /* a stop requested during the call happens now, not inside the loader */
    push {r0, r1}
    bl sandbox_api_done
    mov ip, r0
    pop {r0, r1}
    cmp ip, #0
    bne 3f
    ldr r2, =sandbox_thread_control
    ldr r2, [r2]
    mrs r4, control
    bic r4, r4, #3
    orr r4, r4, r2
    msr control, r4
    isb
    pop {r4, pc}
3:
    movs r0, #0
    b sandbox_exit

/* SVC: sandbox_svc_c returns the EXC_RETURN to use. */
.global isr_svcall
.type isr_svcall, %function
.thumb_func
isr_svcall:
    mov r0, lr
    push {r0, r1}
    bl sandbox_svc_c
    add sp, #8
    bx r0

/* MemManage, BusFault, UsageFault, HardFault and PendSV (a requested stop):
 * a fault in the game ends the game, anything else stops as before. */
.global sandbox_fault_entry
.type sandbox_fault_entry, %function
.thumb_func
sandbox_fault_entry:
    mov r0, lr
    push {r0, r1}
    bl sandbox_fault_c
    pop {r1, r2}
    cbz r0, 1f
    bx r0
1:
    mov lr, r1
    bkpt #0
    b .

.global isr_memmanage
.global isr_busfault
.global isr_usagefault
.global isr_hardfault
.global isr_pendsv
.thumb_set isr_memmanage, sandbox_fault_entry
.thumb_set isr_busfault, sandbox_fault_entry
.thumb_set isr_usagefault, sandbox_fault_entry
.thumb_set isr_hardfault, sandbox_fault_entry
.thumb_set isr_pendsv, sandbox_fault_entry

.ltorg
#endif
