/*----------------------------------------------------------------------------*\
| Nix config                                                                   |
| -> configuration for nix                                                     |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_CONFIG_EXISTS_
#define _NIX_CONFIG_EXISTS_

#include "../00_config.h"

//#define NIX_ALLOW_API_DIRECTX9			1

// which operating system?

#ifdef _WIN32
	#define NIX_OS_WINDOWS
#else
	#define NIX_OS_LINUX
#endif



// which developing environment?

#ifdef _MSC_VER
	#define NIX_IDE_VCS
	#if _MSC_VER >= 1400
		#define NIX_IDE_VCS8
	#else
		#define NIX_IDE_VCS6
	#endif
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



// sound handled by DirectX9?

#ifdef NIX_ALLOW_SOUND
	#ifdef NIX_API_DIRECTX9
		#ifdef NIX_ALLOW_SOUND_BY_DIRECTX9
			#define NIX_SOUND_DIRECTX9
		#endif
	#endif
#endif


#ifndef NIX_OS_WINDOWS
	//#undef NIX_ALLOW_DYNAMIC_TEXTURE
	#undef NIX_ALLOW_VIDEO_TEXTURE
	#undef NIX_ALLOW_API_DIRECTX9
	#undef NIX_ALLOW_TYPES_BY_DIRECTX9
	#undef NIX_ALLOW_SOUND_BY_DIRECTX9
#endif


#include <math.h>
#include "../hui/hui.h"
#include "../file/msg.h"


#define NIX_MAX_VBS				4096
#define NIX_MAX_TEXTURES		128
#define NIX_MAX_CUBEMAPS		16
#define NIX_MAX_LIGHTS			128
#define NIX_MAX_SHADERFILES		32
#define NIX_MAX_SOUNDS			128

typedef void callback_function();



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


extern int NixFontHeight;
extern char NixFontName[128];

extern CHuiWindow *NixWindow;

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

#endif
