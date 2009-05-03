/*----------------------------------------------------------------------------*\
| Nix                                                                          |
| -> abstraction layer for api (graphics, sound, networking)                   |
| -> OpenGL and DirectX9 supported                                             |
|   -> able to switch in runtime                                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.03.23 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#include "nix.h"


char NixVersion[32]="0.8.27.1";


#ifdef NIX_OS_WINDOWS
	#define _WIN32_WINDOWS 0x500
#endif
#ifdef NIX_OS_WINDOWS
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <mmsystem.h>
	#ifdef NIX_ALLOW_VIDEO_TEXTURE
		#include "vfw.h"
	#endif
#endif
#ifdef NIX_OS_LINUX
	#include <GL/glx.h>
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <X11/extensions/xf86vmode.h>
	#include <X11/keysym.h>
	#include <stdlib.h>
	#include <gdk/gdkx.h>
	#include <sys/time.h>
	#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
	#define DWORD		unsigned int
	#define WORD		unsigned short
	#define BYTE		unsigned char

	#undef NIX_ALLOW_VIDEO_TEXTURE
#endif

#ifdef NIX_API_DIRECTX9
	#include <d3dx9.h>
	#include <dsound.h>
	#ifdef NIX_SOUND_DIRECTX9
		#include "_dsutil.h" // get rid of this crap!!!!!!!!!!!!!!!!!!!!!!!!
	#endif
	//#define DIRECTINPUT_VERSION 0x0800
	//#include <dinput.h>
	#ifdef NIX_IDE_VCS
		#include <dshow.h>
	#endif
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
	#pragma comment(lib,"d3dx9dt.lib")
	#pragma comment(lib,"d3dxof.lib")
	#pragma comment(lib,"dxguid.lib")
#endif
#ifdef NIX_API_OPENGL
	#pragma comment(lib,"opengl32.lib")
	#pragma comment(lib,"glu32.lib")
	//#pragma comment(lib,"glut32.lib")
	#pragma comment(lib,"glaux.lib ")
#endif
#ifdef NIX_ALLOW_VIDEO_TEXTURE
	#pragma comment(lib,"strmiids.lib")
	#pragma comment(lib,"vfw32.lib")
#endif


// libraries to link:
/*-ldsound
-ldinput8
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
-lvfw_ms32*/






// environment
static CHuiWindow *HuiWindow;
static float View3DWidth,View3DHeight,View3DCenterX,View3DCenterY,View3DRatio;	// 3D transformation
static float View2DScaleX,View2DScaleY;				// 2D transformation
static int PerspectiveModeSize,PerspectiveModeCenter,PerspectiveMode2DScale;
static bool Usable,DoingEvilThingsToTheDevice;

static bool Enabled3D;
static int NumVBs,VBNumTrias[NIX_MAX_VBS],VBNumPoints[NIX_MAX_VBS],VBMaxTrias[NIX_MAX_VBS];
static bool VBIndexed[NIX_MAX_VBS];
// things'n'stuff
static int NumLights;
static bool WireFrame;
static matrix ProjectionMatrix,InvProjectionMatrix,ViewMatrix;
static matrix *PostProjectionMatrix; // for creating the ProjectionMatrix
static vector ViewScale=vector(1,1,1);
static int ClipPlaneMask;
static int FontGlyphWidth[256];

static char TextureFile[NIX_MAX_TEXTURES][256];
static int NumTextures;
static bool TextureIsDynamic[NIX_MAX_TEXTURES];
static int TextureLifeTime[NIX_MAX_TEXTURES];
static int NumCubeMaps;
static int CubeMapSize[NIX_MAX_CUBEMAPS];

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

#ifdef NIX_API_DIRECTX9
	// DirectInput
	/*static LPDIRECTINPUT		lpDI=NULL;
	static LPDIRECTINPUTDEVICE	pKeyboard=NULL;
	static LPDIRECTINPUTDEVICE	pMouse=NULL;*/

	// Direct3D
	static PDIRECT3D9 lpD3D=NULL;
	static IDirect3DDevice9 *lpDevice=NULL;
	static D3DCAPS9 d3dCaps;
	static D3DPRESENT_PARAMETERS d3dpp;
	static D3DSURFACE_DESC d3dsdBackBuffer;
	static LPDIRECT3DSURFACE9 FrontBuffer=NULL,DepthBuffer=NULL;
	static D3DVIEWPORT9 DXViewPort;
	//textures
	static LPDIRECT3DTEXTURE9 DXTexture[NIX_MAX_TEXTURES];
	static LPD3DXRENDERTOSURFACE DXTextureRenderTarget[NIX_MAX_TEXTURES];
	static LPDIRECT3DSURFACE9 DXTextureSurface[NIX_MAX_TEXTURES];
	// cube maps
	static ID3DXRenderToEnvMap *DXRenderToEnvMap[NIX_MAX_CUBEMAPS];
	static IDirect3DCubeTexture9* DXCubeMap[NIX_MAX_CUBEMAPS];
	// vertex buffer
	static LPDIRECT3DVERTEXBUFFER9 DXVB2D,DXVBVertices[NIX_MAX_VBS];
	static LPDIRECT3DINDEXBUFFER9 DXVBIndex[NIX_MAX_VBS];

	// shader files
	int NumShaderFiles=0;
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

	// sound
	#ifdef NIX_SOUND_DIRECTX9
		LPDIRECTSOUND8 pDS=NULL;
		LPDIRECTSOUNDBUFFER pDSBPrimary=NULL;
		DSBUFFERDESC dsbd;
		LPDIRECTSOUND3DLISTENER pDSListener=NULL;
		DS3DLISTENER dsListenerParams;
		int NumSounds=0;
		vector SoundPos[NIX_MAX_SOUNDS],SoundVel[NIX_MAX_SOUNDS];
		float SoundMinDist[NIX_MAX_SOUNDS],SoundMaxDist[NIX_MAX_SOUNDS],SoundRate[NIX_MAX_SOUNDS],SoundSpeed[NIX_MAX_SOUNDS],SoundVolume[NIX_MAX_SOUNDS];
		DWORD SoundFrequency[NIX_MAX_SOUNDS];
		CSound* sound[NIX_MAX_SOUNDS];
		LPDIRECTSOUND3DBUFFER pDS3DBuffer[NIX_MAX_SOUNDS];
		DS3DBUFFER dsBufferParams[NIX_MAX_SOUNDS];
	#endif

	#ifdef NIX_IDE_VCS
		IBaseFilter	*Music[NIX_MAX_SOUNDS];
		IPin *MusicPin[NIX_MAX_SOUNDS];
		IGraphBuilder *MusicGraphBuilder[NIX_MAX_SOUNDS];
		IMediaControl *MusicMediaControl[NIX_MAX_SOUNDS];
		IMediaSeeking *MusicMediaSeeking[NIX_MAX_SOUNDS];
		bool MusicRepeat[NIX_MAX_SOUNDS];
		int NumMusics=0;
	#endif

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

	struct DXVertex3D3
	{
	    float x, y, z;
	    float nx,ny,nz;
	    float tu0, tv0, tu1, tv1, tu2, tv2;
	};

	#define D3D_VERTEX2D	(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)
	#define D3D_VERTEX3D	(D3DFVF_XYZ|D3DFVF_TEX1|D3DFVF_NORMAL)//|D3DFVF_TEXCOORDSIZE3(0))
	#define D3D_VERTEX3D3	(D3DFVF_XYZ|D3DFVF_TEX3|D3DFVF_NORMAL)

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
								VARIABLE_PITCH,Name);
		SelectObject(hDC,hbmBitmap);
		SelectObject(hDC,hFont);
		SetTextColor(hDC,RGB(255,255,255));
		SetBkColor(hDC,0x00000000);
		SetTextAlign(hDC,TA_TOP);
		DWORD x=0;
		DWORD y=0;
		TCHAR str[2]="x";
		SIZE size;
		for(int c=0;c<255;c++){
			str[0]=c;
			GetTextExtentPoint32(hDC,str,1,&size);
			if((DWORD)(x+size.cx+1)>256){
				x=0;
				y+=size.cy+1;
			}
			ExtTextOut(hDC,x+0,y+0,ETO_OPAQUE,NULL,str,1,NULL);
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
	D3DCOLOR color2D3DCOLOR(color &c){
		int a=int(c.a*255.0f);		if (a<0)	a=0;		if (a>255)	a=255;
		int r=int(c.r*255.0f);		if (r<0)	r=0;		if (r>255)	r=255;
		int g=int(c.g*255.0f);		if (g<0)	g=0;		if (g>255)	g=255;
		int b=int(c.b*255.0f);		if (b<0)	b=0;		if (b>255)	b=255;
		return D3DCOLOR_ARGB(a,r,g,b);
	}
	D3DCOLORVALUE color2D3DCOLORVALUE(color &c){
		D3DCOLORVALUE cv;
		cv.a=c.a;
		cv.r=c.r;
		cv.g=c.g;
		cv.b=c.b;
		return cv;
	}
	D3DCOLORVALUE C2CV(D3DCOLOR c)
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
	#endif

	struct OGLVertex3D{
		float tu,tv;
		float nx,ny,nz;
		float x,y,z;
	};
	/*struct OGLVertex2D
	{
		float x,y,z;
		unsigned int color;
		float tu,tv;
	};*/
	static int OGLPixelFormat;
	#ifdef NIX_OS_LINUX
		Display *display;
		int screen;
		GLXContext context;
		bool DoubleBuffered;
	#endif

	int OGLMenuBarHeight;
	static unsigned int OGLTexture[NIX_MAX_TEXTURES];
	// VertexBuffer
	static OGLVertex3D* OGLVBVertices[NIX_MAX_VBS];
	static matrix OGLProjectionMatrix2D;
	static int OGLViewPort[4];

	float temp_float[4];
	float *color2f3(color &c)
	{
		temp_float[0]=c.r;
		temp_float[1]=c.g;
		temp_float[2]=c.b;
		return temp_float;
	}
	float *color2f4(color &c)
	{
		temp_float[0]=c.r;
		temp_float[1]=c.g;
		temp_float[2]=c.b;
		temp_float[3]=c.a;
		return temp_float;
	}

	// font
	int OGLFontDPList;

#ifdef NIX_OS_LINUX
	XF86VidModeModeInfo *original_mode;
#endif
#endif




#ifdef NIX_API_DIRECTX9
void DXSet2DMode()
{
	lpDevice->SetRenderState(D3DRS_LIGHTING,NixLightingEnabled2D);
}

void DXSet3DMode(){
	lpDevice->SetRenderState(D3DRS_LIGHTING,NixLightingEnabled);
}
#endif

#ifdef NIX_API_OPENGL
void OGLSet2DMode()
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

void OGLSet3DMode()
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float*)&ProjectionMatrix);
	glColor3f(1,1,1);
	if (NixLightingEnabled)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
}
#endif

#ifdef NIX_ALLOW_VIDEO_TEXTURE


struct sAviTexture
{
	AVISTREAMINFO		psi;										// Pointer To A Structure Containing Stream Info
	PAVISTREAM			pavi;										// Handle To An Open Stream
	PGETFRAME			pgf;										// Pointer To A GetFrame Object
	BITMAPINFOHEADER	bmih;										// Header Information For DrawDibDraw Decoding
	long				lastframe;									// Last Frame Of The Stream
	int					width;										// Video Width
	int					height;										// Video Height
	char				*pdata;										// Pointer To Texture Data
	unsigned char*		data;										// Pointer To Our Resized Image
	HBITMAP hBitmap;												// Handle To A Device Dependant Bitmap
	float time,fps;
	int ActualFrame;
	HDRAWDIB hdd;												// Handle For Our Dib
	HDC hdc;										// Creates A Compatible Device Context
}*AviTexture[NIX_MAX_TEXTURES];


static void flipIt(void* buffer,int w,int h)
{
	unsigned char *b = (BYTE *)buffer;
	char temp;
    for (int x=0;x<w;x++)
    	for (int y=0;y<h/2;y++){
    		temp=b[(x+(h-y-1)*w)*3+2];
    		b[(x+(h-y-1)*w)*3+2]=b[(x+y*w)*3  ];
    		b[(x+y*w)*3  ]=temp;

    		temp=b[(x+(h-y-1)*w)*3+1];
    		b[(x+(h-y-1)*w)*3+1]=b[(x+y*w)*3+1];
    		b[(x+y*w)*3+1]=temp;

    		temp=b[(x+(h-y-1)*w)*3  ];
    		b[(x+(h-y-1)*w)*3  ]=b[(x+y*w)*3+2];
    		b[(x+y*w)*3+2]=temp;
    	}
}

static int GetBestVideoSize(int s)
{
	return NixMaxVideoTextureSize;
}

void GrabAVIFrame(int texture,int frame)									// Grabs A Frame From The Stream
{
	msg_write("<...GrabAVIFrame>");
	if (!AviTexture[texture])
		return;
	if (AviTexture[texture]->ActualFrame==frame)
		return;
	msg_write("<GrabAVIFrame>");
	int w=NixTextureWidth[texture];
	int h=NixTextureHeight[texture];
	msg_write(w);
	msg_write(h);
	AviTexture[texture]->ActualFrame=frame;
	LPBITMAPINFOHEADER lpbi;									// Holds The Bitmap Header Information
	lpbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(AviTexture[texture]->pgf, frame);	// Grab Data From The AVI Stream
	msg_write("a");
	AviTexture[texture]->pdata=(char *)lpbi+lpbi->biSize+lpbi->biClrUsed * sizeof(RGBQUAD);	// Pointer To Data Returned By AVIStreamGetFrame
	msg_write("b");

	// Convert Data To Requested Bitmap Format
	DrawDibDraw (AviTexture[texture]->hdd, AviTexture[texture]->hdc, 0, 0, w, h, lpbi, AviTexture[texture]->pdata, 0, 0, AviTexture[texture]->width, AviTexture[texture]->height, 0);
	msg_write("c");
	

	//flipIt(AviTexture[texture]->data,w,h);	// Swap The Red And Blue Bytes (GL Compatability)
	msg_write("d");

	// Update The Texture
	#ifdef NIX_API_DIRECTX9
		if (NixApi==NIX_API_DIRECTX9){
			D3DLOCKED_RECT lr;
			DXTexture[texture]->LockRect(0,&lr,NULL,D3DLOCK_DISCARD);
	msg_write("e");
			//memcpy(lr.pBits,AviTexture[texture]->data,3*w*h);
			unsigned char *data=(unsigned char *)lr.pBits;
			for (int i=0;i<w*h;i++){
				data[i*4+2]=AviTexture[texture]->data[3*i  ];
				data[i*4+1]=AviTexture[texture]->data[3*i+1];
				data[i*4+0]=AviTexture[texture]->data[3*i+2];
				//data[i*4+3]=255;
			}
	msg_write("f");
			DXTexture[texture]->UnlockRect(0);
	msg_write("g");
		}
	#endif
	#ifdef NIX_API_OPENGL
		if (NixApi==NIX_API_OPENGL){
			glBindTexture(GL_TEXTURE_2D,OGLTexture[texture]);
			glEnable(GL_TEXTURE_2D);
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, AviTexture[texture]->data);
			//gluBuild2DMipmaps(GL_TEXTURE_2D,3,w,h,GL_RGB,GL_UNSIGNED_BYTE,AVIdata);
		}
	#endif
	msg_write("</GrabAVIFrame>");

}

bool OpenAVI(int texture,LPCSTR szFile)
{
	msg_write("<OpenAvi>");
	AviTexture[texture]->hdc = CreateCompatibleDC(0);
	AviTexture[texture]->hdd = DrawDibOpen();

	// Opens The AVI Stream
	if (AVIStreamOpenFromFile(&AviTexture[texture]->pavi, szFile, streamtypeVIDEO, 0, OF_READ, NULL) !=0){
		msg_error("Failed To Open The AVI Stream");
		return false;
	}

	AVIStreamInfo(AviTexture[texture]->pavi, &AviTexture[texture]->psi, sizeof(AviTexture[texture]->psi));						// Reads Information About The Stream Into psi
	AviTexture[texture]->width=AviTexture[texture]->psi.rcFrame.right-AviTexture[texture]->psi.rcFrame.left;					// Width Is Right Side Of Frame Minus Left
	AviTexture[texture]->height=AviTexture[texture]->psi.rcFrame.bottom-AviTexture[texture]->psi.rcFrame.top;

	AviTexture[texture]->lastframe=AVIStreamLength(AviTexture[texture]->pavi);
	AviTexture[texture]->fps=float(AviTexture[texture]->lastframe)/float(AVIStreamSampleToTime(AviTexture[texture]->pavi,AviTexture[texture]->lastframe)/1000.0f);

	AviTexture[texture]->bmih.biSize = sizeof (BITMAPINFOHEADER);					// Size Of The BitmapInfoHeader
	AviTexture[texture]->bmih.biPlanes = 1;											// Bitplanes
	AviTexture[texture]->bmih.biBitCount = 24;										// Bits Format We Want (24 Bit, 3 Bytes)
//	AviTexture[texture]->bmih.biWidth = AVI_TEXTURE_WIDTH;											// Width We Want (256 Pixels)
//	AviTexture[texture]->bmih.biHeight = AVI_TEXTURE_HEIGHT;										// Height We Want (256 Pixels)
	AviTexture[texture]->bmih.biCompression = BI_RGB;								// Requested Mode = RGB

	AviTexture[texture]->hBitmap = CreateDIBSection (AviTexture[texture]->hdc, (BITMAPINFO*)(&AviTexture[texture]->bmih), DIB_RGB_COLORS, (void**)(&AviTexture[texture]->data), NULL, 0);
	SelectObject (AviTexture[texture]->hdc, AviTexture[texture]->hBitmap);								// Select hBitmap Into Our Device Context (hdc)

	AviTexture[texture]->pgf=AVIStreamGetFrameOpen(AviTexture[texture]->pavi, NULL);						// Create The PGETFRAME	Using Our Request Mode
	if (AviTexture[texture]->pgf==NULL){
		msg_error("Failed To Open The AVI Frame");
		return false;
	}

	NixTextureWidth[texture]=GetBestVideoSize(AviTexture[texture]->width);
	NixTextureHeight[texture]=GetBestVideoSize(AviTexture[texture]->height);
	msg_write(NixTextureWidth[texture]);
	msg_write(NixTextureHeight[texture]);

	AviTexture[texture]->time=0;
	AviTexture[texture]->ActualFrame=1;
	GrabAVIFrame(texture,1);
	msg_write("</OpenAvi>");
	return true;
}

void CloseAVI(int texture)
{
	if (!AviTexture[texture])
		return;
	DeleteObject(AviTexture[texture]->hBitmap);										// Delete The Device Dependant Bitmap Object
	DrawDibClose(AviTexture[texture]->hdd);											// Closes The DrawDib Device Context
	AVIStreamGetFrameClose(AviTexture[texture]->pgf);								// Deallocates The GetFrame Resources
	AVIStreamRelease(AviTexture[texture]->pavi);										// Release The Stream
	//AVIFileExit();												// Release The File
}

#endif

void CreateFontGlyphWidth()
{
#ifdef NIX_OS_WINDOWS
	hDC=GetDC(HuiWindow->hWnd);
	SetMapMode(hDC,MM_TEXT);
	HFONT hFont=CreateFont(	NixFontHeight,0,0,0,FW_EXTRALIGHT,FALSE,
							FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,
							VARIABLE_PITCH,NixFontName);
	SelectObject(hDC,hFont);
	unsigned char str[2]="x";
	SIZE size;
	for(int c=0;c<255;c++){
		str[0]=c;
		GetTextExtentPoint32(hDC,(char*)str,1,&size);
		FontGlyphWidth[c]=size.cx;
	}
	DeleteObject(hFont);
#endif
#ifdef NIX_OS_LINUX
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
	HuiWindow=win;
	win->UsedByNix=true;
	//win->Update();
	NixFullscreen=false; // before nix is started, we're hopefully not in fullscreen mode


#ifdef NIX_OS_WINDOWS
		CoInitialize(NULL);

		// save window data
		WindowStyle=GetWindowLong(win->hWnd,GWL_STYLE);
		hMenu=GetMenu(win->hWnd);
		GetWindowRect(win->hWnd,&WindowBounds);
		GetClientRect(win->hWnd,&WindowClient);
		ShowCursor(FALSE); // will be shown again at next window mode initialization!
		win->NixGetInputFromWindow=&NixGetInputFromWindow;


		// save the original video mode
		DEVMODE mode;
		EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&mode);
		NixDesktopWidth=mode.dmPelsWidth;
		NixDesktopHeight=mode.dmPelsHeight;
		NixDesktopDepth=mode.dmBitsPerPel;
#endif
#ifdef NIX_OS_LINUX
	XF86VidModeModeInfo **modes;
	int NumModes;
	display=XOpenDisplay(0);
	screen=DefaultScreen(display);
	XF86VidModeGetAllModeLines(display,screen,&NumModes,&modes);
	original_mode=modes[0];
	NixDesktopWidth=modes[0]->hdisplay;
	NixDesktopHeight=modes[0]->vdisplay;
	NixDesktopDepth=modes[0]->hdisplay;
	Window SomeWindow;
	int x,y;
	unsigned int w,h,borderDummy,x_depth;
	XGetGeometry(display,GDK_WINDOW_XWINDOW(HuiWindow->gl_widget->window),&SomeWindow,&x,&y,&w,&h,&borderDummy,&x_depth);
	NixDesktopDepth=x_depth;
	msg_db_out(0,string2("Desktop: %dx%dx%d\n",NixDesktopWidth,NixDesktopHeight,NixDesktopDepth));
	int glxMajorVersion,glxMinorVersion;
	int vidModeMajorVersion,vidModeMinorVersion;
	XF86VidModeQueryVersion(display,&vidModeMajorVersion,&vidModeMinorVersion);
	msg_db_out(1,string2("XF86VidModeExtension-Version %d.%d\n",vidModeMajorVersion,vidModeMinorVersion));
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
	irect r=HuiWindow->GetInterior();
	NixTargetWidth=r.x2-r.x1;
	NixTargetHeight=r.y2-r.y1-OGLMenuBarHeight;
	MatrixIdentity(ViewMatrix);
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
	NumTextures=0;
	NumCubeMaps=0;
	NumLights=0;
	ClipPlaneMask=0;
	NixCullingInverted=false;

	// set the new video mode
	NixSetVideoMode(api,xres,yres,depth,fullscreen);
	if (NixFatalError!=FatalErrorNone){
		msg_left();
		return;
	}


#ifdef NIX_SOUND_DIRECTX9

	// initiate sound
	msg_write("-sound");
	DirectSoundCreate8(NULL,&pDS,NULL);
	pDS->SetCooperativeLevel(win->hWnd,DSSCL_PRIORITY);
	ZeroMemory(&dsbd,sizeof(DSBUFFERDESC));
	dsbd.dwSize			=sizeof(DSBUFFERDESC);
	dsbd.dwFlags		=DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes	=0;
	dsbd.lpwfxFormat	=NULL;
	pDS->CreateSoundBuffer(&dsbd,&pDSBPrimary,NULL);
	WAVEFORMATEX wfx;
	ZeroMemory(&wfx,sizeof(WAVEFORMATEX));
	wfx.wFormatTag		=(WORD)WAVE_FORMAT_PCM;
	wfx.nChannels		=(WORD)2;
	wfx.nSamplesPerSec	=(DWORD)22050;
	wfx.wBitsPerSample	=(WORD)16;
	wfx.nBlockAlign		=(WORD) (wfx.wBitsPerSample / 8 * wfx.nChannels);
	wfx.nAvgBytesPerSec	=(DWORD) (wfx.nSamplesPerSec * wfx.nBlockAlign);
	pDSBPrimary->SetFormat(&wfx);
	pDSBPrimary->QueryInterface(IID_IDirectSound3DListener,(void**)&pDSListener);
	pDSBPrimary->Release();	pDSBPrimary=NULL;

	dsListenerParams.dwSize=sizeof(DS3DLISTENER);
	pDSListener->GetAllParameters(&dsListenerParams);
	dsListenerParams.flDistanceFactor=0.01f;
	dsListenerParams.flDopplerFactor=1.0f;
	dsListenerParams.flRolloffFactor=1.0f;
    pDSListener->SetAllParameters( &dsListenerParams, DS3D_IMMEDIATE );

	for (int s=0;s<NIX_MAX_SOUNDS;s++)
		sound[s]=NULL;

#endif

	// more default values of the engine
	NixSetPerspectiveMode(PerspectiveSizeAutoScreen);
	NixSetCull(CullDefault);
	NixSetWire(false);
	NixSetAlpha(AlphaNone);
	NixEnableLighting(false);
	NixEnableLighting2D(false);
	color c;
	NixSetMaterial(White,White,White,0,c=SetColor(0.1f,0.1f,0.1f,0.1f));
	NixSetAmbientLight(Black);
	NixSpecularEnable(false);
	NixCullingInverted=false;
	NixSetView(true,ViewMatrix);
	NixResize();
	memset(&NixInputDataCurrent.key,0,256);
	memset(&NixInputDataLast.key,0,256);
#ifdef NIX_API_DIRECTX9
	DXEffectCurrent=NULL;
#endif

#ifdef NIX_ALLOW_VIDEO_TEXTURE
	// allow AVI textures
	AVIFileInit();
#endif

	VBTemp=NixCreateVB(10240);
	// timer for windows key repitition simulation
	TimerKey=hui->CreateTimer();
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
				bool b=XF86VidModeSwitchToMode(display,screen,original_mode);
				XFlush(display);
				XF86VidModeSetViewPort(display,screen,0,0);
				HuiWindow->SetFullscreen(false);
			#endif
		}
	}
#endif
}

// erlaubt dem Device einen Neustart
void NixKillDeviceObjects()
{
	int i=0;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (!lpDevice)
			return;
		msg_db_out(1,"KillDeviceObjects");
		msg_right();

		// Font-Zeuchs
		msg_db_out(2,"-Font");
		if (DXFont){		DXFont->Release();		DXFont=NULL;	}
		if (DXFontVB){		DXFontVB->Release();	DXFontVB=NULL;	}
		if (DXFontTex){		DXFontTex->Release();	DXFontTex=NULL;	}
		// Vertex-Buffer
		msg_db_out(2,"-VertexBuffer 2D");
		DXVB2D->Release();	DXVB2D=NULL;
		msg_db_out(2,"-VertexBuffer 3D");
		for (i=0;i<NumVBs;i++){
			DXVBVertices[i]->Release();	DXVBVertices[i]=NULL;	}
		// Texturen
		msg_db_out(2,"-Texturen");
		for (i=0;i<NumTextures;i++){
			DXTexture[i]->Release();	DXTexture[i]=NULL;	}
		// Grafik-Puffer
		msg_db_out(2,"-Puffer");
		FrontBuffer->Release();		FrontBuffer=NULL;
		DepthBuffer->Release();		DepthBuffer=NULL;
		// DirectX-Device
		msg_db_out(2,"-Device");
		if (lpDevice->Release() > 0L){
			msg_error("Device noch nicht komplett sauber!");
			return;
		}
		msg_db_out(2,"-D3D");
		lpD3D->Release();
		msg_db_out(2,"-OK");
		msg_left();
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		// Texturen
		msg_db_out(2,"-Texturen");
		for (i=0;i<NumTextures;i++){
			glBindTexture(GL_TEXTURE_2D,OGLTexture[i]);
			glDeleteTextures(1,(unsigned int*)&OGLTexture[i]);
		}
	}
#endif
}

void NixReincarnateDeviceObjects()
{
	msg_db_out(1,"ReincarnateDeviceObjects");
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		int i;
		// vertex buffers
		msg_db_out(2,"-VertexBuffer 3D");
		for (i=0;i<NumVBs;i++)
			NixCreateVB(VBMaxTrias[i],i);
		// textures
		msg_db_out(2,"-Texturen");
		for (i=0;i<NumTextures;i++)
			NixReloadTexture(i);
		// graphical buffer
		msg_db_out(2,"-Puffer");
		lpDevice->GetRenderTarget(0,&FrontBuffer);
		lpDevice->GetDepthStencilSurface(&DepthBuffer);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		int i;
		// textures
		msg_db_out(2,"-Texturen");
		for (i=0;i<NumTextures;i++){
			glGenTextures(1,&OGLTexture[i]);
			NixReloadTexture(i);
		}
	}
#endif
	if (NixRefillAllVertexBuffers)
		NixRefillAllVertexBuffers();
	msg_db_out(2,"-OK");
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
	msg_write("setting video mode");
	msg_right();

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
		return;
	}
	if (fullscreen)	msg_write(string2("[ %s - fullscreen - %d x %d x %d ]",ApiName,xres,yres,depth));
	else			msg_write(string2("[ %s - window mode ]",ApiName));

	bool was_fullscreen=NixFullscreen;
	NixFullscreen=fullscreen;
	NixApi=api;
	Usable=false;
	NixFatalError=FatalErrorNone;
	DoingEvilThingsToTheDevice=true;
	NixKillDeviceObjects();

	// Fenster dem neuen Modus anpassen
#ifdef NIX_OS_WINDOWS
	msg_write("-window");
	if (NixFullscreen){
		DWORD style=WS_POPUP|WS_SYSMENU|WS_VISIBLE;
		//SetWindowLong(hWnd,GWL_STYLE,WS_POPUP);
		SetWindowLong(HuiWindow->hWnd,GWL_STYLE,style);
		SetMenu(HuiWindow->hWnd,NULL);

		WINDOWPLACEMENT wpl;
		GetWindowPlacement(HuiWindow->hWnd,&wpl);
		wpl.rcNormalPosition.left=0;
		wpl.rcNormalPosition.top=0;
		wpl.rcNormalPosition.right=xres;
		wpl.rcNormalPosition.bottom=yres;
		AdjustWindowRect(&wpl.rcNormalPosition, style, FALSE);
		SetWindowPlacement(HuiWindow->hWnd,&wpl);
	}else{
		//SetWindowLong(hWnd,GWL_STYLE,WindowStyle);
		if (hMenu){
			//SetMenu(hWnd,hMenu);
		}
	}
#endif


#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		HRESULT hr;

	// Direct3D
		msg_write("-Direct3D");

		// DirectX9-Handle erstellen
		lpD3D=Direct3DCreate9(D3D_SDK_VERSION);
		if (lpD3D==NULL){
			msg_error("can not initiate DirectX9");
			NixFatalError=FatalErrorNoDirectX9;
			return;
		}

		ZeroMemory(&d3dpp,sizeof(d3dpp));
		d3dpp.Windowed					=!NixFullscreen;
		d3dpp.BackBufferCount			=1;
		d3dpp.SwapEffect				=D3DSWAPEFFECT_DISCARD;
		d3dpp.EnableAutoDepthStencil	=true;
		d3dpp.AutoDepthStencilFormat	=D3DFMT_D24S8;
		d3dpp.hDeviceWindow				=HuiWindow->hWnd;
		if (NixFullscreen){
			d3dpp.BackBufferWidth		=xres;
			d3dpp.BackBufferHeight		=yres;
			if (depth==32)
				d3dpp.BackBufferFormat	=D3DFMT_A8R8G8B8;
			else
				d3dpp.BackBufferFormat	=D3DFMT_R5G6B5;
		}else{
			GetClientRect(HuiWindow->hWnd,&WindowClient);
			d3dpp.BackBufferWidth		=NixDesktopWidth;
			d3dpp.BackBufferHeight		=NixDesktopHeight;
			if (NixDesktopDepth==32)
				d3dpp.BackBufferFormat	=D3DFMT_A8R8G8B8;
			else
				d3dpp.BackBufferFormat	=D3DFMT_R5G6B5;
		}
		// Device -> Hardwaremode
		msg_write("-device");
		hr=lpD3D->CreateDevice(	D3DADAPTER_DEFAULT,
								D3DDEVTYPE_HAL,
								HuiWindow->hWnd,
								D3DCREATE_SOFTWARE_VERTEXPROCESSING,
								&d3dpp,
								&lpDevice);
		// Device -> Softwaremode
		if (FAILED(hr)){
			msg_write(DXErrorMsg(hr));
			msg_error("hardware mode noit supported -> software mode!");
			lpD3D->CreateDevice(D3DADAPTER_DEFAULT,
								D3DDEVTYPE_REF,
								HuiWindow->hWnd,
								D3DCREATE_SOFTWARE_VERTEXPROCESSING,
								&d3dpp,
								&lpDevice);

		}
		if (FAILED(hr)){
			msg_write(DXErrorMsg(hr));
			msg_error("neither hardware moe nor software mode supported");
			NixFatalError=FatalErrorNoDevice;
			msg_left();
			return;
		}

		// Store render target surface desc
		LPDIRECT3DSURFACE9 pBackBuffer = NULL;
		lpDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer );
		pBackBuffer->GetDesc(&d3dsdBackBuffer);
		pBackBuffer->Release();

		lpDevice->GetDeviceCaps( &d3dCaps );
		if ((d3dCaps.StencilCaps & D3DSTENCILCAPS_TWOSIDED)!=0)
			msg_write("(TwoSided)");
		else
			msg_write("(not TwoSided)");



		/*if (!NixWindowed){
		// DirectInput
			DirectInput8Create(	GetModuleHandle(NULL),DIRECTINPUT_VERSION,
								IID_IDirectInput8, (VOID**)&lpDI, NULL );
	
			// Tastatur
			lpDI->CreateDevice(GUID_SysKeyboard,&pKeyboard,NULL);
			pKeyboard->SetDataFormat(&c_dfDIKeyboard);
			//pKeyboard->SetCooperativeLevel(hWnd,DISCL_EXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY );
			pKeyboard->SetCooperativeLevel(hWnd,DISCL_EXCLUSIVE | DISCL_FOREGROUND );
			pKeyboard->Acquire();

			// Maus
			lpDI->CreateDevice(GUID_SysMouse, &pMouse, NULL);
			pMouse->SetDataFormat(&c_dfDIMouse);
			pMouse->SetCooperativeLevel(hWnd,DISCL_EXCLUSIVE | DISCL_FOREGROUND);

			UpdateInput();
		}*/

	// Schriften
		msg_write("-font");
		//if (NixFullscreen)
			InitOwnFont(NixFontName,NixFontHeight);
		/*else{
			HFONT hFont=CreateFont(	NixFontHeight,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
									ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
									ANTIALIASED_QUALITY,FF_DONTCARE,NixFontName);
			D3DXCreateFont(lpDevice,hFont,&DXFont);
		}*/

	// Einstellungen fuer die Grafik
		msg_write("-setting properties");
		lpDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
		lpDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
		lpDevice->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT);
		lpDevice->SetRenderState(D3DRS_ZENABLE,true);
		lpDevice->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_POINT);
		lpDevice->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		lpDevice->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);


	// 2D-Vertex-Buffer
		msg_write("-2D vertex buffer");
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
			msg_write("-pixelformat");
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
										1,						// Versions-Nummer
										PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
										PFD_TYPE_RGBA,
										NixFullscreen?depth:NixDesktopDepth,
										//8, 0, 8, 8, 8, 16, 8, 24,
										0, 0, 0, 0, 0, 0, 0, 0,
										0, 0, 0, 0,
										24,						// 24Bit Z-Buffer
										1,						// one stencil buffer
										0,						// no "Auxiliary"-buffer
										PFD_MAIN_PLANE,
										0, 0, 0, 0 };
			hDC=GetDC(HuiWindow->hWnd);
			OGLPixelFormat=ChoosePixelFormat(hDC,&pfd);
			SetPixelFormat(hDC,OGLPixelFormat,&pfd);
			hRC=wglCreateContext(hDC);
			wglMakeCurrent(hDC,hRC);
			OGLMenuBarHeight=0;

			//
			// If the required extensions are present, get the addresses for the
			// functions that we wish to use...
			//

			#ifdef  NIX_ALLOW_DYNAMIC_TEXTURE
				msg_write("-RenderToTexture-Support");

				char *ext = (char*)glGetString( GL_EXTENSIONS );

				if (strstr(ext,"EXT_framebuffer_object")==NULL){
					msg_error("EXT_framebuffer_object extension was not found");
				}else{
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
				}
			#endif
		#endif
		#ifdef NIX_OS_LINUX
			//createGLWindow(xres,yres,depth,False);
//Bool createGLWindow(int width, int height, int bits, Bool fullscreenflag)
			msg_write("-createGLWindow");
			XVisualInfo *vi;
	//Colormap cmap;
			XF86VidModeModeInfo **modes;
			int num_modes;
			int best_mode=0;
	//XSetWindowAttributes attr;
			XF86VidModeGetAllModeLines(display,screen,&num_modes,&modes);
			for (int i=0;i<num_modes;i++)
				if ((modes[i]->hdisplay==xres)&&(modes[i]->vdisplay==yres))
					best_mode=i;
			vi=glXChooseVisual(display,screen,attrListDbl);
			if (vi){
				msg_write("-doublebuffered");
				DoubleBuffered=true;
			}else{
				msg_error("only singlebuffered visual!");
				vi=glXChooseVisual(display,screen,attrListSgl);
				DoubleBuffered=false;
		    }
			context=glXCreateContext(display,vi,0,GL_TRUE);
	//cmap=XCreateColormap(display,RootWindow(display,vi->screen),vi->visual,AllocNone);
	/*attr.colormap=cmap;
	attr.border_pixel=0;*/
	//Window win=GDK_WINDOW_XWINDOW(HuiWindow->window->window);
			Window win=GDK_WINDOW_XWINDOW(HuiWindow->gl_widget->window);
			if ((int)win<1000)
				msg_error("no GLX window found");
			XSelectInput(display,win, ExposureMask | KeyPress | KeyReleaseMask | StructureNotifyMask);

			if (NixFullscreen){
				XF86VidModeSwitchToMode(display,screen,modes[best_mode]);
				XFlush(display);
				XF86VidModeSetViewPort(display,screen,0,NixDesktopHeight-yres);
				XFlush(display);
		        printf("Resolution %d x %d\n", modes[best_mode]->hdisplay, modes[best_mode]->vdisplay);
		        XFree(modes);
				HuiWindow->SetFullscreen(true);
				//HuiWindow->SetPosition(0,0);
		        XWarpPointer(display, None, win, 0, 0, 0, 0, 0, 0);
				HuiWindow->ShowCursor(false);

		        // create a fullscreen window 
        		/*attr.override_redirect = True;
		        attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
		            StructureNotifyMask;
		        GLWin.win = XCreateWindow(GLWin.dpy, RootWindow(GLWin.dpy, vi->screen),
		            0, 0, dpyWidth, dpyHeight, 0, vi->depth, InputOutput, vi->visual,
		            CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
		            &GLWin.attr);
		        XWarpPointer(display, None, win, 0, 0, 0, 0, 0, 0);
				XMapRaised(display, win);
		        XGrabKeyboard(GLWin.dpy, GLWin.win, True, GrabModeAsync,
		            GrabModeAsync, CurrentTime);
		        XGrabPointer(GLWin.dpy, GLWin.win, True, ButtonPressMask,
		            GrabModeAsync, GrabModeAsync, GLWin.win, None, CurrentTime);*/
			}else{
				//HuiWindow->SetFullscreen(false);
			}
			glXMakeCurrent(display,win,context);
			if (glXIsDirect(display,context)) 
				msg_write("-direct rendering");
			else
				msg_error("-no direct rendering!");
		#endif

		msg_write("-setting properties");
		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST); // "Really Nice Perspective Calculations" (...)

		// Font
		msg_write("-font");
		OGLFontDPList=glGenLists(256);
		#ifdef NIX_OS_WINDOWS
			HFONT hFont=CreateFont(NixFontHeight,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE|DEFAULT_PITCH,NixFontName);
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
			font=XLoadFont(display,"*century*medium-r-normal*--14*");
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


	}
#endif


	DoingEvilThingsToTheDevice=false;
	CreateFontGlyphWidth();

	if (NixFullscreen){
		NixScreenWidth=xres;
		NixScreenHeight=yres;
		NixScreenDepth=depth;
#ifdef NIX_OS_WINDOWS
		ShowCursor(FALSE);
#endif
	}else{
		NixScreenWidth			=NixDesktopWidth;
		NixScreenHeight			=NixDesktopHeight;
		NixScreenDepth			=NixDesktopDepth;
#ifdef NIX_OS_WINDOWS
		msg_db_out(1,"SetWindowPos");
		SetWindowPos(	HuiWindow->hWnd,HWND_NOTOPMOST,
						WindowBounds.left,
						WindowBounds.top,
						(WindowBounds.right-WindowBounds.left),
						(WindowBounds.bottom-WindowBounds.top),
						SWP_SHOWWINDOW );
		ShowCursor(TRUE);
#endif
	}



// Wiederherstellung von Vertex-Buffer'n und Texturen
	NixReincarnateDeviceObjects();


	Usable=true;
	NixResize();


	msg_ok();
	msg_left();
}

void NixTellUsWhatsWrong()
{
	if (NixFatalError==FatalErrorNoDirectX9)
		hui->ErrorBox(HuiWindow,"DirectX 9 nicht gefunden!","Es tut mir au&serordentlich leid, aber ich hatte Probleme, auf Ihrem\nSystem DirectX 9 zu starten und mich damit zu verbinden!");
	if (NixFatalError==FatalErrorNoDevice)
		hui->ErrorBox(HuiWindow,"DirectX 9: weder Hardware- noch Softwaremodus!!","Es tut mir au&serordentlich leid, aber ich hatte Probleme, auf Ihrem\nSystem DirectX 9 weder einen Hardware- noch einen Softwaremodus abzuringen!\n...Unerlaubte Afl&osung?");
}

// Windows-abschiessen
void NixKillWindows()
{
#ifdef NIX_OS_WINDOWS
	msg_write("Killing Windows...");
	HANDLE t;
	OpenProcessToken(	GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&t);
	_TOKEN_PRIVILEGES tp;
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid); 
	tp.PrivilegeCount=1;
	tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(t,FALSE,&tp,0,NULL,0);
	InitiateSystemShutdown(NULL,"Resistance is futile!",10,TRUE,FALSE);
	//ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE,0);
#endif
}

void NixResize()
{
	if (!Usable)
		return;

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
		MatrixTranslation(t,vector(-float(NixTargetWidth)/2.0f,-float(NixTargetHeight/*-OGLMenuBarHeight*/)/2.0f,0));
		MatrixMultiply(OGLProjectionMatrix2D,s,t);

		// Bildschirm
		glViewport(0,0,NixTargetWidth,NixTargetHeight);
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
	NixSetView(Enabled3D,ViewMatrix);
}


#ifdef NIX_API_OPENGL

unsigned int GetIntFromBuffer(unsigned char *buffer,int pos,int bytes)
{
	unsigned int r=0;
	/*for (int i=pos;i<pos+bytes;i++)
		r=r*256+buffer[i];*/
	for (int i=pos+bytes-1;i>=pos;i--)
		r=r*256+buffer[i];
	return r;
}

struct sNixImage{
	int width,height;
	BYTE *data;
	bool alpha_used;
}NixImage;

void NixFillImageColor(BYTE *data,BYTE *col,BYTE *pal,int depth,int alpha_bits,bool pal_tga)
{
	if (depth==8){
		if (pal_tga){
			data[0]=pal[col[0]*3+2];
			data[1]=pal[col[0]*3+1];
			data[2]=pal[col[0]*3  ];
			data[3]=255;
		}
		else{
			data[0]=pal[col[0]*4+2];
			data[1]=pal[col[0]*4+1];
			data[2]=pal[col[0]*4  ];
			data[3]=255;
		}
	}
	if (depth==16){
		if (alpha_bits>0){
			data[0]=(col[1]&252)*2;
			data[1]=(col[1]&3)*64+(col[0]&224)/4;
			data[2]=(col[0]&31)*8;
			data[3]=(col[1]&128)/128*255;
		}
		else{
			data[0]=(col[1]&252)*2;
			data[1]=(col[1]&3)*64+(col[0]&224)/4;
			data[2]=(col[0]&31)*8;
			data[3]=255;
		}
	}
	if (depth==24){
		data[0]=col[2];
		data[1]=col[1];
		data[2]=col[0];
		data[3]=255;
	}
	if (depth==32){
		data[0]=col[2];
		data[1]=col[1];
		data[2]=col[0];
		data[3]=col[3];
	}

	// Color-Key-Test
	if (data[0]==0)
		if (data[1]==255)
			if (data[2]==0){
				// Farbe==gruen?
				// -> dann Farbe=schwarz und Alpha=0
				data[1]=0;
				data[3]=0;
			}
}

void NixLoadBmp(char *filename)
{
	//msg_write("bmp");
	unsigned char Header[56];
	int n;
	BYTE *data=NULL,*pal=NULL,temp_buffer[8];
	FILE* f=fopen(filename,"rb");
	fread(&Header,56,1,f);

	NixImage.width=GetIntFromBuffer(Header,18,4);
	NixImage.height=GetIntFromBuffer(Header,22,4);
	NixImage.alpha_used=false;
	bool reversed=true;
	if (NixImage.height<0){
		NixImage.height=256+NixImage.height;
		reversed=false;
	}
	if (NixImage.width<0){
		NixImage.width=256+NixImage.width;
		reversed=false;
	}
	int depth=GetIntFromBuffer(Header,28,2);
	/*msg_write(NixImage.width);
	msg_write(NixImage.height);
	msg_write(depth);*/
	NixImage.data=new BYTE[NixImage.width * NixImage.height * 4];
	int offset=GetIntFromBuffer(Header,10,4);
	int bytes_per_row_o= NixImage.width * depth / 8;
	int bytes_per_row = bytes_per_row_o+(4 - bytes_per_row_o % 4) % 4;
	int clr_used=GetIntFromBuffer(Header,46,4);

	if (depth<16){
		if (clr_used==0)clr_used=1<<depth;
		//msg_write(clr_used);
		pal= new BYTE[4*clr_used];
		fseek(f,-2,SEEK_CUR);
		fread(pal,4,clr_used,f);
	}

	fseek(f, offset, SEEK_SET);
	data=new BYTE[bytes_per_row * NixImage.height];
	//msg_write(bytes_per_row);
	if (reversed){
		//msg_write("Reversed!");
		for (n=0;n<NixImage.height;n++){
			fread(data + (bytes_per_row_o * n), sizeof(BYTE), bytes_per_row_o, f);
			fread(temp_buffer, 1, bytes_per_row-bytes_per_row_o, f);
		}
	}
	else{
		//msg_write("nicht Reversed!");
		for (n=NixImage.height-1;n>=0;n--){
			fread(data + (bytes_per_row_o * n), sizeof(BYTE), bytes_per_row_o, f);
			fread(temp_buffer, 1, bytes_per_row-bytes_per_row_o, f);
		}
	}

	switch(depth){
		case 24:
		case 8:
			for (n=0;n<NixImage.width * NixImage.height;n++)
				NixFillImageColor(NixImage.data+n*4,data+n*(depth/8),pal,depth,0,false);
			break;
		default:
			msg_error(string("unbehandelte Farbtiefe: ",i2s(depth)));
	}
	if (pal)
		delete[]pal;
	delete[]data;
	fclose(f);
}

void NixLoadTga(char *filename)
{
	//msg_write("tga");
	unsigned char Header[18];
	int x;
	BYTE *data=NULL,*pal=NULL;
	FILE* f=fopen(filename,"rb");
	fread(&Header, 18, 1, f);
	int offset=GetIntFromBuffer(Header,0,1)+18;
	int tga_type=GetIntFromBuffer(Header,2,1);
	//msg_write(tga_type);
	NixImage.width=GetIntFromBuffer(Header,12,2);	if (NixImage.width<0)	NixImage.width=-NixImage.width;
	NixImage.height=GetIntFromBuffer(Header,14,2);	if (NixImage.height<0)	NixImage.height=-NixImage.height;
	int depth=GetIntFromBuffer(Header,16,1);
	int alpha_bits=GetIntFromBuffer(Header,17,1);
	NixImage.alpha_used=alpha_bits>0;
	bool compressed=((tga_type&8)>0);
	/*if (compressed)
		msg_write("komprimiert");
	else
		msg_write("unkomprimiert");
	msg_write(string("Breite: ",i2s(NixImage.width)));
	msg_write(string("Hoehe: ",i2s(NixImage.height)));
	msg_write(string("Farbtiefe: ",i2s(depth)));
	msg_write(string("Alpha-Bits: ",i2s(alpha_bits)));*/
	NixImage.data=new BYTE[4*NixImage.width*NixImage.height+1024];
	int size=NixImage.width*NixImage.height,bpp=depth/8;

	fseek(f,offset,SEEK_SET);
	
	if (depth<16){
		pal=new BYTE[3*256];
		fread(pal,3,256,f);
	}

	if (compressed){
		int currentpixel=0,currentbyte=0;
		BYTE colorbuffer[4];
		switch(depth){
			case 32:
			case 24:
			case 16:
			case 8:
				do{
					BYTE chunkheader = 0;
					fread(&chunkheader, 1, 1, f);
					if ((chunkheader & 0x80) != 0x80){
						chunkheader++;
						//msg_write(string("raw ",i2s(chunkheader)));
						while (chunkheader-- > 0){
							fread(colorbuffer, 1, bpp, f);
							NixFillImageColor(NixImage.data+currentbyte,colorbuffer,pal,depth,alpha_bits,true);
							currentbyte += 4;
							currentpixel++;
						}
					}
					else{
						chunkheader = (BYTE)((chunkheader & 0x7F) + 1);
						//msg_write(string("rle ",i2s(chunkheader)));
						fread(colorbuffer, 1, bpp, f);
						while(chunkheader-- > 0){
							NixFillImageColor(NixImage.data+currentbyte,colorbuffer,pal,depth,alpha_bits,true);
							currentbyte += 4;
							currentpixel++;
						}
					}
				}while(currentpixel < size);
				break;
			default:
				msg_error(string("unbehandelte Farbtiefe (komprimiert): ",i2s(depth)));
				break;
		}
	}else{
		switch(depth){
			case 32:
			case 24:
			case 16:
			case 8:
				data=new BYTE[bpp*size];
				fread(data,bpp,size,f);
				for (x=0;x<NixImage.width*NixImage.height;x++)
					NixFillImageColor(NixImage.data+x*4,data+x*bpp,pal,depth,alpha_bits,true);
				break;
			default:
				msg_error(string("unbehandelte Farbtiefe (unkomprimiert): ",i2s(depth)));
				break;
		}
	}
    delete[]data;
    fclose(f);
}
#define SOI 0xD8
#define EOI 0xD9
#define APP0 0xE0
#define SOF 0xC0
#define DQT 0xDB
#define DHT 0xC4
#define SOS 0xDA
#define DRI 0xDD
#define COM 0xFE

#define SDWORD signed int
#define SBYTE signed char
#define SWORD signed short int

char error_string[90];
#define exit_func(err) { strcpy(error_string, err); return 0;}

static BYTE *buf; // the buffer we use for storing the entire JPG file

static BYTE bp; //current byte
static WORD wp; //current word

static DWORD byte_pos; // current byte position
#define BYTE_p(i) bp=buf[(i)++]
#define WORD_p(i) wp=(((WORD)(buf[(i)]))<<8) + buf[(i)+1]; (i)+=2

// WORD X_image_size,Y_image_size; // X,Y sizes of the image
static WORD X_round,Y_round; // The dimensions rounded to multiple of Hmax*8 (X_round)
			  // and Ymax*8 (Y_round)

static BYTE *im_buffer; // RGBA image buffer
static DWORD X_image_bytes; // size in bytes of 1 line of the image = X_round * 4
static DWORD y_inc_value ; // 32*X_round; // used by decode_MCU_1x2,2x1,2x2

BYTE YH,YV,CbH,CbV,CrH,CrV; // sampling factors (horizontal and vertical) for Y,Cb,Cr
static WORD Hmax,Vmax;


static BYTE zigzag[64]={ 0, 1, 5, 6,14,15,27,28,
				  2, 4, 7,13,16,26,29,42,
				  3, 8,12,17,25,30,41,43,
				  9,11,18,24,31,40,44,53,
				 10,19,23,32,39,45,52,54,
				 20,22,33,38,46,51,55,60,
				 21,34,37,47,50,56,59,61,
				 35,36,48,49,57,58,62,63 };
typedef struct{
	BYTE Length[17];		// k =1-16 ; L[k] indicates the number of Huffman codes of length k
	WORD minor_code[17];	// indicates the value of the smallest Huffman code of length k
	WORD major_code[17];	// similar, but the highest code
	BYTE V[65536];			// V[k][j] = Value associated to the j-th Huffman code of length k
							// High nibble = nr of previous 0 coefficients
							// Low nibble = size (in bits) of the coefficient which will be taken from the data stream
}Huffman_table;

static float *QT[4]; // quantization tables, no more than 4 quantization tables (QT[0..3])
static Huffman_table HTDC[4]; //DC huffman tables , no more than 4 (0..3)
static Huffman_table HTAC[4]; //AC huffman tables                  (0..3)

static BYTE YQ_nr,CbQ_nr,CrQ_nr; // quantization table number for Y, Cb, Cr
static BYTE YDC_nr,CbDC_nr,CrDC_nr; // DC Huffman table number for Y,Cb, Cr
static BYTE YAC_nr,CbAC_nr,CrAC_nr; // AC Huffman table number for Y,Cb, Cr

static BYTE Restart_markers; // if 1 => Restart markers on , 0 => no restart markers
static WORD MCU_restart; //Restart markers appears every MCU_restart MCU blocks
typedef void (*decode_MCU_func)(DWORD);


static SWORD DCY, DCCb, DCCr; // Coeficientii DC pentru Y,Cb,Cr
static SWORD DCT_coeff[64]; // Current DCT_coefficients
static BYTE Y[64],Cb[64],Cr[64]; // Y, Cb, Cr of the current 8x8 block for the 1x1 case
static BYTE Y_1[64],Y_2[64],Y_3[64],Y_4[64];
static BYTE tab_1[64],tab_2[64],tab_3[64],tab_4[64]; // tabelele de supraesantionare pt cele 4 blocuri

static SWORD Cr_tab[256],Cb_tab[256]; // Precalculated Cr, Cb tables
static SWORD Cr_Cb_green_tab[65536];

// Initial conditions:
// byte_pos = start position in the Huffman coded segment
// WORD_get(w1); WORD_get(w2);wordval=w1;

static BYTE d_k=0;  // Bit displacement in memory, relative to the offset of w1
			 // it's always <16
static WORD w1,w2; // w1 = First word in memory; w2 = Second word
static DWORD wordval ; // the actual used value in Huffman decoding.
static DWORD mask[17];
static SWORD neg_pow2[17]={0,-1,-3,-7,-15,-31,-63,-127,-255,-511,-1023,-2047,-4095,-8191,-16383,-32767};
static DWORD start_neg_pow2=(DWORD)neg_pow2;

static int shift_temp;
#define RIGHT_SHIFT(x,shft)  \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~(0L)) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#define DESCALE(x,n)  RIGHT_SHIFT((x) + (1L << ((n)-1)), n)
#define RANGE_MASK 1023L
static BYTE *rlimit_table;
void prepare_range_limit_table()
{/* Allocate and fill in the sample_range_limit table */
	int j;
	rlimit_table = (BYTE *)malloc(5 * 256L + 128) ;
	/* First segment of "simple" table: limit[x] = 0 for x < 0 */
	memset((void *)rlimit_table,0,256);
	rlimit_table += 256;	/* allow negative subscripts of simple table */
	/* Main part of "simple" table: limit[x] = x */
	for (j = 0; j < 256; j++) rlimit_table[j] = j;
	/* End of simple table, rest of first half of post-IDCT table */
	for (j = 256; j < 640; j++) rlimit_table[j] = 255;
	/* Second half of post-IDCT table */
	memset((void *)(rlimit_table + 640),0,384);
	for (j = 0; j < 128 ; j++) rlimit_table[j+1024] = j;
}

BYTE k_for_asm;

WORD lookKbits(BYTE k)
{
#ifdef NIX_IDE_VCS
	_asm {
		mov dl, k
		mov cl, 16
		sub cl, dl
		mov eax, [wordval]
		shr eax, cl
	}
#else
	return (wordval>>(16-k));
	/*k_for_asm=k;
	__asm("mov _k_for_asm, %dl");
	__asm("mov $16, %cl");
	__asm("sub %dl, %cl");
	__asm("mov (_wordval), %eax");
	__asm("shr %cl, %eax");*/
#endif
}

BYTE byte_high_for_asm,byte_low_for_asm;

WORD WORD_hi_lo(BYTE byte_high,BYTE byte_low)
{
#ifdef NIX_IDE_VCS
	_asm {
		mov ah,byte_high
		mov al,byte_low
	}
#else
	return 256*byte_high+byte_low;
	/*byte_high_for_asm=byte_high;
	byte_low_for_asm=byte_low;
	__asm("mov _byte_high_for_asm, %ah");
	__asm("mov _byte_low_for_asm, %al");*/
#endif
}
SWORD get_svalue(BYTE k)
// k>0 always
// Takes k bits out of the BIT stream (wordval), and makes them a signed value
{
#ifdef NIX_IDE_VCS
	_asm {
		xor ecx, ecx
		mov cl,k
		mov eax,[wordval]
		shl eax,cl
		shr eax, 16
		dec cl
		bt eax,ecx
		jc end_macro
		signed_value:inc cl
		mov ebx,[start_neg_pow2]
		add ax,word ptr [ebx+ecx*2]
		end_macro:
	}
#else
	if (wordval<32768)
		return (wordval>>(16-k))-(1<<k)+1;
	else
		return (wordval>>(16-k));
	/*k_for_asm=k;
	__asm("xor ecx, %ecx");
	__asm("mov _k_for_asm, %cl");
	__asm("mov (_wordval), %eax");
	__asm("shl %cl, %eax");
	__asm("shr $16, %eax");
	__asm("dec %cl");
	__asm("bt %ecx, %eax");
	__asm("jc end_macro");
	__asm("signed_value:inc %cl");
	__asm("mov _start_neg_pow2, %ebx");
	__asm("add word ptr [%ebx+%ecx*2],%ax");
	__asm("end_macro:");*/
#endif
}
//#endif

#ifdef __WATCOMC__

WORD lookKbits(BYTE k);
#pragma aux lookKbits=\
					  "mov eax,[wordval]"\
					  "mov cl, 16"\
					  "sub cl, dl"\
					  "shr eax, cl"\
					   parm [dl] \
					   value [ax] \
					   modify [eax cl];
WORD WORD_hi_lo(BYTE byte_high,BYTE BYTE_low);
#pragma aux WORD_hi_lo=\
			   parm [ah] [al]\
			   value [ax] \
			   modify [ax];

SWORD get_svalue(BYTE k);
// k>0 always
// Takes k bits out of the BIT stream (wordval), and makes them a signed value
#pragma aux get_svalue=\
			"xor ecx, ecx"\
			"mov cl, al"\
			"mov eax,[wordval]"\
			"shl eax, cl"\
			"shr eax, 16"\
			"dec cl"\
			"bt eax,ecx"\
			"jc end_macro"\
			"signed_value:inc cl"\
			"mov ebx,[start_neg_pow2]"\
			"add ax,word ptr [ebx+ecx*2]"\
			"end_macro:"\
			parm [al]\
			modify [eax ebx ecx]\
			value [ax];
#endif

void skipKbits(BYTE k)
{
 BYTE b_high,b_low;
 d_k+=k;
 if (d_k>=16) { d_k-=16;
		w1=w2;
		// Get the next word in w2
		BYTE_p(byte_pos);
		if (bp!=0xFF) b_high=bp;
		else {
			  if (buf[byte_pos]==0) byte_pos++; //skip 00
			  else byte_pos--; // stop byte_pos pe restart marker
			  b_high=0xFF;
		}
		BYTE_p(byte_pos);
		if (bp!=0xFF) b_low=bp;
		else {
			  if (buf[byte_pos]==0) byte_pos++; //skip 00
			  else byte_pos--; // stop byte_pos pe restart marker
			  b_low=0xFF;
		}
		w2=WORD_hi_lo(b_high,b_low);
 }

 wordval = ((DWORD)(w1)<<16) + w2;
 wordval <<= d_k;
 wordval >>= 16;
}

SWORD getKbits(BYTE k)
{
 SWORD signed_wordvalue;
 signed_wordvalue=get_svalue(k);
 skipKbits(k);
 return signed_wordvalue;
}

void calculate_mask()
{
  BYTE k;
  DWORD tmpdv;
  for (k=0;k<=16;k++) { tmpdv=0x10000;mask[k]=(tmpdv>>k)-1;} //precalculated bit mask
}

void init_QT()
{
 BYTE i;
 for (i=0;i<=3;i++) QT[i]=(float *)malloc(sizeof(float)*64);
}

void load_quant_table(float *quant_table)
{
 float scalefactor[8]={1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
					   1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
 BYTE j,row,col;
// Load quantization coefficients from JPG file, scale them for DCT and reorder
// from zig-zag order
 for (j=0;j<=63;j++) quant_table[j]=buf[byte_pos+zigzag[j]];
 j=0;
 for (row=0;row<=7;row++)
   for (col=0;col<=7;col++) {
		quant_table[j]*=scalefactor[row]*scalefactor[col];
		j++;
   }
 byte_pos+=64;
}

void load_Huffman_table(Huffman_table *HT)
{
  BYTE k,j;
  DWORD code;

  for (j=1;j<=16;j++) {
	BYTE_p(byte_pos);
	HT->Length[j]=bp;
  }
  for (k=1;k<=16;k++)
	for (j=0;j<HT->Length[k];j++) {
		BYTE_p(byte_pos);
		HT->V[WORD_hi_lo(k,j)]=bp;
	}

  code=0;
  for (k=1;k<=16;k++) {
	 HT->minor_code[k] = (WORD)code;
	 for (j=1;j<=HT->Length[k];j++) code++;
	 HT->major_code[k]=(WORD)(code-1);
	 code*=2;
	 if (HT->Length[k]==0) {
		HT->minor_code[k]=0xFFFF;
		HT->major_code[k]=0;
	 }
  }
}

void process_Huffman_data_unit(BYTE DC_nr, BYTE AC_nr,SWORD *previous_DC)
{
// Process one data unit. A data unit = 64 DCT coefficients
// Data is decompressed by Huffman decoding, then the array is dezigzag-ed
// The result is a 64 DCT coefficients array: DCT_coeff
   BYTE nr,k,j,EOB_found;
   register WORD tmp_Hcode;
   BYTE size_val,count_0;
   WORD *min_code,*maj_code; // min_code[k]=minimum code of length k, maj_code[k]=similar but maximum
   WORD *max_val, *min_val;
   BYTE *huff_values;
   SWORD DCT_tcoeff[64];
   BYTE byte_temp;

// Start Huffman decoding
// First the DC coefficient decoding
   min_code=HTDC[DC_nr].minor_code;
   maj_code=HTDC[DC_nr].major_code;
   huff_values=HTDC[DC_nr].V;

   for (nr = 0; nr < 64 ; nr++) DCT_tcoeff[nr] = 0; //Initialize DCT_tcoeff

   nr=0;// DC coefficient

   min_val = &min_code[1]; max_val = &maj_code[1];
   for (k=1;k<=16;k++) {
	 tmp_Hcode=lookKbits(k);
//	   max_val = &maj_code[k]; min_val = &min_code[k];
	 if ( (tmp_Hcode<=*max_val)&&(tmp_Hcode>=*min_val) ) { //Found a valid Huffman code
		skipKbits(k);
		size_val=huff_values[WORD_hi_lo(k,(BYTE)(tmp_Hcode-*min_val))];
		if (size_val==0) DCT_tcoeff[0]=*previous_DC;
		else {
			DCT_tcoeff[0]=*previous_DC+getKbits(size_val);
			*previous_DC=DCT_tcoeff[0];
		}
		break;
	 }
	 min_val++; max_val++;
   }

// Second, AC coefficient decoding
   min_code=HTAC[AC_nr].minor_code;
   maj_code=HTAC[AC_nr].major_code;
   huff_values=HTAC[AC_nr].V;

   nr=1; // AC coefficient
   EOB_found=0;
   while ( (nr<=63)&&(!EOB_found) )
	{
	 max_val = &maj_code[1]; min_val =&min_code[1];
	 for (k=1;k<=16;k++)
	 {
	   tmp_Hcode=lookKbits(k);
//	   max_val = &maj_code[k]; &min_val = min_code[k];
	   if ( (tmp_Hcode<=*max_val)&&(tmp_Hcode>=*min_val) )
	   {
		skipKbits(k);
		byte_temp=huff_values[WORD_hi_lo(k,(BYTE)(tmp_Hcode-*min_val))];
		size_val=byte_temp&0xF;
		count_0=byte_temp>>4;
		if (size_val==0) {if (count_0==0) EOB_found=1;
						  else if (count_0==0xF) nr+=16; //skip 16 zeroes
						}
		else
		{
		 nr+=count_0; //skip count_0 zeroes
		 DCT_tcoeff[nr++]=getKbits(size_val);
		}
		break;
	   }
	   min_val++; max_val++;
	 }
	 if (k>16) nr++;  // This should not occur
	}
  for (j=0;j<=63;j++) DCT_coeff[j]=DCT_tcoeff[zigzag[j]]; // Et, voila ... DCT_coeff
}

void IDCT_transform(SWORD *incoeff,BYTE *outcoeff,BYTE Q_nr)
// Fast float IDCT transform
{
 BYTE x;
 SBYTE y;
 SWORD *inptr;
 BYTE *outptr;
 float workspace[64];
 float *wsptr;//Workspace pointer
 float *quantptr; // Quantization table pointer
 float dcval;
 float tmp0,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7;
 float tmp10,tmp11,tmp12,tmp13;
 float z5,z10,z11,z12,z13;
 BYTE *range_limit=rlimit_table+128;
 // Pass 1: process COLUMNS from input and store into work array.
 wsptr=workspace;
 inptr=incoeff;
 quantptr=QT[Q_nr];
 for (y=0;y<=7;y++)
  {
   if( (inptr[8]|inptr[16]|inptr[24]|inptr[32]|inptr[40]|inptr[48]|inptr[56])==0)
	{
	 // AC terms all zero (in a column)
	 dcval=inptr[0]*quantptr[0];
	 wsptr[0]  = dcval;
	 wsptr[8]  = dcval;
	 wsptr[16] = dcval;
	 wsptr[24] = dcval;
	 wsptr[32] = dcval;
	 wsptr[40] = dcval;
	 wsptr[48] = dcval;
	 wsptr[56] = dcval;
	 inptr++;quantptr++;wsptr++;//advance pointers to next column
	 continue ;
	}
  //Even part
	tmp0 = inptr[0] *quantptr[0];
	tmp1 = inptr[16]*quantptr[16];
	tmp2 = inptr[32]*quantptr[32];
	tmp3 = inptr[48]*quantptr[48];

	tmp10 = tmp0 + tmp2;// phase 3
	tmp11 = tmp0 - tmp2;

	tmp13 = tmp1 + tmp3;// phases 5-3
	tmp12 = (tmp1 - tmp3) * 1.414213562f - tmp13; // 2*c4

	tmp0 = tmp10 + tmp13;// phase 2
	tmp3 = tmp10 - tmp13;
	tmp1 = tmp11 + tmp12;
	tmp2 = tmp11 - tmp12;

	// Odd part
	tmp4 = inptr[8] *quantptr[8];
	tmp5 = inptr[24]*quantptr[24];
	tmp6 = inptr[40]*quantptr[40];
	tmp7 = inptr[56]*quantptr[56];

	z13 = tmp6 + tmp5;// phase 6
	z10 = tmp6 - tmp5;
	z11 = tmp4 + tmp7;
	z12 = tmp4 - tmp7;

	tmp7 = z11 + z13;// phase 5
	tmp11= (z11 - z13) * 1.414213562f; // 2*c4

	z5 = (z10 + z12) * 1.847759065f; // 2*c2
	tmp10 = 1.082392200f * z12 - z5; // 2*(c2-c6)
	tmp12 = -2.613125930f * z10 + z5;// -2*(c2+c6)

	tmp6 = tmp12 - tmp7;// phase 2
	tmp5 = tmp11 - tmp6;
	tmp4 = tmp10 + tmp5;

	wsptr[0]  = tmp0 + tmp7;
	wsptr[56] = tmp0 - tmp7;
	wsptr[8]  = tmp1 + tmp6;
	wsptr[48] = tmp1 - tmp6;
	wsptr[16] = tmp2 + tmp5;
	wsptr[40] = tmp2 - tmp5;
	wsptr[32] = tmp3 + tmp4;
	wsptr[24] = tmp3 - tmp4;
	inptr++;
	quantptr++;
	wsptr++;//advance pointers to the next column
  }

//  Pass 2: process ROWS from work array, store into output array.
// Note that we must descale the results by a factor of 8 = 2^3
 outptr=outcoeff;
 wsptr=workspace;
 for (x=0;x<=7;x++)
  {
   // Even part
	tmp10 = wsptr[0] + wsptr[4];
	tmp11 = wsptr[0] - wsptr[4];

	tmp13 = wsptr[2] + wsptr[6];
	tmp12 =(wsptr[2] - wsptr[6]) * 1.414213562f - tmp13;

	tmp0 = tmp10 + tmp13;
	tmp3 = tmp10 - tmp13;
	tmp1 = tmp11 + tmp12;
	tmp2 = tmp11 - tmp12;

   // Odd part
	z13 = wsptr[5] + wsptr[3];
	z10 = wsptr[5] - wsptr[3];
	z11 = wsptr[1] + wsptr[7];
	z12 = wsptr[1] - wsptr[7];

	tmp7 = z11 + z13;
	tmp11= (z11 - z13) * 1.414213562f;

	z5 = (z10 + z12) * 1.847759065f; // 2*c2
	tmp10 = 1.082392200f * z12 - z5;  // 2*(c2-c6)
	tmp12 = -2.613125930f * z10 + z5; // -2*(c2+c6)

	tmp6 = tmp12 - tmp7;
	tmp5 = tmp11 - tmp6;
	tmp4 = tmp10 + tmp5;

 // Final output stage: scale down by a factor of 8
	outptr[0] = range_limit[(DESCALE((int) (tmp0 + tmp7), 3)) & 1023L];
	outptr[7] = range_limit[(DESCALE((int) (tmp0 - tmp7), 3)) & 1023L];
	outptr[1] = range_limit[(DESCALE((int) (tmp1 + tmp6), 3)) & 1023L];
	outptr[6] = range_limit[(DESCALE((int) (tmp1 - tmp6), 3)) & 1023L];
	outptr[2] = range_limit[(DESCALE((int) (tmp2 + tmp5), 3)) & 1023L];
	outptr[5] = range_limit[(DESCALE((int) (tmp2 - tmp5), 3)) & 1023L];
	outptr[4] = range_limit[(DESCALE((int) (tmp3 + tmp4), 3)) & 1023L];
	outptr[3] = range_limit[(DESCALE((int) (tmp3 - tmp4), 3)) & 1023L];

	wsptr+=8;//advance pointer to the next row
	outptr+=8;
  }
}

void precalculate_Cr_Cb_tables()
{
 WORD k;
 WORD Cr_v,Cb_v;
 for (k=0;k<=255;k++) Cr_tab[k]=(SWORD)((k-128.0)*1.402);
 for (k=0;k<=255;k++) Cb_tab[k]=(SWORD)((k-128.0)*1.772);

 for (Cr_v=0;Cr_v<=255;Cr_v++)
  for (Cb_v=0;Cb_v<=255;Cb_v++)
   Cr_Cb_green_tab[((WORD)(Cr_v)<<8)+Cb_v]=(int)(-0.34414*(Cb_v-128.0)-0.71414*(Cr_v-128.0));

}

void convert_8x8_YCbCr_to_RGB(BYTE *Y, BYTE *Cb, BYTE *Cr, DWORD im_loc, DWORD X_image_bytes, BYTE *im_buffer)
// Functia (ca optimizare) poate fi apelata si fara parametrii Y,Cb,Cr
// Stim ca va fi apelata doar in cazul 1x1
{
  DWORD x,y;
  BYTE im_nr;
  BYTE *Y_val = Y, *Cb_val = Cb, *Cr_val = Cr;
  BYTE *ibuffer = im_buffer + im_loc;

  for (y=0;y<8;y++)
   {
	im_nr=0;
	for (x=0;x<8;x++)
	  {
	   ibuffer[im_nr++] = rlimit_table[*Y_val + Cr_tab[*Cr_val]]; // R
	   ibuffer[im_nr++] = rlimit_table[*Y_val + Cr_Cb_green_tab[WORD_hi_lo(*Cr_val,*Cb_val)]]; //G
	   ibuffer[im_nr++] = rlimit_table[*Y_val + Cb_tab[*Cb_val]]; //B

// Monochrome display
//	   im_buffer[im_nr++] = *Y_val;
//	   im_buffer[im_nr++] = *Y_val;
//	   im_buffer[im_nr++] = *Y_val;

	   Y_val++; Cb_val++; Cr_val++; im_nr++;
	  }
	ibuffer+=X_image_bytes;
   }
}

void convert_8x8_YCbCr_to_RGB_tab(BYTE *Y, BYTE *Cb, BYTE *Cr, BYTE *tab, DWORD im_loc, DWORD X_image_bytes, BYTE *im_buffer)
// Functia (ca optimizare) poate fi apelata si fara parametrii Cb,Cr
{
  DWORD x,y;
  BYTE nr, im_nr;
  BYTE Y_val,Cb_val,Cr_val;
  BYTE *ibuffer = im_buffer + im_loc;

  nr=0;
  for (y=0;y<8;y++)
   {
	im_nr=0;
	for (x=0;x<8;x++)
	  {
	   Y_val=Y[nr];
	   Cb_val=Cb[tab[nr]]; Cr_val=Cr[tab[nr]]; // reindexare folosind tabelul
	   // de supraesantionare precalculat
	   ibuffer[im_nr++] = rlimit_table[Y_val + Cr_tab[Cr_val]]; // R
	   ibuffer[im_nr++] = rlimit_table[Y_val + Cr_Cb_green_tab[WORD_hi_lo(Cr_val,Cb_val)]]; //G
	   ibuffer[im_nr++] = rlimit_table[Y_val + Cb_tab[Cb_val]]; //B
	   nr++; im_nr++;
	  }
	ibuffer+=X_image_bytes;
   }
}

void calculate_tabs()
{
 BYTE tab_temp[256];
 BYTE x,y;

 // Tabelul de supraesantionare 16x16
 for (y=0;y<16;y++)
	 for (x=0;x<16;x++)
	   tab_temp[y*16+x] = (y/YV)* 8 + x/YH;

 // Din el derivam tabelele corespunzatoare celor 4 blocuri de 8x8 pixeli
 for (y=0;y<8;y++)
	{
	 for (x=0;x<8;x++)
	  tab_1[y*8+x]=tab_temp[y*16+x];
	 for (x=8;x<16;x++)
	  tab_2[y*8+(x-8)]=tab_temp[y*16+x];
	}
 for (y=8;y<16;y++)
	{
	 for (x=0;x<8;x++)
	  tab_3[(y-8)*8+x]=tab_temp[y*16+x];
	 for (x=8;x<16;x++)
	  tab_4[(y-8)*8+(x-8)]=tab_temp[y*16+x];
	}
}


int init_JPG_decoding()
{
 byte_pos=0;
 init_QT();
 calculate_mask();
 prepare_range_limit_table();
 precalculate_Cr_Cb_tables();
 return 1; //for future error check
}

DWORD filesize(FILE *fp)
{
 DWORD pos;
 DWORD pos_cur;
 pos_cur=ftell(fp);
 fseek(fp,0,SEEK_END);
 pos=ftell(fp);
 fseek(fp,pos_cur,SEEK_SET);
 return pos;
}

int load_JPEG_header(FILE *fp, DWORD *X_image, DWORD *Y_image)
{
 DWORD length_of_file;
 BYTE vers,units;
 WORD Xdensity,Ydensity,Xthumbnail,Ythumbnail;
 WORD length;
 float *qtable;
 DWORD old_byte_pos;
 Huffman_table *htable;
 DWORD j;
 BYTE precision,comp_id,nr_components;
 BYTE QT_info,HT_info;
 BYTE SOS_found,SOF_found;

 length_of_file=filesize(fp);
 buf=(BYTE *)malloc(length_of_file+4);
 if (buf==NULL) exit_func("Not enough memory for loading file");
 fread(buf,length_of_file,1,fp);

 if ((buf[0]!=0xFF)||(buf[1]!=SOI)) exit_func("Not a JPG file ?\n");
 if ((buf[2]!=0xFF)||(buf[3]!=APP0)) exit_func("Invalid JPG file.");
 if ( (buf[6]!='J')||(buf[7]!='F')||(buf[8]!='I')||(buf[9]!='F')||
	  (buf[10]!=0) ) exit_func("Invalid JPG file.");

 init_JPG_decoding();
 byte_pos=11;

 BYTE_p(byte_pos);vers=bp;
 if (vers!=1) exit_func("JFIF version not supported");
 BYTE_p(byte_pos); // vers_lo=bp;
 BYTE_p(byte_pos);  units=bp;
 if (units!=0) //exit_func("JPG format not supported");
	;//	printf("units = %d\n", units);
 WORD_p(byte_pos); Xdensity=wp; WORD_p(byte_pos); Ydensity=wp;
 if ((Xdensity!=1)||(Ydensity!=1)) //exit_func("JPG format not supported");
	 ;  //{printf("X density = %d\n",Xdensity); printf("Y density = %d\n",Ydensity);}
 BYTE_p(byte_pos);Xthumbnail=bp;BYTE_p(byte_pos);Ythumbnail=bp;
 if ((Xthumbnail!=0)||(Ythumbnail!=0))
	exit_func(" Cannot process JFIF thumbnailed files\n");
 // Start decoding process
 SOS_found=0; SOF_found=0; Restart_markers=0;
 while ((byte_pos<length_of_file)&&!SOS_found)
 {
  BYTE_p(byte_pos);
  if (bp!=0xFF) continue;
  // A marker was found
  BYTE_p(byte_pos);
  switch(bp)
  {
   case DQT: WORD_p(byte_pos); length=wp; // length of the DQT marker
			 for (j=0;j<(DWORD)(wp-2);)
				{
				 old_byte_pos=byte_pos;
				 BYTE_p(byte_pos); QT_info=bp;
				 if ((QT_info>>4)!=0)
				 exit_func("16 bit quantization table not supported");
				 qtable=QT[QT_info&0xF];
				 load_quant_table(qtable);
				 j+=byte_pos-old_byte_pos;
				}
			 break;
   case DHT: WORD_p(byte_pos); length=wp;
			 for (j=0;j<(DWORD)(wp-2);)
				{
				 old_byte_pos=byte_pos;
				 BYTE_p(byte_pos); HT_info=bp;
				 if ((HT_info&0x10)!=0) htable=&HTAC[HT_info&0xF];
				 else htable=&HTDC[HT_info&0xF];
				 load_Huffman_table(htable);
				 j+=byte_pos-old_byte_pos;
				}
			 break;
   case COM: WORD_p(byte_pos); length=wp;
			 byte_pos+=wp-2;
			 break;
   case DRI: Restart_markers=1;
			 WORD_p(byte_pos); length=wp; //should be = 4
			 WORD_p(byte_pos);  MCU_restart=wp;
			 if (MCU_restart==0) Restart_markers=0;
			 break;
   case SOF: WORD_p(byte_pos); length=wp; //should be = 8+3*3=17
			 BYTE_p(byte_pos); precision=bp;
			 if (precision!=8) exit_func("Only 8 bit precision supported");
			 WORD_p(byte_pos); *Y_image=wp; WORD_p(byte_pos); *X_image=wp;
			 BYTE_p(byte_pos); nr_components=bp;
			 if (nr_components!=3) exit_func("Only truecolor JPGS supported");
			 for (j=1;j<=3;j++)
				{
				 BYTE_p(byte_pos); comp_id=bp;
				 if ((comp_id==0)||(comp_id>3)) exit_func("Only YCbCr format supported");
				 switch (comp_id)
					{
					 case 1: // Y
							BYTE_p(byte_pos); YH=bp>>4;YV=bp&0xF;
							BYTE_p(byte_pos); YQ_nr=bp;
							break;
					 case 2: // Cb
							BYTE_p(byte_pos); CbH=bp>>4;CbV=bp&0xF;
							BYTE_p(byte_pos); CbQ_nr=bp;
							break;
					 case 3: // Cr
							BYTE_p(byte_pos); CrH=bp>>4;CrV=bp&0xF;
							BYTE_p(byte_pos); CrQ_nr=bp;
							break;
					}
				}
			 SOF_found=1;
			 break;
   case SOS: WORD_p(byte_pos); length=wp; //should be = 6+3*2=12
		 BYTE_p(byte_pos); nr_components=bp;
		 if (nr_components!=3) exit_func("Invalid SOS marker");
		 for (j=1;j<=3;j++)
		   {
			BYTE_p(byte_pos); comp_id=bp;
			if ((comp_id==0)||(comp_id>3)) exit_func("Only YCbCr format supported");
			switch (comp_id)
			{
			 case 1: // Y
					BYTE_p(byte_pos); YDC_nr=bp>>4;YAC_nr=bp&0xF;
					break;
			 case 2: // Cb
					BYTE_p(byte_pos); CbDC_nr=bp>>4;CbAC_nr=bp&0xF;
					break;
			 case 3: // Cr
					BYTE_p(byte_pos); CrDC_nr=bp>>4;CrAC_nr=bp&0xF;
					break;
			}
		   }
		 BYTE_p(byte_pos); BYTE_p(byte_pos); BYTE_p(byte_pos); // Skip 3 bytes
		 SOS_found=1;
		 break;
   case 0xFF:
		 break; // do nothing for 0xFFFF, sequence of consecutive 0xFF are for
			// filling purposes and should be ignored
   default:  WORD_p(byte_pos); length=wp;
		 byte_pos+=wp-2; //skip unknown marker
		 break;
  }
 }
 if (!SOS_found) exit_func("Invalid JPG file. No SOS marker found.");
 if (!SOF_found) exit_func("Progressive JPEGs not supported");

 if ((CbH>YH)||(CrH>YH)) exit_func("Vertical sampling factor for Y should be >= sampling factor for Cb,Cr");
 if ((CbV>YV)||(CrV>YV)) exit_func("Horizontal sampling factor for Y should be >= sampling factor for Cb,Cr");

 if ((CbH>=2)||(CbV>=2)) exit_func("Cb sampling factors should be = 1");
 if ((CrV>=2)||(CrV>=2)) exit_func("Cr sampling factors should be = 1");

// Restricting sampling factors for Y,Cb,Cr should give us 4 possible cases for sampling factors
// YHxYV,CbHxCbV,CrHxCrV: 2x2,1x1,1x1;  1x2,1x1,1x1; 2x1,1x1,1x1;
// and 1x1,1x1,1x1 = no upsampling needed

 Hmax=YH,Vmax=YV;
 if ( *X_image%(Hmax*8)==0) X_round=(unsigned short)*X_image; // X_round = Multiple of Hmax*8
 else X_round=(unsigned short)(*X_image/(Hmax*8)+1)*(Hmax*8);
 if ( *Y_image%(Vmax*8)==0) Y_round=(unsigned short)*Y_image; // Y_round = Multiple of Vmax*8
 else Y_round=(unsigned short)(*Y_image/(Vmax*8)+1)*(Vmax*8);

 im_buffer=(BYTE *)malloc(X_round*Y_round*4);
 if (im_buffer==NULL) exit_func("Not enough memory for storing the JPEG image");

 return 1;
}

void resync()
{
	byte_pos+=2;
	BYTE_p(byte_pos);
	if (bp==0xFF) byte_pos++;
	w1=WORD_hi_lo(bp, 0);
	BYTE_p(byte_pos);
	if (bp==0xFF) byte_pos++;
	w1+=bp;
	BYTE_p(byte_pos);
	if (bp==0xFF) byte_pos++;
	w2=WORD_hi_lo(bp, 0);
	BYTE_p(byte_pos);
	if (bp==0xFF) byte_pos++;
	w2+=bp;
	wordval=w1; d_k=0;
	DCY=0; DCCb=0; DCCr=0;
}

void decode_MCU_1x1(DWORD im_loc)
{
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y,YQ_nr);
	process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb);
	IDCT_transform(DCT_coeff,Cb,CbQ_nr);
	process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr);
	IDCT_transform(DCT_coeff,Cr,CrQ_nr);
	convert_8x8_YCbCr_to_RGB(Y,Cb,Cr,im_loc,X_image_bytes,im_buffer);
}
void decode_MCU_2x1(DWORD im_loc)
{
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_1,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_2,YQ_nr);
	process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb);
	IDCT_transform(DCT_coeff,Cb,CbQ_nr);
	process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr);
	IDCT_transform(DCT_coeff,Cr,CrQ_nr);
	convert_8x8_YCbCr_to_RGB_tab(Y_1,Cb,Cr,tab_1,im_loc,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_2,Cb,Cr,tab_2,im_loc+32,X_image_bytes,im_buffer);
}

void decode_MCU_2x2(DWORD im_loc)
{
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_1,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_2,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_3,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_4,YQ_nr);
	process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb);
	IDCT_transform(DCT_coeff,Cb,CbQ_nr);
	process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr);
	IDCT_transform(DCT_coeff,Cr,CrQ_nr);
	convert_8x8_YCbCr_to_RGB_tab(Y_1,Cb,Cr,tab_1,im_loc,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_2,Cb,Cr,tab_2,im_loc+32,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_3,Cb,Cr,tab_3,im_loc+y_inc_value,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_4,Cb,Cr,tab_4,im_loc+y_inc_value+32,X_image_bytes,im_buffer);
}

void decode_MCU_1x2(DWORD im_loc)
{
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_1,YQ_nr);
	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY);
	IDCT_transform(DCT_coeff,Y_2,YQ_nr);
	process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb);
	IDCT_transform(DCT_coeff,Cb,CbQ_nr);
	process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr);
	IDCT_transform(DCT_coeff,Cr,CrQ_nr);
	convert_8x8_YCbCr_to_RGB_tab(Y_1,Cb,Cr,tab_1,im_loc,X_image_bytes,im_buffer);
	convert_8x8_YCbCr_to_RGB_tab(Y_2,Cb,Cr,tab_3,im_loc+y_inc_value,X_image_bytes,im_buffer);
}

void decode_JPEG_image()
{
 decode_MCU_func decode_MCU;

 WORD x_mcu_cnt,y_mcu_cnt;
 DWORD nr_mcu;
 WORD X_MCU_nr,Y_MCU_nr; // Nr de MCU-uri
 DWORD MCU_dim_x; //dimensiunea in bufferul imagine a unui MCU pe axa x
 DWORD im_loc_inc; // = 7 * X_round * 4 sau 15*X_round*4;
 DWORD im_loc; //locatia in bufferul imagine

 byte_pos-=2;
 resync();

 y_inc_value = 32*X_round;
 calculate_tabs(); // Calcul tabele de supraesantionare, tinand cont de YH si YV

 if ((YH==1)&&(YV==1)) decode_MCU=decode_MCU_1x1;
 else {
	   if (YH==2)
	   {
		if (YV==2) decode_MCU=decode_MCU_2x2;
		else decode_MCU=decode_MCU_2x1;
	   }
	   else decode_MCU=decode_MCU_1x2;
 }
 MCU_dim_x=Hmax*8*4;

 Y_MCU_nr=Y_round/(Vmax*8); // nr of MCUs on Y axis
 X_MCU_nr=X_round/(Hmax*8); // nr of MCUs on X axis

 X_image_bytes=X_round*4; im_loc_inc = (Vmax*8-1) * X_image_bytes;
 nr_mcu=0; im_loc=0; // memory location of the current MCU
 for (y_mcu_cnt=0;y_mcu_cnt<Y_MCU_nr;y_mcu_cnt++)
 {
  for (x_mcu_cnt=0;x_mcu_cnt<X_MCU_nr;x_mcu_cnt++)
   {
	decode_MCU(im_loc);
	if ((Restart_markers)&&((nr_mcu+1)%MCU_restart==0)) resync();
	nr_mcu++;
	im_loc+=MCU_dim_x;
   }
  im_loc+=im_loc_inc;
 }
}

int get_JPEG_buffer(WORD X_image,WORD Y_image, BYTE **address_dest_buffer)
{
 WORD y;
 DWORD dest_loc=0;
 BYTE *src_buffer=im_buffer;
 BYTE *dest_buffer_start, *dest_buffer;

 DWORD src_bytes_per_line=X_round*4;
 DWORD dest_bytes_per_line=X_image*4;


 if ((X_round==X_image)&&(Y_round==Y_image))
	*address_dest_buffer=im_buffer;
 else
 {
  dest_buffer_start = (BYTE *)malloc(X_image*Y_image*4);
  if (dest_buffer_start==NULL) exit_func("Not enough memory for storing the JPEG image");
  dest_buffer = dest_buffer_start;
  for (y=0;y<Y_image;y++) {
	   memcpy(dest_buffer,src_buffer,dest_bytes_per_line);
	   src_buffer+=src_bytes_per_line;
	   dest_buffer+=dest_bytes_per_line;
  }
 *address_dest_buffer=dest_buffer_start;
 free(im_buffer);
 }
// spiegeln
	for (y=0;y<Y_image/2;y++){
		memcpy(buf														,*address_dest_buffer+(Y_image-1-y)*dest_bytes_per_line	,dest_bytes_per_line);
		memcpy(*address_dest_buffer+(Y_image-1-y)*dest_bytes_per_line	,*address_dest_buffer+y*dest_bytes_per_line				,dest_bytes_per_line);
		memcpy(*address_dest_buffer+y*dest_bytes_per_line				,buf													,dest_bytes_per_line);
	}
// release the buffer which contains the JPG file
 free(buf);
 return 1;
}

void NixLoadJpg(char *filename)
{
	FILE *fp;
	DWORD X_image, Y_image;

	fp=fopen(filename,"rb");
	load_JPEG_header(fp,&X_image,&Y_image);
	NixImage.width=X_image;
	NixImage.height=Y_image;
	NixImage.alpha_used=false;
	//msg_write(NixImage.width);
	//msg_write(NixImage.height);

	decode_JPEG_image();
	get_JPEG_buffer((unsigned short)X_image,(unsigned short)Y_image,&NixImage.data);
}

#endif

int NixLoadTexture(char *filename)
{
	char _filename[512];
	strcpy(_filename,filename);

	// test existence
	if (!file_test_existence(_filename)){
		msg_error(string("texture file does not exist: ",_filename));
		return -1;
	}

	strcpy(TextureFile[NumTextures],_filename);
	TextureIsDynamic[NumTextures]=false;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		NixReloadTexture(NumTextures);
		if (DXTexture[NumTextures]==NULL)
			return -1;
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glGenTextures(1,&OGLTexture[NumTextures]);
		NixReloadTexture(NumTextures);
		/*if (OGLTexture[NumTextures]<0)
			return -1;*/
	}
#endif
	NumTextures++;
	return NumTextures-1;
}

void NixReloadTexture(int texture)
{
	if (strlen(TextureFile[texture])<1)
		return;
	msg_write(string("loading texture: ",SysFileName(TextureFile[texture])));
	msg_right();

	// test the file's existence
	int h=open(SysFileName(TextureFile[texture]),0);
	if (h<0){
		msg_error("texture file does not exist!");
		msg_left();
		return;
	}
	close(h);

	#ifdef NIX_ALLOW_VIDEO_TEXTURE
		AviTexture[texture]=NULL;
	#endif
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (DXTexture[texture])
			delete(DXTexture[texture]);
		DXTexture[texture]=NULL;
		HRESULT hr;
		D3DXIMAGE_INFO SrcInfo;
		if (strcmp(strstr(TextureFile[texture],"."),".avi")==0){
			#ifdef NIX_ALLOW_VIDEO_TEXTURE
				// avi...
				AviTexture[texture]=new sAviTexture;
				if (!OpenAVI(texture,TextureFile[texture])){
					AviTexture[texture]=NULL;
					msg_left();
					return;
				}
				// dynamic texture
				hr=D3DXCreateTexture(	lpDevice,
										NixTextureWidth[texture],
										NixTextureHeight[texture],
										1, // MipMap-Ebenen
										D3DUSAGE_DYNAMIC,//0,
										D3DFMT_A8R8G8B8,
										D3DPOOL_DEFAULT,//D3DPOOL_SYSTEMMEM,//D3DPOOL_DEFAULT,
										&DXTexture[texture]);
				if ((D3D_OK!=hr)||(!DXTexture[texture])){
					msg_error(string("D3DXCreateTexture: ",DXErrorMsg(hr)));
					msg_left();
					return;
				}
				// set alpha channel
				D3DLOCKED_RECT lr;
				DXTexture[texture]->LockRect(0,&lr,NULL,D3DLOCK_DISCARD);
				unsigned char *data=(unsigned char *)lr.pBits;
				for (int i=0;i<NixTextureWidth[texture]*NixTextureHeight[texture];i++){
					data[i*4+3]=255;
				}
				DXTexture[texture]->UnlockRect(0);
			#else
				msg_error("Support for video textures is not activated!!!");
				msg_write("-> un-comment the NIX_ALLOW_VIDEO_TEXTURE definition in the source file \"00_config.h\" and recompile the program");
				msg_left();
				return;
			#endif
		}else{
			hr=D3DXCreateTextureFromFileEx(	lpDevice,
											TextureFile[texture],
											0,0, // overwrite size
											5, // MipMap-Ebenen
			/* NUR EIN TEST */				0, //D3DUSAGE_RENDERTARGET, // Art der Benutzung: 0=normal
											D3DFMT_UNKNOWN,
											D3DPOOL_DEFAULT,
											D3DX_DEFAULT,
											D3DX_DEFAULT,
											0xFF00FF00, // gruen als KeyColor
											&SrcInfo,
											NULL, // keine Palette
											&DXTexture[texture]);
			NixTextureWidth[texture]=SrcInfo.Width;
			NixTextureHeight[texture]=SrcInfo.Height;
		}
		if ((D3D_OK!=hr)||(!DXTexture[texture])){
			msg_error(string("while loading texture: ",DXErrorMsg(hr)));
			msg_left();
			return;
		}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){

		glBindTexture(GL_TEXTURE_2D,OGLTexture[texture]);

		char Ending[256];
		strcpy(Ending,"");
		int pie=-1;
		for (unsigned int i=0;i<strlen(TextureFile[texture]);i++){
			if (pie>=0){
				Ending[pie]=TextureFile[texture][i];
				Ending[pie+1]=0;
				pie++;
			}
			if (TextureFile[texture][i]=='.')
				pie=0;
		}
		if (strlen(Ending)<1){
			msg_error("texture file ending missing!");
			msg_left();
			return;
		}

	// AVI
		if ((strcmp(Ending,"avi")==0)||(strcmp(Ending,"TGA")==0)){
			//msg_write("avi");
			#ifdef NIX_ALLOW_VIDEO_TEXTURE
				AviTexture[texture]=new sAviTexture;

				glEnable(GL_TEXTURE_2D);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
				if (!OpenAVI(texture,SysFileName(TextureFile[texture]))){
					AviTexture[texture]=NULL;
					msg_left();
					return;
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, NixTextureWidth[texture], NixTextureHeight[texture], 0, GL_RGB, GL_UNSIGNED_BYTE, AviTexture[texture]->data);
			#else
				msg_error("Support for video textures is not activated!!!");
				msg_write("-> un-comment the NIX_ALLOW_VIDEO_TEXTURE definition in the source file \"00_config.h\" and recompile the program");
				msg_left();
				return;
			#endif
		}else{
			NixImage.data=NULL;
			if ((strcmp(Ending,"bmp")==0)||(strcmp(Ending,"BMP")==0))
				NixLoadBmp(SysFileName(TextureFile[texture]));
			else if ((strcmp(Ending,"tga")==0)||(strcmp(Ending,"TGA")==0))
				NixLoadTga(SysFileName(TextureFile[texture]));
			else if ((strcmp(Ending,"jpg")==0)||(strcmp(Ending,"JPG")==0))
				NixLoadJpg(SysFileName(TextureFile[texture]));
			else 
				msg_error(string("unknown ending: ",Ending));

			if (NixImage.data){
				glBindTexture(GL_TEXTURE_2D, OGLTexture[texture]);
				if (NixImage.alpha_used)
					gluBuild2DMipmaps(GL_TEXTURE_2D,4,NixImage.width,NixImage.height,GL_RGBA,GL_UNSIGNED_BYTE,NixImage.data);
				else
					gluBuild2DMipmaps(GL_TEXTURE_2D,3,NixImage.width,NixImage.height,GL_RGBA,GL_UNSIGNED_BYTE,NixImage.data);
				msg_todo("32 bit textures for OpenGL");
				//gluBuild2DMipmaps(GL_TEXTURE_2D,4,NixImage.width,NixImage.height,GL_RGBA,GL_UNSIGNED_BYTE,NixImage.data);
				//glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,128,128,0,GL_RGBA,GL_UNSIGNED_BYTE,NixImage.data);
				//glTexImage2D(GL_TEXTURE_2D,0,4,256,256,0,4,GL_UNSIGNED_BYTE,NixImage.data);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR_MIPMAP_LINEAR);
				NixTextureWidth[texture]=NixImage.width;
				NixTextureHeight[texture]=NixImage.height;
				free(NixImage.data);
			}
		}
	}
#endif
	TextureLifeTime[texture]=0;
	msg_ok();
	msg_left();
}

void NixUnloadTexture(int texture)
{
	msg_write(string("unloading Texture: ",TextureFile[texture]));
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		DXTexture[texture]->Release();
		DXTexture[texture]=NULL;
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glBindTexture(GL_TEXTURE_2D,OGLTexture[texture]);
		glDeleteTextures(1,(unsigned int*)&OGLTexture[texture]);
	}
#endif
	TextureLifeTime[texture]=-1;
}

void NixSetTexture(int texture)
{
	if (texture>=0){
		TextureLifeTime[texture]=0;
		if (TextureLifeTime[texture]<0)
			NixReloadTexture(texture);
	}
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		//extreme Experimente!!!

		// Multi-Texturing
		/*if (texture>0){
			lpDevice->SetTexture(2,Texture[texture-1]);
			lpDevice->SetTextureStageState(2,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
			lpDevice->SetTextureStageState(2,D3DTSS_COLORARG1,D3DTA_TEXTURE);

			lpDevice->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_ADDSIGNED);
			lpDevice->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			lpDevice->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_CURRENT);

			lpDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
			lpDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
			lpDevice->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT);
		}
		if (texture>=0)
			lpDevice->SetTexture(1,Texture[texture]);
		else
			lpDevice->SetTexture(1,NULL);
		lpDevice->SetTexture(0,NULL);*/

		// normal
		if (texture>=0)
			lpDevice->SetTexture(0,DXTexture[texture]);
		else if (texture>=-1)
			lpDevice->SetTexture(0,NULL);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (texture>=0){
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,OGLTexture[texture]);
			/*if (TextureIsDynamic[texture]){
				#ifdef NIX_OS_WONDOWS
				#endif
			}*/
		}else
			glDisable(GL_TEXTURE_2D);
	}
#endif
}

void NixLoadTextureData(char *filename,void **data,int &texture_width,int &texture_height)
{
	msg_write(string("retrieving data from a texture: ",SysFileName(filename)));
/*#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		LPDIRECT3DTEXTURE9 tex;
		HRESULT hr;
		D3DLOCKED_RECT lr;

		lr.pBits=NULL;//(*data);


		// Textur als dynamisch laden
		msg_write(filename);
		hr=D3DXCreateTextureFromFileEx(	lpDevice,
										filename,
										0,0,5,
										//D3DUSAGE_DYNAMIC,
										0,
										D3DFMT_UNKNOWN,
										D3DPOOL_SYSTEMMEM,
										D3DX_DEFAULT,
										D3DX_DEFAULT,
										0x00000000,
										NULL,NULL,
										&tex);
		if (hr!=D3D_OK){
			msg_error("D3DXCreateTextureFromFileEx");
			msg_error(DXErrorMsg(hr));
			return;
		}



		hr=tex->LockRect(0,&lr,NULL,D3DLOCK_READONLY);
		if (hr!=D3D_OK){
			msg_error("LockRect");
			msg_error(DXErrorMsg(hr));
		}

		if ((!lr.pBits)||(!tex))
			msg_error("Pointer sind leer!");

		(*data)=lr.pBits;
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){*/
		char Ending[256];
		strcpy(Ending,"");
		int pie=-1;
		for (unsigned int i=0;i<strlen(filename);i++){
			if (pie>=0){
				Ending[pie]=filename[i];
				Ending[pie+1]=0;
				pie++;
			}
			if (filename[i]=='.')
				pie=0;
		}
		if (strlen(Ending)<1){
			msg_error("texture file ending missing!");
			msg_left();
			return;
		}
		NixImage.data=NULL;
		if (strcmp(Ending,"bmp")==0)
			NixLoadBmp(SysFileName(filename));
		else if (strcmp(Ending,"tga")==0)
			NixLoadTga(SysFileName(filename));
		else if (strcmp(Ending,"jpg")==0)
			NixLoadJpg(SysFileName(filename));
		else 
			msg_error(string("unknown ending: ",Ending));

		if (NixImage.data){
			(*data)=NixImage.data;
			texture_width=NixImage.width;
			texture_height=NixImage.height;
			//free(NixImage.data);
		}
		int t;
		// re-flip the image data
		int *d=(int*)(*data);
		for (int y=0;y<texture_height/2;y++)
			for (int x=0;x<texture_width;x++){
				t=d[x+y*texture_width];
				d[x+y*texture_width]=d[x+(texture_height-y-1)*texture_width];
				d[x+(texture_height-y-1)*texture_width]=t;
			}
/*	}
#endif*/
	msg_ok();
}

void NixSetTextureVideoFrame(int texture,int frame)
{
#ifdef NIX_ALLOW_VIDEO_TEXTURE
	if (!AviTexture[texture])
		return;
	if (frame>AviTexture[texture]->lastframe)
		frame=0;
	GrabAVIFrame(texture,frame);
	AviTexture[texture]->time=float(frame)/AviTexture[texture]->fps;
#endif
}

void NixTextureVideoMove(int texture,float elapsed)
{
#ifdef NIX_ALLOW_VIDEO_TEXTURE
	if (!AviTexture[texture])
		return;
	msg_write("<NixTextureVideoMove>");
	AviTexture[texture]->time+=elapsed;
	if (AviTexture[texture]->time*AviTexture[texture]->fps>AviTexture[texture]->lastframe)
		AviTexture[texture]->time=0;
	GrabAVIFrame(texture,int(AviTexture[texture]->time*AviTexture[texture]->fps));
	msg_write("</NixTextureVideoMove>");
#endif
}

// encode tga files via run-length-encoding
//#define SAVE_TGA_RLE

// data must be   a-r-g-b
// 1 unsigned char per color channel  ( a8 r8 g8 b8 )
// first pixel is top left, the second one leads right
// tga-formats:
//    bits=32	alpha_bits=?	->	a8r8g8b8
//    bits=24	alpha_bits=?	->	a0r8g8b8
//    bits=16	alpha_bits=0	->	a0r5g5b5
//    bits=16	alpha_bits=1	->	a1r5g5b5
//    bits=16	alpha_bits=4	->	a4r4g4b4
void NixSaveTGA(char *filename,int width,int height,int bits,int alpha_bits,void *data,void *palette)
{
	FILE* f=fopen(SysFileName(filename),"wb");
	unsigned char Header[18];
	memset(Header,0,18);
#ifdef SAVE_TGA_RLE
	Header[2]=10;
#else
	Header[2]=2;
#endif
	Header[12]=width%256;
	Header[13]=width/256;
	Header[14]=height%256;
	Header[15]=height/256;
	Header[16]=bits;
	Header[17]=alpha_bits;
	fwrite(&Header, 18, 1, f);
	unsigned int color,last_color=0;
	unsigned char *_data=(unsigned char*)data;
	int a,r,g,b;
	bool rle_even=false;
	int rle_num=-1;
	for (int y=height-1;y>=0;y--)
		for (int x=0;x<width;x++){
			int offset=(x+y*width)*4;
#ifdef SAVE_TGA_RLE
			if (rle_num<0){
				int os1=offset;
				int os2=offset+4;//((x+4)%width+(y-(x+4)/width)*width)*4;
				rle_even=(*(int*)&_data[os1]==*(int*)&_data[os2]);
				for (rle_num=0;rle_num<128;rle_num++){
					os1=offset+rle_num*4  ;//((x+rle_num*4+4)%width+(y-(x+rle_num*4+4)/width)*width)*4;
					os2=offset+rle_num*4+4;//((x+rle_num*4+8)%width+(y-(x+rle_num*4+8)/width)*width)*4;
					if (rle_even){
						if (*(int*)&_data[os1]!=*(int*)&_data[os2])
							break;
					}else{
						if (*(int*)&_data[os1]==*(int*)&_data[os2]){
							rle_num--;
							break;
						}
					}
				}
				if (rle_even)
					msg_write("even");
				else
					msg_write("raw");
				msg_write(rle_num);
				unsigned char rle_header=128+rle_num;
				fwrite(&rle_header, 1, 1, f);
			}
			msg_write(*(int*)&_data[offset]);
			rle_num--;
			if ((rle_even)&&(rle_num>=0))
				continue;
#endif
			a=_data[offset  ];
			r=_data[offset+1];
			g=_data[offset+2];
			b=_data[offset+3];
			switch (bits){
				case 32:
					color=b + g*256 + r*65536 + a*16777216;
					break;
				case 24:
					if (alpha_bits==0)
						color=b + g*256 + r*65536;
					break;
				case 16:
					if (alpha_bits==4)
						color =int(b/16) + int(g/16)*16 + int(r/16)*256 + int(a/16)*4096;
					else
						color =int(b/8)  + int(g/8)*32  + int(r/8)*1024 + int(a/128)*32768;
					break;
			}
			fwrite(&color, bits/8, 1, f);
			//msg_write(color);
		}
    fclose(f);
}

int NixCreateDynamicTexture(int width,int height)
{
	msg_write("creating dynamic texture");
	msg_right();
	msg_write(string("[ ",i2s(width)," x ",i2s(height)," ]"));
#ifdef NIX_ALLOW_DYNAMIC_TEXTURE

	strcpy(TextureFile[NumTextures],"-dynamic Texture-");
	NixTextureWidth[NumTextures]=width;
	NixTextureHeight[NumTextures]=height;
	TextureIsDynamic[NumTextures]=true;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		HRESULT hr;
		DXTexture[NumTextures]=NULL;
		hr=lpDevice->CreateTexture(	width,
									height,
									1,
									D3DUSAGE_RENDERTARGET, // type of usage: 0=default
									d3dsdBackBuffer.Format,
									D3DPOOL_DEFAULT,
									&DXTexture[NumTextures],
									NULL);
		if (hr!=D3D_OK){
			msg_error(string("CreateDynamicTexture: ",DXErrorMsg(hr)));
			msg_left();
			return -1;
		}
		D3DSURFACE_DESC desc;
		DXTexture[NumTextures]->GetSurfaceLevel( 0, &DXTextureSurface[NumTextures] );
		DXTextureSurface[NumTextures]->GetDesc(&desc);

		hr=D3DXCreateRenderToSurface(	lpDevice,
										desc.Width,
										desc.Height,
										desc.Format,
										TRUE,
										D3DFMT_D24S8,
										&DXTextureRenderTarget[NumTextures]);
		if (hr!=D3D_OK){
			msg_write(string("CreateDynamicTexture: ",DXErrorMsg(hr)));
			msg_left();
			return -1;
		}

		NumTextures++;
		msg_ok();
		msg_left();
		return NumTextures-1;
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (!glGenFramebuffersEXT)	return -1;
		// create the render tartet stuff
		glGenFramebuffersEXT(1,&OGLFrameBuffer[NumTextures]);
		glGenRenderbuffersEXT(1,&OGLDepthRenderBuffer[NumTextures]);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,OGLDepthRenderBuffer[NumTextures]);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT24,width, height);

		// create the actual (dynamic) texture
		glGenTextures(1,&OGLTexture[NumTextures]);
		glBindTexture(GL_TEXTURE_2D,OGLTexture[NumTextures]);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,0);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

		NumTextures++;
		msg_ok();
		msg_left();
		return NumTextures-1;
	}
#endif

#else
	msg_error("Support for dynamic textures is not activated!!!");
	msg_write("-> uncomment the NIX_ALLOW_DYNAMIC_TEXTURE definition in the source file \"00_config.h\" and recompile the program");
	msg_left();
#endif
	msg_left();
	return -1;
}

int NixCreateCubeMap(int size)
{
	msg_write("creating cube map");
	msg_right();
	msg_write(string("[ ",i2s(size)," x ",i2s(size)," x 6 ]"));
#ifdef NIX_ALLOW_CUBEMAPS

	CubeMapSize[NumCubeMaps]=size;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		HRESULT hr;

		// create RenderToEnvMap object
		hr=D3DXCreateRenderToEnvMap(	lpDevice,
										size,
										1,					// mip map levels
										d3dsdBackBuffer.Format,
										TRUE,				// depth stencil
										D3DFMT_D16,			// depth stencil format
										&DXRenderToEnvMap[NumCubeMaps]);
		if (hr!=D3D_OK){
			msg_error(string("D3DXCreateRenderToEnvMap: ",DXErrorMsg(hr)));
			msg_left();
			return -1;
		}

		// create the cubemap
		if (d3dCaps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP){
			hr=D3DXCreateCubeTexture(	lpDevice,
										size,
										1,
										D3DUSAGE_RENDERTARGET,
										d3dsdBackBuffer.Format,
										D3DPOOL_DEFAULT,
										&DXCubeMap[NumCubeMaps]);
			if (hr!=D3D_OK){
				msg_error(string("D3DXCreateCubeTexture: ",DXErrorMsg(hr)));
				msg_left();
				return -1;
			}
		}else{
			msg_error("cube maps are not supported by the system!");
			msg_left();
			return -1;
		}

		NumCubeMaps++;
		msg_ok();
		msg_left();
		return NumCubeMaps-1;
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("CreateCubeMap for OpenGL");
		msg_left();
		return -1;
	}
#endif

#else
	msg_error("Support for cube maps is not activated!!!");
	msg_write("-> uncomment the NIX_ALLOW_CUBEMAPS definition in the source file \"00_config.h\" and recompile the program");
	msg_left();
#endif
	msg_left();
	return -1;
}

void NixSetCubeMap(int cube_map,int tex0,int tex1,int tex2,int tex3,int tex4,int tex5)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		HRESULT hr;

		hr=DXRenderToEnvMap[cube_map]->BeginCube(DXCubeMap[cube_map]);
		if (hr!=D3D_OK){
			msg_error(string("DXRenderToEnvMap: ",DXErrorMsg(hr)));
			return;
		}
		rect d=rect(0,(float)CubeMapSize[cube_map],0,(float)CubeMapSize[cube_map]);
		DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_POSITIVE_X,0);
		NixDraw2D(tex0,NULL,NULL,&d,0);
		DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_NEGATIVE_X,0);
		NixDraw2D(tex1,NULL,NULL,&d,0);
		DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_POSITIVE_Y,0);
		NixDraw2D(tex2,NULL,NULL,&d,0);
		DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_NEGATIVE_Y,0);
		NixDraw2D(tex3,NULL,NULL,&d,0);
		DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_POSITIVE_Z,0);
		NixDraw2D(tex4,NULL,NULL,&d,0);
		DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_NEGATIVE_Z,0);
		NixDraw2D(tex5,NULL,NULL,&d,0);

		DXRenderToEnvMap[cube_map]->End(0);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("SetCubeMap for OpenGL");
	}
#endif
}

void SetCubeMatrix(vector pos,vector ang)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		matrix t,r;
		MatrixTranslation(t,-pos);
		MatrixRotationView(r,ang);
		MatrixMultiply(ViewMatrix,r,t);
		lpDevice->SetTransform(D3DTS_VIEW,(D3DXMATRIX*)&ViewMatrix);
		lpDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER,0,1.0f,0);
	}
#endif
}

void NixRenderToCubeMap(int cube_map,vector &pos,callback_function *render_func,int mask)
{
	if (mask<1)	return;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		HRESULT hr;
		matrix vm=ViewMatrix;

		hr=DXRenderToEnvMap[cube_map]->BeginCube(DXCubeMap[cube_map]);
		if (hr!=D3D_OK){
			msg_error(string("DXRenderToEnvMap: ",DXErrorMsg(hr)));
			return;
		}
		D3DXMatrixPerspectiveFovLH((D3DXMATRIX*)&ProjectionMatrix,pi/2,1,NixMinDepth,NixMaxDepth);
		lpDevice->SetTransform(D3DTS_PROJECTION,(D3DXMATRIX*)&ProjectionMatrix);
		MatrixInverse(InvProjectionMatrix,ProjectionMatrix);
		NixTargetWidth=NixTargetHeight=CubeMapSize[cube_map];
		DXViewPort.X=DXViewPort.Y=0;
		DXViewPort.Width=DXViewPort.Height=CubeMapSize[cube_map];
		//lpDevice->SetViewport(&DXViewPort);

		if (mask&1){
			DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_POSITIVE_X,0);
			SetCubeMatrix(pos,vector(0,pi/2,0));
			render_func();
		}
		if (mask&2){
			DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_NEGATIVE_X,0);
			SetCubeMatrix(pos,vector(0,-pi/2,0));
			render_func();
		}
		if (mask&4){
			DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_POSITIVE_Y,0);
			SetCubeMatrix(pos,vector(-pi/2,0,0));
			render_func();
		}
		if (mask&8){
			DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_NEGATIVE_Y,0);
			SetCubeMatrix(pos,vector(pi/2,0,0));
			render_func();
		}
		if (mask&16){
			DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_POSITIVE_Z,0);
			SetCubeMatrix(pos,vector(0,0,0));
			render_func();
		}
		if (mask&32){
			DXRenderToEnvMap[cube_map]->Face(D3DCUBEMAP_FACE_NEGATIVE_Z,0);
			SetCubeMatrix(pos,vector(0,pi,0));
			render_func();
		}
		DXRenderToEnvMap[cube_map]->End(0);

		/*lpDevice->BeginScene();
		SetCubeMatrix(pos,vector(0,pi,0));
		render_func();
		//lpDevice->EndScene();
		End();*/

		ViewMatrix=vm;
		NixSetView(true,ViewMatrix);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("RenderToCubeMap for OpenGL");
	}
#endif
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

void NixDrawStr(int x,int y,char *str)
{
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
		glColor3f(FontColor.r,FontColor.g,FontColor.b);
		glRasterPos3f(float(x),float(y+2+int(float(NixFontHeight)*0.75f)),-1.0f);
		glListBase(OGLFontDPList);
		glCallLists(strlen(str),GL_UNSIGNED_BYTE,str);
		glRasterPos3f(0,0,0);
	}
#endif
}

int NixGetStrWidth(char *str,int start,int end)
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
		glColor3fv(color2f3(c));
		//glLineWidth(1);
		/*glBegin(GL_LINE);
			glColor4fv(color2f4(c));
			glVertex3f((float)x1,(float)y1,depth);
			glVertex3f((float)x2,(float)y2,depth);
		glEnd();*/

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
	}
#endif
}

void NixDrawLineV(int x,int y1,int y2,color c,float depth)
{
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

void SetShaderFileData(int texture0,int texture1,int texture2)
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
		if (hdl=DXEffectCurrent->GetParameterByName(NULL,"ViewMatrix"))
			DXEffectCurrent->SetMatrix(hdl,(D3DXMATRIX*)&ViewMatrix);
		if (hdl=DXEffectCurrent->GetParameterByName(NULL,"ProjectionMatrix"))
			DXEffectCurrent->SetMatrix(hdl,(D3DXMATRIX*)&ProjectionMatrix);
		if (texture0>=0)
			if (hdl=DXEffectCurrent->GetParameterByName(NULL,"tex0"))
				DXEffectCurrent->SetTexture(hdl,DXTexture[texture0]);
		if (texture1>=0)
			if (hdl=DXEffectCurrent->GetParameterByName(NULL,"tex1"))
				DXEffectCurrent->SetTexture(hdl,DXTexture[texture1]);
		if (texture2>=0)
			if (hdl=DXEffectCurrent->GetParameterByName(NULL,"tex2"))
				DXEffectCurrent->SetTexture(hdl,DXTexture[texture2]);
	}
#endif
}

void NixDraw2D(int texture,color *col,rect *src,rect *dest,float depth)
{
	//if (depth==0)	depth=0.5f;
	rect s,d;
	color c;
	// no texture source -> use comlpete texture
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
	SetShaderFileData(texture,-1,-1);
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
				DXEffectCurrent->Pass(i);
				lpDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );
			}
		}else // draw a single time without the shader
			lpDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		depth=depth*2-1;
		OGLSet2DMode();
		glColor4fv(color2f4(c));
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

int NixLoadShaderFile(char *filename)
{
	if (strlen(filename)<1)
		return -1;
	msg_write(string("loading shader file ",SysFileName(filename)));
	msg_right();
#ifndef NIX_IDE_VCS
	msg_error("ignoring shader file....(no visual studio!)");
	msg_left();
	return -1;
#endif
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
												SysFileName(filename),
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
				char ErrStr[4096];
				for (unsigned int i=0;i<pBufferErrors->GetBufferSize();i++)
					ErrStr[i]=((char*)pBufferErrors->GetBufferPointer())[i];
				ErrStr[pBufferErrors->GetBufferSize()]=0;
				hui->ErrorBox(HuiWindow,"error in shader file",ErrStr);
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
		msg_todo("LoadShaderFile for OpenGL");
		msg_left();
		return -1;
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
		msg_todo("DeleteShaderFile for OpenGL");
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
		msg_todo("SetShaderFile for OpenGL");
	}
#endif
}

bool AllowWindowsKeyInput=false;

// Eingaben vonm Fenster entgegennehmen
void NixGetInputFromWindow()
{
	if (!HuiWindow)
		return;

	#ifdef NIX_OS_WINDOWS
		int i;
		POINT mpos;
		UINT message=HuiWindow->CompleteWindowMessage.msg;		HuiWindow->CompleteWindowMessage.msg=0;
		WPARAM wParam=HuiWindow->CompleteWindowMessage.wparam;	HuiWindow->CompleteWindowMessage.wparam=0;
		LPARAM lParam=HuiWindow->CompleteWindowMessage.lparam;	HuiWindow->CompleteWindowMessage.lparam=0;
		//HuiWindow->InputData=NixInputDataCurrent;

		switch(message){
			case WM_MOUSEMOVE:
				if (NixFullscreen){
					GetCursorPos(&mpos);
					HuiWindow->InputData.vx+=(float)mpos.x-NixScreenWidth/2.0f;
					HuiWindow->InputData.vy+=(float)mpos.y-NixScreenHeight/2.0f;
					SetCursorPos(NixScreenWidth/2,NixScreenHeight/2);
					// korrekte Mausposition
					/*HuiWindow->InputData.mx=NixInputDataCurrent.mx+HuiWindow->InputData.vx;
					HuiWindow->InputData.my=NixInputDataCurrent.my+HuiWindow->InputData.vy;*/
					// praktischere Mausposition
					HuiWindow->InputData.mx=NixInputDataCurrent.mx+float(HuiWindow->InputData.vx)/NixMouseMappingWidth*float(NixScreenWidth);
					HuiWindow->InputData.my=NixInputDataCurrent.my+float(HuiWindow->InputData.vy)/NixMouseMappingHeight*float(NixScreenHeight);
				}else{
					HuiWindow->InputData.mx=(float)LOWORD(lParam);
					HuiWindow->InputData.my=(float)HIWORD(lParam);
					if (HuiWindow->InputData.mx>32000)			HuiWindow->InputData.mx=0;
					if (HuiWindow->InputData.my>32000)			HuiWindow->InputData.my=0;
				}
				if (HuiWindow->InputData.mx<0)				HuiWindow->InputData.mx=0;
				if (HuiWindow->InputData.my<0)				HuiWindow->InputData.my=0;
				if (HuiWindow->InputData.mx>NixTargetWidth)	HuiWindow->InputData.mx=(float)NixTargetWidth;
				if (HuiWindow->InputData.my>NixTargetHeight)	HuiWindow->InputData.my=(float)NixTargetHeight;
				if (!NixFullscreen){
					HuiWindow->InputData.vx=HuiWindow->InputData.mx-NixInputDataLast.mx;
					HuiWindow->InputData.vy=HuiWindow->InputData.my-NixInputDataLast.my;
				}
				break;
			case WM_MOUSEWHEEL:
				HuiWindow->InputData.mwheel+=(short)HIWORD(wParam);
				break;
			case WM_LBUTTONDOWN:
				SetCapture(HuiWindow->hWnd);
				HuiWindow->InputData.lb=true;
				//HuiWindow->InputData.mx=LOWORD(lParam);
				//HuiWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_LBUTTONUP:
				ReleaseCapture();
				HuiWindow->InputData.lb=false;
				//HuiWindow->InputData.mx=LOWORD(lParam);
				//HuiWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_MBUTTONDOWN:
				SetCapture(HuiWindow->hWnd);
				HuiWindow->InputData.mb=true;
				//HuiWindow->InputData.mx=LOWORD(lParam);
				//HuiWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_MBUTTONUP:
				ReleaseCapture();
				HuiWindow->InputData.mb=false;
				//HuiWindow->InputData.mx=LOWORD(lParam);
				//HuiWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_RBUTTONDOWN:
				SetCapture(HuiWindow->hWnd);
				HuiWindow->InputData.rb=true;
				//HuiWindow->InputData.mx=LOWORD(lParam);
				//HuiWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_RBUTTONUP:
				ReleaseCapture();
				HuiWindow->InputData.rb=false;
				//HuiWindow->InputData.mx=LOWORD(lParam);
				//HuiWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_KEYDOWN:
				if (GetActiveWindow()==HuiWindow->hWnd){
					AllowWindowsKeyInput=true;
					if (HuiWindow->InputData.KeyBufferDepth>=HUI_MAX_KEYBUFFER_DEPTH-1){
						for (i=0;i<HuiWindow->InputData.KeyBufferDepth-2;i++)
							HuiWindow->InputData.KeyBuffer[i]=HuiWindow->InputData.KeyBuffer[i+1];
						HuiWindow->InputData.KeyBufferDepth--;
					}
					HuiWindow->InputData.KeyBuffer[HuiWindow->InputData.KeyBufferDepth]=KeyID[wParam];
					HuiWindow->InputData.KeyBufferDepth++;
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
		HuiWindow->InputData.mw=(float)atan2(-HuiWindow->InputData.vy-NixInputDataLast.vy,HuiWindow->InputData.vx+NixInputDataLast.vx);
		if (HuiWindow->InputData.mw<0)
			HuiWindow->InputData.mw+=2*pi;

		if (GetActiveWindow()!=HuiWindow->hWnd)
			AllowWindowsKeyInput=false;
	
		if (AllowWindowsKeyInput)
			for (i=0;i<256;i++)
				HuiWindow->InputData.key[KeyID[i]]=((GetAsyncKeyState(i)&(1<<15))!=0);
		else{
			for (i=0;i<256;i++)
				HuiWindow->InputData.key[i]=false;
		}

		// Korrektur (manche Tasten belegen mehrere Array-Elemente) :-S
		if (NixGetKey(KEY_RALT))
			HuiWindow->InputData.key[KEY_LCONTROL]=0;
	#endif
}

bool allow_mb=false;
int gdk_mx=0,gdk_my=0;

// Eingaben behandeln
void NixUpdateInput()
{
	NixInputDataLast=NixInputDataCurrent;

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		NixInputDataCurrent=HuiWindow->InputData;
	}
#endif
#ifdef NIX_API_OPENGL
	#ifdef NIX_OS_WINDOWS
		if (NixApi==NIX_API_OPENGL){
			NixInputDataCurrent=HuiWindow->InputData;
		}
	#endif
	#ifdef NIX_OS_LINUX
		int mod,mx,my;
		NixInputDataCurrent=HuiWindow->InputData;
		//GdkWindow *pw=gdk_window_get_pointer(HuiWindow->window->window,&mx,&my,(GdkModifierType*)&mod);
		GdkWindow *pw=gdk_window_get_pointer(HuiWindow->gl_widget->window,&mx,&my,(GdkModifierType*)&mod);

		if (NixFullscreen){
			NixInputDataCurrent.vx=mx-gdk_mx;
			NixInputDataCurrent.vy=my-gdk_my;
			NixInputDataCurrent.mx=NixInputDataLast.mx+NixInputDataCurrent.vx;
			NixInputDataCurrent.my=NixInputDataLast.my+NixInputDataCurrent.vy;
			if (NixInputDataCurrent.mx<0)					NixInputDataCurrent.mx=0;
			if (NixInputDataCurrent.mx>=NixScreenWidth)		NixInputDataCurrent.mx=NixScreenWidth-1;
			if (NixInputDataCurrent.my<0)					NixInputDataCurrent.my=0;
			if (NixInputDataCurrent.my>=NixScreenHeight)	NixInputDataCurrent.my=NixScreenHeight-1;
			// reset....
			Window win=GDK_WINDOW_XWINDOW(HuiWindow->gl_widget->window);
			XWarpPointer(display, None, win, 0, 0, 0, 0, NixScreenWidth/2, NixDesktopHeight-NixScreenHeight/2);
			XFlush(display);
			GdkWindow *pw=gdk_window_get_pointer(HuiWindow->window->window,&gdk_mx,&gdk_my,(GdkModifierType*)&mod);
		}else{
			NixInputDataCurrent.mx=mx;
			NixInputDataCurrent.my=my;//-OGLMenuBarHeight;
			NixInputDataCurrent.vx=NixInputDataCurrent.mx-NixInputDataLast.mx;
			NixInputDataCurrent.vy=NixInputDataCurrent.my-NixInputDataLast.my;
		}

		int xxx,yyy;
		pw=gdk_window_at_pointer(&xxx,&yyy);

		// refresh allow-state only if not pressed
		if (((mod&GDK_BUTTON1_MASK)==0)&&((mod&GDK_BUTTON2_MASK)==0)&&((mod&GDK_BUTTON3_MASK)==0))
			//allow_mb=(pw==HuiWindow->window->window);
			allow_mb=(pw==HuiWindow->gl_widget->window);

		msg_db_out(1,string2("mouse button state: %d",mod));
		if (allow_mb){
			msg_db_out(1,"mouse input allowed");
			NixInputDataCurrent.lb = (mod&GDK_BUTTON1_MASK);
			NixInputDataCurrent.mb = (mod&GDK_BUTTON2_MASK) || (mod&GDK_BUTTON4_MASK) || (mod&GDK_BUTTON5_MASK);
			NixInputDataCurrent.rb = (mod&GDK_BUTTON3_MASK);
		}else
			NixInputDataCurrent.lb=NixInputDataCurrent.mb=NixInputDataCurrent.rb=false;
		//gdk_window_set_events(HuiWindow->window->window,GdkEventMask(GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK));
		gdk_window_set_events(HuiWindow->gl_widget->window,GdkEventMask(GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK));
	#endif
#endif

	// noch nicht so ganz...
	//HuiWindow->InputData.lb=HuiWindow->InputData.mb=HuiWindow->InputData.rb=false;
	HuiWindow->InputData.vx=HuiWindow->InputData.vy=HuiWindow->InputData.mwheel=0;

	if (!KeyBufferRead){
		HuiWindow->InputData.KeyBufferDepth=0;
	}
	KeyBufferRead=false;
}

void NixDraw3D(int texture,int buffer,matrix *mat)
{
	if (buffer<0)	return;
	matrix m;
	// keine Matrix angegeben -> Matrix ohne Transformation
	if (!mat){	MatrixIdentity(m);	mat=&m;	}
	NixSetTexture(texture);
	// Transformations-Matrix Modell->Welt
	NixSetWorldMatrix(*mat);

	SetShaderFileData(texture,-1,-1);

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){

		DXSet3DMode();
		lpDevice->SetFVF(D3D_VERTEX3D);
		// Vertex-Punkt bergeben
		lpDevice->SetStreamSource(0,DXVBVertices[buffer],0,sizeof(DXVertex3D));

		if (VBIndexed[buffer]){
			lpDevice->SetIndices(DXVBIndex[buffer]);
			// darstellen (je 3 im IndexBuffer angegebene Punkte zu einem Dreieck zufassen)
			if (DXEffectCurrent){ // loop through the shader "passes"
				for (unsigned int i=0;i<DXEffectCurrentNumPasses;i++){
					DXEffectCurrent->Pass(i);
					lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,VBNumPoints[buffer],0,VBNumTrias[buffer]);
				}
			}else // draw a single time without the shader
				lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,VBNumPoints[buffer],0,VBNumTrias[buffer]);
		}else{
			// darstellen (je 3 aufeinander folgende Punkte zu einem Dreieck zufassen)
			if (DXEffectCurrent){ // loop through the shader "passes"
				for (unsigned int i=0;i<DXEffectCurrentNumPasses;i++){
					DXEffectCurrent->Pass(i);
					lpDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,VBNumTrias[buffer]);
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
		glInterleavedArrays(GL_T2F_N3F_V3F,0,OGLVBVertices[buffer]);
		glDrawArrays(GL_TRIANGLES,0,VBNumTrias[buffer]*3);
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

void NixDraw3D3(int texture0,int texture1,int texture2,int buffer,matrix *mat)
{
	if (buffer<0)	return;
	matrix m;
	// keine Matrix angegeben -> Matrix ohne Transformation
	if (!mat){	MatrixIdentity(m);	mat=&m;	}
	NixSetTexture(texture0);
	// Transformations-Matrix Modell->Welt
	NixSetWorldMatrix(*mat);

	SetShaderFileData(texture0,texture1,texture2);

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		
		// multi texturing
		if (texture1>=0)
			lpDevice->SetTexture(1,DXTexture[texture1]);
		else
			lpDevice->SetTexture(1,NULL);
		if (texture2>=0)
			lpDevice->SetTexture(2,DXTexture[texture2]);
		else
			lpDevice->SetTexture(2,NULL);

		DXSet3DMode();
		lpDevice->SetFVF(D3D_VERTEX3D3);
		// Vertex-Punkt bergeben
		lpDevice->SetStreamSource(0,DXVBVertices[buffer],0,sizeof(DXVertex3D3));

		if (VBIndexed[buffer]){
			lpDevice->SetIndices(DXVBIndex[buffer]);
			// darstellen (je 3 im IndexBuffer angegebene Punkte zu einem Dreieck zufassen)
			if (DXEffectCurrent){ // loop through the shader "passes"
				for (unsigned int i=0;i<DXEffectCurrentNumPasses;i++){
					DXEffectCurrent->Pass(i);
					lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,VBNumPoints[buffer],0,VBNumTrias[buffer]);
				}
			}
			else // draw a single time without the shader
				lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,VBNumPoints[buffer],0,VBNumTrias[buffer]);
		}
		else{
			// darstellen (je 3 aufeinander folgende Punkte zu einem Dreieck zufassen)
			if (DXEffectCurrent){ // loop through the shader "passes"
				for (unsigned int i=0;i<DXEffectCurrentNumPasses;i++){
					DXEffectCurrent->Pass(i);
					lpDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,VBNumTrias[buffer]);
				}
			}
			else // draw a single time without the shader
				lpDevice->DrawPrimitive(D3DPT_TRIANGLELIST,0,VBNumTrias[buffer]);
		}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLSet3DMode();
		for (int i=0;i<VBNumTrias[buffer];i++){
			glBegin(GL_TRIANGLES);
				glTexCoord2f(	OGLVBVertices[buffer][i*3  ].tu,1-OGLVBVertices[buffer][i*3  ].tv);
				glNormal3f(		OGLVBVertices[buffer][i*3  ].nx,  OGLVBVertices[buffer][i*3  ].ny,OGLVBVertices[buffer][i*3  ].nz);
				glVertex3f(		OGLVBVertices[buffer][i*3  ].x ,  OGLVBVertices[buffer][i*3  ].y ,OGLVBVertices[buffer][i*3  ].z );
				glTexCoord2f(	OGLVBVertices[buffer][i*3+1].tu,1-OGLVBVertices[buffer][i*3+1].tv);
				glNormal3f(		OGLVBVertices[buffer][i*3+1].nx,  OGLVBVertices[buffer][i*3+1].ny,OGLVBVertices[buffer][i*3+1].nz);
				glVertex3f(		OGLVBVertices[buffer][i*3+1].x ,  OGLVBVertices[buffer][i*3+1].y ,OGLVBVertices[buffer][i*3+1].z );
				glTexCoord2f(	OGLVBVertices[buffer][i*3+2].tu,1-OGLVBVertices[buffer][i*3+2].tv);
				glNormal3f(		OGLVBVertices[buffer][i*3+2].nx,  OGLVBVertices[buffer][i*3+2].ny,OGLVBVertices[buffer][i*3+2].nz);
				glVertex3f(		OGLVBVertices[buffer][i*3+2].x ,  OGLVBVertices[buffer][i*3+2].y ,OGLVBVertices[buffer][i*3+2].z );
			glEnd();
		}
		msg_todo("Draw3D3 for OpenGL");
	}
#endif
	NixNumTrias+=VBNumTrias[buffer];
}

void NixDraw3DCubeMapped(int cube_map,int buffer,matrix *mat)
{
	if (buffer<0)	return;

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetTexture(0,DXCubeMap[cube_map]);
		lpDevice->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
		matrix tm=ViewMatrix;
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

void NixGetVecProject(vector &vout,const vector &vin)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXVec3Project((D3DXVECTOR3*)&vout,(D3DXVECTOR3*)&vin,&DXViewPort,(D3DXMATRIX*)&ProjectionMatrix,(D3DXMATRIX*)&ViewMatrix,NULL);
		/*VecTransform(vout,ViewMatrix,vin);
		VecTransform(vout,ProjectionMatrix,vout);*/
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		/*matrix m;
		MatrixIdentity(m);*/
		double vm[16];
		int i;
		for (i=0;i<16;i++)
			vm[i]=ViewMatrix.e[i];
			//vm[i]=m.e[i];
		double pm[16];
		for (i=0;i<16;i++)
				pm[i]=ProjectionMatrix.e[i];
			//pm[i]=m.e[i];
		double x,y,z;
		gluProject(vin.x,vin.y,vin.z,vm,pm,OGLViewPort,&x,&y,&z);
		vout.x=(float)x;
		vout.y=float((OGLViewPort[1]*2+OGLViewPort[3])-y); // y-Spiegelung
		vout.z=(float)z;//0.999999970197677613f;//(float)z;
		/*VecTransform(vout,ViewMatrix,vin);
		VecTransform(vout,ProjectionMatrix,vout);
		vout.y=((ViewPort[1]*2+ViewPort[3])-vout.y*16)/2;
		vout.x=((ViewPort[0]*2+ViewPort[2])+vout.x*16)/2;
		vout.z=0.99999997f;*/
	}
#endif
}

void NixGetVecProjectRel(vector &vout,const vector &vin)
{
	NixGetVecProject(vout,vin);
	vout.x/=(float)NixTargetWidth;
	vout.y/=(float)NixTargetHeight;
}

void NixGetVecUnproject(vector &vout,const vector &vin)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXVec3Unproject((D3DXVECTOR3*)&vout,(D3DXVECTOR3*)&vin,&DXViewPort,(D3DXMATRIX*)&ProjectionMatrix,(D3DXMATRIX*)&ViewMatrix,NULL);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		double vin_y=OGLViewPort[1]*2+OGLViewPort[3]-(double)vin.y; // y-Spiegelung
		double vm[16];
		int i;
		for (i=0;i<16;i++)
			vm[i]=ViewMatrix.e[i];
		double pm[16];
		for (i=0;i<16;i++)
			pm[i]=ProjectionMatrix.e[i];
		double x,y,z;
		gluUnProject(vin.x,vin_y,vin.z,vm,pm,OGLViewPort,&x,&y,&z);
		vout.x=(float)x;
		vout.y=(float)y;
		vout.z=(float)z;
	}
#endif
}

void NixGetVecUnprojectRel(vector &vout,const vector &vin)
{
	vector vi_r=vin;
	vi_r.x/=(float)NixTargetWidth;
	vi_r.y/=(float)NixTargetHeight;
	NixGetVecUnproject(vout,vi_r);
}

void NixDrawSpriteR(int texture,color *col,rect *src,vector &pos,rect *dest)
{
	rect d;
	float depth;
	vector p;
	NixGetVecProject(p,pos);
	if ((p.z<=0.0f)||(p.z>=1.0))
		return;
	depth=p.z;
	vector u;
	VecTransform(u,ViewMatrix,pos);
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

void NixDrawSprite(int texture,color *col,rect *src,vector &pos,float radius)
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
	if (create)
		index=NumVBs;
	if (max_trias<=0) // unnoetiger Speicher
		return -1;
	msg_write("creating vertex buffer");
	if ((create)&&(NumVBs>=NIX_MAX_VBS)){
		msg_error("too many vertex buffers");
		return -1;
	}
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
		OGLVBVertices[index]=new OGLVertex3D[max_trias*3];
		if (!OGLVBVertices[index]){
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
	msg_ok();
	VBNumTrias[index]=VBNumPoints[index]=0;
	VBMaxTrias[index]=max_trias;
	//VBMaxPoints[index]=max_trias;
	if (create)
		NumVBs++;
	msg_left();
	return index;
}
int NixCreateVB3(int max_trias,int index)
{
	bool create=(index<0);
	if (create)
		index=NumVBs;
	if (max_trias<=0) // unnoetiger Speicher
		return -1;
	msg_write("creating vertex buffer (3 tex coords!)");
	if ((create)&&(NumVBs>=NIX_MAX_VBS)){
		msg_error("too many vertex buffers");
		return -1;
	}
	msg_right();
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		// VertexBuffer selbst
		HRESULT hr;
		hr=lpDevice->CreateVertexBuffer(	3*max_trias*sizeof(DXVertex3D3),
											D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
											D3D_VERTEX3D3,
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
		OGLVBVertices[index]=new OGLVertex3D[max_trias];
		if (!OGLVBVertices[index]){
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
		msg_todo("NixCreateVB3 for OpenGL");
	}
#endif
	msg_ok();
	VBNumTrias[index]=VBNumPoints[index]=0;
	VBMaxTrias[index]=max_trias;
	//VBMaxPoints[index]=max_trias;
	if (create)
		NumVBs++;
	msg_left();
	return index;
}

bool NixVBAddTria(int buffer,vector &p1,vector &n1,float tu1,float tv1,
								vector &p2,vector &n2,float tu2,float tv2,
								vector &p3,vector &n3,float tu3,float tv3)
{
	if (VBNumTrias[buffer]>VBMaxTrias[buffer]){
		msg_error("too many triangles in the vertex buffer!");
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
		OGLVertex3D a;
		a.x=p1.x;	a.y=p1.y;	a.z=p1.z;
		a.nx=n1.x;	a.ny=n1.y;	a.nz=n1.z;
		a.tu=tu1;	a.tv=1-tv1;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3]=a;
		a.x=p2.x;	a.y=p2.y;	a.z=p2.z;
		a.nx=n2.x;	a.ny=n2.y;	a.nz=n2.z;
		a.tu=tu2;	a.tv=1-tv2;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3+1]=a;
		a.x=p3.x;	a.y=p3.y;	a.z=p3.z;
		a.nx=n3.x;	a.ny=n3.y;	a.nz=n3.z;
		a.tu=tu3;	a.tv=1-tv3;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3+2]=a;
	}
#endif
	VBNumTrias[buffer]++;
	VBIndexed[buffer]=false;
	return true;
}

bool NixVBAddTria3(int buffer,	vector &p1,vector &n1,float tu01,float tv01,float tu11,float tv11,float tu21,float tv21,
									vector &p2,vector &n2,float tu02,float tv02,float tu12,float tv12,float tu22,float tv22,
									vector &p3,vector &n3,float tu03,float tv03,float tu13,float tv13,float tu23,float tv23)
{
	if (VBNumTrias[buffer]>VBMaxTrias[buffer]){
		msg_error("too many triangles in the vertex buffer!");
		return false;
	}
	//msg_write("VertexBufferAddTriangle");
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		unsigned char *pVerts=NULL;
		DXVertex3D3 Vert[3];
		Vert[0].x=p1.x;	Vert[0].y=p1.y;	Vert[0].z=p1.z;	Vert[0].nx=n1.x;	Vert[0].ny=n1.y;	Vert[0].nz=n1.z;	Vert[0].tu0=tu01;	Vert[0].tv0=tv01;
		Vert[1].x=p2.x;	Vert[1].y=p2.y;	Vert[1].z=p2.z;	Vert[1].nx=n2.x;	Vert[1].ny=n2.y;	Vert[1].nz=n2.z;	Vert[1].tu0=tu02;	Vert[1].tv0=tv02;
		Vert[2].x=p3.x;	Vert[2].y=p3.y;	Vert[2].z=p3.z;	Vert[2].nx=n3.x;	Vert[2].ny=n3.y;	Vert[2].nz=n3.z;	Vert[2].tu0=tu03;	Vert[2].tv0=tv03;

		Vert[0].tu1=tu11;	Vert[0].tv1=tv11;
		Vert[1].tu1=tu12;	Vert[1].tv1=tv12;
		Vert[2].tu1=tu13;	Vert[2].tv1=tv13;

		Vert[0].tu2=tu21;	Vert[0].tv2=tv21;
		Vert[1].tu2=tu22;	Vert[1].tv2=tv22;
		Vert[2].tu2=tu23;	Vert[2].tv2=tv23;

		DXVBVertices[buffer]->Lock(sizeof(DXVertex3D3)*3*VBNumTrias[buffer],sizeof(DXVertex3D3)*3,(void**)&pVerts,0);
		memcpy(pVerts,Vert,sizeof(DXVertex3D3)*3);
		DXVBVertices[buffer]->Unlock();
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		OGLVertex3D a;
		a.x=p1.x;	a.y=p1.y;	a.z=p1.z;
		a.nx=n1.x;	a.ny=n1.y;	a.nz=n1.z;
		a.tu=tu01;	a.tv=tv01;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3]=a;
		a.x=p2.x;	a.y=p2.y;	a.z=p2.z;
		a.nx=n2.x;	a.ny=n2.y;	a.nz=n2.z;
		a.tu=tu02;	a.tv=tv02;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3+1]=a;
		a.x=p3.x;	a.y=p3.y;	a.z=p3.z;
		a.nx=n3.x;	a.ny=n3.y;	a.nz=n3.z;
		a.tu=tu03;	a.tv=tv03;
		OGLVBVertices[buffer][VBNumTrias[buffer]*3+2]=a;
		msg_todo("VBAddTria3 for OpenGL");
	}
#endif
	VBNumTrias[buffer]++;
	VBIndexed[buffer]=false;
	return true;
}

// for each triangle there have to be 3 vertices (p[i],n[i],t[i*2],t[i*2+1])
void NixVBAddTrias(int buffer,int num_trias,vector *p,vector *n,float *t)
{
	#ifdef NIX_API_DIRECTX9
		if (NixApi==NIX_API_DIRECTX9){
			int i;
			// fill our vertex buffer
			unsigned char *pVerts=NULL;
			DXVertex3D Vert;
			DXVBVertices[buffer]->Lock(0,sizeof(DXVertex3D)*num_trias*3,(void**)&pVerts,0);
			for (i=0;i<num_trias*3;i++){
				Vert.x=p[i].x;	Vert.y=p[i].y;	Vert.z=p[i].z;
				Vert.nx=n[i].x;	Vert.ny=n[i].y;	Vert.nz=n[i].z;
				Vert.tu=t[i*2];	Vert.tv=t[i*2+1];
				memcpy(pVerts,&Vert,sizeof(DXVertex3D));
				pVerts+=sizeof(DXVertex3D);
			}
			DXVBVertices[buffer]->Unlock();
		}
	#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		for (int i=0;i<num_trias*3;i++){
			OGLVertex3D a;
			a.x=p[i].x;		a.y=p[i].y;		a.z=p[i].z;
			a.nx=n[i].x;	a.ny=n[i].y;	a.nz=n[i].z;
			a.tu=t[i*2  ];	a.tv=1-t[i*2+1];
			OGLVBVertices[buffer][VBNumTrias[buffer]*3+i]=a;
		}
	}
#endif
	VBNumTrias[buffer]+=num_trias;
	VBNumPoints[buffer]+=num_trias*3;
}

void NixVBAddTriasIndexed(int buffer,int num_points,int num_trias,vector *p,vector *n,float *tu,float *tv,unsigned short *indices)
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
		if (WireFrame)
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
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
		}
		else{
			if (Write){
				lpDevice->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
				lpDevice->SetRenderState(D3DRS_ZENABLE,TRUE);
				lpDevice->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
			}
			else{
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
		}
		else{
			if (Write){
				glEnable(GL_DEPTH);
				//glDisable(GL_DEPTH_TEST);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_ALWAYS);
				glDepthMask(1);
			}
			else{
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
				// flieï¿½nde Rand-ï¿½ergï¿½ge
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
		DWORD f=0x00ffffff+256*256*256*(int)(factor*255.0f);
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
		//glStencilMask(0xffffffff);
		if (mode==StencilNone){
			glDisable(GL_STENCIL);
			glDisable(GL_STENCIL_TEST);
		}
		if (mode==StencilReset)
			glClearStencil(param);
		if ((mode==StencilIncrease)||(mode==StencilDecrease)||(mode==StencilSet)){
			glEnable(GL_STENCIL);
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS,param,0xffffffff);
			if (mode==StencilIncrease)
				glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
			if (mode==StencilDecrease)
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

void NixSetMaterial(color &ambient,color &diffuse,color &specular,float shininess,color &emission)
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
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,color2f4(ambient));
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,color2f4(diffuse));
		glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,color2f4(specular));
		glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,(float*)&shininess);
		glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,color2f4(emission));
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
void NixSetFog(int mode,float start,float end,float density,color &c)
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
		glFogfv(GL_FOG_COLOR,color2f4(c));
		//glFogf(GL_FOG_DENSITY,0.35f);
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

void NixSetWorldMatrix(matrix &mat)
{
	WorldMatrix=mat;
	MatrixMultiply(WorldViewProjectionMatrix,ViewMatrix,WorldMatrix);
	MatrixMultiply(WorldViewProjectionMatrix,ProjectionMatrix,WorldViewProjectionMatrix);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetTransform(D3DTS_WORLD,(D3DXMATRIX*)&mat);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((float*)&ViewMatrix);
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
		MatrixMultiply(ViewMatrix,r,t);
		//MatrixMultiply(ViewMatrix,s,ViewMatrix);
	//}
	NixSetView(enable3d,ViewMatrix);

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
void NixSetView(bool enable3d,matrix &view_mat)
{
	//SetCull(CullCCW); // ???
	ViewMatrix=view_mat;

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetTransform(D3DTS_VIEW,(D3DXMATRIX*)&ViewMatrix);

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
			MatrixMultiply(ProjectionMatrix,t,p);
			MatrixMultiply(ProjectionMatrix,ProjectionMatrix,s);
			MatrixMultiply(ProjectionMatrix,ProjectionMatrix,s2); // richtige Reihenfolge??????  ...bei Gelegenheit testen!
		}else{
			//msg_todo("NixSetView(2D) fuer DirectX9 (Sonderlichkeiten bei Target!=Screen ???)");
			//MatrixScale(s,1.0f/(float)NixTargetWidth,1.0f/(float)NixTargetHeight,1.0f/(float)NixMaxDepth);
			MatrixScale(s,2*View2DScaleX/(float)NixScreenWidth,2*View2DScaleY/(float)NixScreenHeight,1.0f/(float)NixMaxDepth);
			MatrixTranslation(t,vector(View3DCenterX/float(NixScreenWidth)*2.0f-1,1-View3DCenterY/float(NixScreenHeight)*2.0f,0.5f+ViewPos.z));
			MatrixMultiply(ProjectionMatrix,t,s);
			MatrixScale(s,ViewScale.x,ViewScale.y,ViewScale.z);
			MatrixMultiply(ProjectionMatrix,s,ProjectionMatrix);
		}
		lpDevice->SetTransform(D3DTS_PROJECTION,(D3DXMATRIX*)&ProjectionMatrix);
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
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&ProjectionMatrix);
			//mout(ProjectionMatrix);
			// perspektivische Verzerrung
			gluPerspective(60.0f,View3DRatio,NixMinDepth,NixMaxDepth);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&ProjectionMatrix);
			//mout(ProjectionMatrix);
			glScalef((View3DWidth/(float)NixTargetWidth),(View3DHeight/(float)NixTargetHeight),-1); // -1: Koordinatensystem: Links vs Rechts
			glScalef(ViewScale.x,ViewScale.y,ViewScale.z);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&ProjectionMatrix);
			//mout(ProjectionMatrix);
		}else{
			glTranslatef(View3DCenterX/float(NixTargetWidth)*2.0f-1,1-View3DCenterY/float(NixTargetHeight)*2.0f,0);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&ProjectionMatrix);
			//mout(ProjectionMatrix);
			glScalef(2*View2DScaleX/(float)NixTargetWidth,2*View2DScaleY/(float)NixTargetHeight,1.0f/(float)NixMaxDepth);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&ProjectionMatrix);
			//mout(ProjectionMatrix);
		}
		// Matrix speichern
		glGetFloatv(GL_PROJECTION_MATRIX,(float*)&ProjectionMatrix);

		// OpenGL muss Lichter neu ausrichten, weil sie in Kamera-Koordinaten gespeichert werden!
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		//glLoadIdentity();
		glLoadMatrixf((float*)&ViewMatrix);
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
	MatrixInverse(InvProjectionMatrix,ProjectionMatrix);
	Enabled3D=enable3d;
}

void NixSetViewV(bool enable3d,vector &view_pos,vector &view_ang)
{	NixSetView(enable3d,view_pos,view_ang);	}

void NixSetViewM(bool enable3d,matrix &view_mat)
{
	ViewScale=vector(1,1,1);
	NixSetView(enable3d,view_mat);
}



#define FrustrumAngleCos	0.83f

bool NixIsInFrustrum(vector &pos,float radius)
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
void NixSetLightRadial(int index,vector &pos,float radius,color &ambient,color &diffuse,color &specular)
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
		glLoadMatrixf((float*)&ViewMatrix);
		float f[4];
		f[0]=pos.x;	f[1]=pos.y;	f[2]=pos.z;	f[3]=1;
		glLightfv(GL_LIGHT0+index,GL_POSITION,f); // GL_POSITION,(x,y,z,0)=directional,(x,y,z,1)=positional !!!
		glLightfv(GL_LIGHT0+index,GL_AMBIENT,color2f4(ambient));
		glLightfv(GL_LIGHT0+index,GL_DIFFUSE,color2f4(diffuse));
		glLightfv(GL_LIGHT0+index,GL_SPECULAR,color2f4(specular));
		glLightf(GL_LIGHT0+index,GL_CONSTANT_ATTENUATION,0.9f);
		glLightf(GL_LIGHT0+index,GL_LINEAR_ATTENUATION,2.0f/radius);
		glLightf(GL_LIGHT0+index,GL_QUADRATIC_ATTENUATION,1/(radius*radius));
		glPopMatrix();
	}
#endif
}

// parallele Quelle
// dir =Richtung, in die das Licht scheinen soll
void NixSetLightDirectional(int index,vector &dir,color &ambient,color &diffuse,color &specular)
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
		glLoadMatrixf((float*)&ViewMatrix);
		float f[4];
		f[0]=dir.x;	f[1]=dir.y;	f[2]=dir.z;	f[3]=0;
		glLightfv(GL_LIGHT0+index,GL_POSITION,f); // GL_POSITION,(x,y,z,0)=directional,(x,y,z,1)=positional !!!
		glLightfv(GL_LIGHT0+index,GL_AMBIENT,color2f4(ambient));
		glLightfv(GL_LIGHT0+index,GL_DIFFUSE,color2f4(diffuse));
		glLightfv(GL_LIGHT0+index,GL_SPECULAR,color2f4(specular));
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

void NixSetAmbientLight(color &c)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		int i=color2D3DCOLOR(c);
		lpDevice->SetRenderState(D3DRS_AMBIENT,i);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL)
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT,color2f3(c));
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
			hr=DXTextureRenderTarget[texture]->BeginScene(DXTextureSurface[texture],NULL);
			if (FAILED(hr)){
				msg_error(string("RenderToSurface-BeginScene: ",DXErrorMsg(hr)));
				return false;
			}
			//lpDevice->Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,D3DCOLOR_COLORVALUE(0.0f,0.0f,0.0f,1.0f),1.0f,0);
			lpDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER,0,1.0f,0);
		}
	}

#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		#ifdef NIX_OS_WINDOWS
			if (texture<0){
				#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
					if (glBindFramebufferEXT)
						glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
				#endif
				if (!wglMakeCurrent(hDC,hRC))
					return false;
			}else{
				#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
	//
	// Bind the frame-buffer object and attach to it a render-buffer object
	// set up as a depth-buffer.
	//
					glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, OGLFrameBuffer[texture] );
					//glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, OGLDepthRenderBuffer[texture] );
					glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, OGLTexture[texture], 0 );
					glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, OGLDepthRenderBuffer[texture] );
				#endif
			}
		#endif
		//glClearColor(0.0f,0.0f,0.0f,0.0f);
		glDisable(GL_SCISSOR_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
#endif

	// adjust target size
	if (texture<0){
		if (NixFullscreen){
			// fullscreen mode
			NixTargetWidth=NixScreenWidth;
			NixTargetHeight=NixScreenHeight;
		}else{
			// window mode
			irect ro=HuiWindow->GetOuterior();
			irect ri=HuiWindow->GetInterior();
			OGLMenuBarHeight=ri.y1-ro.y1;
			NixTargetWidth=ri.x2-ri.x1;
			NixTargetHeight=ri.y2-ri.y1;
		}
	}else{
		// texture
		NixTargetWidth=NixTextureWidth[texture];
		NixTargetHeight=NixTextureHeight[texture];
	}
	VPx1=VPy1=0;
	VPx2=NixTargetWidth;
	VPy2=NixTargetHeight;
	NixResize();
	Rendering=true;

	//msg_write("-ok?");
	return true;
}

void NixStartPart(int x1,int y1,int x2,int y2,bool set_centric)
{
	if ((x1<0)||(y1<0)||(x2<0)||(y2<0)){
		x1=0;	y1=0;	x2=NixTargetWidth;	y2=NixTargetHeight;
	}
	int h=NixTargetHeight,w=NixTargetWidth;
	VPx1=x1;
	VPy1=y1;
	VPx2=x2;
	VPy2=y2;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DRECT d,r[4];
		d.x1=x1;	d.y1=y1;	d.x2=x2;	d.y2=y2;	//	Ziel
		r[0].x1=0;	r[0].y1=0;	r[0].x2=w;	r[0].y2=y1;	//	Rand (oben gesammt)
		r[1].x1=0;	r[1].y1=y1;	r[1].x2=x1;	r[1].y2=y2;	//	Rand (links mitte)
		r[2].x1=x2;	r[2].y1=y1;	r[2].x2=w;	r[2].y2=y2;	//	Rand (rechts mitte)
		r[3].x1=0;	r[3].y1=y2;	r[3].x2=w;	r[3].y2=h;	//	Rand (unten gesammt)
		lpDevice->Clear(4,r,D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(0,0,0),-1.0f,0);
		lpDevice->Clear(1,&d,D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(0,0,0),1.0f,0);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glEnable(GL_SCISSOR_TEST);
		glScissor(x1,NixTargetHeight-y2,x2-x1,y2-y1);
	}
#endif
	if (set_centric){
		View3DCenterX=float(x1+x2)/2.0f;
		View3DCenterY=float(y1+y2)/2.0f;
		NixSetView(Enabled3D,ViewMatrix);
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
				irect r=HuiWindow->GetInterior();
				RECT R;	R.left=0;	R.right=r.x2-r.x1;	R.top=0;	R.bottom=r.y2-r.y1;
				lpDevice->Present(&R,NULL,NULL,NULL);
			}
		}else
			DXTextureRenderTarget[RenderingToTexture]->EndScene(0);
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
				if (NixFullscreen)
					XF86VidModeSetViewPort(display,screen,0,NixDesktopHeight-NixScreenHeight);
				//glutSwapBuffers();
				if (DoubleBuffered)
					glXSwapBuffers(display,GDK_WINDOW_XWINDOW(HuiWindow->gl_widget->window));
			#endif
		}
		#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
			if (glBindFramebufferEXT)
				glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
		#endif
	}
#endif
	for (int i=0;i<NumTextures;i++)
		if (TextureLifeTime[i]>=0){
			TextureLifeTime[i]++;
			if (TextureLifeTime[i]>=NixTextureMaxFramesToLive)
				NixUnloadTexture(i);
		}
}

void NixSetClipPlane(int index,plane &pl)
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
		glLoadMatrixf((float*)&ViewMatrix);
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

void NixDoScreenShot(char *filename,rect *source,int width,int height)
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
	msg_write(rect.x1);
	msg_write(rect.y1);
	msg_write(rect.x2);
	msg_write(rect.y2);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXSaveSurfaceToFile(SysFileName(filename),D3DXIFF_BMP,FrontBuffer,NULL,(RECT*)&rect);
		//D3DXSaveSurfaceToFile(SysFileName(filename),D3DXIFF_BMP,FrontBuffer,NULL,NULL);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		int dx=rect.x2-rect.x1;
		int dy=rect.y2-rect.y1;
		char *data=new char[dx*dy*4];
		glReadBuffer(GL_FRONT);
		glReadPixels(	rect.x1,
						rect.y1,
						dx,
						dy,
						GL_RGBA,GL_UNSIGNED_BYTE,data);
		for (int x=0;x<dx;x+=10)
			for (int y=0;y<dy;y+=10){
				int n1=(x+dx*y )*4;
				//msg_write(*(int*)&data[n1  ]);
			}
		for (int x=0;x<dx;x++)
			for (int y=0;y<(dy+1)/2;y++){
				int y2=dy-y-1;
				int n1=(x+dx*y )*4;
				int n2=(x+dx*y2)*4;
				int a =data[n1  ];
				int r =data[n1+1];
				int g =data[n1+2];
				int b =data[n1+3];
				int a2=data[n2  ];
				int r2=data[n2+1];
				int g2=data[n2+2];
				int b2=data[n2+3];
				data[n2+1]=a;
				data[n2+2]=r;
				data[n2+3]=g;
				data[n2  ]=b;
				data[n1+1]=a2;
				data[n1+2]=r2;
				data[n1+3]=g2;
				data[n2  ]=b2;
			}
		NixSaveTGA(filename,dx,dy,(NixScreenDepth==16)?16:24,0,data);
		delete(data);
	}
#endif
	msg_write(string("screenshot saved: ",SysFileName(filename)));
}

int NixGetDx()
{	return (int)NixInputDataCurrent.vx;	}

int NixGetDy()
{	return (int)NixInputDataCurrent.vy;	}

int NixGetWheelD()
{	return (int)NixInputDataCurrent.mwheel;	}

int NixGetMx()
{	return (int)NixInputDataCurrent.mx;	}

int NixGetMy()
{	return (int)NixInputDataCurrent.my;	}

float NixGetMDir()
{	return NixInputDataCurrent.mw;	}

void NixResetCursor()
{
	if (NixFullscreen){
		#ifdef NIX_OS_WINDOWS
			SetCursorPos(NixScreenWidth/2,NixScreenHeight/2);
		#endif
		HuiWindow->InputData.mx=NixInputDataCurrent.mx=NixScreenWidth/2.0f;
		HuiWindow->InputData.my=NixInputDataCurrent.my=NixScreenHeight/2.0f;
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
{	return NixInputDataCurrent.key[Key];	}

bool NixGetKeyUp(int Key)
{	return ((!NixInputDataCurrent.key[Key])&&(NixInputDataLast.key[Key]));	}

bool NixGetKeyDown(int Key)
{	return ((NixInputDataCurrent.key[Key])&&(!NixInputDataLast.key[Key]));	}

char NixGetKeyChar(int key)
{
	int i;
	if (key<0)	return 0;
	if ((NixGetKey(KEY_RCONTROL))||(NixGetKey(KEY_LCONTROL)))
		return 0;
	// shift
	if ((NixGetKey(KEY_RSHIFT))||(NixGetKey(KEY_LSHIFT))){
		for (i=0;i<26;i++)
			if (key==KEY_A+i)
				return 'A'+i;
		if (key==KEY_1)			return '!';
		if (key==KEY_2)			return '"';
		if (key==KEY_3)			return -89;
		if (key==KEY_4)			return '$';
		if (key==KEY_5)			return '%';
		if (key==KEY_6)			return '&';
		if (key==KEY_7)			return '/';
		if (key==KEY_8)			return '(';
		if (key==KEY_9)			return ')';
		if (key==KEY_0)			return '=';
		if (key==KEY_COMMA)		return ';';
		if (key==KEY_DOT)		return ':';
		if (key==KEY_ADD)		return '*';
		if (key==KEY_SUBTRACT)	return '_';
		if (key==KEY_SMALLER)	return '>';
		if (key==KEY_SZ)		return '?';
		if (key==KEY_AE)		return SysStr("&A")[0];
		if (key==KEY_OE)		return SysStr("&O")[0];
		if (key==KEY_UE)		return SysStr("&U")[0];
		if (key==KEY_FENCE)		return '\'';
		if (key==KEY_GRAVE)		return '°';
		if (key==KEY_SPACE)		return ' ';
		return 0;
	}
	// alt
	if (NixGetKey(KEY_RALT)){
		if (key==KEY_Q)			return '@';
		if (key==KEY_E)			return -128;
		if (key==KEY_Y)			return -91;
		if (key==KEY_2)			return -78;
		if (key==KEY_3)			return -77;
		if (key==KEY_7)			return '{';
		if (key==KEY_8)			return '[';
		if (key==KEY_9)			return ']';
		if (key==KEY_0)			return '}';
		if (key==KEY_ADD)		return '~';
		if (key==KEY_SMALLER)	return '|';
		if (key==KEY_SZ)		return '\\';
		return 0;
	}
	// normal
	for (i=0;i<26;i++)
		if (key==KEY_A+i)
			return 'a'+i;
	for (i=0;i<10;i++)
		if ((key==KEY_0+i)||(key==KEY_NUM_0+i))
			return '0'+i;
	if (key==KEY_COMMA)			return ',';
	if (key==KEY_DOT)			return '.';
	if (key==KEY_ADD)			return '+';
	if (key==KEY_SUBTRACT)		return '-';
	if (key==KEY_FENCE)			return '#';
	if (key==KEY_GRAVE)			return '^';
	if (key==KEY_NUM_ADD)		return '+';
	if (key==KEY_NUM_SUBTRACT)	return '-';
	if (key==KEY_NUM_MULTIPLY)	return '*';
	if (key==KEY_NUM_DIVIDE)	return '/';
	if (key==KEY_SMALLER)		return '<';
	if (key==KEY_SZ)			return SysStr("&s")[0];
	if (key==KEY_AE)			return SysStr("&a")[0];
	if (key==KEY_OE)			return SysStr("&o")[0];
	if (key==KEY_UE)			return SysStr("&u")[0];
	if (key==KEY_SPACE)			return ' ';
	if (key==KEY_TAB)			return '\t';
	if (key==KEY_RETURN)		return '\n';
	if (key==KEY_NUM_ENTER)		return '\n';
	// unbekannt:
	return 0;
}

int NixGetKeyRhythmDown()
{
	KeyBufferRead=true;
	if (HuiWindow->InputData.KeyBufferDepth>0){
		int k=HuiWindow->InputData.KeyBuffer[0];
		//msg_write(GetKeyName(k));
		for (int i=0;i<HuiWindow->InputData.KeyBufferDepth-2;i++)
			HuiWindow->InputData.KeyBuffer[i]=HuiWindow->InputData.KeyBuffer[i+1];
		HuiWindow->InputData.KeyBufferDepth--;
		return k;
	}else
		return -1;
}

int NixSoundLoad(char *filename)
{
	#ifdef NIX_SOUND_DIRECTX9
		msg_write(string("loading Sound: ",SysFileName(filename)));
		msg_right();

		int index=-1;
		for (int i=0;i<NumSounds;i++)
			if (!sound[i]){
				//msg_write("greife alten auf");
				index=i;
				break;
			}
		if (index<0){
			//msg_write("erstelle neuen");
			index=NumSounds;
			NumSounds++;
		}

		HRESULT hr;
		HRESULT hrRet = S_OK;
		LPDIRECTSOUNDBUFFER apDSBuffer;
		DWORD                dwDSBufferSize;
		CWaveFile*           pWaveFile      = NULL;


		pWaveFile = new CWaveFile();

		pWaveFile->Open( SysFileName(filename), NULL, WAVEFILE_READ );


		if ( pWaveFile->GetSize() == 0 ){
			msg_error("enthaelt keine Sound-Informationen");
			return -1;
		}

		// Make the DirectSound buffer the same size as the wav file
		dwDSBufferSize = pWaveFile->GetSize();

		// Create the direct sound buffer, and only request the flags needed
		// since each requires some overhead and limits if the buffer can 
		// be hardware accelerated
		DSBUFFERDESC dsBufferDesc;
		ZeroMemory( &dsBufferDesc, sizeof(DSBUFFERDESC) );
		dsBufferDesc.dwSize				= sizeof(DSBUFFERDESC);
		//DWORD dwCreationFlags			= DSBCAPS_CTRL3D | DSBCAPS_MUTE3DATMAXDISTANCE | DSBCAPS_LOCSOFTWARE;
		DWORD dwCreationFlags			= DSBCAPS_CTRL3D | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME;
		dsBufferDesc.dwFlags			= dwCreationFlags;
		dsBufferDesc.dwBufferBytes		= dwDSBufferSize;
		dsBufferDesc.guid3DAlgorithm	= DS3DALG_HRTF_LIGHT; // DS3DALG_NO_VIRTUALIZATION, DS3DALG_HRTF_FULL, DS3DALG_HRTF_LIGHT
		dsBufferDesc.lpwfxFormat		= pWaveFile->m_pwfx;
    
		// DirectSound is only guarenteed to play PCM data.  Other
		// formats may or may not work depending the sound card driver.
		hr = pDS->CreateSoundBuffer( &dsBufferDesc, &apDSBuffer, NULL );

		// Be sure to return this error code if it occurs so the
		// callers knows this happened.
		if( hr == DS_NO_VIRTUALIZATION )
	        hrRet = DS_NO_VIRTUALIZATION;

	    if (FAILED(hr)){
			// DSERR_BUFFERTOOSMALL will be returned if the buffer is
			// less than DSBSIZE_FX_MIN and the buffer is created
			// with DSBCAPS_CTRLFX.
        
			// It might also fail if hardware buffer mixing was requested
			// on a device that doesn't support it.
			msg_error("CreateSoundBuffer");
			msg_error(string("CreateSoundBuffer: ",DXErrorMsg(hr)));
			return -1;
		}

    
		// create the sound
		sound[index] = new CSound( &apDSBuffer, dwDSBufferSize, 1, pWaveFile, dwCreationFlags );

	    // get the 3D buffer from the secondary buffer
		hr=sound[index]->m_apDSBuffer[0]->QueryInterface(IID_IDirectSound3DBuffer,(VOID**)&pDS3DBuffer[index]);
	    if (FAILED(hr)){
			msg_error(string("Get3DBufferInterface: ",DXErrorMsg(hr)));
			return -1;
		}

		// Get the 3D buffer parameters
		dsBufferParams[index].dwSize = sizeof(DS3DBUFFER);
		pDS3DBuffer[index]->GetAllParameters( &dsBufferParams[index] );

		// Set new 3D buffer parameters
		dsBufferParams[index].dwMode = DS3DMODE_HEADRELATIVE;
		pDS3DBuffer[index]->SetAllParameters( &dsBufferParams[index], DS3D_IMMEDIATE );

		DSBCAPS dsbcaps;
		ZeroMemory( &dsbcaps, sizeof(DSBCAPS) );
		dsbcaps.dwSize = sizeof(DSBCAPS);

		SoundPos[index]=SoundVel[index]=vector(0,0,0);
		SoundMinDist[index]=1;
		SoundMaxDist[index]=2;
		SoundRate[index]=1;
		SoundSpeed[index]=1;
		SoundVolume[index]=1;
		sound[index]->m_apDSBuffer[0]->GetFrequency(&SoundFrequency[index]);
		sound[index]->m_apDSBuffer[0]->SetVolume(DSBVOLUME_MAX);

		msg_ok();
		msg_left();
		return index;
	#else
		msg_todo("SoundLoad without DirectX9");
	#endif
	return -1;
}

bool SoundUsable(int index)
{
	#ifdef NIX_SOUND_DIRECTX9
		if ((index<0)||(index>=NumSounds))
			return false;
		if (sound[index])
			return true;
		return false;
	#endif
	return false;
}

void NixSoundDelete(int index)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		pDS3DBuffer[index]->Release();
		pDS3DBuffer[index]=NULL;
		sound[index]->~CSound();
		sound[index]=NULL;
	#endif
}

void NixSoundPlay(int index,bool repeat)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		//msg_write(string("Play ",i2s(index)));
		//nw->SafeMessage("Play");
		float vol=float(pow(SoundVolume[index],0.15f));
		sound[index]->Play( 0, repeat?DSBPLAY_LOOPING:0 );
		//nw->SafeMessage("/Play");
		//msg_write("  -ok");
	#endif
}

void NixSoundStop(int index)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		sound[index]->Stop();
	#endif
}

void NixSoundSetPause(int index,bool pause)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		//sound[index]->;
	#endif
}

bool NixSoundIsPlaying(int index)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return false;
		DWORD dwStatus=0;
		sound[index]->m_apDSBuffer[0]->GetStatus(&dwStatus);
		return (( dwStatus&DSBSTATUS_PLAYING )!=0);
	#endif
	return false;
}

bool NixSoundEnded(int index)
{
	return false;
}

void NixSoundTestRepeat()
{
}

vector ListenerVel;
matrix ListenerInvMatrix;

void NixSoundSetData(int index,vector &pos,vector &vel,float min_dist,float max_dist,float speed,float volume,bool set_now)
{
	#ifdef NIX_SOUND_DIRECTX9
		if (!SoundUsable(index))
			return;
		//msg_write(string("SetData ",i2s(index)));
		SoundPos[index]=pos;
		SoundVel[index]=vel;
		SoundMinDist[index]=min_dist;
		SoundMaxDist[index]=max_dist;
		SoundSpeed[index]=speed;
		SoundVolume[index]=volume;

		if (set_now){
			// Frequenz
			int f=int(float(SoundFrequency[index])*SoundSpeed[index]*SoundRate[index]);
			sound[index]->m_apDSBuffer[0]->SetFrequency(f);

			vector dPos,dVel;
			VecTransform(dPos,ListenerInvMatrix,SoundPos[index]);
			dVel=SoundVel[index]-ListenerVel;
			VecNormalTransform(dVel,ListenerInvMatrix,dVel);
			memcpy( &dsBufferParams[index].vPosition, &dPos, sizeof(D3DVECTOR) );
			memcpy( &dsBufferParams[index].vVelocity, &dVel, sizeof(D3DVECTOR) );
			dsBufferParams[index].flMinDistance=SoundMinDist[index];
			dsBufferParams[index].flMaxDistance=SoundMaxDist[index];
			pDS3DBuffer[index]->SetAllParameters( &dsBufferParams[index], DS3D_IMMEDIATE );
			// Lautstï¿½ke
			float vol=float(pow(SoundVolume[index],0.15f));
			bool should_play=((VecLength(dPos)<SoundMaxDist[index])&&(SoundSpeed[index]<10.0f)&&(SoundSpeed[index]>0.001f));
			if (should_play)
				sound[index]->m_apDSBuffer[0]->SetVolume( long(DSBVOLUME_MAX*vol + DSBVOLUME_MIN*(1-vol)) );
			else
				sound[index]->m_apDSBuffer[0]->SetVolume(DSBVOLUME_MIN);
		}
	#endif
}

// setzt auch alle anderen Aenderungen der Sounds erst in Kraft
void NixSoundSetListener(vector &pos,vector &ang,vector &vel,float meters_per_unit)
{
	#ifdef NIX_SOUND_DIRECTX9
		int i;
		//msg_write("Listener");
		matrix a,b;
		MatrixTranslation(a,pos);
		MatrixRotation(b,ang);
		MatrixMultiply(a,a,b);
		MatrixInverse(ListenerInvMatrix,a);
		ListenerVel=vel;
		dsListenerParams.flDistanceFactor=meters_per_unit;
		dsListenerParams.flDopplerFactor=1.5f;
		dsListenerParams.flRolloffFactor=1.5f;
		/*memcpy( &dsListenerParams.vPosition, &pos, sizeof(D3DVECTOR) );
		memcpy( &dsListenerParams.vVelocity, &vel, sizeof(D3DVECTOR) );*/

		//msg_write("  -AllParameters");
		//nw->SafeMessage("Listener");
		pDSListener->SetAllParameters( &dsListenerParams, DS3D_DEFERRED );
		//nw->SafeMessage("/Listener");
		//msg_write("  -Sounds");

		// die Daten der Sounds erstellen
		for (i=0;i<NumSounds;i++){
			if (!SoundUsable(i))
				continue;
			//nw->SafeMessage(i2s(i));
			/*if (!SoundIsPlaying(i))
				continue;*/

			//msg_write(i);

			//nw->SafeMessage("Frequency");
			vector dPos,dVel;
			int f=int(float(SoundFrequency[i])*SoundSpeed[i]*SoundRate[i]);
			sound[i]->m_apDSBuffer[0]->SetFrequency(f);

			VecTransform(dPos,ListenerInvMatrix,SoundPos[i]);
			dVel=SoundVel[i]-ListenerVel;
			VecNormalTransform(dVel,ListenerInvMatrix,dVel);
			memcpy( &dsBufferParams[i].vPosition, &dPos, sizeof(D3DVECTOR) );
			memcpy( &dsBufferParams[i].vVelocity, &dVel, sizeof(D3DVECTOR) );
			dsBufferParams[i].flMinDistance=SoundMinDist[i];
			dsBufferParams[i].flMaxDistance=SoundMaxDist[i];

			//msg_write("    -ap");
			//nw->SafeMessage("SetAllParameters");
			pDS3DBuffer[i]->SetAllParameters( &dsBufferParams[i], DS3D_DEFERRED );
			//pDS3DBuffer[i]->SetAllParameters( &dsBufferParams[i], DS3D_IMMEDIATE );
			//msg_write("    -V");

			float vol=float(pow(SoundVolume[i],0.15f));
			bool should_play=((VecLength(dPos)<SoundMaxDist[i])&&(SoundSpeed[i]<10.0f)&&(SoundSpeed[i]>0.001f));
			//nw->SafeMessage("Volume");
			if (should_play)
				sound[i]->m_apDSBuffer[0]->SetVolume( long(DSBVOLUME_MAX*vol + DSBVOLUME_MIN*(1-vol)) );
			else
				sound[i]->m_apDSBuffer[0]->SetVolume(DSBVOLUME_MIN);
			//msg_write("    -ok");
		}

		// alles ausfuehren
		//msg_write("  -Commit");
		//nw->SafeMessage("Commit");
		pDSListener->CommitDeferredSettings();

		for (i=0;i<NumSounds;i++)
			SoundVel[i]=vector(0,0,0);
		//msg_write("  -ok");
		//nw->SafeMessage("/Listener");
	#endif
}

int NixMusicLoad(char *filename)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
#ifdef NIX_SOUND_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		msg_write(string("loading music: ",filename));
		msg_right();
		int h=open(filename,0);
		close(h);
		if (h<0){
			msg_error("Musik-Datei nicht gefunden");
			msg_left();
			return -1;
		}
		//NumMusics=0;

		/*msg_write("a");
		WCHAR		wFileName[MAX_PATH];
		msg_write("b");
		CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC,IID_IGraphBuilder,reinterpret_cast<void **>(&MusicGraphBuilder[NumSounds]));
		msg_write("c");
		MusicGraphBuilder[NumMusics]->QueryInterface(IID_IMediaControl,reinterpret_cast<void **>(&MusicMediaControl[NumMusics]));
		msg_write("d");
		MusicGraphBuilder[NumMusics]->QueryInterface(IID_IMediaSeeking,reinterpret_cast<void **>(&MusicMediaSeeking[NumMusics]));
		msg_write("e");
		Music[NumMusics]=NULL;
		//msg_write("f");
		MusicPin[NumMusics]=NULL;
		//msg_write("g");
	#ifndef UNICODE
		MultiByteToWideChar(CP_ACP,0,filename,-1,wFileName,MAX_PATH);
	#else
		lstrcpy(wFileName,filename);
	#endif
	//	msg_write("h");
		MusicGraphBuilder[NumMusics]->AddSourceFilter(wFileName,NULL,&Music[NumMusics]);
	//	msg_write("i");
		Music[NumMusics]->FindPin(L"Output",&MusicPin[NumMusics]);*/


		//msg_write("a");
		WCHAR		wFileName[MAX_PATH];
		//msg_write("b");
		if (NumMusics>0){
			MusicMediaControl[0]->Stop();
		//msg_write("b1");
			Music[NumMusics-1]->Release();
		//msg_write("b2");
			MusicPin[NumMusics-1]->Release();
		//msg_write("b3");
			MusicMediaControl[0]->Release();
		//msg_write("b4");
			MusicMediaSeeking[0]->Release();
		//msg_write("b5");
			MusicGraphBuilder[0]->Release();
		//msg_write("b6");
		NumMusics=0;
		}

		// allgemeines... (nur einmal)
		if (NumMusics<1){
			CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC,IID_IGraphBuilder,reinterpret_cast<void **>(&MusicGraphBuilder[0]));
			//msg_write("c");
			MusicGraphBuilder[0]->QueryInterface(IID_IMediaControl,reinterpret_cast<void **>(&MusicMediaControl[0]));
			//msg_write("d");
			MusicGraphBuilder[0]->QueryInterface(IID_IMediaSeeking,reinterpret_cast<void **>(&MusicMediaSeeking[0]));
			//msg_write("e");
		}else{
			MusicMediaControl[0]->Stop();
			IEnumFilters *pFilterEnum = NULL;
			msg_write(DXErrorMsg(MusicGraphBuilder[0]->EnumFilters(&pFilterEnum)));
			// Allocate space, then pull out all of the
			IBaseFilter *pFilter;
			msg_write(DXErrorMsg(pFilterEnum->Reset()));
			unsigned long nf=0;
			msg_write(DXErrorMsg(pFilterEnum->Next(1, &pFilter, &nf)));
			msg_write(nf);
			msg_write(DXErrorMsg(MusicGraphBuilder[0]->RemoveFilter(pFilter)));
		}


	#ifndef UNICODE
		MultiByteToWideChar(CP_ACP,0,filename,-1,wFileName,MAX_PATH);
	#else
		lstrcpy(wFileName,filename);
	#endif
	//	msg_write("h");
		MusicGraphBuilder[0]->AddSourceFilter(wFileName,NULL,&Music[NumMusics]);
	//	msg_write("i");
		Music[NumMusics]->FindPin(L"Output",&MusicPin[NumMusics]);



	//	msg_write("j");
		NumMusics++;
		msg_ok();
		msg_left();
		return NumMusics-1;
	}
#endif
#endif
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		return -1;
	}
#endif
	return -1;
}

#ifdef NIX_OS_WINDOWS
	VOID CALLBACK MyMusicTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
	{
		NixMusicTestRepeat();
	}
#endif

void NixMusicPlay(int index,bool repeat)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return;
		msg_write("NixMusicPlay");
		/*MusicMediaControl[index]->Stop();
		MusicGraphBuilder[index]->Render(MusicPin[index]);
		LONGLONG llPos=0; // ~sec/10.000.000
		MusicMediaSeeking[index]->SetPositions(&llPos,AM_SEEKING_AbsolutePositioning,&llPos,AM_SEEKING_NoPositioning);
		MusicMediaControl[index]->Run();
		MusicRepeat[index]=repeat;*/

		MusicMediaControl[0]->Stop();
		MusicGraphBuilder[0]->Render(MusicPin[index]);
		LONGLONG llPos=0; // ~sec/10.000.000
		MusicMediaSeeking[0]->SetPositions(&llPos,AM_SEEKING_AbsolutePositioning,&llPos,AM_SEEKING_NoPositioning);
		MusicMediaControl[0]->Run();
		MusicRepeat[index]=repeat;

		SetTimer(HuiWindow->hWnd,666,1000,&MyMusicTimerProc);
	}
#endif
#endif
}

void NixMusicSetRate(int index,float rate)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return;
		//MusicMediaSeeking[index]->SetRate((double)rate);
		MusicMediaSeeking[0]->SetRate((double)rate);
	}
#endif
#endif
}

void NixMusicStop(int index)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return;
		msg_write("NixMusicStop");
		//MusicMediaControl[index]->Stop();
		MusicMediaControl[0]->Stop();
		MusicRepeat[index]=false;
	}
#endif
#endif
}

void NixMusicSetPause(int index,bool pause)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return;
		/*if (pause)
			MusicMediaControl[index]->Pause();
		else{
			MusicMediaControl[index]->Stop();
			MusicMediaControl[index]->Run();
		}*/
		if (pause)
			MusicMediaControl[0]->Pause();
		else{
			MusicMediaControl[0]->Stop();
			MusicMediaControl[0]->Run();
		}
	}
#endif
#endif
}

bool NixMusicIsPlaying(int index)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return false;
		FILTER_STATE fs;
		Music[index]->GetState(0,&fs);
		if (fs==State_Running)
			return true;
	}
#endif
#endif
	return false;
}

bool NixMusicEnded(int index)
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (index<0)
			return false;
		LONGLONG Dur,Pos;
		/*MusicMediaSeeking[index]->GetDuration(&Dur);
		MusicMediaSeeking[index]->GetCurrentPosition(&Pos);*/
		MusicMediaSeeking[0]->GetDuration(&Dur);
		MusicMediaSeeking[0]->GetCurrentPosition(&Pos);
		if (Pos>=Dur)
			return true;
	}
#endif
#endif
	return false;
}

void NixMusicTestRepeat()
{
#ifdef NIX_IDE_VCS
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		/*FILTER_STATE fs;
		for (int m=0;m<NumMusics;m++){
			Music[m]->GetState(0,&fs);
			LONGLONG Dur,Pos;
			MusicMediaSeeking[m]->GetDuration(&Dur);
			MusicMediaSeeking[m]->GetCurrentPosition(&Pos);
			if ((Pos>=Dur)&&(MusicRepeat[m]))
				NixMusicPlay(m,true);
		}
		SetTimer(HuiWindow->hWnd,666,1000,&MyMusicTimerProc);*/
	}
#endif
#endif
}



//---------------------------------------------------------------------//
//                          for using the types                        //
//---------------------------------------------------------------------//

// ZXY -> Objekte und Figur-Teile / Modell-Transformationen
// der Vektor nach vorne (0,0,1) wird
// 1. um die z-Achse gedreht (um sich selbst)
// 2. um die x-Achse gedreht (nach oben oder unten "genickt")
// 3. um die y-Achse gedreht (nach links oder rechts)
// (alle Drehungen um Achsen, die nicht veraendert werden!!!)

// YXZ -> Kamera-/Projektions-Transformation
// der Vektor nach vorne (0,0,1) wird
// ... aber in die jeweils andere Richtung (-ang)

// YXZ-Matrix ist die Inverse zur ZXY-Matrix!!!



//###################
// int

// Betrag
int pos_i(int i)
{
	if (i>=0)
		return i;
	else
		return -i;
}

//###################
// float

// Quadrat
float sqr(float f)
{
	return f*f;
}

// Betrag
float pos_f(float f)
{
	if (f>=0)
		return f;
	else
		return -f;
}

// ...
float AngBetweenPi(float w)
{
	int ac=0;
	if (w> pi)	ac=int( (w-pi)/2.0/pi+1.0f);
	if (w<-pi)	ac=int( (w+pi)/2.0/pi-1.0f);
	w-=pi*2*float(ac);
	return w;
}

//----------------------------------------
// vectors

// echte Laenge des Vektors
float VecLength(const vector &v)
{
	#ifdef NIX_IDE_VCS
	#ifdef NIX_TYPES_BY_DIRECTX9
		return D3DXVec3Length((D3DXVECTOR3*)&v);
	#endif
	#endif
	return float(sqrt( v.x*v.x + v.y*v.y + v.z*v.z ));
}

// gibt die laengste Seite zurueck (="unendlich-Norm")
// (immer <= <echte Laenge>)
float VecLengthFuzzy(const vector &v)
{
	float l=pos_f(v.x);
	float a=pos_f(v.y);
	if (a>l)
		l=a;
	a=pos_f(v.z);
	if (a>l)
		l=a;
	return l;
}

float VecLengthSqr(const vector &v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

// v im Quader(a,b) ?
bool VecBetween(vector &v,vector &a,vector &b)
{
	/*if ((v.x>a.x)&&(v.x>b.x))	return false;
	if ((v.x<a.x)&&(v.x<b.x))	return false;
	if ((v.y>a.y)&&(v.y>b.y))	return false;
	if ((v.y<a.y)&&(v.y<b.y))	return false;
	if ((v.z>a.z)&&(v.z>b.z))	return false;
	if ((v.z<a.z)&&(v.z<b.z))	return false;
	return true;*/
	float f=VecDotProduct(v-a,b-a);
	if (f<0)
		return false;
	f/=VecDotProduct(b-a,b-a);
	return (f<=1);
}

// v = a + f*( b - a )
// ermittelt f
float VecFactorBetween(vector &v,vector &a,vector &b)
{
	if (a.x!=b.x)
		return ((v.x-a.x)/(b.x-a.x));
	else if (a.y!=b.y)
		return ((v.y-a.y)/(b.y-a.y));
	else if (a.z!=b.z)
		return ((v.z-a.z)/(b.z-a.z));
	return 0;
}

// a im Wuerfel mit Radius=d um b ?
bool VecBoundingBox(vector &a,vector &b,float d)
{
	if ((a.x-b.x>d)||(a.x-b.x<-d))
		return false;
	if ((a.y-b.y>d)||(a.y-b.y<-d))
		return false;
	if ((a.z-b.z>d)||(a.z-b.z<-d))
		return false;
	return true;
}

// auf die Laenge 1 bringen
void VecNormalize(vector &vo,const vector &vi)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXVec3Normalize((D3DXVECTOR3*)&vo,(D3DXVECTOR3*)&vi);
	#else
		float l=VecLength(vi);
		if (l>0)
			vo=vi/l;
		else
			vo=vector(0,0,0);
	#endif
}

// cos( Winkel zwischen Vektoren) * Laenge1 * Laenge2
float VecDotProduct(const vector &v1,const vector &v2)
{
	/*#ifdef NIX_TYPES_BY_DIRECTX9
		return D3DXVec3Dot((D3DXVECTOR3*)&v1,(D3DXVECTOR3*)&v2);
	#else*/
		return v1.x*v2.x+v1.y*v2.y+v1.z*v2.z;
	//#endif
}

// Richtung: zu beiden orthogonal!!
// Laenge: sin( Winkel zwischen Vektoren) * Laenge1 * Laenge2
// (0,0,0) bei: ( v1 parallel v2 )
vector VecCrossProduct(const vector &v1,const vector &v2)
{
	vector v;
    v.x = v1.y * v2.z - v1.z * v2.y;
    v.y = v1.z * v2.x - v1.x * v2.z;
    v.z = v1.x * v2.y - v1.y * v2.x;
	return v;

}

// Koordinaten-Transformation
// matrix * vector(x,y,z,1)
void VecTransform(vector &vo,const matrix &m,const vector &vi)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXVec3TransformCoord((D3DXVECTOR3*)&vo,(D3DXVECTOR3*)&vi,(D3DXMATRIX*)&m);
	#else
		vector vi_=vi;
		vo.x= vi_.x*m._00 + vi_.y*m._01 + vi_.z*m._02 + m._03;
		vo.y= vi_.x*m._10 + vi_.y*m._11 + vi_.z*m._12 + m._13;
		vo.z= vi_.x*m._20 + vi_.y*m._21 + vi_.z*m._22 + m._23;
	#endif
}

// Transformation eines Normalenvektors
// matrix * vector(x,y,z,0)
void VecNormalTransform(vector &vo,const matrix &m,const vector &vi)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXVec3TransformNormal((D3DXVECTOR3*)&vo,(D3DXVECTOR3*)&vi,(D3DXMATRIX*)&m);
	#else
		vector vi_=vi;
		vo.x= vi_.x*m._00 + vi_.y*m._01 + vi_.z*m._02;
		vo.y= vi_.x*m._10 + vi_.y*m._11 + vi_.z*m._12;
		vo.z= vi_.x*m._20 + vi_.y*m._21 + vi_.z*m._22;
	#endif
}

// vector(0,0,1) wird um ang rotiert
// ZXY, da nur im Spiel-Koordinaten-System
vector VecAng2Dir(const vector &ang)
{
	return vector(		(float)sin(ang.y)*(float)cos(ang.x),
					-	(float)sin(ang.x),
						(float)cos(ang.y)*(float)cos(ang.x));
}

// um welche Winkel wurde vector(0,0,1) rotiert?
vector VecDir2Ang(const vector &dir)
{
	return vector(	-	(float)atan2(dir.y,sqrt(dir.x*dir.x+dir.z*dir.z)),
						(float)atan2(dir.x,dir.z),
						(float)0); // too few information to get z!
}

// um welche Winkel wurde vector(0,0,1) rotiert?
//    
vector VecDir2Ang2(const vector &dir,const vector &up)
{
	vector right=VecCrossProduct(up,dir);
	return vector(	-	(float)atan2(dir.y,sqrt(dir.x*dir.x+dir.z*dir.z)),
						(float)atan2(dir.x,dir.z),
						(float)atan2(right.y,up.y)); // atan2( < up, (0,1,0) >, < right, (0,1,0) > )    where: right = up x dir
/*	// aus den 3 Basis-Vektoren eine Basis-Wechsel-Matrix erzeugen
	matrix m;
	m._00=right.x;	m._01=up.x;	m._02=dir.x;	m._03=0;
	m._10=right.y;	m._11=up.y;	m._12=dir.y;	m._13=0;
	m._20=right.z;	m._21=up.z;	m._22=dir.z;	m._23=0;
	m._30=0;		m._31=0;	m._32=0;		m._33=1;
	quaternion q;
	msg_todo("VecDir2Ang2 fuer OpenGL");
	QuaternionRotationM(q,m);
	msg_db_out(1,"");
	return QuaternionToAngle(q);*/
}

// addiert 2 Winkelangaben
vector VecAngAdd(const vector &ang1,const vector &ang2)
{
	quaternion q,q1,q2;
	QuaternionRotationV(q1,ang1);
	QuaternionRotationV(q2,ang2);
	QuaternionMultiply(q,q2,q1);
	return QuaternionToAngle(q);
}

// kuerzeste Entfernung von p zur Geraden(l1,l2)
float VecLineDistance(const vector &p,const vector &l1,const vector &l2)
{
	return (float)sqrt( VecLengthSqr(p-l1) - sqr(VecDotProduct(l2-l1,p-l1))/VecLengthSqr(l2-l1) );
}

// Punkt der Geraden(l1,l2), der p am naechsten ist
vector VecLineNearestPoint(vector &p,vector &l1,vector &l2)
{
	vector n=l2-l1,np;
	VecNormalize(n,n);
	plane pl;
	PlaneFromPointNormal(pl,p,n);
	PlaneIntersectLine(np,pl,l1,l2);
	return np;
}

void VecMin(vector &v,vector &test_partner)
{
	if (test_partner.x<v.x)	v.x=test_partner.x;
	if (test_partner.y<v.y)	v.y=test_partner.y;
	if (test_partner.z<v.z)	v.z=test_partner.z;
}

void VecMax(vector &v,vector &test_partner)
{
	if (test_partner.x>v.x)	v.x=test_partner.x;
	if (test_partner.y>v.y)	v.y=test_partner.y;
	if (test_partner.z>v.z)	v.z=test_partner.z;
}

//----------------------------------------
// matrices

#define _ps(a,b,i,j)	(a._e(i,0)*b._e(0,j) + a._e(i,1)*b._e(1,j) + a._e(i,2)*b._e(2,j) + a._e(i,3)*b._e(3,j))

// combining two transformation matrices (first do m1, then m2:   m = m2 * m1 )
void MatrixMultiply(matrix &m,matrix &m2,matrix &m1)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixMultiply((D3DXMATRIX*)&m,(D3DXMATRIX*)&m1,(D3DXMATRIX*)&m2);
	#else
		// m_ij = (sum k) m2_ik * m1_kj
		matrix _m;
		_m._00=_ps(m2,m1,0,0);	_m._01=_ps(m2,m1,0,1);	_m._02=_ps(m2,m1,0,2);	_m._03=_ps(m2,m1,0,3);
		_m._10=_ps(m2,m1,1,0);	_m._11=_ps(m2,m1,1,1);	_m._12=_ps(m2,m1,1,2);	_m._13=_ps(m2,m1,1,3);
		_m._20=_ps(m2,m1,2,0);	_m._21=_ps(m2,m1,2,1);	_m._22=_ps(m2,m1,2,2);	_m._23=_ps(m2,m1,2,3);
		_m._30=_ps(m2,m1,3,0);	_m._31=_ps(m2,m1,3,1);	_m._32=_ps(m2,m1,3,2);	_m._33=_ps(m2,m1,3,3);
		m=_m;
		/*#ifdef NIX_API_OPENGL
			glMatrixMode(GL_MODELVIEW);	
			glLoadMatrixf((float*)&m2);
			glMultMatrixf((float*)&m1);
			glGetFloatv(GL_MODELVIEW_MATRIX,(float*)&m);
		#endif*/
	#endif
}

static const float _IdentityMatrix[16]={	1,0,0,0,	0,1,0,0,	0,0,1,0,	0,0,0,1	};

// identity (no transformation: m*v=v)
void MatrixIdentity(matrix &m)
{
	memcpy(&m,&_IdentityMatrix,sizeof(matrix));
	/*#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixIdentity((D3DXMATRIX*)&m);
	#else
		m._00=1;	m._01=0;	m._02=0;	m._03=0;
		m._10=0;	m._11=1;	m._12=0;	m._13=0;
		m._20=0;	m._21=0;	m._22=1;	m._23=0;
		m._30=0;	m._31=0;	m._32=0;	m._33=1;
	#endif*/
}

float _Determinant(float m[16])
{
	return
		m[12]*m[9]*m[6]*m[3]-
		m[8]*m[13]*m[6]*m[3]-
		m[12]*m[5]*m[10]*m[3]+
		m[4]*m[13]*m[10]*m[3]+
		m[8]*m[5]*m[14]*m[3]-
		m[4]*m[9]*m[14]*m[3]-
		m[12]*m[9]*m[2]*m[7]+
		m[8]*m[13]*m[2]*m[7]+
		m[12]*m[1]*m[10]*m[7]-
		m[0]*m[13]*m[10]*m[7]-
		m[8]*m[1]*m[14]*m[7]+
		m[0]*m[9]*m[14]*m[7]+
		m[12]*m[5]*m[2]*m[11]-
		m[4]*m[13]*m[2]*m[11]-
		m[12]*m[1]*m[6]*m[11]+
		m[0]*m[13]*m[6]*m[11]+
		m[4]*m[1]*m[14]*m[11]-
		m[0]*m[5]*m[14]*m[11]-
		m[8]*m[5]*m[2]*m[15]+
		m[4]*m[9]*m[2]*m[15]+
		m[8]*m[1]*m[6]*m[15]-
		m[0]*m[9]*m[6]*m[15]-
		m[4]*m[1]*m[10]*m[15]+
		m[0]*m[5]*m[10]*m[15];
}

// inverting the transformation
void MatrixInverse(matrix &mo,matrix &mi)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixInverse((D3DXMATRIX*)&mo,NULL,(D3DXMATRIX*)&mi);
	#else
		float *m=(float*)&mi;
		float *i=(float*)&mo;
		float x=_Determinant(m);

		/*msg_write("Matrix Inverse");
		mout(mi);
		msg_write(f2s(x,3));*/

		if (x==0){
			msg_write("MatrixInverse:  matrix not invertible");
			return;
		}

		i[0]= (-m[13]*m[10]*m[7] +m[9]*m[14]*m[7] +m[13]*m[6]*m[11] -m[5]*m[14]*m[11] -m[9]*m[6]*m[15] +m[5]*m[10]*m[15])/x;
		i[4]= ( m[12]*m[10]*m[7] -m[8]*m[14]*m[7] -m[12]*m[6]*m[11] +m[4]*m[14]*m[11] +m[8]*m[6]*m[15] -m[4]*m[10]*m[15])/x;
		i[8]= (-m[12]*m[9]* m[7] +m[8]*m[13]*m[7] +m[12]*m[5]*m[11] -m[4]*m[13]*m[11] -m[8]*m[5]*m[15] +m[4]*m[9]* m[15])/x;
		i[12]=( m[12]*m[9]* m[6] -m[8]*m[13]*m[6] -m[12]*m[5]*m[10] +m[4]*m[13]*m[10] +m[8]*m[5]*m[14] -m[4]*m[9]* m[14])/x;
		i[1]= ( m[13]*m[10]*m[3] -m[9]*m[14]*m[3] -m[13]*m[2]*m[11] +m[1]*m[14]*m[11] +m[9]*m[2]*m[15] -m[1]*m[10]*m[15])/x;
		i[5]= (-m[12]*m[10]*m[3] +m[8]*m[14]*m[3] +m[12]*m[2]*m[11] -m[0]*m[14]*m[11] -m[8]*m[2]*m[15] +m[0]*m[10]*m[15])/x;
		i[9]= ( m[12]*m[9]* m[3] -m[8]*m[13]*m[3] -m[12]*m[1]*m[11] +m[0]*m[13]*m[11] +m[8]*m[1]*m[15] -m[0]*m[9]* m[15])/x;
		i[13]=(-m[12]*m[9]* m[2] +m[8]*m[13]*m[2] +m[12]*m[1]*m[10] -m[0]*m[13]*m[10] -m[8]*m[1]*m[14] +m[0]*m[9]* m[14])/x;
		i[2]= (-m[13]*m[6]* m[3] +m[5]*m[14]*m[3] +m[13]*m[2]*m[7]  -m[1]*m[14]*m[7] -m[5]*m[2]*m[15] +m[1]*m[6]* m[15])/x;
		i[6]= ( m[12]*m[6]* m[3] -m[4]*m[14]*m[3] -m[12]*m[2]*m[7]  +m[0]*m[14]*m[7] +m[4]*m[2]*m[15] -m[0]*m[6]* m[15])/x;
		i[10]=(-m[12]*m[5]* m[3] +m[4]*m[13]*m[3] +m[12]*m[1]*m[7]  -m[0]*m[13]*m[7] -m[4]*m[1]*m[15] +m[0]*m[5]* m[15])/x;
		i[14]=( m[12]*m[5]* m[2] -m[4]*m[13]*m[2] -m[12]*m[1]*m[6]  +m[0]*m[13]*m[6] +m[4]*m[1]*m[14] -m[0]*m[5]* m[14])/x;
		i[3]= ( m[9]* m[6]* m[3] -m[5]*m[10]*m[3] -m[9]* m[2]*m[7]  +m[1]*m[10]*m[7] +m[5]*m[2]*m[11] -m[1]*m[6]* m[11])/x;
		i[7]= (-m[8]* m[6]* m[3] +m[4]*m[10]*m[3] +m[8]* m[2]*m[7]  -m[0]*m[10]*m[7] -m[4]*m[2]*m[11] +m[0]*m[6]* m[11])/x;
		i[11]=( m[8]* m[5]* m[3] -m[4]*m[9]* m[3] -m[8]* m[1]*m[7]  +m[0]*m[9]* m[7] +m[4]*m[1]*m[11] -m[0]*m[5]* m[11])/x;
		i[15]=(-m[8]* m[5]* m[2] +m[4]*m[9]* m[2] +m[8]* m[1]*m[6]  -m[0]*m[9]* m[6] -m[4]*m[1]*m[10] +m[0]*m[5]* m[10])/x;


		//mout(mo);

		//msg_todo("MatrixInverse for OpenGL");
	#endif
}

// transposes a matrix
void MatrixTranspose(matrix &mo,matrix &mi)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixTranspose((D3DXMATRIX*)&mo,(D3DXMATRIX*)&mi);
	#else
		matrix _m;
		_m._00=mi._00;	_m._01=mi._10;	_m._02=mi._20;	_m._03=mi._30;
		_m._10=mi._01;	_m._11=mi._11;	_m._12=mi._21;	_m._13=mi._31;
		_m._20=mi._02;	_m._21=mi._12;	_m._22=mi._22;	_m._23=mi._32;
		_m._30=mi._03;	_m._31=mi._13;	_m._32=mi._23;	_m._33=mi._33;
		mo=_m;
	#endif
}

// translation by a vector (m*v=v+t)
void MatrixTranslation(matrix &m,const vector &t)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixTranslation((D3DXMATRIX*)&m,t.x,t.y,t.z);
	#else
		m._00=1;	m._01=0;	m._02=0;	m._03=t.x;
		m._10=0;	m._11=1;	m._12=0;	m._13=t.y;
		m._20=0;	m._21=0;	m._22=1;	m._23=t.z;
		m._30=0;	m._31=0;	m._32=0;	m._33=1;
	#endif
}

// Rotation um die X-Achse (nach unten)
void MatrixRotationX(matrix &m,float w)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixRotationX((D3DXMATRIX*)&m,w);
	#else
		float sw=(float)sin(w);
		float cw=(float)cos(w);
		m._00=1;	m._01=0;	m._02=0;	m._03=0;
		m._10=0;	m._11=cw;	m._12=-sw;	m._13=0;
		m._20=0;	m._21=sw;	m._22=cw;	m._23=0;
		m._30=0;	m._31=0;	m._32=0;	m._33=1;
		/*#ifdef NIX_API_OPENGL
			glMatrixMode(GL_MODELVIEW);	
			glLoadIdentity();
			glRotatef(w*180.0f/pi,1,0,0);
			glGetFloatv(GL_MODELVIEW_MATRIX,(float*)&m);
		#endif*/
	#endif
}

// Rotation um die Y-Achse (nach rechts)
void MatrixRotationY(matrix &m,float w)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixRotationY((D3DXMATRIX*)&m,w);
	#else
		float sw=(float)sin(w);
		float cw=(float)cos(w);
		m._00=cw;	m._01=0;	m._02=sw;	m._03=0;
		m._10=0;	m._11=1;	m._12=0;	m._13=0;
		m._20=-sw;	m._21=0;	m._22=cw;	m._23=0;
		m._30=0;	m._31=0;	m._32=0;	m._33=1;
		/*#ifdef NIX_API_OPENGL
			glMatrixMode(GL_MODELVIEW);	
			glLoadIdentity();
			glRotatef(w*180.0f/pi,0,1,0);
			glGetFloatv(GL_MODELVIEW_MATRIX,(float*)&m);
		#endif*/
	#endif
}

// Rotation um die Z-Achse (gegen den Uhrzeigersinn)
void MatrixRotationZ(matrix &m,float w)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixRotationZ((D3DXMATRIX*)&m,w);
	#else
		float sw=(float)sin(w);
		float cw=(float)cos(w);
		m._00=cw;	m._01=-sw;	m._02=0;	m._03=0;
		m._10=sw;	m._11=cw;	m._12=0;	m._13=0;
		m._20=0;	m._21=0;	m._22=1;	m._23=0;
		m._30=0;	m._31=0;	m._32=0;	m._33=1;
		/*#ifdef NIX_API_OPENGL
			glMatrixMode(GL_MODELVIEW);	
			glLoadIdentity();
			glRotatef(w*180.0f/pi,0,0,1);
			glGetFloatv(GL_MODELVIEW_MATRIX,(float*)&m);
		#endif*/
	#endif
}

// ZXY -> fuer alles IM Spiel
void MatrixRotation(matrix &m,const vector &ang)
{
	/*matrix x,y,z;
	MatrixRotationX(x,ang.x);
	MatrixRotationY(y,ang.y);
	MatrixRotationZ(z,ang.z);
	// m=y*x*z
	MatrixMultiply(m,y,x);
	MatrixMultiply(m,m,z);*/
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixRotationYawPitchRoll((D3DXMATRIX*)&m,ang.y,ang.x,ang.z);
	#else
		/*#ifdef NIX_API_OPENGL
			glMatrixMode(GL_MODELVIEW);	
			glLoadIdentity();
			glRotatef(ang.z*180.0f/pi,0,0,1);
			glRotatef(ang.x*180.0f/pi,1,0,0);
			glRotatef(ang.y*180.0f/pi,0,1,0);
			glGetFloatv(GL_MODELVIEW_MATRIX,(float*)&m);
		#endif*/
		float sx=(float)sin(ang.x);
		float cx=(float)cos(ang.x);
		float sy=(float)sin(ang.y);
		float cy=(float)cos(ang.y);
		float sz=(float)sin(ang.z);
		float cz=(float)cos(ang.z);
		m._00= sx*sy*sz + cy*cz;	m._01= sx*sy*cz - cy*sz;	m._02= cx*sy;	m._03=0;
		m._10= cx*sz;				m._11= cx*cz;				m._12=-sx;		m._13=0;
		m._20= sx*cy*sz - sy*cz;	m._21= sx*cy*cz + sy*sz;	m._22= cx*cy;	m._23=0;
		m._30= 0;					m._31= 0;					m._32=0;		m._33=1;
	#endif
}

// YXZ -> fuer Kamera-Rotationen
// ist die Inverse zu MatrixRotation!!
void MatrixRotationView(matrix &m,const vector &ang)
{
	/*matrix x,y,z;
	MatrixRotationX(x,-ang.x);
	MatrixRotationY(y,-ang.y);
	MatrixRotationZ(z,-ang.z);
	// z*x*y
	MatrixMultiply(m,z,x);
	MatrixMultiply(m,m,y);*/
	float sx=(float)sin(ang.x);
	float cx=(float)cos(ang.x);
	float sy=(float)sin(ang.y);
	float cy=(float)cos(ang.y);
	float sz=(float)sin(ang.z);
	float cz=(float)cos(ang.z);
	// the transposed (=inverted) of MatrixView
	m._00= sx*sy*sz + cy*cz;	m._01= cx*sz;	m._02= sx*cy*sz - sy*cz;	m._03=0;
	m._10= sx*sy*cz - cy*sz;	m._11= cx*cz;	m._12= sx*cy*cz + sy*sz;	m._13=0;
	m._20= cx*sy;				m._21=-sx;		m._22= cx*cy;				m._23=0;
	m._30= 0;					m._31= 0;		m._32=0;					m._33=1;
	/*#ifdef NIX_TYPES_BY_DIRECTX9
		matrix x,y,z;
		MatrixRotationX(x,-ang.x);
		MatrixRotationY(y,-ang.y);
		MatrixRotationZ(z,-ang.z);
		MatrixMultiply(m,z,x);
		MatrixMultiply(m,m,y);
	#else
		glMatrixMode(GL_MODELVIEW);	
		glLoadIdentity();
		glRotatef(-ang.y*180.0f/pi,0,1,0);
		glRotatef(-ang.x*180.0f/pi,1,0,0);
		glRotatef(-ang.z*180.0f/pi,0,0,1);
		glGetFloatv(GL_MODELVIEW_MATRIX,(float*)&m);
	#endif*/
}

// Rotations-Matrix aus Quaternion erzeugen
void MatrixRotationQ(matrix &m,const quaternion &q)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixRotationQuaternion((D3DXMATRIX*)&m,(D3DXQUATERNION*)&q);
	#else
		m._00=1-2*q.y*q.y-2*q.z*q.z;	m._01=  2*q.x*q.y-2*q.w*q.z;	m._02=  2*q.x*q.z+2*q.w*q.y;	m._03=0;
		m._10=  2*q.x*q.y+2*q.w*q.z;	m._11=1-2*q.x*q.x-2*q.z*q.z;	m._12=  2*q.y*q.z-2*q.w*q.x;	m._13=0;
		m._20=  2*q.x*q.z-2*q.w*q.y;	m._21=  2*q.y*q.z+2*q.w*q.x;	m._22=1-2*q.x*q.x-2*q.y*q.y;	m._23=0;
		m._30=0;						m._31=0;						m._32=0;						m._33=1;
	/* [ 1 - 2y2 - 2z2        2xy - 2wz        2xz + 2wy
	         2xy + 2wz    1 - 2x2 - 2z2        2yz - 2wx
		     2xz - 2wy        2yz + 2wx    1 - 2x2 - 2y2 ] */
	#endif
}

// scale orthogonally in 3 dimensions
void MatrixScale(matrix &m,float fx,float fy,float fz)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixScaling((D3DXMATRIX*)&m,fx,fy,fz);
	#else
		m._00=fx;	m._01=0;	m._02=0;	m._03=0;
		m._10=0;	m._11=fy;	m._12=0;	m._13=0;
		m._20=0;	m._21=0;	m._22=fz;	m._23=0;
		m._30=0;	m._31=0;	m._32=0;	m._33=1;
	#endif
}

// create a transformation that reflects at a <plane pl>
void MatrixReflect(matrix &m,plane &pl)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXMatrixReflect((D3DXMATRIX*)&m,(D3DXPLANE*)&pl);
	#else
		vector n=vector(pl.a,pl.b,pl.c);
		vector p=-n*pl.d;
		// mirror: matrix s from transforming the basis vectors:
		//    e_i' = e_i - 2 < n, e_i >
		//     or thinking of it as a tensor product (projection): s = id - 2n(x)n
		// translation by p: t_p
		// complete reflection is: r = t_p * s * t_(-p) = t_(2p) * s
		m._00=1-2*n.x*n.x;	m._01= -2*n.y*n.x;	m._02= -2*n.z*n.x;	m._03=2*p.x;
		m._10= -2*n.x*n.y;	m._11=1-2*n.y*n.y;	m._12= -2*n.z*n.y;	m._13=2*p.y;
		m._20= -2*n.x*n.z;	m._21= -2*n.y*n.z;	m._22=1-2*n.z*n.z;	m._23=2*p.z;
		m._30=0;			m._31=0;			m._32=0;			m._33=1;
		msg_todo("TestMe: MatrixReflect for OpenGL");
	#endif
}

//----------------------------------------
// quaternions

void QuaternionIdentity(quaternion &q)
{
	q.w=1;
	q.x=q.y=q.z=0;
}

// rotation with an <angle w> and an <axis axis>
void QuaternionRotationA(quaternion &q,const vector &axis,float w)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQuaternionRotationAxis((D3DXQUATERNION*)&q,(D3DXVECTOR3*)&axis,w);
	#else
		float w_half=w*0.5f;
		float s=(float)sin(w_half);
		q.w=(float)cos(w_half);
		q.x=axis.x*s;
		q.y=axis.y*s;
		q.z=axis.z*s;
		//msg_todo("TestMe: QuaternionRotation(a,w) for OpenGL");
	#endif
}

// ZXY -> everything IN the game (world transformation)
void QuaternionRotationV(quaternion &q,const vector &ang)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQuaternionRotationYawPitchRoll((D3DXQUATERNION*)&q,ang.y,ang.x,ang.z);
	#else
		float wx_2=ang.x*0.5f;
		float wy_2=ang.y*0.5f;
		float wz_2=ang.z*0.5f;
		float cx=(float)cos(wx_2);
		float cy=(float)cos(wy_2);
		float cz=(float)cos(wz_2);
		float sx=(float)sin(wx_2);
		float sy=(float)sin(wy_2);
		float sz=(float)sin(wz_2);
		q.w=(cy*cx*cz) + (sy*sx*sz);
		q.x=(cy*sx*cz) + (sy*cx*sz);
		q.y=(sy*cx*cz) - (cy*sx*sz);
		q.z=(cy*cx*sz) - (sy*sx*cz);
		/*quaternion x,y,z;
		QuaternionRotationA(x,vector(1,0,0),ang.x);
		QuaternionRotationA(y,vector(0,1,0),ang.y);
		QuaternionRotationA(z,vector(0,0,1),ang.z);
		// y*x*z
		QuaternionMultiply(q,x,z);
		QuaternionMultiply(q,y,q);*/
		//msg_todo("TestMe: QuaternionRotation(w) for OpenGL");
	#endif
}

// create a quaternion from a (rotation-) matrix
void QuaternionRotationM(quaternion &q,matrix &m)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQuaternionRotationMatrix((D3DXQUATERNION*)&q,(D3DXMATRIX*)&m);
	#else
		float tr=m._00+m._11+m._22;
		float w=(float)acos((tr-1)/2);
		if ((w<0.00000001f)&&(w>-0.00000001f))
			QuaternionIdentity(q);
		else{
			float s=0.5f/(float)sin(w);
			vector n;
			n.x=(m._21-m._12)*s;
			n.y=(m._02-m._20)*s;
			n.z=(m._10-m._01)*s;
			VecNormalize(n,n);
			QuaternionRotationA(q,n,w);
		}
		//msg_todo("TestMe: QuaternionRotation(m) for OpenGL");
	#endif
}

// invert a quaternion rotation
void QuaternionInverse(quaternion &qo,quaternion &qi)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQuaternionInverse((D3DXQUATERNION*)&qo,(D3DXQUATERNION*)&qi);
	#else
		float l=(qi.x*qi.x)+(qi.y*qi.y)+(qi.z*qi.z)+(qi.w*qi.w);
		l=1.0f/l;
		qo.w= qi.w*l;
		qo.x=-qi.x*l;
		qo.y=-qi.y*l;
		qo.z=-qi.z*l;
		//msg_todo("TestMe: QuaternionInverse for OpenGL");
	#endif
}

// unite 2 rotations (first rotate by q1, then by q2: q = q2*q1)
void QuaternionMultiply(quaternion &q,quaternion &q2,quaternion &q1)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQuaternionMultiply((D3DXQUATERNION*)&q,(D3DXQUATERNION*)&q1,(D3DXQUATERNION*)&q2);
	#else
		quaternion _q;
		_q.w = q2.w*q1.w - q2.x*q1.x - q2.y*q1.y - q2.z*q1.z;
		_q.x = q2.w*q1.x + q2.x*q1.w + q2.y*q1.z - q2.z*q1.y;
		_q.y = q2.w*q1.y + q2.y*q1.w + q2.z*q1.x - q2.x*q1.z;
		_q.z = q2.w*q1.z + q2.z*q1.w + q2.x*q1.y - q2.y*q1.x;
		q=_q;
		//QuaternionNormalize(q,q);
		//msg_todo("TestMe: QuaternionMultiply for OpenGL");
	#endif
}

// q = q1 + t*( q2 - q1)
void QuaternionInterpolate(quaternion &q,quaternion &q1,quaternion &q2,float t)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQuaternionSlerp((D3DXQUATERNION*)&q,(D3DXQUATERNION*)&q1,(D3DXQUATERNION*)&q2,t);
	#else
		msg_todo("TestMe: QuaternionInterpolate(2q) for OpenGL");
		q=q1;

		t=1-t; // ....?

		// dot product = cos angle(q1,q2)
		float c = q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
		float t2;
		bool flip=false;
		// flip, if q1 and q2 on opposite hemispheres
		if (c<0){
			c=-c;
			flip=true;
		}
		// q1 and q2 "too equal"?
		if (c>0.9999f)
			t2=1.0f-t;
		else{
			float theta=(float)acos(c);
			float phi=theta;//+spin*pi; // spin for additional circulations...
			float s=(float)sin(theta);
			t2=(float)sin(theta-t*phi)/s;
			t=(float)sin(t*phi)/s;
		}
		if (flip)
			t=-t;

		q.x = t*q1.x + t2*q2.x;
		q.y = t*q1.y + t2*q2.y;
		q.z = t*q1.z + t2*q2.z;
		q.w = t*q1.w + t2*q2.w;
	#endif
}

void QuaternionInterpolate(quaternion &q,quaternion &q1,quaternion &q2,quaternion &q3,quaternion &q4,float t)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQUATERNION A,B,C;
		D3DXQuaternionSquadSetup(&A,&B,&C,(D3DXQUATERNION*)&q1,(D3DXQUATERNION*)&q2,(D3DXQUATERNION*)&q3,(D3DXQUATERNION*)&q4);
		D3DXQuaternionSquad((D3DXQUATERNION*)&q,(D3DXQUATERNION*)&q2,&A,&B,&C,t);
	#else
		q=q2;
		msg_todo("QuaternionInterpolate(4q) for OpenGL");
	#endif
}

// convert a quaternion into 3 angles (ZXY)
vector QuaternionToAngle(quaternion &q)
{
	vector ang,v;
	v=vector(0,0,1000.0f);
	matrix m,x,y;
	MatrixRotationQ(m,q);
	VecTransform(v,m,v);
	ang.y= float(atan2(v.x,v.z));
	ang.x=-float(atan2(v.y,sqrt(v.x*v.x+v.z*v.z)));
	MatrixRotationX(x,-ang.x);
	MatrixRotationY(y,-ang.y);
	MatrixMultiply(m,y,m);
	MatrixMultiply(m,x,m);
	v=vector(1000.0f,0,0);
	VecTransform(v,m,v);
	ang.z=float(atan2(v.y,v.x));
	return ang;
}

// scale the angle of the rotation
void QuaternionScale(quaternion &q,float f)
{
	float w=GetAngle(q);
	if (w==0)	return;

	q.w=(float)cos(w*f/2);
	float factor=(float)sin(w*f/2)/(float)sin(w/2);
	q.x=q.x*factor;
	q.y=q.y*factor;
	q.z=q.z*factor;
}

// quaternion correction
void QuaternionNormalize(quaternion &qo,quaternion &qi)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQuaternionNormalize((D3DXQUATERNION*)&qo,(D3DXQUATERNION*)&qi);
	#else
		float l=(float)sqrt((qi.x*qi.x)+(qi.y*qi.y)+(qi.z*qi.z)+(qi.w*qi.w));
		l=1.0f/l;
		qo.x=qi.x*l;
		qo.y=qi.y*l;
		qo.z=qi.z*l;
		qo.w=qi.w*l;
	#endif
}

// the axis of our quaternion rotation
vector GetAxis(quaternion &q)
{
	vector ax=vector(q.x,q.y,q.z);
	VecNormalize(ax,ax);
	return ax;
}

// angle value of the quaternion
float GetAngle(quaternion &q)
{
	return (float)acos(q.w)*2;
}

//----------------------------------------
// planes

// Ebene, in der a, b und c liegen
void PlaneFromPoints(plane &pl,vector &a,vector &b,vector &c)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXPlaneFromPoints((D3DXPLANE*)&pl,(D3DXVECTOR3*)&a,(D3DXVECTOR3*)&b,(D3DXVECTOR3*)&c);
	#else
		vector ba=b-a,ca=c-a,n;
		n=VecCrossProduct(ba,ca);
		VecNormalize(n,n);
		pl.a=n.x;	pl.b=n.y;	pl.c=n.z;
		pl.d=-(n.x*a.x+n.y*a.y+n.z*a.z);
	#endif
}

// Ebene liegt auf p mit <Normalenvektor n>
void PlaneFromPointNormal(plane &pl,vector &p,vector &n)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXPlaneFromPointNormal((D3DXPLANE*)&pl,(D3DXVECTOR3*)&p,(D3DXVECTOR3*)&n);
	#else
		pl.a=n.x;	pl.b=n.y;	pl.c=n.z;
		pl.d=-(n.x*p.x+n.y*p.y+n.z*p.z);
	#endif
}

// Ebene mit einer Matrix transformieren
void PlaneTransform(plane &plo,matrix &m,plane &pli)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXPlaneTransform((D3DXPLANE*)&plo,(D3DXPLANE*)&pli,(D3DXMATRIX*)&m);
	#else
		// get the normal vector and transform it
		vector n=vector(pli.a,pli.b,pli.c);
	//	VecNormalize(n,n);
		VecNormalTransform(n,m,n);
		// get a pount within the plane and transform it
		vector p=-n*pli.d;
		// create a new plane from the transformed data
		PlaneFromPointNormal(plo,p,n);
		msg_todo("TestMe: PlaneTransform for OpenGL");
	#endif
}

// in welche Richtung zeigt die Ebene?
vector GetNormal(plane &pl)
{
	vector n=vector(pl.a,pl.b,pl.c);
//	VecNormalize(n,n);
	return n;
}

// Schnittpunkt von Ebene pl und Gerade(l1,l2)
// (false, wenn parallel!)
bool PlaneIntersectLine(vector &i,plane &pl,vector &l1,vector &l2)
{
	#ifdef NIX_TYPES_BY_DIRECTX9
		if (D3DXPlaneIntersectLine((D3DXVECTOR3*)&i,(D3DXPLANE*)&pl,(D3DXVECTOR3*)&l1,(D3DXVECTOR3*)&l2))
			return true;
	#else
		//msg_todo("TestMe: PlaneIntersectLine for OpenGL");
		vector n=vector(pl.a,pl.b,pl.c);
	//	VecNormalize(n,n);
		float d=-pl.d;
		float e=VecDotProduct(n,l1);
		float f=VecDotProduct(n,l2);
		if (e==f) // parallel?
			return false;
		float t=(d-f)/(e-f);
		//if ((t>=0)&&(t<=1)){
			//i = l1 + t*(l2-l1);
			i = l2 + t*(l1-l2);
			return true;
		//}
	#endif
	return false;
}

// Ebene an sich selbst spiegeln
void PlaneInverse(plane &pl)
{
	pl.a=-pl.a;
	pl.b=-pl.b;
	pl.c=-pl.c;
}

// in welcher Koordinaten-Ebene hat v die groesste Auslenkung?
int ImportantPlane(vector &v)
{
	v.x=pos_f(v.x);
	v.y=pos_f(v.y);
	v.z=pos_f(v.z);
	if ((v.x<=v.y)&&(v.x<=v.z))
		return 1;	// Y-Z-Ebene
	if (v.y<=v.z)
		return 2;	// X-Z-Ebene
	return 3;		// X-Y-Ebene
}

// P = A + f*( B - A ) + g*( C - A )
void GetBaryCentric(vector &P,vector &A,vector &B,vector &C,float &f,float &g)
{
	// Bezugs-System: A
	vector ba=B-A,ca=C-A,dir;
	plane pl;
	PlaneFromPoints(pl,A,B,C); // Ebene des Dreiecks
	dir=GetNormal(pl);//vector(pl.a,pl.b,pl.c); // Normalen-Vektor
	vector pvec;
	pvec=VecCrossProduct(dir,ca); // Laenge: |ca|         Richtung: Dreiecks-Ebene, orth zu ca
	float det=VecDotProduct(ba,pvec); // = |ba| * |ca| * cos( pvec->ba )   -> =Flaeche des Parallelogramms
	vector pa;
	if (det>0)
		pa=P-A;
	else
	{
		pa=A-P;
		det=-det;
	}
	f=VecDotProduct(pa,pvec);
	vector qvec;
	qvec=VecCrossProduct(pa,ba);
	g=VecDotProduct(dir,qvec);
	//float t=VecDotProduct(ca,qvec);
	float InvDet=1.0f/det;
	//t*=InvDet;
	f*=InvDet;
	g*=InvDet;
}

// wird das Dreieck(t1,t2,t3) von der Geraden(l1,l2) geschnitten?
// Schnittpunkt = col
bool LineIntersectsTriangle(vector &t1,vector &t2,vector &t3,vector &l1,vector &l2,vector &col,bool vm)
{
	plane p;
	PlaneFromPoints(p,t1,t2,t3);
	if (!PlaneIntersectLine(col,p,l1,l2))
		return false;
	GetBaryCentric(col,t1,t2,t3,LineIntersectsTriangleF,LineIntersectsTriangleG);
	if ((LineIntersectsTriangleF>0)&&(LineIntersectsTriangleG>0)&&(LineIntersectsTriangleF+LineIntersectsTriangleG<1))
		return true;
	return false;
}

// distance <point p> to <plane pl>
float PlaneDistance(plane &pl,vector &p)
{
	return pl.a*p.x + pl.b*p.y + pl.c*p.z + pl.d;
}

//###################
// Farbe

// create a color from (alpha, red, green blue)
// (no limitations)
color SetColor(float a,float r,float g, float b)
{
	color c;
	c.a=a;	c.r=r;	c.g=g;	c.b=b;
	return c;
}

// create a color from (alpha, red, green blue)
// (values of set [0..1])
color SetColorSave(float a,float r,float g, float b)
{
	if (a<0)	a=0;	if (a>1)	a=1;
	if (r<0)	r=0;	if (r>1)	r=1;
	if (g<0)	g=0;	if (g>1)	g=1;
	if (b<0)	b=0;	if (b>1)	b=1;
	color c;
	c.a=a;	c.r=r;	c.g=g;	c.b=b;
	return c;
}

color SetColorHSB(float a,float hue,float saturation,float brightness)
{
	int h=int(hue*6)%6;
	float f=hue*6.0f-h;
	float p=brightness*(1-saturation);
	float q=brightness*(1-saturation*f);
	float t=brightness*(1-saturation*(1-f));
	color c;
	if (h==0)	c=SetColor(a,brightness,t,p);
	if (h==1)	c=SetColor(a,q,brightness,p);
	if (h==2)	c=SetColor(a,p,brightness,t);
	if (h==3)	c=SetColor(a,p,q,brightness);
	if (h==4)	c=SetColor(a,t,p,brightness);
	if (h==5)	c=SetColor(a,brightness,p,q);
	return c;
}

// scale the elements of a color
color ColorScale(color &c,float f)
{
	return SetColor(c.a*f,c.r*f,c.g*f,c.b*f);
}

// create a mixed color = a * (1-t)  +  b * t
color ColorInterpolate(color &a,color &b,float t)
{
	return SetColor(	a.a*(1-t)+b.a*t,
						a.r*(1-t)+b.r*t,
						a.g*(1-t)+b.g*t,
						a.b*(1-t)+b.b*t	);
}

color ColorMultiply(color &a,color &b)
{
	return SetColor(	a.a*b.a,
						a.r*b.r,
						a.g*b.g,
						a.b*b.b	);
}




//####################################################################
//                  Netzwerk-Relevantes
//####################################################################
#ifdef NIX_OS_WINDOWS
	#include <winsock.h>
	#pragma comment(lib,"wsock32.lib")

	static WSADATA wsaData;
#endif
#ifdef NIX_OS_LINUX
	#include <stdio.h>
	#include <string.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>

	#include <netdb.h>

#endif


#define NIXNET_MAX_BUFFER		(1048576*4)
#define NIXNET_MAX_SEND			4096

char Buffer[NIXNET_MAX_BUFFER];
int BufferUsed;


#define NIXNET_DEBUG		10


void so(int dbg,char *str)
{
	if (dbg<=NIXNET_DEBUG)
		msg_write(str);
}

void so(int dbg,int i)
{
	if (dbg<=NIXNET_DEBUG)
		msg_write(i);
}



void NixNetInit()
{
	if (!msg_inited)
		msg_init();
#ifdef NIX_OS_WINDOWS
	if (WSAStartup(MAKEWORD(1,1),&wsaData)!=0){
		msg_error("WSAStartup  (Network....)");
	}
#endif
}




void NixNetCloseSocket(int &s)
{
	//so("close");
#ifdef NIX_OS_WINDOWS
	closesocket(s);
#endif
#ifdef NIX_OS_LINUX
	close(s);
#endif
	s=-1;
}

void NixNetSetBlocking(int s,bool blocking)
{
#ifdef NIX_OS_WINDOWS
	unsigned long l=blocking?0:1;
	ioctlsocket(s,FIONBIO,&l);
#endif
#ifdef NIX_OS_LINUX
	fcntl(s,F_SETFL,blocking?0:O_NONBLOCK);
#endif
}

int NixNetCreateSocket_()
{
	so(1,"socket...");
	int s=socket(AF_INET, SOCK_STREAM, 0);
	if (s<0){
		so(0,"  -ERROR (CreateSocket)");
		return -1;
	}else
		so(1,"  -ok");
	return s;
}

int NixNetCreateSocket(int port,bool blocking)
{
	so(1,"socket...");
	int s=socket(AF_INET, SOCK_STREAM, 0);
	if (s<0){
		so(0,"  -ERROR");
		return -1;
	}else
		so(1,"  -ok");

	NixNetSetBlocking(s,blocking);

	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY; /// An jedem Device warten

	so(1,"bind...");
	if (bind(s,(struct sockaddr *)&my_addr, sizeof(my_addr))==-1){
		so(0,"  -ERROR (bind)");
		NixNetCloseSocket(s);
		return -1;
	}else
		so(1,"  -ok");

	so(1,"listen...");
	if (listen(s, 1)==-1){
		so(0,"  -ERROR (listen)");
		return -1;
	}else
		so(1,"  -ok");
	return s;
}

int NixNetAcceptSocket(int sh)
{
//	so(1,"accept...");
	struct sockaddr_in remote_addr;
	int size=sizeof(remote_addr);
	int sc;
#ifdef NIX_OS_WINDOWS
	sc=accept(sh, (struct sockaddr *)&remote_addr, &size);
#endif
#ifdef NIX_OS_LINUX
	socklen_t len=*(socklen_t*)&size;
	sc=accept(sh, (struct sockaddr *)&remote_addr, &len);
#endif
	if (sc < 0){
		//so("  -FEHLER");
		NixNetCloseSocket(sc);
		return -1;
	}else{
		so(1,"accept...");
		so(1,"  -ok");
	}

	so(1,"  -client found");
	#ifdef NIX_OS_WINDOWS
		so(1,inet_ntoa(remote_addr.sin_addr));//.s_addr));
	#endif
	NixNetSetBlocking(sc,true);
	return sc;
}

int NixNetConnectSocket(char *addr,int port)
{
	int s=-1;
	struct sockaddr_in host_addr;
	struct hostent *host;


	so(1,"GetHostByName...");
	host=gethostbyname(addr);
	if (host==NULL){
		so(0,"  -ERROR (GetHostByName)");
		return -1;
	}else
		so(1,"  -ok");

	s=NixNetCreateSocket_();

	host_addr.sin_family = AF_INET;
	host_addr.sin_addr = *((struct in_addr *)host->h_addr);
	host_addr.sin_port = htons(port);

	so(1,"connect...");
	if (connect(s, (struct sockaddr *)&host_addr, sizeof(host_addr))==-1){
		so(0,"  -ERROR (connect)");
		#ifdef NIX_OS_WINDOWS
			so(0,WSAGetLastError());
		#endif
		NixNetCloseSocket(s);
		return -1;
	}else
		so(1,"  -ok");
	NixNetSetBlocking(s,true);
	return s;
}

/*int NixNetRead(int s,char *buf,int max_size)
{
	int r=recv(s,buf,max_size,0);
	//msg_write(string("recv: ",i2s(r)));
	return r;
}

int NixNetWrite(int s,char *buf,int size)
{
	int r=send(s,buf,size,0);
	return r;
}*/

void NixNetResetBuffer()
{
	// 4bytes for the buffer length information!
	BufferUsed=4;
}

bool NixNetReadBuffer(int s)
{
	/*ZeroMemory(Buffer,NIXNET_MAX_BUFFER);
	so(1,"empfange BUFFER");
	bool ok=true;
	int r=recv(s,Buffer,NIXNET_MAX_BUFFER,0);
	if (r<=0){
		msg_error("beim Empfangen von Daten");
		msg_write(r);
		ok=false;
	}
	if ((NixNetReadyToRead(s))&&(ok)){
		so(1,"##############################################################################");
		so(1,"restliche Daten:");
		so(1,"##############################################################################");
		char B[5000];
		so(1,recv(s,B,sizeof(B),0));
	}
#if NIXNET_DEBUG>2
	msg_write(Buffer,200);
#endif
	BufferUsed=0;
	return ok;*/

	so(1,"RECIEVING!...");
	int recieved=0;
	BufferUsed=4;
	while(recieved<BufferUsed){
		int current=recv(s,&Buffer[recieved],NIXNET_MAX_SEND,0);
		if (current<0){
			so(0,"ERROR (recv)");
			return false;
		}
		recieved+=current;
		BufferUsed=*(int*)&Buffer;
		so(1,string2("%d/%d",recieved,BufferUsed));
	}
	BufferUsed=4;

	return true;
}

int NixNetReadInt()
{
	int i=*(int*)&Buffer[BufferUsed];
	BufferUsed+=sizeof(i);
	so(2,"read int");
	so(2,i);
	return i;
}

bool NixNetReadBool()
{
	bool b=*(bool*)&Buffer[BufferUsed];
	BufferUsed+=sizeof(b);
	so(2,"read bool");
	if (b)	so(2,"true");
	else	so(2,"false");
	return b;
}

float NixNetReadFloat()
{
	float f=*(float*)&Buffer[BufferUsed];
	BufferUsed+=sizeof(f);
	so(2,"read float");
	so(2,f2s(f,3));
	return f;
}

vector NixNetReadVector()
{
	vector v;
	v.x=NixNetReadFloat();
	v.y=NixNetReadFloat();
	v.z=NixNetReadFloat();
	return v;
}

char NixNetReadChar()
{
	char c=Buffer[BufferUsed];
	BufferUsed+=sizeof(c);
	so(2,"read char");
	so(2,c);
	return c;
}

static char NixNetTempStr[1024];

char *NixNetReadStr()
{
	so(2,"read string");
	int l=NixNetReadInt();
	memcpy(NixNetTempStr,&Buffer[BufferUsed],l);
	NixNetTempStr[l]=0;
	BufferUsed+=l;
	so(2,NixNetTempStr);
	return NixNetTempStr;
}

void NixNetReadStrL(char *str,int &length)
{
	so(2,"read string");
	length=NixNetReadInt();
	memcpy(str,&Buffer[BufferUsed],length);
	str[length]=0;
	BufferUsed+=length;
	//msg_write(str,length);
}

bool NixNetWriteBuffer(int s)
{
	so(1,"SENDING!...");
	*(int*)&Buffer=BufferUsed;
	int sent=0;
	while(sent<BufferUsed){
		int current=send(s,&Buffer[sent],(BufferUsed>NIXNET_MAX_SEND)?NIXNET_MAX_SEND:BufferUsed,0);
		if (current<0){
			so(0,"ERROR (send)");
			return false;
		}
		sent+=current;
		so(1,string2("%d/%d",sent,BufferUsed));
	}
	NixNetResetBuffer();
	return true;
}

void NixNetWriteInt(int i)
{
	so(2,"write int");
	so(2,i);
	*(int*)&Buffer[BufferUsed]=i;
	BufferUsed+=sizeof(i);
}

void NixNetWriteBool(bool b)
{
	so(2,"write bool");
	if (b)	so(2,"true");
	else	so(2,"false");
	*(bool*)&Buffer[BufferUsed]=b;
	BufferUsed+=sizeof(b);
}

void NixNetWriteFloat(float f)
{
	so(2,"write float");
	so(2,f2s(f,3));
	*(float*)&Buffer[BufferUsed]=f;
	BufferUsed+=sizeof(f);
}

void NixNetWriteVector(vector v)
{
	NixNetWriteFloat(v.x);
	NixNetWriteFloat(v.y);
	NixNetWriteFloat(v.z);
}

void NixNetWriteChar(char c)
{
	so(2,"write char");
	so(2,c);
	Buffer[BufferUsed]=c;
	BufferUsed+=sizeof(c);
}

void NixNetWriteStr(char *str)
{
	int l=strlen(str);
	NixNetWriteInt(l);
	memcpy(&Buffer[BufferUsed],str,l);
	BufferUsed+=l;
	so(2,"write string");
	so(2,str);
}

void NixNetWriteStrL(char *str,int length)
{
	so(2,"write string");
	NixNetWriteInt(length);
	memcpy(&Buffer[BufferUsed],str,length);
	BufferUsed+=length;
	//msg_write(str,length);
}

bool NixNetReadyToWrite(int s)
{
	//so("teste socket...");
	fd_set r,w;
	FD_ZERO(&r);
	FD_SET(((unsigned)s),&r);
	FD_ZERO(&w);
	FD_SET(((unsigned)s),&w);
	struct timeval t;
	t.tv_sec=0;
	t.tv_usec=10;
	select(s+1,&r,&w,(fd_set*)0,&t);
	/*if (FD_ISSET(s,&w)>0)
		so("w=1");
	else
		so("w=0");*/
	return (FD_ISSET(s,&w)>0);
}

bool NixNetReadyToRead(int s)
{
	//so("teste socket...");
	fd_set r,w;
	FD_ZERO(&r);
	FD_SET(((unsigned)s),&r);
	FD_ZERO(&w);
	FD_SET(((unsigned)s),&w);
	struct timeval t;
	t.tv_sec=0;
	t.tv_usec=10;
	select(s+1,&r,&w,(fd_set*)0,&t);
	/*if (FD_ISSET(s,&r)>0)
		so("r=1");
	else
		so("r=0");*/
	return (FD_ISSET(s,&r)>0);
}



































#if 0

#define NIXWRK_DEBUG		10

#define KnockPort			2184

#define GreetConnect		1
#define GreetGetSessionName	2


static int SG,SC[16];
static char Addr[16][128];
static int NumClients=0;
static char SessionName[128];

static int NumPossibleHosts,NumAvailableHosts;
static char PossibleHostName[32][64],AvailableHostName[32][64],AvailableHostSessionName[32][64];
static bool IAmHost,IAmClient;

char Buffer[10000];
int BufferUsed;





void so(int dbg,char *str)
{
	if (dbg<=NIXWRK_DEBUG)
		msg_write(str);
}

void so(int dbg,int i)
{
	if (dbg>=NIXWRK_DEBUG)
		msg_write(i);
}

CNixWrk::CNixWrk()
{
	if (!msg)
		msg=new CMsg();
#ifdef NIX_OS_WINDOWS
	so(1,"Windows-Standart-Socket-Start");
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1,1),&wsaData)!=0){
		so(1,"  -FEHLER");
		//exit(1);
	}
	else
		so(1,"  -ok");
#endif
	IAmHost=IAmClient=false;
	SG=-1;
	NumPossibleHosts=0;
}

CNixWrk::~CNixWrk()
{
	if ((IAmHost)||(IAmClient))
		CloseSocket(SG);
	if (IAmHost)
		for (int i=0;i<NumClients;i++)
			CloseSocket(SC[i]);
	IAmHost=IAmClient=false;
}

void CNixWrk::CloseSocket(int &s)
{
	//so("close");
#ifdef NIX_OS_WINDOWS
	closesocket(s);
#endif
#ifdef NIX_OS_LINUX
	close(s);
#endif
	s=-1;
}

void CNixWrk::SetBlocking(int s,bool blocking)
{
#ifdef NIX_OS_WINDOWS
	unsigned long l=blocking?1:0;
	ioctlsocket(s,FIONBIO,&l);
#endif
#ifdef NIX_OS_LINUX
	fcntl(s,F_SETFL,blocking?0:O_NONBLOCK);
#endif
}

int CNixWrk::CreateSocket()
{
	so(1,"socket...");
	int s=socket(AF_INET, SOCK_STREAM, 0);
	if (s<0){
		so(0,"  -FEHLER (CreateSocket)");
		return -1;
	}
	else
		so(1,"  -ok");
	return s;
}

int CNixWrk::CreateSocket(int Port,bool Blocking)
{
	so(1,"socket...");
	int s=socket(AF_INET, SOCK_STREAM, 0);
	if (s<0){
		so(0,"  -FEHLER");
		return -1;
	}
	else
		so(1,"  -ok");

	if (!Blocking)
		SetBlocking(s,Blocking);

	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(Port);
	my_addr.sin_addr.s_addr = INADDR_ANY; /// An jedem Device warten

	so(1,"bind...");
	if (bind(s,(struct sockaddr *)&my_addr, sizeof(my_addr))==-1){
		so(0,"  -FEHLER (CreateSocket)");
		CloseSocket(s);
		return -1;
	}
	else
		so(1,"  -ok");

	so(1,"listen...");
	if (listen(s, 1)==-1){
		so(0,"  -FEHLER (Listen)");
		return -1;
	}
	else
		so(1,"  -ok");
	return s;
}

int CNixWrk::AcceptSocket(int sh)
{
	//so("accept...");
	struct sockaddr_in remote_addr;
	int size=sizeof(remote_addr);
	int sc;
#ifdef NIX_OS_WINDOWS
	sc=accept(sh, (struct sockaddr *)&remote_addr, &size);
#endif
#ifdef NIX_OS_LINUX
	socklen_t len=*(socklen_t*)&size;
	sc=accept(sh, (struct sockaddr *)&remote_addr, &len);
#endif
	if (sc < 0){
		//so("  -FEHLER");
		CloseSocket(sc);
		return -1;
	}
	else
		so(1,"  -ok");

	so(1,"  -Client gefunden:");
	#ifdef NIX_OS_WINDOWS
		so(1,inet_ntoa(remote_addr.sin_addr));//.s_addr));
	#endif
	return sc;
}

int CNixWrk::ConnectSocket(char *addr,int port)
{
	int s=-1;
	struct sockaddr_in host_addr;
	struct hostent *host;


	so(1,"GetHostByName...");
	host=gethostbyname(addr);
	if (host==NULL){
		so(0,"  -FEHLER (GetHostByName)");
		return -1;
	}
	else
		so(1,"  -ok");

	s=CreateSocket();

	host_addr.sin_family = AF_INET;
	host_addr.sin_addr = *((struct in_addr *)host->h_addr);
	host_addr.sin_port = htons(port);

	so(1,"connect...");
	if (connect(s, (struct sockaddr *)&host_addr, sizeof(host_addr))==-1){
		#ifdef NIX_OS_WINDOWS
			so(0,string("  -FEHLER (Connect) ",i2s(WSAGetLastError())));
		#endif
		#ifdef NIX_OS_LINUX
			so(0,"  -FEHLER (Connect)");
		#endif
		CloseSocket(s);
		return -1;
	}
	else
		so(1,"  -ok");
	return s;
}

int CNixWrk::Read(int s,char *buf,int max_size)
{
	int r=recv(s,buf,max_size,0);
	//msg_write(string("recv: ",i2s(r)));
	return r;
}

int CNixWrk::Write(int s,char *buf,int size)
{
	int r=send(s,buf,size,0);
	return r;
}

void CNixWrk::DelBuffer()
{
	BufferUsed=0;
}

bool CNixWrk::ReadBuffer(int s)
{
	for (int i=0;i<sizeof(Buffer);i++)
		Buffer[i]=0;
	so(1,"empfange BUFFER");
	bool ok=true;
	int r=recv(s,Buffer,sizeof(Buffer),0);
	if (r<=0){
		msg_error("beim Empfangen von Daten");
		ok=false;
	}
	if ((ReadyToRead(s))&&(ok)){
		so(1,"##############################################################################");
		so(1,"restliche Daten:");
		so(1,"##############################################################################");
		char B[5000];
		so(1,recv(s,B,sizeof(B),0));
	}
#if NIXWRK_DEBUG>2
	msg_write(Buffer,200);
#endif
	BufferUsed=0;
	return ok;
}

int CNixWrk::ReadInt()
{
	int i=*(int*)&Buffer[BufferUsed];
	BufferUsed+=sizeof(i);
	so(2,"empfange int");
	so(2,i);
	return i;
}

bool CNixWrk::ReadBool()
{
	//bool b=*(bool*)&Buffer[BufferUsed];
	//BufferUsed+=sizeof(b);
	bool b=(ReadInt()!=0);
	so(2,"empfange bool");
	if (b)	so(2,"true");
	else	so(2,"false");
	return b;
}

float CNixWrk::ReadFloat()
{
	float f=*(float*)&Buffer[BufferUsed];
	BufferUsed+=sizeof(f);
	so(2,"empfange float");
	so(2,int(f*1000.0f));
	return f;
}

vector CNixWrk::ReadVector()
{
	vector v;
	v.x=ReadFloat();
	v.y=ReadFloat();
	v.z=ReadFloat();
	return v;
}

char CNixWrk::ReadChar()
{
	char c=Buffer[BufferUsed];
	BufferUsed+=sizeof(c);
	so(2,"empfange char");
	so(2,c);
	return c;
}

void CNixWrk::ReadStr(char *str)
{
	int l=ReadInt();
	for (int p=0;p<l;p++)
		str[p]=Buffer[BufferUsed+p];
	str[l]=0;
	BufferUsed+=l;
	so(2,"empfange str");
	so(2,str);
}

bool CNixWrk::WriteBuffer(int s)
{
	so(0,string("sende BUFFER (",f2s(100.0f*BufferUsed/sizeof(Buffer),1),"%)"));
#if NIXWRK_DEBUG>2
	msg_write(Buffer,BufferUsed);
#endif
	bool ok=true;
	if (send(s,Buffer,sizeof(Buffer),0)<0){
		msg_error("beim Senden von Daten");
		ok=false;
	}
	BufferUsed=0;
	return ok;
}

void CNixWrk::WriteInt(int i)
{
	so(2,"sende int");
	so(2,i);
	*(int*)&Buffer[BufferUsed]=i;
	BufferUsed+=sizeof(i);
}

void CNixWrk::WriteBool(bool b)
{
	so(2,"sende bool");
	if (b)	so(2,"true");
	else	so(2,"false");
	if (b)	WriteInt(1);
	else	WriteInt(0);
	//*(bool*)&Buffer[BufferUsed]=b;
	//BufferUsed+=sizeof(b);
}

void CNixWrk::WriteFloat(float f)
{
	so(2,"sende float");
	so(2,int(f*1000.0f));
	*(float*)&Buffer[BufferUsed]=f;
	BufferUsed+=sizeof(f);
}

void CNixWrk::WriteVector(vector v)
{
	WriteFloat(v.x);
	WriteFloat(v.y);
	WriteFloat(v.z);
}

void CNixWrk::WriteChar(char c)
{
	so(2,"sende char");
	so(2,c);
	Buffer[BufferUsed]=c;
	BufferUsed+=sizeof(c);
}

void CNixWrk::WriteStr(char *str)
{
	WriteInt(strlen(str));
	char *b;
	b=&Buffer[BufferUsed];
	strcpy(b,str);
	BufferUsed+=strlen(str);
	so(2,"sende str");
	so(2,str);
}

bool CNixWrk::ReadyToWrite(int s)
{
	//so("teste socket...");
	fd_set r,w;
	FD_ZERO(&r);
	FD_SET(((unsigned)s),&r);
	FD_ZERO(&w);
	FD_SET(((unsigned)s),&w);
	struct timeval t;
	t.tv_sec=0;
	t.tv_usec=10;
	select(s+1,&r,&w,(fd_set*)0,&t);
	/*if (FD_ISSET(s,&w)>0)
		so("w=1");
	else
		so("w=0");*/
	return (FD_ISSET(s,&w)>0);
}

bool CNixWrk::ReadyToRead(int s)
{
	//so("teste socket...");
	fd_set r,w;
	FD_ZERO(&r);
	FD_SET(((unsigned)s),&r);
	FD_ZERO(&w);
	FD_SET(((unsigned)s),&w);
	struct timeval t;
	t.tv_sec=0;
	t.tv_usec=10;
	select(s+1,&r,&w,(fd_set*)0,&t);
	/*if (FD_ISSET(s,&r)>0)
		so("r=1");
	else
		so("r=0");*/
	return (FD_ISSET(s,&r)>0);
}

void CNixWrk::SearchAvailableHosts()
{
	so(1,"SearchAvailableHosts");
	NumAvailableHosts=0;
	for (int i=0;i<NumPossibleHosts;i++){
		so(1,PossibleHostName[i]);
		// Verbindungs-Versuch
		SG=ConnectSocket(PossibleHostName[i],KnockPort);
		if (SG>=0){
			so(1,"kontakt");
			strcpy(AvailableHostName[NumAvailableHosts],PossibleHostName[i]);
			DelBuffer();
			WriteInt(GreetGetSessionName);
			WriteBuffer(SG);
			ReadBuffer(SG);
			ReadStr(AvailableHostSessionName[NumAvailableHosts]);
			msg_write(AvailableHostSessionName[NumAvailableHosts]);
			NumAvailableHosts++;
			CloseSocket(SG);
		}
	}
	so(1,"-fertig");
}

void CNixWrk::SearchAvailableHosts(int num_possible_hosts,char possible_hosts[64][256])
{
	NumPossibleHosts=num_possible_hosts;
	for (int i=0;i<NumPossibleHosts;i++){
		strcpy(PossibleHostName[i],possible_hosts[i]);
	}
	SearchAvailableHosts();
}

int CNixWrk::GetNumAvailableHosts()
{
	return NumAvailableHosts;
}

char *CNixWrk::GetAvailableHostName(int index)
{
	return AvailableHostName[index];
}

char *CNixWrk::GetAvailableHostSessionName(int index)
{
	return AvailableHostSessionName[index];
}

void CNixWrk::TryToConnectToHosts()
{
	IAmHost=false;
	so(1,"TryToConnectToHosts");
	for (int i=0;i<NumPossibleHosts;i++){
		// Verbindungs-Versuch
		so(1,PossibleHostName[i]);
		SG=ConnectSocket(PossibleHostName[i],KnockPort);
		if (SG>=0){
			so(1,"kontakt");
			DelBuffer();
			WriteInt(GreetConnect);
			WriteBuffer(SG);
			IAmClient=true;
			break;
		}
	}
	so(1,"-fertig");
}

void CNixWrk::ConnectToAvailableHost(int index)
{
	// Verbindungs-Versuch
	SG=ConnectSocket(AvailableHostName[index],KnockPort);
	if (SG>=0){
		DelBuffer();
		WriteInt(GreetConnect);
		WriteBuffer(SG);
		IAmClient=true;
	}
}

void CNixWrk::StartHost(char *sessionname)
{
	NumClients=0;
	if (SG<0)
		SG=CreateSocket(KnockPort,false);
	IAmHost=(SG>=0);
	strcpy(SessionName,sessionname);
	IAmClient=false;
}

void CNixWrk::DoClientStuff()
{
}

void CNixWrk::DoHostStuff()
{
	int SGC=AcceptSocket(SG);
	if (SGC>0){
		so(0,"Host-Anruf");
		SetBlocking(SGC,true);
		ReadBuffer(SGC);
		msg_write(Buffer);
		int gc=ReadInt();
		switch (gc){
			case GreetConnect:
				so(0," -Connect");
				SC[NumClients]=SGC;
				NumClients++;
				break;
			case GreetGetSessionName:
				so(0," -SessionName");
				DelBuffer();
				WriteStr(SessionName);
				WriteBuffer(SGC);
				CloseSocket(SGC);
				break;
			default:
				so(0," -???");
				CloseSocket(SGC);
		}
		so(0,"-fertig");
	}
}

int CNixWrk::GetNumClients()
{
	return NumClients;
}

int CNixWrk::GetSClient(int index)
{
	return SC[index];
}

int CNixWrk::GetSHost()
{
	return SG;
}

void CNixWrk::DelClient(int index)
{
	so(1,"####################################");
	so(1,"entferne Client");
	so(1,"####################################");
	CloseSocket(SC[index]);
	for (int c=index;c<NumClients-1;c++){
		SC[c]=SC[c+1];
	}
	NumClients--;
}

void CNixWrk::EndClient()
{
	so(1,"####################################");
	so(1,"beende Client");
	so(1,"####################################");
	CloseSocket(SG);
	IAmClient=false;
}

bool CNixWrk::AmIHost()
{
	return IAmHost;
}

bool CNixWrk::AmIClient()
{
	return IAmClient;
}

void CNixWrk::SafeMessage(char *str)
{
	int s=ConnectSocket("michi.is-a-geek.org",2005);
	strcpy(Buffer,str);
	BufferUsed=strlen(str);
	WriteBuffer(s);
	CloseSocket(s);
}

#endif
