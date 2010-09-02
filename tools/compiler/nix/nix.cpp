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
#include "nix_common.h"


char NixVersion[]="0.9.1.1";


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
bool NixUsable,NixDoingEvilThingsToTheDevice;

// things'n'stuff
static bool WireFrame;
static matrix *PostProjectionMatrix; // for creating the NixProjectionMatrix
static matrix identity_matrix;
static int ClipPlaneMask;
int NixFontGlyphWidth[256];

static int TimerKey;


int NixApi;
int NixScreenWidth,NixScreenHeight,NixScreenDepth;		// current screen resolution
int NixDesktopWidth,NixDesktopHeight,NixDesktopDepth;	// pre-NIX-resolution
int NixTargetWidth,NixTargetHeight;						// render target size (window/texture)
rect NixTargetRect;
bool NixFullscreen;
callback_function *NixRefillAllVertexBuffers;

float NixMouseMappingWidth,NixMouseMappingHeight;		// fullscreen mouse territory
int NixFatalError;
int NixNumTrias;
int NixTextureMaxFramesToLive,NixMaxVideoTextureSize=256;
float NixMaxDepth,NixMinDepth;

bool NixLightingEnabled,NixLightingEnabled2D;
bool NixCullingInverted;


int NixFontHeight=20;
char NixFontName[128]="Times New Roman";

int VBTemp;

#ifdef NIX_OS_WINDOWS
	static HMENU hMenu;
	static HDC hDC;
	static HGLRC hRC;
	static RECT WindowClient,WindowBounds;
	static DWORD WindowStyle;
#endif



//#define ENABLE_INDEX_BUFFERS

#ifdef NIX_API_DIRECTX9

	// Direct3D
	PDIRECT3D9 lpD3D=NULL;
	IDirect3DDevice9 *lpDevice=NULL;
	D3DCAPS9 d3dCaps;
	D3DPRESENT_PARAMETERS d3dpp;
	D3DSURFACE_DESC d3dsdBackBuffer;
	LPDIRECT3DSURFACE9 FrontBuffer=NULL,DepthBuffer=NULL;
	D3DVIEWPORT9 DXViewPort;
	// 2d vertex buffer
	LPDIRECT3DVERTEXBUFFER9 DXVB2D;

	// shader files
	LPD3DXEFFECT DXEffectCurrent;
	unsigned int DXEffectCurrentNumPasses;

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
		bool NixGLDoubleBuffered;
	#endif

	matrix NixOGLProjectionMatrix2D;

	// shader files
	int glShaderCurrent = 0;

	// font
	int NixOGLFontDPList;
#endif

#ifdef NIX_OS_LINUX
#ifdef NIX_ALLOW_FULLSCREEN
	XF86VidModeModeInfo *original_mode;
#endif
	int screen;
#endif


#ifdef NIX_API_OPENGL
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
		NixFontGlyphWidth[c]=size.cx;
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
		glListBase(NixOGLFontDPList);
		glCallLists(1,GL_UNSIGNED_BYTE,&c);
		glGetFloatv(GL_CURRENT_RASTER_POSITION,x);
		NixFontGlyphWidth[c]=int(x[0]+0.5f)-x0;
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



void NixInit(int api,int xres,int yres,int depth,bool fullscreen,CHuiWindow *win)
{
	NixUsable=false;
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
		NixWindow->NixGetInputFromWindow = &NixGetInputFromWindow;
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
		NixDesktopWidth = XDisplayWidth(hui_x_display, 0);
		NixDesktopHeight = XDisplayHeight(hui_x_display, 0);
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
	NixTargetRect = rect(0, NixTargetWidth, 0, NixTargetHeight);
	MatrixIdentity(identity_matrix);
	MatrixIdentity(NixViewMatrix);
	MatrixIdentity(NixProjectionMatrix);
	NixSetPerspectiveMode(PerspectiveSizeAutoScreen);
	NixSetPerspectiveMode(PerspectiveCenterAutoTarget);
	NixSetPerspectiveMode(Perspective2DScaleSet,1,1);
	NixSetPerspectiveMode(PerspectiveRatioSet,4.0f/3.0f);
	NixEnabled3D=true;
	NixMouseMappingWidth=1024;
	NixMouseMappingHeight=768;
	NixMaxDepth=100000.0f;
	NixMinDepth=1.0f;
	NixTextureMaxFramesToLive=4096*8;
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
	NixUsable=true;


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
	GLX_STENCIL_SIZE,4,
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
	NixUsable = false;
	NixFatalError = FatalErrorNone;
	NixDoingEvilThingsToTheDevice = true;
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
			if ((!NixFullscreen) && (was_fullscreen)){
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
				NixGLDoubleBuffered = true;
			}else{
				msg_error("only singlebuffered visual!");
				vi = glXChooseVisual(hui_x_display, screen, attrListSgl);
				NixGLDoubleBuffered = false;
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
		NixOGLFontDPList=glGenLists(256);
		#ifdef NIX_OS_WINDOWS
			HFONT hFont=CreateFont(NixFontHeight,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE|DEFAULT_PITCH,hui_tchar_str(NixFontName));
			SelectObject(hDC,hFont);
			wglUseFontBitmaps(hDC,0,255,NixOGLFontDPList);
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
			glXUseXFont(font,0,256,NixOGLFontDPList);
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


	NixDoingEvilThingsToTheDevice = false;
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


	NixUsable = true;
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
	InitiateSystemShutdown(NULL,(win_str)hui_tchar_str("Resistance is futile!"),10,TRUE,FALSE);
	//ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE,0);
	msg_db_l(0);
#endif
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
	if ((NixViewScale.x*NixViewScale.y*NixViewScale.z<0)!=(NixCullingInverted)){
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
		}else if (mode==StencilReset){
			glClearStencil(param);
			glClear(GL_STENCIL_BUFFER_BIT);
		}else if ((mode==StencilIncrease)||(mode==StencilDecrease)||(mode==StencilDecreaseNotNegative)||(mode==StencilSet)){
			glEnable(GL_STENCIL);
			//glDisable(GL_STENCIL_TEST);
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS,param,0xffffffff);
			if (mode==StencilIncrease)
				glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
			else if ((mode==StencilDecrease)||(mode==StencilDecreaseNotNegative))
				glStencilOp(GL_KEEP,GL_KEEP,GL_DECR);
			else if (mode==StencilSet)
				glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
		}else if ((mode==StencilMaskEqual)||(mode==StencilMaskNotEqual)||(mode==StencilMaskLessEqual)||(mode==StencilMaskLess)||(mode==StencilMaskGreaterEqual)||(mode==StencilMaskGreater)){
			glEnable(GL_STENCIL);
			glEnable(GL_STENCIL_TEST);
			glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
			if (mode==StencilMaskEqual)
				glStencilFunc(GL_EQUAL,param,0xffffffff);
			else if (mode==StencilMaskNotEqual)
				glStencilFunc(GL_NOTEQUAL,param,0xffffffff);
			else if (mode==StencilMaskLessEqual)
				glStencilFunc(GL_LEQUAL,param,0xffffffff);
			else if (mode==StencilMaskLess)
				glStencilFunc(GL_LESS,param,0xffffffff);
			else if (mode==StencilMaskGreaterEqual)
				glStencilFunc(GL_GEQUAL,param,0xffffffff);
			else if (mode==StencilMaskGreater)
				glStencilFunc(GL_GREATER,param,0xffffffff);
		}
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
