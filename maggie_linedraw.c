#include "maggie_internal.h"
#include "maggie_vertex.h"

/*****************************************************************************/

void DrawLineAsm(magEdgePos *edge __asm("a0"),
				const struct MaggieTransVertex *v0 __asm("a1"),
				const struct MaggieTransVertex *v1 __asm("a2"),
				float corrFactor __asm("fp0"),
				float preStep0 __asm("fp1"),
				int lineLen __asm("d0"));
void DrawLineAffineAsm(magEdgePosAffine *edge __asm("a0"),
				const struct MaggieTransVertex *v0 __asm("a1"),
				const struct MaggieTransVertex *v1 __asm("a2"),
				float corrFactor __asm("fp0"),
				float preStep0 __asm("fp1"),
				int lineLen __asm("d0"));

static void DrawLineAffine(magEdgePosAffine * restrict edge, int tex, const struct MaggieTransVertex * restrict v0, const struct MaggieTransVertex * restrict v1, int miny)
{
	int y0 = (int)v0->pos.y;
	int y1 = (int)v1->pos.y;

	int lineLen = y1 - y0;

	if(lineLen <= 0)
		return;

	float ooLineLen = 1.0f / lineLen;

	edge += y0 - miny;

	float preStep = 1.0f + y0 - v0->pos.y;
    float ooYLen = 1.0f / (v1->pos.y - v0->pos.y);

#if 1
	DrawLineAffineAsm(edge, v0, v1, ooYLen, preStep, lineLen);
#else
	float xLen = v1->pos.x - v0->pos.x;
	float zLen = v1->pos.z - v0->pos.z;
	float iLen = v1->colour - v0->colour;
	float uLen = v1->tex[tex].u - v0->tex[tex].u;
	float vLen = v1->tex[tex].v - v0->tex[tex].v;

	float xDDA = xLen * ooYLen;
	float zDDA = zLen * ooYLen;
	float iDDA = iLen * ooYLen;
	float uDDA = uLen * ooYLen;
	float vDDA = vLen * ooYLen;

	float xVal = v0->pos.x + preStep * xDDA;
	float zow = v0->pos.z + preStep * zDDA;
	float iow = v0->colour + preStep * iDDA;
	float uow = v0->tex[tex].u + preStep * uDDA;
	float vow = v0->tex[tex].v + preStep * vDDA;

	for(int i = 0; i < lineLen; ++i)
	{
		edge[i].xPosLeft = xVal;
		xVal += xDDA;
		edge[i].zowLeft = zow;
		iow += iDDA;
		edge[i].iowLeft = iow;
		zow += zDDA;
		edge[i].uLeft = uow;
		uow += uDDA;
		edge[i].vLeft = vow;
		vow += vDDA;
	}
	#endif
}

/*****************************************************************************/

static void DrawLine(magEdgePos * restrict edge, int tex, const struct MaggieTransVertex * restrict v0, const struct MaggieTransVertex * restrict v1, int miny)
{
	int y0 = (int)v0->pos.y;
	int y1 = (int)v1->pos.y;

	int lineLen = y1 - y0;

	if(lineLen <= 0)
		return;

	float ooLineLen = 1.0f / lineLen;

	edge += y0 - miny;

	float preStep = 1.0f + y0 - v0->pos.y;
    float ooYLen = 1.0f / (v1->pos.y - v0->pos.y);
#if 1
	DrawLineAsm(edge, v0, v1, ooYLen, preStep, lineLen);
#else
	float xLen = v1->pos.x - v0->pos.x;
	float zLen = v1->pos.z - v0->pos.z;
	float iLen = v1->colour - v0->colour;
	float wLen = v1->pos.w - v0->pos.w;
	float uLen = v1->tex[tex].u - v0->tex[tex].u;
	float vLen = v1->tex[tex].v - v0->tex[tex].v;

	float xDDA = xLen * ooYLen;
	float zDDA = zLen * ooYLen;
	float iDDA = iLen * ooYLen;
	float wDDA = wLen * ooYLen;
	float uDDA = uLen * ooYLen;
	float vDDA = vLen * ooYLen;

	float xVal = v0->pos.x + preStep * xDDA;
	float zow = v0->pos.z + preStep * zDDA;
	float iow = v0->colour + preStep * iDDA;
	float oow = v0->pos.w + preStep * wDDA;
	float uow = v0->tex[tex].u + preStep * uDDA;
	float vow = v0->tex[tex].v + preStep * vDDA;

	for(int i = 0; i < lineLen; ++i)
	{
		edge[i].xPosLeft = xVal;
		xVal += xDDA;
		edge[i].zowLeft = zow;
		iow += iDDA;
		edge[i].iowLeft = iow;
		zow += zDDA;
		edge[i].oowLeft = oow;
		oow += wDDA;
		edge[i].uowLeft = uow;
		uow += uDDA;
		edge[i].vowLeft = vow;
		vow += vDDA;
	}
#endif
}
/*****************************************************************************/

void DrawEdge(struct MaggieTransVertex *vtx0, struct MaggieTransVertex *vtx1, int miny, int maxy, MaggieBase *lib)
{
#if PROFILE
	ULONG startTime = GetClocks();
#endif
	if(lib->drawMode & MAG_DRAWMODE_AFFINE_MAPPING)
	{
		if(lib->drawMode & MAG_DRAWMODE_CULL_CCW)
		{
			if(vtx0->pos.y <= vtx1->pos.y)
			{
				DrawLineAffine((magEdgePosAffine *)((float *)&lib->magEdgeAffine[0].xPosRight), 0, vtx0, vtx1, miny);
			}
			else
			{
				DrawLineAffine((magEdgePosAffine *)lib->magEdgeAffine, 0, vtx1, vtx0, miny);
			}
		}
		else
		{
			if(vtx0->pos.y <= vtx1->pos.y)
			{
				DrawLineAffine((magEdgePosAffine *)lib->magEdgeAffine, 0, vtx0, vtx1, miny);
			}
			else
			{
				DrawLineAffine((magEdgePosAffine *)((float *)&lib->magEdgeAffine[0].xPosRight), 0, vtx1, vtx0, miny);
			}
		}
	}
	else
	{
		if(lib->drawMode & MAG_DRAWMODE_CULL_CCW)
		{
			if(vtx0->pos.y <= vtx1->pos.y)
			{
				DrawLine((magEdgePos *)((float *)&lib->magEdge[0].xPosRight), 0, vtx0, vtx1, miny);
			}
			else
			{
				DrawLine(lib->magEdge, 0, vtx1, vtx0, miny);
			}
		}
		else
		{
			if(vtx0->pos.y <= vtx1->pos.y)
			{
				DrawLine(lib->magEdge, 0, vtx0, vtx1, miny);
			}
			else
			{
				DrawLine((magEdgePos *)((float *)&lib->magEdge[0].xPosRight), 0, vtx1, vtx0, miny);
			}
		}
	}
#if PROFILE
	lib->profile.lines += GetClocks() - startTime;
	lib->profile.nLinePixels += fabs(vtx1->pos.y - vtx0->pos.y);
#endif
}
