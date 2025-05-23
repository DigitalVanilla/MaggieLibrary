#ifndef MAGGIE_H_INCLUDED
#define MAGGIE_H_INCLUDED

#include <exec/types.h>

struct MaggieVertex;
struct MaggieClippedVertex;
struct MaggieSpriteVertex;
struct SpanPosition;

struct vec3
typedef struct vec3 vec3;

void __magDummy(); // Duff prototype.. Not a function.

void magSetScreenMemory(APTR *pixels, UWORD xres, UWORD yres);
void magSetTexture(UWORD unit, UWORD txtr);
void magSetDrawMode(UWORD mode);
void magSetRGB(ULONG rgb);

/*****************************************************************************/

UWORD *magGetDepthBuffer(); // This is the live depth buffer!

/*****************************************************************************/
/*****************************************************************************/

// These are reset on EndDraw.
void magSetWorldMatrix(float *matrix);
void magSetViewMatrix(float *matrix);
void magSetPerspectiveMatrix(float *matrix);

/*****************************************************************************/

// Calls OwnBlitter/DisownBlitter
void magDrawTrianglesUP(struct MaggieVertex *vtx, UWORD nVerts);
void magDrawIndexedTrianglesUP(struct MaggieVertex *vtx, UWORD nVtx, UWORD *indx, UWORD nIndx);
void magDrawIndexedPolygonsUP(struct MaggieVertex *vtx, UWORD nVtx, UWORD *indx, UWORD nIndx);
// TODO : Clipped / screenspace path

/*****************************************************************************/

void magSetVertexBuffer(WORD vBuffer);
void magSetIndexBuffer(WORD iBuffer);

/*****************************************************************************/

// Calls OwnBlitter/DisownBlitter
void magDrawTriangles(UWORD startVtx, UWORD nVtx);
void magDrawIndexedTriangles(UWORD firstVtx, UWORD nVtx, UWORD startIndx, UWORD nIndx);
void magDrawIndexedPolygons(UWORD firstVtx, UWORD nVtx, UWORD startIndx, UWORD nIndx);
// TODO : fast partial draw path.

/*****************************************************************************/

void magDrawLinearSpan(struct SpanPosition *start, struct SpanPosition *end);
void magDrawSpan(struct MaggieClippedVertex *start, struct MaggieClippedVertex *end);

/*****************************************************************************/
// All Buffers/textures are GLOBAL, and must be freed at exit.
/*****************************************************************************/

// Allocate vertex buffers.

UWORD magAllocateVertexBuffer(UWORD nVerts);
void magUploadVertexBuffer(UWORD vBuffer, struct MaggieVertex *vtx, UWORD startVtx, UWORD nVerts);
void magFreeVertexBuffer(UWORD vBuffer);

/*****************************************************************************/
/*****************************************************************************/

// Allocate index buffers

UWORD magAllocateIndexBuffer(UWORD nIndx);
void magUploadIndexBuffer(UWORD iBuffer, UWORD *indx, UWORD startIndx, UWORD nIndx);
void magFreeIndexBuffer(UWORD iBuffer);

/*****************************************************************************/

// Allocate texture
UWORD magAllocateTexture(UWORD size);
void magUploadTexture(UWORD txtr, UWORD mipmap, APTR data, UWORD format);
void magFreeTexture(UWORD txtr);

/*****************************************************************************/

// Library semaphore Lock. May reset _all_ state.
void magBeginScene();
void magEndScene();

/*****************************************************************************/

// Immediate mode, or "slow mode"..
// Calls OwnBlitter/DisownBlitter.
void magBegin();
void magEnd();

void magVertex(float x, float y, float z);
void magNormal(float x, float y, float z);
void magTexCoord(int texReg, float u, float v);
void magTexCoord3(int texReg, float u, float v, float w);
void magColour(ULONG col);

/*****************************************************************************/

void magClear(UWORD buffers);

/*****************************************************************************/

void magSetLightType(UWORD light, UWORD type);
void magSetLightPosition(UWORD light, float x, float y, float z);
void magSetLightDirection(UWORD light, float x, float y, float z);
void magSetLightCone(UWORD light, float phi);
void magSetLightAttenuation(UWORD light, float attenuation);
void magSetLightColour(UWORD light, ULONG colour);

/*****************************************************************************/

void magClearColour(ULONG colour);
void magClearDepth(UWORD depth);

/*****************************************************************************/

void magScissor(UWORD x, UWORD y, UWORD width, UWORD height);

/*****************************************************************************/

void magDrawSprites(UWORD startVtx, UWORD nSprites, float spriteSize);
void magDrawSpritesUP(struct MaggieSpriteVertex *vtx, UWORD nSprites, float spriteSize);

/*****************************************************************************/

void magUploadVertexPositions(UWORD vBuffer, struct vec3 *vtx, UWORD startVtx, UWORD nVerts);
void magUploadVertexNormals(UWORD vBuffer, struct vec3 *normals, UWORD startVtx, UWORD nVerts);
void magUploadVertexTexCoords2(UWORD vBuffer, struct vec3 *texCoords, UWORD startVtx, UWORD nVerts);
void magUploadVertexTexCoords3(UWORD vBuffer, struct vec2 *texCoords, UWORD startVtx, UWORD nVerts);
void magUploadVertexColours(UWORD vBuffer, ULONG *colours, UWORD startVtx, UWORD nVerts);

/*****************************************************************************/

#endif // MAGGIE_H_INCLUDED
