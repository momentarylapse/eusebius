/*----------------------------------------------------------------------------*\
| Nix view                                                                     |
| -> camera etc...                                                             |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_VIEW_EXISTS_
#define _NIX_VIEW_EXISTS_

// configuring the view
void _cdecl NixSetPerspectiveMode(int mode,float param1=0,float param2=0);
void _cdecl NixSetWorldMatrix(const matrix &mat);
void _cdecl NixSetView(bool enable3d,const vector &view_pos,const vector &view_ang,const vector &scale);
void _cdecl NixSetView(bool enable3d,const matrix &view_mat);
void _cdecl NixSetViewV(bool enable3d,const vector &view_pos,const vector &view_ang);
void _cdecl NixSetViewM(bool enable3d,const matrix &view_mat);
//	void SetView2(bool enable3d,matrix &view_mat);
void _cdecl NixGetVecProject(vector &vout,const vector &vin);
void _cdecl NixGetVecUnproject(vector &vout,const vector &vin);
void _cdecl NixGetVecProjectRel(vector &vout,const vector &vin);
void _cdecl NixGetVecUnprojectRel(vector &vout,const vector &vin);
bool _cdecl NixIsInFrustrum(const vector &pos,float radius);
void _cdecl NixSetClipPlane(int index,const plane &pl);
void _cdecl NixEnableClipPlane(int index,bool enabled);
void NixResize();

bool _cdecl NixStart(int texture=-1);
void _cdecl NixStartPart(int x1,int y1,int x2,int y2,bool set_centric);
void _cdecl NixEnd();

void _cdecl NixDoScreenShot(const char *filename,const rect *source=NULL);


#endif
