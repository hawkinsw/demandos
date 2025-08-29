/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * M-mode Trap Handler Code for RISC-V Core
 *
 * Copyright (c) 2017 Microsemi Corporation.
 * Copyright (c) 2017 Padmarao Begari <Padmarao.Begari@microsemi.com>
 *
 * Copyright (C) 2017 Andes Technology Corporation
 * Rick Chen, Andes Technology Corporation <rick@andestech.com>
 *
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 *
 * Copyright (C) 2025, Will Hawkins <whh8b@obs.cr>
 */

.data

 _trap_handlers:
 .quad 0
 .quad software_interrupt_handler
 .quad 0
 .quad 0
 .quad 0
 .quad timer_interrupt_handler
 .quad 0
 .quad 0
 .quad system_call_dispatcher


.macro allocate_stack size:req
	addi sp, sp, -1 * (\size)
.endm

.macro deallocate_stack size:req
	addi sp, sp, (\size)
.endm

.macro push_registers
	sd x1,   1 * 8(sp)
	sd x2,   2 * 8(sp)
	sd x3,   3 * 8(sp)
	sd x4,   4 * 8(sp)
	sd x5,   5 * 8(sp)
	sd x6,   6 * 8(sp)
	sd x7,   7 * 8(sp)
	sd x8,   8 * 8(sp)
	sd x9,   9 * 8(sp)
	sd x10, 10 * 8(sp)
	sd x11, 11 * 8(sp)
	sd x12, 12 * 8(sp)
	sd x13, 13 * 8(sp)
	sd x14, 14 * 8(sp)
	sd x15, 15 * 8(sp)
	sd x16, 16 * 8(sp)
	sd x17, 17 * 8(sp)
	sd x18, 18 * 8(sp)
	sd x19, 19 * 8(sp)
	sd x20, 20 * 8(sp)
	sd x21, 21 * 8(sp)
	sd x22, 22 * 8(sp)
	sd x23, 23 * 8(sp)
	sd x24, 24 * 8(sp)
	sd x25, 25 * 8(sp)
	sd x26, 26 * 8(sp)
	sd x27, 27 * 8(sp)
	sd x28, 28 * 8(sp)
	sd x29, 29 * 8(sp)
	sd x30, 30 * 8(sp)
	sd x31, 31 * 8(sp)
.endm

.macro get_push_trap_registers scause:req, sepc:req, stval:req
	csrr \scause, scause
	sd \scause,  32 * 8(sp)
	csrr \sepc, sepc
	sd \sepc,  33 * 8(sp)
	csrr \stval, stval
	sd \stval,  34 * 8(sp)
.endm

.macro load_trap_handler_address into:req, trap: req
	// There is a jump table to the actual handler.
	la \into, _trap_handlers
	// Each entry is 8 bytes long. Multiply the cause by 8.
	// Assume it is valid.
	slli \trap, \trap, 3
	// Compute the address of the address of the handler.
	add \into, \into, \trap
	// Load the address of the handler.
	ld  \into, 0(\into)
.endm

.text
	.align 6
	.global _trap_dispatch
_trap_dispatch:
	allocate_stack size=(43*8)
	push_registers
	get_push_trap_registers scause=t0 sepc=t1 stval=t2

	sd a7,  35 * 8(sp) // user cause
	sd a0,  36 * 8(sp) // system call arguments
	sd a1,  37 * 8(sp) // system call arguments
	sd a2,  38 * 8(sp) // system call arguments
	sd a3,  39 * 8(sp) // system call arguments
	sd a4,  40 * 8(sp) // system call arguments
	sd a5,  41 * 8(sp) // system call arguments
	sd sp,  42 * 8(sp) // system call arguments

	load_trap_handler_address into=t6 trap=t0

	//ld a0,  32 * 8(sp) // cause
	csrr a0,  sstatus
	ld a1,  33 * 8(sp) // exception address
	ld a2,  34 * 8(sp) // stval
	ld a3,  42 * 8(sp) // stack pointer
	ld a4,  35 * 8(sp) // user cause
	ld a5,  36 * 8(sp) // a0
	ld a6,  37 * 8(sp) // a1
	ld a7,  38 * 8(sp) // a2

	jalr t6
	
	// Find out the address of the exception.
	ld t1,  33 * 8(sp)

	li t6, 0x8
	ld t5,  32 * 8(sp) // get the stored scause.
	bne t5, t6, skip_sepc_adjust
	// Don't return right there ... return to
	// the next one!
	addi t1, t1, 4
skip_sepc_adjust:
	csrw sepc, t1

	ld x1,   1 * 8(sp)
	ld x2,   2 * 8(sp)
	ld x3,   3 * 8(sp)
	ld x4,   4 * 8(sp)
	ld x5,   5 * 8(sp)
	ld x6,   6 * 8(sp)
	ld x7,   7 * 8(sp)
	ld x8,   8 * 8(sp)
	ld x9,   9 * 8(sp)
	// Do not overwrite a0
	// or a1
	beq t5, t6, skip_return_value_restore
	ld x10,   10 * 8(sp)
	ld x11,   11 * 8(sp)
skip_return_value_restore:
	ld x12, 12 * 8(sp)
	ld x13, 13 * 8(sp)
	ld x14, 14 * 8(sp)
	ld x15, 15 * 8(sp)
	ld x16, 16 * 8(sp)
	ld x17, 17 * 8(sp)
	ld x18, 18 * 8(sp)
	ld x19, 19 * 8(sp)
	ld x20, 20 * 8(sp)
	ld x21, 21 * 8(sp)
	ld x22, 22 * 8(sp)
	ld x23, 23 * 8(sp)
	ld x24, 24 * 8(sp)
	ld x25, 25 * 8(sp)
	ld x26, 26 * 8(sp)
	ld x27, 27 * 8(sp)
	ld x28, 28 * 8(sp)
	ld x29, 29 * 8(sp)
	ld x30, 30 * 8(sp)
	ld x31, 31 * 8(sp)

	deallocate_stack size=(43*8)
	sret
