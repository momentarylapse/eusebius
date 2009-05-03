/*----------------------------------------------------------------------------*\
| CHui                                                                         |
| -> Heroic User Interface                                                     |
| -> abstraction layer for GUI                                                 |
|   -> Windows (default api) or Linux (Gtk+)                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2007.03.25 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(HUI_H)
#define HUI_H


extern char HuiVersion[32];



// which operating system?

#ifdef WIN32
	#define HUI_OS_WINDOWS
#else
	#define HUI_OS_LINUX
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
#ifdef HUI_OS_LINUX
	#include <gtk/gtk.h>
	#include <gdk/gdkkeysyms.h>
	#define _cdecl
#endif



#ifdef HUI_OS_WINDOWS
	extern unsigned char KeyID[256];
	extern HINSTANCE HuiInstance;
#endif


#define HUI_MAX_WINDOWS			256
#define HUI_MAX_MENU_ITEMS	 	64
#define HUI_MAX_KEY_CODES		256
#define HUI_MAX_TIMERS			128

enum{
	HUI_YES=20,
	HUI_NO,
	HUI_CANCEL
};

typedef void message_function(int);
typedef void void_function();

#ifdef HUI_OS_WINDOWS
	extern void HuiSetArgs(char *arg);
#else
	extern void HuiSetArgs(int num_args,char *args[]);
#endif



// for a system independent usage of this library
#ifdef HUI_OS_WINDOWS
	#define hui_main	\
hui_main2();\
WINAPI WinMain(HINSTANCE hThisInstance,HINSTANCE hPrevInstance,LPSTR lpszArgument,int nFunsterStil)\
{\
	HuiInstance=hThisInstance;\
	HuiSetArgs(lpszArgument);\
	return hui_main2();\
}\
hui_main2
#endif
#ifdef HUI_OS_LINUX
	#define hui_main	\
hui_main2();\
int main(int NumArgs,char *Args[])\
{\
	HuiSetArgs(NumArgs,Args);\
	return hui_main2();\
}\
int hui_main2
#endif

// usage:
//
// int hui_main()
// {
//     hui=new CHui();
//     ....
//     return hui->Run();
// }
	

class CHuiWindow;

class CHui
{
public:
	CHui();
	virtual ~CHui();
	int Run();
	void DoSingleMainLoop();
	void End();

#ifdef HUI_OS_WINDOWS
	HICON main_icon;
#endif
	void_function *IdleFunction,*ErrorFunction;
	bool HaveToExit;
	bool Running;

	int NumWindows;
	CHuiWindow *Window[HUI_MAX_WINDOWS];
	bool WindowClosed[HUI_MAX_WINDOWS];
	void WaitTillWindowClosed(CHuiWindow *win);
	void UseCorrectValues(bool correct);
	float GtkCorrectionFactor;
	void _cdecl Sleep(int duration_ms);
	void _cdecl SetDirectory(char *dir);
	void SetErrorFunction(void_function *error_function);

	// file dialogs
	bool _cdecl FileDialogOpen(CHuiWindow *win,char *title,char *dir,char *show_filter,char *filter);
	bool _cdecl FileDialogSave(CHuiWindow *win,char *title,char *dir,char *show_filter,char *filter);
	bool _cdecl FileDialogDir(CHuiWindow *win,char *title,char *dir/*,char *root_dir*/);
	char FileDialogPath[1024],FileDialogFile[256],FileDialogCompleteName[1024];

	// message dialogs
	int _cdecl QuestionBox(CHuiWindow *win,char *title,char *text,bool allow_cancel=false);
	void _cdecl InfoBox(CHuiWindow *win,char *title,char *text);
	void _cdecl ErrorBox(CHuiWindow *win,char *title,char *text);

	int LoadImage(char *filename);

	// configuration
	void _cdecl ConfigWriteInt(char *name,int val);
	void _cdecl ConfigWriteStr(char *name,char *str);
	void _cdecl ConfigReadInt(char *name,int &val,int default_val=0);
	void _cdecl ConfigReadStr(char *name,char *str,char *default_str=NULL);
	void _cdecl RegisterFileType(char *ending,char *description,char *icon_path,char *open_with,char *command_name,bool set_default);

	// clipboard
	void _cdecl CopyToClipBoard(char *buffer,int length);
	void _cdecl PasteFromClipBoard(char **buffer,int &length);
	void _cdecl OpenDocument(char *filename);

	// language
	void _cdecl SetLanguageFile(char *filename);
	char *_cdecl GetLanguage(int id);
	void _cdecl UpdateAll();

	// resources
	void _cdecl LoadResource(char *filename);

	// input
	char *_cdecl GetKeyName(int key);
	char *_cdecl GetKeyCodeName(int k);

	// timers
	int _cdecl CreateTimer();
	float _cdecl GetTime(int index);

	// HUI configuration
	char ComboBoxSeparator,_Pseudo_Byte_,*Separator;
	bool UseFlatButtons;
	bool Multiline;
	bool CreateHiddenWindows;

	int NumKeyCodes;
	int KeyCode[HUI_MAX_KEY_CODES],KeyCodeID[HUI_MAX_KEY_CODES];

	char AppFilename[256],AppDirectory[256];
	char SingleParam[512];
};

extern CHui *hui;

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
};

class CHuiMenu
{
public:
	CHuiMenu();
	virtual ~CHuiMenu();
	void _cdecl OpenPopup(CHuiWindow *win,int x,int y);
	void _cdecl AddEntry(char *name,int id);
	void _cdecl AddEntryImage(char *name,int image,int id);
	void _cdecl AddEntryCheckable(char *name,int id);
	void _cdecl AddSeparator();
	void _cdecl AddSubMenu(char *name,int id,CHuiMenu *menu);
	void _cdecl CheckItem(int id,bool checked);
	bool _cdecl IsItemChecked(int id);
	void _cdecl EnableItem(int id,bool enabled);
	void _cdecl SetText(int id,char *text);
	void _cdecl SetID(int id);

#ifdef HUI_OS_WINDOWS
	HMENU hMenu;
#endif
#ifdef HUI_OS_LINUX
    GtkWidget *g_menu,*g_item[HUI_MAX_MENU_ITEMS];
#endif
	int NumItems;
	CHuiMenu *SubMenu[HUI_MAX_MENU_ITEMS];
	int ItemID[HUI_MAX_MENU_ITEMS],ItemImage[HUI_MAX_MENU_ITEMS];
	char ItemName[HUI_MAX_MENU_ITEMS][64];
	bool ItemEnabled[HUI_MAX_MENU_ITEMS],ItemIsSeparator[HUI_MAX_MENU_ITEMS],ItemChecked[HUI_MAX_MENU_ITEMS],ItemCheckable[HUI_MAX_MENU_ITEMS];
};

CHuiMenu *_cdecl HuiCreateMenu();

struct sCompleteWindowMessage
{
	#ifdef HUI_OS_WINDOWS
		unsigned int msg,wparam,lparam;
	#endif
};

#define HUI_MAX_KEYBUFFER_DEPTH		128

// user input
struct sInputData{
	// mouse
	float mx,my,vx,vy,mwheel;	// position
	float mw;					// drection
	bool lb,mb,rb;				// keys
	// keyboard
	int KeyBufferDepth;
	bool key[256];
	int KeyBuffer[HUI_MAX_KEYBUFFER_DEPTH];
};

struct irect
{
public:
	int x1,y1,x2,y2;
	irect(){};
	irect(int x1,int x2,int y1,int y2)
	{	this->x1=x1;	this->x2=x2;	this->y1=y1;	this->y2=y2;	}
};

#define MAX_CONTROLS_PER_WINDOW		128

struct sHuiControl{
	int ID,Kind;
#ifdef HUI_OS_WINDOWS
	HWND hWnd,hWnd2;
	int TextNr;
#endif
#ifdef HUI_OS_LINUX
    GtkWidget *win;
	int Selected;
	GtkTreeIter *_item_;
	int _num_items_;
#endif
	int TabID,TabPage;
	bool Enabled;
};

struct sHuiToolBarItem{
	int ID,Kind;
#ifdef HUI_OS_LINUX
	GtkToolItem *item;
#endif
	bool Enabled;
	CHuiMenu *menu;
};

struct sHuiToolBar{
	int NumItems;
	sHuiToolBarItem Item[HUI_MAX_MENU_ITEMS];
	bool Enabled,TextEnabled,LargeIcons;
#ifdef HUI_OS_LINUX
	GtkWidget *tool_bar;
#endif
};

class CHuiWindow
{
public:
	CHuiWindow(char *title,int x,int y,int width,int height,CHuiWindow *root,bool allow_root,int bg_mode,bool show,message_function *mf);
	virtual ~CHuiWindow();

	// the window
	void _cdecl Update();
	void _cdecl Hide(bool hide);
	void _cdecl SetMaximized(bool maximized);
	bool _cdecl IsMaximized();
	bool _cdecl IsMinimized();
	void _cdecl SetID(int id);
	void _cdecl SetFullscreen(bool fullscreen);
	void _cdecl SetTitle(char *title);
	void _cdecl SetPosition(int x,int y);
	void _cdecl SetPositionSpecial(CHuiWindow *win,int mode);
	void _cdecl SetOuterior(irect rect);
	irect _cdecl GetOuterior();
	void _cdecl SetInerior(irect rect);
	irect _cdecl GetInterior();
	void _cdecl Activate(int control_id=-1);
	bool _cdecl IsActive(bool include_sub_windows=false);

	void _cdecl SetCursorPos(int x,int y);
	void _cdecl ShowCursor(bool show);

	void _cdecl EnableStatusBar(bool enabled);
	void _cdecl SetStatusText(char *str);
	void _cdecl EnableToolBar(bool enabled);
	void _cdecl ToolBarSetCurrent(int index);
	void _cdecl ToolBarConfigure(bool text_enabled,bool large_icons);
	void _cdecl ToolBarAddItem(char *title,char *tool_tip,int image,int id);
	void _cdecl ToolBarAddItemCheckable(char *title,char *tool_tip,int image,int id);
	void _cdecl ToolBarAddItemMenu(char *title,char *tool_tip,int image,CHuiMenu *menu,int id);
	void _cdecl ToolBarAddItemMenuByID(char *title,char *tool_tip,int image,int menu_id,int id);
	void _cdecl ToolBarAddSeparator();
	void _cdecl ToolBarReset();
	void _cdecl ToolBarSetByID(int id);

	// creating controls
	void _cdecl AddButton(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddDefButton(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddCheckBox(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddText(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddEdit(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddGroup(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddComboBox(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddTabControl(char *title,int x,int y,int width,int height,int id);
	void _cdecl SetTabCreationPage(int id,int page);
	void _cdecl AddListView(char *title,int x,int y,int width,int height,int id);
//	void _cdecl AddIconList(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddListView_Test(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddProgressBar(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddImage(char *title,int x,int y,int width,int height,int id);

	// using controls
	void _cdecl SetControlText(int id,char *str);
	void _cdecl SetControlInt(int id,int i);
	void _cdecl SetControlFloat(int id,float f,int dec);
	void _cdecl AddControlText(int id,char *str);
	void _cdecl AddControlChildText(int id,int parent_row,char *str);
	void _cdecl ChangeControlText(int id,int row,char *str);
	void _cdecl SetControlImage(int id,int image);
	char *_cdecl GetControlText(int id);
	int _cdecl GetControlInt(int id);
	float _cdecl GetControlFloat(int id);
	void _cdecl EnableControl(int id,bool enabled);
	bool _cdecl IsControlEnabled(int id);
	void _cdecl CheckControl(int id,bool checked);
	bool _cdecl IsControlChecked(int id);
	int _cdecl GetControlSelection(int id);
	int _cdecl GetControlSelectionM(int id,int *indices);
	void _cdecl SetControlSelection(int id,int index);
	void _cdecl ResetControl(int id);

	// input
	bool GetKey(int key);
	bool GetKeyDown(int key);
	bool GetKeyUp(int key);
	char GetKeyChar(int key);
	int GetKeyRhythmDown();
	
	int TabCreationID,TabCreationPage;

	bool AllowInput;

#ifdef HUI_OS_WINDOWS
	void_function *NixGetInputFromWindow;
	HWND hWnd;
	RECT WindowBounds,WindowClient;
	DWORD WindowStyle;
#endif
#ifdef HUI_OS_LINUX
    GtkWidget *window,*vbox,*menu_bar,*status_bar,*cur_cnt,*fixed,*__ttt__,*gtk_menu[HUI_MAX_MENU_ITEMS];
//,*gtk_tool[HUI_MAX_MENU_ITEMS];
	GtkWidget *gl_widget;
	int gtk_num_menus;
//,gtk_num_tools;
//	int gtk_tool_id[HUI_MAX_MENU_ITEMS];
#endif
	bool UsedByNix;
	int NumControls;
	sHuiControl Control[MAX_CONTROLS_PER_WINDOW];
	sHuiToolBar tool_bar[4],*tb;
	CHuiMenu *Menu,*Popup;
	bool StatusBarEnabled;
	message_function *MessageFunction;
	int mx,my;
	bool Allowed,AllowKeys;
	CHuiWindow *Root,*TerrorChild;
	int NumSubWindows;
	CHuiWindow *SubWindow[32];

	int ID;
	bool IsHidden;
	
	int NumTexts,TextX[MAX_CONTROLS_PER_WINDOW/2],TextY[MAX_CONTROLS_PER_WINDOW/2];
	char *TextStr[MAX_CONTROLS_PER_WINDOW/2];
	bool TextShow[MAX_CONTROLS_PER_WINDOW/2];

	sInputData InputData,OwnData,OwnDataOld;
	sCompleteWindowMessage CompleteWindowMessage;
};

void _cdecl HuiWindowAddControl(CHuiWindow *win,int control_type,char *title,int x,int y,int width,int height,int id);

CHuiWindow *_cdecl HuiCreateWindow(char *title,int x,int y,int width,int height,message_function *mf);
CHuiWindow *_cdecl HuiCreateNixWindow(char *title,int x,int y,int width,int height,message_function *mf);
CHuiWindow *_cdecl HuiCreateDialog(char *title,int width,int height,CHuiWindow *root,bool allow_root,message_function *mf);

CHuiWindow *_cdecl HuiCreateResourceDialog(int id,CHuiWindow *root,message_function *mf);
CHuiMenu *_cdecl HuiCreateResourceMenu(int id);

enum{
	BGModeBlack,
	BGModeStyleWindow,
	BGModeStyleDialog
};

#define HuiLeft		1
#define HuiRight	2
#define HuiTop		4
#define HuiBottom	8

extern char *SysStr(char *str);
extern char *DeSysStr(char *str);

enum{
	HUI_WIN_EMPTY=-1000,
	HUI_WIN_CLOSE,
	//HUI_WIN_DESTROY,
	HUI_WIN_SIZE,
	HUI_WIN_MOVE,
	HUI_WIN_RENDER,
	HUI_WIN_ERASEBKGND,
	HUI_WIN_LBUTTONDOWN,
	HUI_WIN_LBUTTONUP,
	HUI_WIN_RBUTTONDOWN,
	HUI_WIN_RBUTTONUP,
	HUI_WIN_KEYDOWN,
	HUI_WIN_KEYUP
};

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
	int i_param[6];
};
struct sHuiResource: sHuiResourceCommand
{
	int num_cmds;
	sHuiResourceCommand *cmd;
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
	HuiKindImage
};

// tool bar items
enum{
	HuiToolButton,
	HuiToolCheckable,
	HuiToolSeparator,
	HuiToolMenu
};

// which one of the toolbars?
enum{
	HuiToolBarTop,
	HuiToolBarBottom,
	HuiToolBarLeft,
	HuiToolBarRight
};


// key codes
enum{
	KEY_LCONTROL,
	KEY_RCONTROL,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LALT,
	KEY_RALT,
	KEY_ADD,
	KEY_SUBTRACT,
	KEY_FENCE,		// "Raute"???
	KEY_END,
	KEY_NEXT,
	KEY_PRIOR,
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_RETURN,
	KEY_ESCAPE,
	KEY_INSERT,
	KEY_DELETE,
	KEY_SPACE,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_HOME,
	KEY_NUM_0,
	KEY_NUM_1,
	KEY_NUM_2,
	KEY_NUM_3,
	KEY_NUM_4,
	KEY_NUM_5,
	KEY_NUM_6,
	KEY_NUM_7,
	KEY_NUM_8,
	KEY_NUM_9,
	KEY_NUM_ADD,
	KEY_NUM_SUBTRACT,
	KEY_NUM_MULTIPLY,
	KEY_NUM_DIVIDE,
	KEY_NUM_COMMA,
	KEY_NUM_ENTER,
	KEY_COMMA,
	KEY_DOT,
	KEY_SMALLER,
	KEY_SZ,
	KEY_AE,
	KEY_OE,
	KEY_UE,
	KEY_GRAVE,
	KEY_WINDOWS_L,
	KEY_WINDOWS_R,
	HUI_NUM_KEYS		// 98
};

#endif


