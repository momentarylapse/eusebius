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

int NixLoadTexture(const char *filename);
int _cdecl NixCreateDynamicTexture(int width,int height);
//void NixReloadTexture(int texture);
//void NixUnloadTexture(int texture);
void NixLoadTextureData(const char *filename, void **data, int &texture_width, int &texture_height);
void NixSetTexture(int texture);
void NixSetTextures(int *texture,int num_textures);
void NixSetTextureVideoFrame(int texture,int frame);
void NixTextureVideoMove(int texture,float elapsed);
void NixSaveTGA(const char *filename,int width,int height,int bits,int alpha_bits,void *data);
int NixCreateCubeMap(int size);
void NixRenderToCubeMap(int cube_map,vector &pos,callback_function *render_scene,int mask);
void NixSetCubeMap(int cube_map,int tex0,int tex1,int tex2,int tex3,int tex4,int tex5);



struct sNixTexture
{
	char Filename[256];
	int Width, Height;
	bool IsDynamic, Valid;
	int LifeTime;
	
#ifdef NIX_API_DIRECTX9
	LPDIRECT3DTEXTURE9 dxTexture;
	LPD3DXRENDERTOSURFACE dxRenderTarget;
	LPDIRECT3DSURFACE9 dxTextureSurface;
#endif
#ifdef NIX_API_OPENGL
	unsigned int glTexture;
	#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
		unsigned int glFrameBuffer;
		unsigned int glDepthRenderBuffer;
	#endif
#endif
};

extern std::vector<sNixTexture> NixTexture;

#endif
