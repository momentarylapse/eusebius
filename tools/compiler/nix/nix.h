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
| last update: 2008.10.27 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#ifndef _NIX_EXISTS_
#define _NIX_EXISTS_


extern char NixVersion[32];


#include "../00_config.h"


#include "nix_config.h"
#include "nix_types.h"
#include "nix_textures.h"
#include "nix_sound.h"
#include "nix_net.h"



//--------------------------------------------------------------------//
//                     all about graphics                             //
//--------------------------------------------------------------------//
void avi_close(int texture);

void _cdecl NixInit(int api,int xres,int yres,int depth,bool fullscreen,CHuiWindow *win);
void _cdecl NixSetVideoMode(char api,int xres,int yres,int depth,bool fullscreen);
void NixTellUsWhatsWrong();
void NixKillDeviceObjects();
void NixReincarnateDeviceObjects();
void NixKill();
void NixKillWindows();
void NixResize();

// drawing functions
void _cdecl NixResetToColor(const color &c);
void _cdecl NixSetFontColor(color c);
void _cdecl NixDrawChar(int x,int y,char c);
void _cdecl NixDrawStr(int x,int y,const char *str);
int _cdecl NixGetStrWidth(const char *str,int start,int end);
void _cdecl NixDrawFloat(int x,int y,float fl,int com);
void _cdecl NixDrawInt(int x,int y,int num);
void _cdecl NixDrawLine(float x1,float y1,float x2,float y2,color c,float depth);
void _cdecl NixDrawLineV(int x,int y1,int y2,color c,float depth);
void _cdecl NixDrawLineH(int x1,int x2,int y,color c,float depth);
void _cdecl NixDrawLine3D(vector l1,vector l2,color c);
void _cdecl NixDrawRect(float x1,float x2,float y1,float y2,color c,float depth);
void _cdecl NixDraw2D(int texture,const color *col,const rect *src,const rect *dest,float depth);
void _cdecl NixDraw3D(int texture,int buffer,const matrix *mat);
void _cdecl NixDraw3DM(int *texture,int buffer,const matrix *mat);
void _cdecl NixDrawSpriteR(int texture,const color *col,const rect *src,const vector &pos,const rect *dest);
void _cdecl NixDrawSprite(int texture,const color *col,const rect *src,const vector &pos,float radius);
void _cdecl NixDraw3DCubeMapped(int cube_map,int vertex_buffer,const matrix *mat);

// configuring the view
void _cdecl NixSetPerspectiveMode(int mode,float param1=0,float param2=0);
void _cdecl NixSetWorldMatrix(const matrix &mat);
void _cdecl NixSetView(bool enable3d,vector view_pos=vector(0,0,0),vector view_ang=vector(0,0,0),vector scale=vector(1,1,1));
void _cdecl NixSetView(bool enable3d,const matrix &view_mat);
void _cdecl NixSetViewV(bool enable3d,const vector &view_pos,const vector &view_ang);
void _cdecl NixSetViewM(bool enable3d,const matrix &view_mat);
//	void SetView2(bool enable3d,matrix &view_mat);
void _cdecl NixGetVecProject(vector &vout,const vector &vin);
void _cdecl NixGetVecUnproject(vector &vout,const vector &vin);
void _cdecl NixGetVecProjectRel(vector &vout,const vector &vin);
void _cdecl NixGetVecUnprojectRel(vector &vout,const vector &vin);
bool _cdecl NixIsInFrustrum(const vector &pos,float radius);
bool _cdecl NixStart(int texture=-1);
void _cdecl NixStartPart(int x1,int y1,int x2,int y2,bool set_centric);
void _cdecl NixEnd();
void _cdecl NixSetClipPlane(int index,const plane &pl);
void _cdecl NixEnableClipPlane(int index,bool enabled);
void _cdecl NixDoScreenShot(const char *filename,const rect *source=NULL);

// vertex buffers
int _cdecl NixCreateVB(int max_trias,int index=-1);
int _cdecl NixCreateVBM(int max_trias,int num_textures,int index=-1);
void _cdecl NixDeleteVB(int buffer);
bool NixVBAddTria(int buffer,	const vector &p1,const vector &n1,const float tu1,const float tv1,
								const vector &p2,const vector &n2,const float tu2,const float tv2,
								const vector &p3,const vector &n3,const float tu3,const float tv3);
bool NixVBAddTriaM(int buffer,	const vector &p1,const vector &n1,const float *t1,
								const vector &p2,const vector &n2,const float *t2,
								const vector &p3,const vector &n3,const float *t3);
void _cdecl NixVBAddTrias(int buffer,int num_trias,const vector *p,const vector *n,const float *t);
void _cdecl NixVBAddTriasM(int buffer,int num_trias,const vector *p,const vector *n,const float *t);
void _cdecl NixVBAddTriasIndexed(int buffer,int num_points,int num_trias,const vector *p,const vector *n,const float *tu,const float *tv,const unsigned short *indices);
void _cdecl NixVBEmpty(int buffer);
void _cdecl NixVBEmptyM(int buffer);

// shader files
int _cdecl NixLoadShaderFile(const char *filename);
void _cdecl NixDeleteShaderFile(int index);
void _cdecl NixSetShaderFile(int index);
void _cdecl NixSetShaderData(int index,const char *var_name,const void *data,int size);
void _cdecl NixGetShaderData(int index,const char *var_name,void *data,int size);

// engine properties
void _cdecl NixSetWire(bool enabled);
void _cdecl NixSetCull(int mode);
void _cdecl NixSetZ(bool write,bool test);
void _cdecl NixSetAlpha(int mode);
void _cdecl NixSetAlpha(int src,int dst);
void _cdecl NixSetAlpha(float factor);
void _cdecl NixSetAlphaM(int mode);
void _cdecl NixSetAlphaSD(int src,int dst);
void _cdecl NixSetFog(int mode,float start,float end,float density,const color &c);
void _cdecl NixEnableFog(bool enabled);
void _cdecl NixSetStencil(int mode,unsigned long param=0);
void _cdecl NixSetShading(int mode);

// light
void _cdecl NixEnableLighting(bool enabled);
void _cdecl NixEnableLighting2D(bool enabled);
int _cdecl NixCreateLight();
void _cdecl NixDeleteLight(int index);
void _cdecl NixSetLightRadial(int num,const vector &pos,float radius,const color &ambient,const color &diffuse,const color &specular);
void _cdecl NixSetLightDirectional(int num,const vector &dir,const color &ambient,const color &diffuse,const color &specular);
void _cdecl NixEnableLight(int num,bool enabled);
void _cdecl NixSetAmbientLight(const color &c);
void _cdecl NixSetMaterial(const color &ambient,const color &diffuse,const color &specular,float shininess,const color &emission);
void _cdecl NixSpecularEnable(bool enabled);

// handle user input
void _cdecl NixUpdateInput();
void _cdecl NixResetInput();
void _cdecl NixGetInputFromWindow();
float _cdecl NixGetDx();
float _cdecl NixGetDy();
float _cdecl NixGetWheelD();
float _cdecl NixGetMx();
float _cdecl NixGetMy();
vector _cdecl NixGetMouseRel();
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
char *_cdecl NixGetKeyChar(int key);
int _cdecl NixGetKeyRhythmDown();




#endif
