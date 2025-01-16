#include <stdio.h>
#include <proto/exec.h>
#include <proto/lowlevel.h>

#include <proto/Maggie.h>
#include <maggie_vec.h>
#include <maggie_vertex.h>
#include <maggie_flags.h>

/*****************************************************************************/

#include <hardware/custom.h>
#include <hardware/dmabits.h>

/*****************************************************************************/

extern struct Custom custom;

/*****************************************************************************/

#define XRES 640
#define YRES 360
#define BPP 2
#define NFRAMES 3

# define MAGGIE_MODE 0x0b02		/* 640x360x16bpp */

#define SCREENSIZE (XRES * YRES * BPP)

/*****************************************************************************/

struct Library *LowLevelBase;
struct Library *MaggieBase;

/*****************************************************************************/

static volatile int vblPassed;

/*****************************************************************************/

static int VBLInterrupt()
{
	vblPassed = 1;
	return 0;
}

/*****************************************************************************/

void WaitVBLPassed()
{
	while(!vblPassed)
		;
	vblPassed = 0;
}

/*****************************************************************************/

UWORD LoadTexture(const char *filename)
{
	UBYTE *data = NULL;
	FILE *fp = fopen(filename, "rb");
	if(!fp)
		return 0xffff;

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 128, SEEK_SET);

	data = AllocMem(size - 128, MEMF_ANY);
	fread(data, 1, size - 128, fp);
	fclose(fp);

	UWORD txtr = magAllocateTexture(8);
	magUploadTexture(txtr, 8, data, 0);
	FreeMem(data, size - 128);

	return txtr;
}

/*****************************************************************************/

static struct MaggieVertex CubeVertices[6 * 4] = 
{
	{ {-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, { { 1.0f, 1.0f } }, 0x00ffffff },
	{ {-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, { { 1.0f, 0.0f } }, 0x00ffffff },
	{ { 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, { { 0.0f, 0.0f } }, 0x00ffffff },
	{ { 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, { { 0.0f, 1.0f } }, 0x00ffffff },
	{ {-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, { { 0.0f, 1.0f } }, 0x00ffffff },
	{ { 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, { { 1.0f, 1.0f } }, 0x00ffffff },
	{ { 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, { { 1.0f, 0.0f } }, 0x00ffffff },
	{ {-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f,  1.0f}, { { 0.0f, 0.0f } }, 0x00ffffff },
	{ {-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, { { 0.0f, 1.0f } }, 0x00ffffff },
	{ { 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, { { 1.0f, 1.0f } }, 0x00ffffff },
	{ { 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, { { 1.0f, 0.0f } }, 0x00ffffff },
	{ {-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, { { 0.0f, 0.0f } }, 0x00ffffff },
	{ {-1.0f,  1.0f, -1.0f}, {0.0f,  1.0f, 0.0f}, { { 1.0f, 1.0f } }, 0x00ffffff },
	{ {-1.0f,  1.0f,  1.0f}, {0.0f,  1.0f, 0.0f}, { { 1.0f, 0.0f } }, 0x00ffffff },
	{ { 1.0f,  1.0f,  1.0f}, {0.0f,  1.0f, 0.0f}, { { 0.0f, 0.0f } }, 0x00ffffff },
	{ { 1.0f,  1.0f, -1.0f}, {0.0f,  1.0f, 0.0f}, { { 0.0f, 1.0f } }, 0x00ffffff },
	{ {-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, { { 1.0f, 0.0f } }, 0x00ffffff },
	{ {-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, { { 0.0f, 0.0f } }, 0x00ffffff },
	{ {-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, { { 0.0f, 1.0f } }, 0x00ffffff },
	{ {-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, { { 1.0f, 1.0f } }, 0x00ffffff },
	{ { 1.0f, -1.0f, -1.0f}, { 1.0f, 0.0f, 0.0f}, { { 1.0f, 1.0f } }, 0x00ffffff },
	{ { 1.0f,  1.0f, -1.0f}, { 1.0f, 0.0f, 0.0f}, { { 1.0f, 0.0f } }, 0x00ffffff },
	{ { 1.0f,  1.0f,  1.0f}, { 1.0f, 0.0f, 0.0f}, { { 0.0f, 0.0f } }, 0x00ffffff },
	{ { 1.0f, -1.0f,  1.0f}, { 1.0f, 0.0f, 0.0f}, { { 0.0f, 1.0f } }, 0x00ffffff },
};

/*****************************************************************************/

UWORD CubeIndices[5 * 6 - 1] =
{
	 0,  1,  2,  3, 0xffff,
	 4,  5,  6,  7, 0xffff,
	 8,  9, 10, 11, 0xffff,
	12, 13, 14, 15, 0xffff,
	16, 17, 18, 19, 0xffff,
	20, 21, 22, 23
};

/*****************************************************************************/

int main(int argc, char *argv[])
{
	int i;
	UBYTE *screenMem;
	UBYTE *screenPixels[NFRAMES];
	volatile UWORD *SAGA_ScreenModeRead = (UWORD *)0xdfe1f4;
	volatile ULONG *SAGA_ChunkyDataRead = (ULONG *)0xdfe1ec;
	volatile UWORD *SAGA_ScreenMode = (UWORD *)0xdff1f4;
	volatile ULONG *SAGA_ChunkyData = (ULONG *)0xdff1ec;
	UWORD oldMode;
	ULONG oldScreen;
	int currentBufferNumber = 0;
	int iBuffer;
	int vBuffer;
	UWORD txtr;
	APTR vblHandle;
	float targetRatio = 9.0f / 16.0f;
	mat4 worldMatrix, viewMatrix, perspective;
	float xangle = 0.0f;
	float yangle = 0.0f;

	MaggieBase = OpenLibrary((UBYTE *)"maggie.library", 0);

	if(!MaggieBase)
	{
		printf("Can't open maggie.library\n");
		return 0;
	}

	LowLevelBase = OpenLibrary((UBYTE *)"lowlevel.library", 0);

	if(!LowLevelBase)
	{
		printf("Can't open lowlevel.library\n");
		CloseLibrary(MaggieBase);
		return 0;
	}

	screenMem = AllocMem(SCREENSIZE * NFRAMES, MEMF_ANY | MEMF_CLEAR);

	screenPixels[0] = screenMem;
	for(i = 1; i < NFRAMES; ++i)
	{
		screenPixels[i] = screenPixels[i - 1] + SCREENSIZE;
	}

	SystemControl(SCON_TakeOverSys, TRUE, TAG_DONE);

	UWORD oldDMAcon = custom.dmaconr & (DMAF_RASTER | DMAF_COPPER);
	custom.dmacon = DMAF_RASTER | DMAF_COPPER;

	oldMode = *SAGA_ScreenModeRead;
	oldScreen = *SAGA_ChunkyDataRead;

	*SAGA_ScreenMode = MAGGIE_MODE;

	mat4_identity(&worldMatrix);
	mat4_translate(&viewMatrix, 0.0f, 0.0f, 9.0f);
	mat4_perspective(&perspective, 60.0f, targetRatio, 0.01f, 100.0f);

	txtr = LoadTexture("TestTexture.dds");

	vblHandle = AddVBlankInt((APTR)VBLInterrupt, NULL);

	vBuffer = magAllocateVertexBuffer(6 * 4);
	magUploadVertexBuffer(vBuffer, CubeVertices, 0, 6 * 4);

	iBuffer = magAllocateIndexBuffer(6 * 5 - 1);
	magUploadIndexBuffer(iBuffer, CubeIndices, 0, 6 * 5 - 1);

	while(!(ReadJoyPort(0) & JPF_BUTTON_RED)) /* While left mouse button not pressed */
	{
		UWORD *pixels;
		mat4 xRot, yRot;

		*SAGA_ChunkyData = (ULONG)screenPixels[currentBufferNumber];

		currentBufferNumber = (currentBufferNumber + 1) % NFRAMES;
		pixels = (UWORD *)screenPixels[currentBufferNumber];

		WaitVBLPassed();

		mat4_rotateX(&xRot, xangle);
		mat4_rotateY(&yRot, yangle);
		mat4_mul(&worldMatrix, &xRot, &yRot);

		magBeginScene();

		magSetDrawMode(MAG_DRAWMODE_BILINEAR | MAG_DRAWMODE_LIGHTING);

		magSetScreenMemory((APTR)pixels, XRES, YRES);

		magClear(MAG_CLEAR_COLOUR);

		magSetWorldMatrix((float *)&worldMatrix);
		magSetViewMatrix((float *)&viewMatrix);
		magSetPerspectiveMatrix((float *)&perspective);

		magSetLightPosition(0, 0.0f, 0.0f, -10.0f);
		magSetLightDirection(0, 0.0f, 0.0f, 1.0f);
		magSetLightColour(0, 0x00ffffff);
		magSetLightAttenuation(0, 100.0f);
		magSetLightType(0, MAG_LIGHT_POINT);

		magSetLightColour(1, 0x00101010);
		magSetLightType(1, MAG_LIGHT_AMBIENT);

		magSetTexture(0, txtr);

		magSetVertexBuffer(vBuffer);
		magSetIndexBuffer(iBuffer);

		magDrawIndexedPolygons(0, 6 * 4, 0, 6 * 5 - 1);

		magEndScene();

		xangle += 0.01f;
		yangle += 0.0123f;
	}
	magFreeTexture(txtr);
	magFreeVertexBuffer(vBuffer);
	magFreeIndexBuffer(iBuffer);

	RemVBlankInt(vblHandle);

	*SAGA_ScreenMode = oldMode;
	*SAGA_ChunkyData = oldScreen;

	custom.dmacon = DMAF_SETCLR | oldDMAcon;

	SystemControl(SCON_TakeOverSys, FALSE, TAG_DONE); /* Restore system */

	FreeMem(screenMem, SCREENSIZE * NFRAMES);

	CloseLibrary(LowLevelBase);
	CloseLibrary(MaggieBase);

	return 0;
}
