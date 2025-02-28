#include "maggie_internal.h"
#include "maggie_vertex.h"
#include "maggie_debug.h"

/*****************************************************************************/

static int FToI(float f)
{
#if 0
	int i;

	__asm(
		"fmove.l %1,%0\n"
		: "=d" (i)
		: "f" (f)
		: "cc"
	);
	return i;
#else
	return (int)f;
#endif
}

/*****************************************************************************/

static unsigned int FToU(float f)
{
#if 0
	unsigned int i;

	__asm(
		"fintrz.x %1,%1\n"
		"\tfmove.l %1,%0\n"
		: "=d" (i)
		: "f" (f)
		: "cc"
	);
	return i;
#else
	return (unsigned int)f;
#endif
}

/*****************************************************************************/

static void my_WaitBlit()
{
	volatile UWORD *dmaconr = (UWORD *)0xdff002;
	while(*dmaconr & 0x4000) { }
}

/*****************************************************************************/

static void DrawHardwareSpan(int len, int dUUuu, int dVVvv)
{
	maggieRegs.uDelta = dUUuu;
	maggieRegs.vDelta = dVVvv;
	maggieRegs.startLength = len;
}

/*****************************************************************************/

void DrawSpansHW16ZBuffer(int ymin, int ymax, MaggieBase *lib)
{
	int ylen = ymax - ymin;

	if(ylen <= 0)
	{
		return;
	}
	magEdgePos *edges = &lib->magEdge[ymin];

	int modulo = lib->xres;
	UWORD *pixels = ((UWORD *)lib->screen) + ymin * lib->xres;
	UWORD *zbuffer = lib->depthBuffer + ymin * lib->xres;

	int scissorLeft = lib->scissor.x0;
	int scissorRight = lib->scissor.x1;

	for(int i = 0; i < ylen; ++i)
	{
		int x0 = edges[i].xPosLeft;
		int x1 = edges[i].xPosRight;

		UWORD *dstColPtr = pixels + x0;
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

		float zDDA = (edges[i].zowRight - edges[i].zowLeft) * ooXLength;
		maggieRegs.depthDelta = zDDA;
		maggieRegs.depthStart = edges[i].zowLeft + preStep * zDDA;
		float iDDA = (edges[i].iowRight - edges[i].iowLeft) * ooXLength;
		maggieRegs.lightDelta = iDDA;
		maggieRegs.light = edges[i].iowLeft + preStep * iDDA;
		float wDDA = (edges[i].oowRight - edges[i].oowLeft) * ooXLength;
		float wPos = edges[i].oowLeft + preStep * wDDA;
		float uDDA = (edges[i].uowRight - edges[i].uowLeft) * ooXLength;
		float uPos = edges[i].uowLeft + preStep * uDDA;
		float vDDA = (edges[i].vowRight - edges[i].vowLeft) * ooXLength;
		float vPos = edges[i].vowLeft + preStep * vDDA;

		float w = 1.0f / wPos;
		LONG uStart = (uPos * w);
		LONG vStart = (vPos * w);

		maggieRegs.uCoord = uStart;
		maggieRegs.vCoord = vStart;

		int consecutiveRunFlag = 0;

		if(len >= PIXEL_RUN)
		{
			float wDDAFullRun = wDDA * PIXEL_RUN;
			float uDDAFullRun = uDDA * PIXEL_RUN;
			float vDDAFullRun = vDDA * PIXEL_RUN;

			float ooLen = 1.0f / PIXEL_RUN;

			while(len >= PIXEL_RUN)
			{
				wPos += wDDAFullRun;
				w = 1.0f / wPos;
				uPos += uDDAFullRun;
				vPos += vDDAFullRun;

				LONG uEnd = (uPos * w);
				LONG vEnd = (vPos * w);

				LONG dUUuu = (LONG)((uEnd - uStart) >> PIXEL_RUNSHIFT);
				LONG dVVvv = (LONG)((vEnd - vStart) >> PIXEL_RUNSHIFT);
				DrawHardwareSpan(consecutiveRunFlag | PIXEL_RUN, dUUuu, dVVvv);

				uStart = uEnd;
				vStart = vEnd;
				len -= PIXEL_RUN;
				consecutiveRunFlag |= 0x8000;
			}
		}
		if(len > 0)
		{
			float ooLen = 1.0f / len;

			wPos += wDDA * len;
			w = 1.0f / wPos;
			uPos += FToI(uDDA * len);
			vPos += FToI(vDDA * len);

			LONG uEnd = FToI(uPos * w);
			LONG vEnd = FToI(vPos * w);

			LONG dUUuu = FToI((uEnd - uStart) * ooLen);
			LONG dVVvv = FToI((vEnd - vStart) * ooLen);

			DrawHardwareSpan(consecutiveRunFlag | len, dUUuu, dVVvv);
		}
	}
}

/*****************************************************************************/

void DrawSpansHW16(int ymin, int ymax, MaggieBase *lib)
{
	magEdgePos *edges = &lib->magEdge[ymin];
	int ylen = ymax - ymin;
	int modulo = lib->xres;
	UWORD *pixels = ((UWORD *)lib->screen) + ymin * lib->xres;

	int scissorLeft = lib->scissor.x0;
	int scissorRight = lib->scissor.x1;

	for(int i = 0; i < ylen; ++i)
	{
		int x0 = edges[i].xPosLeft;
		int x1 = edges[i].xPosRight;

		UWORD *dstColPtr = pixels + x0;

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

		float iDDA = (edges[i].iowRight - edges[i].iowLeft) * ooXLength;
		maggieRegs.lightDelta = iDDA;
		maggieRegs.light = edges[i].iowLeft + preStep * iDDA;

		float wDDA = (edges[i].oowRight - edges[i].oowLeft) * ooXLength;
		float wPos = edges[i].oowLeft + preStep * wDDA;
		float uDDA = (edges[i].uowRight - edges[i].uowLeft) * ooXLength;
		float uPos = edges[i].uowLeft + preStep * uDDA;
		float vDDA = (edges[i].vowRight - edges[i].vowLeft) * ooXLength;
		float vPos = edges[i].vowLeft + preStep * vDDA;

		float w = 1.0f / wPos;
		LONG uStart = FToI(uPos * w);
		LONG vStart = FToI(vPos * w);

		maggieRegs.uCoord = uStart;
		maggieRegs.vCoord = vStart;

		int consecutiveRunFlag = 0;

		if(len > PIXEL_RUN)
		{
			float wDDAFullRun = wDDA * PIXEL_RUN;
			LONG uDDAFullRun = uDDA * PIXEL_RUN;
			LONG vDDAFullRun = vDDA * PIXEL_RUN;

			float ooLen = 1.0f / PIXEL_RUN;

			while(len >= PIXEL_RUN)
			{
				wPos += wDDAFullRun;
				w = 1.0f / wPos;
				uPos += uDDAFullRun;
				vPos += vDDAFullRun;

				LONG uEnd = FToI(uPos * w);
				LONG vEnd = FToI(vPos * w);

				LONG dUUuu = (uEnd - uStart) >> PIXEL_RUNSHIFT;
				LONG dVVvv = (vEnd - vStart) >> PIXEL_RUNSHIFT;

				DrawHardwareSpan(consecutiveRunFlag | PIXEL_RUN, dUUuu, dVVvv);

				uStart = uEnd;
				vStart = vEnd;
				len -= PIXEL_RUN;
				consecutiveRunFlag |= 0x8000;
			}
		}
		if(len > 0)
		{
			float w = 1.0f / wPos;
			wPos += wDDA * len;
			float ooLen = 1.0f / len;

			uPos += FToI(uDDA * len);
			vPos += FToI(vDDA * len);

			w = 1.0f / wPos;

			LONG uEnd = FToI(uPos * w);
			LONG vEnd = FToI(vPos * w);

			LONG dUUuu = (LONG)((uEnd - uStart) * ooLen);
			LONG dVVvv = (LONG)((vEnd - vStart) * ooLen);

			DrawHardwareSpan(consecutiveRunFlag | len, dUUuu, dVVvv);
		}
	}
}

/*****************************************************************************/
