	section	.text

	include <exec/types.i>
	include "raster/raster_structs.i"

;------------------------------------------------------------------------------

_DrawScanlines32:
.bpp = 4
	include "raster/raster_perspective16.inc"
	public _DrawScanlines32

;------------------------------------------------------------------------------

_DrawScanlines32IZ:
.bpp = 4
	include "raster/raster_perspective16_depth.inc"
	public _DrawScanlines32IZ

;------------------------------------------------------------------------------

_DrawScanlines32Affine:
.bpp = 4
	include "raster/raster_affine.inc"
	public _DrawScanlines32Affine

;------------------------------------------------------------------------------

_DrawScanlines32ZAffine:
.bpp = 4
	include "raster/raster_affine_depth.inc"
	public _DrawScanlines32ZAffine

;------------------------------------------------------------------------------
;------------------------------------------------------------------------------

_DrawScanlines16:
.bpp = 2
	include "raster/raster_perspective16.inc"
	public _DrawScanlines16

;------------------------------------------------------------------------------

_DrawScanlines16IZ:
.bpp = 2
	include "raster/raster_perspective16_depth.inc"
	public _DrawScanlines16IZ

;------------------------------------------------------------------------------

_DrawScanlines16Affine:
.bpp = 2
	include "raster/raster_affine.inc"
	public _DrawScanlines16Affine

;------------------------------------------------------------------------------

_DrawScanlines16ZAffine:
.bpp = 2
	include "raster/raster_affine_depth.inc"
	public _DrawScanlines16ZAffine

;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
