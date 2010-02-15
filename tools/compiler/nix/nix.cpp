/*----------------------------------------------------------------------------*\
| Nix                                                                          |
| -> abstraction layer for api (graphics, sound, networking)                   |
| -> OpenGL and DirectX9 supported                                             |
|   -> able to switch in runtime                                               |
| -> mathematical types and functions                                          |
|   -> vector, matrix, matrix3, quaternion, plane, color                       |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2009.10.03 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#include "nix.h"


char NixVersion[]="0.8.34.11";


#ifdef NIX_OS_WINDOWS
	#define _WIN32_WINDOWS 0x500
#endif
#ifdef NIX_OS_WINDOWS
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <mmsystem.h>
	#pragma warning(disable : 4995)
#endif
#ifdef NIX_OS_LINUX
	#define GL_GLEXT_PROTOTYPES
	#include <GL/glx.h>
	#include <GL/gl.h>
	#include <GL/glext.h>
	#include <GL/glu.h>
#ifdef NIX_ALLOW_FULLSCREEN
	#include <X11/extensions/xf86vmode.h>
#endif
	#include <X11/keysym.h>
	#include <stdlib.h>
	#include <gdk/gdkx.h>
	#include <sys/time.h>
	#include <sys/types.h>

	#include <fcntl.h>
	#include <stdio.h>
	#include <unistd.h>
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
		#include <GL/gl.h>
		#include <GL/glu.h>
		//#include <GL/glut.h>
	#endif
#endif

// libraries (in case Visual C++ is used)
#ifdef NIX_API_DIRECTX9
	#pragma comment(lib,"d3d9.lib")
	#pragma comment(lib,"d3dx9.lib")
	#pragma comment(lib,"dsound.lib")
	//#pragma comment(lib,"dinput9.lib")
	#pragma comment(lib,"dxerr9.lib")
//	#pragma comment(lib,"d3dx9dt.lib")
	#pragma comment(lib,"d3dxof.lib")
	#pragma comment(lib,"dxguid.lib")
#endif
#ifdef NIX_API_OPENGL
	#pragma comment(lib,"opengl32.lib")
	#pragma comment(lib,"glu32.lib")
	//#pragma comment(lib,"glut32.lib")
//	#pragma comment(lib,"glaux.lib ")
#endif
#ifdef NIX_ALLOW_VIDEO_TEXTURE
	#pragma comment(lib,"strmiids.lib")
	#pragma comment(lib,"vfw32.lib")
#endif


/*
libraries to link:

-ldsound
-ld3dx9d
-ld3d9
-ld3dxof
-ldxguid
-lwinmm
-lcomctl32
-lkernel32
-luser32
-lgdi32
-lwinspool
-lcomdlg32
-ladvapi32
-lshell32
-lole32
-loleaut32
-luuid
-lodbc32
-lodbccp32
-lwinmm
-lgdi32
-lwsock32
-lopengl32
-lglu32
-lvfw_avi32
-lvfw_ms32
*/






// environment
CHuiWindow *NixWindow;
static float View3DWidth,View3DHeight,View3DCenterX,View3DCenterY,View3DRatio;	// 3D transformation
static float View2DScaleX,View2DScaleY;				// 2D transformation
static int PerspectiveModeSize,PerspectiveModeCenter,PerspectiveMode2DScale;
static bool Usable,DoingEvilThingsToTheDevice;

static bool Enabled3D;
static int NumVBs,VBNumTrias[NIX_MAX_VBS],VBNumPoints[NIX_MAX_VBS],VBMaxTrias[NIX_MAX_VBS],VBNumTextures[NIX_MAX_VBS];
static bool VBIndexed[NIX_MAX_VBS],VBUsed[NIX_MAX_VBS];
// things'n'stuff
static int NumLights;
static bool WireFrame;
matrix NixViewMatrix,NixProjectionMatrix,NixInvProjectionMatrix;
static matrix *PostProjectionMatrix; // for creating the NixProjectionMatrix
static matrix identity_matrix;
static vector ViewScale=vector(1,1,1);
static int ClipPlaneMask;
static int FontGlyphWidth[256];

static bool KeyBufferRead;
static int TimerKey;


int NixApi;
int NixScreenWidth,NixScreenHeight,NixScreenDepth;		// current screen resolution
int NixDesktopWidth,NixDesktopHeight,NixDesktopDepth;	// pre-NIX-resolution
int NixTargetWidth,NixTargetHeight;						// render target size (window/texture)
bool NixFullscreen;
callback_function *NixRefillAllVertexBuffers;

float NixMouseMappingWidth,NixMouseMappingHeight;		// fullscreen mouse territory
int NixFatalError;
int NixNumTrias;

int NixTextureWidth[NIX_MAX_TEXTURES],NixTextureHeight[NIX_MAX_TEXTURES];
int NixTextureMaxFramesToLive,NixMaxVideoTextureSize=256;
float NixMaxDepth,NixMinDepth;

sInputData NixInputDataCurrent,NixInputDataLast;
bool NixLightingEnabled,NixLightingEnabled2D;
bool NixCullingInverted;


int NixFontHeight=20;
char NixFontName[128]="Times New Roman";
static color FontColor=White;

int VBTemp;

// sizes
static int VPx1,VPy1,VPx2,VPy2;
#ifdef NIX_OS_WINDOWS
	static HMENU hMenu;
	static HDC hDC;
	static HGLRC hRC;
	static RECT WindowClient,WindowBounds;
	static DWORD WindowStyle;
#endif


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
sLight *Light[NIX_MAX_LIGHTS];


//#define ENABLE_INDEX_BUFFERS

static int RenderingToTexture=-1;
static int NumShaderFiles=0;

#ifdef NIX_API_DIRECTX9
	// DirectInput
	/*static LPDIRECTINPUT		lpDI=NULL;
	static LPDIRECTINPUTDEVICE	pKeyboard=NULL;
	static LPDIRECTINPUTDEVICE	pMouse=NULL;*/

	// Direct3D
	PDIRECT3D9 lpD3D=NULL;
	IDirect3DDevice9 *lpDevice=NULL;
	D3DCAPS9 d3dCaps;
	D3DPRESENT_PARAMETERS d3dpp;
	D3DSURFACE_DESC d3dsdBackBuffer;
	LPDIRECT3DSURFACE9 FrontBuffer=NULL,DepthBuffer=NULL;
	D3DVIEWPORT9 DXViewPort;
	// vertex buffer
	LPDIRECT3DVERTEXBUFFER9 DXVB2D,DXVBVertices[NIX_MAX_VBS];
	LPDIRECT3DINDEXBUFFER9 DXVBIndex[NIX_MAX_VBS];

	// shader files
	LPD3DXEFFECT DXEffect[NIX_MAX_SHADERFILES],DXEffectCurrent;
	D3DXHANDLE DXEffectTech[NIX_MAX_SHADERFILES];
	unsigned int DXEffectCurrentNumPasses;
	//D3DXEFFECT_DESC m_EffectDesc[NIX_MAX_SHADERFILES];

	// font
	static ID3DXFont* DXFont;
	static float DXFontTexCoords[256][4];
	static IDirect3DStateBlock9 *DXFontStateBlock;
	static LPDIRECT3DTEXTURE9 DXFontTex=NULL;
	static LPDIRECT3DVERTEXBUFFER9 DXFontVB=NULL;

	struct DXVertex2D
	{
		float x,y,z,rhw;
		DWORD color;
		float tu,tv;
	};

	struct DXVertex3D
	{
	    float x, y, z;
	    float nx,ny,nz;
	    float tu, tv;//, tw;
	};

	struct DXVertex3D2
	{
	    float x, y, z;
	    float nx,ny,nz;
	    float tu0, tv0, tu1, tv1;
	};

	struct DXVertex3D3
	{
	    float x, y, z;
	    float nx,ny,nz;
	    float tu0, tv0, tu1, tv1, tu2, tv2;
	};

	struct DXVertex3D4
	{
	    float x, y, z;
	    float nx,ny,nz;
	    float tu0, tv0, tu1, tv1, tu2, tv2, tu3, tv3;
	};

	#define D3D_VERTEX2D	(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)
	#define D3D_VERTEX3D	(D3DFVF_XYZ|D3DFVF_TEX1|D3DFVF_NORMAL)//|D3DFVF_TEXCOORDSIZE3(0))
	#define D3D_VERTEX3D2	(D3DFVF_XYZ|D3DFVF_TEX2|D3DFVF_NORMAL)
	#define D3D_VERTEX3D3	(D3DFVF_XYZ|D3DFVF_TEX3|D3DFVF_NORMAL)
	#define D3D_VERTEX3D4	(D3DFVF_XYZ|D3DFVF_TEX4|D3DFVF_NORMAL)

	inline DXVertex2D InitVertex2D(float x,float y,float z,D3DCOLOR col,float tu,float tv)
	{
		DXVertex2D v;
		v.x=x;
		v.y=y;
		v.z=z;
		v.rhw=1;
		v.color=col;
		v.tu=tu;
		v.tv=tv;
		return v;
	}

	void InitOwnFont(const char *Name,int Height)
	{
		lpDevice->CreateVertexBuffer(	256*6*sizeof(DXVertex2D),
										D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
										D3D_VERTEX2D,
										D3DPOOL_DEFAULT,
										&DXFontVB,
										NULL);
		lpDevice->CreateTexture(256,256,1,0,D3DFMT_A4R4G4B4,D3DPOOL_MANAGED,&DXFontTex,NULL);
		DWORD* pBitmapBits;
		BITMAPINFO bmi;
		ZeroMemory(&bmi.bmiHeader,sizeof(BITMAPINFOHEADER));
		bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth=(int)256;
		bmi.bmiHeader.biHeight=-(int)256;
		bmi.bmiHeader.biPlanes=1;
		bmi.bmiHeader.biCompression=BI_RGB;
		bmi.bmiHeader.biBitCount=32;
		HDC hDC=CreateCompatibleDC(NULL);
		HBITMAP hbmBitmap=CreateDIBSection(hDC,&bmi,DIB_RGB_COLORS,(VOID**)&pBitmapBits,NULL,0);
		SetMapMode(hDC,MM_TEXT);
		INT nHeight=Height;//-MulDiv(Height,(INT)(GetDeviceCaps(hDC,LOGPIXELSY)),72);
		HFONT hFont=CreateFont(	nHeight,0,0,0,FW_EXTRALIGHT,FALSE,
								FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
								CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,
								VARIABLE_PITCH,sys_str(Name));
		SelectObject(hDC,hbmBitmap);
		SelectObject(hDC,hFont);
		SetTextColor(hDC,RGB(255,255,255));
		SetBkColor(hDC,0x00000000);
		SetTextAlign(hDC,TA_TOP);
		DWORD x=0;
		DWORD y=0;
		char str[5];
		SIZE size;
		for(int c=0;c<255;c++){
			str[0]=c;
			str[1]=0;
			GetTextExtentPoint32(hDC,sys_str(str),1,&size);
			if((DWORD)(x+size.cx+1)>256){
				x=0;
				y+=size.cy+1;
			}
			ExtTextOut(hDC,x+0,y+0,ETO_OPAQUE,NULL,sys_str(str),1,NULL);
			DXFontTexCoords[c][0]=((FLOAT)(x+0))/256;
			DXFontTexCoords[c][1]=((FLOAT)(y+0))/256;
			DXFontTexCoords[c][2]=((FLOAT)(x+0+size.cx))/256;
			DXFontTexCoords[c][3]=((FLOAT)(y+0+size.cy))/256;
			x+=size.cx+1;
		}
		D3DLOCKED_RECT d3dlr;
		DXFontTex->LockRect(0,&d3dlr,0,0);
		BYTE* pDstRow=(BYTE*)d3dlr.pBits;
		WORD* pDst16;
		BYTE bAlpha;
		for(y=0;y<256;y++){
	        pDst16=(WORD*)pDstRow;
			for(x=0;x<256;x++){
				bAlpha=(BYTE)((pBitmapBits[256*y+x]&0xff)>>4);
				if (bAlpha>0)
					*pDst16++=(bAlpha<<12)|0x0fff;
				else
					*pDst16++=0x0000;
			}
			pDstRow+=d3dlr.Pitch;
		}
		DXFontTex->UnlockRect(0);
		DeleteObject(hbmBitmap);
		DeleteDC(hDC);
		DeleteObject(hFont);
		lpDevice->BeginStateBlock();
		lpDevice->SetTexture(0,DXFontTex);
		lpDevice->SetRenderState(D3DRS_ZENABLE,FALSE);
		lpDevice->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
		lpDevice->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);
		lpDevice->SetRenderState(D3DRS_STENCILENABLE,FALSE);
		lpDevice->SetRenderState(D3DRS_CLIPPING,TRUE);
		//lpDevice->SetRenderState(D3DRS_EDGEANTIALIAS,FALSE);
		lpDevice->SetRenderState(D3DRS_CLIPPLANEENABLE,FALSE);
		lpDevice->SetRenderState(D3DRS_VERTEXBLEND,FALSE);
		lpDevice->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE,FALSE);
		lpDevice->SetRenderState(D3DRS_FOGENABLE,FALSE);
		lpDevice->EndStateBlock(&DXFontStateBlock);
	}
	inline D3DCOLOR color2D3DCOLOR(color &c){
		int a=int(c.a*255.0f);		if (a<0)	a=0;		if (a>255)	a=255;
		int r=int(c.r*255.0f);		if (r<0)	r=0;		if (r>255)	r=255;
		int g=int(c.g*255.0f);		if (g<0)	g=0;		if (g>255)	g=255;
		int b=int(c.b*255.0f);		if (b<0)	b=0;		if (b>255)	b=255;
		return D3DCOLOR_ARGB(a,r,g,b);
	}
	inline D3DCOLORVALUE color2D3DCOLORVALUE(color &c){
		D3DCOLORVALUE cv;
		cv.a=c.a;
		cv.r=c.r;
		cv.g=c.g;
		cv.b=c.b;
		return cv;
	}
	inline D3DCOLORVALUE C2CV(D3DCOLOR c)
	{
		D3DCOLORVALUE cv;
		cv.a=((c>>24)%256)/255.0f;
		cv.r=((c>>16)%256)/255.0f;
		cv.g=((c>> 8)%256)/255.0f;
		cv.b=( c     %256)/255.0f;
		return cv;
	}
	char *DXErrorMsg(HRESULT h)
	{
		if (h==D3D_OK)								return("D3D_OK");
		if (h==D3DERR_CONFLICTINGRENDERSTATE)		return("D3DERR_CONFLICTINGRENDERSTATE");
		if (h==D3DERR_CONFLICTINGTEXTUREFILTER)		return("D3DERR_CONFLICTINGTEXTUREFILTER");
		if (h==D3DERR_CONFLICTINGTEXTUREPALETTE)	return("D3DERR_CONFLICTINGTEXTUREPALETTE");
		if (h==D3DERR_DEVICELOST)					return("D3DERR_DEVICELOST");
		if (h==D3DERR_DEVICENOTRESET)				return("D3DERR_DEVICENOTRESET");
		if (h==D3DERR_DRIVERINTERNALERROR)			return("D3DERR_DRIVERINTERNALERROR");
		if (h==D3DERR_INVALIDCALL)					return("D3DERR_INVALIDCALL");
		if (h==D3DERR_INVALIDDEVICE)				return("D3DERR_INVALIDDEVICE");
		if (h==D3DERR_MOREDATA)						return("D3DERR_MOREDATA");
		if (h==D3DERR_NOTAVAILABLE)					return("D3DERR_NOTAVAILABLE");
		if (h==D3DERR_NOTFOUND)						return("D3DERR_NOTFOUND");
		if (h==D3DERR_OUTOFVIDEOMEMORY)				return("D3DERR_OUTOFVIDEOMEMORY");
		if (h==D3DERR_TOOMANYOPERATIONS)			return("D3DERR_TOOMANYOPERATIONS");
		if (h==D3DERR_UNSUPPORTEDALPHAARG)			return("D3DERR_UNSUPPORTEDALPHAARG");
		if (h==D3DERR_UNSUPPORTEDALPHAOPERATION)	return("D3DERR_UNSUPPORTEDALPHAOPERATION");
		if (h==D3DERR_UNSUPPORTEDCOLORARG)			return("D3DERR_UNSUPPORTEDCOLORARG");
		if (h==D3DERR_UNSUPPORTEDCOLOROPERATION)	return("D3DERR_UNSUPPORTEDCOLOROPERATION");
		if (h==D3DERR_UNSUPPORTEDFACTORVALUE)		return("D3DERR_UNSUPPORTEDFACTORVALUE");
		if (h==D3DERR_UNSUPPORTEDTEXTUREFILTER)		return("D3DERR_UNSUPPORTEDTEXTUREFILTER");
		if (h==D3DERR_WRONGTEXTUREFORMAT)			return("D3DERR_WRONGTEXTUREFORMAT");
		if (h==E_FAIL)								return("E_FAIL");
		if (h==E_INVALIDARG)						return("E_INVALIDARG");
		//if (h==E_INVALIDCALL)						return("E_INVALIDCALL");
		if (h==E_OUTOFMEMORY)						return("E_OUTOFMEMORY");
		if (h==S_OK)								return("S_OK");
		return(string("unbekannter Fehler ",i2s(h)));
	}
#endif
#ifdef NIX_API_OPENGL
	#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
		#ifdef NIX_OS_WINDOWS
			#include "glext.h"
			extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT = NULL;
			extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = NULL;
			extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT = NULL;
			extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = NULL;
			extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = NULL;
			extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT = NULL;
			extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT = NULL;
			extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = NULL;
			extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = NULL;
			extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = NULL;
			extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
			extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT = NULL;
			extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = NULL;
			extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT = NULL;
			extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = NULL;
			extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT = NULL;
			extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT = NULL;
		#else
		#endif
		bool OGLDynamicTextureSupport = false;
		GLuint OGLFrameBuffer[NIX_MAX_TEXTURES];
		GLuint OGLDepthRenderBuffer[NIX_MAX_TEXTURES];

	/*		// WGL_ARB_extensions_string
			PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = NULL;
			// WGL_ARB_pbuffer
			PFNWGLCREATEPBUFFERARBPROC    wglCreatePbufferARB    = NULL;
			PFNWGLGETPBUFFERDCARBPROC     wglGetPbufferDCARB     = NULL;
			PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB = NULL;
			PFNWGLDESTROYPBUFFERARBPROC   wglDestroyPbufferARB   = NULL;
			PFNWGLQUERYPBUFFERARBPROC     wglQueryPbufferARB     = NULL;
			// WGL_ARB_pixel_format
			PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
			// WGL_ARB_render_texture
			PFNWGLBINDTEXIMAGEARBPROC     wglBindTexImageARB     = NULL;
			PFNWGLRELEASETEXIMAGEARBPROC  wglReleaseTexImageARB  = NULL;
			PFNWGLSETPBUFFERATTRIBARBPROC wglSetPbufferAttribARB = NULL;
			HPBUFFERARB odt_hPBuffer[NIX_MAX_TEXTURES];
			HDC         odt_hDC[NIX_MAX_TEXTURES];
			HGLRC       odt_hRC[NIX_MAX_TEXTURES];*/
	#endif

	/*struct OGLVertex3D{
		float tu,tv;
		float nx,ny,nz;
		float x,y,z;
	};*/
	/*struct OGLVertex2D
	{
		float x,y,z;
		unsigned int color;
		float tu,tv;
	};*/
	static int OGLPixelFormat;
	#ifdef NIX_OS_LINUX
		static GLXContext context;
		static bool DoubleBuffered;
	#endif

	extern unsigned int OGLTexture[NIX_MAX_TEXTURES];
	// VertexBuffer
	//static OGLVertex3D* OGLVBVertices[NIX_MAX_VBS];
	static vector *OGLVBVertices[NIX_MAX_VBS];
	static vector *OGLVBNormals[NIX_MAX_VBS];
	static float *OGLVBTexCoords[NIX_MAX_VBS][4];
	static matrix OGLProjectionMatrix2D;
	static int OGLViewPort[4];

	// shader files
	int OGLShader[NIX_MAX_SHADERFILES],OGLShaderCurrent=0;

	// font
	int OGLFontDPList;
#endif

#ifdef NIX_OS_LINUX
#ifdef NIX_ALLOW_FULLSCREEN
	XF86VidModeModeInfo *original_mode;
#endif
	int screen;
#endif




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
	glLoadMatrixf((float*)&OGLProjectionMatrix2D);
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


void CreateFontGlyphWidth()
{
#ifdef NIX_OS_WINDOWS
	hDC=GetDC(NixWindow->hWnd);
	SetMapMode(hDC,MM_TEXT);
	HFONT hFont=CreateFont(	NixFontHeight,0,0,0,FW_EXTRALIGHT,FALSE,
							FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,
							VARIABLE_PITCH,hui_tchar_str(NixFontName));
	SelectObject(hDC,hFont);
	unsigned char str[5];
	SIZE size;
	for(int c=0;c<255;c++){
		str[0]=c;
		str[1]=0;
		GetTextExtentPoint32(hDC,hui_tchar_str((char*)str),1,&size);
		FontGlyphWidth[c]=size.cx;
	}
	DeleteObject(hFont);
#endif
#ifdef NIX_OS_LINUX
#ifdef NIX_API_OPENGL
	OGLSet2DMode();
	for(int c=0;c<255;c++){
		glRasterPos3f(0,0,0);
		int x0;
		float x[4];
		glGetFloatv(GL_CURRENT_RASTER_POSITION,x);
		x0=int(x[0]+0.5f);
		glListBase(OGLFontDPList);
		glCallLists(1,GL_UNSIGNED_BYTE,&c);
		glGetFloatv(GL_CURRENT_RASTER_POSITION,x);
		FontGlyphWidth[c]=int(x[0]+0.5f)-x0;
	}
#endif
#endif
}

void MatrixOut(matrix &m)
{
	msg_write("MatrixOut");
	msg_write(string2("	%f:2	%f:2	%f:2	%f:2",m._00,m._01,m._02,m._03));
	msg_write(string2("	%f:2	%f:2	%f:2	%f:2",m._10,m._11,m._12,m._13));
	msg_write(string2("	%f:2	%f:2	%f:2	%f:2",m._20,m._21,m._22,m._23));
	msg_write(string2("	%f:2	%f:2	%f:2	%f:2",m._30,m._31,m._32,m._33));
}

void mout(matrix &m)
{
	msg_write(string2("		%f	%f	%f	%f",m._00,m._01,m._02,m._03));
	msg_write(string2("		%f	%f	%f	%f",m._10,m._11,m._12,m._13));
	msg_write(string2("		%f	%f	%f	%f",m._20,m._21,m._22,m._23));
	msg_write(string2("		%f	%f	%f	%f",m._30,m._31,m._32,m._33));
}


matrix WorldMatrix,WorldViewProjectionMatrix;


void NixInit(int api,int xres,int yres,int depth,bool fullscreen,CHuiWindow *win)
{
	Usable=false;
	if (!msg_inited)
		msg_init();
	NixWindow=win;
	if (NixWindow)
		NixWindow->UsedByNix=true;
	//win->Update();
	NixFullscreen=false; // before nix is started, we're hopefully not in fullscreen mode

	NixNetInit();

#ifdef HUI_API_WIN
	//CoInitialize(NULL);

	// save window data
	if (NixWindow){
		WindowStyle=GetWindowLong(NixWindow->hWnd,GWL_STYLE);
		hMenu=GetMenu(NixWindow->hWnd);
		GetWindowRect(NixWindow->hWnd,&WindowBounds);
		GetClientRect(NixWindow->hWnd,&WindowClient);
		ShowCursor(FALSE); // will be shown again at next window mode initialization!
		NixWindow->NixGetInputFromWindow=&NixGetInputFromWindow;
	}


	// save the original video mode
	DEVMODE mode;
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&mode);
	NixDesktopWidth=mode.dmPelsWidth;
	NixDesktopHeight=mode.dmPelsHeight;
	NixDesktopDepth=mode.dmBitsPerPel;
#endif
#ifdef NIX_OS_LINUX
	#ifdef NIX_ALLOW_FULLSCREEN
		XF86VidModeModeInfo **modes;
		int NumModes;
		screen = DefaultScreen(hui_x_display);
		XF86VidModeGetAllModeLines(hui_x_display, screen, &NumModes, &modes);
		original_mode = modes[0];
		NixDesktopWidth = modes[0]->hdisplay;
		NixDesktopHeight = modes[0]->vdisplay;
		NixDesktopDepth = modes[0]->hdisplay;
	#else
		NixDesktopWidth = 1024;
		NixDesktopHeight = 768;
		NixDesktopDepth = 32;
	#endif
	if (NixWindow){
		Window SomeWindow;
		int x,y;
		unsigned int w,h,borderDummy,x_depth;
		XGetGeometry(hui_x_display,GDK_WINDOW_XWINDOW(NixWindow->gl_widget->window),&SomeWindow,&x,&y,&w,&h,&borderDummy,&x_depth);
		NixDesktopDepth=x_depth;
		//msg_db_m(string2("Desktop: %dx%dx%d\n",NixDesktopWidth,NixDesktopHeight,NixDesktopDepth),0);
	}
	/*int glxMajorVersion,glxMinorVersion;
	int vidModeMajorVersion,vidModeMinorVersion;
	XF86VidModeQueryVersion(display,&vidModeMajorVersion,&vidModeMinorVersion);
	msg_db_m(string2("XF86VidModeExtension-Version %d.%d\n",vidModeMajorVersion,vidModeMinorVersion),1);*/
#endif
	NixScreenWidth=NixDesktopWidth;
	NixScreenHeight=NixDesktopHeight;

	msg_write("Nix");
	msg_right();
	msg_write(string("[",NixVersion,"]"));

	// reset data
	NixApi=-1;
	//NixFullscreen=fullscreen;
	NixFatalError=FatalErrorNone;
	NixRefillAllVertexBuffers=NULL;


	// default values of the engine
	if (NixWindow){
		irect r = NixWindow->GetInterior();
		NixTargetWidth = r.x2 - r.x1;
		NixTargetHeight = r.y2 - r.y1;
	}else{
		NixTargetWidth = 800;
		NixTargetHeight = 600;
	}
	MatrixIdentity(identity_matrix);
	MatrixIdentity(NixViewMatrix);
	MatrixIdentity(NixProjectionMatrix);
	NixSetPerspectiveMode(PerspectiveSizeAutoScreen);
	NixSetPerspectiveMode(PerspectiveCenterAutoTarget);
	NixSetPerspectiveMode(Perspective2DScaleSet,1,1);
	NixSetPerspectiveMode(PerspectiveRatioSet,4.0f/3.0f);
	Enabled3D=true;
	NixMouseMappingWidth=1024;
	NixMouseMappingHeight=768;
	NixMaxDepth=100000.0f;
	NixMinDepth=1.0f;
	NixTextureMaxFramesToLive=4096*8;
	NumVBs=0;
	NumLights=0;
	ClipPlaneMask=0;
	NixCullingInverted=false;

	// set the new video mode
	NixSetVideoMode(api,xres,yres,depth,fullscreen);
	if (NixFatalError!=FatalErrorNone){
		msg_left();
		return;
	}

	// initiate sound
	NixSoundInit();

	// more default values of the engine
	NixSetPerspectiveMode(PerspectiveSizeAutoScreen);
	NixSetCull(CullDefault);
	NixSetWire(false);
	NixSetAlpha(AlphaNone);
	NixEnableLighting(false);
	NixEnableLighting2D(false);
	color c;
	NixSetMaterial(White,White,White,0,c=color(0.1f,0.1f,0.1f,0.1f));
	NixSetAmbientLight(Black);
	NixSpecularEnable(false);
	NixCullingInverted=false;
	NixSetView(true,NixViewMatrix);
	NixResize();
	memset(&NixInputDataCurrent,0,sizeof(sInputData));
	memset(&NixInputDataLast,0,sizeof(sInputData));
#ifdef NIX_API_DIRECTX9
	DXEffectCurrent=NULL;
#endif

	NixTexturesInit();

	VBTemp=NixCreateVB(10240);
	// timer for windows key repitition simulation
	TimerKey=HuiCreateTimer();
	Usable=true;


	msg_ok();
	msg_left();
}

void NixKill()
{
	NixKillDeviceObjects();
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (NixFullscreen){
			#ifdef NIX_OS_WINDOWS
				/*DEVMODE dmScreenSettings;								// Device Mode
				memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
				dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
				dmScreenSettings.dmPelsWidth	= 0;			// Selected Screen Width
				dmScreenSettings.dmPelsHeight	= 0;			// Selected Screen Height
				dmScreenSettings.dmBitsPerPel	= 0;			// Selected Bits Per Pixel
				dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
				ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN);*/
			#endif

			#ifdef NIX_OS_LINUX
				msg_write("Restore Video Mode");
				int r = system(string2("xrandr --size %dx%d", NixDesktopWidth, NixDesktopHeight));
				/*bool b=XF86VidModeSwitchToMode(hui_x_display,screen,original_mode);
				XFlush(hui_x_display);
				XF86VidModeSetViewPort(hui_x_display,screen,0,0);*/
			#endif
		}
	}
#endif
	if (NixWindow){
		NixWindow->SetFullscreen(false);
		NixWindow->ShowCursor(true);
	}
}

// erlaubt dem Device einen Neustart
void NixKillDeviceObjects()
{
	msg_db_r("NixKillDeviceObjects",1);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (!lpDevice){
			msg_db_l(1);
			return;
		}

		// font stuff
		msg_db_m("-font",2);
		if (DXFont){		DXFont->Release();		DXFont=NULL;	}
		if (DXFontVB){		DXFontVB->Release();	DXFontVB=NULL;	}
		if (DXFontTex){		DXFontTex->Release();	DXFontTex=NULL;	}
		// vertex buffer
		msg_db_m("-vertex buffer 2D",2);
		DXVB2D->Release();	DXVB2D=NULL;
		msg_db_m("-vertex buffer",2);
		for (int i=0;i<NumVBs;i++)
			if (VBUsed[i]){
				DXVBVertices[i]->Release();
				DXVBVertices[i]=NULL;
			}
		// textures
		msg_db_m("-textures",2);
		NixReleaseTextures();
		// graphics buffer
		msg_db_m("-buffer",2);
		FrontBuffer->Release();		FrontBuffer=NULL;
		DepthBuffer->Release();		DepthBuffer=NULL;
		// DirectX device
		msg_db_m("-device",2);
		if (lpDevice->Release() > 0L){
			msg_error("device not quite clean yet!");
			msg_left();
			return;
		}
		msg_db_m("-Direct3D",2);
		lpD3D->Release();
		msg_db_l(1);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		// textures
		msg_db_m("-textures",2);
		NixReleaseTextures();
	}
#endif
	msg_db_l(1);
}

void NixReincarnateDeviceObjects()
{
	msg_db_r("ReincarnateDeviceObjects",1);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		// vertex buffers
		msg_db_m("-vertex buffer",2);
		for (int i=0;i<NumVBs;i++)
			NixCreateVB(VBMaxTrias[i],i);
		// textures
		msg_db_m("-textures",2);
		void NixReincarnateTextures();
		// graphical buffers
		msg_db_m("-buffers",2);
		lpDevice->GetRenderTarget(0,&FrontBuffer);
		lpDevice->GetDepthStencilSurface(&DepthBuffer);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		// textures
		msg_db_m("-textures",2);
		NixReincarnateTextures();
	}
#endif
	if (NixRefillAllVertexBuffers)
		NixRefillAllVertexBuffers();
	msg_db_l(1);
}

#ifdef NIX_OS_LINUX
/* attributes for a single buffered visual in RGBA format with at least
 * 4 bits per color and a 16 bit depth buffer */
int attrListSgl[]={
	GLX_RGBA,
	GLX_RED_SIZE,	4,
	GLX_GREEN_SIZE,	4,
	GLX_BLUE_SIZE,	4,
	GLX_DEPTH_SIZE,	16,
	None
};

/* attributes for a double buffered visual in RGBA format with at least
 * 4 bits per color and a 16 bit depth buffer */
int attrListDbl[]={
	GLX_RGBA,
	GLX_DOUBLEBUFFER,
	GLX_RED_SIZE,	4,
	GLX_GREEN_SIZE,	4,
	GLX_BLUE_SIZE,	4,
	GLX_DEPTH_SIZE,	16,
	//GLX_STENCIL_SIZE,16,
	//GLX_ALPHA_SIZE,	4,
	None
};
#endif





void NixSetVideoMode(char api,int xres,int yres,int depth,bool fullscreen)
{
	msg_db_r("setting video mode",0);

	char ApiName[32]="";
	if (api==NIX_API_NONE){	strcpy(ApiName,"no API !!!");	fullscreen=false;	}
#ifdef NIX_API_DIRECTX9
	if (api==NIX_API_DIRECTX9)		strcpy(ApiName,"DirectX9");
#endif
#ifdef NIX_API_OPENGL
	if (api==NIX_API_OPENGL)		strcpy(ApiName,"OpenGL");
#endif
	if (strlen(ApiName)<1){
		msg_error(string("unknown index for graphics api: ",i2s(api)));
		NixFatalError=FatalErrorUnknownApi;
		msg_db_l(0);
		return;
	}
	if (fullscreen){
		msg_db_m(string2("[ %s - fullscreen - %d x %d x %d ]",ApiName,xres,yres,depth),0);
	}else{
		msg_db_m(string2("[ %s - window mode ]",ApiName),0);
		xres=NixDesktopWidth;
		yres=NixDesktopHeight;
	}

	if (api==NIX_API_NONE){
		msg_db_l(0);
		return;
	}

	bool was_fullscreen = NixFullscreen;
	NixFullscreen = fullscreen;
	NixApi = api;
	Usable = false;
	NixFatalError = FatalErrorNone;
	DoingEvilThingsToTheDevice = true;
	NixKillDeviceObjects();


#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		HRESULT hr;

	// Direct3D
		msg_db_m("-Direct3D",1);

		// create DirectX9 handle
		lpD3D=Direct3DCreate9(D3D_SDK_VERSION);
		if (lpD3D==NULL){
			msg_error("can not initiate DirectX9");
			NixFatalError=FatalErrorNoDirectX9;
			msg_db_l(0);
			return;
		}

		ZeroMemory(&d3dpp,sizeof(d3dpp));
		d3dpp.Windowed				=!NixFullscreen;
		d3dpp.BackBufferCount		=1;
		d3dpp.SwapEffect			=D3DSWAPEFFECT_DISCARD;
		d3dpp.EnableAutoDepthStencil=true;
		d3dpp.AutoDepthStencilFormat=D3DFMT_D24S8;
		d3dpp.hDeviceWindow			=NixWindow->hWnd;
		d3dpp.BackBufferWidth		=xres;
		d3dpp.BackBufferHeight		=yres;
		if (NixFullscreen){
			if (depth==32)
				d3dpp.BackBufferFormat	=D3DFMT_A8R8G8B8;
			else
				d3dpp.BackBufferFormat	=D3DFMT_R5G6B5;
		}else{
			GetClientRect(NixWindow->hWnd,&WindowClient);
			d3dpp.BackBufferWidth		=NixDesktopWidth;
			d3dpp.BackBufferHeight		=NixDesktopHeight;
			if (NixDesktopDepth==32)
				d3dpp.BackBufferFormat	=D3DFMT_A8R8G8B8;
			else
				d3dpp.BackBufferFormat	=D3DFMT_R5G6B5;
		}
		// Device -> hardware mode
		msg_db_m("-device",1);
		hr=lpD3D->CreateDevice(	D3DADAPTER_DEFAULT,
								D3DDEVTYPE_HAL,
								NixWindow->hWnd,
								D3DCREATE_SOFTWARE_VERTEXPROCESSING,
								&d3dpp,
								&lpDevice);
		// Device -> software mode
		if (FAILED(hr)){
			msg_write(DXErrorMsg(hr));
			msg_error("hardware mode noit supported -> software mode!");
			lpD3D->CreateDevice(D3DADAPTER_DEFAULT,
								D3DDEVTYPE_REF,
								NixWindow->hWnd,
								D3DCREATE_SOFTWARE_VERTEXPROCESSING,
								&d3dpp,
								&lpDevice);

		}
		if (FAILED(hr)){
			msg_write(DXErrorMsg(hr));
			msg_error("neither hardware moe nor software mode supported");
			NixFatalError=FatalErrorNoDevice;
			msg_db_l(0);
			return;
		}

		// store render target surface desc
		LPDIRECT3DSURFACE9 pBackBuffer = NULL;
		lpDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer );
		pBackBuffer->GetDesc(&d3dsdBackBuffer);
		pBackBuffer->Release();

		lpDevice->GetDeviceCaps( &d3dCaps );
		if ((d3dCaps.StencilCaps & D3DSTENCILCAPS_TWOSIDED)!=0){
			msg_db_m("(TwoSided)",5);
		}else{
			msg_db_m("(not TwoSided)",5);
		}


	// font
		msg_db_m("-font",1);
		//if (NixFullscreen)
			InitOwnFont(NixFontName,NixFontHeight);
		/*else{
			HFONT hFont=CreateFont(	NixFontHeight,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
									ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
									ANTIALIASED_QUALITY,FF_DONTCARE,NixFontName);
			D3DXCreateFont(lpDevice,hFont,&DXFont);
		}*/

	// graphics settings
		msg_db_m("-setting properties",1);
		lpDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
		lpDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
		lpDevice->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT);
		lpDevice->SetRenderState(D3DRS_ZENABLE,true);
		lpDevice->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_POINT);
		lpDevice->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		lpDevice->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);


	// 2D vertex buffer
		msg_db_m("-2D vertex buffer",1);
		lpDevice->CreateVertexBuffer(	8*sizeof(DXVertex2D),
										D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
										D3D_VERTEX2D,
										D3DPOOL_DEFAULT,
										&DXVB2D,
										NULL); // RESERVED!!!


	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
	
		#ifdef NIX_OS_WINDOWS
			
			msg_db_m("-pixelformat",1);
			if ((!NixFullscreen)&&(was_fullscreen)){
				DEVMODE dmScreenSettings;
				memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
				dmScreenSettings.dmSize=sizeof(dmScreenSettings);
				dmScreenSettings.dmPelsWidth=NixDesktopWidth;
				dmScreenSettings.dmPelsHeight=NixDesktopHeight;
				dmScreenSettings.dmBitsPerPel=NixDesktopDepth;
				dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
				ChangeDisplaySettings(&dmScreenSettings,0);
			}else if (NixFullscreen){
				DEVMODE dmScreenSettings;
				memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
				dmScreenSettings.dmSize=sizeof(dmScreenSettings);
				dmScreenSettings.dmPelsWidth=xres;
				dmScreenSettings.dmPelsHeight=yres;
				dmScreenSettings.dmBitsPerPel=depth;
				dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
				ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN);
			}

			PIXELFORMATDESCRIPTOR pfd={	sizeof(PIXELFORMATDESCRIPTOR),
										1,						// versions nummer
										PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
										PFD_TYPE_RGBA,
										NixFullscreen?depth:NixDesktopDepth,
										//8, 0, 8, 8, 8, 16, 8, 24,
										0, 0, 0, 0, 0, 0, 0, 0, 0,
										0, 0, 0, 0,
										24,						// 24Bit Z-Buffer
										1,						// one stencil buffer
										0,						// no "Auxiliary"-buffer
										PFD_MAIN_PLANE,
										0, 0, 0, 0 };
			hDC = GetDC(NixWindow->hWnd);
			//hDC = GetDC(NixWindow->gl_hwnd);
			if (!hDC){
				HuiErrorBox(NixWindow, "Fehler", "GetDC...");
				exit(0);
			}
			OGLPixelFormat = ChoosePixelFormat(hDC, &pfd);
			SetPixelFormat(hDC, OGLPixelFormat, &pfd);
			hRC=wglCreateContext(hDC);
			if (!hRC){
				HuiErrorBox(NixWindow, "Fehler", "wglCreateContext...");
				exit(0);
			}
			int rr=wglMakeCurrent(hDC, hRC);

		#endif // NIX_OS_WINDOWS
		#ifdef NIX_OS_LINUX
			XVisualInfo *vi;
	//Colormap cmap;
			#ifdef NIX_ALLOW_FULLSCREEN
				XF86VidModeModeInfo **modes;
				int num_modes;
				int best_mode = 0;
	//XSetWindowAttributes attr;
				XF86VidModeGetAllModeLines(hui_x_display, screen, &num_modes, &modes);
				for (int i=0;i<num_modes;i++)
					if ((modes[i]->hdisplay == xres) && (modes[i]->vdisplay == yres))
						best_mode = i;
			#endif
			vi = glXChooseVisual(hui_x_display, screen, attrListDbl);
			if (vi){
				msg_db_m("-doublebuffered", 1);
				DoubleBuffered = true;
			}else{
				msg_error("only singlebuffered visual!");
				vi = glXChooseVisual(hui_x_display, screen, attrListSgl);
				DoubleBuffered = false;
			}
			context = glXCreateContext(hui_x_display, vi, 0, GL_TRUE);
	//cmap=XCreateColormap(display,RootWindow(display,vi->screen),vi->visual,AllocNone);
	/*attr.colormap=cmap;
	attr.border_pixel=0;*/
	//Window win=GDK_WINDOW_XWINDOW(NixWindow->window->window);
			Window win = GDK_WINDOW_XWINDOW(NixWindow->gl_widget->window);
			if ((int)(long)win < 1000)
				msg_error("no GLX window found");
			//XSelectInput(hui_x_display,win, ExposureMask | KeyPress | KeyReleaseMask | StructureNotifyMask);

			// set video mode
			if (NixFullscreen){
				/*XF86VidModeSwitchToMode(hui_x_display, screen, modes[best_mode]);
				XFlush(hui_x_display);
				int x, y;
				XF86VidModeGetViewPort(hui_x_display, screen, &x, &y);
				printf("view port:    %d %d\n", x, y);
				XF86VidModeSetViewPort(hui_x_display, screen, 0, 0);
				XFlush(hui_x_display);
				XF86VidModeGetViewPort(hui_x_display, screen, &x, &y);
				printf("view port:    %d %d\n", x, y);
				//XF86VidModeSetViewPort(hui_x_display, screen, 0, NixDesktopHeight - yres);
				printf("Resolution %d x %d\n", modes[best_mode]->hdisplay, modes[best_mode]->vdisplay);
				XFree(modes);*/
				int r = system(string2("xrandr --size %dx%d", xres, yres));
				XWarpPointer(hui_x_display, None, win, 0, 0, 0, 0, xres / 2, yres / 2);
			}
			glXMakeCurrent(hui_x_display, win, context);
			if (glXIsDirect(hui_x_display, context)){
				msg_db_m("-direct rendering",1);
			}else
				msg_error("-no direct rendering!");
		#endif // NIX_OS_LINUX

		msg_db_m("-setting properties",1);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST); // "Really Nice Perspective Calculations" (...)
		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
		#ifdef NIX_OS_LINUX
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
		#endif

		// font
		msg_db_m("-font",1);
		OGLFontDPList=glGenLists(256);
		#ifdef NIX_OS_WINDOWS
			HFONT hFont=CreateFont(NixFontHeight,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE|DEFAULT_PITCH,hui_tchar_str(NixFontName));
			SelectObject(hDC,hFont);
			wglUseFontBitmaps(hDC,0,255,OGLFontDPList);
		#endif
		#ifdef NIX_OS_LINUX
			Font font;
    /*fontInfo = XLoadQueryFont(GLWin.dpy, FontName);
	if (fontInfo){
		printf("------------Font--------------\n");
	}else{
		printf("------------kein Font--------------\n");
	}
    font = fontInfo->fid;*/ 
			//font=XLoadFont(display,"-adobe-new century schoolbook-medium-r-normal--14-140-75-75-p-82-iso8859-1");
			font=XLoadFont(hui_x_display,"*century*medium-r-normal*--14*");
			/*if (font<0)
				font=XLoadFont(display,"*medium-r-normal*--14*");
			if (font<0)
				font=XLoadFont(display,"*--14*");*/
			glXUseXFont(font,0,256,OGLFontDPList);
		/*	int num;   
		char **fl=XListFonts(GLWin.dpy, "*", 10240, &num);
			msg_write(num);
		for (int i=0;i<num;i++)
			msg_write(fl[i]);
		XFreeFontNames(fl);
			printf("----\n");*/
		#endif


	

		#ifdef  NIX_ALLOW_DYNAMIC_TEXTURE
			msg_db_m("-RenderToTexture-Support",1);

			char *ext = (char*)glGetString( GL_EXTENSIONS );
		
		if (strstr(ext,"EXT_framebuffer_object")==NULL){
				msg_error("EXT_framebuffer_object extension was not found");
			}else{
				OGLDynamicTextureSupport = true;
#ifdef NIX_OS_WINDOWS
				glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)wglGetProcAddress("glIsRenderbufferEXT");
				glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
				glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
				glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
				glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
				glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)wglGetProcAddress("glGetRenderbufferParameterivEXT");
				glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)wglGetProcAddress("glIsFramebufferEXT");
				glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
				glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
				glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
				glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
				glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)wglGetProcAddress("glFramebufferTexture1DEXT");
				glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
				glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)wglGetProcAddress("glFramebufferTexture3DEXT");
				glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
				glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)wglGetProcAddress("glGetFramebufferAttachmentParameterivEXT");
				glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)wglGetProcAddress("glGenerateMipmapEXT");

				if (!glIsRenderbufferEXT || !glBindRenderbufferEXT || !glDeleteRenderbuffersEXT ||
					!glGenRenderbuffersEXT || !glRenderbufferStorageEXT || !glGetRenderbufferParameterivEXT ||
					!glIsFramebufferEXT || !glBindFramebufferEXT || !glDeleteFramebuffersEXT ||
					!glGenFramebuffersEXT || !glCheckFramebufferStatusEXT || !glFramebufferTexture1DEXT ||
					!glFramebufferTexture2DEXT || !glFramebufferTexture3DEXT || !glFramebufferRenderbufferEXT||
					!glGetFramebufferAttachmentParameterivEXT || !glGenerateMipmapEXT ){
						msg_error("One or more EXT_framebuffer_object functions were not found");
				}
#endif
			}
		#endif // NIX_ALLOW_DYNAMIC_TEXTURE
			

	}
#endif

	/*			char *ext = (char*)glGetString( GL_EXTENSIONS );
				if (ext){
					msg_write(strlen(ext));
					for (int i=0;i<strlen(ext);i++)
						if (ext[i]==' ')
							ext[i]='\n';
					msg_write(ext);
				}else
					msg_error("keine Extensions?!?");*/


	DoingEvilThingsToTheDevice = false;
	CreateFontGlyphWidth();

	// adjust window for new mode
	NixWindow->ShowCursor(!NixFullscreen);
	NixWindow->SetFullscreen(NixFullscreen);
	//NixWindow->SetPosition(0, 0);
/*#ifdef NIX_OS_WINDOWS
	msg_db_m("-window",1);
	if (NixFullscreen){
		DWORD style=WS_POPUP|WS_SYSMENU|WS_VISIBLE;
		//SetWindowLong(hWnd,GWL_STYLE,WS_POPUP);
		SetWindowLong(NixWindow->hWnd,GWL_STYLE,style);

		WINDOWPLACEMENT wpl;
		GetWindowPlacement(NixWindow->hWnd,&wpl);
		wpl.rcNormalPosition.left=0;
		wpl.rcNormalPosition.top=0;
		wpl.rcNormalPosition.right=xres;
		wpl.rcNormalPosition.bottom=yres;
		AdjustWindowRect(&wpl.rcNormalPosition, style, FALSE);
		SetWindowPlacement(NixWindow->hWnd,&wpl);
	}else{
		//SetWindowLong(hWnd,GWL_STYLE,WindowStyle);
	}
#endif*/

	if (NixFullscreen){
		NixScreenWidth=xres;
		NixScreenHeight=yres;
		NixScreenDepth=depth;
	}else{
		NixScreenWidth			=NixDesktopWidth;
		NixScreenHeight			=NixDesktopHeight;
		NixScreenDepth			=NixDesktopDepth;
/*#ifdef NIX_OS_WINDOWS
		msg_db_m("SetWindowPos",1);
		SetWindowPos(	NixWindow->hWnd,HWND_NOTOPMOST,
						WindowBounds.left,
						WindowBounds.top,
						(WindowBounds.right-WindowBounds.left),
						(WindowBounds.bottom-WindowBounds.top),
						SWP_SHOWWINDOW );
#endif*/
	}



// recreate vertex buffers and textures
	NixReincarnateDeviceObjects();


	Usable = true;
	NixResize();


	msg_db_l(0);
}

void NixTellUsWhatsWrong()
{
	if (NixFatalError == FatalErrorNoDirectX9)
		HuiErrorBox(NixWindow, "DirectX 9 nicht gefunden!","Es tut mir au&serordentlich leid, aber ich hatte Probleme, auf Ihrem\nSystem DirectX 9 zu starten und mich damit zu verbinden!");
	if (NixFatalError == FatalErrorNoDevice)
		HuiErrorBox(NixWindow,"DirectX 9: weder Hardware- noch Softwaremodus!!","Es tut mir au&serordentlich leid, aber ich hatte Probleme, auf Ihrem\nSystem DirectX 9 weder einen Hardware- noch einen Softwaremodus abzuringen!\n...Unerlaubte Afl&osung?");
}

// shoot down windows
void NixKillWindows()
{
#ifdef NIX_OS_WINDOWS
	msg_db_r("Killing Windows...",0);
	HANDLE t;
	OpenProcessToken(	GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&t);
	_TOKEN_PRIVILEGES tp;
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid); 
	tp.PrivilegeCount=1;
	tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(t,FALSE,&tp,0,NULL,0);
	InitiateSystemShutdown(NULL,hui_tchar_str("Resistance is futile!"),10,TRUE,FALSE);
	//ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE,0);
	msg_db_l(0);
#endif
}

void NixResize()
{
	if (!Usable)
		return;

	msg_db_r("NixResize",10);

	if (NixTargetWidth<=0)
		NixTargetWidth=1;
	if (NixTargetHeight<=0)
		NixTargetHeight=1;

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->GetViewport(&DXViewPort);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){

		// Projektion 2D
		matrix s,t;
		// OpenGl hat (0,0) in Fenster-Mitte und berdeckt einen Bereich von -1 bis 1 (x und y)
		MatrixScale(s,2.0f/float(NixTargetWidth),-2.0f/float(NixTargetHeight),1);
		MatrixTranslation(t,vector(-float(NixTargetWidth)/2.0f,-float(NixTargetHeight)/2.0f,0));
		MatrixMultiply(OGLProjectionMatrix2D,s,t);

		// Bildschirm
		glViewport(0,0,NixTargetWidth,NixTargetHeight);
		//glViewport(0,0,NixTargetWidth,NixTargetHeight);
		OGLViewPort[0]=0;
		OGLViewPort[1]=0;
		OGLViewPort[2]=NixTargetWidth;
		OGLViewPort[3]=NixTargetHeight;

		/*glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();*/
	}
#endif

	if (PerspectiveModeCenter==PerspectiveCenterAutoTarget){
		View3DCenterX=float(NixTargetWidth)/2.0f;
		View3DCenterY=float(NixTargetHeight)/2.0f;
	}
	if (PerspectiveModeSize==PerspectiveSizeAutoTarget){
		View3DWidth=float(NixTargetWidth);
		View3DHeight=float(NixTargetHeight);
	}
	if (PerspectiveModeSize==PerspectiveSizeAutoScreen){
		View3DWidth=float(NixScreenWidth);
		View3DHeight=float(NixScreenHeight);
	}

	// Kamera
	NixSetView(Enabled3D,NixViewMatrix);

	msg_db_l(10);
}



void NixSetFontColor(color c)
{
	FontColor=c;
}

void NixDrawChar(int x,int y,char c)
{
	char str[2];
	str[0]=c;
	str[1]=0;
	NixDrawStr(x,y,str);
}

void NixDrawStr(int x,int y,const char *str)
{
	msg_db_r("NixDrawStr",10);
	char *str2=_file_get_str_();
	int ll=strlen(str),l=0;
	for (int i=0;i<ll;i++){
		if (str[i]=='&'){
			i++;
			if (str[i]=='a')		str2[l++]=(signed char)0xe4;
			else if (str[i]=='o')	str2[l++]=(signed char)0xf6;
			else if (str[i]=='u')	str2[l++]=(signed char)0xfc;
			else if (str[i]=='A')	str2[l++]=(signed char)0xc4;
			else if (str[i]=='O')	str2[l++]=(signed char)0xd6;
			else if (str[i]=='U')	str2[l++]=(signed char)0xdc;
			else if (str[i]=='s')	str2[l++]=(signed char)0xdf;
			else str2[l++]=str[i-1];
		}else
			str2[l++]=str[i];
	}
	str2[l++]=0;
	str=str2;
	NixSetTexture(-1);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		DXSet2DMode();
		lpDevice->SetFVF(D3D_VERTEX2D);
		//if (NixFullscreen){
			DXVertex2D* pVertices=NULL;
			DWORD dwNumTriangles=0;
			DXFontVB->Lock(0,0,(void**)&pVertices,D3DLOCK_DISCARD);

			lpDevice->SetTexture(0,DXFontTex);
			lpDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);//D3DTOP_SELECTARG1);
			lpDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			lpDevice->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
			/*LPDIRECT3DSTATEBLOCK9 SavedStateBlock;
			SavedStateBlock->Capture();
			DXFontStateBlock->Apply();*/
			lpDevice->SetStreamSource(0,DXFontVB,0,sizeof(DXVertex2D));
			float sx=(float)x;
			float sy=(float)y;
			char* Text=NULL;
			int NumTriangles=0;
			Text=str;
			while(*Text){
				unsigned char c=*Text++;
				float tx1=DXFontTexCoords[c][0];
				float tx2=DXFontTexCoords[c][2];
				float ty1=DXFontTexCoords[c][1];
				float ty2=DXFontTexCoords[c][3];
				float w=(tx2-tx1)*256;
				float h=(ty2-ty1)*256;
				float depth=0.0f;
				*pVertices++=InitVertex2D(sx+0-0.5f,sy+h-0.5f,depth,color2D3DCOLOR(FontColor),tx1,ty2);
				*pVertices++=InitVertex2D(sx+0-0.5f,sy+0-0.5f,depth,color2D3DCOLOR(FontColor),tx1,ty1);
				*pVertices++=InitVertex2D(sx+w-0.5f,sy+h-0.5f,depth,color2D3DCOLOR(FontColor),tx2,ty2);
				*pVertices++=InitVertex2D(sx+w-0.5f,sy+0-0.5f,depth,color2D3DCOLOR(FontColor),tx2,ty1);
				*pVertices++=InitVertex2D(sx+w-0.5f,sy+h-0.5f,depth,color2D3DCOLOR(FontColor),tx2,ty2);
				*pVertices++=InitVertex2D(sx+0-0.5f,sy+0-0.5f,depth,color2D3DCOLOR(FontColor),tx1,ty1);
				NumTriangles+=2;
				sx+=w;
			}
			DXFontVB->Unlock();
			if (NumTriangles>0)
				lpDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,NumTriangles);
			//SavedStateBlock->Apply();
		/*}else{
			RECT rct;
			DXFont->Begin();
			rct.left	=x;
			rct.right	=NixTargetWidth;
			rct.top		=y;
			rct.bottom	=NixTargetHeight;
			DXFont->DrawText( str, -1, &rct, 0, color2D3DCOLOR(FontColor) );
			DXFont->End();
		}*/
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLSet2DMode();
		bool z_test, z_write;
		glGetBooleanv(GL_DEPTH_TEST, (GLboolean*)&z_test);
		glGetBooleanv(GL_DEPTH_WRITEMASK, (GLboolean*)&z_write);
		if (z_test)
			NixSetZ(false, false);
		glColor3f(FontColor.r,FontColor.g,FontColor.b);
		glRasterPos3f(float(x),float(y+2+int(float(NixFontHeight)*0.75f)),-1.0f);
		glListBase(OGLFontDPList);
		glCallLists(strlen(str),GL_UNSIGNED_BYTE,str);
		glRasterPos3f(0,0,0);
		if (z_test)
			NixSetZ(z_write, z_test);
	}
#endif
	msg_db_l(10);
}

int NixGetStrWidth(const char *str,int start,int end)
{
	if (start<0)	start=0;
	if (end<0)		end=strlen(str);
	int w=0;
	for (int i=start;i<end;i++)
		w+=FontGlyphWidth[(unsigned char)str[i]];
/*#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
	}
#endif*/
	return w;
}

void NixDrawFloat(int x,int y,float fl,int com)
{
	NixDrawStr(x,y,f2s(fl,com));
}

void NixDrawInt(int x,int y,int num)
{
	NixDrawStr(x,y,i2s(num));
}

void NixDrawLine(float x1,float y1,float x2,float y2,color c,float depth)
{
	float dx=x2-x1;
	if (dx<0)	dx=-dx;
	float dy=y2-y1;
	if (dy<0)	dy=-dy;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		DXSet2DMode();
		DXVertex2D* pVertices = NULL;
		lpDevice->SetStreamSource( 0, DXVB2D, 0, sizeof(DXVertex2D) );
		lpDevice->SetFVF(D3D_VERTEX2D);
		lpDevice->SetTexture(0,NULL);
		DXVB2D->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );
		if (dx>dy){
			if (x1>x2){
				float x=x2;	x2=x1;	x1=x;
				float y=y2;	y2=y1;	y1=y;
			}
			*pVertices++ = InitVertex2D( x1,y1+1,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x1,y1  ,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x2,y2+1,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x2,y2  ,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x2,y2+1,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x1,y1  ,depth, color2D3DCOLOR(c), 0,0 );
		}else{
			if (y1<y2){
				float x=x2;	x2=x1;	x1=x;
				float y=y2;	y2=y1;	y1=y;
			}
			*pVertices++ = InitVertex2D( x1+1,y1,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x1  ,y1,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x2+1,y2,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x2  ,y2,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x2+1,y2,depth, color2D3DCOLOR(c), 0,0 );
			*pVertices++ = InitVertex2D( x1  ,y1,depth, color2D3DCOLOR(c), 0,0 );
		}
		DXVB2D->Unlock();
		lpDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLSet2DMode();
		glDisable(GL_TEXTURE_2D);
		glColor3fv((float*)&c);

#ifdef NIX_OS_LINUX
		glLineWidth(1);
		glBegin(GL_LINE);
			glVertex3f((float)x1,(float)y1,depth);
			glVertex3f((float)x2,(float)y2,depth);
		glEnd();
#else

		if (dx>dy){
			if (x1>x2){
				float x=x2;	x2=x1;	x1=x;
				float y=y2;	y2=y1;	y1=y;
			}
			glBegin(GL_TRIANGLES);
				glVertex3f(x1,y1+1,depth);
				glVertex3f(x1,y1  ,depth);
				glVertex3f(x2,y2+1,depth);
				glVertex3f(x2,y2  ,depth);
				glVertex3f(x2,y2+1,depth);
				glVertex3f(x1,y1  ,depth);
			glEnd();
		}else{
			if (y1<y2){
				float x=x2;	x2=x1;	x1=x;
				float y=y2;	y2=y1;	y1=y;
			}
			glBegin(GL_TRIANGLES);
				glVertex3f(x1+1,y1,depth);
				glVertex3f(x1  ,y1,depth);
				glVertex3f(x2+1,y2,depth);
				glVertex3f(x2  ,y2,depth);
				glVertex3f(x2+1,y2,depth);
				glVertex3f(x1  ,y1,depth);
			glEnd();
		}
#endif
	}
#endif
}

void NixDrawLineV(int x,int y1,int y2,color c,float depth)
{
	/*NixDrawLine((float)x,(float)y1,(float)x,(float)y2,c,depth);
	return;*/
	if (y1>y2){
		int y=y2;	y2=y1;	y1=y;
	}
	rect d;
	d.x1=(float)x;
	d.x2=(float)x+1;
	d.y1=(float)y1;
	d.y2=(float)y2;
	NixDraw2D(-1,&c,NULL,&d,depth);
}

void NixDrawLineH(int x1,int x2,int y,color c,float depth)
{
	/*NixDrawLine((float)x1,(float)y,(float)x2,(float)y,c,depth);
	return;*/
	if (x1>x2){
		int x=x2;
		x2=x1;
		x1=x;
	}
	rect d;
	d.x1=(float)x1;
	d.x2=(float)x2;
	d.y1=(float)y;
	d.y2=(float)y+1;
	NixDraw2D(-1,&c,NULL,&d,depth);
}

void NixDrawLine3D(vector l1,vector l2,color c)
{
/*#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLSet2DMode();
		glDisable(GL_TEXTURE_2D);
		glColor3fv(color2f3(c));
		glLineWidth(1);
		glBegin(GL_LINE);
			glVertex3fv((float*)&l1);
			glVertex3fv((float*)&l2);
		glEnd();
	}else
#endif*/
	{
		NixGetVecProject(l1,l1);
		NixGetVecProject(l2,l2);
		if ((l1.z>0)&&(l2.z>0)&&(l1.z<1)&&(l2.z<1))
			NixDrawLine(l1.x,l1.y,l2.x,l2.y,c,(l1.z+l2.z)/2);
	}
}

void NixDrawRect(float x1,float x2,float y1,float y2,color c,float depth)
{
	float t;
	if (x1>x2){
		t=x1;	x1=x2;	x2=t;
	}
	if (y1>y2){
		t=y1;	y1=y2;	y2=t;
	}
#ifdef NIX_API_OPENGL
	if ((NixApi==NIX_API_OPENGL)&&(!NixFullscreen)){
		int pa=40;
		for (int i=0;i<int(x2-x1-1)/pa+1;i++){
			for (int j=0;j<int(y2-y1-1)/pa+1;j++){
				float _x1=x1+i*pa;
				float _y1=y1+j*pa;

				float _x2=x2;
				if (x2-x1-i*pa>pa)	_x2=x1+i*pa+pa;
				float _y2=y2;
				if (y2-y1-j*pa>pa)	_y2=y1+j*pa+pa;

				rect r=rect(_x1,_x2,_y1,_y2);
				NixDraw2D(-1,&c,NULL,&r,depth);
			}
		}
		return;
	}
#endif
	rect r=rect(x1,x2,y1,y2);
	NixDraw2D(-1,&c,NULL,&r,depth);
}

inline void SetShaderFileData(int texture0,int texture1,int texture2,int texture3)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (!DXEffectCurrent)
			return;

		D3DXHANDLE hdl;
		if (hdl=DXEffectCurrent->GetParameterByName(NULL,"WorldViewProjectionMatrix"))
			DXEffectCurrent->SetMatrix(hdl,(D3DXMATRIX*)&WorldViewProjectionMatrix);
		if (hdl=DXEffectCurrent->GetParameterByName(NULL,"WorldMatrix"))
			DXEffectCurrent->SetMatrix(hdl,(D3DXMATRIX*)&WorldMatrix);
		if (hdl=DXEffectCurrent->GetParameterByName(NULL,"NixViewMatrix"))
			DXEffectCurrent->SetMatrix(hdl,(D3DXMATRIX*)&NixViewMatrix);
		if (hdl=DXEffectCurrent->GetParameterByName(NULL,"NixProjectionMatrix"))
			DXEffectCurrent->SetMatrix(hdl,(D3DXMATRIX*)&NixProjectionMatrix);
		NixSetShaderTexturesDX(DXEffectCurrent,texture0,texture1,texture2,texture3);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
#ifdef NIX_OS_LINUX
		if (OGLShaderCurrent==0)	return;
		int loc;
		loc=glGetUniformLocationARB(OGLShaderCurrent,"tex0");
		if (loc>-1)	glUniform1iARB(loc, 0);
		loc=glGetUniformLocationARB(OGLShaderCurrent,"tex1");
		if (loc>-1)	glUniform1iARB(loc, 1);
		loc=glGetUniformLocationARB(OGLShaderCurrent,"tex2");
		if (loc>-1)	glUniform1iARB(loc, 2);
		loc=glGetUniformLocationARB(OGLShaderCurrent,"tex3");
		if (loc>-1)	glUniform1iARB(loc, 3);
#endif
	}
#endif

}

void NixDraw2D(int texture,const color *col,const rect *src,const rect *dest,float depth)
{
	//if (depth==0)	depth=0.5f;
	rect s,d;
	color c;
	// no texture source -> use complete texture
	if (src)	s=(*src);
	else		s=rect(0,1,0,1);
	// no destination -> use complete target
	if (dest){
		d=(*dest);
		// manchmal Probleme mit zu grossen Ziel-Bereichen... :-S
		/*float mx=(s.x2-s.x1)/(d.x2-d.x1);
		float my=(s.y2-s.y1)/(d.y2-d.y1);
		if (d.x1<0){			s.x1-=mx*d.x1;					d.x1=0;						}
		if (d.x2>NixTargetWidth){	s.x2-=mx*(d.x2-NixTargetWidth);	d.x2=float(NixTargetWidth);	}
		if (d.y1<0){			s.y1-=my*d.y1;					d.y1=0;						}
		if (d.y2>NixTargetHeight){	s.y2-=my*(d.y2-NixTargetHeight);	d.y2=float(NixTargetHeight);	}*/
	}else{
		d.x1=0;
		d.x2=(float)NixTargetWidth;
		d.y1=0;
		d.y2=(float)NixTargetHeight;
	}
	// no color -> white
	if (!col)	c=White;
	else		c=(*col);
	//msg_write("2D");
	NixSetTexture(texture);
	SetShaderFileData(texture,-1,-1,-1);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		DXSet2DMode();
		DXVertex2D* pVertices = NULL;
		lpDevice->SetStreamSource( 0, DXVB2D, 0, sizeof(DXVertex2D) );
		lpDevice->SetFVF(D3D_VERTEX2D);
		DXVB2D->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD );
		*pVertices++ = InitVertex2D( d.x1,d.y2,depth, color2D3DCOLOR(c), s.x1, s.y2 );
		*pVertices++ = InitVertex2D( d.x1,d.y1,depth, color2D3DCOLOR(c), s.x1, s.y1 );
		*pVertices++ = InitVertex2D( d.x2,d.y2,depth, color2D3DCOLOR(c), s.x2, s.y2 );
		*pVertices++ = InitVertex2D( d.x2,d.y1,depth, color2D3DCOLOR(c), s.x2, s.y1 );
		*pVertices++ = InitVertex2D( d.x2,d.y2,depth, color2D3DCOLOR(c), s.x2, s.y2 );
		*pVertices++ = InitVertex2D( d.x1,d.y1,depth, color2D3DCOLOR(c), s.x1, s.y1 );
		DXVB2D->Unlock();

		if (DXEffectCurrent){ // loop through the shader "passes"
			for (unsigned int i=0;i<DXEffectCurrentNumPasses;i++){

				// if you encounter a compiler error here (BeginPass not defined....) please use
				// a more current version of directx9 (9.0c)...

				DXEffectCurrent->BeginPass(i);
				lpDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );
				DXEffectCurrent->EndPass();
			}
		}else // draw a single time without the shader
			lpDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		depth=depth*2-1;
		OGLSet2DMode();
		glColor4fv((float*)&c);
		glBegin(GL_TRIANGLES);
			glTexCoord2f(s.x1,1-s.y2);
			glVertex3f(d.x1,d.y2,depth);
			glTexCoord2f(s.x1,1-s.y1);
			glVertex3f(d.x1,d.y1,depth);
			glTexCoord2f(s.x2,1-s.y1);
			glVertex3f(d.x2,d.y1,depth);

			glTexCoord2f(s.x2,1-s.y1);
			glVertex3f(d.x2,d.y1,depth);
			glTexCoord2f(s.x2,1-s.y2);
			glVertex3f(d.x2,d.y2,depth);
			glTexCoord2f(s.x1,1-s.y2);
			glVertex3f(d.x1,d.y2,depth);

		glEnd();
	}
#endif
}

static char ErrStr[4096];
static char shader_buf[2048];

int NixLoadShaderFile(const char *filename)
{
	if (strlen(filename)<1)
		return -1;
	msg_write(string("loading shader file ",SysFileName(filename)));
	msg_right();
/*#ifndef NIX_IDE_VCS
	msg_error("ignoring shader file....(no visual studio!)");
	msg_left();
	return -1;
#endif*/
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		int index=-1;
		for (int i=0;i<NumShaderFiles;i++)
			if (!DXEffect[i])
				index=i;
		if (index<0){
			index=NumShaderFiles;
			NumShaderFiles++;
		}
		LPD3DXBUFFER pBufferErrors=NULL;
		HRESULT hr=D3DXCreateEffectFromFile(	lpDevice,
												sys_str_f(filename),
												NULL,
												NULL,
												0,//D3DXSHADER_DEBUG,
												NULL,
												&DXEffect[index],
												&pBufferErrors);
		if (hr!=D3D_OK){
			msg_error(DXErrorMsg(hr));
			if (pBufferErrors){
				msg_write((char*)pBufferErrors->GetBufferPointer(),pBufferErrors->GetBufferSize());
				for (unsigned int i=0;i<pBufferErrors->GetBufferSize();i++)
					ErrStr[i]=((char*)pBufferErrors->GetBufferPointer())[i];
				ErrStr[pBufferErrors->GetBufferSize()]=0;
				HuiErrorBox(NixWindow,"error in shader file",ErrStr);
			}
			msg_left();
			return -1;
		}
		DXEffect[index]->FindNextValidTechnique(NULL,&DXEffectTech[index]);
		if (!DXEffectTech[index]){
			msg_error("none of the techniques found is supported by the hardware!");
			msg_left();
			return -1;
		}
		//D3DXEFFECT_DESC m_EffectDesc[NumShaderFiles];

		msg_ok();
		msg_left();
		return index;
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
#ifdef NIX_OS_LINUX
	//	msg_todo("NixLoadShaderFile for OpenGL");
		//msg_write("-shader");
		int s=glCreateShader(GL_FRAGMENT_SHADER);
		if (s<=0){
			msg_error("could not create gl shader object");
			msg_left();
			return -1;
		}

		CFile *f=new CFile();
		if (!f->Open(string(filename,".glsl"))){
			delete(f);
			msg_left();
			return -1;
		}
		const char *pbuf[2];
		int size;
		f->ReadComplete(shader_buf,size);
		f->Close();
		delete(f);

		pbuf[0]=shader_buf;
		glShaderSource( s, 1, pbuf,NULL );


		//msg_write("-compile");
		glCompileShader( s );

		int status;
		glGetShaderiv(s,GL_COMPILE_STATUS,&status);
		//msg_write(status);
		if (status!=GL_TRUE){
			msg_error("while compiling shader...");
			glGetShaderInfoLog(s,sizeof(ErrStr),&size,ErrStr);
			msg_write(ErrStr);
			HuiErrorBox(NixWindow,"error in shader file",ErrStr);
			msg_left();
			return -1;
		}

		//msg_write("-program");
		int p=glCreateProgram();
		if (p<=0){
			msg_error("could not create gl shader program");
			msg_left();
			return -1;
		}

		//msg_write("-attach");
		glAttachShader(p,s);

		//msg_write("-link");
		glLinkProgram(p);
		glGetProgramiv(p,GL_LINK_STATUS,&status);
		//msg_write(status);
		if (status!=GL_TRUE){
			msg_error("while linking the shader program...");
			glGetProgramInfoLog(s,sizeof(ErrStr),&size,ErrStr);
			msg_write(ErrStr);
			HuiErrorBox(NixWindow,"error linking shader file",ErrStr);
			msg_left();
			return -1;
		}
		OGLShader[NumShaderFiles++]=p;
		msg_write("ok?");

		msg_left();
		return NumShaderFiles-1;
#endif
	}
#endif
	return -1;
}

void NixDeleteShaderFile(int index)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if ((index<0)||(index>=NumShaderFiles))
			return;
		if (!DXEffect[index])
			return;

		DXEffect[index]->Release();
		DXEffect[index]=NULL;
		msg_write(string("deleted shader file ",i2s(index)));
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("NixDeleteShaderFile for OpenGL");
	}
#endif
}

void NixSetShaderFile(int index)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (DXEffectCurrent){
			DXEffectCurrent->End();
			DXEffectCurrent=NULL;
		}
		if ((index<0)||(index>=NumShaderFiles))
			return;

		DXEffectCurrent=NULL;
		DXEffectCurrentNumPasses;
		if (SUCCEEDED(DXEffect[index]->SetTechnique(DXEffectTech[index]))){
			DXEffectCurrent=DXEffect[index];
			DXEffect[index]->Begin(&DXEffectCurrentNumPasses,0);
		}else{
			msg_error("could not set the shader technique");
		}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
#ifdef NIX_OS_LINUX
		if (index>=0)
			OGLShaderCurrent=OGLShader[index];
		else
			OGLShaderCurrent=0;
		glUseProgram(OGLShaderCurrent);
#endif
	}
#endif
}

void NixSetShaderData(int index,const char *var_name,const void *data,int size)
{
	if (index<0)
		return;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXHANDLE hdl;
		if (hdl=DXEffect[index]->GetParameterByName(NULL,"WorldViewProjectionMatrix"))
			DXEffect[index]->SetValue(hdl,data,size);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("NixSetShaderData for OpenGL");
		/*int loc = glGetUniformLocationARB(my_program, “my_color_texture”);

glActiveTexture(GL_TEXTURE0 + i);
glBindTexture(GL_TEXTURE_2D, my_texture_object);

glUniform1iARB(my_sampler_uniform_location, i);*/

	}
#endif
}

void NixGetShaderData(int index,const char *var_name,void *data,int size)
{
	if (index<0)
		return;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXHANDLE hdl;
		if (hdl=DXEffect[index]->GetParameterByName(NULL,"WorldViewProjectionMatrix"))
			DXEffect[index]->GetValue(hdl,data,size);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("NixGetShaderData for OpenGL");
	}
#endif
}

bool AllowWindowsKeyInput=false;

// Eingaben vonm Fenster entgegennehmen
void NixGetInputFromWindow()
{
	if (!NixWindow)
		return;

	#ifdef HUI_API_WIN
		int i;
		POINT mpos;
		UINT message=NixWindow->CompleteWindowMessage.msg;		NixWindow->CompleteWindowMessage.msg=0;
		WPARAM wParam=NixWindow->CompleteWindowMessage.wparam;	NixWindow->CompleteWindowMessage.wparam=0;
		LPARAM lParam=NixWindow->CompleteWindowMessage.lparam;	NixWindow->CompleteWindowMessage.lparam=0;
		//NixWindow->InputData=NixInputDataCurrent;

		int mycd=0;
		RECT ToolBarRect;

		switch(message){
			case WM_MOUSEMOVE:
				// correction due to toolbar?
				if (NixWindow->tool_bar[HuiToolBarTop].Enabled){
					//SendMessage(NixWindow->tool_bar[HuiToolBarTop].hWnd,TB_AUTOSIZE,0,0);
					GetWindowRect(NixWindow->tool_bar[HuiToolBarTop].hWnd,&ToolBarRect);
					mycd=-ToolBarRect.bottom+ToolBarRect.top;
				}
				if (NixFullscreen){
					GetCursorPos(&mpos);
					NixWindow->InputData.dx+=(float)mpos.x-NixScreenWidth/2.0f;
					NixWindow->InputData.dy+=(float)mpos.y-NixScreenHeight/2.0f;
					SetCursorPos(NixScreenWidth/2,NixScreenHeight/2);
					// korrekte Mausposition
					/*NixWindow->InputData.mx=NixInputDataCurrent.mx+NixWindow->InputData.vx;
					NixWindow->InputData.my=NixInputDataCurrent.my+NixWindow->InputData.vy;*/
					// praktischere Mausposition
					NixWindow->InputData.x=NixInputDataCurrent.x+float(NixWindow->InputData.dx)/NixMouseMappingWidth*float(NixScreenWidth);
					NixWindow->InputData.y=NixInputDataCurrent.y+float(NixWindow->InputData.dy)/NixMouseMappingHeight*float(NixScreenHeight);
				}else{
					NixWindow->InputData.x=(float)LOWORD(lParam);
					NixWindow->InputData.y=(float)HIWORD(lParam)+mycd;
					if (NixWindow->InputData.x>32000)			NixWindow->InputData.x=0;
					if (NixWindow->InputData.y>32000)			NixWindow->InputData.y=0;
				}
				if (NixWindow->InputData.x<0)				NixWindow->InputData.x=0;
				if (NixWindow->InputData.y<0)				NixWindow->InputData.y=0;
				if (NixWindow->InputData.x>NixTargetWidth)	NixWindow->InputData.x=(float)NixTargetWidth;
				if (NixWindow->InputData.y>NixTargetHeight)	NixWindow->InputData.y=(float)NixTargetHeight;
				if (!NixFullscreen){
					NixWindow->InputData.dx=NixWindow->InputData.x-NixInputDataLast.x;
					NixWindow->InputData.dy=NixWindow->InputData.y-NixInputDataLast.y;
				}
				break;
			case WM_MOUSEWHEEL:
				NixWindow->InputData.dz+=(short)HIWORD(wParam);
				break;
			case WM_LBUTTONDOWN:
				SetCapture(NixWindow->hWnd);
				NixWindow->InputData.lb=true;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_LBUTTONUP:
				ReleaseCapture();
				NixWindow->InputData.lb=false;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_MBUTTONDOWN:
				SetCapture(NixWindow->hWnd);
				NixWindow->InputData.mb=true;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_MBUTTONUP:
				ReleaseCapture();
				NixWindow->InputData.mb=false;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_RBUTTONDOWN:
				SetCapture(NixWindow->hWnd);
				NixWindow->InputData.rb=true;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_RBUTTONUP:
				ReleaseCapture();
				NixWindow->InputData.rb=false;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_KEYDOWN:
				if (GetActiveWindow()==NixWindow->hWnd){
					AllowWindowsKeyInput=true;
					if (NixWindow->InputData.KeyBufferDepth>=HUI_MAX_KEYBUFFER_DEPTH-1){
						for (i=0;i<NixWindow->InputData.KeyBufferDepth-2;i++)
							NixWindow->InputData.KeyBuffer[i]=NixWindow->InputData.KeyBuffer[i+1];
						NixWindow->InputData.KeyBufferDepth--;
					}
					NixWindow->InputData.KeyBuffer[NixWindow->InputData.KeyBufferDepth]=HuiKeyID[wParam];
					NixWindow->InputData.KeyBufferDepth++;
				}
				break;
			/*case WM_KEYDOWN:
				key[wParam]=true;
				break;
			case WM_KEYUP:
				key[wParam]=false;
				break;*/
			/*case WM_SIZE:
				NixResize();
				break;*/
		}
		NixWindow->InputData.mw=(float)atan2(-NixWindow->InputData.dy-NixInputDataLast.dy,NixWindow->InputData.dx+NixInputDataLast.dx);
		if (NixWindow->InputData.mw<0)
			NixWindow->InputData.mw+=2*pi;

		if (GetActiveWindow()!=NixWindow->hWnd)
			AllowWindowsKeyInput=false;
	
		if (AllowWindowsKeyInput)
			for (i=0;i<256;i++)
				NixWindow->InputData.key[HuiKeyID[i]]=((GetAsyncKeyState(i)&(1<<15))!=0);
		else{
			for (i=0;i<256;i++)
				NixWindow->InputData.key[i]=false;
		}

		// Korrektur (manche Tasten belegen mehrere Array-Elemente) :-S
		if (NixGetKey(KEY_RALT))
			NixWindow->InputData.key[KEY_LCONTROL]=0;
	#endif
}

bool allow_mb=false;
int gdk_mx=0,gdk_my=0;

// Eingaben behandeln
void NixUpdateInput()
{
	NixInputDataLast = NixInputDataCurrent;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		NixInputDataCurrent = NixWindow->InputData;
	}
#endif
#ifdef NIX_API_OPENGL
	#ifdef NIX_OS_WINDOWS
		if (NixApi==NIX_API_OPENGL){
			NixInputDataCurrent = NixWindow->InputData;
		}
	#endif
	#ifdef NIX_OS_LINUX
	
		if (NixFullscreen){
			int mx, my, mod;
			NixInputDataCurrent = NixWindow->InputData;
		//GdkWindow *pw=gdk_window_get_pointer(NixWindow->window->window,&mx,&my,(GdkModifierType*)&mod);
			GdkWindow *pw = gdk_window_get_pointer(NixWindow->gl_widget->window, &mx, &my, (GdkModifierType*)&mod);
			NixInputDataCurrent.dx = mx - gdk_mx;
			NixInputDataCurrent.dy = my - gdk_my;
			NixInputDataCurrent.x=NixInputDataLast.x+NixInputDataCurrent.dx;
			NixInputDataCurrent.y=NixInputDataLast.y+NixInputDataCurrent.dy;
			clampf(NixInputDataCurrent.x, 0, NixScreenWidth-1);
			clampf(NixInputDataCurrent.y, 0, NixScreenHeight-1);
			// reset....
			Window win=GDK_WINDOW_XWINDOW(NixWindow->gl_widget->window);
			XWarpPointer(hui_x_display, None, win, 0, 0, 0, 0, NixScreenWidth/2, NixScreenHeight/2);
			XFlush(hui_x_display);
			GdkWindow *pw2=gdk_window_get_pointer(NixWindow->window->window,&gdk_mx,&gdk_my,(GdkModifierType*)&mod);
			
			/*int mx,my, x0, x1, y0, y1;
			// old pos
			mx = NixInputDataCurrent.x;
			my = NixInputDataCurrent.y;
			// iterate
			NixInputDataCurrent = NixWindow->InputData;
			// get pos change
			x0 = NixWindow->InputData.x;
			y0 = NixWindow->InputData.y;
			printf("0: %d %d\n", x0, y0);
			NixWindow->SetCursorPos(NixScreenWidth / 2, NixScreenHeight / 2);
			x1 = NixWindow->InputData.x;
			y1 = NixWindow->InputData.y;
			printf("1: %d %d\n", x1, y1);
			NixInputDataCurrent.dx = x0 - x1;
			NixInputDataCurrent.dy = y0 - y1;
			// new pos
			mx += NixInputDataCurrent.dx;
			my += NixInputDataCurrent.dy;
			//clampi(mx, 0, NixScreenWidth);
			//clampi(my, 0, NixScreenHeight);
			NixInputDataCurrent.x = mx;
			NixInputDataCurrent.y = my;*/
		}else{
			NixInputDataCurrent = NixWindow->InputData;
			clampf(NixInputDataCurrent.x, 0, NixTargetWidth-1);
			clampf(NixInputDataCurrent.y, 0, NixTargetHeight-1);
			NixInputDataCurrent.dx = NixInputDataCurrent.x - NixInputDataLast.x;
			NixInputDataCurrent.dy = NixInputDataCurrent.y - NixInputDataLast.y;
		}
	#endif
#endif

	// noch nicht so ganz...
	//NixWindow->InputData.lb=NixWindow->InputData.mb=NixWindow->InputData.rb=false;
	NixWindow->InputData.dx = NixWindow->InputData.dy = NixWindow->InputData.dz = 0;

	if (!KeyBufferRead){
		NixWindow->InputData.KeyBufferDepth = 0;
	}
	KeyBufferRead = false;
}

void NixResetInput()
{
	memset(&NixInputDataCurrent,0,sizeof(sInputData));
	//NixInputDataCurrent.mx=NixScreenWidth/2;
	//NixInputDataCurrent.my=NixScreenHeight/2;
	NixUpdateInput();
	NixInputDataLast=NixInputDataCurrent;
}

void NixDraw3D(int texture,int buffer,const matrix *mat)
{
	if (buffer<0)	return;
	// keine Matrix angegeben -> Matrix ohne Transformation
	if (!mat)
		mat = &identity_matrix;
	NixSetTexture(texture);
	// Transformations-Matrix Modell->Welt
	NixSetWorldMatrix(*mat);

	SetShaderFileData(texture,-1,-1,-1);

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){

		DXSet3DMode();
		lpDevice->SetFVF(D3D_VERTEX3D);
		// Vertex-Punkte uebergeben
		lpDevice->SetStreamSource(0,DXVBVertices[buffer],0,sizeof(DXVertex3D));

		if (VBIndexed[buffer]){
			lpDevice->SetIndices(DXVBIndex[buffer]);
			// darstellen (je 3 im IndexBuffer angegebene Punkte zu einem Dreieck zufassen)
			if (DXEffectCurrent){ // loop through the shader "passes"
				for (unsigned int i=0;i<DXEffectCurrentNumPasses;i++){
					DXEffectCurrent->BeginPass(i);
					lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,VBNumPoints[buffer],0,VBNumTrias[buffer]);
					DXEffectCurrent->EndPass();
				}
			}else // draw a single time without the shader
				lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,VBNumPoints[buffer],0,VBNumTrias[buffer]);
		}else{
			// darstellen (je 3 aufeinander folgende Punkte zu einem Dreieck zufassen)
			if (DXEffectCurrent){ // loop through the shader "passes"
				for (unsigned int i=0;i<DXEffectCurrentNumPasses;i++){
					DXEffectCurrent->BeginPass(i);
					lpDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,VBNumTrias[buffer]);
					DXEffectCurrent->EndPass();
				}
			}else // draw a single time without the shader
				lpDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,VBNumTrias[buffer]);
		}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLSet3DMode();
#if 1
		glEnableClientState( GL_VERTEX_ARRAY );
		glEnableClientState( GL_NORMAL_ARRAY );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glVertexPointer( 3, GL_FLOAT, 0, OGLVBVertices[buffer] );
		glNormalPointer( GL_FLOAT, 0, OGLVBNormals[buffer] );
#ifdef NIX_OS_LINUX
		glClientActiveTexture(GL_TEXTURE0_ARB);
#endif
		glTexCoordPointer( 2, GL_FLOAT, 0, OGLVBTexCoords[buffer][0] );
		glDrawArrays(GL_TRIANGLES,0,VBNumTrias[buffer]*3);
		//glDrawArrays(GL_TRIANGLE_STRIP,0,VBNumTrias[buffer]*3);
#else

		for (int i=0;i<VBNumTrias[buffer];i++){
			glBegin(GL_TRIANGLES);
				glTexCoord2f(	OGLVBVertices[buffer][i*3  ].tu,OGLVBVertices[buffer][i*3  ].tv);
				glNormal3f(		OGLVBVertices[buffer][i*3  ].nx,OGLVBVertices[buffer][i*3  ].ny,OGLVBVertices[buffer][i*3  ].nz);
				glVertex3f(		OGLVBVertices[buffer][i*3  ].x ,OGLVBVertices[buffer][i*3  ].y ,OGLVBVertices[buffer][i*3  ].z );
				glTexCoord2f(	OGLVBVertices[buffer][i*3+1].tu,OGLVBVertices[buffer][i*3+1].tv);
				glNormal3f(		OGLVBVertices[buffer][i*3+1].nx,OGLVBVertices[buffer][i*3+1].ny,OGLVBVertices[buffer][i*3+1].nz);
				glVertex3f(		OGLVBVertices[buffer][i*3+1].x ,OGLVBVertices[buffer][i*3+1].y ,OGLVBVertices[buffer][i*3+1].z );
				glTexCoord2f(	OGLVBVertices[buffer][i*3+2].tu,OGLVBVertices[buffer][i*3+2].tv);
				glNormal3f(		OGLVBVertices[buffer][i*3+2].nx,OGLVBVertices[buffer][i*3+2].ny,OGLVBVertices[buffer][i*3+2].nz);
				glVertex3f(		OGLVBVertices[buffer][i*3+2].x ,OGLVBVertices[buffer][i*3+2].y ,OGLVBVertices[buffer][i*3+2].z );
			glEnd();
		}
#endif
	}
#endif
	NixNumTrias+=VBNumTrias[buffer];
}

void NixDraw3DM(int *texture,int buffer,const matrix *mat)
{
	if (buffer<0)	return;
	matrix m;
	// keine Matrix angegeben -> Matrix ohne Transformation
	if (!mat)
		mat = &identity_matrix;
	NixSetTextures(texture,VBNumTextures[buffer]);
	// Transformations-Matrix Modell->Welt
	NixSetWorldMatrix(*mat);

	SetShaderFileData(texture[0],texture[1],texture[2],texture[3]);

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){

		DXSet3DMode();
		// Vertex-Punkt uebergeben
		if (VBNumTextures[buffer]==2){
			lpDevice->SetFVF(D3D_VERTEX3D2);
			lpDevice->SetStreamSource(0,DXVBVertices[buffer],0,sizeof(DXVertex3D2));
		}else if (VBNumTextures[buffer]==3){
			lpDevice->SetFVF(D3D_VERTEX3D3);
			lpDevice->SetStreamSource(0,DXVBVertices[buffer],0,sizeof(DXVertex3D3));
		}else if (VBNumTextures[buffer]==4){
			lpDevice->SetFVF(D3D_VERTEX3D4);
			lpDevice->SetStreamSource(0,DXVBVertices[buffer],0,sizeof(DXVertex3D4));
		}

		if (VBIndexed[buffer]){
			lpDevice->SetIndices(DXVBIndex[buffer]);
			// darstellen (je 3 im IndexBuffer angegebene Punkte zu einem Dreieck zufassen)
			if (DXEffectCurrent){ // loop through the shader "passes"
				for (unsigned int i=0;i<DXEffectCurrentNumPasses;i++){
					DXEffectCurrent->BeginPass(i);
					lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,VBNumPoints[buffer],0,VBNumTrias[buffer]);
					DXEffectCurrent->EndPass();
				}
			}else // draw a single time without the shader
				lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,VBNumPoints[buffer],0,VBNumTrias[buffer]);
		}else{
			// darstellen (je 3 aufeinander folgende Punkte zu einem Dreieck zufassen)
			if (DXEffectCurrent){ // loop through the shader "passes"
				for (unsigned int i=0;i<DXEffectCurrentNumPasses;i++){
					DXEffectCurrent->BeginPass(i);
					lpDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,VBNumTrias[buffer]);
					DXEffectCurrent->EndPass();
				}
			}else // draw a single time without the shader
				lpDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,VBNumTrias[buffer]);
		}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLSet3DMode();
#if 1
		glEnableClientState( GL_VERTEX_ARRAY );
		glEnableClientState( GL_NORMAL_ARRAY );
		glVertexPointer( 3, GL_FLOAT, 0, OGLVBVertices[buffer] );
		glNormalPointer( GL_FLOAT, 0, OGLVBNormals[buffer] );
#ifdef NIX_OS_LINUX
		for (int i=0;i<VBNumTextures[buffer];i++){
			glActiveTexture(GL_TEXTURE0+i);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,OGLTexture[texture[i]]);
			glClientActiveTexture(GL_TEXTURE0+i);
			glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			glTexCoordPointer( 2, GL_FLOAT, 0, OGLVBTexCoords[buffer][i] );
		}
#else
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,OGLTexture[texture[0]]);
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, 0, OGLVBTexCoords[buffer][0] );
#endif
		glDrawArrays(GL_TRIANGLES,0,VBNumTrias[buffer]*3);
#ifdef NIX_OS_LINUX
		for (int i=1;i<VBNumTextures[buffer];i++){
			glActiveTexture(GL_TEXTURE0+i);
			glDisable(GL_TEXTURE_2D);
			glClientActiveTexture(GL_TEXTURE0+i);
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );
		}
		glActiveTexture(GL_TEXTURE0);
		glClientActiveTexture(GL_TEXTURE0);
#endif
#else

		for (int i=0;i<VBNumTrias[buffer];i++){
			glBegin(GL_TRIANGLES);
				glTexCoord2f(	OGLVBVertices[buffer][i*3  ].tu,OGLVBVertices[buffer][i*3  ].tv);
				glNormal3f(		OGLVBVertices[buffer][i*3  ].nx,OGLVBVertices[buffer][i*3  ].ny,OGLVBVertices[buffer][i*3  ].nz);
				glVertex3f(		OGLVBVertices[buffer][i*3  ].x ,OGLVBVertices[buffer][i*3  ].y ,OGLVBVertices[buffer][i*3  ].z );
				glTexCoord2f(	OGLVBVertices[buffer][i*3+1].tu,OGLVBVertices[buffer][i*3+1].tv);
				glNormal3f(		OGLVBVertices[buffer][i*3+1].nx,OGLVBVertices[buffer][i*3+1].ny,OGLVBVertices[buffer][i*3+1].nz);
				glVertex3f(		OGLVBVertices[buffer][i*3+1].x ,OGLVBVertices[buffer][i*3+1].y ,OGLVBVertices[buffer][i*3+1].z );
				glTexCoord2f(	OGLVBVertices[buffer][i*3+2].tu,OGLVBVertices[buffer][i*3+2].tv);
				glNormal3f(		OGLVBVertices[buffer][i*3+2].nx,OGLVBVertices[buffer][i*3+2].ny,OGLVBVertices[buffer][i*3+2].nz);
				glVertex3f(		OGLVBVertices[buffer][i*3+2].x ,OGLVBVertices[buffer][i*3+2].y ,OGLVBVertices[buffer][i*3+2].z );
			glEnd();
		}
#endif
	}
#endif
	NixNumTrias+=VBNumTrias[buffer];
}

void NixDraw3DCubeMapped(int cube_map,int buffer,const matrix *mat)
{
	if (buffer<0)	return;

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		NixSetCubeMapDX(cube_map);
		lpDevice->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
		matrix tm=NixViewMatrix;
		tm._03=tm._13=tm._23=tm._33=tm._30=tm._31=tm._32=0;
		/*quaternion q;
		QuaternionRotationM(q,tm);
		vector ang=QuaternionToAngle(q);
		MatrixRotationView(tm,ang);*/
		MatrixTranspose(tm,tm);
		lpDevice->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT3);

		lpDevice->SetTransform(D3DTS_TEXTURE0,(D3DMATRIX*)&tm);


		NixDraw3D(-2,buffer,mat);
		lpDevice->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_PASSTHRU);
		lpDevice->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("Draw3DCubeMapped for OpenGL");

		//Draw3D(-1,buffer,mat);
	}
#endif
}

// world -> screen (0...NixTargetWidth,0...NixTargetHeight,0...1)
void NixGetVecProject(vector &vout,const vector &vin)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXVec3Project((D3DXVECTOR3*)&vout,(D3DXVECTOR3*)&vin,&DXViewPort,(D3DXMATRIX*)&NixProjectionMatrix,(D3DXMATRIX*)&NixViewMatrix,NULL);
		/*VecTransform(vout,NixViewMatrix,vin);
		VecTransform(vout,NixProjectionMatrix,vout);*/
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		/*matrix m;
		MatrixIdentity(m);*/
		double vm[16];
		int i;
		for (i=0;i<16;i++)
			vm[i]=NixViewMatrix.e[i];
			//vm[i]=m.e[i];
		double pm[16];
		for (i=0;i<16;i++)
				pm[i]=NixProjectionMatrix.e[i];
			//pm[i]=m.e[i];
		double x,y,z;
		gluProject(vin.x,vin.y,vin.z,vm,pm,OGLViewPort,&x,&y,&z);
		vout.x=(float)x;
		vout.y=float((OGLViewPort[1]*2+OGLViewPort[3])-y); // y-Spiegelung
		vout.z=(float)z;//0.999999970197677613f;//(float)z;
		/*VecTransform(vout,NixViewMatrix,vin);
		VecTransform(vout,NixProjectionMatrix,vout);
		vout.y=((ViewPort[1]*2+ViewPort[3])-vout.y*16)/2;
		vout.x=((ViewPort[0]*2+ViewPort[2])+vout.x*16)/2;
		vout.z=0.99999997f;*/
	}
#endif
}

// world -> screen (0...1,0...1,0...1)
void NixGetVecProjectRel(vector &vout,const vector &vin)
{
	NixGetVecProject(vout,vin);
	vout.x/=(float)NixTargetWidth;
	vout.y/=(float)NixTargetHeight;
}

// screen (0...NixTargetWidth,0...NixTargetHeight,0...1) -> world
void NixGetVecUnproject(vector &vout,const vector &vin)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXVec3Unproject((D3DXVECTOR3*)&vout,(D3DXVECTOR3*)&vin,&DXViewPort,(D3DXMATRIX*)&NixProjectionMatrix,(D3DXMATRIX*)&NixViewMatrix,NULL);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		double vin_y=OGLViewPort[1]*2+OGLViewPort[3]-(double)vin.y; // y-Spiegelung
		double vm[16];
		int i;
		for (i=0;i<16;i++)
			vm[i]=NixViewMatrix.e[i];
		double pm[16];
		for (i=0;i<16;i++)
			pm[i]=NixProjectionMatrix.e[i];
		double x,y,z;
		gluUnProject(vin.x,vin_y,vin.z,vm,pm,OGLViewPort,&x,&y,&z);
		vout.x=(float)x;
		vout.y=(float)y;
		vout.z=(float)z;
	}
#endif
}

// screen (0...1,0...1,0...1) -> world
void NixGetVecUnprojectRel(vector &vout,const vector &vin)
{
	vector vi_r=vin;
	vi_r.x*=(float)NixTargetWidth;
	vi_r.y*=(float)NixTargetHeight;
	NixGetVecUnproject(vout,vi_r);
}

void NixDrawSpriteR(int texture,const color *col,const rect *src,const vector &pos,const rect *dest)
{
	rect d;
	float depth;
	vector p;
	NixGetVecProject(p,pos);
	if ((p.z<=0.0f)||(p.z>=1.0))
		return;
	depth=p.z;
	vector u;
	VecTransform(u,NixViewMatrix,pos);
	float q=NixMaxDepth/(NixMaxDepth-NixMinDepth);
	float f=1.0f/(u.z*q*NixMinDepth*View3DRatio);
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		//depth=depth*2.0f-1.0f;
		//f*=2;
	}
#endif
	//if (f>20)	f=20;
	d.x1=p.x+f*(dest->x1)*ViewScale.x*NixTargetWidth;
	d.x2=p.x+f*(dest->x2)*ViewScale.x*NixTargetWidth;
	d.y1=p.y+f*(dest->y1)*ViewScale.y*NixTargetHeight*View3DRatio;
	d.y2=p.y+f*(dest->y2)*ViewScale.y*NixTargetHeight*View3DRatio;
	NixDraw2D(texture,col,src,&d,depth);
}

void NixDrawSprite(int texture,const color *col,const rect *src,const vector &pos,float radius)
{
	rect d;
	d.x1=-radius;
	d.x2=radius;
	d.y1=-radius;
	d.y2=radius;
	NixDrawSpriteR(texture,col,src,pos,&d);
}

// bei angegebenem index wird der bestehende VB neu erstellt
int NixCreateVB(int max_trias,int index)
{
	bool create=(index<0);
	if (create){
		index=NumVBs;
		for (int i=0;i<NumVBs;i++)
			if (!VBUsed[i]){
				index=i;
				break;
			}
	}
	if (max_trias<=0) // unnoetiger Speicher
		return -1;
	msg_write("creating vertex buffer");
	if ((create)&&(index>=NIX_MAX_VBS)&&(NumVBs>=NIX_MAX_VBS)){
		msg_error("too many vertex buffers");
		return -1;
	}
	VBNumTextures[index]=1;
	msg_right();
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		// VertexBuffer selbst
		HRESULT hr;
		hr=lpDevice->CreateVertexBuffer(	3*max_trias*sizeof(DXVertex3D),
											D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
											D3D_VERTEX3D,
											D3DPOOL_DEFAULT,
											&DXVBVertices[index],
											NULL);
		if (!SUCCEEDED(hr))
			DXVBVertices[index]=NULL;
		if (!DXVBVertices[index]){
			msg_error("couldn't create vertex buffer");
			msg_write(DXErrorMsg(hr));
			return -1;
		}
		// IndexBuffer
		#ifdef ENABLE_INDEX_BUFFERS
		hr=lpDevice->CreateIndexBuffer(	2*3*max_trias, // 2byte * 3(Punkte pro Dreieck) * AnzahlDreiecke
										D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
										D3DFMT_INDEX16, // noch nicht mehr als 65536 Punkte!
										D3DPOOL_DEFAULT,
										&DXVBIndex[index],
										NULL);
		if (!SUCCEEDED(hr))
			DXVBIndex[index]=NULL;
		if (!DXVBIndex[index]){
			msg_error("couldn't create index buffer");
			msg_write(DXErrorMsg(hr));
			return -1;
		}
		#endif
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLVBVertices[index]=new vector[max_trias*3];
		OGLVBNormals[index]=new vector[max_trias*3];
		OGLVBTexCoords[index][0]=new float[max_trias*6];
		if ((!OGLVBVertices[index])||(!OGLVBNormals[index])||(!OGLVBTexCoords[index][0])){
			msg_error("couldn't create vertex buffer");
			return -1;
		}
		#ifdef ENABLE_INDEX_BUFFERS
			/*VBIndex[NumVBs]=new ...; // noch zu bearbeiten...
			if (!OGLVBIndex[index]){
				msg_error("IndexBuffer konnte nicht erstellt werden");
				return -1;
			}*/
		#endif
	}
#endif
	//msg_write(index);
	//msg_write(max_trias);
	VBNumTrias[index]=VBNumPoints[index]=0;
	VBMaxTrias[index]=max_trias;
	//VBMaxPoints[index]=max_trias;
	VBUsed[index]=true;
	if (create)
		NumVBs++;
	//msg_ok();
	msg_left();
	return index;
}

int NixCreateVBM(int max_trias,int num_textures,int index)
{
	bool create=(index<0);
	if (create){
		index=NumVBs;
		for (int i=0;i<NumVBs;i++)
			if (!VBUsed[i]){
				index=i;
				break;
			}
	}
	if (max_trias<=0) // unnecessary memory...
		return -1;
	msg_write(string2("creating vertex buffer (%d tex coords)",num_textures));
	if ((create)&&(NumVBs>=NIX_MAX_VBS)){
		msg_error("too many vertex buffers");
		return -1;
	}
	VBNumTextures[index]=num_textures;
	msg_right();
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		// VertexBuffer selbst
		HRESULT hr;
		int s0=0,ss=0;
		if (num_textures==2){
			s0=sizeof(DXVertex3D2);
			ss=D3D_VERTEX3D2;
		}
		if (num_textures==3){
			s0=sizeof(DXVertex3D3);
			ss=D3D_VERTEX3D3;
		}
		if (num_textures==4){
			s0=sizeof(DXVertex3D4);
			ss=D3D_VERTEX3D4;
		}
		hr=lpDevice->CreateVertexBuffer(	3 * max_trias * s0,
											D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
											ss,
											D3DPOOL_DEFAULT,
											&DXVBVertices[index],
											NULL);
		if (!SUCCEEDED(hr))
			DXVBVertices[index]=NULL;
		if (!DXVBVertices[index]){
			msg_error("couldn't create vertex buffer");
			msg_write(DXErrorMsg(hr));
			return -1;
		}
		// IndexBuffer
		#ifdef ENABLE_INDEX_BUFFERS
		hr=lpDevice->CreateIndexBuffer(	2*3*max_trias, // 2byte * 3(Punkte pro Dreieck) * AnzahlDreiecke
										D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
										D3DFMT_INDEX16, // noch nicht mehr als 65536 Punkte!
										D3DPOOL_DEFAULT,
										&DXVBIndex[index],
										NULL);
		if (!SUCCEEDED(hr))
			DXVBIndex[index]=NULL;
		if (!DXVBIndex[index]){
			msg_error("couldn't create index buffer");
			msg_write(DXErrorMsg(hr));
			return -1;
		}
		#endif
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLVBVertices[index]=new vector[max_trias*3];
		OGLVBNormals[index]=new vector[max_trias*3];
		bool failed=((!OGLVBVertices[index])||(!OGLVBNormals[index]));
		for (int i=0;i<num_textures;i++){
			OGLVBTexCoords[index][i]=new float[max_trias*6];
			failed = failed || (!OGLVBTexCoords[index][i]);
		}
		if (failed){
			msg_error("couldn't create vertex buffer");
			return -1;
		}
		#ifdef ENABLE_INDEX_BUFFERS
			/*VBIndex[NumVBs]=new ...; // noch zu bearbeiten...
			if (!OGLVBIndex[index]){
				msg_error("IndexBuffer konnte nicht erstellt werden");
				return -1;
			}*/
		#endif
	}
#endif
	VBNumTrias[index]=VBNumPoints[index]=0;
	VBMaxTrias[index]=max_trias;
	//VBMaxPoints[index]=max_trias;
	VBUsed[index]=true;
	if (create)
		NumVBs++;
	//msg_ok();
	msg_left();
	return index;
}

void NixDeleteVB(int buffer)
{
	if (buffer<0)
		return;
	msg_write("deleting vertex buffer");
	msg_right();
	//msg_write(buffer);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		//msg_write("release");
		DXVBVertices[buffer]->Release();
		//msg_write("del");
		//delete(DXVBVertices[buffer]);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		delete[](OGLVBVertices[buffer]);
		delete[](OGLVBNormals[buffer]);
		for (int i=0;i<VBNumTextures[buffer];i++)
			delete[](OGLVBTexCoords[buffer][i]);
	}
#endif
	VBUsed[buffer]=false;
	//msg_ok();
	msg_left();
}

bool NixVBAddTria(int buffer,	const vector &p1,const vector &n1,float tu1,float tv1,
								const vector &p2,const vector &n2,float tu2,float tv2,
								const vector &p3,const vector &n3,float tu3,float tv3)
{
	if (VBNumTrias[buffer]>VBMaxTrias[buffer]){
		msg_error("too many triangles in the vertex buffer!");
		msg_write(buffer);
		return false;
	}
	//msg_write("VertexBufferAddTriangle");
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		unsigned char *pVerts=NULL;
		DXVertex3D Vert[3];
		Vert[0].x=p1.x;	Vert[0].y=p1.y;	Vert[0].z=p1.z;	Vert[0].nx=n1.x;	Vert[0].ny=n1.y;	Vert[0].nz=n1.z;	Vert[0].tu=tu1;	Vert[0].tv=tv1;
		Vert[1].x=p2.x;	Vert[1].y=p2.y;	Vert[1].z=p2.z;	Vert[1].nx=n2.x;	Vert[1].ny=n2.y;	Vert[1].nz=n2.z;	Vert[1].tu=tu2;	Vert[1].tv=tv2;
		Vert[2].x=p3.x;	Vert[2].y=p3.y;	Vert[2].z=p3.z;	Vert[2].nx=n3.x;	Vert[2].ny=n3.y;	Vert[2].nz=n3.z;	Vert[2].tu=tu3;	Vert[2].tv=tv3;

		Vert[0].tu=tu1;	Vert[0].tv=tv1;
		Vert[1].tu=tu2;	Vert[1].tv=tv2;
		Vert[2].tu=tu3;	Vert[2].tv=tv3;

		DXVBVertices[buffer]->Lock(sizeof(DXVertex3D)*3*VBNumTrias[buffer],sizeof(DXVertex3D)*3,(void**)&pVerts,0);
		memcpy(pVerts,Vert,sizeof(DXVertex3D)*3);
		DXVBVertices[buffer]->Unlock();
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLVBVertices[buffer][VBNumTrias[buffer]*3  ]=p1;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3+1]=p2;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3+2]=p3;
		OGLVBNormals[buffer][VBNumTrias[buffer]*3  ]=n1;
		OGLVBNormals[buffer][VBNumTrias[buffer]*3+1]=n2;
		OGLVBNormals[buffer][VBNumTrias[buffer]*3+2]=n3;
		OGLVBTexCoords[buffer][0][VBNumTrias[buffer]*6  ]=tu1;
		OGLVBTexCoords[buffer][0][VBNumTrias[buffer]*6+1]=1-tv1;
		OGLVBTexCoords[buffer][0][VBNumTrias[buffer]*6+2]=tu2;
		OGLVBTexCoords[buffer][0][VBNumTrias[buffer]*6+3]=1-tv2;
		OGLVBTexCoords[buffer][0][VBNumTrias[buffer]*6+4]=tu3;
		OGLVBTexCoords[buffer][0][VBNumTrias[buffer]*6+5]=1-tv3;
	}
#endif
	VBNumTrias[buffer]++;
	VBIndexed[buffer]=false;
	return true;
}

bool NixVBAddTriaM(int buffer,	const vector &p1,const vector &n1,const float *t1,
								const vector &p2,const vector &n2,const float *t2,
								const vector &p3,const vector &n3,const float *t3)
{
	if (buffer<0)	return false;
	if (VBNumTrias[buffer]>VBMaxTrias[buffer]){
		msg_error("too many triangles in the vertex buffer!");
		return false;
	}
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		unsigned char *pVerts=NULL;
		if (VBNumTextures[buffer]==2){
			DXVertex3D2 Vert[3];
			Vert[0].x=p1.x;	Vert[0].y=p1.y;	Vert[0].z=p1.z;	Vert[0].nx=n1.x;	Vert[0].ny=n1.y;	Vert[0].nz=n1.z;	Vert[0].tu0=t1[0];	Vert[0].tv0=t1[1];
			Vert[1].x=p2.x;	Vert[1].y=p2.y;	Vert[1].z=p2.z;	Vert[1].nx=n2.x;	Vert[1].ny=n2.y;	Vert[1].nz=n2.z;	Vert[1].tu0=t2[0];	Vert[1].tv0=t2[1];
			Vert[2].x=p3.x;	Vert[2].y=p3.y;	Vert[2].z=p3.z;	Vert[2].nx=n3.x;	Vert[2].ny=n3.y;	Vert[2].nz=n3.z;	Vert[2].tu0=t3[0];	Vert[2].tv0=t3[1];

			Vert[0].tu1=t1[2];	Vert[0].tv1=t1[3];
			Vert[1].tu1=t2[2];	Vert[1].tv1=t2[3];
			Vert[2].tu1=t3[2];	Vert[2].tv1=t3[3];

			DXVBVertices[buffer]->Lock(sizeof(DXVertex3D2)*3*VBNumTrias[buffer],sizeof(DXVertex3D2)*3,(void**)&pVerts,0);
			memcpy(pVerts,Vert,sizeof(DXVertex3D2)*3);
		}else if (VBNumTextures[buffer]==3){
			DXVertex3D3 Vert[3];
			Vert[0].x=p1.x;	Vert[0].y=p1.y;	Vert[0].z=p1.z;	Vert[0].nx=n1.x;	Vert[0].ny=n1.y;	Vert[0].nz=n1.z;	Vert[0].tu0=t1[0];	Vert[0].tv0=t1[1];
			Vert[1].x=p2.x;	Vert[1].y=p2.y;	Vert[1].z=p2.z;	Vert[1].nx=n2.x;	Vert[1].ny=n2.y;	Vert[1].nz=n2.z;	Vert[1].tu0=t2[0];	Vert[1].tv0=t2[1];
			Vert[2].x=p3.x;	Vert[2].y=p3.y;	Vert[2].z=p3.z;	Vert[2].nx=n3.x;	Vert[2].ny=n3.y;	Vert[2].nz=n3.z;	Vert[2].tu0=t3[0];	Vert[2].tv0=t3[1];

			Vert[0].tu1=t1[2];	Vert[0].tv1=t1[3];
			Vert[1].tu1=t2[2];	Vert[1].tv1=t2[3];
			Vert[2].tu1=t3[2];	Vert[2].tv1=t3[3];

			Vert[0].tu2=t1[4];	Vert[0].tv2=t1[5];
			Vert[1].tu2=t2[4];	Vert[1].tv2=t2[5];
			Vert[2].tu2=t3[4];	Vert[2].tv2=t3[5];

			DXVBVertices[buffer]->Lock(sizeof(DXVertex3D3)*3*VBNumTrias[buffer],sizeof(DXVertex3D3)*3,(void**)&pVerts,0);
			memcpy(pVerts,Vert,sizeof(DXVertex3D3)*3);
		}else if (VBNumTextures[buffer]==4){
			DXVertex3D4 Vert[3];
			Vert[0].x=p1.x;	Vert[0].y=p1.y;	Vert[0].z=p1.z;	Vert[0].nx=n1.x;	Vert[0].ny=n1.y;	Vert[0].nz=n1.z;	Vert[0].tu0=t1[0];	Vert[0].tv0=t1[1];
			Vert[1].x=p2.x;	Vert[1].y=p2.y;	Vert[1].z=p2.z;	Vert[1].nx=n2.x;	Vert[1].ny=n2.y;	Vert[1].nz=n2.z;	Vert[1].tu0=t2[0];	Vert[1].tv0=t2[1];
			Vert[2].x=p3.x;	Vert[2].y=p3.y;	Vert[2].z=p3.z;	Vert[2].nx=n3.x;	Vert[2].ny=n3.y;	Vert[2].nz=n3.z;	Vert[2].tu0=t3[0];	Vert[2].tv0=t3[1];

			Vert[0].tu1=t1[2];	Vert[0].tv1=t1[3];
			Vert[1].tu1=t2[2];	Vert[1].tv1=t2[3];
			Vert[2].tu1=t3[2];	Vert[2].tv1=t3[3];

			Vert[0].tu2=t1[4];	Vert[0].tv2=t1[5];
			Vert[1].tu2=t2[4];	Vert[1].tv2=t2[5];
			Vert[2].tu2=t3[4];	Vert[2].tv2=t3[5];

			Vert[0].tu3=t1[6];	Vert[0].tv3=t1[7];
			Vert[1].tu3=t2[6];	Vert[1].tv3=t2[7];
			Vert[2].tu3=t3[6];	Vert[2].tv3=t3[7];

			DXVBVertices[buffer]->Lock(sizeof(DXVertex3D4)*3*VBNumTrias[buffer],sizeof(DXVertex3D4)*3,(void**)&pVerts,0);
			memcpy(pVerts,Vert,sizeof(DXVertex3D4)*3);
		}
		DXVBVertices[buffer]->Unlock();
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLVBVertices[buffer][VBNumTrias[buffer]*3  ]=p1;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3+1]=p2;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3+2]=p3;
		OGLVBNormals[buffer][VBNumTrias[buffer]*3  ]=n1;
		OGLVBNormals[buffer][VBNumTrias[buffer]*3+1]=n2;
		OGLVBNormals[buffer][VBNumTrias[buffer]*3+2]=n3;
		for (int i=0;i<VBNumTextures[buffer];i++){
			OGLVBTexCoords[buffer][i][VBNumTrias[buffer]*6  ]=t1[i*2  ];
			OGLVBTexCoords[buffer][i][VBNumTrias[buffer]*6+1]=1-t1[i*2+1];
			OGLVBTexCoords[buffer][i][VBNumTrias[buffer]*6+2]=t2[i*2  ];
			OGLVBTexCoords[buffer][i][VBNumTrias[buffer]*6+3]=1-t2[i*2+1];
			OGLVBTexCoords[buffer][i][VBNumTrias[buffer]*6+4]=t3[i*2  ];
			OGLVBTexCoords[buffer][i][VBNumTrias[buffer]*6+5]=1-t3[i*2+1];
		}
	}
#endif
	VBNumTrias[buffer]++;
	VBIndexed[buffer]=false;
	return true;
}

// for each triangle there have to be 3 vertices (p[i],n[i],t[i*2],t[i*2+1])
void NixVBAddTrias(int buffer,int num_trias,const vector *p,const vector *n,const float *t)
{
	#ifdef NIX_API_DIRECTX9
		if (NixApi==NIX_API_DIRECTX9){
			// fill our vertex buffer
			unsigned char *pVerts=NULL;
			DXVertex3D Vert;
			DXVBVertices[buffer]->Lock(0,sizeof(DXVertex3D)*num_trias*3,(void**)&pVerts,0);
			for (int i=0;i<num_trias*3;i++){
				Vert.x=p[i].x;	Vert.y=p[i].y;	Vert.z=p[i].z;
				Vert.nx=n[i].x;	Vert.ny=n[i].y;	Vert.nz=n[i].z;
				Vert.tu=t[i*2];	Vert.tv=t[i*2+1];
				//memcpy(pVerts,&Vert,sizeof(DXVertex3D));
				*(DXVertex3D*)pVerts=Vert;
				pVerts+=sizeof(DXVertex3D);
			}
			DXVBVertices[buffer]->Unlock();
		}
	#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		memcpy(OGLVBVertices[buffer],p,sizeof(vector)*num_trias*3);
		memcpy(OGLVBNormals[buffer],n,sizeof(vector)*num_trias*3);
		//memcpy(OGLVBTexCoords[buffer][0],t,sizeof(float)*num_trias*6);
		for (int i=0;i<num_trias*3;i++){
			OGLVBTexCoords[buffer][0][i*2  ]=t[i*2];
			OGLVBTexCoords[buffer][0][i*2+1]=t[i*2+1];
		}
	}
#endif
	VBNumTrias[buffer]+=num_trias;
	VBNumPoints[buffer]+=num_trias*3;
}

void NixVBAddTriasIndexed(int buffer,int num_points,int num_trias,const vector *p,const vector *n,const float *tu,const float *tv,const unsigned short *indices)
{
	#ifdef ENABLE_INDEX_BUFFERS
		#ifdef NIX_API_DIRECTX9
		if (NixApi==NIX_API_DIRECTX9){
			int i;
			// VertexBuffer
			unsigned char *pVerts=NULL;
			Vertex3D Vert;
			VBVertices[buffer]->Lock(0,sizeof(Vertex3D)*num_points,&pVerts,0);
			for (i=0;i<num_points;i++){
				Vert.x=p[i].x;	Vert.y=p[i].y;	Vert.z=p[i].z;
				Vert.nx=n[i].x;	Vert.ny=n[i].y;	Vert.nz=n[i].z;
				Vert.tu=tu[i];	Vert.tv=tv[i];
				memcpy(pVerts,&Vert,sizeof(Vertex3D));
				pVerts+=sizeof(Vertex3D);
			}
			VBVertices[buffer]->Unlock();
			// IndexBuffer
			unsigned char *pIndex=NULL;
			VBIndex[buffer]->Lock(0,2*3*num_trias,&pIndex,0);
			for (i=0;i<num_trias*3;i++){
				pIndex[i*2  ]=indices[i]/256;
				pIndex[i*2+1]=indices[i]%256;
			}
			VBIndex[buffer]->Unlock();
		}
		#endif
		VBNumTrias[buffer]=num_trias;
		VBNumPoints[buffer]=num_points;
		VBIndexed[buffer]=true;
	#endif
}

void NixVBEmpty(int buffer)
{
	VBNumTrias[buffer]=0;
	VBNumPoints[buffer]=0;
	VBIndexed[buffer]=false;
}

void NixSetWire(bool enabled)
{
	WireFrame=enabled;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (WireFrame)
			lpDevice->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
		else
			lpDevice->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (WireFrame){
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
			glDisable(GL_CULL_FACE);
		}else{
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
			glEnable(GL_CULL_FACE);
		}
	}
#endif
}

void NixSetCull(int mode)
{
	// Sicht-Feld gespiegelt?
	if ((ViewScale.x*ViewScale.y*ViewScale.z<0)!=(NixCullingInverted)){
		if (mode==CullCCW)	mode=CullCW;
		else	if (mode==CullCW)	mode=CullCCW;
	}
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (mode==CullNone)		lpDevice->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
		if (mode==CullCCW)		lpDevice->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);
		if (mode==CullCW)		lpDevice->SetRenderState(D3DRS_CULLMODE,D3DCULL_CW);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		if (mode==CullNone)		glDisable(GL_CULL_FACE);
		if (mode==CullCCW)		glCullFace(GL_FRONT);
		if (mode==CullCW)		glCullFace(GL_BACK);
	}
#endif
}

void NixSetZ(bool Write,bool Test)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
		if (Test){
			lpDevice->SetRenderState(D3DRS_ZENABLE,TRUE);
			if (Write)
				lpDevice->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
			else
				lpDevice->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
		}else{
			if (Write){
				lpDevice->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
				lpDevice->SetRenderState(D3DRS_ZENABLE,TRUE);
				lpDevice->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
			}else{
				lpDevice->SetRenderState(D3DRS_ZENABLE,FALSE);
			}
		}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (Test){
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_DEPTH);
			glEnable(GL_DEPTH_TEST);
			if (Write)
				glDepthMask(1);
			else
				glDepthMask(0);
		}else{
			if (Write){
				glEnable(GL_DEPTH);
				//glDisable(GL_DEPTH_TEST);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_ALWAYS);
				glDepthMask(1);
			}else{
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_DEPTH);
			}
		}
	}
#endif
}

void NixSetAlpha(int mode)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
		lpDevice->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
		switch (mode){
			case AlphaNone:
				lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
				lpDevice->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
				break;
			case AlphaColorKey:
				lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
				lpDevice->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
				// flieï¿œnde Rand-ï¿œergï¿œge
				lpDevice->SetRenderState(D3DRS_ALPHAREF,0x08);
				lpDevice->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATEREQUAL);
				lpDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
				lpDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
				lpDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
				lpDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
				break;
			case AlphaColorKeyHard:
				//lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
				lpDevice->SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
				// fliessende Rand-Ueergaenge abschneiden
				lpDevice->SetRenderState(D3DRS_ALPHAREF,0x80);
				lpDevice->SetRenderState(D3DRS_ALPHAFUNC,D3DCMP_GREATEREQUAL);
				lpDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
				lpDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
				lpDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
				lpDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
				break;
			case AlphaMaterial:
				lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
				lpDevice->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);

				lpDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
				lpDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

				/*lpDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
				lpDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);*/
				lpDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
				lpDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
				lpDevice->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_TEXTURE);
				break;
		}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glDisable(GL_ALPHA_TEST);
		switch (mode){
			case AlphaNone:
				glDisable(GL_BLEND);
				break;
			case AlphaColorKey:
			case AlphaColorKeyHard:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_ALPHA_TEST);
				if (mode==AlphaColorKeyHard)
					glAlphaFunc(GL_GEQUAL,0.5f);
				else
					glAlphaFunc(GL_GEQUAL,0.04f);
				break;
			case AlphaMaterial:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
				break;
		}
	}
#endif
}

void NixSetAlphaM(int mode)
{	NixSetAlpha(mode);	}

#ifdef NIX_API_DIRECTX9
DWORD DXGetAlphaMode(int mode)
{
	if (mode==AlphaZero)			return D3DBLEND_ZERO;
	if (mode==AlphaOne)				return D3DBLEND_ONE;
	if (mode==AlphaSourceColor)		return D3DBLEND_SRCCOLOR;
	if (mode==AlphaSourceInvColor)	return D3DBLEND_INVSRCCOLOR;
	if (mode==AlphaSourceAlpha)		return D3DBLEND_SRCALPHA;
	if (mode==AlphaSourceInvAlpha)	return D3DBLEND_INVSRCALPHA;
	if (mode==AlphaDestColor)		return D3DBLEND_DESTCOLOR;
	if (mode==AlphaDestInvColor)	return D3DBLEND_INVDESTCOLOR;
	if (mode==AlphaDestAlpha)		return D3DBLEND_DESTALPHA;
	if (mode==AlphaDestInvAlpha)	return D3DBLEND_INVDESTALPHA;
	return D3DBLEND_ZERO;
}
#endif
#ifdef NIX_API_OPENGL
unsigned int OGLGetAlphaMode(int mode)
{
	if (mode==AlphaZero)			return GL_ZERO;
	if (mode==AlphaOne)				return GL_ONE;
	if (mode==AlphaSourceColor)		return GL_SRC_COLOR;
	if (mode==AlphaSourceInvColor)	return GL_ONE_MINUS_SRC_COLOR;
	if (mode==AlphaSourceAlpha)		return GL_SRC_ALPHA;
	if (mode==AlphaSourceInvAlpha)	return GL_ONE_MINUS_SRC_ALPHA;
	if (mode==AlphaDestColor)		return GL_DST_COLOR;
	if (mode==AlphaDestInvColor)	return GL_ONE_MINUS_DST_COLOR;
	if (mode==AlphaDestAlpha)		return GL_DST_ALPHA;
	if (mode==AlphaDestInvAlpha)	return GL_ONE_MINUS_DST_ALPHA;
	// GL_SRC_ALPHA_SATURATE
	return GL_ZERO;
}
#endif

void NixSetAlpha(int src,int dst)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
		lpDevice->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
		lpDevice->SetRenderState(D3DRS_SRCBLEND,DXGetAlphaMode(src));
		lpDevice->SetRenderState(D3DRS_DESTBLEND,DXGetAlphaMode(dst));
		lpDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
		lpDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
		lpDevice->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_TEXTURE);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glBlendFunc(OGLGetAlphaMode(src),OGLGetAlphaMode(dst));
	}
#endif
}

void NixSetAlphaSD(int src,int dst)
{	NixSetAlpha(src,dst);	}

void NixSetAlpha(float factor)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		DWORD f=0x00ffffff+( ((int)(factor*255.0f)) << 24 );
		lpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
		lpDevice->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);

		lpDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		lpDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

		lpDevice->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
		lpDevice->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TFACTOR);
		lpDevice->SetRenderState(D3DRS_TEXTUREFACTOR,f);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glDisable(GL_ALPHA_TEST);
		float di[4];
		glGetMaterialfv(GL_FRONT,GL_DIFFUSE,di);
		di[3]=factor;
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,di);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
#endif
}

void NixSetStencil(int mode,unsigned long param)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		DWORD dw=param;
		if (mode==StencilNone)
			lpDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
		if (mode==StencilReset)
			lpDevice->Clear(0,NULL,D3DCLEAR_STENCIL,D3DCOLOR_XRGB(0,0,0),1.0f,dw);
		if ((mode==StencilIncrease)||(mode==StencilDecrease)||(mode==StencilDecreaseNotNegative)||(mode==StencilSet)){
			lpDevice->SetRenderState( D3DRS_STENCILENABLE, TRUE );
			lpDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
			if (mode==99)//StencilDecreaseNotNegative)
				lpDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_GREATEREQUAL );
			lpDevice->SetRenderState( D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP );
			lpDevice->SetRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );
			lpDevice->SetRenderState( D3DRS_STENCILREF,       dw );
			lpDevice->SetRenderState( D3DRS_STENCILMASK,      0xffffffff );
			lpDevice->SetRenderState( D3DRS_STENCILWRITEMASK, 0xffffffff );
			if (mode==StencilIncrease)
				lpDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_INCR );
			if ((mode==StencilDecrease)||(mode==StencilDecreaseNotNegative))
				lpDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_DECR );
			if (mode==StencilSet)
				lpDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );
		}
		if ((mode==StencilMaskEqual)||(mode==StencilMaskLessEqual)||(mode==StencilMaskLess)||(mode==StencilMaskGreaterEqual)||(mode==StencilMaskGreater)){
			lpDevice->SetRenderState( D3DRS_STENCILENABLE, TRUE );
			lpDevice->SetRenderState( D3DRS_STENCILREF,  dw );
			lpDevice->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );
			if (mode==StencilMaskEqual)
				lpDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_EQUAL );
			if (mode==StencilMaskLessEqual)
				lpDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL );
			if (mode==StencilMaskLess)
				lpDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_LESS );
			if (mode==StencilMaskGreaterEqual)
				lpDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_GREATEREQUAL );
			if (mode==StencilMaskGreater)
				lpDevice->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_GREATER );
		}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glStencilMask(0xffffffff);
		if (mode==StencilNone){
			glDisable(GL_STENCIL);
			glDisable(GL_STENCIL_TEST);
		}
		if (mode==StencilReset)
			glClearStencil(param);
		if ((mode==StencilIncrease)||(mode==StencilDecrease)||(mode==StencilDecreaseNotNegative)||(mode==StencilSet)){
			glEnable(GL_STENCIL);
			//glDisable(GL_STENCIL_TEST);
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS,param,0xffffffff);
			if (mode==StencilIncrease)
				glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
			if ((mode==StencilDecrease)||(mode==StencilDecreaseNotNegative))
				glStencilOp(GL_KEEP,GL_KEEP,GL_DECR);
			if (mode==StencilSet)
				glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
		}
		if ((mode==StencilMaskEqual)||(mode==StencilMaskLessEqual)||(mode==StencilMaskLess)||(mode==StencilMaskGreaterEqual)||(mode==StencilMaskGreater)){
			glEnable(GL_STENCIL);
			glEnable(GL_STENCIL_TEST);
			glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
			if (mode==StencilMaskEqual)
				glStencilFunc(GL_EQUAL,param,0xffffffff);
			if (mode==StencilMaskLessEqual)
				glStencilFunc(GL_LEQUAL,param,0xffffffff);
			if (mode==StencilMaskLess)
				glStencilFunc(GL_LESS,param,0xffffffff);
			if (mode==StencilMaskGreaterEqual)
				glStencilFunc(GL_GEQUAL,param,0xffffffff);
			if (mode==StencilMaskGreater)
				glStencilFunc(GL_GREATER,param,0xffffffff);
		}
	}
#endif
}

void NixSetShading(int mode)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (mode==ShadingPlane)
			lpDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );
		if (mode==ShadingRound)
			lpDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (mode==ShadingPlane)
			glShadeModel(GL_FLAT);
		if (mode==ShadingRound)
			glShadeModel(GL_SMOOTH);
	}
#endif
}

void NixSetMaterial(const color &ambient,const color &diffuse,const color &specular,float shininess,const color &emission)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DMATERIAL9 Material;
		ZeroMemory(&Material,sizeof(D3DMATERIAL9));
		Material.Ambient	=color2D3DCOLORVALUE(ambient);
		Material.Diffuse	=color2D3DCOLORVALUE(diffuse);
		Material.Specular	=color2D3DCOLORVALUE(specular);
		Material.Power		=shininess;
		Material.Emissive	=color2D3DCOLORVALUE(emission);
		lpDevice->SetMaterial(&Material);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,(float*)&ambient);
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,(float*)&diffuse);
		glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,(float*)&specular);
		glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,(float*)&shininess);
		glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,(float*)&emission);
	}
#endif
}

void NixSpecularEnable(bool enabled)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetRenderState(D3DRS_SPECULARENABLE,enabled);
	}
#endif
}

// mode=FogLinear:			start/end
// mode=FogExp/FogExp2:		density
void NixSetFog(int mode,float start,float end,float density,const color &c)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		float f;
		//lpDevice->SetRenderState(D3DRS_RANGEFOGENABLE,TRUE);
		if (mode==FogLinear)		lpDevice->SetRenderState(D3DRS_FOGTABLEMODE,D3DFOG_LINEAR);
		else if (mode==FogExp)		lpDevice->SetRenderState(D3DRS_FOGTABLEMODE,D3DFOG_EXP);
		else if (mode==FogExp2)		lpDevice->SetRenderState(D3DRS_FOGTABLEMODE,D3DFOG_EXP2);
		f=density;
		lpDevice->SetRenderState(D3DRS_FOGDENSITY,*((DWORD*) (&f)));
		f=start;
		lpDevice->SetRenderState(D3DRS_FOGSTART,*((DWORD*) (&f)));
		f=end;
		lpDevice->SetRenderState(D3DRS_FOGEND,*((DWORD*) (&f)));
		D3DCOLOR C=color2D3DCOLOR(c);
		lpDevice->SetRenderState(D3DRS_FOGCOLOR,*((DWORD*) (&C)));
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (mode==FogLinear)		glFogi(GL_FOG_MODE,GL_LINEAR);
		else if (mode==FogExp)		glFogi(GL_FOG_MODE,GL_EXP);
		else if (mode==FogExp2)		glFogi(GL_FOG_MODE,GL_EXP2);
		glFogfv(GL_FOG_COLOR,(float*)&c);
		glFogf(GL_FOG_DENSITY,density);
		glFogf(GL_FOG_START,start);
		glFogf(GL_FOG_END,end);
		glHint(GL_FOG_HINT,GL_DONT_CARE); // ??
	}
#endif
}

void NixEnableFog(bool Enabled)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetRenderState(D3DRS_FOGENABLE,Enabled);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (Enabled)
			glEnable(GL_FOG);
		else
			glDisable(GL_FOG);
	}
#endif
}

void NixSetWorldMatrix(const matrix &mat)
{
	WorldMatrix=mat;
	MatrixMultiply(WorldViewProjectionMatrix,NixViewMatrix,WorldMatrix);
	MatrixMultiply(WorldViewProjectionMatrix,NixProjectionMatrix,WorldViewProjectionMatrix);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetTransform(D3DTS_WORLD,(D3DXMATRIX*)&mat);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((float*)&NixViewMatrix);
		glMultMatrixf((float*)&mat);
	}
#endif
}

void NixSetPerspectiveMode(int mode,float param1,float param2)
{
// width and height of the 3D projection
	if (mode==PerspectiveSizeAutoTarget){
		PerspectiveModeSize=mode;
		View3DWidth=float(NixTargetWidth);
		View3DHeight=float(NixTargetHeight);
	}
	if (mode==PerspectiveSizeAutoScreen){
		PerspectiveModeSize=mode;
		View3DWidth=float(NixScreenWidth);
		View3DHeight=float(NixScreenHeight);
	}
	if (mode==PerspectiveSizeSet){
		PerspectiveModeSize=mode;
		View3DWidth=param1;
		View3DHeight=param2;
	}
// vanishing point
	if (mode==PerspectiveCenterSet){
		PerspectiveModeCenter=mode;
		View3DCenterX=param1;
		View3DCenterY=param2;
	}
	if (mode==PerspectiveCenterAutoTarget){
		PerspectiveModeCenter=mode;
		View3DCenterX=float(NixTargetWidth)/2.0f;
		View3DCenterY=float(NixTargetHeight)/2.0f;
	}
// 2D transformation
	if (mode==Perspective2DScaleSet){
		PerspectiveMode2DScale=mode;
		View2DScaleX=param1;
		View2DScaleY=param2;
	}
// aspect ratio
	if (mode==PerspectiveRatioSet){
		//PerspectiveModeRatio=mode;
		View3DRatio=param1;
	}
}

static vector ViewPos,ViewDir;
static vector Frustrum[8];
static plane FrustrumPl[6];

void NixSetView(bool enable3d,vector view_pos,vector view_ang,vector scale)
{
	ViewPos=view_pos;
	ViewDir=VecAng2Dir(view_ang);
	ViewScale=scale;

	//if (enable3d){
		matrix t,r,s;
		MatrixTranslation(t,-view_pos);
		MatrixRotationView(r,view_ang);
		//MatrixScale(s,scale.x,scale.y,scale.z);
		MatrixMultiply(NixViewMatrix,r,t);
		//MatrixMultiply(NixViewMatrix,s,NixViewMatrix);
	//}
	NixSetView(enable3d,NixViewMatrix);

	// die Eckpunkte des Sichtfeldes
	/*NixGetVecUnproject(Frustrum[0],vector(                   0,                    0,0.0f));
	NixGetVecUnproject(Frustrum[1],vector(float(NixScreenWidth-1),                    0,0.0f));
	NixGetVecUnproject(Frustrum[2],vector(                   0,float(NixScreenHeight-1),0.0f));
	NixGetVecUnproject(Frustrum[3],vector(float(NixScreenWidth-1),float(NixScreenHeight-1),0.0f));
	NixGetVecUnproject(Frustrum[4],vector(                   0,                    0,0.9f));
	NixGetVecUnproject(Frustrum[5],vector(float(NixScreenWidth-1),                    0,0.9f));
	NixGetVecUnproject(Frustrum[6],vector(                   0,float(NixScreenHeight-1),0.9f));
	NixGetVecUnproject(Frustrum[7],vector(float(NixScreenWidth-1),float(NixScreenHeight-1),0.9f));

	// Ebenen des Sichtfeldes (gegen UZS nach innen!?)
	PlaneFromPoints(FrustrumPl[0],Frustrum[0],Frustrum[1],Frustrum[2]); // nahe Ebene
	//PlaneFromPoints(FrustrumPl[1],Frustrum[4],Frustrum[6],Frustrum[7]); // ferne Ebene
	//PlaneFromPoints(FrustrumPl[2],Frustrum[0],Frustrum[2],Frustrum[3]); // linke Ebene
	//PlaneFromPoints(FrustrumPl[3],Frustrum[1],Frustrum[5],Frustrum[7]); // rechte Ebene
	//PlaneFromPoints(FrustrumPl[4],Frustrum[0],Frustrum[4],Frustrum[5]); // untere Ebene
	//PlaneFromPoints(FrustrumPl[5],Frustrum[2],Frustrum[3],Frustrum[7]); // obere Ebene*/
}

// 3D-Matrizen erstellen (Einstellungen ueber SetPerspectiveMode vor NixStart() zu treffen)
// enable3d: true  -> 3D-Ansicht auf (View3DWidth,View3DHeight) gemapt
//           false -> Pixel-Angaben~~~
// beide Bilder sind um View3DCenterX,View3DCenterY (3D als Fluchtpunkt) verschoben
void NixSetView(bool enable3d,const matrix &view_mat)
{
	//SetCull(CullCCW); // ???
	NixViewMatrix=view_mat;

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetTransform(D3DTS_VIEW,(D3DXMATRIX*)&NixViewMatrix);

		matrix p,t,s,s2;
		if (enable3d){
			if (RenderingToTexture>=0)
				MatrixTranslation(t,vector(View3DCenterX/float(NixTextureWidth[RenderingToTexture])*2.0f-1,1-View3DCenterY/float(NixTextureHeight[RenderingToTexture])*2.0f,0));
			else
				MatrixTranslation(t,vector(View3DCenterX/float(NixScreenWidth)*2.0f-1,1-View3DCenterY/float(NixScreenHeight)*2.0f,0));
			// perspektivische Verzerrung
			D3DXMatrixPerspectiveFovLH((D3DXMATRIX*)&p,pi/3,View3DRatio,NixMinDepth,NixMaxDepth);
			if (RenderingToTexture>=0)
				MatrixScale(s,View3DWidth/((float)NixTextureWidth[RenderingToTexture]),View3DHeight/((float)NixTextureHeight[RenderingToTexture]),1);
			else
				MatrixScale(s,View3DWidth/((float)NixScreenWidth),View3DHeight/((float)NixScreenHeight),1);
			MatrixScale(s2,ViewScale.x,ViewScale.y,ViewScale.z);
			MatrixMultiply(NixProjectionMatrix,t,p);
			MatrixMultiply(NixProjectionMatrix,NixProjectionMatrix,s);
			MatrixMultiply(NixProjectionMatrix,NixProjectionMatrix,s2); // richtige Reihenfolge??????  ...bei Gelegenheit testen!
		}else{
			//msg_todo("NixSetView(2D) fuer DirectX9 (Sonderlichkeiten bei Target!=Screen ???)");
			//MatrixScale(s,1.0f/(float)NixTargetWidth,1.0f/(float)NixTargetHeight,1.0f/(float)NixMaxDepth);
			MatrixScale(s,2*View2DScaleX/(float)NixScreenWidth,2*View2DScaleY/(float)NixScreenHeight,1.0f/(float)NixMaxDepth);
			MatrixTranslation(t,vector(View3DCenterX/float(NixScreenWidth)*2.0f-1,1-View3DCenterY/float(NixScreenHeight)*2.0f,0.5f+ViewPos.z));
			MatrixMultiply(NixProjectionMatrix,t,s);
			MatrixScale(s,ViewScale.x,ViewScale.y,ViewScale.z);
			MatrixMultiply(NixProjectionMatrix,s,NixProjectionMatrix);
		}
		lpDevice->SetTransform(D3DTS_PROJECTION,(D3DXMATRIX*)&NixProjectionMatrix);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		//msg_write("NixSetView");
		// Projektions-Matrix editieren
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		/*msg_write(NixTargetWidth);
		msg_write(NixTargetHeight);
		msg_write(View3DCenterX);
		msg_write(View3DCenterY);
		msg_write(View3DWidth);
		msg_write(View3DHeight);
		msg_write(View2DScaleX);
		msg_write(View2DScaleY);
		msg_write(NixMaxDepth);*/
		if (enable3d){
			//msg_write("3d");
			glTranslatef(View3DCenterX/float(NixTargetWidth)*2.0f-1,1-View3DCenterY/float(NixTargetHeight)*2.0f,0);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
			// perspektivische Verzerrung
			gluPerspective(60.0f,View3DRatio,NixMinDepth,NixMaxDepth);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
			glScalef((View3DWidth/(float)NixTargetWidth),(View3DHeight/(float)NixTargetHeight),-1); // -1: Koordinatensystem: Links vs Rechts
			glScalef(ViewScale.x,ViewScale.y,ViewScale.z);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
		}else{
			glTranslatef(View3DCenterX/float(NixTargetWidth)*2.0f-1,1-View3DCenterY/float(NixTargetHeight)*2.0f,0);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
			glScalef(2*View2DScaleX/(float)NixTargetWidth,2*View2DScaleY/(float)NixTargetHeight,1.0f/(float)NixMaxDepth);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
		}
		// Matrix speichern
		glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);

		// OpenGL muss Lichter neu ausrichten, weil sie in Kamera-Koordinaten gespeichert werden!
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		//glLoadIdentity();
		glLoadMatrixf((float*)&NixViewMatrix);
		for (int i=0;i<NumLights;i++){
			if (!Light[i]->Used)	continue;
		//	if (OGLLightNo[i]<0)	continue;
			float f[4];
			/*f[0]=LightVector[i].x;	f[1]=LightVector[i].y;	f[2]=LightVector[i].z;
			if (LightDirectional[i])
				f[3]=0;
			else
				f[3]=1;
			glLightfv(OGLLightNo[i],GL_POSITION,f);*/
			if (Light[i]->Type==LightTypeDirectional){
				f[0]=Light[i]->Dir.x;	f[1]=Light[i]->Dir.y;	f[2]=Light[i]->Dir.z;	f[3]=0;
			}else if (Light[i]->Type==LightTypeRadial){
				f[0]=Light[i]->Pos.x;	f[1]=Light[i]->Pos.y;	f[2]=Light[i]->Pos.z;	f[3]=1;
			}
			glLightfv(GL_LIGHT0+i,GL_POSITION,f);
			//msg_write(i);
		}
		glPopMatrix();
	}
#endif
	MatrixInverse(NixInvProjectionMatrix,NixProjectionMatrix);
	Enabled3D=enable3d;
}

void NixSetViewV(bool enable3d,const vector &view_pos,const vector &view_ang)
{	NixSetView(enable3d,view_pos,view_ang);	}

void NixSetViewM(bool enable3d,const matrix &view_mat)
{
	ViewScale=vector(1,1,1);
	NixSetView(enable3d,view_mat);
}



#define FrustrumAngleCos	0.83f

bool NixIsInFrustrum(const vector &pos,float radius)
{
	// die absoluten Eckpunkte der BoundingBox
	vector p[8];
	p[0]=pos+vector(-radius,-radius,-radius);
	p[1]=pos+vector( radius,-radius,-radius);
	p[2]=pos+vector(-radius, radius,-radius);
	p[3]=pos+vector( radius, radius,-radius);
	p[4]=pos+vector(-radius,-radius, radius);
	p[5]=pos+vector( radius,-radius, radius);
	p[6]=pos+vector(-radius, radius, radius);
	p[7]=pos+vector( radius, radius, radius);

	bool in=false;
	for (int i=0;i<8;i++)
		//for (int j=0;j<6;j++)
			if (PlaneDistance(FrustrumPl[0],p[i])<0)
				in=true;
	/*vector d;
	VecNormalize(d,pos-ViewPos); // zu einer Berechnung zusammenfassen!!!!!!
	float fdp=VecLengthFuzzy(pos-ViewPos);
	if (fdp<radius)
		return true;
	if (VecDotProduct(d,ViewDir)>FrustrumAngleCos-radius/fdp*0.04f)
		return true;
	return false;*/
	return in;
}

// nur "3D" Polygone betreffend
void NixEnableLighting(bool Enabled)
{
	NixLightingEnabled=Enabled;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9)
		lpDevice->SetRenderState(D3DRS_LIGHTING,Enabled);
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (Enabled)
			glEnable(GL_LIGHTING);
		else
			glDisable(GL_LIGHTING);
	}
#endif
}

// nur "2D" Polygone betreffend
void NixEnableLighting2D(bool Enabled)
{
	NixLightingEnabled2D=Enabled;
}

int NixCreateLight()
{
	/*if (NumLights>=32)
		return -1;*/
/*#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		switch (NumLights){
			case 0:	OGLLightNo[NumLights]=GL_LIGHT0;	break;
			case 1:	OGLLightNo[NumLights]=GL_LIGHT1;	break;
			case 2:	OGLLightNo[NumLights]=GL_LIGHT2;	break;
			case 3:	OGLLightNo[NumLights]=GL_LIGHT3;	break;
			case 4:	OGLLightNo[NumLights]=GL_LIGHT4;	break;
			case 5:	OGLLightNo[NumLights]=GL_LIGHT5;	break;
			case 6:	OGLLightNo[NumLights]=GL_LIGHT6;	break;
			case 7:	OGLLightNo[NumLights]=GL_LIGHT7;	break;
			default:OGLLightNo[NumLights]=-1;
		}
	}
#endif
	NumLights++;
	return NumLights-1;*/
	for (int i=0;i<NumLights;i++)
		if (!Light[i]->Used){
			Light[i]->Used=true;
			Light[i]->Enabled=false;
			return i;
		}
	Light[NumLights]=new sLight;
	Light[NumLights]->Used=true;
	Light[NumLights]->Enabled=false;
	NumLights++;
	return NumLights-1;
}

void NixDeleteLight(int index)
{
	if ((index<0)||(index>NumLights))	return;
	NixEnableLight(index,false);
	Light[index]->Used=false;
}

// Punkt-Quelle
void NixSetLightRadial(int index,const vector &pos,float radius,const color &ambient,const color &diffuse,const color &specular)
{
	if ((index<0)||(index>NumLights))	return;
	Light[index]->Pos=pos;
	Light[index]->Type=LightTypeRadial;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DLIGHT9 light;
		ZeroMemory(&light,sizeof(D3DLIGHT9));
		light.Type=D3DLIGHT_POINT;
		light.Position=*(D3DXVECTOR3*)&pos;
		light.Range=radius*4;
		light.Attenuation0=0.9f;
		light.Attenuation1=2/radius;
		light.Attenuation2=1/(radius*radius);
		light.Ambient	=color2D3DCOLORVALUE(ambient);
		light.Diffuse	=color2D3DCOLORVALUE(diffuse);
		light.Specular	=color2D3DCOLORVALUE(specular);
		lpDevice->SetLight(index,&light);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		//if (OGLLightNo[index]<0)	return;
		glPushMatrix();
		//glLoadIdentity();
		glLoadMatrixf((float*)&NixViewMatrix);
		float f[4];
		f[0]=pos.x;	f[1]=pos.y;	f[2]=pos.z;	f[3]=1;
		glLightfv(GL_LIGHT0+index,GL_POSITION,f); // GL_POSITION,(x,y,z,0)=directional,(x,y,z,1)=positional !!!
		glLightfv(GL_LIGHT0+index,GL_AMBIENT,(float*)&ambient);
		glLightfv(GL_LIGHT0+index,GL_DIFFUSE,(float*)&diffuse);
		glLightfv(GL_LIGHT0+index,GL_SPECULAR,(float*)&specular);
		glLightf(GL_LIGHT0+index,GL_CONSTANT_ATTENUATION,0.9f);
		glLightf(GL_LIGHT0+index,GL_LINEAR_ATTENUATION,2.0f/radius);
		glLightf(GL_LIGHT0+index,GL_QUADRATIC_ATTENUATION,1/(radius*radius));
		glPopMatrix();
	}
#endif
}

// parallele Quelle
// dir =Richtung, in die das Licht scheinen soll
void NixSetLightDirectional(int index,const vector &dir,const color &ambient,const color &diffuse,const color &specular)
{
	if ((index<0)||(index>NumLights))	return;
	Light[index]->Dir=dir;
	Light[index]->Type=LightTypeDirectional;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DLIGHT9 light;
		ZeroMemory(&light,sizeof(D3DLIGHT9));
		light.Type=D3DLIGHT_DIRECTIONAL;
		light.Direction=-*(D3DXVECTOR3*)&dir;
		light.Ambient	=color2D3DCOLORVALUE(ambient);
		light.Diffuse	=color2D3DCOLORVALUE(diffuse);
		light.Specular	=color2D3DCOLORVALUE(specular);
		lpDevice->SetLight(index,&light);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		//if (OGLLightNo[index]<0)	return;
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		//glLoadIdentity();
		glLoadMatrixf((float*)&NixViewMatrix);
		float f[4];
		f[0]=dir.x;	f[1]=dir.y;	f[2]=dir.z;	f[3]=0;
		glLightfv(GL_LIGHT0+index,GL_POSITION,f); // GL_POSITION,(x,y,z,0)=directional,(x,y,z,1)=positional !!!
		glLightfv(GL_LIGHT0+index,GL_AMBIENT,(float*)&ambient);
		glLightfv(GL_LIGHT0+index,GL_DIFFUSE,(float*)&diffuse);
		glLightfv(GL_LIGHT0+index,GL_SPECULAR,(float*)&specular);
		glPopMatrix();
	}
#endif
}

void NixEnableLight(int index,bool enabled)
{
	if ((index<0)||(index>NumLights))	return;
	Light[index]->Enabled=enabled;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9)
		lpDevice->LightEnable(index,enabled);
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
//		if (OGLLightNo[index]<0)	return;
		if (enabled)
			glEnable(GL_LIGHT0+index);
		else
			glDisable(GL_LIGHT0+index);
	}
#endif
}

void NixSetAmbientLight(const color &c)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		int i=color2D3DCOLOR(c);
		lpDevice->SetRenderState(D3DRS_AMBIENT,i);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL)
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT,(float*)&c);
#endif
}

void NixResetToColor(const color &c)
{
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glClearColor(c.r, c.g, c.b, c.a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
#endif
}

bool Rendering=false;

bool NixStart(int texture)
{
	if (DoingEvilThingsToTheDevice)
		return false;

	NixNumTrias=0;
	RenderingToTexture=texture;
	//msg_write(string("Start ",i2s(texture)));
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){



		// Quell-Code-Chaos


		HRESULT hr;

		if (!lpDevice)
			msg_write("kein Device");
		// Test the cooperative level to see if it's okay to render
		if (FAILED(hr=lpDevice->TestCooperativeLevel())){
			msg_write("TCL evil");
        	// If the device was lost, do not render until we get it back
        	if (D3DERR_DEVICELOST==hr){
				msg_write("DeviceLost");
				return false;
			}

			// Check if the device needs to be resized.
			if (D3DERR_DEVICENOTRESET==hr){
				msg_write("DeviceNotReset");
#if 0
				// If we are windowed, read the desktop mode and use the same format for
				// the back buffer
				if (!NixFullscreen){
					//D3DAdapterInfo* pAdapterInfo = &m_Adapters[m_dwAdapter];
					//m_pD3D->GetAdapterDisplayMode( m_dwAdapter, &pAdapterInfo->d3ddmDesktop );
					d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;//pAdapterInfo->d3ddmDesktop.Format;
				}
#endif
				Usable=false;
				msg_write("......trying to repair...");
#if 0
				hr=lpDevice->TestCooperativeLevel();
				msg_write(DXErrorMsg(hr));

				// Release all vidmem objects
				// KillDeviceObjects();

				// Reset the device
				if (FAILED(hr=lpDevice->Reset(&d3dpp))){
					msg_write(DXErrorMsg(hr));
					return false;
				}else
					msg_write("Hurra!");
#endif
				NixSetVideoMode(NixApi,NixScreenWidth,NixScreenHeight,NixScreenDepth,NixFullscreen);

				// Initialize the app's device-dependent objects
				//ReincarnateDeviceObjects();

				Usable=true;
			}
			return false;
		}


		if (!Usable)
			return false;


		/*if (!lpD3D)
			msg_write("kein Direct3D!");
		if (!lpDevice)
			msg_write("kein Device!");
		if ((NumTextures>0)&&(!Texture[0]))
			msg_write("keine Texturen!");*/


		if (texture<0){
			hr=lpDevice->BeginScene();
			if (FAILED(hr)){
				msg_error(string("Device-BeginScene: ",DXErrorMsg(hr)));
				return false;
			}
			if ((WireFrame)||(!NixFullscreen))
				lpDevice->Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,D3DCOLOR_XRGB(0,0,0),1.0f,0);
			else
				lpDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,D3DCOLOR_XRGB(0,0,0),1.0f,0);
		}else{
			NixRenderToTextureBeginDX(texture);
			//lpDevice->Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,D3DCOLOR_COLORVALUE(0.0f,0.0f,0.0f,1.0f),1.0f,0);
			//lpDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER,0,1.0f,0);
			lpDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,1.0f,0);
		}
	}

#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (texture<0){
			#ifdef NIX_OS_WINDOWS
				#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
					if (OGLDynamicTextureSupport)
						glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
				#endif
				if (!wglMakeCurrent(hDC,hRC))
					return false;
			#endif
		}else{
			#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
				if (OGLDynamicTextureSupport){
					
					glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, OGLFrameBuffer[texture] );
					//glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, OGLDepthRenderBuffer[texture] );
					glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, OGLTexture[texture], 0 );
					glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, OGLDepthRenderBuffer[texture] );
					GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
					if (status == GL_FRAMEBUFFER_COMPLETE_EXT){
						//msg_write("hurra");
					}else{
						msg_write("we're screwed! (NixStart with dynamic texture target)");
						return false;
					}
				}
			#endif
		}
		glClearColor(0.0f,0.0f,0.0f,0.0f);
		glDisable(GL_SCISSOR_TEST);
		//glClearStencil(0);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		//glClear(GL_COLOR_BUFFER_BIT);
	}
#endif

	// adjust target size
	if (texture < 0){
		if (NixFullscreen){
			// fullscreen mode
			NixTargetWidth = NixScreenWidth;
			NixTargetHeight = NixScreenHeight;
		}else{
			// window mode
			irect r = NixWindow->GetInterior();
			NixTargetWidth = r.x2 - r.x1;
			NixTargetHeight = r.y2 - r.y1;
		}
	}else{
		// texture
		NixTargetWidth = NixTextureWidth[texture];
		NixTargetHeight = NixTextureHeight[texture];
	}
	VPx1 = VPy1 = 0;
	VPx2 = NixTargetWidth;
	VPy2 = NixTargetHeight;
	NixResize();
	Rendering = true;

	//msg_write("-ok?");
	return true;
}

void NixStartPart(int x1,int y1,int x2,int y2,bool set_centric)
{
	bool enable_scissors=true;
	if ((x1<0)||(y1<0)||(x2<0)||(y2<0)){
		x1=0;	y1=0;	x2=NixTargetWidth;	y2=NixTargetHeight;
		enable_scissors=false;
	}
	VPx1=x1;
	VPy1=y1;
	VPx2=x2;
	VPy2=y2;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		/*d.x1=x1;	d.y1=y1;	d.x2=x2;	d.y2=y2;	//	Ziel
		r[0].x1=0;	r[0].y1=0;	r[0].x2=w;	r[0].y2=y1;	//	Rand (oben gesammt)
		r[1].x1=0;	r[1].y1=y1;	r[1].x2=x1;	r[1].y2=y2;	//	Rand (links mitte)
		r[2].x1=x2;	r[2].y1=y1;	r[2].x2=w;	r[2].y2=y2;	//	Rand (rechts mitte)
		r[3].x1=0;	r[3].y1=y2;	r[3].x2=w;	r[3].y2=h;	//	Rand (unten gesammt)*/
		RECT r;
		r.left=x1;		r.right=x2;
		r.top=y1;		r.bottom=y2;
		lpDevice->SetScissorRect( &r );
		lpDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,(enable_scissors?TRUE:FALSE));

		//lpDevice->Clear(4,r,D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(0,0,0),-1.0f,0);
		lpDevice->Clear(1,(D3DRECT*)&r,D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(0,0,0),1.0f,0);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (enable_scissors)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
		glScissor(x1,NixTargetHeight-y2,x2-x1,y2-y1);
		glClearDepth(1.0f);
	}
#endif
	if (set_centric){
		View3DCenterX=float(x1+x2)/2.0f;
		View3DCenterY=float(y1+y2)/2.0f;
		NixSetView(Enabled3D,NixViewMatrix);
	}
}

void NixEnd()
{
	if (!Rendering)
		return;
	Rendering=false;
	//msg_write("End");
	NixSetTexture(-1);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (RenderingToTexture<0){
			lpDevice->EndScene();
			// auf den Bildschirm
			if (NixFullscreen)
				lpDevice->Present(NULL,NULL,NULL,NULL);
			else{
				irect r=NixWindow->GetInterior();
				RECT R;	R.left=0;	R.right=r.x2-r.x1;	R.top=0;	R.bottom=r.y2-r.y1;
				lpDevice->Present(&R,NULL,NULL,NULL);
			}
		}else
			NixRenderToTextureEndDX(RenderingToTexture);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glDisable(GL_SCISSOR_TEST);
		if (RenderingToTexture<0){
			// auf den Bildschirm
			#ifdef NIX_OS_WINDOWS
				if (RenderingToTexture<0)
					SwapBuffers(hDC);
			#endif
			#ifdef NIX_OS_LINUX
				#ifdef NIX_ALLOW_FULLSCREEN
					if (NixFullscreen)
						XF86VidModeSetViewPort(hui_x_display,screen,0,NixDesktopHeight-NixScreenHeight);
				#endif
				//glutSwapBuffers();
				if (DoubleBuffered)
					glXSwapBuffers(hui_x_display,GDK_WINDOW_XWINDOW(NixWindow->gl_widget->window));
			#endif
		}
		#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
			if (OGLDynamicTextureSupport)
				glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
		#endif
	}
#endif

	NixProgressTextureLifes();
}

void NixSetClipPlane(int index,const plane &pl)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetClipPlane(index,(float*)&pl);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		GLdouble d[4];
		d[0]=pl.a;	d[1]=pl.b;	d[2]=pl.c;	d[3]=pl.d;
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadMatrixf((float*)&NixViewMatrix);
		glClipPlane(GL_CLIP_PLANE0+index,d);
		glPopMatrix();
		//msg_todo("SetClipPlane fuer OpenGL");
	}
#endif
}

void NixEnableClipPlane(int index,bool enabled)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		DWORD mask=(1<<index);
		if (enabled)
			ClipPlaneMask=ClipPlaneMask|mask;
		else
			ClipPlaneMask-=ClipPlaneMask&mask;
		lpDevice->SetRenderState(D3DRS_CLIPPLANEENABLE,ClipPlaneMask);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (enabled)
			glEnable(GL_CLIP_PLANE0+index);
		else
			glDisable(GL_CLIP_PLANE0+index);
	}
#endif
}

void NixDoScreenShot(const char *filename,const rect *source)
{
	irect rect;
	if (source){
		rect.x1=(int)source->x1;
		rect.y1=(int)source->y1;
		rect.x2=(int)source->x2;
		rect.y2=(int)source->y2;
	}else{
		rect.x1=0;
		rect.y1=0;
		rect.x2=NixTargetWidth;
		rect.y2=NixTargetHeight;
	}
	/*msg_write(rect.x1);
	msg_write(rect.y1);
	msg_write(rect.x2);
	msg_write(rect.y2);*/
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXSaveSurfaceToFile(sys_str_f(filename),D3DXIFF_BMP,FrontBuffer,NULL,(RECT*)&rect);
		//D3DXSaveSurfaceToFile(sys_str_f(filename),D3DXIFF_BMP,FrontBuffer,NULL,NULL);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		int x,y;
		int dx=rect.x2-rect.x1;
		int dy=rect.y2-rect.y1;
		int *data=new int[dx*dy];
		glReadBuffer(GL_FRONT);
		glReadPixels(	rect.x1,
						rect.y1,
						dx,
						dy,
						GL_RGBA,GL_UNSIGNED_BYTE,data);
		// flip image...
		for (x=0;x<dx;x++)
			for (y=0;y<(dy+1)/2;y++){
				int y2=dy-y-1;
				int n1=(x+dx*y );
				int n2=(x+dx*y2);
				int c=data[n1];
				data[n1]=data[n2];
				data[n2]=c;
			}
		NixSaveTGA((char*)filename,dx,dy,(NixScreenDepth==16)?16:24,0,data);
		delete[](data);
	}
#endif
	msg_write(string("screenshot saved: ",SysFileName(filename)));
}

float NixGetDx()
{	return NixInputDataCurrent.dx;	}

float NixGetDy()
{	return NixInputDataCurrent.dy;	}

float NixGetWheelD()
{	return NixInputDataCurrent.dz;	}

float NixGetMx()
{	return NixInputDataCurrent.x;	}

float NixGetMy()
{	return NixInputDataCurrent.y;	}

float NixGetMDir()
{	return NixInputDataCurrent.mw;	}

vector NixGetMouseRel()
{
	return vector(	NixInputDataCurrent.x/(float)NixTargetWidth,
					NixInputDataCurrent.y/(float)NixTargetHeight,
					0);
}

void NixResetCursor()
{
	if (NixFullscreen){
		#ifdef NIX_OS_WINDOWS
			SetCursorPos(NixScreenWidth/2,NixScreenHeight/2);
		#endif
		NixWindow->InputData.x=NixInputDataCurrent.x=NixScreenWidth/2.0f;
		NixWindow->InputData.y=NixInputDataCurrent.y=NixScreenHeight/2.0f;
	}
}

bool NixGetButL()
{	return NixInputDataCurrent.lb;	}

bool NixGetButM()
{	return NixInputDataCurrent.mb;	}

bool NixGetButR()
{	return NixInputDataCurrent.rb;	}

bool NixGetButLDown()
{	return ((NixInputDataCurrent.lb)&&(!NixInputDataLast.lb));	}

bool NixGetButMDown()
{	return ((NixInputDataCurrent.mb)&&(!NixInputDataLast.mb));	}

bool NixGetButRDown()
{	return ((NixInputDataCurrent.rb)&&(!NixInputDataLast.rb));	}

bool NixGetButLUp()
{	return ((!NixInputDataCurrent.lb)&&(NixInputDataLast.lb));	}

bool NixGetButMUp()
{	return ((!NixInputDataCurrent.mb)&&(NixInputDataLast.mb));	}

bool NixGetButRUp()
{	return ((!NixInputDataCurrent.rb)&&(NixInputDataLast.rb));	}

bool NixGetKey(int Key)
{
	if (Key==KEY_ANY){
		for (int i=0;i<HUI_NUM_KEYS;i++)
			if (NixInputDataCurrent.key[i])
				return true;
		return false;
	}else
		return NixInputDataCurrent.key[Key];
}

bool NixGetKeyUp(int Key)
{
	if (Key==KEY_ANY){
		for (int i=0;i<HUI_NUM_KEYS;i++)
			if ((!NixInputDataCurrent.key[i])&&(NixInputDataLast.key[i]))
				return true;
		return false;
	}else
		return ((!NixInputDataCurrent.key[Key])&&(NixInputDataLast.key[Key]));
}

bool NixGetKeyDown(int Key)
{
	if (Key==KEY_ANY){
		for (int i=0;i<HUI_NUM_KEYS;i++)
			if ((NixInputDataCurrent.key[i])&&(!NixInputDataLast.key[i]))
				return true;
		return false;
	}else
		return ((NixInputDataCurrent.key[Key])&&(!NixInputDataLast.key[Key]));
}

static char key_char_str[16];
char *NixGetKeyChar(int key)
{
	strcpy(key_char_str,"");
	if (key<0)	return key_char_str;
	if ((NixGetKey(KEY_RCONTROL))||(NixGetKey(KEY_LCONTROL)))
		return key_char_str;
	// shift
	if ((NixGetKey(KEY_RSHIFT))||(NixGetKey(KEY_LSHIFT))){
		for (int i=0;i<26;i++)
			if (key==KEY_A+i){
				key_char_str[0]='A'+i;
				key_char_str[1]=0;
				return key_char_str;
			}
		if (key==KEY_1)			return "!";
		if (key==KEY_2)			return "\"";
		if (key==KEY_3)			return "§";
		if (key==KEY_4)			return "$";
		if (key==KEY_5)			return "%";
		if (key==KEY_6)			return "&&";
		if (key==KEY_7)			return "/";
		if (key==KEY_8)			return "(";
		if (key==KEY_9)			return ")";
		if (key==KEY_0)			return "=";
		if (key==KEY_COMMA)		return ";";
		if (key==KEY_DOT)		return ":";
		if (key==KEY_ADD)		return "*";
		if (key==KEY_SUBTRACT)	return "_";
		if (key==KEY_SMALLER)	return ">";
		if (key==KEY_SZ)		return "?";
		if (key==KEY_AE)		return "&A";
		if (key==KEY_OE)		return "&O";
		if (key==KEY_UE)		return "&U";
		if (key==KEY_FENCE)		return "\'";
		if (key==KEY_GRAVE)		return "°";
		if (key==KEY_SPACE)		return " ";
		return key_char_str;
	}
	// alt
	if ((NixGetKey(KEY_RALT))||(NixGetKey(KEY_LALT))){
		if (key==KEY_Q)			return "@";
		/*if (key==KEY_E)			return -128;
		if (key==KEY_Y)			return -91;
		if (key==KEY_2)			return -78;
		if (key==KEY_3)			return -77;*/
		if (key==KEY_7)			return "{";
		if (key==KEY_8)			return "[";
		if (key==KEY_9)			return "]";
		if (key==KEY_0)			return "}";
		if (key==KEY_ADD)		return "~";
		if (key==KEY_SMALLER)	return "|";
		if (key==KEY_SZ)		return "\\";
		return key_char_str;
	}
	// normal
	for (int i=0;i<26;i++)
		if (key==KEY_A+i){
			key_char_str[0]='a'+i;
			key_char_str[1]=0;
			return key_char_str;
		}
	for (int i=0;i<10;i++)
		if ((key==KEY_0+i)||(key==KEY_NUM_0+i)){
			key_char_str[0]='0'+i;
			key_char_str[1]=0;
			return key_char_str;
		}
	if (key==KEY_COMMA)			return ",";
	if (key==KEY_DOT)			return ".";
	if (key==KEY_ADD)			return "+";
	if (key==KEY_SUBTRACT)		return "-";
	if (key==KEY_FENCE)			return "#";
	if (key==KEY_GRAVE)			return "^";
	if (key==KEY_NUM_ADD)		return "+";
	if (key==KEY_NUM_SUBTRACT)	return "-";
	if (key==KEY_NUM_MULTIPLY)	return "*";
	if (key==KEY_NUM_DIVIDE)	return "/";
	if (key==KEY_SMALLER)		return "<";
	if (key==KEY_SZ)			return "&s";
	if (key==KEY_AE)			return "&a";
	if (key==KEY_OE)			return "&o";
	if (key==KEY_UE)			return "&u";
	if (key==KEY_SPACE)			return " ";
	if (key==KEY_TAB)			return "\t";
	if (key==KEY_RETURN)		return "\n";
	if (key==KEY_NUM_ENTER)		return "\n";
	// unknown:
	return key_char_str;
}

int NixGetKeyRhythmDown()
{
	KeyBufferRead=true;
	if (NixWindow->InputData.KeyBufferDepth>0){
		int k=NixWindow->InputData.KeyBuffer[0];
		//msg_write(GetKeyName(k));
		for (int i=0;i<NixWindow->InputData.KeyBufferDepth-2;i++)
			NixWindow->InputData.KeyBuffer[i]=NixWindow->InputData.KeyBuffer[i+1];
		NixWindow->InputData.KeyBufferDepth--;
		return k;
	}else
		return -1;
}




