; Warwick's joystick interrupt routine in MOTOROLA/Jas syntax -- Eero
	.text

	.globl	_JoyISR

_JoyISR:
	move.l	#_JoyFlags,a1
	moveq.l	#0,d0
	move.l	4(sp),a0	; Get joystick packet address.
	move.b	1(a0),d0
	cmp.b	#0xFF,(a0)
	bne.s	Joy0
	move.b	2(a0),d0
	addq.l	#4,a1
Joy0:
	move.l	d0,(a1)

	rts
