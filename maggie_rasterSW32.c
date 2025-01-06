#include "maggie_internal.h"
#include "maggie_vertex.h"
#include "maggie_debug.h"

/*****************************************************************************/

static void DrawSpanZBuffer32(ULONG *destCol, UWORD *zBuffer, int len, ULONG Zz, LONG Ii, LONG dZz, LONG dIi)
{
	for(int i = 0; i < len; ++i)
	{
		UWORD z = (Zz >> 16);
		if(*zBuffer > z)
		{
			int ti = Ii >> 8;
			if(ti > 255)
				ti = 255;
			*zBuffer = z;
			*destCol = ti * 0x010101;
		}
		destCol++;
		zBuffer++;
		Ii += dIi;
		Zz += dZz;
	}
}

/*****************************************************************************/

void DrawSpansSW32ZBuffer(int ymin, int ymax, MaggieBase *lib)
{
	magEdgePos *edges = &lib->magEdge[ymin];
	int ylen = ymax - ymin;
	int modulo = lib->xres;
	ULONG *pixels = ((ULONG *)lib->screen) + ymin * lib->xres;
	UWORD *zbuffer = lib->depthBuffer + ymin * lib->xres;

	int scissorLeft = lib->scissor.x0;
	int scissorRight = lib->scissor.x1;

	for(int i = 0; i < ylen; ++i)
	{
		int x0 = edges[i].xPosLeft;
		int x1 = edges[i].xPosRight;

		ULONG *dstColPtr = pixels + x0;
		UWORD *dstZPtr = zbuffer + x0;

		pixels += modulo;
		zbuffer += modulo;

		int len = x1 - x0;

		if(len <= 0)
			continue;

		int xScissorDiff = 0;
		if(x0 < scissorLeft)
		{
			xScissorDiff = scissorLeft - x0;
			len -= xScissorDiff;
		}
		if(x1 > scissorRight)
		{
			len -= x1 - scissorRight;
		}
		if(len <= 0)
		{
			continue;
		}
		maggieRegs.pixDest = dstColPtr + xScissorDiff;
		maggieRegs.depthDest = dstZPtr + xScissorDiff;

		float xFracStart = edges[i].xPosLeft - x0;
		float preStep = 1.0f - xFracStart + xScissorDiff;
		float ooXLength = 1.0f / (edges[i].xPosRight - edges[i].xPosLeft);

		LONG zDDA = (edges[i].zowRight - edges[i].zowLeft) * ooXLength;
		ULONG zPos = edges[i].zowLeft + preStep * zDDA;
		LONG iDDA = (edges[i].iowRight - edges[i].iowLeft) * ooXLength;
		LONG iPos =  edges[i].iowLeft + preStep * iDDA;

		DrawSpanZBuffer32(dstColPtr, dstZPtr, len, zPos, iPos, zDDA, iDDA);
	}
}

/*****************************************************************************/

// static unsigned int seed = 2234234;
// unsigned int rndNum()
// {
// 	seed = seed * 1015871 + 1023499;
// 	return seed;
// }

static void DrawSpan32(ULONG *destCol, int len, LONG Ii, LONG dIi)
{
	for(int i = 0; i < len; ++i)
	{
		int ti = Ii >> 8;
		if(ti > 255)
			ti = 255;

		*destCol = ti * 0x010101;

		destCol++;
		Ii += dIi;
	}
}

/*****************************************************************************/

void DrawSpansSW32(int ymin, int ymax, MaggieBase *lib)
{
	magEdgePos *edges = &lib->magEdge[ymin];
	int ylen = ymax - ymin;
	int modulo = lib->xres;
	ULONG *pixels = ((ULONG *)lib->screen) + ymin * lib->xres;

	int scissorLeft = lib->scissor.x0;
	int scissorRight = lib->scissor.x1;

	for(int i = 0; i < ylen; ++i)
	{
		int x0 = edges[i].xPosLeft;
		int x1 = edges[i].xPosRight;

		ULONG *dstColPtr = pixels + x0;

		pixels += modulo;

		int len = x1 - x0;

		if(len <= 0)
			continue;

		int xScissorDiff = 0;
		if(x0 < scissorLeft)
		{
			xScissorDiff = scissorLeft - x0;
			len -= xScissorDiff;
		}
		if(x1 > scissorRight)
		{
			len -= x1 - scissorRight;
		}
		if(len <= 0)
		{
			continue;
		}
		maggieRegs.pixDest = dstColPtr + xScissorDiff;

		float xFracStart = edges[i].xPosLeft - x0;
		float preStep = 1.0f - xFracStart + xScissorDiff;
		float ooXLength = 1.0f / (edges[i].xPosRight - edges[i].xPosLeft);
		LONG iDDA = (edges[i].iowRight - edges[i].iowLeft) * ooXLength;
		LONG iPos =  edges[i].iowLeft + preStep * iDDA;

		DrawSpan32(dstColPtr, len, iPos, iDDA);
	}
}

/*****************************************************************************/
/*

ULONG MortonCode(ULONG x)
{
    x = (x | (x << 8)) & 0x00ff00ff;
    x = (x | (x << 4)) & 0x0f0f0f0f;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;

    return x;
}

void DrawSpanZBufferedTxtr32(ULONG *destCol, UWORD *zBuffer, ULONG *texture, ULONG txtrMask,
											int len,
											ULONG ZZzz, ULONG UUuu, ULONG VVvv, UWORD Ii,
											LONG dZZzz, ULONG dUUuu, ULONG dVVvv, UWORD dIi)
{
    UUuu = (UUuu >> 8) & 0xffff;
    VVvv = (VVvv >> 8) & 0xffff;
    dUUuu = (dUUuu >> 8) & 0xffff;
    dVVvv = (dVVvv >> 8) & 0xffff;

	UUuu = MortonCode(UUuu) | 0xaaaaaaaa;
	VVvv = (MortonCode(VVvv) << 1) | 0x55555555;
	dUUuu = MortonCode(dUUuu);
	dVVvv = MortonCode(dVVvv) << 1;

	for(int i = 0; i < len; ++i)
	{
		UWORD z = (ZZzz >> 16);
		if(*zBuffer > z)
		{
			int ti = Ii >> 8;
			ULONG col = texture[((UUuu & VVvv) >> 16) & txtrMask];
			ULONG colRB = (col & 0xff00ff) * ti >> 8;
			ULONG colG = (col & 0x00ff00) * ti >> 8;

			*zBuffer = z;
			*destCol = colRB | colG;
		}
		destCol++;
		zBuffer++;
		UUuu = (UUuu + dUUuu) | 0xaaaaaaaa;
		VVvv = (VVvv + dVVvv) | 0x55555555;
		Ii += dIi;
		ZZzz += dZZzz;
	}
}

*/
/*****************************************************************************/
