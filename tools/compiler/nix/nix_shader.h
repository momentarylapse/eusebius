/*----------------------------------------------------------------------------*\
| Nix shader                                                                   |
| -> shader files                                                              |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_SHADER_EXISTS_
#define _NIX_SHADER_EXISTS_


// shader files
int _cdecl NixLoadShaderFile(const char *filename);
void _cdecl NixDeleteShaderFile(int index);
void _cdecl NixSetShaderFile(int index);
void _cdecl NixSetShaderData(int index,const char *var_name,const void *data,int size);
void _cdecl NixGetShaderData(int index,const char *var_name,void *data,int size);


#endif
