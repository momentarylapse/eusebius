/*----------------------------------------------------------------------------*\
| Nix common                                                                   |
| -> common data references                                                    |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/


#include <stdio.h>

#ifdef NIX_OS_WINDOWS
	#define _WIN32_WINDOWS 0x500
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <mmsystem.h>
	#pragma warning(disable : 4995)
	#ifdef NIX_ALLOW_VIDEO_TEXTURE
		#include "vfw.h"
	#endif
#endif

#ifdef NIX_OS_LINUX
#ifdef NIX_ALLOW_FULLSCREEN
	#include <X11/extensions/xf86vmode.h>
#endif
	#include <X11/keysym.h>
	#include <stdlib.h>
	#include <gdk/gdkx.h>
	#include <sys/time.h>
	#include <sys/types.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#define _open	open
	#define _close	close
#endif

#ifdef NIX_API_DIRECTX9
	#include <d3dx9.h>
#endif

#ifdef NIX_API_OPENGL
	#ifdef NIX_OS_WINDOWS
		#include <gl\gl.h>
		#include <gl\glu.h>
	#endif
	#ifdef NIX_OS_LINUX
		#define GL_GLEXT_PROTOTYPES
		#include <GL/glx.h>
		#include <GL/gl.h>
		#include <GL/glext.h>
		#include <GL/glu.h>
	#endif
#endif





#ifdef NIX_API_DIRECTX9
	extern IDirect3DDevice9 *lpDevice;
	extern D3DCAPS9 d3dCaps;
	extern D3DSURFACE_DESC d3dsdBackBuffer;
	extern D3DVIEWPORT9 DXViewPort;

	extern char *DXErrorMsg(HRESULT h);
#endif
#ifdef NIX_API_OPENGL
	#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
		#ifdef NIX_OS_WINDOWS
			#include "glext.h"
			extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
			extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
			extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
			extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
			extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
			extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
			extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
			extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
			extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
			extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
			extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
			extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
			extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
			extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
			extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
			extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
			extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;
		#endif

		extern bool OGLDynamicTextureSupport;
	#endif
	extern matrix NixOGLProjectionMatrix2D;
	extern bool NixGLDoubleBuffered;
	extern int NixOGLFontDPList;
	extern int NixglShaderCurrent;
#endif


extern matrix NixViewMatrix,NixProjectionMatrix,NixInvProjectionMatrix;
extern matrix NixWorldMatrix,NixWorldViewProjectionMatrix;

extern float NixView3DRatio;
extern vector NixViewScale;

extern bool NixUsable, NixDoingEvilThingsToTheDevice;
extern bool NixEnabled3D;

extern int NixFontGlyphWidth[256];


// light-sources
enum{
	LightTypeDirectional,
	LightTypeRadial
};
struct sLight{
	bool Used,Allowed,Enabled;
	int Type;
#ifdef NIX_API_DIRECTX9
#endif
#ifdef NIX_API_OPENGL
	int OGLLightNo;
#endif
	int Light;
	vector Pos,Dir;
	float Radius;
	color Ambient,Diffuse,Specular;
};

extern std::vector<sLight> NixLight;


struct sVertexBuffer
{
	int NumTrias, NumPoints, MaxTrias, NumTextures;
	bool Indexed, Used, NotedFull;
#ifdef NIX_API_DIRECTX9
	LPDIRECT3DVERTEXBUFFER9 dxVertices;
	LPDIRECT3DINDEXBUFFER9 dxIndex;
#endif
#ifdef NIX_API_OPENGL
	//OGLVertex3D* glVertices;
	vector *glVertices;
	vector *glNormals;
	float *glTexCoords[4];
#endif
};

extern std::vector<sVertexBuffer> NixVB;

struct sShaderFile
{
#ifdef NIX_API_DIRECTX9
	LPD3DXEFFECT dxEffect;
	D3DXHANDLE dxEffectTech;
	//D3DXEFFECT_DESC m_EffectDesc;
#endif
#ifdef NIX_API_OPENGL
	int glShader;
#endif
};

extern std::vector<sShaderFile> NixShaderFile;





#ifdef NIX_API_DIRECTX9
inline void DXSet2DMode()
{
	lpDevice->SetRenderState(D3DRS_LIGHTING,NixLightingEnabled2D);
}

inline void DXSet3DMode(){
	lpDevice->SetRenderState(D3DRS_LIGHTING,NixLightingEnabled);
}
#endif

#ifdef NIX_API_OPENGL
inline void OGLSet2DMode()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float*)&NixOGLProjectionMatrix2D);
	if (NixLightingEnabled2D)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
}

inline void OGLSet3DMode()
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float*)&NixProjectionMatrix);
	glColor3f(1,1,1);
	if (NixLightingEnabled)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
}
#endif


