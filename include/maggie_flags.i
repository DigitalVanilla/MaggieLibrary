
MAG_DRAWMODE_NORMAL			equ	$0000
MAG_DRAWMODE_DEPTHBUFFER	equ	$0001
MAG_DRAWMODE_BILINEAR		equ	$0002
MAG_DRAWMODE_32BIT			equ	$0004
MAG_DRAWMODE_LIGHTING		equ	$0008
MAG_DRAWMODE_CULL_CCW		equ	$0010
MAG_DRAWMODE_MIPMAP			equ	$0080

MAG_DRAWMODE_BLEND_REPLACE	equ	$0000
MAG_DRAWMODE_BLEND_ADD		equ	$0100
MAG_DRAWMODE_BLEND_MUL		equ	$0200
MAG_DRAWMODE_BLEND_MASK		equ	$0300

;*****************************************************************************

MAG_DRAWMODE_TEXGEN_UV		equ	$0000
MAG_DRAWMODE_TEXGEN_POS		equ	$0020
MAG_DRAWMODE_TEXGEN_NORMAL	equ	$0040
MAG_DRAWMODE_TEXGEN_REFLECT	equ	$0060

MAG_DRAWMODE_TEXGEN_MASK	equ	$0060

;*****************************************************************************

MAG_CLEAR_COLOUR			equ $0001
MAG_CLEAR_DEPTH				equ $0002

;*****************************************************************************

MAG_TEXFMT_DXT1				equ	$0000
MAG_TEXFMT_RGB				equ	$0001
MAG_TEXFMT_RGBA				equ	$0002
MAG_TEXFMT_MASK				equ	$00ff
MAG_TEXCOMP_HQ				equ	$8000

;*****************************************************************************

MAG_MAX_LIGHTS 				equ	8

MAG_LIGHT_OFF				equ	$0000
MAG_LIGHT_POINT				equ	$0001
MAG_LIGHT_DIRECTIONAL		equ	$0002
MAG_LIGHT_SPOT				equ	$0003
MAG_LIGHT_AMBIENT			equ	$0004
