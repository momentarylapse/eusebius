/*----------------------------------------------------------------------------*\
| Nix input                                                                    |
| -> user input (mouse/keyboard)                                               |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_INPUT_EXISTS_
#define _NIX_INPUT_EXISTS_


void _cdecl NixUpdateInput();
void _cdecl NixResetInput();
void _cdecl NixGetInputFromWindow();
/*float _cdecl NixGetDx();
float _cdecl NixGetDy();
float _cdecl NixGetWheelD();
float _cdecl NixGetMx();
float _cdecl NixGetMy();
vector _cdecl NixGetMouseRel();*/
float _cdecl NixGetMDir();
void _cdecl NixResetCursor();
/*bool _cdecl NixGetButL();
bool _cdecl NixGetButM();
bool _cdecl NixGetButR();
bool _cdecl NixGetButLDown();
bool _cdecl NixGetButMDown();
bool _cdecl NixGetButRDown();
bool _cdecl NixGetButLUp();
bool _cdecl NixGetButMUp();
bool _cdecl NixGetButRUp();*/
bool _cdecl NixGetButton(int but);
bool _cdecl NixGetButtonDown(int but);
bool _cdecl NixGetButtonUp(int but);
bool _cdecl NixGetKey(int key);
bool _cdecl NixGetKeyUp(int key);
bool _cdecl NixGetKeyDown(int key);
const char *_cdecl NixGetKeyChar(int key);
int _cdecl NixGetKeyRhythmDown();

extern vector NixMouse, NixMouseRel, NixMouseD, NixMouseDRel;


#endif
