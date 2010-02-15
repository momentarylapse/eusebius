/*----------------------------------------------------------------------------*\
| Nix textures                                                                 |
| -> texture loading and handling                                              |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2008.11.09 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"

#include <stdio.h>

#ifdef NIX_OS_WINDOWS
	#ifdef NIX_ALLOW_VIDEO_TEXTURE
		#include "vfw.h"
	#endif
	#include <io.h>
#endif
#ifdef NIX_OS_LINUX
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#define _open	open
	#define _close	close
#endif

#ifdef NIX_API_DIRECTX9
	#include <d3dx9.h>
#endif
#ifdef NIX_API_OPENGL
	#ifdef NIX_OS_WINDOWS
		#include <gl\gl.h>
		#include <gl\glu.h>
	#endif
	#ifdef NIX_OS_LINUX
		#define GL_GLEXT_PROTOTYPES
		#include <GL/gl.h>
		#include <GL/glext.h>
		#include <GL/glu.h>
		//#include <GL/glut.h>
	#endif
#endif


static char TextureFile[NIX_MAX_TEXTURES][256];
static int NumTextures;
static bool TextureIsDynamic[NIX_MAX_TEXTURES];
static int TextureLifeTime[NIX_MAX_TEXTURES];
static int NumCubeMaps;
static int CubeMapSize[NIX_MAX_CUBEMAPS];


#ifdef NIX_API_DIRECTX9
	extern IDirect3DDevice9 *lpDevice;
	extern D3DCAPS9 d3dCaps;
	extern D3DSURFACE_DESC d3dsdBackBuffer;
	extern D3DVIEWPORT9 DXViewPort;
	//textures
	LPDIRECT3DTEXTURE9 DXTexture[NIX_MAX_TEXTURES];
	LPD3DXRENDERTOSURFACE DXTextureRenderTarget[NIX_MAX_TEXTURES];
	LPDIRECT3DSURFACE9 DXTextureSurface[NIX_MAX_TEXTURES];
	// cube maps
	ID3DXRenderToEnvMap *DXRenderToEnvMap[NIX_MAX_CUBEMAPS];
	IDirect3DCubeTexture9* DXCubeMap[NIX_MAX_CUBEMAPS];

	extern char *DXErrorMsg(HRESULT h);
#endif
#ifdef NIX_API_OPENGL
	unsigned int OGLTexture[NIX_MAX_TEXTURES];
	#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
		#ifdef NIX_OS_WINDOWS
			#include "glext.h"
			extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
			extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
			extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
			extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
			extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
			extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
			extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
			extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
			extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
			extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
			extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
			extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
			extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
			extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
			extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
			extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
			extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;
		#endif

		extern GLuint OGLFrameBuffer[NIX_MAX_TEXTURES];
		extern GLuint OGLDepthRenderBuffer[NIX_MAX_TEXTURES];
		extern bool OGLDynamicTextureSupport;
	#endif
#endif


extern matrix NixViewMatrix,NixProjectionMatrix,NixInvProjectionMatrix;
extern void NixDraw2D(int texture,const color *col,const rect *src,const rect *dest,float depth);
extern void NixSetView(bool enable3d,matrix &view_mat);
extern bool NixStart(int texture);
extern void NixEnd();





//--------------------------------------------------------------------------------------------------
// avi files
//--------------------------------------------------------------------------------------------------

#ifdef NIX_ALLOW_VIDEO_TEXTURE


struct s_avi_info
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
}*avi_info[NIX_MAX_TEXTURES];


static void avi_flip(void* buffer,int w,int h)
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

void avi_grab_frame(int texture,int frame)									// Grabs A Frame From The Stream
{
	if (texture<0)
		return;
	if (!avi_info[texture])
		return;
	if (avi_info[texture]->ActualFrame==frame)
		return;
	int w=NixTextureWidth[texture];
	int h=NixTextureHeight[texture];
	msg_write(w);
	msg_write(h);
	avi_info[texture]->ActualFrame=frame;
	LPBITMAPINFOHEADER lpbi;									// Holds The Bitmap Header Information
	lpbi = (LPBITMAPINFOHEADER)AVIStreamGetFrame(avi_info[texture]->pgf, frame);	// Grab Data From The AVI Stream
	avi_info[texture]->pdata=(char *)lpbi+lpbi->biSize+lpbi->biClrUsed * sizeof(RGBQUAD);	// Pointer To Data Returned By AVIStreamGetFrame

	// Convert Data To Requested Bitmap Format
	DrawDibDraw (avi_info[texture]->hdd, avi_info[texture]->hdc, 0, 0, w, h, lpbi, avi_info[texture]->pdata, 0, 0, avi_info[texture]->width, avi_info[texture]->height, 0);
	

	//avi_flip(avi_info[texture]->data,w,h);	// Swap The Red And Blue Bytes (GL Compatability)

	// Update The Texture
	#ifdef NIX_API_DIRECTX9
		if (NixApi==NIX_API_DIRECTX9){
			D3DLOCKED_RECT lr;
			DXTexture[texture]->LockRect(0,&lr,NULL,D3DLOCK_DISCARD);
			//memcpy(lr.pBits,avi_info[texture]->data,3*w*h);
			unsigned char *data=(unsigned char *)lr.pBits;
			for (int i=0;i<w*h;i++){
				data[i*4+2]=avi_info[texture]->data[3*i  ];
				data[i*4+1]=avi_info[texture]->data[3*i+1];
				data[i*4+0]=avi_info[texture]->data[3*i+2];
				//data[i*4+3]=255;
			}
			DXTexture[texture]->UnlockRect(0);
		}
	#endif
	#ifdef NIX_API_OPENGL
		if (NixApi==NIX_API_OPENGL){
			glBindTexture(GL_TEXTURE_2D,OGLTexture[texture]);
			glEnable(GL_TEXTURE_2D);
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, avi_info[texture]->data);
			//gluBuild2DMipmaps(GL_TEXTURE_2D,3,w,h,GL_RGB,GL_UNSIGNED_BYTE,AVIdata);
		}
	#endif
}

bool avi_open(int texture,LPCSTR szFile)
{
	msg_db_r("OpenAvi",1);
	avi_info[texture]->hdc = CreateCompatibleDC(0);
	avi_info[texture]->hdd = DrawDibOpen();

	// Opens The AVI Stream
	if (AVIStreamOpenFromFile(&avi_info[texture]->pavi, sys_str_f(szFile), streamtypeVIDEO, 0, OF_READ, NULL) !=0){
		msg_error("Failed To Open The AVI Stream");
		return false;
	}

	AVIStreamInfo(avi_info[texture]->pavi, &avi_info[texture]->psi, sizeof(avi_info[texture]->psi));						// Reads Information About The Stream Into psi
	avi_info[texture]->width=avi_info[texture]->psi.rcFrame.right-avi_info[texture]->psi.rcFrame.left;					// Width Is Right Side Of Frame Minus Left
	avi_info[texture]->height=avi_info[texture]->psi.rcFrame.bottom-avi_info[texture]->psi.rcFrame.top;

	avi_info[texture]->lastframe=AVIStreamLength(avi_info[texture]->pavi);
	avi_info[texture]->fps=float(avi_info[texture]->lastframe)/float(AVIStreamSampleToTime(avi_info[texture]->pavi,avi_info[texture]->lastframe)/1000.0f);

	avi_info[texture]->bmih.biSize = sizeof (BITMAPINFOHEADER);					// Size Of The BitmapInfoHeader
	avi_info[texture]->bmih.biPlanes = 1;											// Bitplanes
	avi_info[texture]->bmih.biBitCount = 24;										// Bits Format We Want (24 Bit, 3 Bytes)
//	avi_info[texture]->bmih.biWidth = AVI_TEXTURE_WIDTH;											// Width We Want (256 Pixels)
//	avi_info[texture]->bmih.biHeight = AVI_TEXTURE_HEIGHT;										// Height We Want (256 Pixels)
	avi_info[texture]->bmih.biCompression = BI_RGB;								// Requested Mode = RGB

	avi_info[texture]->hBitmap = CreateDIBSection (avi_info[texture]->hdc, (BITMAPINFO*)(&avi_info[texture]->bmih), DIB_RGB_COLORS, (void**)(&avi_info[texture]->data), NULL, 0);
	SelectObject (avi_info[texture]->hdc, avi_info[texture]->hBitmap);								// Select hBitmap Into Our Device Context (hdc)

	avi_info[texture]->pgf=AVIStreamGetFrameOpen(avi_info[texture]->pavi, NULL);						// Create The PGETFRAME	Using Our Request Mode
	if (avi_info[texture]->pgf==NULL){
		msg_error("Failed To Open The AVI Frame");
		return false;
	}

	NixTextureWidth[texture]=GetBestVideoSize(avi_info[texture]->width);
	NixTextureHeight[texture]=GetBestVideoSize(avi_info[texture]->height);
	msg_write(NixTextureWidth[texture]);
	msg_write(NixTextureHeight[texture]);

	avi_info[texture]->time=0;
	avi_info[texture]->ActualFrame=1;
	avi_grab_frame(texture,1);
	msg_db_l(1);
	return true;
}

void avi_close(int texture)
{
	if (!avi_info[texture])
		return;
	DeleteObject(avi_info[texture]->hBitmap);										// Delete The Device Dependant Bitmap Object
	DrawDibClose(avi_info[texture]->hdd);											// Closes The DrawDib Device Context
	AVIStreamGetFrameClose(avi_info[texture]->pgf);								// Deallocates The GetFrame Resources
	AVIStreamRelease(avi_info[texture]->pavi);										// Release The Stream
	//AVIFileExit();												// Release The File
}

#endif

//#ifdef NIX_API_OPENGL

inline unsigned int get_int_from_buffer(unsigned char *buffer,int pos,int bytes)
{
	unsigned int r=0;
	/*for (int i=pos;i<pos+bytes;i++)
		r=r*256+buffer[i];*/
	for (int i=pos+bytes-1;i>=pos;i--)
		r=r*256+buffer[i];
	return r;
}

struct sNixImageColor
{
	unsigned char r,g,b,a;
};

struct sNixImage
{
	int width,height;
	unsigned char *data;
	bool alpha_used;
	bool error;
}NixImage;


//--------------------------------------------------------------------------------------------------
// bmp files
//--------------------------------------------------------------------------------------------------

inline void nix_fill_image_color( sNixImageColor *data, unsigned char *col, unsigned char *pal, int depth, int alpha_bits, bool pal_tga )
{
	// 8 bit (paletted tga/bmp)
	if (depth==8){
		if (pal_tga){
			data->r=pal[col[0]*3+2];
			data->g=pal[col[0]*3+1];
			data->b=pal[col[0]*3  ];
			data->a=255;
		}else{
			data->r=pal[col[0]*4+2];
			data->g=pal[col[0]*4+1];
			data->b=pal[col[0]*4  ];
			data->a=255;
		}
	}

	// 16 bit (just tga)
	if (depth==16){
		if (alpha_bits>0){
			data->r=(col[1]&252)*2;
			data->g=(col[1]&3)*64+(col[0]&224)/4;
			data->b=(col[0]&31)*8;
			data->a=(col[1]&128)/128*255;
		}else{
			data->r=(col[1]&252)*2;
			data->g=(col[1]&3)*64+(col[0]&224)/4;
			data->b=(col[0]&31)*8;
			data->a=255;
		}
	}

	// 24 bit (tga/bmp)
	if (depth==24){
		data->r=col[2];
		data->g=col[1];
		data->b=col[0];
		data->a=255;
	}

	// 32 bit (just tga)
	if (depth==32){
		data->r=col[2];
		data->g=col[1];
		data->b=col[0];
		data->a=col[3];
	}

	// color key test
	if (data->r==0)
		if (data->g==255)
			if (data->b==0){
				// color == green?
				//   -> color := black    alpha := 0
				data->g=0;
				data->a=0;
			}
}

void NixLoadBmp(char *filename)
{
	//msg_write("bmp");
	unsigned char Header[56];
	int n;
	unsigned char *data=NULL,*pal=NULL,temp_buffer[8];
	FILE* f=fopen(filename,"rb");
	int r=fread(&Header,56,1,f);

	NixImage.width=get_int_from_buffer(Header,18,4);
	NixImage.height=get_int_from_buffer(Header,22,4);
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
	int depth=get_int_from_buffer(Header,28,2);
	/*msg_write(NixImage.width);
	msg_write(NixImage.height);
	msg_write(depth);*/
	NixImage.data=new unsigned char[NixImage.width * NixImage.height * 4];
	int offset=get_int_from_buffer(Header,10,4);
	int bytes_per_row_o= NixImage.width * depth / 8;
	int bytes_per_row = bytes_per_row_o+(4 - bytes_per_row_o % 4) % 4;
	int clr_used=get_int_from_buffer(Header,46,4);

	if (depth<16){
		if (clr_used==0)clr_used=1<<depth;
		//msg_write(clr_used);
		pal= new unsigned char[4*clr_used];
		fseek(f,-2,SEEK_CUR);
		r=fread(pal,4,clr_used,f);
	}

	fseek(f, offset, SEEK_SET);
	data=new unsigned char[bytes_per_row * NixImage.height];
	//msg_write(bytes_per_row);
	if (reversed){
		//msg_write("Reversed!");
		for (n=0;n<NixImage.height;n++){
			r=fread(data + (bytes_per_row_o * n), sizeof(unsigned char), bytes_per_row_o, f);
			r=fread(temp_buffer, 1, bytes_per_row-bytes_per_row_o, f);
		}
	}else{
		//msg_write("nicht Reversed!");
		for (n=NixImage.height-1;n>=0;n--){
			r=fread(data + (bytes_per_row_o * n), sizeof(unsigned char), bytes_per_row_o, f);
			r=fread(temp_buffer, 1, bytes_per_row-bytes_per_row_o, f);
		}
	}

	switch(depth){
		case 24:
		case 8:
			for (n=0;n<NixImage.width * NixImage.height;n++)
				nix_fill_image_color((sNixImageColor*)(NixImage.data+n*4),data+n*(depth/8),pal,depth,0,false);
			break;
		default:
			msg_error(string("unbehandelte Farbtiefe: ",i2s(depth)));
	}
	if (pal)
		delete[]pal;
	delete[]data;
	fclose(f);
}


//--------------------------------------------------------------------------------------------------
// tga files
//--------------------------------------------------------------------------------------------------


void NixLoadTga(char *filename)
{
	//msg_write("tga");
	unsigned char Header[18];
	int x;
	unsigned char *data=NULL,*pal=NULL;
	FILE* f=fopen(filename,"rb");
	int r=fread(&Header, 18, 1, f);
	int offset=get_int_from_buffer(Header,0,1)+18;
	int tga_type=get_int_from_buffer(Header,2,1);
	//msg_write(tga_type);
	NixImage.width=get_int_from_buffer(Header,12,2);	if (NixImage.width<0)	NixImage.width=-NixImage.width;
	NixImage.height=get_int_from_buffer(Header,14,2);	if (NixImage.height<0)	NixImage.height=-NixImage.height;
	int depth=get_int_from_buffer(Header,16,1);
	int alpha_bits=get_int_from_buffer(Header,17,1);
	NixImage.alpha_used=alpha_bits>0;
	bool compressed=((tga_type&8)>0);
	/*if (compressed)	msg_write("komprimiert");
	else			msg_write("unkomprimiert");
	msg_write(string("Breite: ",i2s(NixImage.width)));
	msg_write(string("Hoehe: ",i2s(NixImage.height)));
	msg_write(string("Farbtiefe: ",i2s(depth)));
	msg_write(string("Alpha-Bits: ",i2s(alpha_bits)));*/
	int ss=4*NixImage.width*NixImage.height+1024;
	//			msg_write(ss);
	NixImage.data=new unsigned char[ss];
	int size=NixImage.width*NixImage.height,bpp=depth/8;

	fseek(f,offset,SEEK_SET);
	
	if (depth<16){
		pal=new unsigned char[3*256];
		r=fread(pal,3,256,f);
	}

	if (compressed){
		int currentpixel=0,currentbyte=0;
		unsigned char colorbuffer[4];
		switch(depth){
			case 32:
			case 24:
			case 16:
			case 8:
				do{
					unsigned char chunkheader = 0;
					r=fread(&chunkheader, 1, 1, f);
					if ((chunkheader & 0x80) != 0x80){
						chunkheader++;
						//msg_write(string("raw ",i2s(chunkheader)));
						while (chunkheader-- > 0){
							r=fread(colorbuffer, 1, bpp, f);
							nix_fill_image_color((sNixImageColor*)(NixImage.data+currentbyte),colorbuffer,pal,depth,alpha_bits,true);
							currentbyte += 4;
							currentpixel++;
						}
					}else{
						chunkheader = (unsigned char)((chunkheader & 0x7F) + 1);
						//msg_write(string("rle ",i2s(chunkheader)));
						r=fread(colorbuffer, 1, bpp, f);
						while(chunkheader-- > 0){
							nix_fill_image_color((sNixImageColor*)(NixImage.data+currentbyte),colorbuffer,pal,depth,alpha_bits,true);
							currentbyte += 4;
							currentpixel++;
						}
					}
				}while(currentpixel < size);
				break;
			default:
				msg_error(string("NixLoadTGA: unsupported color depth (compressed): ",i2s(depth)));
				break;
		}
	}else{
		switch(depth){
			case 32:
			case 24:
			case 16:
			case 8:
				data=new unsigned char[bpp*size];
				r=fread(data,bpp,size,f);
				for (x=0;x<NixImage.width*NixImage.height;x++)
					nix_fill_image_color((sNixImageColor*)(NixImage.data+x*4),data+x*bpp,pal,depth,alpha_bits,true);
				break;
			default:
				msg_error(string("NixLoadTGA: unsupported color depth (uncompressed): ",i2s(depth)));
				break;
		}
	}
    delete[]data;
    fclose(f);
}



//--------------------------------------------------------------------------------------------------
// jpg files
//--------------------------------------------------------------------------------------------------


struct s_jpg_huffman_table
{
	unsigned char bits[16];
	unsigned char value[4096];
	int min[16],max[16];
};

static s_jpg_huffman_table jpg_htac[4],jpg_htdc[4];

static float jpg_qt[4][64];

static unsigned char jpg_zz[64]={
	 0, 1, 5, 6,14,15,27,28,
	 2, 4, 7,13,16,26,29,42,
	 3, 8,12,17,25,30,41,43,
	 9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
};

struct s_jpg_color_info
{
	int ac[3],dc[3]; // huffman table ac/dc
	int v[3],h[3]; // vertical/horizontal sampling
	int q[3]; // quantization table
};

static int jpg_temp_bits=0;

static float jpg_cos_table[8][8];

int jpg_load_huffman_table(unsigned char *b,s_jpg_huffman_table *h)
{
	// length
	for (int i=0;i<16;i++)
		h->bits[i]=(*(b++));

	// values
	int nv=0;
	for (int i=0;i<16;i++){
		for (int j=0;j<h->bits[i];j++){
			//msg_write(i*256+j);
			//msg_write( h->value[i*256+j]=(*(b++)) );
			h->value[i*256+j]=(*(b++));
			nv++;
		}
	}

	// pre process for searching faster
	int t=0;
	for (int i=0;i<16;i++){
		h->min[i]=(h->bits[i]==0)?0xffff:t;
		t+=h->bits[i];
		h->max[i]=t-1;
		//msg_write(string2("min=%d  max=%d",h->min[i],h->max[i]));
		t*=2;
	}
	return nv+16;
}

int jpg_load_quantization_table(unsigned char *b,float *q)
{
	for (int i=0;i<64;i++)
		q[i]= (float)b[jpg_zz[i]];
	return 64;
}

inline int jpg_get_bits(int bits,bool sign)
{
	if (sign)
		if ( (jpg_temp_bits & 0x8000) == 0 )
			return (jpg_temp_bits>>(16-bits)) + 1 + (0xffffffff << bits);
	return jpg_temp_bits>>(16-bits);
}

inline void jpg_update_bits(unsigned char *&b,int &bit_off)
{
	// correct bit shifting over byte boundaries
	if (bit_off>=16){
		bit_off-=16;
		if ( ((b[-1]==0xff)&&(b[0]==0x00)) || ((b[0]==0xff)&&(b[1]==0x00)) ){
			b+=3;
			//msg_write("+=3     (0xff00) 2");
		}else{
			b+=2;
			//msg_write("+=2");
		}
	}else if (bit_off>=8){
		bit_off-=8;
		if ((b[-1]==0xff)&&(b[0]==0x00)){
			b+=2;
			//msg_write("+=2     (0xff00) 1");
		}else{
			b++;
			//msg_write("+=1");
		}
	}
	// convert 0xff00 to 0xff  m(T_T)m
	unsigned char *a=b,b0,b1,b2;
	if ((a[-1]==0xff)&&(a[0]==0x00))
		a++;
	b0=a[0];
	if ((a[0]==0xff)&&(a[1]==0x00))
		a++;
	b1=a[1];
	if ((a[1]==0xff)&&(a[2]==0x00))
		a++;
	b2=a[2];
	// join...but in "wrong" order!  m(T_T)m
	jpg_temp_bits=( ( ( (b0<<16) + (b1<<8) + b2) << bit_off ) >> 8 ) & 0x0000ffff;
	//msg_write(d2h(&jpg_temp_bits,3,true));
}

inline unsigned char jpg_get_huffman(s_jpg_huffman_table *h,unsigned char *&b,int &bit_off)
{
	jpg_update_bits(b,bit_off);
	//msg_write(d2h(&jpg_temp_bits,3,true));
	for (int i=0;i<16;i++){
		int c=jpg_get_bits(i+1,false);
		if ((c>=h->min[i])&&(c<=h->max[i])){
			//msg_write(i*256+(c-h->min[i]));
			//msg_write(string2("gefunden!   ii=%d   i=%d   c=%d   min=%d   v=%d",htii,i,c,h->min[i],h->value[i*256+(c-h->min[i])]));
			bit_off+=i+1;
			return h->value[i*256+(c-h->min[i])];
		}
	}
	msg_write("huffman :~~(");
	return 0;
}

void jpg_decode_huffman(int ac,int dc,unsigned char *&b,int &bit_off,int &prev,int *coeff)
{
	//msg_write("huffman");
	int coeff2[80];
	memset(coeff2,0,4*64);

	unsigned char *b0=b;

	// "dc"
	int size=jpg_get_huffman(&jpg_htdc[dc],b,bit_off);
	//msg_write(size);
	jpg_update_bits(b,bit_off);
	coeff2[0]=jpg_get_bits(size,true)+prev;
	prev=coeff2[0];
	//msg_write(coeff2[0]);
	bit_off+=size;

	// "ac"
	//msg_write("ac");
	for (int i=1;i<64;i++){
		unsigned char v=jpg_get_huffman(&jpg_htac[ac],b,bit_off);
	//msg_write(v);
		int size=(v & 0x0f);
		int n=(v >> 4);
		if (size==0){
			if (n==0)
				break;
			if (n==0x0f)
				i+=15;
		}else{
			i+=n; // zeroes!
			jpg_update_bits(b,bit_off);
			//msg_write(i);
			coeff2[i]=jpg_get_bits(size,true);
			//msg_write(coeff2[i]);
			bit_off+=size;
		}
	}
	//msg_write("zzz1");
	for (int i=0;i<64;i++)
		coeff[i]=coeff2[jpg_zz[i]];
	//msg_write("zzz2");
}

inline void jpg_cosine_retransform(int *coeff_in,int q,float *coeff_out)
{
	// de-quantizing....
	float coeff_in_f[64];
	float *qt=jpg_qt[q];
	for (int i=0;i<64;i++)
		coeff_in_f[i]=(float)coeff_in[i]*qt[i];

	float inv_sqrt2=1.0f/sqrt(2.0f);

	// cos transformation
	for (int x=0;x<8;x++)
		for (int y=0;y<8;y++){
			float f=0;
			for (int m=0;m<8;m++)
				for (int n=0;n<8;n++){
					float g=coeff_in_f[n*8+m]*jpg_cos_table[x][m]*jpg_cos_table[y][n];
					if (m==0)	g*=inv_sqrt2;
					if (n==0)	g*=inv_sqrt2;
					f+=g;
				}
			coeff_out[y*8+x]=0.25f*f;
		}
}

// combine some 8x8 blocks into a meta block (using sub sampling)
inline void jpg_combine_blocks(float *block,int &sx,int &sy,int &sh,int &sv,int &i,int &j,float *col)
{
	int fh=sx/sh/8;
	int fv=sy/sv/8;
	for (int x=0;x<8;x++){
		int x0=(x+i*8)*fh;
		for (int y=0;y<8;y++){
			float c=block[y*8+x];
			int y0=(y+j*8)*fv;
			// sub sampling
			for (int yy=0;yy<fv;yy++){
				float *tcol=&col[(y0+yy)*sx + x0];
				for (int xx=0;xx<fh;xx++){
					*tcol=c;
					tcol++;
				}
			}
		}
	}
}

#define _color_clamp_(c)\
	( (c<0.0f) ? 0.0f : ( (c>255.0f) ? 255.0f : c ) )

inline void jpg_insert_into_image(float *col0,float *col1,float *col2,int &sx,int &sy,int &x,int &y)
{
	for (int i=0;i<sx;i++){
		// "real position" in the image
		int _y=NixImage.height-(i+sy*y+1);
		int _x0=sx*x;
		unsigned char *d=&NixImage.data[( _y*NixImage.width + _x0 )*4];
		for (int j=0;j<sy;j++){
			// within "real image"?
			if (_x0+j<NixImage.width)
				if (_y>=0){
					// retransform color space YCbCr -> RGB
					col0[0]+=128.0f;
					float r=_color_clamp_(*col0                     + 1.402f    * *col2);
					float g=_color_clamp_(*col0 - 0.344136f * *col1 - 0.714136f * *col2);
					float b=_color_clamp_(*col0 + 1.772f    * *col1                    );
					// insert
					*(d++)=(int)r;
					*(d++)=(int)g;
					*(d++)=(int)b;
					d++;
				}
			col0++;
			col1++;
			col2++;
		}
	}
}

#define _max(a,b)	( (a>b) ? a  : b )

void jpg_decode(unsigned char *b,s_jpg_color_info ci)
{
	unsigned char *a=b;
	//msg_write("start decode!");
	// size of meta blocks
	int sx=_max(ci.h[0],_max(ci.h[1],ci.h[2]))*8;
	int sy=_max(ci.v[0],_max(ci.v[1],ci.v[2]))*8;
	//msg_write(string2("%d x %d",sx,sy));
	// number of meta blocks in the image
	int nx=(NixImage.width +(sx-1))/sx;
	int ny=(NixImage.height+(sy-1))/sy;
	//msg_write(string2("%d x %d",nx,ny));
	// image dimensions in file (including "nonsense")
	int iw=nx*sx;
	int ih=ny*sy;

	// temporary data
	int coeff[64]; // block
	float col_block[64];
	float *col[3]; // meta block
	col[0]=new float[sx*sy];
	col[1]=new float[sx*sy];
	col[2]=new float[sx*sy];
	NixImage.data=new unsigned char[NixImage.width*NixImage.height*4];

	for (int i=0;i<8;i++)
		for (int j=0;j<8;j++)
			jpg_cos_table[i][j]=(float)cos( (2*i+1)*j*pi/16.0f );


/*CFile *f=new CFile();
f->Create("test");
f->SetBinaryMode(true);*/

	// read data
	int bit_off=0;
	int prev[3]={0,0,0};
	// meta blocks
	for (int y=0;y<ny;y++)
		for (int x=0;x<nx;x++){
			// 3 colors
			for (int c=0;c<3;c++){
				// sub blocks
				for (int i=0;i<ci.h[c];i++)
					for (int j=0;j<ci.v[c];j++){
						jpg_decode_huffman(ci.ac[c],ci.dc[c],b,bit_off,prev[c],coeff);
						jpg_cosine_retransform(coeff,ci.q[c],col_block);
						jpg_combine_blocks(col_block,sx,sy,ci.h[c],ci.v[c],j,i,col[c]);
					}
			}
			jpg_insert_into_image(col[0],col[1],col[2],sx,sy,x,y);
		}

		//f->WriteBuffer(image.data,image.width*image.height*3);

/*f->Close();
delete(f);*/

	delete[](col[0]);
	delete[](col[1]);
	delete[](col[2]);
}

void NixLoadJpg(char *filename)
{
	CFile *f=new CFile();
	f->SilentFileAccess=true;
	if (!f->Open(filename)){
		delete(f);
		return;
	}
	f->SetBinaryMode(true);
	int buf_len=f->GetSize();
	unsigned char *buf=new unsigned char[buf_len+128];
	f->ReadComplete(buf,buf_len);
	f->Close();
	delete(f);

	unsigned char *b=buf;

	NixImage.width=-1;
	NixImage.height=-1;
	NixImage.data=NULL;
	NixImage.alpha_used=false;
	NixImage.error=false;

	s_jpg_color_info ci;

	while(true){

		// read segments
		int seg_len=0;
		//msg_write("-------------------------");
		//msg_write(string2("%s (off:%d)",d2h(b,2,false),(int)b-(int)buf));
		if (b[0]==0xff){
			if (b[1]==0xd8){		// StartOfImage
				b+=2;
				//msg_write("SOI");
			}else if (b[1]==0xd9){	// EndOfImage
				//msg_write("EOI");
				break;
			}else if (b[1]==0xe0){	// JFIF-Tag
				//msg_write("APP0");
				seg_len=b[2]*256+b[3]-2;
				b+=4;
				b+=seg_len;
			}else if (b[1]==0xdb){	// QuantifyingTable
				//msg_write("DQT");
				seg_len=b[2]*256+b[3]-2;
				b+=4;
				int t=0;
				while(t<seg_len){
					int nt=(b[t] & 0x0f); // table index
					t+=jpg_load_quantization_table(&b[t+1],jpg_qt[nt])+1;
				}
				b+=seg_len;
			}else if (b[1]==0xc0){	// dimensions/color sampling
				//msg_write("SOF0");
				seg_len=b[2]*256+b[3]-2;
				b+=4;
				int bpp=b[0];
				NixImage.height=b[1]*256+b[2];
				NixImage.width=b[3]*256+b[4];
				//msg_write(string2("jpg: %d x %d, depth=%d",NixImage.width,NixImage.height,bpp*3));
				if (bpp!=8){	msg_error("jpg: depth!=24 unsupported!");	break;	}
				NixImage.data=new unsigned char[4*NixImage.width*NixImage.height];
				int nc=b[5];
				if (nc!=3){
					msg_error("jpg: number of colors != 3 (unsupported)");
					break;
				}
				for (int i=0;i<3;i++){
					int c=b[i*3+6];
					ci.h[c-1]=b[i*3+7]>>4;
					ci.v[c-1]=b[i*3+7]&0x0f;
					ci.q[c-1]=b[i*3+8];
				}
				b+=seg_len;
			}else if (b[1]==0xc4){	// HuffmanTable
				//msg_write("DHT");
				seg_len=b[2]*256+b[3]-2;
				b+=4;
				int t=0;
				while(t<seg_len){
					int nt=(b[t] & 0x0f); // table index
					//msg_write(string2("n=%d , %s",nt,( (b[t] & 0x10) > 0 )?"ac":"dc"));
					if ( (b[t] & 0x10) > 0 ) // ac/dc?
						t+=jpg_load_huffman_table(&b[t+1],&jpg_htac[nt])+1;
					else
						t+=jpg_load_huffman_table(&b[t+1],&jpg_htdc[nt])+1;
				}
				b+=seg_len;
			}else if (b[1]==0xda){	// StartOfScan
				//msg_write("SOS");
				b+=4;
				while((b[seg_len]!=0xff)||(b[seg_len+1]==0x00)){
					seg_len++;
				}
				seg_len-=10;
				int nc=*(b++);
				if (nc!=3){
					msg_error("jpg: number of colors != 3 (unsupported)");
					break;
				}
				for (int i=0;i<3;i++){
					int c=*(b++);
					ci.ac[c-1]=b[0]>>4;
					ci.dc[c-1]=*(b++)&0x0f;
				}
				b+=3;
				jpg_decode(b,ci);
				b+=seg_len;
			}else{
				//msg_error("jpg: unknown seg");
				seg_len=b[2]*256+b[3]-2;
				b+=4;
				//msg_write(seg_len);
				b+=seg_len;
			}
			if ((long)b-(long)buf>buf_len){
				msg_error("jpg: end of file");
				break;
			}
		}else{
			msg_error("jpg: broken");
			break;
		}
	}


	delete[](buf);
}


//--------------------------------------------------------------------------------------------------
// png files
//--------------------------------------------------------------------------------------------------


void NixLoadPng(char *filename)
{
	CFile *f=new CFile();
	f->SilentFileAccess=true;
	if (!f->Open(filename)){
		delete(f);
		return;
	}
	f->SetBinaryMode(true);
	int buf_len=f->GetSize();
	unsigned char *buf=new unsigned char[buf_len+128];
	f->ReadComplete(buf,buf_len);
	f->Close();
	delete(f);

	unsigned char *b=buf;

	NixImage.width=-1;
	NixImage.height=-1;
	NixImage.data=NULL;
	NixImage.alpha_used=false;
	NixImage.error=false;

	// header
	b+=8;

	while(true){
		// chunks
		int csize=(b[0]<<24)+(b[1]<<16)+(b[2]<<8)+(b[3]);
		msg_write(csize);
		msg_write((char*)&b[4],4);
		b+=8+csize+4;
		if ((long)b-(long)buf>=buf_len)
			break;
	}


	delete[](buf);
}

//#endif

//--------------------------------------------------------------------------------------------------
// common stuff
//--------------------------------------------------------------------------------------------------

void NixTexturesInit()
{
	NumTextures=0;
	NumCubeMaps=0;
	#ifdef NIX_ALLOW_VIDEO_TEXTURE
		// allow AVI textures
		AVIFileInit();
	#endif
}

void NixReleaseTextures()
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		for (int i=0;i<NumTextures;i++)
			if (DXTexture[i]){
				DXTexture[i]->Release();
				DXTexture[i]=NULL;
			}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		for (int i=0;i<NumTextures;i++){
			glBindTexture(GL_TEXTURE_2D,OGLTexture[i]);
			glDeleteTextures(1,(unsigned int*)&OGLTexture[i]);
		}
	}
#endif
}

void NixReincarnateTextures()
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		for (int i=0;i<NumTextures;i++)
			NixReloadTexture(i);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		for (int i=0;i<NumTextures;i++){
			glGenTextures(1,&OGLTexture[i]);
			NixReloadTexture(i);
		}
	}
#endif
}

void NixProgressTextureLifes()
{
#ifdef NIX_API_DIRECTX9
	for (int i=0;i<NumTextures;i++)
		if (TextureLifeTime[i]>=0){
			TextureLifeTime[i]++;
			if (TextureLifeTime[i]>=NixTextureMaxFramesToLive)
				NixUnloadTexture(i);
		}
#endif
}

void NixSetShaderTexturesDX(void *fx,int texture0,int texture1,int texture2,int texture3)
{
#ifdef NIX_API_DIRECTX9
	D3DXHANDLE hdl;
	if (texture0>=0)
		if (hdl=((LPD3DXEFFECT)fx)->GetParameterByName(NULL,"tex0"))
			((LPD3DXEFFECT)fx)->SetTexture(hdl,DXTexture[texture0]);
	if (texture1>=0)
		if (hdl=((LPD3DXEFFECT)fx)->GetParameterByName(NULL,"tex1"))
			((LPD3DXEFFECT)fx)->SetTexture(hdl,DXTexture[texture1]);
	if (texture2>=0)
		if (hdl=((LPD3DXEFFECT)fx)->GetParameterByName(NULL,"tex2"))
			((LPD3DXEFFECT)fx)->SetTexture(hdl,DXTexture[texture2]);
	if (texture3>=0)
		if (hdl=((LPD3DXEFFECT)fx)->GetParameterByName(NULL,"tex3"))
			((LPD3DXEFFECT)fx)->SetTexture(hdl,DXTexture[texture3]);
#endif
}

int NixLoadTexture(char *filename)
{
//	msg_write("NixLoadTexture");
	char _filename[512];
	strcpy(_filename,filename);

//	msg_write("teste");
	// test existence
	if (!file_test_existence(_filename)){
		msg_error(string("texture file does not exist: ",_filename));
		return -1;
	}
//msg_write("ok....");
	strcpy(TextureFile[NumTextures],_filename);
	TextureIsDynamic[NumTextures]=false;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
//		msg_write("reload....");
		NixReloadTexture(NumTextures);
		if (DXTexture[NumTextures]==NULL)
			return -1;
//		msg_write("ok2");
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
#ifdef NIX_ALLOW_VIDEO_TEXTURE
	avi_info[NumTextures]=NULL;
#endif
//	msg_write("ok");
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
	int h=_open(SysFileName(TextureFile[texture]),0);
	if (h<0){
		msg_error("texture file does not exist!");
		msg_left();
		return;
	}
	_close(h);

	#ifdef NIX_ALLOW_VIDEO_TEXTURE
		avi_info[texture]=NULL;
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
				avi_info[texture]=new s_avi_info;
				if (!avi_open(texture,TextureFile[texture])){
					avi_info[texture]=NULL;
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
											sys_str_f(TextureFile[texture]),
											0,0, // overwrite size
											5, // mip map levels
											0, // usage: 0=default
											D3DFMT_UNKNOWN,
											D3DPOOL_DEFAULT,
											D3DX_DEFAULT,
											D3DX_DEFAULT,
											0xFF00FF00, // green as key color
											&SrcInfo,
											NULL, // no palette
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

		char extension[256];
		strcpy(extension,"");
		int pie=-1;
		for (unsigned int i=0;i<strlen(TextureFile[texture]);i++){
			if (pie>=0){
				extension[pie]=TextureFile[texture][i];
				extension[pie+1]=0;
				pie++;
			}
			if (TextureFile[texture][i]=='.')
				pie=0;
		}
		if (strlen(extension)<1){
			msg_error("texture file extension missing!");
			msg_left();
			return;
		}

	// AVI
		if ((strcmp(extension,"avi")==0)||(strcmp(extension,"AVI")==0)){
			//msg_write("avi");
			#ifdef NIX_ALLOW_VIDEO_TEXTURE
				avi_info[texture]=new s_avi_info;

				glEnable(GL_TEXTURE_2D);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
				if (!avi_open(texture,SysFileName(TextureFile[texture]))){
					avi_info[texture]=NULL;
					msg_left();
					return;
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, NixTextureWidth[texture], NixTextureHeight[texture], 0, GL_RGB, GL_UNSIGNED_BYTE, avi_info[texture]->data);
			#else
				msg_error("Support for video textures is not activated!!!");
				msg_write("-> un-comment the NIX_ALLOW_VIDEO_TEXTURE definition in the source file \"00_config.h\" and recompile the program");
				msg_left();
				return;
			#endif
		}else{
			NixImage.data=NULL;
			if ((strcmp(extension,"bmp")==0)||(strcmp(extension,"BMP")==0))
				NixLoadBmp(SysFileName(TextureFile[texture]));
			else if ((strcmp(extension,"tga")==0)||(strcmp(extension,"TGA")==0))
				NixLoadTga(SysFileName(TextureFile[texture]));
			else if ((strcmp(extension,"jpg")==0)||(strcmp(extension,"JPG")==0))
				NixLoadJpg(SysFileName(TextureFile[texture]));
			else if ((strcmp(extension,"png")==0)||(strcmp(extension,"PNG")==0))
				NixLoadPng(SysFileName(TextureFile[texture]));
			else 
				msg_error(string("unknown extension: ",extension));

			if (NixImage.data){
				glBindTexture(GL_TEXTURE_2D, OGLTexture[texture]);
				if (NixImage.alpha_used)
					gluBuild2DMipmaps(GL_TEXTURE_2D,4,NixImage.width,NixImage.height,GL_RGBA,GL_UNSIGNED_BYTE,NixImage.data);
				else
					gluBuild2DMipmaps(GL_TEXTURE_2D,3,NixImage.width,NixImage.height,GL_RGBA,GL_UNSIGNED_BYTE,NixImage.data);
	//msg_todo("32 bit textures for OpenGL");
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
		if (TextureLifeTime[texture]<0)
			NixReloadTexture(texture);
		TextureLifeTime[texture]=0;
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
		if (texture>=0){
			lpDevice->SetTexture(0,DXTexture[texture]);
			lpDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
			lpDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			//lpDevice->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT);
			lpDevice->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
		}else if (texture==-1){
			lpDevice->SetTexture(0,NULL);
			lpDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
			lpDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
		}
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

void NixSetTextures(int *texture,int num_textures)
{
	for (int i=0;i<num_textures;i++)
		if (texture[i]>=0){
			if (TextureLifeTime[texture[i]]<0)
				NixReloadTexture(texture[i]);
			TextureLifeTime[texture[i]]=0;
		}else
			break;

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		for (int i=0;i<num_textures;i++)
			if (texture[i]>=0)
				lpDevice->SetTexture(0,DXTexture[texture[i]]);
			else{
				lpDevice->SetTexture(0,NULL);
				break;
			}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (texture>=0){
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,OGLTexture[texture[0]]);
			/*if (TextureIsDynamic[texture[0]]){
				#ifdef NIX_OS_WONDOWS
				#endif
			}*/
		}else
			glDisable(GL_TEXTURE_2D);
	}
#endif
}

void NixSetCubeMapDX(int texture)
{
#ifdef NIX_API_DIRECTX9
	lpDevice->SetTexture(0,DXCubeMap[texture]);
	lpDevice->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
	lpDevice->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
#endif
}

void NixRenderToTextureBeginDX(int texture)
{
#ifdef NIX_API_DIRECTX9
	HRESULT hr=DXTextureRenderTarget[texture]->BeginScene(DXTextureSurface[texture],NULL);
	if (FAILED(hr))
		msg_error(string("RenderToSurface-BeginScene: ",DXErrorMsg(hr)));
#endif
}

void NixRenderToTextureEndDX(int texture)
{
#ifdef NIX_API_DIRECTX9
	DXTextureRenderTarget[texture]->EndScene(0);
#endif
}

// mode: rgba
//    = r + g<<8 + b<<16 + a<<24
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
		char extension[256];
		strcpy(extension, file_extension(filename));
		if (strlen(extension)<1){
			msg_error("texture file extension missing!");
			msg_left();
			return;
		}
		NixImage.data=NULL;
		if (strcmp(extension,"bmp")==0)
			NixLoadBmp(SysFileName(filename));
		else if (strcmp(extension,"tga")==0)
			NixLoadTga(SysFileName(filename));
		else if (strcmp(extension,"jpg")==0)
			NixLoadJpg(SysFileName(filename));
		else 
			msg_error(string("unknown extension: ",extension));

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
	if (texture<0)
		return;
	if (!avi_info[texture])
		return;
	if (frame>avi_info[texture]->lastframe)
		frame=0;
	avi_grab_frame(texture,frame);
	avi_info[texture]->time=float(frame)/avi_info[texture]->fps;
#endif
}

void NixTextureVideoMove(int texture,float elapsed)
{
#ifdef NIX_ALLOW_VIDEO_TEXTURE
	if (texture<0)
		return;
	if (!avi_info[texture])
		return;
	msg_write("<NixTextureVideoMove>");
	avi_info[texture]->time+=elapsed;
	if (avi_info[texture]->time*avi_info[texture]->fps>avi_info[texture]->lastframe)
		avi_info[texture]->time=0;
	avi_grab_frame(texture,int(avi_info[texture]->time*avi_info[texture]->fps));
	msg_write("</NixTextureVideoMove>");
#endif
}

// encode tga files via run-length-encoding
//#define SAVE_TGA_RLE

// data must be   r-g-b-a
// 1 unsigned char per color channel  ( r8 g8 b8 a8 )
// first pixel is top left, the second one leads right
// tga-formats:
//    bits=32	alpha_bits=?	->	a8r8g8b8
//    bits=24	alpha_bits=?	->	a0r8g8b8
//    bits=16	alpha_bits=0	->	a0r5g5b5
//    bits=16	alpha_bits=1	->	a1r5g5b5
//    bits=16	alpha_bits=4	->	a4r4g4b4
static unsigned char Header[18];
void NixSaveTGA(char *filename,int width,int height,int bits,int alpha_bits,void *data,void *palette)
{
	FILE* f=fopen(SysFileName(filename),"wb");
	if (!f){
		msg_error(string("couldn't save tga file: ",SysFileName(filename)));
		return;
	}
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
	int rw=fwrite(&Header, 18, 1, f);
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
				rw=fwrite(&rle_header, 1, 1, f);
			}
			msg_write(*(int*)&_data[offset]);
			rle_num--;
			if ((rle_even)&&(rle_num>=0))
				continue;
#endif
			r=_data[offset  ];
			g=_data[offset+1];
			b=_data[offset+2];
			a=_data[offset+3];
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
			rw=fwrite(&color, bits/8, 1, f);
			//msg_write(color);
		}
    fclose(f);
}

int NixCreateDynamicTexture(int width,int height)
{
	msg_db_r("creating dynamic texture", 0);
	msg_write(string("[ ",i2s(width)," x ",i2s(height)," ]"));
#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
	strcpy(TextureFile[NumTextures],"-dynamic Texture-");
	NixTextureWidth[NumTextures]=width;
	NixTextureHeight[NumTextures]=height;
	TextureIsDynamic[NumTextures]=true;
#ifdef NIX_ALLOW_VIDEO_TEXTURE
	avi_info[NumTextures]=NULL;
#endif
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
			msg_db_l(0);
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
			msg_db_l(0);
			return -1;
		}
		
		// clear texture
		NixStart(NumTextures);
		color c=color(0,0,0,0);
		NixDraw2D(-1,&c,NULL,NULL,0);
		NixEnd();

		NumTextures++;
		msg_db_l(0);
		return NumTextures-1;
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (!OGLDynamicTextureSupport)	return -1;
		// create the render target stuff
		glGenFramebuffersEXT(1,&OGLFrameBuffer[NumTextures]);
		glGenRenderbuffersEXT(1,&OGLDepthRenderBuffer[NumTextures]);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,OGLDepthRenderBuffer[NumTextures]);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT24,width, height);

		// create the actual (dynamic) texture
		glGenTextures(1,&OGLTexture[NumTextures]);
		glBindTexture(GL_TEXTURE_2D,OGLTexture[NumTextures]);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		
		// clear texture
/*		NixStart(NumTextures);
		color c=color(0,0,0,0);
		NixDraw2D(-1,&c,NULL,NULL,0);
		NixEnd();*/

		NumTextures++;
		msg_db_l(0);
		return NumTextures-1;
	}
#endif

#else
	msg_error("Support for dynamic textures is not activated!!!");
	msg_write("-> uncomment the NIX_ALLOW_DYNAMIC_TEXTURE definition in the source file \"00_config.h\" and recompile the program");
#endif
	msg_db_l(0);
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
		MatrixMultiply(NixViewMatrix,r,t);
		lpDevice->SetTransform(D3DTS_VIEW,(D3DXMATRIX*)&NixViewMatrix);
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
		matrix vm=NixViewMatrix;

		hr=DXRenderToEnvMap[cube_map]->BeginCube(DXCubeMap[cube_map]);
		if (hr!=D3D_OK){
			msg_error(string("DXRenderToEnvMap: ",DXErrorMsg(hr)));
			return;
		}
		D3DXMatrixPerspectiveFovLH((D3DXMATRIX*)&NixProjectionMatrix,pi/2,1,NixMinDepth,NixMaxDepth);
		lpDevice->SetTransform(D3DTS_PROJECTION,(D3DXMATRIX*)&NixProjectionMatrix);
		MatrixInverse(NixInvProjectionMatrix,NixProjectionMatrix);
		NixTargetWidth=NixTargetHeight=CubeMapSize[cube_map];
		DXViewPort.X=DXViewPort.Y=0;
		DXViewPort.Width=DXViewPort.Height=CubeMapSize[cube_map];
		lpDevice->SetViewport(&DXViewPort);

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
			SetCubeMatrix(pos,v0);
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

		NixViewMatrix=vm;
		NixSetView(true,NixViewMatrix);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("RenderToCubeMap for OpenGL");
	}
#endif
}
