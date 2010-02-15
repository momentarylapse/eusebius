/*----------------------------------------------------------------------------*\
| Hui window                                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2009.12.05 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _HUI_WINDOW_EXISTS_
#define _HUI_WINDOW_EXISTS_

#include "hui_config.h"

class CHuiMenu;


struct sCompleteWindowMessage
{
	#ifdef HUI_API_WIN
		unsigned int msg,wparam,lparam;
	#endif
};


// user input
struct sInputData{
	// mouse
	float x, y, dx, dy, dz;	// position, change
	float mw;					// drection
	bool lb,mb,rb;				// keys
	// keyboard
	bool key[256];
	int KeyBufferDepth;
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


struct sHuiControl{
	int ID,Kind;
	int x,y;
#ifdef HUI_API_WIN
	HWND hWnd,hWnd2,hWnd3;
	std::vector<HWND> _item_;
	int Color[4]; // ColorButton...
#endif
#ifdef HUI_API_GTK
    GtkWidget *win;
	int Selected;
	std::vector<GtkTreeIter> _item_;
#endif
	int TabID,TabPage;
	bool Enabled;
};

struct sHuiToolBarItem{
	int ID,Kind;
#ifdef HUI_API_GTK
	GtkToolItem *item;
#endif
	bool Enabled;
	CHuiMenu *menu;
};

struct sHuiToolBar{
	std::vector<sHuiToolBarItem> Item;
	bool Enabled,TextEnabled,LargeIcons;
#ifdef HUI_API_WIN
	HWND hWnd;
#endif
#ifdef HUI_API_GTK
	GtkWidget *tool_bar;
#endif
};

class CHuiWindow
{
public:
	CHuiWindow(char *title,int x,int y,int width,int height,CHuiWindow *root,bool allow_root,int mode,bool show,message_function *mf);
	CHuiWindow(char *title,int x,int y,int width,int height,message_function *mf);
	~CHuiWindow();

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
	void _cdecl SetOuteriorDesired(irect rect);
	irect _cdecl GetOuteriorDesired();
	void _cdecl SetInterior(irect rect);
	irect _cdecl GetInterior();
	void _cdecl Activate(int control_id=-1);
	bool _cdecl IsActive(bool include_sub_windows=false);

	void _cdecl SetCursorPos(int x,int y);
	void _cdecl ShowCursor(bool show);

	// tool bars
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
	void _cdecl AddColorButton(char *title,int x,int y,int width,int height,int id);
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
	void _cdecl AddSlider(char *title,int x,int y,int width,int height,int id);
	void _cdecl AddImage(char *title,int x,int y,int width,int height,int id);

	// using controls
	void _cdecl SetControlText(int id,char *str);
	void _cdecl SetControlInt(int id,int i);
	void _cdecl SetControlFloat(int id,float f,int dec);
	void _cdecl AddControlText(int id,char *str);
	void _cdecl AddControlChildText(int id,int parent_row,char *str);
	void _cdecl ChangeControlText(int id,int row,char *str);
	void _cdecl SetControlImage(int id,int image);
	void _cdecl SetControlColor(int id,int *c,bool use_alpha);
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
	void _cdecl GetControlColor(int id,int *c,bool use_alpha);
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
	HWND hWnd;
#endif
#ifdef HUI_API_WIN
	bool ready;
	void_function *NixGetInputFromWindow;
	HWND status_bar,gl_hwnd;
	RECT WindowBounds,WindowClient;
	DWORD WindowStyle;
	int cdx,cdy;
#endif
#ifdef HUI_API_GTK
	GtkWidget *window,*vbox,*hbox,*menu_bar,*status_bar,*cur_cnt,*fixed,*__ttt__;
	std::vector<GtkWidget*> gtk_menu;
	GtkWidget *gl_widget;
	int gtk_num_menus;
#endif
	bool UsedByNix;
	std::vector<sHuiControl> Control;
	sHuiToolBar tool_bar[4],*tb;
	CHuiMenu *Menu,*Popup;
	bool StatusBarEnabled;
	message_function *MessageFunction;
	int mx,my;
	bool Allowed,AllowKeys;
	CHuiWindow *Root,*TerrorChild;
	std::vector<CHuiWindow*> SubWindow;

	int ID,uid;
	bool IsHidden;

	sInputData InputData,OwnData,OwnDataOld;
	sCompleteWindowMessage CompleteWindowMessage;
};

void _cdecl HuiWindowAddControl(CHuiWindow *win,int control_type,char *title,int x,int y,int width,int height,int id);

CHuiWindow *_cdecl HuiCreateWindow(char *title,int x,int y,int width,int height,message_function *mf);
CHuiWindow *_cdecl HuiCreateNixWindow(char *title,int x,int y,int width,int height,message_function *mf);
CHuiWindow *_cdecl HuiCreateDummyWindow(char *title,int x,int y,int width,int height,message_function *mf);
CHuiWindow *_cdecl HuiCreateDialog(char *title,int width,int height,CHuiWindow *root,bool allow_root,message_function *mf);
void _cdecl HuiCloseWindow(CHuiWindow *win);

// use this as message_function for a window, that should not respond to any message
extern void HuiMessageFunctionNone(int);

enum{
	HuiWinModeResizable = 1,
	HuiWinModeNoFrame = 2,
	HuiWinModeNoTitle = 4,
	HuiWinModeBGDialog = 8,
	HuiWinModeWindow = 1, // default
	HuiWinModeDialog = 8,
	//HuiWinModeBGBlack = 16,
	HuiWinModeNix = 32,
};

#define HuiLeft		1
#define HuiRight	2
#define HuiTop		4
#define HuiBottom	8

enum{
	HUI_WIN_EMPTY=-1000,
	HUI_WIN_CLOSE,
	HUI_WIN_SIZE,
	HUI_WIN_MOVE,
	HUI_WIN_RENDER,
	HUI_WIN_MOUSEMOVE,
	HUI_WIN_MOUSEWHEEL,
	HUI_WIN_LBUTTONDOWN,
	HUI_WIN_LBUTTONUP,
	HUI_WIN_RBUTTONDOWN,
	HUI_WIN_RBUTTONUP,
	HUI_WIN_KEYDOWN,
	HUI_WIN_KEYUP
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

	HUI_NUM_KEYS,

	KEY_ANY
};

#endif
