#ifndef MAGGIE_FLAGS_H_INCLUDED
#define MAGGIE_FLAGS_H_INCLUDED

/*****************************************************************************/

#define MAG_DRAWMODE_NORMAL			0x0000
#define MAG_DRAWMODE_DEPTHBUFFER	0x0001
#define MAG_DRAWMODE_BILINEAR		0x0002
#define MAG_DRAWMODE_32BIT			0x0004
#define MAG_DRAWMODE_LIGHTING		0x0008
#define MAG_DRAWMODE_CULL_CCW		0x0010
#define MAG_DRAWMODE_MIPMAP			0x0080

/*****************************************************************************/

#define MAG_DRAWMODE_AFFINE_MAPPING	0x1000

/*****************************************************************************/

#define MAG_DRAWMODE_BLEND_REPLACE	0x0000
#define MAG_DRAWMODE_BLEND_ADD		0x0100
#define MAG_DRAWMODE_BLEND_MUL		0x0200
#define MAG_DRAWMODE_BLEND_MASK		0x0300

/*****************************************************************************/

#define MAG_DRAWMODE_TEXGEN_UV		0x0000
#define MAG_DRAWMODE_TEXGEN_POS		0x0020
#define MAG_DRAWMODE_TEXGEN_NORMAL	0x0040
#define MAG_DRAWMODE_TEXGEN_REFLECT	0x0060

#define MAG_DRAWMODE_TEXGEN_MASK	0x0060

/*****************************************************************************/

#define MAG_CLEAR_COLOUR 0x0001
#define MAG_CLEAR_DEPTH 0x0002

/*****************************************************************************/

#define MAG_TEXFMT_DXT1	0x0000
#define MAG_TEXFMT_RGB	0x0001
#define MAG_TEXFMT_RGBA	0x0002
#define MAG_TEXFMT_MASK	0x00ff
#define MAG_TEXCOMP_HQ	0x8000

/*****************************************************************************/

#define MAG_MAX_LIGHTS			8

#define MAG_LIGHT_OFF			0x0000
#define MAG_LIGHT_POINT			0x0001
#define MAG_LIGHT_DIRECTIONAL	0x0002
#define MAG_LIGHT_SPOT			0x0003
#define MAG_LIGHT_AMBIENT		0x0004

/*****************************************************************************/

/* MAGGIE_FLAGS_H_INCLUDED */
#endif
