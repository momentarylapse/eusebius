/*----------------------------------------------------------------------------*\
| Nix                                                                          |
| -> abstraction layer for api (graphics, sound, networking)                   |
| -> OpenGL and DirectX9 supported                                             |
|   -> able to switch in runtime                                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.03.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#ifndef _NIX_EXISTS_
#define _NIX_EXISTS_


extern char NixVersion[32];


#include "00_config.h"



// which operating system?

#ifdef _WIN32
	#define NIX_OS_WINDOWS
#else
	#define NIX_OS_LINUX
#endif



// which developing environment?

#if _MSC_VER > 1000
	#define NIX_IDE_VCS
#else
	#define NIX_IDE_DEVCPP
#endif
//#define NIX_IDE_KDEVELOP ...?



// which graphics api possible?

#define NIX_API_NONE					0
#ifdef NIX_OS_WINDOWS
	#ifdef NIX_ALLOW_API_DIRECTX9
		#define NIX_API_DIRECTX9		1
	#endif
#endif
#ifdef NIX_ALLOW_API_OPENGL
	#define NIX_API_OPENGL				2
#endif



// types handled by DirectX9?

#ifdef NIX_OS_WINDOWS
	#ifdef NIX_ALLOW_TYPES_BY_DIRECTX9
		#define NIX_TYPES_BY_DIRECTX9
	#endif
	#ifdef NIX_API_DIRECTX9
		#define NIX_TYPES_BY_DIRECTX9
	#endif
#endif



// sound handled by DirectX9?

#ifdef NIX_ALLOW_SOUND
	#ifdef NIX_API_DIRECTX9
		#ifdef NIX_ALLOW_SOUND_BY_DIRECTX9
			#define NIX_SOUND_DIRECTX9
		#endif
	#endif
#endif




#include <math.h>
#include "hui.h"
#include "msg.h"



// types
struct rect
{
public:
	float x1,x2,y1,y2;
	rect(){};
	rect(float x1,float x2,float y1,float y2)
	{	this->x1=x1;	this->x2=x2;	this->y1=y1;	this->y2=y2;	}
};
struct color
{
public:
	float a,r,g,b;
	color(){};
	color(float a,float r,float g,float b)
	{	this->a=a;	this->r=r;	this->g=g;	this->b=b;	}
	/*color& operator = (const color& c)
	{	a=c.a;	r=c.r;	g=c.g;	b=c.b;	return *this;	}*/
};
struct vector
{
public:
	float x,y,z;
	vector(){};
	vector(float x,float y,float z)
	{	this->x=x;	this->y=y;	this->z=z;	}
	// assignment operators
	vector& operator += (const vector& v)
	{	x+=v.x;	y+=v.y;	z+=v.z;	return *this;	}
	vector& operator -= (const vector& v)
	{	x-=v.x;	y-=v.y;	z-=v.z;	return *this;	}
	vector& operator *= (float f)
	{	x*=f;	y*=f;	z*=f;	return *this;	}
	vector& operator /= (float f)
	{	x/=f;	y/=f;	z/=f;	return *this;	}
	// unitary operator(s)
	vector operator - ()
	{	return vector(-x,-y,-z);	}
	// binary operators
	vector operator + (const vector &v) const
	{	return vector( x+v.x , y+v.y , z+v.z );	}
	vector operator - (const vector &v) const
	{	return vector( x-v.x , y-v.y , z-v.z );	}
	vector operator * (float f) const
	{	return vector( x*f , y*f , z*f );	}
	vector operator / (float f) const
	{	return vector( x/f , y/f , z/f );	}
	friend vector operator * (float f,const vector &v)
	{	return v*f;	}
	bool operator == (const vector &v) const
	{	return ((x==v.x)&&(y==v.y)&&(z==v.z));	}
	bool operator != (const vector &v) const
	{	return !((x==v.x)&&(y==v.y)&&(z==v.z));	}
	float operator * (vector v) const
	{	return x*v.x + y*v.y + z*v.z;	}
	vector operator ^ (vector v) const
	{	return vector( y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x );	}
};
#define _element(row,col)	e[row+col*4]
struct matrix
{
public:
	union{
		struct{
			// the squared form of this block is "transposed"!
			float _00,_10,_20,_30;
			float _01,_11,_21,_31;
			float _02,_12,_22,_32;
			float _03,_13,_23,_33;
		};
		float __e[4][4];
#define _e(i,j)		__e[j][i]
		float e[16];
	};

	matrix(){};
	matrix operator + (const matrix &m) const
	{
		matrix r;
		for (int i=0;i<16;i++)
			r.e[i]=e[i]+m.e[i];
		return r;
	}
	matrix operator - (const matrix &m) const
	{
		matrix r;
		for (int i=0;i<16;i++)
			r.e[i]=e[i]-m.e[i];
		return r;
	}
	matrix operator * (float f) const
	{
		matrix r;
		for (int i=0;i<16;i++)
			r.e[i]=e[i]*f;
		return r;
	}
	friend matrix operator * (float f,const matrix &m)
	{	return m*f;	}
	vector operator * (vector v) const
	{
		return vector(	v.x*_00 + v.y*_01 + v.y*_02 + _03,
						v.x*_10 + v.y*_11 + v.y*_12 + _13,
						v.x*_20 + v.y*_21 + v.y*_22 + _23);
	}
	friend vector operator * (vector v, const matrix &m)
	{	return m*v;	}
};
struct plane{	float a,b,c,d;	};
struct quaternion
{
public:
	float x,y,z,w;
	bool operator == (const quaternion& q) const
	{	return ((x==q.x)&&(y==q.y)&&(z==q.z)&&(w==q.w));	}
	bool operator != (const quaternion& q) const
	{	return !((x==q.x)&&(y==q.y)&&(z==q.z)&&(w!=q.w));	}
};



#define NIX_MAX_VBS			256
#define NIX_MAX_TEXTURES	128
#define NIX_MAX_CUBEMAPS	16
#define NIX_MAX_LIGHTS		128
#define NIX_MAX_SHADERFILES	32
#define NIX_MAX_SOUNDS		128

//--------------------------------------------------------------------//
//                     all about graphics                             //
//--------------------------------------------------------------------//
void CloseAVI(int texture);
extern int NixFontHeight;
extern char NixFontName[128];

typedef void callback_function();

	void _cdecl NixInit(int api,int xres,int yres,int depth,bool fullscreen,CHuiWindow *win);
	void _cdecl NixSetVideoMode(char api,int xres,int yres,int depth,bool fullscreen);
	void NixTellUsWhatsWrong();
	void NixKillDeviceObjects();
	void NixReincarnateDeviceObjects();
	void NixKill();
	void NixKillWindows();
	void NixResize();

	// textures
	int NixLoadTexture(char *filename);
	int _cdecl NixCreateDynamicTexture(int width,int height);
	void NixReloadTexture(int texture);
	void NixUnloadTexture(int texture);
	void NixLoadTextureData(char *filename,void **data,int &texture_width,int &texture_height);
	void NixSetTexture(int texture);
	void NixSetTextureVideoFrame(int texture,int frame);
	void NixTextureVideoMove(int texture,float elapsed);
	void NixSaveTGA(char *filename,int width,int height,int bits,int alpha_bits,void *data,void *palette=NULL);
	int NixCreateCubeMap(int size);
	void NixRenderToCubeMap(int cube_map,vector &pos,callback_function *render_scene,int mask);
	void NixSetCubeMap(int cube_map,int tex0,int tex1,int tex2,int tex3,int tex4,int tex5);

	// drawing functions
	void _cdecl NixSetFontColor(color c);
	void _cdecl NixDrawChar(int x,int y,char c);
	void _cdecl NixDrawStr(int x,int y,char *str);
	int _cdecl NixGetStrWidth(char *str,int start,int end);
	void _cdecl NixDrawFloat(int x,int y,float fl,int com);
	void _cdecl NixDrawInt(int x,int y,int num);
	void _cdecl NixDrawLine(float x1,float y1,float x2,float y2,color c,float depth);
	void _cdecl NixDrawLineV(int x,int y1,int y2,color c,float depth);
	void _cdecl NixDrawLineH(int x1,int x2,int y,color c,float depth);
	void _cdecl NixDrawLine3D(vector l1,vector l2,color c);
	void _cdecl NixDrawRect(float x1,float x2,float y1,float y2,color c,float depth);
	void _cdecl NixDraw2D(int texture,color *col,rect *src,rect *dest,float depth);
	void _cdecl NixDraw3D(int texture,int buffer,matrix *mat);
	void _cdecl NixDraw3D3(int texture0,int texture1,int texture2,int buffer,matrix *mat);
	void _cdecl NixDrawSpriteR(int texture,color *col,rect *src,vector &pos,rect *dest);
	void _cdecl NixDrawSprite(int texture,color *col,rect *src,vector &pos,float radius);
	void _cdecl NixDraw3DCubeMapped(int cube_map,int vertex_buffer,matrix *mat);

	// configuring the view
	void _cdecl NixSetPerspectiveMode(int mode,float param1=0,float param2=0);
	void _cdecl NixSetWorldMatrix(matrix &mat);
	void _cdecl NixSetView(bool enable3d,vector view_pos=vector(0,0,0),vector view_ang=vector(0,0,0),vector scale=vector(1,1,1));
	void _cdecl NixSetView(bool enable3d,matrix &view_mat);
	void _cdecl NixSetViewV(bool enable3d,vector &view_pos,vector &view_ang);
	void _cdecl NixSetViewM(bool enable3d,matrix &view_mat);
//	void SetView2(bool enable3d,matrix &view_mat);
	void _cdecl NixGetVecProject(vector &vout,const vector &vin);
	void _cdecl NixGetVecUnproject(vector &vout,const vector &vin);
	void _cdecl NixGetVecProjectRel(vector &vout,const vector &vin);
	void _cdecl NixGetVecUnprojectRel(vector &vout,const vector &vin);
	bool _cdecl NixIsInFrustrum(vector &pos,float radius);
	bool _cdecl NixStart(int texture=-1);
	void _cdecl NixStartPart(int x1,int y1,int x2,int y2,bool set_centric);
	void _cdecl NixEnd();
	void _cdecl NixSetClipPlane(int index,plane &pl);
	void _cdecl NixEnableClipPlane(int index,bool enabled);
	void _cdecl NixDoScreenShot(char *filename,rect *source=NULL,int width=-1,int height=-1);

	// vertex buffers
	int _cdecl NixCreateVB(int max_trias,int index=-1);
	int _cdecl NixCreateVB3(int max_trias,int index=-1);
	bool NixVBAddTria(int buffer,	vector &p1,vector &n1,float tu1,float tv1,
									vector &p2,vector &n2,float tu2,float tv2,
									vector &p3,vector &n3,float tu3,float tv3);
	bool NixVBAddTria3(int buffer,	vector &p1,vector &n1,float tu01,float tv01,float tu11,float tv11,float tu21,float tv21,
									vector &p2,vector &n2,float tu02,float tv02,float tu12,float tv12,float tu22,float tv22,
									vector &p3,vector &n3,float tu03,float tv03,float tu13,float tv13,float tu23,float tv23);
	void _cdecl NixVBAddTrias(int buffer,int num_trias,vector *p,vector *n,float *t);
	void _cdecl NixVBAddTrias3(int buffer,int num_trias,vector *p,vector *n,float *t0,float *t1,float *t2);
	void _cdecl NixVBAddTriasIndexed(int buffer,int num_points,int num_trias,vector *p,vector *n,float *tu,float *tv,unsigned short *indices);
	void _cdecl NixVBEmpty(int buffer);

	// shader files
	int _cdecl NixLoadShaderFile(char *filename);
	void _cdecl NixDeleteShaderFile(int index);
	void NixSetShaderFile(int index);

	// engine properties
	void _cdecl NixSetWire(bool enabled);
	void _cdecl NixSetCull(int mode);
	void _cdecl NixSetZ(bool write,bool test);
	void _cdecl NixSetAlpha(int mode);
	void _cdecl NixSetAlpha(int src,int dst);
	void _cdecl NixSetAlpha(float factor);
	void _cdecl NixSetAlphaM(int mode);
	void _cdecl NixSetAlphaSD(int src,int dst);
	void _cdecl NixSetFog(int mode,float start,float end,float density,color &c);
	void _cdecl NixEnableFog(bool enabled);
	void _cdecl NixSetStencil(int mode,unsigned long param=0);
	void _cdecl NixSetShading(int mode);

	// light
	void _cdecl NixEnableLighting(bool enabled);
	void _cdecl NixEnableLighting2D(bool enabled);
	int _cdecl NixCreateLight();
	void _cdecl NixDeleteLight(int index);
	void _cdecl NixSetLightRadial(int num,vector &pos,float radius,color &ambient,color &diffuse,color &specular);
	void _cdecl NixSetLightDirectional(int num,vector &dir,color &ambient,color &diffuse,color &specular);
	void _cdecl NixEnableLight(int num,bool enabled);
	void _cdecl NixSetAmbientLight(color &c);
	void _cdecl NixSetMaterial(color &ambient,color &diffuse,color &specular,float shininess,color &emission);
	void _cdecl NixSpecularEnable(bool enabled);

	// handle user input
	void _cdecl NixUpdateInput();
	void _cdecl NixResetInput();
	void _cdecl NixGetInputFromWindow();
	int _cdecl NixGetDx();
	int _cdecl NixGetDy();
	int _cdecl NixGetWheelD();
	int _cdecl NixGetMx();
	int _cdecl NixGetMy();
	float _cdecl NixGetMDir();
	void _cdecl NixResetCursor();
	bool _cdecl NixGetButL();
	bool _cdecl NixGetButM();
	bool _cdecl NixGetButR();
	bool _cdecl NixGetButLDown();
	bool _cdecl NixGetButMDown();
	bool _cdecl NixGetButRDown();
	bool _cdecl NixGetButLUp();
	bool _cdecl NixGetButMUp();
	bool _cdecl NixGetButRUp();
	bool _cdecl NixGetKey(int key);
	bool _cdecl NixGetKeyUp(int key);
	bool _cdecl NixGetKeyDown(int key);
	char _cdecl NixGetKeyChar(int key);
	int _cdecl NixGetKeyRhythmDown();

	// sound
	int NixSoundLoad(char *filename);
	void NixSoundDelete(int index);
	void NixSoundPlay(int index,bool repeat);
	void NixSoundStop(int index);
	void NixSoundSetPause(int index,bool pause);
	bool NixSoundIsPlaying(int index);
	bool NixSoundEnded(int index);
	void NixSoundTestRepeat();
	void NixSoundSetData(int index,vector &pos,vector &vel,float min_dist,float max_dist,float speed,float volume,bool set_now=false);
	void NixSoundSetListener(vector &pos,vector &ang,vector &vel,float meters_per_unit);

	// music
	int _cdecl NixMusicLoad(char *filename);
	void _cdecl NixMusicPlay(int index,bool repeat);
	void _cdecl NixMusicSetRate(int index,float rate);
	void _cdecl NixMusicStop(int index);
	void _cdecl NixMusicSetPause(int index,bool pause);
	bool _cdecl NixMusicIsPlaying(int index);
	bool _cdecl NixMusicEnded(int index);
	void _cdecl NixMusicTestRepeat();


extern int NixApi;
extern int NixScreenWidth,NixScreenHeight,NixScreenDepth;		// current screen resolution
extern int NixDesktopWidth,NixDesktopHeight,NixDesktopDepth;	// pre-NIX-resolution
extern int NixTargetWidth,NixTargetHeight;						// render target size (window/texture)
extern bool NixFullscreen;
extern callback_function *NixRefillAllVertexBuffers;			// animate the application to refill lost VertexBuffers
extern sInputData NixInputDataCurrent,NixInputDataLast;
extern bool NixLightingEnabled,NixLightingEnabled2D;
extern bool NixCullingInverted;

extern float NixMouseMappingWidth,NixMouseMappingHeight;		// fullscreen mouse territory
extern int NixFatalError;
extern int NixNumTrias;

extern int NixTextureWidth[NIX_MAX_TEXTURES],NixTextureHeight[NIX_MAX_TEXTURES];
extern int NixTextureMaxFramesToLive,NixMaxVideoTextureSize;
extern float NixMaxDepth,NixMinDepth;

extern int VBTemp; // vertex buffer for 1-frame geometries



enum{
	FatalErrorNone,
	FatalErrorNoDirectX8,
	FatalErrorNoDirectX9,
	FatalErrorNoDevice,
	FatalErrorUnknownApi
};

//#define ResX	NixScreenWidth
//#define ResY	NixScreenHeight
#define MaxX	NixTargetWidth
#define MaxY	NixTargetHeight



//--------------------------------------------------------------------//
//                         all about types                            //
//--------------------------------------------------------------------//

const float pi=3.141592654f;


// int
int pos_i(int i);

// float
float sqr(float f);
static float sqrt_t3(float f) // f  ( 0.1 , 1 ]
{	return 1 + (0.5f*(f-1)) - (0.125f*sqr(f-1));	}
static float sqrt_t2(float f) // f  ( 0.1 , 1 ]
{	return 1 + (0.5f*(f-1));		}
float pos_f(float f);
float AngBetweenPi(float w);

// vectors
float _cdecl VecLength(const vector &v);
float VecLengthFuzzy(const vector &v);
float VecLengthSqr(const vector &v);
bool VecBetween(vector &v,vector &a,vector &b);
float VecFactorBetween(vector &v,vector &a,vector &b);
bool VecBoundingBox(vector &a,vector &b,float d);
void _cdecl VecNormalize(vector &vo,const vector &vi);
float _cdecl VecDotProduct(const vector &v1,const vector &v2);
vector _cdecl VecCrossProduct(const vector &v1,const vector &v2);
vector _cdecl VecAng2Dir(const vector &ang);
vector _cdecl VecDir2Ang(const vector &dir);
vector _cdecl VecDir2Ang2(const vector &dir,const vector &up);
vector _cdecl VecAngAdd(const vector &ang1,const vector &ang2);
float VecLineDistance(const vector &p,const vector &l1,const vector &l2);
vector VecLineNearestPoint(vector &p,vector &l1,vector &l2);
void _cdecl VecTransform(vector &vo,const matrix &m,const vector &vi);
void _cdecl VecNormalTransform(vector &vo,const matrix &m,const vector &vi);
void VecMin(vector &v,vector &test_partner);
void VecMax(vector &v,vector &test_partner);

// matrices
void _cdecl MatrixMultiply(matrix &m,matrix &m2,matrix &m1);
void _cdecl MatrixIdentity(matrix &m);
void _cdecl MatrixInverse(matrix &mo,matrix &mi);
void _cdecl MatrixTranspose(matrix &mo,matrix &mi);
void _cdecl MatrixTranslation(matrix &m,const vector &v);
void _cdecl MatrixRotationX(matrix &m,float w);
void _cdecl MatrixRotationY(matrix &m,float w);
void _cdecl MatrixRotationZ(matrix &m,float w);
void _cdecl MatrixRotation(matrix &m,const vector &ang);
void _cdecl MatrixRotationView(matrix &m,const vector &ang);
void _cdecl MatrixRotationQ(matrix &m,const quaternion &q);
void _cdecl MatrixScale(matrix &m,float fx,float fy,float fz);
void _cdecl MatrixReflect(matrix &m,plane &pl);

// qaternions
void _cdecl QuaternionRotationA(quaternion &q,const vector &axis,float w);
void _cdecl QuaternionRotationV(quaternion &q,const vector &ang);
//void QuaternionRotationView(quaternion &q,const vector &ang);
void _cdecl QuaternionRotationM(quaternion &q,matrix &m);
void _cdecl QuaternionInverse(quaternion &qo,quaternion &qi);
void _cdecl QuaternionMultiply(quaternion &q,quaternion &q2,quaternion &q1);
void _cdecl QuaternionInterpolate(quaternion &q,quaternion &q1,quaternion &q2,float t);
void _cdecl QuaternionInterpolate(quaternion &q,quaternion &q1,quaternion &q2,quaternion &q3,quaternion &q4,float t);
vector _cdecl QuaternionToAngle(quaternion &q);
void _cdecl QuaternionScale(quaternion &q,float f);
void _cdecl QuaternionNormalize(quaternion &qo,quaternion &qi);
vector GetAxis(quaternion &q);
float GetAngle(quaternion &q);

// planes
void PlaneFromPoints(plane &pl,vector &a,vector &b,vector &c);
void PlaneFromPointNormal(plane &pl,vector &p,vector &n);
void PlaneTransform(plane &plo,matrix &m,plane &pli);
vector GetNormal(plane &pl);
bool PlaneIntersectLine(vector &i,plane &pl,vector &l1,vector &l2);
int ImportantPlane(vector &v);
void GetBaryCentric(vector &P,vector &A,vector &B,vector &C,float &f,float &g);
static float LineIntersectsTriangleF,LineIntersectsTriangleG;
bool LineIntersectsTriangle(vector &t1,vector &t2,vector &t3,vector &l1,vector &l2,vector &col,bool vm);
float PlaneDistance(plane &pl,vector &p);
void PlaneInverse(plane &pl);

// colors
color SetColor(float a,float r,float g, float b);
color SetColorSave(float a,float r,float g, float b);
color SetColorHSB(float a,float hue,float saturation,float brightness);
color ColorScale(color &c,float f);
color ColorInterpolate(color &a,color &b,float t);
color ColorMultiply(color &a,color &b);

static color White=SetColor(1,1,1,1);
static color Black=SetColor(1,0,0,0);
static color Grey=SetColor(1,0.5f,0.5f,0.5f);
static color Gray=SetColor(1,0.5f,0.5f,0.5f);
static color Red=SetColor(1,1,0,0);
static color Green=SetColor(1,0,1,0);
static color Blue=SetColor(1,0,0,1);
static color Yellow=SetColor(1,1,1,0);
static color Orange=SetColor(1,1,0.5f,0);




#define AlphaNone			0
#define AlphaZero			0
#define AlphaOne			1
#define AlphaSourceColor	2
#define AlphaSourceInvColor	3
#define AlphaSourceAlpha	4
#define AlphaSourceInvAlpha	5
#define AlphaDestColor		6
#define AlphaDestInvColor	7
#define AlphaDestAlpha		8
#define AlphaDestInvAlpha	9

#define AlphaColorKey		10
#define AlphaColorKeySmooth	10
#define AlphaColorKeyHard	11
#define AlphaAdd			12
#define AlphaMaterial		13

enum{
	CullNone,
	CullCCW,
	CullCW
};
#define CullDefault		CullCCW

enum{
	StencilNone,
	StencilIncrease,
	StencilDecrease,
	StencilDecreaseNotNegative,
	StencilSet,
	StencilMaskEqual,
	StencilMaskNotEqual,
	StencilMaskLess,
	StencilMaskLessEqual,
	StencilMaskGreater,
	StencilMaskGreaterEqual,
	StencilReset
};

enum{
	FogLinear,
	FogExp,
	FogExp2
};

#define ShadingPlane			0
#define ShadingRound			1

enum{
	// vanishing point (center of projection)
	PerspectiveCenterSet,			// keep values set
	PerspectiveCenterAutoTarget,	// center always in center of target (window, texture)
	// mapped size of perspective view
	PerspectiveSizeSet,				// keep values set
	PerspectiveSizeAutoTarget,		// align size to target
	PerspectiveSizeAutoScreen,		// align size to screen
	// size of the 2D transformation (screen pixels per vertex unit)
	Perspective2DScaleSet,			// keep values set
	Perspective2DScaleAutoTarget,	// target has height and width of 1
	Perspective2DScaleAutoScreen,	// screen has height and width of 1
	// apsect ratio
	PerspectiveRatioSet				// use a user specified apsect ratio
};


//--------------------------------------------------------------------//
//                  all about networking                              //
//--------------------------------------------------------------------//
/*#ifdef NIX_OS_WINDOWS
	#include <winsock.h>
	#pragma comment(lib,"wsock32.lib")
#endif
#ifdef NIX_OS_LINUX
	#include <stdio.h>
	#include <string.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>

	#include <netdb.h>

#endif

class CNixWrk
{
public:
	CNixWrk();
	virtual ~CNixWrk();
	void CloseSocket(int &s);
	void SetBlocking(int s,bool blocking);
	int CreateSocket();
	int CreateSocket(int Port,bool Blocking);
	int AcceptSocket(int sh);
	int ConnectSocket(char *addr,int port);
	int Read(int s,char *buf,int max_size);
	int Write(int s,char *buf,int size);
	void DelBuffer();
	bool ReadBuffer(int s);
	int ReadInt();
	bool ReadBool();
	float ReadFloat();
	vector ReadVector();
	void ReadStr(char *str);
	char ReadChar();
	bool WriteBuffer(int s);
	void WriteInt(int i);
	void WriteBool(bool b);
	void WriteFloat(float f);
	void WriteVector(vector v);
	void WriteChar(char c);
	void WriteStr(char *str);
	bool ReadyToWrite(int s);
	bool ReadyToRead(int s);

	void SearchAvailableHosts();
	void SearchAvailableHosts(int num_possible_hosts,char possible_hosts[64][256]);
	int GetNumAvailableHosts();
	char *GetAvailableHostName(int index);
	char *GetAvailableHostSessionName(int index);

	void TryToConnectToHosts();
	void ConnectToAvailableHost(int index);
	void StartHost(char *sessionname);
	void DoClientStuff();
	void DoHostStuff();
	int GetNumClients();
	int GetSClient(int index);
	int GetSHost();
	void DelClient(int index);
	void EndClient();
	bool AmIHost();
	bool AmIClient();

	void SafeMessage(char *str);
};

extern CNixWrk *nw;*/

void _cdecl NixNetInit();
void _cdecl NixNetCloseSocket(int &s);
void _cdecl NixNetSetBlocking(int s,bool blocking);
int _cdecl NixNetCreateSocket_();
int _cdecl NixNetCreateSocket(int port,bool blocking);
int _cdecl NixNetAcceptSocket(int sh);
int _cdecl NixNetConnectSocket(char *addr,int port);
int _cdecl NixNetRead(int s,char *buf,int max_size);
int _cdecl NixNetWrite(int s,char *buf,int size);
void _cdecl NixNetResetBuffer();
bool _cdecl NixNetReadBuffer(int s);
int _cdecl NixNetReadInt();
bool _cdecl NixNetReadBool();
float _cdecl NixNetReadFloat();
vector _cdecl NixNetReadVector();
char *_cdecl NixNetReadStr();
void _cdecl NixNetReadStrL(char *str,int &length);
char _cdecl NixNetReadChar();
bool _cdecl NixNetWriteBuffer(int s);
void _cdecl NixNetWriteInt(int i);
void _cdecl NixNetWriteBool(bool b);
void _cdecl NixNetWriteFloat(float f);
void _cdecl NixNetWriteVector(vector v);
void _cdecl NixNetWriteChar(char c);
void _cdecl NixNetWriteStr(char *str);
void _cdecl NixNetWriteStrL(char *str,int length);
bool _cdecl NixNetReadyToWrite(int s);
bool _cdecl NixNetReadyToRead(int s);

#endif
