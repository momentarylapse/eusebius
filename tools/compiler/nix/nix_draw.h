/*----------------------------------------------------------------------------*\
| Nix draw                                                                     |
| -> drawing functions                                                         |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_DRAW_EXISTS_
#define _NIX_DRAW_EXISTS_


void _cdecl NixResetToColor(const color &c);
void _cdecl NixSetFontColor(const color &c);
void _cdecl NixDrawChar(int x,int y,char c);
void _cdecl NixDrawStr(int x,int y,const char *str);
int _cdecl NixGetStrWidth(const char *str,int start,int end);
void _cdecl NixDrawFloat(int x,int y,float fl,int com);
void _cdecl NixDrawInt(int x,int y,int num);
void _cdecl NixDrawLine(float x1,float y1,float x2,float y2,const color &c,float depth);
void _cdecl NixDrawLineV(int x,int y1,int y2,const color &c,float depth);
void _cdecl NixDrawLineH(int x1,int x2,int y,const color &c,float depth);
void _cdecl NixDrawLines(float *x, float *y, int num_lines, bool contiguous, const color &c, float depth);
void _cdecl NixDrawLine3D(const vector &l1,const vector &l2,const color &c);
void _cdecl NixDrawRect(float x1,float x2,float y1,float y2,const color &c,float depth);
void _cdecl NixDraw2D(int texture,const color &col,const rect &src,const rect &dest,float depth);
void _cdecl NixDraw3D(int texture,int buffer,const matrix &mat);
void _cdecl NixDraw3DM(int *texture,int buffer,const matrix &mat);
void _cdecl NixDrawSpriteR(int texture,const color &col,const rect &src,const vector &pos,const rect &dest);
void _cdecl NixDrawSprite(int texture,const color &col,const rect &src,const vector &pos,float radius);
void _cdecl NixDraw3DCubeMapped(int cube_map,int vertex_buffer,const matrix &mat);

extern float NixLineWidth;
extern bool NixSmoothLines;

#endif
