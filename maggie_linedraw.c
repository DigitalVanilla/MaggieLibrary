#include "maggie_internal.h"
#include "maggie_vertex.h"

/*****************************************************************************/

void DrawLineAsm(magEdgePos *edge __asm("a0"),
				const struct MaggieTransVertex *v0 __asm("a1"),
				const struct MaggieTransVertex *v1 __asm("a2"),
				float corrFactor __asm("fp0"),
				float preStep0 __asm("fp1"),
				int lineLen __asm("d0"));

void DrawLine(magEdgePos * restrict edge, int tex, const struct MaggieTransVertex * restrict v0, const struct MaggieTransVertex * restrict v1)
{
	int y0 = (int)v0->pos.y;
	int y1 = (int)v1->pos.y;

	int lineLen = y1 - y0;

	if(lineLen <= 0)
		return;

	float ooLineLen = 1.0f / lineLen;

	edge += y0;

	float preStep = 1.0f + y0 - v0->pos.y;
    float ooYLen = 1.0f / (v1->pos.y - v0->pos.y);
#if 0
	DrawLineAsm(edge, v0, v1, yLen, preStep0, lineLen);
#else
	float xLen = v1->pos.x - v0->pos.x;
	float zLen = v1->pos.z - v0->pos.z;
	int iLen = ((int)v1->colour - (int)v0->colour);
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
		edge[i].zowLeft = zow;
		edge[i].iowLeft = iow;
		edge[i].oowLeft = oow;
		edge[i].uowLeft = uow;
		edge[i].vowLeft = vow;
		xVal += xDDA;
		iow += iDDA;
		zow += zDDA;
		oow += wDDA;
		uow += uDDA;
		vow += vDDA;
	}
#endif
}

/*****************************************************************************/

void DrawEdge(struct MaggieTransVertex *vtx0, struct MaggieTransVertex *vtx1, MaggieBase *lib)
{
#if PROFILE
	ULONG startTime = GetClocks();
#endif
	if(lib->drawMode & MAG_DRAWMODE_CULL_CCW)
	{
		if(vtx0->pos.y <= vtx1->pos.y)
		{
			DrawLine((magEdgePos *)((float *)&lib->magEdge[0].xPosRight), 0, vtx0, vtx1);
		}
		else
		{
			DrawLine(lib->magEdge, 0, vtx1, vtx0);
		}
	}
	else
	{
		if(vtx0->pos.y <= vtx1->pos.y)
		{
			DrawLine(lib->magEdge, 0, vtx0, vtx1);
		}
		else
		{
			DrawLine((magEdgePos *)((float *)&lib->magEdge[0].xPosRight), 0, vtx1, vtx0);
		}
	}
#if PROFILE
	lib->profile.lines += GetClocks() - startTime;
	lib->profile.nLinePixels += fabs(vtx1->pos.y - vtx0->pos.y);
#endif
}
