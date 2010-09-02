/*----------------------------------------------------------------------------*\
| Hui config                                                                   |
| -> configuration for hui                                                     |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2009.12.05 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _HUI_CONFIG_EXISTS_
#define _HUI_CONFIG_EXISTS_


#include "../00_config.h"

// which operating system?

#ifdef WIN32
	#define HUI_OS_WINDOWS
#else
	#define HUI_OS_LINUX
#endif



#ifdef HUI_OS_WINDOWS
	#ifdef HUI_USE_GTK_ON_WINDOWS
		#define HUI_API_GTK
	#else
		#define HUI_API_WIN
	#endif
#else
	#define HUI_API_GTK
#endif



#ifdef HUI_OS_WINDOWS
	#ifndef _WIN32_WINDOWS
		#ifndef _WIN32_WINDOWS
			#define _WIN32_WINDOWS 0x500
		#endif
		#ifndef _WIN32_WINNT
			#define _WIN32_WINNT 0x0500
		#endif
	#endif
	#include <windows.h>
#endif
#ifdef HUI_API_GTK
	#include <gtk/gtk.h>
	#include <gdk/gdkkeysyms.h>
#endif
#ifdef HUI_OS_LINUX
	#define _cdecl
#endif



#include <vector>
#include <string>

typedef void message_function(int);
typedef void void_function();



#define HUI_MAX_KEYBUFFER_DEPTH			128

enum{
	HUI_YES=20,
	HUI_NO,
	HUI_CANCEL
};

enum{
	HuiImageNew,
	HuiImageOpen,
	HuiImageSave,
	HuiImageSaveAs,
	HuiImageClose,
	HuiImageQuit,
	HuiImageCopy,
	HuiImagePaste,
	HuiImageCut,
	HuiImageDelete,
	HuiImageFind,
	HuiImageEdit,
	HuiImageUndo,
	HuiImageRedo,
	HuiImageRefresh,
	HuiImageYes,
	HuiImageNo,
	HuiImageOk,
	HuiImageCancel,
	HuiImageApply,
	HuiImagePreferences,
	HuiImageClear,
	HuiImageAdd,
	HuiImageExecute,
	HuiImageFullscreen,
	HuiImageUp,
	HuiImageDown,
	HuiImageBack,
	HuiImageForward,
	HuiImageHelp,
	HuiImageInfo,
	HuiImagePrint,
	HuiImageRemove,
	HuiImageFont,
	HuiImageSelectAll,
	HuiImageStop,
	HuiImageZoomIn,
	HuiImageZoomOut,
	HuiImageZoomOne,
	HuiImageZoomFit,
	HuiImageProperties,
	HuiImageMediaPlay,
	HuiImageMediaStop,
	HuiImageMediaPause,
	HuiImageMediaRecord,
	HuiImageAbout,
};


#endif
