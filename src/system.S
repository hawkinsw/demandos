.data
.global _poweroff
_poweroff:
	mv a0, zero
	mv a1, zero
	mv a7, zero
	li a7, 0x53525354
	mv a6, zero
	ecall