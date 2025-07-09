.section .text

.global _boot
_boot:

	mv tp, a0
	mv s1, a1

	.option push
	.option norelax
	//la gp, __global_pointer$
	mv gp, zero
	.option pop

	/* set up the interrupts table. */
	//la t0, _isr_table
	//csrw stvec, t0

	la t0, _trap_dispatch
	csrw stvec, t0

	/* Set the sp (sp) and esp (end of the stack)
	 * based on what is in the linker script.
	 */
	la sp,_sp
	la t1, _esp
	li t2, -1 

	// Set the stack to all 1s
	// Use t2 as the value to write ...
	mv t2, zero
	addi t2, t2, 1
	// Use t3 as the iterator.
	mv t3, t1
one_stack_loop:
	sd t2, 0(t3)
	addi t3, t3, 1
	blt t3, sp, one_stack_loop

	// Set the stack to all 1s
	// Use t2 as the value to write ...
	mv t2, zero
	addi t2, t2, 0
	// Use t3 as the iterator.
	mv t3, t1
clear_stack_loop:
	sd t2, 0(t3)
	addi t3, t3, 1
	blt t3, sp, clear_stack_loop

	la t0, _trap_dispatch
	csrw stvec, t0

	// Turn off virtual memory.
	csrw satp, zero

	la a0, _brk_start
	call set_brk_s

	call configure_pci
	call configure_io
	
	call configure_syscall_handlers

	// Tell the CPU that we want to jump back to u-mode.
	li a0, 0x100
	csrc sstatus, a0

	// Enable interrupts
	csrsi sstatus, 2

	li t1, 32
	csrs sie, t1

	// And when we get there, start executing at _start.
	la t0, _start
	csrw sepc, t0

	li a0, 0x0
	li a1, 0x0
	li a2, 0x0
	li a3, 0x0
	li a4, 0x0
	li a5, 0x0
	li a6, 0x0
	li a7, 0x0
	
	// Go!
	sret
