#include "maggie_internal.h"
#include "maggie_debug.h"
#include <proto/graphics.h>
#include <string.h>

void magSetScreenMemory(REG(a0, APTR *pixels), REG(d0, UWORD xres), REG(d1, UWORD yres), REG(a6, MaggieBase *lib))
{
	lib->xres = xres;
	lib->yres = yres;
	lib->screen = pixels;

	lib->scissor.x0 = 0;
	lib->scissor.y0 = 0;
	lib->scissor.x1 = xres;
	lib->scissor.y1 = yres;

	lib->dirtyMatrix = 1;
}

/*****************************************************************************/

void magSetTexture(REG(d0, UWORD unit), REG(d1, UWORD txtr), REG(a6, MaggieBase *lib))
{
	lib->txtrIndex = txtr;
}

/*****************************************************************************/

void magSetDrawMode(REG(d0, UWORD mode), REG(a6, MaggieBase *lib))
{
	lib->drawMode = mode;
}

/*****************************************************************************/

void magSetRGB(REG(d0, ULONG rgb), REG(a6, MaggieBase *lib))
{
	lib->colour = rgb;
}

/*****************************************************************************/

UWORD *magGetDepthBuffer(REG(a6, MaggieBase *lib))
{
	return lib->depthBuffer;
}

/*****************************************************************************/

void magSetVertexBuffer(REG(d0, WORD vBuffer), REG(a6, MaggieBase *lib))
{
	lib->vBuffer = vBuffer;
}

/*****************************************************************************/

void magSetIndexBuffer(REG(d0, WORD iBuffer), REG(a6, MaggieBase *lib))
{
	lib->iBuffer = iBuffer;
}

/*****************************************************************************/
/*****************************************************************************/

ULONG ColourTo16Bit(ULONG colour)
{
	return ((colour >> 8) & 0xf800) | ((colour >> 1) & 0x07e0)  | ((colour >> 3) & 0x001f);
}

/*****************************************************************************/

UWORD GetVBNumVerts(ULONG *mem)
{
	return mem[0];
}

/*****************************************************************************/

struct MaggieVertex *GetVBVertices(ULONG *mem)
{
	return (struct MaggieVertex *)&mem[2];
}

/*****************************************************************************/

UBYTE *GetVBClipCodes(ULONG *mem)
{
	return ((UBYTE *)&mem[2]) + sizeof(struct MaggieVertex) * GetVBNumVerts(mem);
}

/*****************************************************************************/

struct MaggieTransVertex *GetVBTransVertices(ULONG *mem)
{
	return (struct MaggieTransVertex *)(((UBYTE *)&mem[2]) + (sizeof(struct MaggieVertex) + 1) * GetVBNumVerts(mem));
}

/*****************************************************************************/

UWORD magAllocateVertexBuffer(REG(d0, UWORD nVerts), REG(a6, MaggieBase *lib))
{
	UWORD vBuffer = 0xffff;

	struct ExecBase *SysBase = lib->sysBase;

	ObtainSemaphore(&lib->lock);

	for(int i = 0; i < MAX_VERTEX_BUFFERS; ++i)
	{
		if(!lib->vertexBuffers[i])
		{
			vBuffer = i;
			lib->vertexBuffers[i] = (ULONG *)1;
			break;
		}
	}

	ReleaseSemaphore(&lib->lock);

	if(vBuffer == 0xffff)
		return 0xffff;
	ULONG memSize = (sizeof(struct MaggieVertex) + sizeof(struct MaggieTransVertex) + sizeof(UBYTE)) * nVerts + sizeof(ULONG) * 2;
	ULONG *mem = (ULONG *)AllocMem(memSize, MEMF_ANY | MEMF_CLEAR);
	mem[0] = nVerts;
	mem[1] = memSize;
	lib->vertexBuffers[vBuffer] = mem;

	return vBuffer;
}

/*****************************************************************************/

void magUploadVertexBuffer(REG(d0, UWORD vBuffer), REG(a0, struct MaggieVertex *vtx), REG(d1, UWORD startVtx), REG(d2, UWORD nVerts), REG(a6, MaggieBase *lib))
{
	ULONG *mem = lib->vertexBuffers[vBuffer];
	if(!mem)
		return;

	if(startVtx + nVerts > GetVBNumVerts(mem))
	{
		nVerts = GetVBNumVerts(mem) - startVtx;
	}

	struct MaggieVertex *dst = GetVBVertices(mem);
	for(int i = 0; i < nVerts; ++i)
	{
		dst[i + startVtx] = vtx[startVtx + i];
	}
	struct MaggieTransVertex *transDst = GetVBTransVertices(mem);
	PrepareVertexBuffer(&transDst[startVtx], &dst[startVtx], nVerts);
}

/*****************************************************************************/

void magFreeVertexBuffer(REG(d0, UWORD vBuffer), REG(a6, MaggieBase *lib))
{
	if(vBuffer >= MAX_VERTEX_BUFFERS)
		return;

	ULONG *mem = lib->vertexBuffers[vBuffer];

	if(!mem)
		return;

	ULONG size = mem[1];

	struct ExecBase *SysBase = lib->sysBase;

	FreeMem(mem, size);

	lib->vertexBuffers[vBuffer] = NULL;
}

/*****************************************************************************/

UWORD GetIBNumIndices(ULONG *mem)
{
	return mem[0];
}

/*****************************************************************************/

UWORD *GetIBIndices(ULONG *mem)
{
	return (UWORD *)(&mem[2]);
}

/*****************************************************************************/

UWORD magAllocateIndexBuffer(REG(d0, UWORD nIndx), REG(a6, MaggieBase *lib))
{
	UWORD iBuffer = 0xffff;

	struct ExecBase *SysBase = lib->sysBase;

	ObtainSemaphore(&lib->lock);

	for(int i = 0; i < MAX_INDEX_BUFFERS; ++i)
	{
		if(!lib->indexBuffers[i])
		{
			iBuffer = i;
			lib->indexBuffers[i] = (ULONG *)1;
			break;
		}
	}

	ReleaseSemaphore(&lib->lock);

	if(iBuffer == 0xffff)
		return 0xffff;

	ULONG *mem = (ULONG *)AllocMem(sizeof(UWORD) * nIndx + sizeof(ULONG) * 2, MEMF_ANY | MEMF_CLEAR);
	mem[0] = nIndx;
	mem[1] = sizeof(UWORD) * nIndx + sizeof(ULONG) * 2;
	lib->indexBuffers[iBuffer] = mem;

	return iBuffer;
}

/*****************************************************************************/

void magUploadIndexBuffer(REG(d0, UWORD iBuffer), REG(a0, UWORD *indx), REG(d1, UWORD startIndx), REG(d2, UWORD nIndx), REG(a6, MaggieBase *lib))
{
	ULONG *mem = lib->indexBuffers[iBuffer];
	if(!mem)
		return;

	if(startIndx + nIndx > mem[0])
	{
		nIndx = mem[0] - startIndx;
	}

	UWORD *dst = (UWORD *)&mem[2];
	for(int i = 0; i < nIndx; ++i)
	{
		dst[i] = indx[startIndx + i];
	}
}

/*****************************************************************************/

void magFreeIndexBuffer(REG(d0, UWORD iBuffer), REG(a6, MaggieBase *lib))
{
	if(iBuffer >= MAX_INDEX_BUFFERS)
		return;

	ULONG *mem = lib->indexBuffers[iBuffer];

	if(!mem)
		return;

	struct ExecBase *SysBase = lib->sysBase;

	ULONG size = mem[1];
	FreeMem(mem, size);

	lib->indexBuffers[iBuffer] = NULL;
}

/*****************************************************************************/

// Library Lock..
void magBeginScene(REG(a6, MaggieBase *lib))
{
	struct ExecBase *SysBase = lib->sysBase;
	ObtainSemaphore(&lib->lock);
	for(int i = 0; i < MAG_MAX_LIGHTS; ++i)
	{
		lib->lights[i].type = MAG_LIGHT_OFF;
	}
	DebugReset();
	lib->colour = 0x00ffffff;
#if PROFILE
	memset(&lib->profile, 0, sizeof(lib->profile));
	lib->profile.frame = GetClocks();
#endif
	lib->frameCounter = lib->drawMode & 0x8000 ? 1 : 0;
}

/*****************************************************************************/

void magEndScene(REG(a6, MaggieBase *lib))
{
	struct ExecBase *SysBase = lib->sysBase;
#if PROFILE
	lib->profile.frame = GetClocks() - lib->profile.frame;
	if(lib->profile.frame)
	{
		TextOut(lib, "Frame  : %d", lib->profile.frame);
		TextOut(lib, "Lines  : %d - %d%%", lib->profile.lines, lib->profile.lines * 100 / lib->profile.frame);
		TextOut(lib, "Spans  : %d - %d%%", lib->profile.spans, lib->profile.spans * 100 / lib->profile.frame);
		TextOut(lib, "Trans  : %d - %d%%", lib->profile.trans, lib->profile.trans * 100 / lib->profile.frame);
		TextOut(lib, "Clear  : %d - %d%%", lib->profile.clear, lib->profile.clear * 100 / lib->profile.frame);
		TextOut(lib, "Light  : %d - %d%%", lib->profile.light, lib->profile.light * 100 / lib->profile.frame);
		TextOut(lib, "Draw   : %d - %d%%", lib->profile.draw, lib->profile.draw * 100 / lib->profile.frame);
		TextOut(lib, "TexGen : %d - %d%%", lib->profile.texgen, lib->profile.texgen * 100 / lib->profile.frame);
		if(lib->profile.nLinePixels)
			TextOut(lib, "Line time per pixel %d (%d)", lib->profile.lines / lib->profile.nLinePixels, lib->profile.nLinePixels);
		if(lib->profile.nPixels)
			TextOut(lib, "Span time per pixel %d (%d)", lib->profile.spans / lib->profile.nPixels, lib->profile.nPixels);
	}
#endif
	ReleaseSemaphore(&lib->lock);
}

/*****************************************************************************/
/*****************************************************************************/

// Immediate mode, or "slow mode"..
void magBegin(REG(a6, MaggieBase *lib))
{
	lib->nIModeVtx = 0;
	if(lib->immModeVtx == 0xffff)
	{
		lib->immModeVtx = magAllocateVertexBuffer(IMM_MODE_MAGGIE_VERTS, lib);
	}
}

/*****************************************************************************/

void magEnd(REG(a6, MaggieBase *lib))
{
	if(lib->nIModeVtx)
	{
		FlushImmediateMode(lib);
	}
}

/*****************************************************************************/

void magVertex(REG(fp0, float x), REG(fp1, float y), REG(fp2, float z), REG(a6, MaggieBase *lib))
{
	if(lib->nIModeVtx >= IMM_MODE_MAGGIE_VERTS)
	{
		FlushImmediateMode(lib);
	}
	if(lib->immModeVtx == 0xffff)
		return;
	struct MaggieVertex *vtx = GetVBVertices(lib->vertexBuffers[lib->immModeVtx]);
	vtx[lib->nIModeVtx].pos.x = x;
	vtx[lib->nIModeVtx].pos.y = y;
	vtx[lib->nIModeVtx].pos.z = z;
	vtx[lib->nIModeVtx].normal = lib->ImmVtx.normal;
	for(int i = 0; i < MAGGIE_MAX_TEXCOORDS; ++i)
	{
		vtx[lib->nIModeVtx].tex[i].u = lib->ImmVtx.tex[i].u * 256.0f * 65536.0f;
		vtx[lib->nIModeVtx].tex[i].v = lib->ImmVtx.tex[i].v * 256.0f * 65536.0f;
	}
	vtx[lib->nIModeVtx].colour = RGBToGrayScale(lib->ImmVtx.colour);
	lib->nIModeVtx++;
}

/*****************************************************************************/

void magNormal(REG(fp0, float x), REG(fp1, float y), REG(fp2, float z), REG(a6, MaggieBase *lib))
{
	lib->ImmVtx.normal.x = x;
	lib->ImmVtx.normal.y = y;
	lib->ImmVtx.normal.z = z;
}

/*****************************************************************************/

void magTexCoord(REG(d0, UWORD texReg), REG(fp0, float u), REG(fp1, float v), REG(a6, MaggieBase *lib))
{
	lib->ImmVtx.tex[texReg].u = u;
	lib->ImmVtx.tex[texReg].v = v;
	lib->ImmVtx.tex[texReg].w = 1.0f;
}

/*****************************************************************************/

void magTexCoord3(REG(d0, UWORD texReg), REG(fp0, float u), REG(fp1, float v), REG(fp2, float w), REG(a6, MaggieBase *lib))
{
	lib->ImmVtx.tex[texReg].u = u;
	lib->ImmVtx.tex[texReg].v = v;
	lib->ImmVtx.tex[texReg].w = w;
}

/*****************************************************************************/

void magColour(REG(d0, ULONG col), REG(a6, MaggieBase *lib))
{
	lib->ImmVtx.colour = col;
}

/*****************************************************************************/

void magClear(REG(d0, UWORD buffers), REG(a6, MaggieBase *lib))
{
#if PROFILE
	ULONG startTime = GetClocks();
#endif

	struct ExecBase *SysBase = lib->sysBase;
	if(buffers & MAG_CLEAR_COLOUR)
	{
		if(lib->drawMode & MAG_DRAWMODE_32BIT)
		{
			int size = (lib->xres * lib->yres * 4) & ~15;
			magFastClear(lib->screen, size, lib->clearColour);
		}
		else
		{
			int size = (lib->xres * lib->yres * 2) & ~15;
			ULONG colour = ColourTo16Bit(lib->clearColour);
			colour |= colour << 16;
			magFastClear(lib->screen, size, colour);
		}
	}
	if(buffers & MAG_CLEAR_DEPTH)
	{
		if(lib->depthBuffer)
		{
			int size = lib->xres * lib->yres * 2;
			size = (size + 15) & ~15;
			magFastClear(lib->depthBuffer, size, lib->clearDepth | (lib->clearDepth << 16));
		}
	}
#if PROFILE
	lib->profile.clear += GetClocks() - startTime;
#endif
}

/*****************************************************************************/

void magClearColour(REG(d0, ULONG colour), REG(a6, MaggieBase *lib))
{
	lib->clearColour = colour;
}

/*****************************************************************************/

void magClearDepth(REG(d0, UWORD depth), REG(a6, MaggieBase *lib))
{
	lib->clearDepth = depth;
}

/*****************************************************************************/
