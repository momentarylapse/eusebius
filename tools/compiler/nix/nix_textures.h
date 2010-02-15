/*----------------------------------------------------------------------------*\
| Nix textures                                                                 |
| -> texture loading and handling                                              |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2008.11.02 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_TEXTURES_EXISTS_
#define _NIX_TEXTURES_EXISTS_

// textures
void NixTexturesInit();
void NixReleaseTextures();
void NixReincarnateTextures();
void NixProgressTextureLifes();
void NixSetShaderTexturesDX(void *fx,int texture0,int texture1,int texture2,int texture3);
void NixSetCubeMapDX(int texture);
void NixRenderToTextureBeginDX(int texture);
void NixRenderToTextureEndDX(int texture);
//void NixTextures
int NixLoadTexture(char *filename);
int _cdecl NixCreateDynamicTexture(int width,int height);
void NixReloadTexture(int texture);
void NixUnloadTexture(int texture);
void NixLoadTextureData(char *filename,void **data,int &texture_width,int &texture_height);
void NixSetTexture(int texture);
void NixSetTextures(int *texture,int num_textures);
void NixSetTextureVideoFrame(int texture,int frame);
void NixTextureVideoMove(int texture,float elapsed);
void NixSaveTGA(char *filename,int width,int height,int bits,int alpha_bits,void *data,void *palette=NULL);
int NixCreateCubeMap(int size);
void NixRenderToCubeMap(int cube_map,vector &pos,callback_function *render_scene,int mask);
void NixSetCubeMap(int cube_map,int tex0,int tex1,int tex2,int tex3,int tex4,int tex5);

#endif
