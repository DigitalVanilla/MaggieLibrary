#include "maggie_internal.h"
#include "maggie_debug.h"
#include <proto/graphics.h>
#include <float.h>

/*****************************************************************************/
/*****************************************************************************/

static void SetupHW(MaggieBase *lib)
{
	UWORD mode = lib->drawMode;
	UWORD drawMode = 0;
	if(mode & MAG_DRAWMODE_BILINEAR)
	{
		drawMode |=  0x0001;
	}
	if(mode & MAG_DRAWMODE_DEPTHBUFFER)
	{
		drawMode |= 0x0002;
	}
	if((mode & MAG_DRAWMODE_BLEND_MASK) == MAG_DRAWMODE_BLEND_ADD)
	{
		drawMode |= 0x0040;
	}
	if((mode & MAG_DRAWMODE_BLEND_MASK) == MAG_DRAWMODE_BLEND_MUL)
	{
		drawMode |= 0x0080;
	}
	UWORD modulo = 4;
	if(!(mode & MAG_DRAWMODE_32BIT))
	{
		drawMode |= 0x0004;
		modulo = 2;
	}
	maggieRegs.mode = drawMode;
	maggieRegs.modulo = modulo;
	maggieRegs.lightRGBA = lib->colour;

	APTR txtrData = GetTextureData(lib->textures[lib->txtrIndex]);
	UWORD txtrSize = GetTexSizeIndex(lib->textures[lib->txtrIndex]);

	maggieRegs.texture = txtrData;

	if((txtrSize != 5) && (mode & MAG_DRAWMODE_MIPMAP))
	{
		maggieRegs.texSize = txtrSize | 0x0010;
	}
	else
	{
		maggieRegs.texSize = txtrSize;
	}
}

/*****************************************************************************/

/*****************************************************************************/

#define CLIPPED_OUT		0
#define CLIPPED_IN		1
#define CLIPPED_PARTIAL 2

/*****************************************************************************/
/*****************************************************************************/

static float TriangleArea(vec4 *p0, vec4 *p1, vec4 *p2)
{
	float x0 = p1->x - p0->x;
	float y0 = p1->y - p0->y;
	float x1 = p2->x - p0->x;
	float y1 = p2->y - p0->y;

	return x0 * y1 - x1 * y0;
}

/*****************************************************************************/

static UBYTE ClipCode(const vec4 *v)
{
	UBYTE code = 0;

	if(-v->w >= v->x) code |= 0x01;
	if( v->w <= v->x) code |= 0x02;
	if(-v->w >= v->y) code |= 0x04;
	if( v->w <= v->y) code |= 0x08;
	if( 0.0f >= v->z) code |= 0x10;
	if( v->w <= v->z) code |= 0x20;

	return code;
}

/*****************************************************************************/

int ComputeClipCodes(UBYTE *clipCodes, struct MaggieTransVertex *vtx, UWORD nVerts)
{
	UBYTE out = ~0;
	UBYTE in = 0;
	for(int i = 0; i < nVerts; ++i)
	{
		clipCodes[i] = ClipCode(&vtx[i].pos);
		out &= clipCodes[i];
		in |= clipCodes[i];
	}
	if(out)
		return CLIPPED_OUT;
	if(!in)
		return CLIPPED_IN;
	return CLIPPED_PARTIAL;
}

/*****************************************************************************/

void NormaliseVertexBuffer(struct MaggieTransVertex *vtx, int nVerts, UBYTE *clipCodes, MaggieBase *lib)
{
	float offsetScaleX = (lib->xres + 0.5f) * 0.5f;
	float offsetScaleY = (lib->yres + 0.5f) * 0.5f;
	for(int i = 0; i < nVerts; ++i)
	{
		if(clipCodes[i])
			continue;

		float oow = 1.0f / vtx[i].pos.w;

		vtx[i].pos.x = offsetScaleX * (vtx[i].pos.x * oow + 1.0f);
		vtx[i].pos.y = offsetScaleY * (vtx[i].pos.y * oow + 1.0f);
		vtx[i].pos.z = vtx[i].pos.z * oow * 65536.0f * 65535.0f;
		vtx[i].pos.w = oow;
		for(int j = 0; j < MAGGIE_MAX_TEXCOORDS; ++j)
		{
			vtx[i].tex[j].u = vtx[i].tex[j].u * oow;
			vtx[i].tex[j].v = vtx[i].tex[j].v * oow;
		}
	}
}

/*****************************************************************************/

void NormaliseClippedVertexBuffer(struct MaggieTransVertex *vtx, int nVerts, MaggieBase *lib)
{
	float offsetScaleX = (lib->xres + 0.5f) * 0.5f;
	float offsetScaleY = (lib->yres + 0.5f) * 0.5f;
	for(int i = 0; i < nVerts; ++i)
	{
		float oow = 1.0f / vtx[i].pos.w;

		vtx[i].pos.x = offsetScaleX * (vtx[i].pos.x * oow + 1.0f);
		vtx[i].pos.y = offsetScaleY * (vtx[i].pos.y * oow + 1.0f);
		vtx[i].pos.z = vtx[i].pos.z * oow * 65536.0f * 65535.0f;
		vtx[i].pos.w = oow;
		for(int j = 0; j < MAGGIE_MAX_TEXCOORDS; ++j)
		{
			vtx[i].tex[j].u = vtx[i].tex[j].u * oow;
			vtx[i].tex[j].v = vtx[i].tex[j].v * oow;
		}
	}
}

/*****************************************************************************/

int my_abs(int v)
{
	if(v < 0)
		return -v;
	return v;
}

/*****************************************************************************/

void DrawScreenLine(MaggieBase *lib, int x0, int y0, int x1, int y1)
{
	int dx = my_abs(x1 - x0);
	int dy = my_abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;
	int e2;
	ULONG *screen = (ULONG *)lib->screen;
	for(;;)
	{
		if((x0 > lib->scissor.x0) && (x0 < lib->scissor.x1) && (y0 > lib->scissor.y0) && (y0 < lib->scissor.y1))
			screen[y0 * lib->xres + x0] = 0xffffff;

		if((x0 == x1) && (y0 == y1))
			break;

		e2 = 2 * err;
		if(e2 > -dy)
		{
			err -= dy;
			x0 += sx;
		}
		if(e2 < dx)
		{
			err += dx;
			y0 += sy;
		}
	}
}

/*****************************************************************************/

void DrawTriangle(struct MaggieTransVertex *vtx0, struct MaggieTransVertex *vtx1, struct MaggieTransVertex *vtx2, MaggieBase *lib)
{
	float area = TriangleArea(&vtx0->pos, &vtx1->pos, &vtx2->pos);

	if(lib->drawMode & MAG_DRAWMODE_CULL_CCW)
		area = -area;
	if(area > 0.0f)
		return;

	if(lib->frameCounter & 1)
	{
		DrawScreenLine(lib, vtx0->pos.x, vtx0->pos.y, vtx1->pos.x, vtx1->pos.y);
		DrawScreenLine(lib, vtx1->pos.x, vtx1->pos.y, vtx2->pos.x, vtx2->pos.y);
		DrawScreenLine(lib, vtx2->pos.x, vtx2->pos.y, vtx0->pos.x, vtx0->pos.y);
		return;
	}

	int miny = vtx0->pos.y;
	if(miny > vtx1->pos.y)
		miny = vtx1->pos.y;
	if(miny > vtx2->pos.y)
		miny = vtx2->pos.y;

	int maxy = vtx0->pos.y;
	if(maxy < vtx1->pos.y)
		maxy = vtx1->pos.y;
	if(maxy < vtx2->pos.y)
		maxy = vtx2->pos.y;

	DrawEdge(vtx0, vtx1, lib);
	DrawEdge(vtx1, vtx2, lib);
	DrawEdge(vtx2, vtx0, lib);
	DrawSpans(miny, maxy, lib);
}

/*****************************************************************************/

void DrawPolygon(struct MaggieTransVertex *vtx, int nVerts, MaggieBase *lib)
{
	if(nVerts < 3)
		return;

	float area = TriangleArea(&vtx[0].pos, &vtx[1].pos, &vtx[2].pos);
	for(int i = 2; i < nVerts - 1; ++i)
	{
		area += TriangleArea(&vtx[0].pos, &vtx[i].pos, &vtx[i + 1].pos);
	}

	if(lib->drawMode & MAG_DRAWMODE_CULL_CCW)
		area = -area;
	if(area >= 0.0f)
		return;

	if(lib->frameCounter & 1)
	{
		for(int i = 0; i < nVerts; ++i)
		{
			DrawScreenLine(lib, vtx[i].pos.x, vtx[i].pos.y, vtx[(i + 1) % nVerts].pos.x, vtx[(i + 1) % nVerts].pos.y);
		}
		return;
	}
	int miny = vtx[0].pos.y;
	int maxy = vtx[0].pos.y;
	for(int i = 1; i < nVerts; ++i)
	{
		if(miny > vtx[i].pos.y)
		{
			miny = vtx[i].pos.y;
		}
		if(maxy < vtx[i].pos.y)
		{
			maxy = vtx[i].pos.y;
		}
	}
	int prev = nVerts - 1;
	for(int i = 0; i < nVerts; ++i)
	{
		DrawEdge(&vtx[prev], &vtx[i], lib);	
		prev = i;
	}
	DrawSpans(miny, maxy, lib);
}

/*****************************************************************************/

void DrawIndexedPolygon(struct MaggieTransVertex *vtx, UWORD *indx, int nIndx, MaggieBase *lib)
{
	if(nIndx < 3)
		return;

	float area = TriangleArea(&vtx[indx[0]].pos, &vtx[indx[1]].pos, &vtx[indx[2]].pos);
	for(int i = 2; i < nIndx - 1; ++i)
	{
		area += TriangleArea(&vtx[indx[0]].pos, &vtx[indx[i]].pos, &vtx[indx[i + 1]].pos);
	}
	if(lib->drawMode & MAG_DRAWMODE_CULL_CCW)
		area = -area;
	if(area >= 0.0f)
		return;

	if(lib->frameCounter & 1)
	{
		for(int i = 0; i < nIndx; ++i)
		{
			DrawScreenLine(lib, vtx[indx[i]].pos.x, vtx[indx[i]].pos.y, vtx[indx[(i + 1) % nIndx]].pos.x, vtx[indx[(i + 1) % nIndx]].pos.y);
		}
		return;
	}
	int miny = vtx[indx[0]].pos.y;
	int maxy = vtx[indx[0]].pos.y;

	for(int i = 1; i < nIndx; ++i)
	{
		if(miny > vtx[indx[i]].pos.y)
		{
			miny = vtx[indx[i]].pos.y;
		}
		if(maxy < vtx[indx[i]].pos.y)
		{
			maxy = vtx[indx[i]].pos.y;
		}
	}
	int prev = nIndx - 1;
	for(int i = 0; i < nIndx; ++i)
	{
		DrawEdge(&vtx[indx[prev]], &vtx[indx[i]], lib);	
		prev = i;
	}
	DrawSpans(miny, maxy, lib);
}

/*****************************************************************************/

static struct MaggieVertex vtxBufferUP[65536];
static struct MaggieSpriteVertex spriteBufferUP[65536];
static struct MaggieTransVertex transVtxBufferUP[65536];
static UBYTE transClipCodesUP[65536];
static struct MaggieTransVertex clippedPoly[MAG_MAX_POLYSIZE + 8];

/*****************************************************************************/

void magDrawTrianglesUP(REG(a0, struct MaggieVertex *vtx), REG(d0, UWORD nVerts), REG(a6, MaggieBase *lib))
{
	for(int i = 0; i < nVerts; ++i)
	{
		vtxBufferUP[i] = vtx[i];
	}

	PrepareVertexBuffer(transVtxBufferUP, vtxBufferUP, nVerts);

	TransformVertexBuffer(transVtxBufferUP, vtxBufferUP, nVerts, lib);

	if(lib->drawMode & MAG_DRAWMODE_LIGHTING)
		LightBuffer(transVtxBufferUP, vtxBufferUP, nVerts, lib);

	TexGenBuffer(transVtxBufferUP, vtxBufferUP, nVerts, lib);

	int clipRes = ComputeClipCodes(transClipCodesUP, transVtxBufferUP, nVerts);

	if(clipRes == CLIPPED_OUT)
		return;

	struct GfxBase *GfxBase = lib->gfxBase;

	OwnBlitter();

	SetupHW(lib);

	if(clipRes == CLIPPED_IN)
	{
		NormaliseClippedVertexBuffer(transVtxBufferUP, nVerts, lib);

		for(int i = 0; i < nVerts; i += 3)
		{
			DrawTriangle(&transVtxBufferUP[i + 0], &transVtxBufferUP[i + 1], &transVtxBufferUP[i + 2], lib);
		}
	}
	else if(clipRes == CLIPPED_PARTIAL)
	{
		for(int i = 0; i < nVerts; i += 3)
		{
			if(transClipCodesUP[i + 0] | transClipCodesUP[i + 1] | transClipCodesUP[i + 2])
			{
				if(!(transClipCodesUP[i + 0] & transClipCodesUP[i + 1] & transClipCodesUP[i + 2]))
				{
					clippedPoly[0] = transVtxBufferUP[i + 0];
					clippedPoly[1] = transVtxBufferUP[i + 1];
					clippedPoly[2] = transVtxBufferUP[i + 2];
					int nClippedVerts = ClipPolygon(clippedPoly, 3);
					if(nClippedVerts > 2)
					{
						NormaliseClippedVertexBuffer(clippedPoly, nClippedVerts, lib);
						DrawPolygon(clippedPoly, nClippedVerts, lib);
					}
				}
			}
			else
			{
				NormaliseClippedVertexBuffer(&transVtxBufferUP[i], 3, lib);
				DrawTriangle(&transVtxBufferUP[i + 0], &transVtxBufferUP[i + 1], &transVtxBufferUP[i + 2], lib);
			}
		}
	}
	DisownBlitter();
}

/*****************************************************************************/

void magDrawIndexedTrianglesUP(REG(a0, struct MaggieVertex *vtx), REG(d0, UWORD nVerts), REG(a1, UWORD *indx), REG(d1, UWORD nIndx), REG(a6, MaggieBase *lib))
{
	for(int i = 0; i < nVerts; ++i)
	{
		vtxBufferUP[i] = vtx[i];
	}

	PrepareVertexBuffer(transVtxBufferUP, vtxBufferUP, nVerts);

	TransformVertexBuffer(transVtxBufferUP, vtxBufferUP, nVerts, lib);

	if(lib->drawMode & MAG_DRAWMODE_LIGHTING)
		LightBuffer(transVtxBufferUP, vtxBufferUP, nVerts, lib);

	TexGenBuffer(transVtxBufferUP, vtxBufferUP, nVerts, lib);

	int clipRes = ComputeClipCodes(transClipCodesUP, transVtxBufferUP, nVerts);

	if(clipRes == CLIPPED_OUT)
		return;

	struct GfxBase *GfxBase = lib->gfxBase;

	OwnBlitter();

	SetupHW(lib);

	if(clipRes == CLIPPED_IN)
	{
		NormaliseClippedVertexBuffer(transVtxBufferUP, nVerts, lib);
		for(int i = 0; i < nIndx; i += 3)
		{
			int i0 = indx[i + 0];
			int i1 = indx[i + 1];
			int i2 = indx[i + 2];
			DrawTriangle(&transVtxBufferUP[i0], &transVtxBufferUP[i1], &transVtxBufferUP[i2], lib);
		}
	}
	if(clipRes == CLIPPED_PARTIAL)
	{
		for(int i = 0; i < nIndx; i += 3)
		{
			int i0 = indx[i + 0];
			int i1 = indx[i + 1];
			int i2 = indx[i + 2];

			if(transClipCodesUP[i0] | transClipCodesUP[i1] | transClipCodesUP[i2])
			{
				if(!(transClipCodesUP[i0] & transClipCodesUP[i1] & transClipCodesUP[i2]))
				{
					clippedPoly[0] = transVtxBufferUP[i0];
					clippedPoly[1] = transVtxBufferUP[i1];
					clippedPoly[2] = transVtxBufferUP[i2];
					int nClippedVerts = ClipPolygon(clippedPoly, 3);
					if(nClippedVerts > 2)
					{
						NormaliseClippedVertexBuffer(clippedPoly, nClippedVerts, lib);
						DrawPolygon(clippedPoly, nClippedVerts, lib);
					}
				}
			}
			else
			{
				clippedPoly[0] = transVtxBufferUP[i0];
				clippedPoly[1] = transVtxBufferUP[i1];
				clippedPoly[2] = transVtxBufferUP[i2];
				NormaliseClippedVertexBuffer(clippedPoly, 3, lib);
				DrawTriangle(&clippedPoly[0], &clippedPoly[1], &clippedPoly[2], lib);
			}
		}
	}
	DisownBlitter();
}

/*****************************************************************************/

void magDrawIndexedPolygonsUP(REG(a0, struct MaggieVertex *vtx), REG(d0, UWORD nVerts), REG(a1, UWORD *indx), REG(d1, UWORD nIndx), REG(a6, MaggieBase *lib))
{
	for(int i = 0; i < nVerts; ++i)
	{
		vtxBufferUP[i] = vtx[i];
	}
	PrepareVertexBuffer(transVtxBufferUP, vtxBufferUP, nVerts);

	TransformVertexBuffer(transVtxBufferUP, vtxBufferUP, nVerts, lib);

	if(lib->drawMode & MAG_DRAWMODE_LIGHTING)
		LightBuffer(transVtxBufferUP, vtxBufferUP, nVerts, lib);

	TexGenBuffer(transVtxBufferUP, vtxBufferUP, nVerts, lib);

	int clipRes = ComputeClipCodes(transClipCodesUP, transVtxBufferUP, nVerts);

	if(clipRes == CLIPPED_OUT)
		return;

	struct GfxBase *GfxBase = lib->gfxBase;

	OwnBlitter();

	SetupHW(lib);

	if(clipRes == CLIPPED_IN)
	{
		NormaliseClippedVertexBuffer(transVtxBufferUP, nVerts, lib);
		int indxPos = 0;
		while(indxPos < nIndx)
		{
			int nPolyVerts = 0;
			for(int i = indxPos; i < nIndx; ++i)
			{
				if(indx[i] == 0xffff)
					break;

				nPolyVerts++;
			}
			if(nPolyVerts >= 3)
			{
				DrawIndexedPolygon(transVtxBufferUP, &indx[indxPos], nPolyVerts, lib);
			}
			indxPos += nPolyVerts + 1;
		}
	}
	if(clipRes == CLIPPED_PARTIAL)
	{
		int indxPos = 0;

		while(indxPos < nIndx)
		{
			int clippedAll = ~0;
			int clippedAny = 0;
			int nPolyVerts = 0;
			for(int i = indxPos; i < nIndx; ++i)
			{
				if(indx[i] == 0xffff)
				{
					break;
				}
				nPolyVerts++;
				clippedAll &= transClipCodesUP[indx[i]];
				clippedAny |= transClipCodesUP[indx[i]];
			}
			if(clippedAll | (nPolyVerts < 3))
			{
				indxPos += nPolyVerts + 1;
				continue;
			}

			for(int i = 0; i < nPolyVerts; ++i)
			{
				clippedPoly[i] = transVtxBufferUP[indx[i + indxPos]];
			}
			indxPos += nPolyVerts + 1;
			if(clippedAny)
			{
				int nClippedVerts = ClipPolygon(clippedPoly, nPolyVerts);
				if(nClippedVerts > 2)
				{
					NormaliseClippedVertexBuffer(clippedPoly, nClippedVerts, lib);
					DrawPolygon(clippedPoly, nClippedVerts, lib);
				}
			}
			else
			{
				NormaliseClippedVertexBuffer(clippedPoly, nPolyVerts, lib);
				DrawPolygon(clippedPoly, nPolyVerts, lib);
			}
		}
	}
	DisownBlitter();
}

/*****************************************************************************/

void FlushImmediateMode(MaggieBase *lib)
{
	if(lib->immModeVtx == 0xffff)
		return;
	if(lib->nIModeVtx >= 3)
	{
		UWORD oldVBuffer = lib->vBuffer;
		lib->vBuffer = lib->immModeVtx;

		magDrawTriangles(0, lib->nIModeVtx, lib);

		lib->vBuffer = oldVBuffer;
	}
	lib->nIModeVtx = 0;
}

/*****************************************************************************/

void magDrawTriangles(REG(d0, UWORD startVtx), REG(d1, UWORD nVerts), REG(a6, MaggieBase *lib))
{
	struct MaggieVertex *vtx = GetVBVertices(lib->vertexBuffers[lib->vBuffer]) + startVtx;
	struct MaggieTransVertex *transVtx = GetVBTransVertices(lib->vertexBuffers[lib->vBuffer]) + startVtx;
	UBYTE *clipCodes = GetVBClipCodes(lib->vertexBuffers[lib->vBuffer]) + startVtx;

	TransformVertexBuffer(transVtx, vtx, nVerts, lib);

	int clipRes = ComputeClipCodes(clipCodes, transVtx, nVerts);

	if(clipRes == CLIPPED_OUT)
		return;

	if(lib->drawMode & MAG_DRAWMODE_LIGHTING)
		LightBuffer(transVtx, vtx, nVerts, lib);

	TexGenBuffer(transVtx, vtx, nVerts, lib);

	struct GfxBase *GfxBase = lib->gfxBase;
	OwnBlitter();

	SetupHW(lib);

	if(clipRes == CLIPPED_IN)
	{
		NormaliseClippedVertexBuffer(transVtx, nVerts, lib);

		for(int i = 0; i < nVerts; i += 3)
		{
			DrawTriangle(&transVtx[i + 0], &transVtx[i + 1], &transVtx[i + 2], lib);
		}
	}
	else if(clipRes == CLIPPED_PARTIAL)
	{
		for(int i = 0; i < nVerts; i += 3)
		{
			if(clipCodes[i + 0] | clipCodes[i + 1] | clipCodes[i + 2])
			{
				if(!(clipCodes[i + 0] & clipCodes[i + 1] & clipCodes[i + 2]))
				{
					clippedPoly[0] = transVtx[i + 0];
					clippedPoly[1] = transVtx[i + 1];
					clippedPoly[2] = transVtx[i + 2];
					int nClippedVerts = ClipPolygon(clippedPoly, 3);
					if(nClippedVerts > 2)
					{
						NormaliseClippedVertexBuffer(clippedPoly, nClippedVerts, lib);
						DrawPolygon(clippedPoly, nClippedVerts, lib);
					}
				}
			}
			else
			{
				NormaliseClippedVertexBuffer(&transVtx[i], 3, lib);
				DrawTriangle(&transVtx[i + 0], &transVtx[i + 1], &transVtx[i + 2], lib);
			}
		}
	}
	DisownBlitter();
}

/*****************************************************************************/

void magDrawIndexedTriangles(REG(d0, UWORD startVtx), REG(d1, UWORD nVerts), REG(d2, UWORD startIndx), REG(d3, UWORD nIndx), REG(a6, MaggieBase *lib))
{
	SetupHW(lib);

	UWORD *indexBuffer = GetIBIndices(lib->indexBuffers[lib->iBuffer]) + startIndx;
	struct MaggieVertex *vtx = GetVBVertices(lib->vertexBuffers[lib->vBuffer]);
	struct MaggieTransVertex *transVtx = GetVBTransVertices(lib->vertexBuffers[lib->vBuffer]);
	UBYTE *clipCodes = GetVBClipCodes(lib->vertexBuffers[lib->vBuffer]);

	TransformVertexBuffer(&transVtx[startVtx], &vtx[startVtx], nVerts, lib);

	if(lib->drawMode & MAG_DRAWMODE_LIGHTING)
		LightBuffer(&transVtx[startVtx], &vtx[startVtx], nVerts, lib);

	TexGenBuffer(&transVtx[startVtx], &vtx[startVtx], nVerts, lib);

	int clipRes = ComputeClipCodes(&clipCodes[startVtx], &transVtx[startVtx], nVerts);

	if(clipRes == CLIPPED_OUT)
		return;

	struct GfxBase *GfxBase = lib->gfxBase;
	OwnBlitter();

	if(clipRes == CLIPPED_IN)
	{
		NormaliseClippedVertexBuffer(&transVtx[startVtx], nVerts, lib);
		for(int i = 0; i < nIndx; i += 3)
		{
			int i0 = indexBuffer[i + 0];
			int i1 = indexBuffer[i + 1];
			int i2 = indexBuffer[i + 2];
			DrawTriangle(&transVtx[i0], &transVtx[i1], &transVtx[i2], lib);
		}
	}
	if(clipRes == CLIPPED_PARTIAL)
	{
		for(int i = 0; i < nIndx; i += 3)
		{
			int i0 = indexBuffer[i + 0];
			int i1 = indexBuffer[i + 1];
			int i2 = indexBuffer[i + 2];

			if(!(clipCodes[i0] & clipCodes[i1] & clipCodes[i2]))
			{
				clippedPoly[0] = transVtx[i0];
				clippedPoly[1] = transVtx[i1];
				clippedPoly[2] = transVtx[i2];
				if(clipCodes[i0] | clipCodes[i1] | clipCodes[i2])
				{
					int nClippedVerts = ClipPolygon(clippedPoly, 3);
					if(nClippedVerts > 2)
					{
						NormaliseClippedVertexBuffer(clippedPoly, nClippedVerts, lib);
						DrawPolygon(clippedPoly, nClippedVerts, lib);
					}
				}
				else
				{
					NormaliseClippedVertexBuffer(clippedPoly, 3, lib);
					DrawTriangle(&clippedPoly[0], &clippedPoly[1], &clippedPoly[2], lib);
				}
			}
		}
	}
	DisownBlitter();
}

/*****************************************************************************/

void magDrawIndexedPolygons(REG(d0, UWORD startVtx), REG(d1, UWORD nVerts), REG(d2, UWORD startIndx), REG(d3, UWORD nIndx), REG(a6, MaggieBase *lib))
{
#if PROFILE
	ULONG drawStart = GetClocks();
#endif
	UWORD *indexBuffer = GetIBIndices(lib->indexBuffers[lib->iBuffer]) + startIndx;
	struct MaggieVertex *vtx = GetVBVertices(lib->vertexBuffers[lib->vBuffer]);
	struct MaggieTransVertex *transVtx = GetVBTransVertices(lib->vertexBuffers[lib->vBuffer]);
	UBYTE *clipCodes = GetVBClipCodes(lib->vertexBuffers[lib->vBuffer]);

	TransformVertexBuffer(&transVtx[startVtx], &vtx[startVtx], nVerts, lib);

	int clipRes = ComputeClipCodes(&clipCodes[startVtx], &transVtx[startVtx], nVerts);

	if(clipRes == CLIPPED_OUT)
	{
		return;
	}

	if(lib->drawMode & MAG_DRAWMODE_LIGHTING)
	{
		LightBuffer(&transVtx[startVtx], &vtx[startVtx], nVerts, lib);
	}

	if(lib->txtrIndex != 0xffff)
	{
		TexGenBuffer(&transVtx[startVtx], &vtx[startVtx], nVerts, lib);
	}

	struct GfxBase *GfxBase = lib->gfxBase;

	OwnBlitter();

	SetupHW(lib);

	if(clipRes == CLIPPED_IN)
	{
		NormaliseClippedVertexBuffer(&transVtx[startVtx], nVerts, lib);
		int indxPos = 0;
		while(indxPos < nIndx)
		{
			int nPolyVerts = 0;
			for(int i = indxPos; i < nIndx; ++i)
			{
				if(indexBuffer[i] == 0xffff)
					break;

				nPolyVerts++;
			}
			if(nPolyVerts >= 3)
			{
				DrawIndexedPolygon(transVtx, &indexBuffer[indxPos], nPolyVerts, lib);
			}
			indxPos += nPolyVerts + 1;
		}
	}
	if(clipRes == CLIPPED_PARTIAL)
	{
		int indxPos = 0;

		while(indxPos < nIndx)
		{
			int clippedAll = ~0;
			int clippedAny = 0;
			int nPolyVerts = 0;
			for(int i = indxPos; i < nIndx; ++i)
			{
				if(indexBuffer[i] == 0xffff)
				{
					break;
				}
				nPolyVerts++;
				clippedAll &= clipCodes[indexBuffer[i]];
				clippedAny |= clipCodes[indexBuffer[i]];
			}
			if(clippedAll || (nPolyVerts < 3))
			{
				indxPos += nPolyVerts + 1;
				continue;
			}
			for(int i = 0; i < nPolyVerts; ++i)
			{
				clippedPoly[i] = transVtx[indexBuffer[i + indxPos]];
			}
			indxPos += nPolyVerts + 1;
			if(clippedAny)
			{
				nPolyVerts = ClipPolygon(clippedPoly, nPolyVerts);
				if(nPolyVerts > 2)
				{
					NormaliseClippedVertexBuffer(clippedPoly, nPolyVerts, lib);
					DrawPolygon(clippedPoly, nPolyVerts, lib);
				}
			}
			else
			{
				NormaliseClippedVertexBuffer(clippedPoly, nPolyVerts, lib);
				DrawPolygon(clippedPoly, nPolyVerts, lib);
			}
		}
	}
	DisownBlitter();
#if PROFILE
//	lib->profile.draw += GetClocks() - drawStart;
#endif
}

/*****************************************************************************/

void magDrawLinearSpan(REG(a0, struct SpanPosition *start), REG(a1, struct SpanPosition *end), REG(a6, MaggieBase *lib))
{
	SetupHW(lib);
}

/*****************************************************************************/

void magDrawSpan(REG(a0, struct MaggieClippedVertex *start), REG(a1, struct MaggieClippedVertex *end), REG(a6, MaggieBase *lib))
{
	SetupHW(lib);
}

/*****************************************************************************/

void ExpandSpriteBuffer(struct MaggieTransVertex *dest, struct MaggieSpriteVertex *srcVtx, int nSprites, float spriteSize, MaggieBase *lib)
{
	for(int i = 0; i < nSprites; ++i)
	{
		vec3 p0 = srcVtx[i].pos; p0.x += spriteSize; p0.y += spriteSize;
		vec3 p1 = srcVtx[i].pos; p1.x += spriteSize; p1.y -= spriteSize;
		vec3 p2 = srcVtx[i].pos; p2.x -= spriteSize; p2.y -= spriteSize;
		vec3 p3 = srcVtx[i].pos; p3.x -= spriteSize; p3.y += spriteSize;
		vec3_tformh(&dest[i * 4 + 0].pos, &lib->perspectiveMatrix, &p0, 1.0f);
		dest[i * 4 + 0].tex[0].u = 0.0f * 256.0f * 65536.0f;
		dest[i * 4 + 0].tex[0].v = 0.0f * 256.0f * 65536.0f;
		dest[i * 4 + 0].tex[0].w = 1.0f;
		dest[i * 4 + 0].colour = srcVtx[i].colour;
		vec3_tformh(&dest[i * 4 + 1].pos, &lib->perspectiveMatrix, &p1, 1.0f);
		dest[i * 4 + 1].tex[0].u = 0.0f * 256.0f * 65536.0f;
		dest[i * 4 + 1].tex[0].v = 1.0f * 256.0f * 65536.0f;
		dest[i * 4 + 1].tex[0].w = 1.0f;
		dest[i * 4 + 1].colour = srcVtx[i].colour;
		vec3_tformh(&dest[i * 4 + 2].pos, &lib->perspectiveMatrix, &p2, 1.0f);
		dest[i * 4 + 2].tex[0].u = 1.0f * 256.0f * 65536.0f;
		dest[i * 4 + 2].tex[0].v = 1.0f * 256.0f * 65536.0f;
		dest[i * 4 + 2].tex[0].w = 1.0f;
		dest[i * 4 + 2].colour = srcVtx[i].colour;
		vec3_tformh(&dest[i * 4 + 3].pos, &lib->perspectiveMatrix, &p3, 1.0f);
		dest[i * 4 + 3].tex[0].u = 1.0f * 256.0f * 65536.0f;
		dest[i * 4 + 3].tex[0].v = 0.0f * 256.0f * 65536.0f;
		dest[i * 4 + 3].tex[0].w = 1.0f;
		dest[i * 4 + 3].colour = srcVtx[i].colour;
	}
}

/*****************************************************************************/

void magDrawSprites(REG(d0, UWORD startVtx), REG(d1, UWORD nSprites), REG(fp0, float spriteSize), REG(a6, MaggieBase *lib))
{
	struct MaggieVertex *vtx = GetVBVertices(lib->vertexBuffers[lib->vBuffer]);
	TransformToSpriteBuffer(spriteBufferUP, &vtx[startVtx], nSprites, lib);
	ExpandSpriteBuffer(transVtxBufferUP, spriteBufferUP, nSprites, spriteSize * 0.5f, lib);
	int clipRes = ComputeClipCodes(transClipCodesUP, transVtxBufferUP, nSprites * 4);

	if(clipRes == CLIPPED_OUT)
	{
		return;
	}
	struct GfxBase *GfxBase = lib->gfxBase;

	OwnBlitter();

	SetupHW(lib);

	if(clipRes == CLIPPED_IN)
	{
		NormaliseClippedVertexBuffer(transVtxBufferUP, nSprites * 4, lib);
		for(int i = 0; i < nSprites; ++i)
		{
			DrawPolygon(&transVtxBufferUP[i * 4], 4, lib);
		}
	}
	else
	{
		for(int i = 0; i < nSprites; ++i)
		{
			clippedPoly[0] = transVtxBufferUP[i * 4 + 0];
			clippedPoly[1] = transVtxBufferUP[i * 4 + 1];
			clippedPoly[2] = transVtxBufferUP[i * 4 + 2];
			clippedPoly[3] = transVtxBufferUP[i * 4 + 3];
			int nClippedVerts = ClipPolygon(clippedPoly, 4);
			if(nClippedVerts > 2)
			{
				NormaliseClippedVertexBuffer(clippedPoly, nClippedVerts, lib);
				DrawPolygon(clippedPoly, nClippedVerts, lib);
			}
		}
	}
	DisownBlitter();
}

/*****************************************************************************/

void magDrawSpritesUP(REG(a0, struct MaggieSpriteVertex *vtx), REG(d0, UWORD nSprites), REG(fp0, float spriteSize), REG(a6, MaggieBase *lib))
{

	TransformSpriteBuffer(spriteBufferUP, vtx, nSprites, lib);
	ExpandSpriteBuffer(transVtxBufferUP, spriteBufferUP, nSprites, spriteSize * 0.5f, lib);
	int clipRes = ComputeClipCodes(transClipCodesUP, transVtxBufferUP, nSprites * 4);

	if(clipRes == CLIPPED_OUT)
	{
		return;
	}
	struct GfxBase *GfxBase = lib->gfxBase;

	OwnBlitter();

	SetupHW(lib);

	if(clipRes == CLIPPED_IN)
	{
		NormaliseClippedVertexBuffer(transVtxBufferUP, nSprites * 4, lib);
		for(int i = 0; i < nSprites; ++i)
		{
			DrawPolygon(&transVtxBufferUP[i * 4], 4, lib);
		}
	}
	else
	{
		for(int i = 0; i < nSprites; ++i)
		{
			clippedPoly[0] = transVtxBufferUP[i * 4 + 0];
			clippedPoly[1] = transVtxBufferUP[i * 4 + 1];
			clippedPoly[2] = transVtxBufferUP[i * 4 + 2];
			clippedPoly[3] = transVtxBufferUP[i * 4 + 3];
			int nClippedVerts = ClipPolygon(clippedPoly, 4);
			if(nClippedVerts > 2)
			{
				NormaliseClippedVertexBuffer(clippedPoly, nClippedVerts, lib);
				DrawPolygon(clippedPoly, nClippedVerts, lib);
			}
		}
	}
	DisownBlitter();
}

/*****************************************************************************/
