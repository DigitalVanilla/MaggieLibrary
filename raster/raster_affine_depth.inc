	sub.l d0,d1
	ble.w .noVlinesToDo

	movem.l    d2-d5/a2-a4,-(a7)

	move.l  d1,d5

	GetEdgesPtr a2
	GetXRes d2
	mulu.w	d2,d0
	GetScreenPtr a3
	GetDepthPtr a4

	lea		(a3,d0.l*.bpp),a3			;pixels
	lea		(a4,d0.l*2),a4			;zbuffer

	fmove.w	#1,e4
	lea		$dff250,a0
.vLoop:
	fmove.s	EAffPos_xPosLeft(a2),e0	;xLeft
	fmove.s	EAffPos_xPosRight(a2),e1	;xRight

	fmove.l	e0,d3					;xStart = (int)xLeft;
	fmove.l	e1,d4					;xEnd = (int)xRight;

 	sub.l	d3,d4					;len = x1 - x0
 	ble		.noPixels

	fadd.l	d3,e4,e2
	fsub	e0,e1,e3	; xDiff = xRight - xLeft

	lea		(a3,d3.l*.bpp),a1			;dstColPtr = pixels + xStart
	move.l	a1,Maggie_pixDest(a0)
	lea		(a4,d3.l*2),a1			;dstColPtr = zbuffer + xStart
	move.l	a1,Maggie_depthDest(a0)

	fmove.s	EAffPos_zLeft(a2),e10	; zLeft
	fmove.s	EAffPos_zRight(a2),e11	; zRight
	fsub	e0,e2,e2				;preStep = 1 + xStart - edges[i].xPosLeft
	fdiv	e3,e4,e3				; ooXLength = 1.0f / xDiff
	fmove.s	EAffPos_iLeft(a2),e12	; iowLeft
	fmove.s	EAffPos_iRight(a2),e13	; iowRight
	fmove.s	EAffPos_uLeft(a2),e17	;uowLeft
	fmove.s	EAffPos_uRight(a2),e18	;uowRight
	fmove.s	EAffPos_vLeft(a2),e20	;vowLeft
	fmove.s	EAffPos_vRight(a2),e21	;vowRight
	fsub	e10,e11					; zDiff = zRight - zLeft
	fsub	e12,e13					; iDiff = iRight - iLeft
	fsub	e17,e18					;uDiff = uRight - uLeft
	fsub	e20,e21					;vDiff = vRight - vLeft
	fmul	e3,e11					; zDDA = zDiff * ooXLength
	fmul	e3,e13					; iDDA = iDiff * ooXLength
	fmul	e3,e18					;uDDA = uDiff * ooXLength
	fmul	e3,e21					;vDDA = vDiff * ooXLength
	fmul	e2,e11,e7				; preStepZ = preStep * zDDA
	fmul	e2,e13,e8				; preStepI = preStep * iDDA
	fmul	e2,e18,e19				;preStepU = preStep * uDDA
	fmul	e2,e21,e22				;preStepV = preStep * vDDA
	fadd	e7,e10					; depthStart = zLeft + preStepZ
	fadd	e8,e12					; light = iLeft + preStepI
	fadd	e19,e17					;uPos = uLeft + preStepU
	fadd	e22,e20					;vPos = vLeft + preStepV

	fmove.l	e11,Maggie_depthDelta(a0)
	fmove.w	e13,Maggie_lightDelta(a0)

	fsub.s	#$4f000000,e10

	fmove.l	e18,Maggie_uDelta(a0)
	fmove.l	e21,Maggie_vDelta(a0)
	fmove.l	e17,Maggie_uCoord(a0)
	fmove.l	e20,Maggie_vCoord(a0)

	fmove.l	e12,d1
	fmove.l e10,d0
	add.l	#-2147483648,d0

	move.w	d1,Maggie_light(a0)
	move.l	d0,Maggie_depthStart(a0)
	move.w	d4,Maggie_startLength(a0)
.noPixels:
	lea		(a3,d2.l*.bpp),a3			;pixels += modulo
	lea		(a4,d2.l*2),a4			;zbuffer += modulo
	lea		EAffPos_Size(a2),a2

	subq.l	#1,d5 ; ylen
	bne		.vLoop

	movem.l	(a7)+,d2-d5/a2-a4

.noVlinesToDo:
	rts
