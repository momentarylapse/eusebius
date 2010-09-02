/*----------------------------------------------------------------------------*\
| CHui                                                                         |
| -> Heroic User Interface                                                     |
| -> abstraction layer for GUI                                                 |
|   -> Windows (default api) or Linux (Gtk+)                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2009.12.05 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/

#ifndef _HUI_EXISTS_
#define _HUI_EXISTS_

#include "hui_config.h"

extern char HuiVersion[32];




#ifdef HUI_OS_WINDOWS
	extern HINSTANCE hui_win_instance;
#endif
#ifdef HUI_API_WIN
	extern unsigned char HuiKeyID[256];
	extern HFONT hui_win_default_font;
	extern HICON hui_win_main_icon;
#else
	extern int HuiKeyID[256];
	extern void *invisible_cursor;
	extern const char *get_stock_id(int image);
	extern void *get_gtk_image(int image,bool large);
	extern void *_hui_x_display_;
	#define hui_x_display (Display*)_hui_x_display_
#endif
extern int allow_signal_level;


class CHuiWindow;
class CHuiMenu;


//----------------------------------------------------------------------------------
// string conversion

#ifdef HUI_OS_WINDOWS
	extern const TCHAR *hui_tchar_str(const char *str);
	extern const TCHAR *hui_tchar_str_f(const char *str);
	extern const char *hui_de_tchar_str(const TCHAR *str);
	extern const char *hui_de_tchar_str_f(const TCHAR *str);
	#ifdef UNICODE
		#define win_str LPWSTR
	#else
		#define win_str LPSTR
	#endif
#endif
#ifdef HUI_API_WIN
	#define sys_str			hui_tchar_str
	#define sys_str_f		hui_tchar_str_f
	#define de_sys_str		hui_de_tchar_str
	#define de_sys_str_f	hui_de_tchar_str_f
#else
	extern const char *sys_str(const char *str);
	extern const char *sys_str_f(const char *str);
	extern const char *de_sys_str(const char *str);
	extern const char *de_sys_str_f(const char *str);
#endif
	extern const char *str_m2utf8(const char *str);
	extern const char *str_utf82m(const char *str);
	extern const char *str_ascii2m(const char *str);
	extern const char *str_m2ascii(const char *str);

	extern const char *get_lang(int id,const char *text,bool allow_keys=false);
#ifdef HUI_API_WIN
	extern const TCHAR *get_lang_sys(int id,const char *text,bool allow_keys=false);
#else
	extern const char *get_lang_sys(int id,const char *text,bool allow_keys=false);
#endif




//----------------------------------------------------------------------------------
// resource handling

CHuiWindow *_cdecl HuiCreateResourceDialog(int id,CHuiWindow *root,message_function *mf);
CHuiMenu *_cdecl HuiCreateResourceMenu(int id);

// resource commands (don't change the order!!!)
enum{
	HuiResMenu,
	HuiResDialog,
	HuiResToolBar,
};
enum{
	HuiCmdMenuAddItem,
	HuiCmdMenuAddItemImage,
	HuiCmdMenuAddItemCheckable,
	HuiCmdMenuAddItemSeparator,
	HuiCmdMenuAddItemPopup,
	HuiCmdDialogAddControl,
};
struct sHuiResourceCommand
{
	int type,id;
	bool b_param[2];
	int i_param[7];
};
struct sHuiResource: sHuiResourceCommand
{
	std::vector<sHuiResourceCommand> cmd;
};

// dialog controls (don't change the order!!!)
enum{
	HuiKindButton,
	HuiKindDefButton,
	HuiKindEdit,
	HuiKindText,
	HuiKindCheckBox,
	HuiKindGroup,
	HuiKindComboBox,
	HuiKindTabControl,
	HuiKindListView,
	HuiKindListViewTree,
	HuiKindListViewIcons,
	HuiKindProgressBar,
	HuiKindImage,
	HuiKindColorButton,
	HuiKindSlider
};



//----------------------------------------------------------------------------------
// system independence of main() function

#ifdef HUI_OS_WINDOWS
	extern void HuiSetArgs(char *arg);
#else
	extern void HuiSetArgs(int num_args,char *args[]);
#endif



// for a system independent usage of this library
#ifdef HUI_OS_WINDOWS
	#define hui_main	\
hui_main_win();\
int WINAPI WinMain(HINSTANCE hThisInstance,HINSTANCE hPrevInstance,LPSTR lpszArgument,int nFunsterStil)\
{\
	hui_win_instance=hThisInstance;\
	HuiSetArgs(lpszArgument);\
	return hui_main_win();\
}\
int hui_main_win
#endif
#ifdef HUI_OS_LINUX
	#define hui_main	\
hui_main_lin();\
int main(int NumArgs,char *Args[])\
{\
	HuiSetArgs(NumArgs,Args);\
	return hui_main_lin();\
}\
int hui_main_lin
#endif

// usage:
//
// int hui_main()
// {
//     HuiInit();
//     ....
//     return HuiRun();
// }




//----------------------------------------------------------------------------------
// hui itself


// execution
void HuiInit();
void HuiInitExtended(const char *program,const char *version,void_function *error_cleanup_function,void *send_bug_report_function,bool load_res,const char *def_lang);
int HuiRun();
void HuiSetIdleFunction(void_function *idle_function);
void HuiRunLater(int time_ms, void_function *function);
void HuiDoSingleMainLoop();
void HuiEnd();
void HuiWaitTillWindowClosed(CHuiWindow *win);
void _cdecl HuiSleep(int duration_ms);
extern bool HuiEndKeepMsgAlive;

void _cdecl HuiSetDirectory(const char *dir);
int HuiGetCpuCount();

// error handling
void HuiSetErrorFunction(void_function *error_function);
void HuiSetDefaultErrorHandler(const char *program,const char *version,void_function *error_cleanup_function,void *send_bug_report_function);
void HuiSendBugReport();
void HuiRaiseError(const char *message);

// file dialogs
bool _cdecl HuiFileDialogOpen(CHuiWindow *win,const char *title,const char *dir,const char *show_filter,const char *filter);
bool _cdecl HuiFileDialogSave(CHuiWindow *win,const char *title,const char *dir,const char *show_filter,const char *filter);
bool _cdecl HuiFileDialogDir(CHuiWindow *win,const char *title,const char *dir/*,char *root_dir*/);
extern char HuiFileDialogPath[1024],HuiFileDialogFile[256],HuiFileDialogCompleteName[1024];
bool _cdecl HuiSelectColor(CHuiWindow *win,int r,int g,int b);
extern int HuiColor[4];
void _cdecl HuiSetProperty(const char *name, const char *value);
void _cdecl HuiAboutBox(CHuiWindow *win);

// message dialogs
int _cdecl HuiQuestionBox(CHuiWindow *win,const char *title,const char *text,bool allow_cancel=false);
void _cdecl HuiInfoBox(CHuiWindow *win,const char *title,const char *text);
void _cdecl HuiErrorBox(CHuiWindow *win,const char *title,const char *text);

int HuiLoadImage(const char *filename);
extern std::vector<std::string> hui_image_file; // hui internal, don't use!!!
struct sHuiKeyCode
{
	int ID, Code;
};
extern std::vector<sHuiKeyCode> HuiKeyCode; // hui internal, don't use!!!

// configuration
void _cdecl HuiConfigWriteInt(const char *name, int val);
void _cdecl HuiConfigWriteFloat(const char *name, float val);
void _cdecl HuiConfigWriteBool(const char *name, bool val);
void _cdecl HuiConfigWriteStr(const char *name, const char *str);
void _cdecl HuiConfigReadInt(const char *name, int &val, int default_val = 0);
void _cdecl HuiConfigReadFloat(const char *name, float &val, float default_val = 0);
void _cdecl HuiConfigReadBool(const char *name, bool &val, bool default_val = false);
void _cdecl HuiConfigReadStr(const char *name, char *str, const char *default_str = NULL);
void _cdecl HuiSaveConfigFile();
void _cdecl HuiRegisterFileType(const char *ending,const char *description,const char *icon_path,const char *open_with,const char *command_name,bool set_default);

// clipboard
void _cdecl HuiCopyToClipBoard(const char *buffer,int length);
void _cdecl HuiPasteFromClipBoard(char **buffer,int &length);
void _cdecl HuiOpenDocument(const char *filename);

// language
extern std::vector<std::string> HuiLanguageName;
void _cdecl HuiSetLanguage(const char *language);
const char *_cdecl HuiGetLanguage(int id);
const char *_cdecl HuiGetLanguageS(const char *str);
#define L(id)	HuiGetLanguage(id)
#define _(str)	HuiGetLanguageS(str)
void _cdecl HuiUpdateAll();
extern bool HuiLanguaged;
extern char HuiCurLanguageName[128];


// resources
void _cdecl HuiLoadResource(const char *filename);
extern std::vector<sHuiResource> _HuiResource_; // hui internal, don't use!!!

// input
const char *_cdecl HuiGetKeyName(int key);
const char *_cdecl HuiGetKeyCodeName(int k);

// timers
int _cdecl HuiCreateTimer();
float _cdecl HuiGetTime(int index);

// HUI configuration
extern char HuiComboBoxSeparator,*HuiSeparator;
extern bool HuiUseFlatButtons;
extern bool HuiMultiline;
extern bool HuiCreateHiddenWindows;

// key codes and id table ("shortcuts")
void HuiAddKeyCode(int id, int key_code);

#ifdef HUI_API_GTK
extern GdkEvent *HuiGdkEvent;
#endif

// data from hui (...don't change...)
extern char HuiAppFilename[256],HuiAppDirectory[256],HuiInitialWorkingDirectory[256];
extern char HuiSingleParam[512];
extern std::vector<char*> HuiArgument;
extern bool HuiRunning;

// hui internal window lists... don't use!!!
extern std::vector<CHuiWindow*> _HuiWindow_;
struct sHuiClosedWindow
{
	CHuiWindow *win;
	int UID;
};
extern std::vector<sHuiClosedWindow> _HuiClosedWindow_;



#include "hui_menu.h"
#include "hui_window.h"


#endif


