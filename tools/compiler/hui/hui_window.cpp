/*----------------------------------------------------------------------------*\
| Hui window                                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2010.07.14 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "hui.h"

#include "../file/file.h"
#include <stdio.h>
#include <signal.h>
#ifdef HUI_API_WIN
	#include <shlobj.h>
	#include <winuser.h>
	#include <direct.h>
	#include <commctrl.h>
	#include <tchar.h>
	#pragma comment(lib,"winmm.lib")
	#pragma warning(disable : 4995)
#endif
#ifdef HUI_OS_LINUX
	#include <string.h>
	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/timeb.h>
	#include <time.h>
	#include <gdk/gdkx.h>
#endif
#ifdef HUI_API_GTK
	GtkTreeIter dummy_iter;
	#ifdef HUI_OS_WINDOWS
		#include <gdk/gdkwin32.h>
	#endif
#endif

// for unique window identifiers
static int current_uid=0;

//extern int allow_signal_level; // visual studio needs this line.... (-_-)

// recursively find a menu item and execute message_function
bool TestMenuID(CHuiMenu *menu,int id,message_function *mf)
{
	if (!menu)
		return false;
	for (unsigned int i=0;i<menu->Item.size();i++){
		if (menu->Item[i].ID==id){
			mf(id);
			return true;
		}
		if (menu->Item[i].SubMenu)
			if (TestMenuID(menu->Item[i].SubMenu,id,mf))
				return true;
	}
	return false;
}

void add_key_to_buffer(sInputData *d, int key)
{
	// full -> remove the first key
	if (d->KeyBufferDepth >= HUI_MAX_KEYBUFFER_DEPTH - 1){
		for (int k=0;k<d->KeyBufferDepth-2;k++)
			d->KeyBuffer[k] = d->KeyBuffer[k+1];
		d->KeyBufferDepth --;
	}
	d->KeyBuffer[d->KeyBufferDepth ++] = key;
}


//----------------------------------------------------------------------------------
// windows message handling


#ifdef HUI_API_WIN

// find a toolbar item
bool TestToolBarID(CHuiWindow *win,int id,message_function *mf)
{
	if (id<0)
		return false;
	for (int k=0;k<4;k++)
		if (win->tool_bar[k].Enabled)
			for (unsigned int i=0;i<win->tool_bar[k].Item.size();i++)
				if (win->tool_bar[k].Item[i].ID==id){
					if (win->ready)
						mf(id);
					return true;
				}
	return false;
}

static int win_reg_no=0;


void UpdateTabPages(CHuiWindow *win)
{
	for (unsigned int i=0;i<win->Control.size();i++){
		int n_tab=-1;
		int cmd=SW_SHOW;
		// find the tab-control
	    if (win->Control[i].TabID>=0)
			for (unsigned int j=0;j<i;j++)
				if ((win->Control[j].Kind==HuiKindTabControl)&&(win->Control[j].ID==win->Control[i].TabID))
					n_tab=j;
	    if (n_tab>=0){
			cmd=SW_HIDE;
			// recursive: Tab-Control visible?    (n_tab < i !!!!!)
			if ((win->Control[i].TabPage==win->GetControlSelection(win->Control[i].TabID))&& (IsWindowVisible(win->Control[n_tab].hWnd)))
			    cmd=SW_SHOW;
		}
		if (win->Control[i].hWnd2)//win->Control[i].Kind==HuiKindEdit)
			ShowWindow(win->Control[i].hWnd2,cmd);
		if (win->Control[i].hWnd3)
			ShowWindow(win->Control[i].hWnd3,cmd);
		ShowWindow(win->Control[i].hWnd,cmd);
	}
	//msg_write>Write("//Tab");
}

/*static void ExecuteWinMessageFunc(CHuiWindow *win,int id)
{
	if (win){
		win->OwnDataOld=win->OwnData;
		for (i=0;i<256;i++)
			win->OwnData.key[KeyID[i]]=(GetAsyncKeyState(i)!=0);
		int k=-1;
		if ((!win->GetKey(KEY_RALT))&&(!win->GetKey(KEY_LALT))){
			for (i=6;i<NUM_KEYS;i++)
				if (win->GetKeyDown(i)){
					k=i;
					if ((win->GetKey(KEY_RCONTROL))||(win->GetKey(KEY_LCONTROL)))	k+=256;
					if ((win->GetKey(KEY_RSHIFT))||(win->GetKey(KEY_LSHIFT)))		k+=512;
				}
			if (k>=0)
				for (i=0;i<HuiNumKeyCodes;i++)
					if (k==HuiKeyCode[i])
						mf(HuiKeyCodeID[i]);
		}
	}
}*/

struct s_win_bitmap{
	BITMAPINFOHEADER header; 
	RGBQUAD color[64]; 
};

static s_win_bitmap _win_bitmap_;
static int win_temp_color[4];

void _win_color_interpolate_(int c[4],int c1[4],int c2[4],float t)
{
	c[0]=(int)( (float)c1[0] * (1-t) + (float)c2[0] * t );
	c[1]=(int)( (float)c1[1] * (1-t) + (float)c2[1] * t );
	c[2]=(int)( (float)c1[2] * (1-t) + (float)c2[2] * t );
}

RGBQUAD _win_color_from_i4_(int c[4])
{
	RGBQUAD wc;
	wc.rgbRed  =c[0];
	wc.rgbGreen=c[1];
	wc.rgbBlue =c[2];
	return wc;
}

static int _win_c_tbg1_[4]={255,255,255,0};
static int _win_c_tbg2_[4]={120,120,120,0};
static int _win_c_gray_[4]={127,127,127,0};

HRESULT _win_color_brush_(int c[4],bool enabled)
{
	memset(&_win_bitmap_.header,0,sizeof(_win_bitmap_.header));
	_win_bitmap_.header.biSize=sizeof(BITMAPINFOHEADER);
	_win_bitmap_.header.biHeight=8;
	_win_bitmap_.header.biWidth=8;
	_win_bitmap_.header.biBitCount=32;
	_win_bitmap_.header.biCompression=BI_RGB;
	_win_bitmap_.header.biPlanes=1;
	int c1[4],c2[4];
	_win_color_interpolate_(c1,_win_c_tbg1_,c,float(c[3])/255.0f);
	_win_color_interpolate_(c2,_win_c_tbg2_,c,float(c[3])/255.0f);
	if (!enabled){
		_win_c_gray_[0]=GetRValue(GetSysColor(COLOR_3DFACE));
		_win_c_gray_[1]=GetGValue(GetSysColor(COLOR_3DFACE));
		_win_c_gray_[2]=GetBValue(GetSysColor(COLOR_3DFACE));
		_win_color_interpolate_(c1,_win_c_gray_,c1,0.5f);
		_win_color_interpolate_(c2,_win_c_gray_,c2,0.5f);
	}
	RGBQUAD r1,r2;
	r1=_win_color_from_i4_(c1);
	r2=_win_color_from_i4_(c2);
	for (int x=0;x<8;x++)
		for (int y=0;y<8;y++){
			_win_bitmap_.color[y*8+x] = ( (x<4)^(y<4) ) ? r1 : r2;
		}
	return (LRESULT)CreateDIBPatternBrushPt((PBITMAPINFO)&_win_bitmap_,DIB_RGB_COLORS);
}

static bool win_proc_force_return;
static HBRUSH bkground;

static LRESULT WindowProcedureDefaultStuff(CHuiWindow *win,HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	win_proc_force_return=true;

	//msg_write>Write(" w");
	win->CompleteWindowMessage.msg=message;
	win->CompleteWindowMessage.wparam=(unsigned int)wParam;
	win->CompleteWindowMessage.lparam=(unsigned int)lParam;

	// losing focus -> disable keyboard handling
	if (GetActiveWindow()!=win->hWnd){
		win->AllowKeys=false;
		win->OwnData.KeyBufferDepth=0;
	}

	//msg_write>Write("-");
	//msg_write>Write((int)message);
	//msg_write>Write((int)(GetActiveWindow()==win->hWnd));


	// Nix input handling
	if (win->UsedByNix)
		if (win->NixGetInputFromWindow)
			win->NixGetInputFromWindow();

	switch (message){
		case WM_COMMAND:
			if (HIWORD(wParam)==BN_CLICKED){
				for (unsigned int i=0;i<win->Control.size();i++)
					if (win->Control[i].Kind==HuiKindColorButton){
						if (win->Control[i].hWnd==(HWND)lParam)
							// color button clicked -> start color selector
							if (HuiSelectColor(win,win->Control[i].Color[0],win->Control[i].Color[1],win->Control[i].Color[2])){
								memcpy(win->Control[i].Color,HuiColor,12);
								//SendMessage(win->Control[i].hWnd2,(UINT),0,0);
								ShowWindow(win->Control[i].hWnd2,SW_HIDE);
								ShowWindow(win->Control[i].hWnd2,SW_SHOW);
							}
						if (win->Control[i].hWnd3==(HWND)lParam)
							// color button clicked -> start color selector     (alpha... ugly, but ok...)
							if (HuiSelectColor(win,win->Control[i].Color[3],win->Control[i].Color[3],win->Control[i].Color[3])){
								win->Control[i].Color[3]=(HuiColor[0]+HuiColor[1]+HuiColor[2])/3;
								//SendMessage(win->Control[i].hWnd2,(UINT),0,0);
								ShowWindow(win->Control[i].hWnd2,SW_HIDE);
								ShowWindow(win->Control[i].hWnd2,SW_SHOW);
							}
					}
			}
			break;
			
		case WM_CTLCOLORSTATIC:
			// setting background for color button
			for (int i=0;i<win->Control.size();i++)
				if (win->Control[i].Kind==HuiKindColorButton)
					if (win->Control[i].hWnd2==(HWND)lParam){
						//return (LRESULT)CreateSolidBrush(RGB(win->Control[i].Color[0],win->Control[i].Color[1],win->Control[i].Color[2]));
						return _win_color_brush_(win->Control[i].Color,win->Control[i].Enabled);
					}
			// default color...
			SetBkColor ((HDC) wParam, GetSysColor(COLOR_3DFACE));
			bkground = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
			for (int i=0;i<win->Control.size();i++)
				if (win->Control[i].hWnd==(HWND)lParam)
					if (win->Control[i].TabID>=0){
						SetBkColor ((HDC) wParam, GetSysColor(COLOR_BTNHIGHLIGHT));
						bkground = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT));
					}
			return((DWORD) bkground);
			//return DefWindowProc(hwnd,message,wParam,lParam);
	}

	// disabled windows (having terror children) -> default...
	if (!win->Allowed){
		//msg_write>Write(string2("!win->Allowed %d",message));
		switch (message){
			case WM_PAINT:
				DefWindowProc(hwnd,message,wParam,lParam);
				if (win->MessageFunction)
					win->MessageFunction(HUI_WIN_RENDER);
				break;
			case WM_ERASEBKGND:
				DefWindowProc(hwnd,message,wParam,lParam);
				//if (win->MessageFunction)
				//	win->MessageFunction(HUI_WIN_ERASEBKGND);
				break;
			case WM_SETFOCUS:
				if (win->TerrorChild)
					SetFocus(win->TerrorChild->hWnd);
				break;
			case WM_ENABLE:
				if (win->TerrorChild)
					EnableWindow(win->TerrorChild->hWnd,TRUE);
				break;
			/*case WM_ACTIVATE:
				if (win->TerrorChild)
					SetActiveWindow(win->TerrorChild->hWnd);
				break;*/
			case WM_SIZE:
				// automatically reposition statusbar and toolbar
				if (win->StatusBarEnabled)
					SendMessage(win->status_bar,(UINT)WM_SIZE,0,0);
				if (win->tool_bar[HuiToolBarTop].Enabled)
					SendMessage(win->tool_bar[HuiToolBarTop].hWnd,(UINT)TB_AUTOSIZE,0,0);
				return DefWindowProc(hwnd,message,wParam,lParam);
			default:
				return DefWindowProc(hwnd,message,wParam,lParam);
		}
		return 0;
	}

	//msg_write>Write(string2("win %d",message));

	// default cursor...
	switch (message){
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			win->mx=LOWORD(lParam);
			win->my=HIWORD(lParam);
			break;
	}

	win_proc_force_return=false;
	return 0;
}

static LRESULT WindowProcedureWithMF(CHuiWindow *win,message_function *mf,HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	win_proc_force_return=true;

	
	//msg_write>Write(string2("mf   %d",message));
	switch (message){
		case WM_COMMAND:

			// control item ids
			for (int i=0;i<win->Control.size();i++){
				if (win->Control[i].hWnd==(HWND)lParam){

					// default actions
					switch (HIWORD(wParam)){
						case CBN_SELCHANGE:
						case TCN_SELCHANGE:
						case BN_CLICKED:
						case EN_CHANGE:
							if (win->ready)
								mf(win->Control[i].ID);
							return 0;
					}
					if (win->ready)
						mf(win->Control[i].ID);
    			}
				//msg_write>Write(string2("hw wP = %d",(int)HIWORD(wParam)));
			}

			// menu and toolbar ids
			if (HIWORD(wParam)==0){//WM_USER){
				if (TestMenuID(win->Menu,LOWORD(wParam),mf))
					return 0;
				if (TestMenuID(win->Popup,LOWORD(wParam),mf))
					return 0;
				if (TestToolBarID(win,LOWORD(wParam),mf))
					return 0;
			}
			break;
		case WM_NOTIFY:

			// more control item actions and ids
			for (int i=0;i<win->Control.size();i++)
				if (((LPNMHDR)lParam)->hwndFrom==win->Control[i].hWnd){

					// double click list view
					if ((win->Control[i].Kind==HuiKindListView)&&(((LPNMHDR)lParam)->code==NM_DBLCLK))
						if (win->ready)
							mf(win->Control[i].ID);

					// select tab page
					if ((win->Control[i].Kind==HuiKindTabControl)&&(((LPNMHDR)lParam)->code==TCN_SELCHANGE)){
						UpdateTabPages(win);
						if (win->ready)
							mf(win->Control[i].ID);
						return 0;
					}
				}
               DefWindowProc(hwnd,message,wParam,lParam);
		case WM_MOUSEMOVE:
			if (win->ready)
				mf(HUI_WIN_MOUSEMOVE);
			break;
		case WM_MOUSEWHEEL:
			if (win->ready)
				mf(HUI_WIN_MOUSEWHEEL);
			break;
		case WM_LBUTTONDOWN:
			if (win->ready)
				mf(HUI_WIN_LBUTTONDOWN);
			break;
		case WM_LBUTTONUP:
			if (win->ready)
				mf(HUI_WIN_LBUTTONUP);
			break;
		case WM_RBUTTONDOWN:
			if (win->ready)
				mf(HUI_WIN_RBUTTONDOWN);
			break;
		case WM_RBUTTONUP:
			if (win->ready)
				mf(HUI_WIN_RBUTTONUP);
			break;
		case WM_KEYDOWN:
			// quite complicated...
			if (GetActiveWindow()==win->hWnd){
				win->AllowKeys=true;
				// ...save to a key buffer!
				add_key_to_buffer(&win->OwnData, HuiKeyID[wParam]);
				if (win->ready)
					mf(HUI_WIN_KEYDOWN);
			}
			break;
		case WM_KEYUP:
			if (win->ready)
				mf(HUI_WIN_KEYUP);
			break;
		case WM_SIZE:
		case WM_SIZING:
			// automatically reposition statusbar and toolbar
			if (win->StatusBarEnabled)
				SendMessage(win->status_bar,(UINT)WM_SIZE,0,0);
			if (win->tool_bar[HuiToolBarTop].Enabled)
				SendMessage(win->tool_bar[HuiToolBarTop].hWnd,(UINT)TB_AUTOSIZE,0,0);
			if (win->ready)
				mf(HUI_WIN_SIZE);
               //DefWindowProc(hwnd,message,wParam,lParam);
			break;
		case WM_MOVE:
		case WM_MOVING:
			if (win->ready)
				mf(HUI_WIN_MOVE);
			break;
		case WM_PAINT:
			//if (!win->UsedByNix)
				DefWindowProc(hwnd,message,wParam,lParam); // else message boxes wouldn't work
			if (win->ready)
				mf(HUI_WIN_RENDER);
			break;
		case WM_ERASEBKGND:
			if (!win->UsedByNix)
				DefWindowProc(hwnd,message,wParam,lParam);
			//mf(HUI_WIN_ERASEBKGND);
			break;
		//case WM_DESTROY:
		//	mf(HUI_WIN_DESTROY);
		//	break;
		case WM_CLOSE:
			if (win->ready)
				mf(HUI_WIN_CLOSE);
			break;
		case WM_NCACTIVATE:
			// reactivate key handling if we get the focus
			if (win->ready)
				win->AllowKeys=true;
			return DefWindowProc(hwnd,message,wParam,lParam);
		default:
			//mf(HUI_WIN_EMPTY);
			return DefWindowProc(hwnd,message,wParam,lParam);
	}

	win_proc_force_return=false;
	return 0;
}

static LRESULT WindowProcedureNoMF(CHuiWindow *win,HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	win_proc_force_return=true;
	
	//msg_write>Write(string2("!mf   %d",message));

	// default message_function (when none is applied)
	bool allow_exit=true;
	switch (message){
		case WM_NOTIFY:
			for (int i=0;i<win->Control.size();i++)
				if (((LPNMHDR)lParam)->hwndFrom==win->Control[i].hWnd){
					// select tab page
					if ((win->Control[i].Kind==HuiKindTabControl)&&(((LPNMHDR)lParam)->code==TCN_SELCHANGE)){
						UpdateTabPages(win);
						return 0;
					}
				}
			break;
		case WM_DESTROY:
			for (int i=0;i<_HuiWindow_.size();i++)
				if (_HuiWindow_[i]->MessageFunction)
					allow_exit=false;
			if (allow_exit)
				HuiEnd();
			if (win){
				win->hWnd=NULL;
				delete(win);
			}
			break;
		case WM_SIZE:
			if (win){
				// automatically reposition statusbar and toolbar
				if (win->StatusBarEnabled)
					SendMessage(win->status_bar,(UINT)WM_SIZE,0,0);
				if (win->tool_bar[HuiToolBarTop].Enabled)
					SendMessage(win->tool_bar[HuiToolBarTop].hWnd,(UINT)TB_AUTOSIZE,0,0);
			}
			return DefWindowProc(hwnd,message,wParam,lParam);
		default:
			return DefWindowProc(hwnd,message,wParam,lParam);
	}

	win_proc_force_return=false;
	return 0;
}

static LRESULT CALLBACK WindowProcedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	//msg_write>Write("WP");

	// find hui window for hwnd...
	CHuiWindow *win=NULL;
	message_function *mf=NULL;
	for (int i=0;i<_HuiWindow_.size();i++){
		if (_HuiWindow_[i]->hWnd==hwnd){
			win=_HuiWindow_[i];
			mf=win->MessageFunction;
			break;
		}
	}

	HRESULT hr;

	// default stuff
	if (win){
		hr=WindowProcedureDefaultStuff(win,hwnd,message,wParam,lParam);
		if (win_proc_force_return)
			return hr;
	}
	//msg_write>Write(" a");

	/*if (HuiIdleFunction)
		HuiIdleFunction(0);*/


	// keyboard and shortcuts handling
	if (win)
		if ((GetActiveWindow()==win->hWnd)&&(win->AllowKeys)){
			win->OwnDataOld=win->OwnData;
			for (int i=0;i<256;i++)
				win->OwnData.key[HuiKeyID[i]]=((GetAsyncKeyState(i)&(1<<15))!=0);
			int k=-1;
			if ((!win->GetKey(KEY_RALT))&&(!win->GetKey(KEY_LALT))){
				for (int i=6;i<HUI_NUM_KEYS;i++)
					if (win->GetKeyDown(i)){
						k=i;
						if ((win->GetKey(KEY_RCONTROL))||(win->GetKey(KEY_LCONTROL)))	k+=256;
						if ((win->GetKey(KEY_RSHIFT))||(win->GetKey(KEY_LSHIFT)))		k+=512;
					}
				if (k>=0)
					for (int i=0;i<HuiKeyCode.size();i++)
						if (k==HuiKeyCode[i].Code){
							//msg_write>Write("---------------------------------");
							//msg_write>Write(HuiKeyCode[i].ID);
							mf(HuiKeyCode[i].ID);
						}
			}
		}


	// with message function
	if ((mf)&&(allow_signal_level<=0)){

		hr=WindowProcedureWithMF(win,mf,hwnd,message,wParam,lParam);
		if (win_proc_force_return)
			return hr;

	// without message function -> default
	}else{

		hr=WindowProcedureNoMF(win,hwnd,message,wParam,lParam);
		if (win_proc_force_return)
			return hr;

	}
	//msg_write>Write(" x");

	/*	//if (GetActiveWindow()!=_HuiWindow_->hWnd)
		//	AllowWindowsKeyInput=false;
	
		//if (AllowWindowsKeyInput)
	//	if (GetActiveWindow()==win->hWnd)
			for (i=0;i<256;i++)
				win->OwnData.key[KeyID[i]]=(GetAsyncKeyState(i)!=0);
		else{
			for (i=0;i<256;i++)
				win->OwnData.key[i]=false;
		}*/

		// Korrektur (manche Tasten belegen mehrere Array-Elemente) :-S
		//if (GetKey(KEY_RALT))
		//	_HuiWindow_->OwnData.key[KEY_LCONTROL]=0;

    return 0;
}
#endif

#ifdef HUI_API_GTK


//----------------------------------------------------------------------------------
// linux message handling

inline CHuiWindow *win_from_widget(void *widget)
{
	for (int i=0;i<_HuiWindow_.size();i++)
		if (_HuiWindow_[i]->window == widget)
			return _HuiWindow_[i];
	return NULL;
}

bool win_send_message(GtkWidget *widget, int message)
{
	if (!widget)
		return false;
	for (int i=0;i<_HuiWindow_.size();i++)
		if (_HuiWindow_[i]->window == widget){
		//if (gtk_window_has_toplevel_focus(GTK_WINDOW(_HuiWindow_[i]->window))){
			//_so("top_level");
			if ((_HuiWindow_[i]->MessageFunction)&&(_HuiWindow_[i]->AllowInput)){
				_HuiWindow_[i]->MessageFunction(message);
				return true;
			}
		}
	return false;
}

gboolean CallbackMenu(GtkWidget *widget, gpointer data)
{
	if (allow_signal_level > 0)
		return FALSE;
	msg_db_m("CallbackMenu", 1);
	if ((int)(long)data < 0)
		return FALSE;
	for (int i=0;i<_HuiWindow_.size();i++)
		//if (gtk_window_has_toplevel_focus(GTK_WINDOW(_HuiWindow_[i]->window))){
			//_so("top_level");
			if ((_HuiWindow_[i]->MessageFunction)&&(_HuiWindow_[i]->AllowInput)){
				_HuiWindow_[i]->MessageFunction((int)(long)data);
				return true;
			}
	//int i = (int)win_send_message(GTK_WIDGET(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), (int)(long)data);
	return FALSE;
}

/*gboolean CallbackWindow(GtkWidget *widget,gpointer data)
{
	if (allow_signal_level>0)
		return FALSE;
	msg_db_m("CallbackWindow",1);
	//_so((int)data);
	//_so((char*)data);
	CHuiWindow *win=NULL;
	message_function *mf=NULL;
	for (int i=0;i<_HuiNumWindows_;i++){
		if (_HuiWindow_[i]->window==widget){
			win=_HuiWindow_[i];
			if (win->AllowInput)
				mf=win->MessageFunction;
			break;
		}
	}
	if (mf){
		printf("%d\n", (int)data);
		mf(HUI_WIN_CLOSE);//(int)data);
		return TRUE;
	}else{
		//if ((int)data==HUI_WIN_CLOSE)
			HuiEnd();
		return FALSE;
	}
}*/

void NotifyWindow(CHuiWindow *win, GtkWidget *control)
{
	if (allow_signal_level > 0)
		return;
	msg_db_m("NotifyWindow", 1);
	for (int i=0;i<_HuiWindow_.size();i++)
		if (_HuiWindow_[i] == win){
			CHuiWindow *win = _HuiWindow_[i];
			message_function *mf = win->MessageFunction;
			int id = -1;
			if ((mf) && (win->AllowInput)){
				for (int j=0;j<win->Control.size();j++)
					if (win->Control[j].win == control)
						id = win->Control[j].ID;
				for (int t=0;t<4;t++)
					for (int j=0;j<win->tool_bar[t].Item.size();j++)
						if ((GtkWidget*)win->tool_bar[t].Item[j].item == control)
							id = win->tool_bar[t].Item[j].ID;
				mf(id);
			}
		}
	//msg_write>Write("/NotifyWindow");
}

void CallbackControl(GtkWidget *widget, gpointer data)
{
	msg_db_m("CallbackControl",1);
	NotifyWindow((CHuiWindow*)data,widget);
}

void CallbackControl2(GtkWidget *widget, void* a, gpointer data)
{
	msg_db_m(string2("CallbackControl2 %d", (int)(long)a), 1);
	NotifyWindow((CHuiWindow*)data, widget);
}

void CallbackControl3(GtkWidget *widget, void* a, void* b, gpointer data)
{
	msg_db_m(string2("CallbackControl3 %d %d", (int)(long)a, (int)(long)b), 1);
	NotifyWindow((CHuiWindow*)data, widget);
}

void CallbackTabControl(GtkWidget *widget, GtkNotebookPage *page, guint page_num, gpointer data)
{
	msg_db_m("CallbackTabControl",1);
	for (int i=0;i<_HuiWindow_.size();i++)
		if (_HuiWindow_[i] == data){
			CHuiWindow *win = _HuiWindow_[i];
			message_function *mf = win->MessageFunction;
			for (int j=0;j<win->Control.size();j++)
				if (win->Control[j].win == widget)
					win->Control[j].Selected = page_num;
			if ((mf) && (win->AllowInput))
				for (int j=0;j<win->Control.size();j++)
					if (win->Control[j].win == widget){
						mf(win->Control[j].ID);
						break;
		    		}
		}
}

GtkWidget *KeyReciever;

int nk = 0;



bool process_key(GdkEventKey *event)
{
	//printf("%d   %d\n", nk++, event->time);
	// convert hardware keycode into GDK keyvalue
	GdkKeymapKey kmk;
	kmk.keycode = event->hardware_keycode;
	kmk.group = event->group;
	kmk.level = 0;
	int keyvalue = gdk_keymap_lookup_key(NULL, &kmk);
	//msg_write(keyvalue);

	// convert GDK keyvalue into HUI key id
	int key=-1;
	for (int i=0;i<HUI_NUM_KEYS;i++)
		//if ((KeyID[i]==k->keyval)||(KeyID2[i]==k->keyval))
		if (HuiKeyID[i] == keyvalue)
			key = i;
	if (key < 0){
		msg_db_m(string2("unknown key: %d", event->hardware_keycode), 1);
		return false;
	}

	bool down = (event->type == GDK_KEY_PRESS);
	CHuiWindow *win = win_from_widget(KeyReciever);//event->window);
	if (!win)	return false;

#ifdef HUI_OS_LINUX
	char key_map_stat[32];
	XQueryKeymap(hui_x_display, key_map_stat);
	bool actual_state = ((key_map_stat[event->hardware_keycode >> 3] >> (event->hardware_keycode & 7)) & 1);
#else
	bool actual_state = down;
#endif

	if (win->AllowInput)
		win->InputData.key[key] = actual_state;

	//printf("%d\n", event->state & GDK_CONTROL_MASK);

	msg_db_m(string2("%s:  %s", down ? "down" : "up", HuiGetKeyName(key)), 1);
	//if ((down) && (win->InputData.key[key])){
	if (down){
		// correct modifiers...
		/*if ((event->state & GDK_SHIFT_MASK) == 0)
			win->InputData.key[KEY_RSHIFT] = win->InputData.key[KEY_LSHIFT] = false;
		if ((event->state & GDK_CONTROL_MASK) == 0)
			win->InputData.key[KEY_RCONTROL] = win->InputData.key[KEY_LCONTROL] = false;
		if ((event->state & GDK_MOD1_MASK) == 0)
			win->InputData.key[KEY_RALT] = win->InputData.key[KEY_LALT] = false;*/
		add_key_to_buffer(&win->InputData, key);
		message_function *mf = win->MessageFunction;
		if ((mf) && (win->AllowInput)){
			// key code?
			int key_code = -1;
			if ((!win->InputData.key[KEY_RALT]) && (!win->InputData.key[KEY_LALT])){
				key_code = key;
				if ((win->InputData.key[KEY_RCONTROL]) || (win->InputData.key[KEY_LCONTROL]))	key_code += 256;
				if ((win->InputData.key[KEY_RSHIFT]) || (win->InputData.key[KEY_LSHIFT]))	key_code += 512;
			}
			if (key_code >= 0)
				for (int k=0;k<HuiKeyCode.size();k++)
					if (key_code == HuiKeyCode[k].Code){
						//msg_write>Write("---------------------------------");
						//msg_write>Write(HuiKeyCode[kk].ID);
						mf(HuiKeyCode[k].ID);
					}
		}
	}
	/*if (gdk_events_pending())
		msg_write("pend 2");*/
	/*while(gdk_events_pending()){
		msg_write("-");
		GdkEvent *e = gdk_event_peek();
		printf("%p\n", e);
		if (!e)
			break;
			msg_write("0");
		GdkEventType type = e->type;
			msg_write(type);
		if ((type == GDK_KEY_PRESS) || (type == GDK_KEY_RELEASE)){
			msg_write("a");
			gdk_event_free(e);
			msg_write("b");
			e = gdk_event_get();
			msg_write("c");
		}
		gdk_event_free(e);
			msg_write("d");
	}*/
	return true;
}

gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	if (win_send_message(widget, HUI_WIN_CLOSE))
		return true;
	// no message function: end program
	HuiEnd();
	return false;
}

void size_request(GtkWidget *widget, GtkRequisition *requisition, gpointer user_data)
{	win_send_message(widget, HUI_WIN_SIZE);	}

gboolean motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	CHuiWindow *win = win_from_widget(widget);
	if (win){
		// don't listen to "event", it lacks behind
		int mod, mx, my;
		gdk_window_get_pointer(win->window->window, &mx, &my, (GdkModifierType*)&mod);
		irect ri = win->GetInterior();
		irect ro = win->GetOuterior();
		win->InputData.x = mx + ro.x1 - ri.x1;
		win->InputData.y = my + ro.y1 - ri.y1;

		// only allow setting when cursor is in the interior
		if ((win->InputData.x < 0) || (win->InputData.x >= ri.x2 - ri.x1) || (win->InputData.y < 0) || (win->InputData.y >= ri.y2 - ri.y1)){
			// outside -> only reset allowed
			win->InputData.lb &= ((mod & GDK_BUTTON1_MASK) > 0);
			win->InputData.mb &= ((mod & GDK_BUTTON2_MASK) > 0);
			win->InputData.rb &= ((mod & GDK_BUTTON3_MASK) > 0);
		}else{
			win->InputData.lb = ((mod & GDK_BUTTON1_MASK) > 0);
			win->InputData.mb = ((mod & GDK_BUTTON2_MASK) > 0);
			win->InputData.rb = ((mod & GDK_BUTTON3_MASK) > 0);
		}
	}
	win_send_message(widget, HUI_WIN_MOUSEMOVE);
	gdk_event_request_motions(event); // too prevent too many signals for slow message processing
	return false;
}

gboolean scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
	CHuiWindow *win = win_from_widget(widget);
	if (win){
		if (event->direction == GDK_SCROLL_UP)
			win->InputData.dz += 1;
		else if (event->direction == GDK_SCROLL_DOWN)
			win->InputData.dz -= 1;
	}
	win_send_message(widget, HUI_WIN_MOUSEWHEEL);
	return false;
}

gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	KeyReciever = widget;
	if (process_key(event))
		win_send_message(widget, HUI_WIN_KEYDOWN);
	return false;
}

gboolean key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	KeyReciever = widget;
	if (process_key(event))
		win_send_message(widget, HUI_WIN_KEYUP);
	return false;
}

int xpi = 0;

gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
	//msg_write(string2("expose %d", xpi++));
	win_send_message(widget, HUI_WIN_RENDER);
	return false;
}

gboolean expose_event_gl(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
	//msg_write(string2("expose gl %d", xpi++));
	for (int i=0;i<_HuiWindow_.size();i++)
		if (_HuiWindow_[i]->gl_widget == widget)
			if (_HuiWindow_[i]->MessageFunction)
				_HuiWindow_[i]->MessageFunction(HUI_WIN_RENDER);
	//return false;
	return true; // stop handler...
}

gboolean visnot_event(GtkWidget *widget, GdkEventVisibility *event, gpointer user_data)
{
	//msg_write("visible");
	win_send_message(widget, HUI_WIN_RENDER);
	return false;
}

bool set_button_state(GtkWidget *widget, GdkEventButton *event)
{
	CHuiWindow *win = win_from_widget(widget);
	if (win){
		if (event->type == GDK_BUTTON_PRESS){
			irect r = win->GetInterior();
			if ((win->InputData.x < 0) || (win->InputData.x >= r.x2 - r.x1) || (win->InputData.y < 0) || (win->InputData.y >= r.y2 - r.y1))
				return false;
		}
		// don't listen to "event", it lacks behind
		if (event->button == 1)
			win->InputData.lb = (event->type == GDK_BUTTON_PRESS);
		else if (event->button == 2)
			win->InputData.mb = (event->type == GDK_BUTTON_PRESS);
		else if (event->button == 3)
			win->InputData.rb = (event->type == GDK_BUTTON_PRESS);
	}
	return true;
}

gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (set_button_state(widget, event))
		win_send_message(widget, HUI_WIN_LBUTTONDOWN);
	return false;
}

gboolean button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	if (set_button_state(widget, event))
		win_send_message(widget, HUI_WIN_LBUTTONUP);
	return false;
}

gboolean focus_in_event(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
	// make sure the contro/alt/shift keys are unset
	CHuiWindow *win = win_from_widget(widget);
	if (win){
		win->InputData.key[KEY_RSHIFT] = win->InputData.key[KEY_LSHIFT] = false;
		win->InputData.key[KEY_RCONTROL] = win->InputData.key[KEY_LCONTROL] = false;
		win->InputData.key[KEY_RALT] = win->InputData.key[KEY_LALT] = false;
	}
	return false;
}

#endif




//----------------------------------------------------------------------------------
// window functions


// general window
CHuiWindow::CHuiWindow(const char *title, int x, int y, int width, int height, CHuiWindow *root, bool allow_root, int mode, bool show, message_function *mf)
{
	msg_db_r("CHuiWindow()",1);
	_HuiWindow_.push_back(this);

	MessageFunction = mf;
	UsedByNix = false;
	Allowed = true;
	AllowKeys = true;
	Root = root;
	TerrorChild=NULL;
	if (Root){
		Root->Allowed=allow_root;
		if (!Root->Allowed)
			Root->TerrorChild=this;
		Root->SubWindow.push_back(this);
	}
	Menu=Popup=NULL;
	StatusBarEnabled;
	for (int i=0;i<4;i++){
		tool_bar[i].Enabled=false;
		tool_bar[i].TextEnabled=tool_bar[i].LargeIcons=true;
		tool_bar[i].Item.clear();
	}
	tb=&tool_bar[HuiToolBarTop];
	memset(&InputData,0,sizeof(InputData));
	/*InputData.KeyBufferDepth=0;
	memset(&InputData.key,0,sizeof(InputData.key));*/
	memset(&OwnData,0,sizeof(OwnData));
	/*OwnData.KeyBufferDepth=0;
	memset(&OwnData.key,0,sizeof(OwnData.key));*/
	TabCreationID=TabCreationPage=-1;

	IsHidden=false;
	Menu=NULL;
	ID=-1;
	NumFloatDecimals = 3;
	uid=current_uid++;
	AllowInput=false; // allow only if ->Update() was called

#ifdef HUI_API_WIN
	ready = false;
	gl_hwnd = NULL;
	status_bar = NULL;
	cdx = cdy = 0;
	NixGetInputFromWindow=NULL;
	TCHAR ClassName[64];
	_tcscpy(ClassName,sys_str(string2("HuiWindowClass %d",win_reg_no++)));
	WNDCLASSEX wincl;
	wincl.hInstance=hui_win_instance;
	wincl.lpszClassName=ClassName;
	wincl.lpfnWndProc=WindowProcedure;
	wincl.style=CS_DBLCLKS;
	wincl.cbSize=sizeof(WNDCLASSEX);

	wincl.hIcon=hui_win_main_icon;
	wincl.hIconSm=hui_win_main_icon;
	/*wincl.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wincl.hIconSm=LoadIcon(NULL,IDI_APPLICATION);*/

	wincl.hCursor=LoadCursor(NULL,IDC_ARROW);
	wincl.lpszMenuName=NULL;
	wincl.cbClsExtra=0;
	wincl.cbWndExtra=0;
	if ((mode & HuiWinModeNix) > 0)
		wincl.hbrBackground=CreateSolidBrush(RGB(0,0,0));
	else if ((mode & HuiWinModeBGDialog) > 0)
		wincl.hbrBackground=GetSysColorBrush(COLOR_3DFACE);
		//wincl.hbrBackground=GetSysColorBrush(COLOR_BTNHIGHLIGHT);
	else // default
		wincl.hbrBackground=GetSysColorBrush(COLOR_WINDOW);

	if (!RegisterClassEx(&wincl)){
		msg_error("new CHuiWindow - RegisterClassEx");
		msg_db_l(1);
		return;
	}
		
	DWORD style_ex = 0;
	if ((mode & HuiWinModeBGDialog) > 0)
		style_ex |= WS_EX_DLGMODALFRAME | WS_EX_NOPARENTNOTIFY;
	DWORD style = WS_OVERLAPPEDWINDOW;
	if ((mode & HuiWinModeBGDialog) > 0)
		style = DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;

	// align dialog box
	if ((mode & HuiWinModeBGDialog) > 0){
		if (root){
			// center on root window
			irect r=root->GetOuterior();
			x=r.x1+(r.x2-r.x1-width)/2;
			y=r.y1+(r.y2-r.y1-height)/2;
		}else{
			// center on screen
			DEVMODE mode;
			EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&mode);
			x=(mode.dmPelsWidth-width)/2;
			y=(mode.dmPelsHeight-height)/2;
		}
	}

	hWnd=CreateWindowEx(	style_ex,
							ClassName,
							sys_str(title),
							style,
							x<0?CW_USEDEFAULT:x,
							y<0?CW_USEDEFAULT:y,
							width<0?CW_USEDEFAULT:width,
							height<0?CW_USEDEFAULT:height,
							root?root->hWnd:HWND_DESKTOP,
							NULL,
							hui_win_instance,
							NULL);
	if (!hWnd){
		msg_error("new CHuiWindow - CreateWindowEx");
		msg_db_l(1);
		return;
	}
	ShowWindow(hWnd,SW_HIDE);


	// status bar
	status_bar=CreateWindow(STATUSCLASSNAME,_T(""),
							WS_CHILD, // | SBARS_SIZEGRIP,
							0,0,0,0,
							hWnd,NULL,hui_win_instance,NULL);

	// tool bar(s)
	tool_bar[HuiToolBarTop].hWnd=CreateWindowEx(0,TOOLBARCLASSNAME,NULL,
								//WS_CHILD | TBSTYLE_FLAT | CCS_LEFT | CCS_VERT,
								WS_CHILD | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | TBSTYLE_TOOLTIPS,
								0, 0, 0, 0,
								hWnd,(HMENU)0,hui_win_instance,NULL);
	//RECT rr;
	//SendMessage(tool_bar[HuiToolBarTop].hWnd, TB_SETROWS, MAKEWPARAM(13, FALSE), (LPARAM)&rr);
	TBADDBITMAP bitid;
	bitid.hInst = HINST_COMMCTRL;
	bitid.nID = IDB_STD_LARGE_COLOR;//IDB_STD_SMALL_COLOR;
	SendMessage(tool_bar[HuiToolBarTop].hWnd, TB_ADDBITMAP, 1, (long)&bitid);
	SendMessage(tool_bar[HuiToolBarTop].hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

	//if ((show)&&(!HuiCreateHiddenWindows))
	//	ShowWindow(hWnd,SW_SHOW);

#if 0
	if ((mode & HuiWinModeNix) > 0){
		/*gl_hwnd = CreateWindow(	_T("STATIC"),_T("test"),
							WS_CHILD | WS_VISIBLE | SS_BITMAP,//WS_VISIBLE | WS_CHILD,
							100,100,300,300,
							hWnd,NULL,hui_win_instance,NULL);*/
		/*gl_hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW,
							_T("STATIC"),
							_T("GL_Window "),
							WS_CHILD | WS_VISIBLE,
							100, 100,
							640, 480,
							hWnd,						// parent
							NULL,
							hui_win_instance,
							NULL);*/
		gl_hwnd=CreateWindow(	_T("STATIC"),_T(""),
											WS_CHILD | WS_VISIBLE | SS_BITMAP,
											100,100,200,200,
											hWnd,NULL,hui_win_instance,NULL);
		
		if (!gl_hwnd){
			HuiErrorBox(this, "Fehler", "gl_win");
			exit(0);
		}
	}
#endif
#endif
#ifdef HUI_API_GTK

	// creation
	if ((mode & HuiWinModeBGDialog) > 0){
		window=gtk_dialog_new();
	//	gtk_window_set_modal(GTK_WINDOW(window),false);
		gtk_dialog_set_has_separator(GTK_DIALOG(window),false);
		gtk_container_set_border_width(GTK_CONTAINER(window),0);
	}else
		window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
#ifdef HUI_OS_WINDOWS
	hWnd = (HWND)gdk_win32_drawable_get_handle(window->window);
#endif

	gtk_window_set_title(GTK_WINDOW(window), sys_str(title));
	if ((mode & HuiWinModeBGDialog) > 0){
		// dialog -> center on screen or root (if given)    ->  done by gtk....later
		/*if (Root){
			irect r=Root->GetOuterior();
			x = r.x1 + (r.x2-r.x1-width)/2;
			y = r.y1 + (r.y2-r.y1-height)/2;
		}else{
			GdkScreen *screen=gtk_window_get_screen(GTK_WINDOW(window));
			x=(gdk_screen_get_width(screen)-width)/2;
			y=(gdk_screen_get_height(screen)-height)/2;
		}
		//gtk_window_move(GTK_WINDOW(window),x,y);*/
		gtk_window_set_resizable(GTK_WINDOW(window),false);
		gtk_widget_set_size_request(window,width,height);
	}else{
		if ((x >= 0) && (y >= 0))
			gtk_window_move(GTK_WINDOW(window),x,y);
		gtk_window_resize(GTK_WINDOW(window),width,height);
	}

	if (Root)
		gtk_window_set_transient_for(GTK_WINDOW(window),GTK_WINDOW(Root->window));

	// icon
	if (file_test_existence(string(HuiAppDirectory, "Data/icon.ico")))
		gtk_window_set_icon_from_file(GTK_WINDOW(window), string(HuiAppDirectory, "Data/icon.ico"), NULL);

	// catch signals
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(&delete_event), NULL);
	//g_signal_connect(G_OBJECT(window), "expose-event", G_CALLBACK(&expose_event), NULL);
	g_signal_connect(G_OBJECT(window), "visibility-notify-event", G_CALLBACK(&visnot_event), NULL);
	g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(&scroll_event), NULL);
	g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(&key_press_event), NULL);
	g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(&key_release_event), NULL);
	g_signal_connect(G_OBJECT(window), "size-request", G_CALLBACK(&size_request), NULL);
	g_signal_connect(G_OBJECT(window), "motion-notify-event", G_CALLBACK(&motion_notify_event), NULL);
	g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(&button_press_event), NULL);
	g_signal_connect(G_OBJECT(window), "button-release-event", G_CALLBACK(&button_release_event), NULL);
	g_signal_connect(G_OBJECT(window), "focus-in-event", G_CALLBACK(&focus_in_event), NULL);
	int mask;
	g_object_get(G_OBJECT(window), "events", &mask, NULL);
	mask |= GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_VISIBILITY_NOTIFY_MASK; // GDK_POINTER_MOTION_HINT_MASK = "fewer motions"
	//mask = GDK_ALL_EVENTS_MASK;
	g_object_set(G_OBJECT(window), "events", mask, NULL);

	// fill in some stuff
	gtk_container_set_border_width(GTK_CONTAINER(window),0);
	if ((mode & HuiWinModeBGDialog) > 0)
		vbox=GTK_DIALOG(window)->vbox;
	else{
		vbox = gtk_vbox_new (FALSE, 0);
		gtk_container_add (GTK_CONTAINER (window), vbox);
		gtk_widget_show (vbox);
	}

	// menu bar
	menu_bar = gtk_menu_bar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), menu_bar, FALSE, FALSE, 0);
	gtk_widget_show (menu_bar);

	// tool bars
	tool_bar[HuiToolBarTop].tool_bar=gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tool_bar[HuiToolBarTop].tool_bar),true);
	tool_bar[HuiToolBarBottom].tool_bar=gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tool_bar[HuiToolBarBottom].tool_bar),true);
	tool_bar[HuiToolBarLeft].tool_bar=gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tool_bar[HuiToolBarLeft].tool_bar),true);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(tool_bar[HuiToolBarLeft].tool_bar),GTK_ORIENTATION_VERTICAL);
	tool_bar[HuiToolBarRight].tool_bar=gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tool_bar[HuiToolBarRight].tool_bar),true);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(tool_bar[HuiToolBarRight].tool_bar),GTK_ORIENTATION_VERTICAL);

	gtk_box_pack_start (GTK_BOX (vbox), tool_bar[HuiToolBarTop].tool_bar, FALSE, FALSE, 0);


	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	//gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	gtk_box_pack_start (GTK_BOX (hbox), tool_bar[HuiToolBarLeft].tool_bar, FALSE, FALSE, 0);

	if ((mode & HuiWinModeNix) > 0){
		fixed = NULL;
		// "drawable" (for opengl)
		gl_widget = gtk_drawing_area_new();
		g_signal_connect(G_OBJECT(gl_widget), "expose-event", G_CALLBACK(&expose_event_gl), NULL);
		gtk_container_add(GTK_CONTAINER(hbox), gl_widget);
		gtk_widget_show(gl_widget);
		gtk_widget_realize(gl_widget);
		GdkRectangle r;
		r.x = 0;
		r.y = 0;
		r.width = width;
		r.height = height;
		gdk_window_invalidate_region(gl_widget->window, gdk_region_rectangle(&r), false);
		gdk_window_process_all_updates();

		// prevent the toolbar from using keys...
		gtk_widget_set_can_focus(gl_widget, true);
		gtk_widget_grab_focus(gl_widget);
	}else{
		// "fixed" (for buttons etc)
		fixed = gtk_fixed_new();
		gtk_container_add(GTK_CONTAINER(hbox), fixed);
		gtk_widget_show(fixed);
		gl_widget = fixed;
		cur_cnt = fixed;
	}

	gtk_box_pack_start (GTK_BOX (hbox), tool_bar[HuiToolBarRight].tool_bar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), tool_bar[HuiToolBarBottom].tool_bar, FALSE, FALSE, 0);

	// status bar
	status_bar = gtk_statusbar_new ();
//	gtk_container_add(GTK_CONTAINER(vbox),status_bar);
	gtk_box_pack_start (GTK_BOX (vbox), status_bar, FALSE,FALSE, 0);

	gtk_num_menus=0;
	//###########################################################################################
	// never dare to ask, why there has to be a hidden edit box!
	/*if (bg_mode==HuiBGModeBlack){
		GtkWidget *edit=gtk_entry_new();
		gtk_widget_set_size_request(edit,1,1);
		gtk_fixed_put(GTK_FIXED(fixed),edit,0,0);
		gtk_widget_show(edit);
	}*/
	//###########################################################################################
#endif



// well,.... kind of ugly....
#ifdef HUI_API_WIN
	//if (bg_mode==HuiBGModeStyleDialog)
		//SetOuterior(irect(x,x+width+4,y,y+height+22));
#endif
	msg_db_l(1);
}


// dummy window
CHuiWindow::CHuiWindow(const char *title,int x,int y,int width,int height,message_function *mf)
{
	msg_db_r("CHuiWindow()",1);
	_HuiWindow_.push_back(this);

	MessageFunction=mf;
	UsedByNix=false;
	Allowed=true;
	AllowKeys=true;
	Root=NULL;
	TerrorChild=NULL;
	Menu=Popup=NULL;
	StatusBarEnabled=false;
	for (int i=0;i<4;i++){
		tool_bar[i].Enabled=false;
		tool_bar[i].TextEnabled=tool_bar[i].LargeIcons=true;
		tool_bar[i].Item.clear();
	}
	tb=&tool_bar[HuiToolBarTop];
	InputData.KeyBufferDepth=0;
	memset(&InputData.key,0,sizeof(InputData.key));
	OwnData.KeyBufferDepth=0;
	memset(&OwnData.key,0,sizeof(OwnData.key));
	TabCreationID=TabCreationPage=-1;

	IsHidden=false;
	Menu=NULL;
	ID=-1;
	uid=current_uid++;
	AllowInput=false; // allow only if ->Update() was called

#ifdef HUI_API_GTK

	// creation
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(window),sys_str(title));
	if ((x >= 0) && (y >= 0))
		gtk_window_move(GTK_WINDOW(window),x,y);
	gtk_window_resize(GTK_WINDOW(window),width,height);

	// icon
	if (file_test_existence(string(HuiAppDirectory, "Data/icon.ico")))
		gtk_window_set_icon_from_file(GTK_WINDOW(window), string(HuiAppDirectory, "Data/icon.ico"), NULL);
	
	// catch signals
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(&delete_event), NULL);
	g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(&key_press_event), NULL);
	g_signal_connect(G_OBJECT(window), "key-release-event", G_CALLBACK(&key_release_event), NULL);
	g_signal_connect(G_OBJECT(window), "size-request", G_CALLBACK(&size_request), NULL);
	g_signal_connect(G_OBJECT(window), "motion-notify-event", G_CALLBACK(&motion_notify_event), NULL);
	g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(&button_press_event), NULL);
	g_signal_connect(G_OBJECT(window), "button-release-event", G_CALLBACK(&button_release_event), NULL);
	g_signal_connect(G_OBJECT(window), "focus-in-event", G_CALLBACK(&focus_in_event), NULL);
	int mask;
	g_object_get(G_OBJECT(window), "events", &mask, NULL);
	mask |= GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK; // GDK_POINTER_MOTION_HINT_MASK = "fewer motions"
	g_object_set(G_OBJECT(window), "events", mask, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(window),0);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_widget_show (vbox);

	gl_widget = fixed = NULL;

	// menu bar
	menu_bar = gtk_menu_bar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), menu_bar, FALSE, FALSE, 0);
	gtk_widget_show (menu_bar);

	// tool bars
	tool_bar[HuiToolBarTop].tool_bar=gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tool_bar[HuiToolBarTop].tool_bar),true);
	tool_bar[HuiToolBarBottom].tool_bar=gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tool_bar[HuiToolBarBottom].tool_bar),true);
	tool_bar[HuiToolBarLeft].tool_bar=gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tool_bar[HuiToolBarLeft].tool_bar),true);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(tool_bar[HuiToolBarLeft].tool_bar),GTK_ORIENTATION_VERTICAL);
	tool_bar[HuiToolBarRight].tool_bar=gtk_toolbar_new();
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tool_bar[HuiToolBarRight].tool_bar),true);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(tool_bar[HuiToolBarRight].tool_bar),GTK_ORIENTATION_VERTICAL);

	gtk_box_pack_start (GTK_BOX (vbox), tool_bar[HuiToolBarTop].tool_bar, FALSE, FALSE, 0);


	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	gtk_widget_show (hbox);

	gtk_box_pack_start (GTK_BOX (hbox), tool_bar[HuiToolBarLeft].tool_bar, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), tool_bar[HuiToolBarRight].tool_bar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), tool_bar[HuiToolBarBottom].tool_bar, FALSE, FALSE, 0);

	// status bar
	status_bar = gtk_statusbar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), status_bar, FALSE,FALSE, 0);

	gtk_num_menus=0;
#endif
	msg_db_l(1);
}

CHuiWindow::~CHuiWindow()
{
	msg_db_r("~CHuiWindow",1);
	//msg_write((int)this);
	sHuiClosedWindow c;
	c.UID = uid;
	c.win = this;
	_HuiClosedWindow_.push_back(c);
#ifdef HUI_API_WIN
	if (Root){
		Root->Allowed=true;
		Root->TerrorChild=NULL;
		//Root->Activate();
		/*SetFocus(Root->hWnd);
		UpdateWindow(Root->hWnd);
		ShowWindow(Root->hWnd,SW_SHOW);*/
		//Root->Update();
		for (int i=0;i<Root->SubWindow.size();i++)
			if (Root->SubWindow[i]==this){
				Root->SubWindow.erase(Root->SubWindow.begin() + i);
				break;
			}
	}
	if (hWnd){
		for (int i=0;i<Control.size();i++)
			if (Control[i].hWnd)
				DestroyWindow(Control[i].hWnd);
		DestroyWindow(hWnd);
	}
#endif
#ifdef HUI_API_GTK
	gtk_widget_destroy(window);
#endif
	// unregister window
	//msg_write("unregistriere Fenster");
	for (int i=0;i<_HuiWindow_.size();i++)
		if (_HuiWindow_[i] == this){
			if (_HuiWindow_[i]->Root)
				if (_HuiWindow_[i]->Root->MessageFunction){
					HuiDoSingleMainLoop();
					_HuiWindow_[i]->Root->MessageFunction(HUI_WIN_RENDER);
				}
			_HuiWindow_.erase(_HuiWindow_.begin() + i);
			break;
		}
	msg_db_l(1);
}

// should be called after creating (and filling) the window to actually show it
void CHuiWindow::Update()
{
#ifdef HUI_API_WIN

	// cruel hack!!!!
	//     overrule windows behavior
	irect ro=GetOuterior();
	RECT ri;
	GetClientRect(hWnd,&ri);
	ro.x2+=(ro.x2-ro.x1)-ri.right;
	ro.y2+=(ro.y2-ro.y1)-ri.bottom;
	SetOuterior(ro);

	if (Menu)
		SetMenu(hWnd,Menu->hMenu);
	else
		SetMenu(hWnd,NULL);
	for (int i=0;i<Control.size();i++)
		if (Control[i].Kind==HuiKindTabControl)
			TabCtrl_SetCurSel(Control[i].hWnd,1);
	UpdateTabPages(this);
	for (int i=0;i<Control.size();i++)
		if (Control[i].Kind==HuiKindTabControl)
			TabCtrl_SetCurSel(Control[i].hWnd,0);
	UpdateWindow(hWnd);
	UpdateTabPages(this);
	if (IsHidden)
	    ShowWindow(hWnd,SW_HIDE);
	else
	    ShowWindow(hWnd,SW_SHOW);

	/*for (int i=0;i<NumControls;i++)
		if (Control[i].Kind==HuiKindListView){
			LVCOLUMN col;
			for (int j=0;j<128;j++)
				if (ListView_GetColumn(Control[i].hWnd,j,&col))
					ListView_SetColumnWidth(Control[i].hWnd,j,LVSCW_AUTOSIZE_USEHEADER);
				else
					break;
		}*/

	UpdateTabPages(this);
	ready = true;
#endif
#ifdef HUI_API_GTK
	// reset menu bar
	for (int i=0;i<gtk_num_menus;i++)
		gtk_container_remove(GTK_CONTAINER(menu_bar), GTK_WIDGET(gtk_menu[i]));
	gtk_menu.clear();
	gtk_num_menus = 0;
	// insert new menu
	if (Menu){
		gtk_widget_show(menu_bar);
		gtk_num_menus = Menu->Item.size();
		for (int i=0;i<Menu->Item.size();i++){
			sHuiMenuItem *it = &Menu->Item[i];
			gtk_menu.push_back(gtk_menu_item_new_with_label(it->Name));
			gtk_widget_show(gtk_menu[i]);
			if (it->SubMenu){
		//gtk_menu_item_remove_submenu(GTK_MENU_ITEM(it->g_item));
				//gtk_menu_detach(GTK_MENU(it->SubMenu->g_menu));
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(gtk_menu[i]), it->SubMenu->g_menu);//it->g_item);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), gtk_menu[i]);
			}else{
				gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), gtk_menu[i]);
				g_signal_connect(G_OBJECT(gtk_menu[i]),"activate",G_CALLBACK(CallbackMenu),(void*)it->ID);
			}
			gtk_widget_set_sensitive(gtk_menu[i], it->Enabled);
		}
	}else
		gtk_widget_hide(menu_bar);
	if (!IsHidden)
		gtk_widget_show(window);
#endif
	AllowInput=true;
}

// show/hide without closing the window
void CHuiWindow::Hide(bool hide)
{
#ifdef HUI_API_WIN
	if (hide)
	    ShowWindow(hWnd,SW_HIDE);
	else
	    ShowWindow(hWnd,SW_SHOW);
#endif
#ifdef HUI_API_GTK
	if (hide)
		gtk_widget_hide(window);
	else
		gtk_widget_show(window);
#endif
	IsHidden=hide;
}

// set the string in the title bar
void CHuiWindow::SetTitle(const char *title)
{
#ifdef HUI_API_WIN
	SetWindowText(hWnd,sys_str(title));
#endif
#ifdef HUI_API_GTK
	gtk_window_set_title(GTK_WINDOW(window),sys_str(title));
#endif
}

// identify window (for automatic title assignment with language strings)
void CHuiWindow::SetID(int id)
{
	ID=id;
	if ((HuiLanguaged)&&(id>=0))
		SetTitle(HuiGetLanguage(id));
}

// set the upper left corner of the window in screen corrdinates
void CHuiWindow::SetPosition(int x,int y)
{
#ifdef HUI_API_WIN
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	int w=lpwndpl.rcNormalPosition.right-lpwndpl.rcNormalPosition.left;
	int h=lpwndpl.rcNormalPosition.bottom-lpwndpl.rcNormalPosition.top;
	// nicht maximiert!!
	lpwndpl.showCmd=SW_SHOW;
	lpwndpl.rcNormalPosition.left=x;
	lpwndpl.rcNormalPosition.top=y;
	lpwndpl.rcNormalPosition.right=x+w;
	lpwndpl.rcNormalPosition.bottom=y+h;
	SetWindowPlacement(hWnd,&lpwndpl);
#endif
#ifdef HUI_API_GTK
	gtk_window_move(GTK_WINDOW(window),x,y);
#endif
}

// align window relative to another window (like..."top right corner")
void CHuiWindow::SetPositionSpecial(CHuiWindow *win,int mode)
{
	irect rp=win->GetOuterior();
	irect ro=GetOuterior();
	int x=ro.x1,y=ro.y1;
	if ((mode & HuiLeft)>0)
		x=rp.x1 + 2;
	if ((mode & HuiRight)>0)
		x=rp.x2 - (ro.x2-ro.x1) -2;
	if ((mode & HuiTop)>0)
		y=rp.y1 + 20;
	if ((mode & HuiBottom)>0)
		y=rp.y2 - (ro.y2-ro.y1) -2;
	SetPosition(x,y);
}

// set the current window position and size (including the frame and menu/toolbars...)
//    if maximized this will un-maximize the window!
void CHuiWindow::SetOuterior(irect rect)
{
#ifdef HUI_API_WIN
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	// not maximized!!
	lpwndpl.showCmd=SW_SHOW;
	lpwndpl.rcNormalPosition.left=rect.x1;
	lpwndpl.rcNormalPosition.top=rect.y1;
	lpwndpl.rcNormalPosition.right=rect.x2;
	lpwndpl.rcNormalPosition.bottom=rect.y2;
	SetWindowPlacement(hWnd,&lpwndpl);
#endif
#ifdef HUI_API_GTK
	gtk_window_unmaximize(GTK_WINDOW(window));
	gtk_window_move(GTK_WINDOW(window),rect.x1,rect.y1);
	gtk_window_resize(GTK_WINDOW(window),rect.x2-rect.x1,rect.y2-rect.y1);
#endif
}

// get the current window position and size (including the frame and menu/toolbars...)
irect CHuiWindow::GetOuterior()
{
	irect r;
#ifdef HUI_API_WIN
	RECT rect;
	GetWindowRect(hWnd,&rect);
	r.x1=rect.left;
	r.y1=rect.top;
	r.x2=rect.right;
	r.y2=rect.bottom;
#endif
#ifdef HUI_API_GTK
	gtk_window_get_position(GTK_WINDOW(window),&r.x1,&r.y1);
	gtk_window_get_size(GTK_WINDOW(window),&r.x2,&r.y2);
	r.x2+=r.x1;
	r.y2+=r.y1;
#endif
	return r;
}

// set the window position and size it had wouldn't it be maximized (including the frame and menu/toolbars...)
//    if not maximized this behaves like <SetOuterior>
void CHuiWindow::SetOuteriorDesired(irect rect)
{
#ifdef HUI_API_WIN
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	lpwndpl.rcNormalPosition.left=rect.x1;
	lpwndpl.rcNormalPosition.top=rect.y1;
	lpwndpl.rcNormalPosition.right=rect.x2;
	lpwndpl.rcNormalPosition.bottom=rect.y2;
	SetWindowPlacement(hWnd,&lpwndpl);
#endif
#ifdef HUI_API_GTK
	// bad hack
	bool maximized = (gdk_window_get_state(window->window) & GDK_WINDOW_STATE_MAXIMIZED) > 0;
	if (maximized)
		gtk_window_unmaximize(GTK_WINDOW(window));
	gtk_window_move(GTK_WINDOW(window),rect.x1,rect.y1);
	gtk_window_resize(GTK_WINDOW(window),rect.x2-rect.x1,rect.y2-rect.y1);
	if (maximized)
		gtk_window_maximize(GTK_WINDOW(window));
#endif
}

// get the window position and size it had wouldn't it be maximized (including the frame and menu/toolbars...)
//    if not maximized this behaves like <GetOuterior>
irect CHuiWindow::GetOuteriorDesired()
{
	irect r;
#ifdef HUI_API_WIN
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	r.x1=lpwndpl.rcNormalPosition.left;
	r.y1=lpwndpl.rcNormalPosition.top;
	r.x2=lpwndpl.rcNormalPosition.right;
	r.y2=lpwndpl.rcNormalPosition.bottom;
#endif
#ifdef HUI_API_GTK
	// bad hack
	bool maximized = (gdk_window_get_state(window->window) & GDK_WINDOW_STATE_MAXIMIZED) > 0;
	if (maximized){
		// very nasty hack   m(-_-)m
		r .x1 = 50;
		r .y1 = 50;
		r .x2 = r.x1 + 800;
		r .y2 = r.y1 + 600;
	}else{
		gtk_window_get_position(GTK_WINDOW(window),&r.x1,&r.y1);
		gtk_window_get_size(GTK_WINDOW(window),&r.x2,&r.y2);
		r.x2+=r.x1;
		r.y2+=r.y1;
	}
	/*if (maximized){
		gtk_window_unmaximize(GTK_WINDOW(window));
		for (int i=0;i<5;i++)
			HuiDoSingleMainLoop();
	}
	gtk_window_get_position(GTK_WINDOW(window),&r.x1,&r.y1);
	gtk_window_get_size(GTK_WINDOW(window),&r.x2,&r.y2);
	r.x2+=r.x1;
	r.y2+=r.y1;
	if (maximized){
		gtk_window_maximize(GTK_WINDOW(window));
		for (int i=0;i<20;i++)
			HuiDoSingleMainLoop();
	}*/
#endif
	return r;
}

int _tool_bar_size(sHuiToolBar *tool_bar)
{
	int s=32-4;
	if (tool_bar->LargeIcons)
		s+=8;
	if (tool_bar->TextEnabled)
		s+=16;
	return s;
}

// get the "usable" part of the window: controllers/graphics area
irect CHuiWindow::GetInterior()
{
	irect r;
#ifdef HUI_API_WIN
	RECT WindowClient,ToolBarRect;
	GetClientRect(hWnd,&WindowClient);
	POINT p;
	p.x=WindowClient.left;
	p.y=WindowClient.top;
	ClientToScreen(hWnd,&p);
	r.x1=p.x;
	r.y1=p.y;
	p.x=WindowClient.right;
	p.y=WindowClient.bottom;
	ClientToScreen(hWnd,&p);
	r.x2=p.x;
	r.y2=p.y;
	if (tool_bar[HuiToolBarTop].Enabled){
		SendMessage(tool_bar[HuiToolBarTop].hWnd,TB_AUTOSIZE,0,0);
		GetWindowRect(tool_bar[HuiToolBarTop].hWnd,&ToolBarRect);
		int tbh=ToolBarRect.bottom-ToolBarRect.top;
		r.y1+=tbh;
	}
#endif
#ifdef HUI_API_GTK
	gtk_window_get_position(GTK_WINDOW(window), &r.x1, &r.y1);
	gtk_window_get_size(GTK_WINDOW(window), &r.x2, &r.y2);
	r.x2 += r.x1;
	r.y2 += r.y1;
	GtkWidget *interior = (fixed == NULL) ? gl_widget : fixed;
	if (interior){
		int w, h, x, y;
		gdk_window_get_position(interior->window, &x, &y);
		gdk_window_get_size(interior->window, &w, &h);
		r.x1 += x;
		r.y1 += y;
		r.x2 = r.x1 + w;
		r.y2 = r.y1 + h;
	}
#endif
	return r;
}

void CHuiWindow::ShowCursor(bool show)
{
#ifdef HUI_API_WIN
	int s=::ShowCursor(show);
	if (show){
		while(s<0)
			s=::ShowCursor(show);
	}else{
		while(s>=0)
			s=::ShowCursor(show);
	}
#endif
#ifdef HUI_API_GTK
	if (show)
		gdk_window_set_cursor(vbox->window,NULL);
	else
		gdk_window_set_cursor(vbox->window,(GdkCursor*)invisible_cursor);
#endif
}

// relative to Interior
void CHuiWindow::SetCursorPos(int x,int y)
{
	irect ri = GetInterior();
	irect ro = GetOuterior();
#ifdef HUI_API_WIN
	::SetCursorPos(x + ri.x1, y + ri.y1);
#endif
#ifdef HUI_API_GTK
	#ifdef HUI_OS_LINUX
		//XWarpPointer(hui_x_display, None, GDK_WINDOW_XWINDOW(gl_widget->window), 0, 0, 0, 0, x, y);
		XWarpPointer(hui_x_display, None, GDK_WINDOW_XID(window->window), 0, 0, 0, 0, x - ro.x1 + ri.x1, y - ro.y1 + ri.y1);
		XFlush(hui_x_display);
	#endif
#endif
	InputData.x = (float)x;
	InputData.y = (float)y;
	InputData.dx = 0;
	InputData.dy = 0;
}

void CHuiWindow::SetMaximized(bool maximized)
{
#ifdef HUI_API_WIN
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	lpwndpl.showCmd=(maximized?SW_SHOWMAXIMIZED:SW_SHOW);
	SetWindowPlacement(hWnd,&lpwndpl);
#endif
#ifdef HUI_API_GTK
	if (maximized)
		gtk_window_maximize(GTK_WINDOW(window));
	else
		gtk_window_unmaximize(GTK_WINDOW(window));
#endif
}

bool CHuiWindow::IsMaximized()
{
#ifdef HUI_API_WIN
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	return (lpwndpl.showCmd==SW_SHOWMAXIMIZED);
#endif
#ifdef HUI_API_GTK
	int state=gdk_window_get_state(window->window);
	return ((state & GDK_WINDOW_STATE_MAXIMIZED)>0);
#endif
}

bool CHuiWindow::IsMinimized()
{
#ifdef HUI_API_WIN
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	return ((lpwndpl.showCmd==SW_SHOWMINIMIZED)||(lpwndpl.showCmd==SW_MINIMIZE));
#endif
#ifdef HUI_API_GTK
	int state=gdk_window_get_state(window->window);
	return ((state & GDK_WINDOW_STATE_ICONIFIED)>0);
#endif
}

void CHuiWindow::SetFullscreen(bool fullscreen)
{
#ifdef HUI_API_WIN
		if (fullscreen){
			// save window data
			WindowStyle = GetWindowLong(hWnd,GWL_STYLE);
			//hMenu=GetMenu(hWnd);
			GetWindowRect(hWnd,&WindowBounds);
			GetClientRect(hWnd,&WindowClient);
			DWORD style = WS_POPUP|WS_SYSMENU|WS_VISIBLE;
			//SetWindowLong(hWnd,GWL_STYLE,WS_POPUP);
			SetWindowLong(hWnd,GWL_STYLE,style);

			WINDOWPLACEMENT wpl;
			GetWindowPlacement(hWnd,&wpl);
			wpl.rcNormalPosition.left=0;
			wpl.rcNormalPosition.top=0;
			wpl.rcNormalPosition.right=1024;//xres;
			wpl.rcNormalPosition.bottom=768;//yres;
			AdjustWindowRect(&wpl.rcNormalPosition, style, FALSE);
			SetWindowPlacement(hWnd,&wpl);
		}
#endif
#ifdef HUI_API_GTK
	if (fullscreen)
		gtk_window_fullscreen(GTK_WINDOW(window));
	else
		gtk_window_unfullscreen(GTK_WINDOW(window));
#endif
}

void CHuiWindow::EnableStatusBar(bool enabled)
{
#ifdef HUI_API_WIN
	if (enabled)
		ShowWindow(status_bar,SW_SHOW);
	else
		ShowWindow(status_bar,SW_HIDE);
#endif
#ifdef HUI_API_GTK
	if (enabled)
	    gtk_widget_show(status_bar);
	else
	    gtk_widget_hide(status_bar);
#endif
	StatusBarEnabled=enabled;
}

void CHuiWindow::SetStatusText(const char *str)
{
#ifdef HUI_API_WIN
	SendMessage(status_bar,SB_SETTEXT,0,(LPARAM)sys_str(str));
#endif
#ifdef HUI_API_GTK
	gtk_statusbar_push(GTK_STATUSBAR(status_bar),0,sys_str(str));
#endif
}

void CHuiWindow::EnableToolBar(bool enabled)
{
#ifdef HUI_API_WIN
	if (tb!=&tool_bar[HuiToolBarTop])
		return;
	if (enabled)
		ShowWindow(tb->hWnd, SW_SHOW);
	else
		ShowWindow(tb->hWnd, SW_HIDE);

	if (enabled){
		RECT ToolBarRect;
		SendMessage(tool_bar[HuiToolBarTop].hWnd,TB_AUTOSIZE,0,0);
		GetWindowRect(tool_bar[HuiToolBarTop].hWnd,&ToolBarRect);
		int tbh=ToolBarRect.bottom-ToolBarRect.top;
		cdy+=tbh;
	}
#endif
#ifdef HUI_API_GTK
	if (enabled)
		gtk_widget_show(tb->tool_bar);
	else
		gtk_widget_hide(tb->tool_bar);
#endif
	tb->Enabled=enabled;
}

void CHuiWindow::ToolBarSetCurrent(int index)
{
#ifdef HUI_API_WIN
	index=HuiToolBarTop; // ... m(-_-)m
#endif
	tb=&tool_bar[index];
}

void CHuiWindow::ToolBarConfigure(bool text_enabled,bool large_icons)
{
#ifdef HUI_API_WIN
	//SendMessage(tb->hWnd,TB_SETBUTTONSIZE,0,MAKELPARAM(80,30);
	SendMessage(tb->hWnd,TB_SETMAXTEXTROWS,(WPARAM)(text_enabled?1:0),0);

	
	/*TBADDBITMAP bitid;
	bitid.hInst = HINST_COMMCTRL;
	bitid.nID = large_icons?IDB_STD_LARGE_COLOR:IDB_STD_SMALL_COLOR;
	SendMessage(tb->hWnd, TB_ADDBITMAP, 1, (long)&bitid);
	SendMessage(tb->hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);*/
#endif
#ifdef HUI_API_GTK
	gtk_toolbar_set_style(GTK_TOOLBAR(tb->tool_bar),text_enabled?GTK_TOOLBAR_BOTH:GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(tb->tool_bar),large_icons?GTK_ICON_SIZE_LARGE_TOOLBAR:GTK_ICON_SIZE_SMALL_TOOLBAR);
#endif
	tb->TextEnabled=text_enabled;
	tb->LargeIcons=large_icons;
}

#ifdef HUI_API_WIN
static HWND _win_cur_hwnd_;
static char win_image_buffer[1048576];
int win_get_tb_image_id(sHuiToolBar *tb,int image)
{
	CHECKJPEGFORMAT;
	if (image>=1024){
		HBITMAP hbmp;
		int size=(tb->LargeIcons?24:16);
		if (strstr(hui_image_file[image-1024].c_str(),".png")){
			msg_todo("Windows support for png images... trying fallback to bmp");
			char filename[256];
			strcpy(filename,hui_image_file[image-1024].c_str());
			strcpy(strstr(filename,".png"),".bmp");
			//msg_write(filename);
			return win_get_tb_image_id(tb,HuiLoadImage(filename));
		}else if (strstr(hui_image_file[image-1024].c_str(),".ico")){
			HICON hIcon;
			//msg_write("ico  :(");
			hIcon = (HICON)LoadImage(	NULL,sys_str_f(hui_image_file[image-1024].c_str()),
										IMAGE_ICON,
										size,size,LR_LOADFROMFILE);
			//msg_write((int)hbmp);

			ICONINFO iconinfo;

			//hIcon = LoadIcon(NULL, sys_str_f(hui_image_file[image-1024].c_str()));
			if (hIcon==NULL){
				msg_error("LoadImage ico");
				msg_write(GetLastError());
			}
			//msg_write((int)hIcon);
			GetIconInfo(hIcon, &iconinfo);
			hbmp = iconinfo.hbmColor;
			//msg_write((int)hbmp);

		}else /*if (strstr(hui_image_file[image-1024].c_str(),".bmp"))*/{
			hbmp = (HBITMAP)LoadImage(	NULL,sys_str_f(hui_image_file[image-1024].c_str()),
										IMAGE_BITMAP,
										size,size,LR_LOADFROMFILE);//|LR_LOADTRANSPARENT);
		}
		TBADDBITMAP bitid;
		bitid.hInst = NULL;
		bitid.nID = (UINT)hbmp;
		int r=SendMessage(tb->hWnd, TB_ADDBITMAP, 1, (long)&bitid);
		//msg_write(r);
		return r;
	}
	if (image==HuiImageOpen)	return STD_FILEOPEN;
	if (image==HuiImageNew)		return STD_FILENEW;
	if (image==HuiImageSave)	return STD_FILESAVE;

	if (image==HuiImageCopy)	return STD_COPY;
	if (image==HuiImagePaste)	return STD_PASTE;
	if (image==HuiImageCut)		return STD_CUT;
	if (image==HuiImageDelete)	return STD_DELETE;
	if (image==HuiImageFind)	return STD_FIND;

	if (image==HuiImageRedo)	return STD_REDOW;
	if (image==HuiImageUndo)	return STD_UNDO;
	if (image==HuiImagePreferences)	return STD_PROPERTIES;

	if (image==HuiImageHelp)	return STD_HELP;
	if (image==HuiImagePrint)	return STD_PRINT;

	return STD_FILENEW;
}
#endif

// just a helper function
void AddToolBarItem(sHuiToolBar *tb,int id,int type,CHuiMenu *menu)
{
	sHuiToolBarItem i;
	i.ID = id;
	i.Kind = type;
	i.Enabled = true;
	i.menu = menu;
	tb->Item.push_back(i);
}

// add a default button
void CHuiWindow::ToolBarAddItem(const char *title,const char *tool_tip,int image,int id)
{
#ifdef HUI_API_WIN
	if (tb!=&tool_bar[HuiToolBarTop])
		return;
	TBBUTTON tbb;
	ZeroMemory(&tbb,sizeof(tbb));
	_win_cur_hwnd_=hWnd;
	tbb.iBitmap = win_get_tb_image_id(tb,image);
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = TBSTYLE_BUTTON;
	tbb.iString = (int)sys_str(title);
	tbb.idCommand = id;
	SendMessage(tb->hWnd, TB_ADDBUTTONS, 1, (LPARAM)&tbb);
#endif
#ifdef HUI_API_GTK
	GtkWidget *im=(GtkWidget*)get_gtk_image(image,true);
	gtk_widget_show(im);
	GtkToolItem *it=gtk_tool_button_new(im,sys_str(title));
	gtk_tool_item_set_tooltip(it,gtk_tooltips_new(),sys_str(tool_tip),sys_str(tool_tip));
	gtk_tool_item_set_homogeneous(it,true);
	gtk_widget_show(GTK_WIDGET(it));
	gtk_toolbar_insert(GTK_TOOLBAR(tb->tool_bar),it,-1);
	g_signal_connect(G_OBJECT(it),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddToolBarItem(tb,id,HuiToolButton,NULL);
#ifdef HUI_API_GTK
	tb->Item[tb->Item.size()-1].item=it;
#endif
}

// add a checkable button
void CHuiWindow::ToolBarAddItemCheckable(const char *title,const char *tool_tip,int image,int id)
{
#ifdef HUI_API_WIN
	if (tb!=&tool_bar[HuiToolBarTop])
		return;
	TBBUTTON tbb;
	ZeroMemory(&tbb,sizeof(tbb));
	_win_cur_hwnd_=hWnd;
	tbb.iBitmap = win_get_tb_image_id(tb,image);
	tbb.fsState = TBSTATE_ENABLED;
	tbb.fsStyle = TBSTYLE_CHECK;
	tbb.iString = (int)sys_str(title);
	tbb.idCommand = id;
	SendMessage(tb->hWnd, TB_ADDBUTTONS, 1, (LPARAM)&tbb);
#endif
#ifdef HUI_API_GTK
	GtkWidget *im=(GtkWidget*)get_gtk_image(image,true);
	gtk_widget_show(im);
	GtkToolItem *it=gtk_toggle_tool_button_new();
	gtk_tool_item_set_tooltip(it,gtk_tooltips_new(),sys_str(tool_tip),sys_str(tool_tip));
	gtk_tool_item_set_homogeneous(it,true);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(it),sys_str(title));
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(it),im);
	gtk_widget_show(GTK_WIDGET(it));
	gtk_toolbar_insert(GTK_TOOLBAR(tb->tool_bar),it,-1);
	g_signal_connect(G_OBJECT(it),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddToolBarItem(tb,id,HuiToolCheckable,NULL);
#ifdef HUI_API_GTK
	tb->Item[tb->Item.size()-1].item=it;
#endif
}

void CHuiWindow::ToolBarAddItemMenu(const char *title,const char *tool_tip,int image,CHuiMenu *menu,int id)
{
	if (!menu)
		return;
#ifdef HUI_API_WIN
	if (tb!=&tool_bar[HuiToolBarTop])
		return;
	ToolBarAddItem(title,tool_tip,image,id);
	return;
#endif
#ifdef HUI_API_GTK
	GtkWidget *im=(GtkWidget*)get_gtk_image(image,true);
	gtk_widget_show(im);
	GtkToolItem *it=gtk_menu_tool_button_new(im,sys_str(title));
	gtk_tool_item_set_tooltip(it,gtk_tooltips_new(),sys_str(tool_tip),sys_str(tool_tip));
	gtk_tool_item_set_homogeneous(it,true);
	gtk_widget_show(GTK_WIDGET(it));
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(it),menu->g_menu);
	gtk_toolbar_insert(GTK_TOOLBAR(tb->tool_bar),it,-1);
	g_signal_connect(G_OBJECT(it),"clicked",G_CALLBACK(CallbackControl),this);
	//g_signal_connect(G_OBJECT(menu->g_menu),"activate",G_CALLBACK(CallbackMenu),(void*)Menu->ItemID[i]);
#endif
	AddToolBarItem(tb,id,HuiToolMenu,menu);
#ifdef HUI_API_GTK
	tb->Item[tb->Item.size()-1].item=it;
#endif
}

// add a menu to the toolbar by resource id
void CHuiWindow::ToolBarAddItemMenuByID(const char *title,const char *tool_tip,int image,int menu_id,int id)
{
	CHuiMenu *menu=HuiCreateResourceMenu(menu_id);
	ToolBarAddItemMenu(title,tool_tip,image,menu,id);
}

void CHuiWindow::ToolBarAddSeparator()
{
#ifdef HUI_API_WIN
	if (tb!=&tool_bar[HuiToolBarTop])
		return;
	TBBUTTON tbb;
	ZeroMemory(&tbb,sizeof(tbb));
	tbb.fsStyle = TBSTYLE_SEP;
	SendMessage(tb->hWnd, TB_ADDBUTTONS, 1, (LPARAM)&tbb);
#endif
#ifdef HUI_API_GTK
	GtkToolItem *it=gtk_separator_tool_item_new();
	gtk_widget_show(GTK_WIDGET(it));
	gtk_toolbar_insert(GTK_TOOLBAR(tb->tool_bar),it,-1);
#endif
	AddToolBarItem(tb,-1,HuiToolSeparator,NULL);
#ifdef HUI_API_GTK
	tb->Item[tb->Item.size()-1].item=it;
#endif
}

// remove all items from the toolbar
void CHuiWindow::ToolBarReset()
{
#ifdef HUI_API_WIN
	if (tb!=&tool_bar[HuiToolBarTop])
		return;
	for (int i=0;i<tb->Item.size();i++)
		SendMessage(tb->hWnd,(UINT)TB_DELETEBUTTON,(WPARAM)0/*i_button*/,(LPARAM)0);
#endif
#ifdef HUI_API_GTK
	for (int i=0;i<tb->Item.size();i++)
		gtk_widget_destroy(GTK_WIDGET(tb->Item[i].item));
#endif
	tb->Item.clear();
}

// create and apply a toolbar bar resource id
void CHuiWindow::ToolBarSetByID(int id)
{
	msg_db_r("ToolBarSetByID",1);
	msg_db_m(i2s(id),1);
	if (id<0){
		msg_db_l(1);
		return;
	}
	for (int r=0;r<_HuiResource_.size();r++){
		sHuiResource *res = &_HuiResource_[r];
//		msg_write>Write(res->id);
		if (res->id == id){
			ToolBarReset();
			ToolBarConfigure(res->b_param[0], res->b_param[1]);
			for (int i=0;i<res->cmd.size();i++){
				sHuiResourceCommand *cmd = &res->cmd[i];
				if (cmd->type == HuiCmdMenuAddItem)
					ToolBarAddItem(get_lang(cmd->id, "", false), "", cmd->i_param[1], cmd->id);
				else if (cmd->type == HuiCmdMenuAddItemCheckable)
					ToolBarAddItemCheckable(get_lang(cmd->id, "", false), "", cmd->i_param[1], cmd->id);
				else if (cmd->type == HuiCmdMenuAddItemSeparator)
					ToolBarAddSeparator();
				else if (cmd->type == HuiCmdMenuAddItemPopup){
					char title[256];
					strcpy(title, get_lang(cmd->id, "", false));
					ToolBarAddItemMenuByID(title, "", cmd->i_param[1], cmd->i_param[2], cmd->id);
				}
			}
			EnableToolBar(true);
			msg_db_m(":)",1);
			msg_db_l(1);
			return;
		}
	}
	msg_error("ToolBarSetByID  :~~(");
	msg_db_l(1);
	return;
}

// give our window the focus....and try to focus the specified control item
void CHuiWindow::Activate(int control_id)
{
#ifdef HUI_API_WIN
	SetFocus(hWnd);
	WindowStyle=GetWindowLong(hWnd,GWL_STYLE);
	if ((WindowStyle & WS_MINIMIZE)>0)
		ShowWindow(hWnd,SW_RESTORE);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	SetForegroundWindow(hWnd);
	/*if (control_id>=0){
		for (int i=0;i<NumControls;i++)
			if (control_id==ControlID[i])
				SetFocus(Control[i]);
	}*/
#endif
#ifdef HUI_API_GTK
	gtk_widget_grab_focus(window);
	gtk_window_present(GTK_WINDOW(window));
	if (control_id>=0)
		for (int i=0;i<Control.size();i++)
			if (control_id==Control[i].ID)
				gtk_widget_grab_focus(Control[i].win);
#endif
}

bool CHuiWindow::IsActive(bool include_sub_windows)
{
	bool ia=false;
#ifdef HUI_API_WIN
	ia=(GetActiveWindow()==hWnd);
#endif
#ifdef HUI_API_GTK
	/*ghjghjghj
	// TODO!!!
	gtk_widget_grab_focus(window);
	gtk_window_present(GTK_WINDOW(window));*/
#endif
	if ((!ia)&&(include_sub_windows)){
		for (int i=0;i<SubWindow.size();i++)
			if (SubWindow[i]->IsActive(true))
				return true;
	}
	return ia;
}

bool CHuiWindow::GetKey(int k)
{	return InputData.key[k];	}

bool CHuiWindow::GetKeyDown(int k)
{	return ((InputData.key[k])&&(!InputData.key[k]));	}

bool CHuiWindow::GetKeyUp(int k)
{	return ((!InputData.key[k])&&(InputData.key[k]));	}

char CHuiWindow::GetKeyChar(int key)
{
	int i;
	if (key<0)	return 0;
	if ((GetKey(KEY_RCONTROL))||(GetKey(KEY_LCONTROL)))
		return 0;
	// shift
	if ((GetKey(KEY_RSHIFT))||(GetKey(KEY_LSHIFT))){
		for (i=0;i<26;i++)
			if (key==KEY_A+i)
				return 'A'+i;
		if (key==KEY_1)			return '!';
		if (key==KEY_2)			return '"';
		if (key==KEY_3)			return -89;
		if (key==KEY_4)			return '$';
		if (key==KEY_5)			return '%';
		if (key==KEY_6)			return '&';
		if (key==KEY_7)			return '/';
		if (key==KEY_8)			return '(';
		if (key==KEY_9)			return ')';
		if (key==KEY_0)			return '=';
		if (key==KEY_COMMA)		return ';';
		if (key==KEY_DOT)		return ':';
		if (key==KEY_ADD)		return '*';
		if (key==KEY_SUBTRACT)	return '_';
		if (key==KEY_SMALLER)	return '>';
		if (key==KEY_SZ)		return '?';
		/*if (key==KEY_AE)		return sys_str("&A")[0];
		if (key==KEY_OE)		return sys_str("&O")[0];
		if (key==KEY_UE)		return sys_str("&U")[0];*/
		if (key==KEY_FENCE)		return '\'';
		if (key==KEY_GRAVE)		return '°';
		if (key==KEY_SPACE)		return ' ';
		return 0;
	}
	// alt
	if (GetKey(KEY_RALT)){
		if (key==KEY_Q)			return '@';
		if (key==KEY_E)			return -128;
		if (key==KEY_Y)			return -91;
		if (key==KEY_2)			return -78;
		if (key==KEY_3)			return -77;
		if (key==KEY_7)			return '{';
		if (key==KEY_8)			return '[';
		if (key==KEY_9)			return ']';
		if (key==KEY_0)			return '}';
		if (key==KEY_ADD)		return '~';
		if (key==KEY_SMALLER)	return '|';
		if (key==KEY_SZ)		return '\\';
		return 0;
	}
	// normal
	for (i=0;i<26;i++)
		if (key==KEY_A+i)
			return 'a'+i;
	for (i=0;i<10;i++)
		if ((key==KEY_0+i)||(key==KEY_NUM_0+i))
			return '0'+i;
	if (key==KEY_COMMA)			return ',';
	if (key==KEY_DOT)			return '.';
	if (key==KEY_ADD)			return '+';
	if (key==KEY_SUBTRACT)		return '-';
	if (key==KEY_FENCE)			return '#';
	if (key==KEY_GRAVE)			return '^';
	if (key==KEY_NUM_ADD)		return '+';
	if (key==KEY_NUM_SUBTRACT)	return '-';
	if (key==KEY_NUM_MULTIPLY)	return '*';
	if (key==KEY_NUM_DIVIDE)	return '/';
	if (key==KEY_SMALLER)		return '<';
	/*if (key==KEY_SZ)			return sys_str("&s")[0];
	if (key==KEY_AE)			return sys_str("&a")[0];
	if (key==KEY_OE)			return sys_str("&o")[0];
	if (key==KEY_UE)			return sys_str("&u")[0];*/
	if (key==KEY_SPACE)			return ' ';
	if (key==KEY_TAB)			return '\t';
	if (key==KEY_RETURN)		return '\n';
	if (key==KEY_NUM_ENTER)		return '\n';
	// unbekannt:
	return 0;
}

int CHuiWindow::GetKeyRhythmDown()
{
	/*KeyBufferRead=true;
	if (_HuiWindow_->InputData.KeyBufferDepth>0){
		int k=_HuiWindow_->InputData.KeyBuffer[0];
		//msg_write>Write(GetKeyName(k));
		for (int i=0;i<_HuiWindow_->InputData.KeyBufferDepth-2;i++)
			_HuiWindow_->InputData.KeyBuffer[i]=_HuiWindow_->InputData.KeyBuffer[i+1];
		_HuiWindow_->InputData.KeyBufferDepth--;
		return k;
	}else
		return -1;*/
	for (int i=6;i<HUI_NUM_KEYS;i++)
		if (GetKeyDown(i))
			return i;
	return -1;
}





//----------------------------------------------------------------------------------
// creating control items




static bool hui_option_tree;
static bool hui_option_icons;
static bool hui_option_extended;
static bool hui_option_multiline;
static bool hui_option_alpha;
static bool hui_option_bold;
static bool hui_option_italic;
static bool hui_option_nobar;

static int NumPartStrings;
static char PartString[64][512], OptionString[512];

const char *ScanOptions(int id, const char *title)
{
	const char *title2 = get_lang(id, title);
	if (title2[0] == '!'){
		for (unsigned int i=0;i<strlen(title2);i++){
			if (title2[i] == HuiComboBoxSeparator){
				OptionString[i]=0;
				break;
			}
			OptionString[i] = title2[i];
		}
		hui_option_tree = strstr(OptionString, "tree");
		hui_option_icons = strstr(OptionString, "icons");
		hui_option_extended = strstr(OptionString, "extended");
		hui_option_multiline = strstr(OptionString, "multiline");
		hui_option_alpha = strstr(OptionString, "alpha");
		hui_option_bold = strstr(OptionString, "bold");
		hui_option_italic = strstr(OptionString, "italic");
		hui_option_nobar = strstr(OptionString, "nobar");
		return &title2[strlen(OptionString) + 1];
	}else{
		hui_option_tree = false;
		hui_option_icons = false;
		hui_option_extended = false;
		hui_option_multiline = false;
		hui_option_alpha = false;
		hui_option_bold = false;
		hui_option_italic = false;
		hui_option_nobar = false;
		return title2;
	}
}

void GetPartStrings(int id,const char *title)
{
	char str[2048];
	const char *title2 = ScanOptions(id,title);
	int l=0;
	NumPartStrings=0;
	for (unsigned int i=0;i<strlen(title2);i++){
		str[l]=title2[i];
		str[l+1]=0;
		if ((title2[i]==HuiComboBoxSeparator)||(i==strlen(title2)-1)){
			if (title2[i]==HuiComboBoxSeparator)
				str[l]=0;
			l=-1;
			strcpy(PartString[NumPartStrings++],str);
		}
		l++;
	}
}




// general control...just a helper function, don't use!!!
void AddControl(CHuiWindow *win,sHuiControl *c,int id,int kind)
{
#ifdef HUI_API_WIN
	if (c->hWnd)
		SendMessage(c->hWnd,(UINT)WM_SETFONT,(WPARAM)hui_win_default_font,(LPARAM)TRUE);
	/*if (win->NumControl.size()==0)
		SetFocus(c);*/
	if (kind!=HuiKindColorButton)
		c->hWnd3=NULL;
#endif
	c->ID=id;
	c->Kind=kind;
	c->Enabled=true;
	c->TabID=win->TabCreationID;
	c->TabPage=win->TabCreationPage;
	/*c->x=0;
	c->y=0;*/
	win->Control.push_back(*c);
}

void CHuiWindow::AddButton(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
#ifdef HUI_API_WIN
	c.hWnd=CreateWindow(	_T("BUTTON"),get_lang_sys(id,title),
							WS_VISIBLE | WS_CHILD | (HuiUseFlatButtons?BS_FLAT:0),
							x+cdx,y+cdy,width,height,
							hWnd,NULL,hui_win_instance,NULL);
#endif
#ifdef HUI_API_GTK
	c.win=gtk_button_new_with_label(get_lang_sys(id,title));
	gtk_widget_set_size_request(c.win,width+2,height+2);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x-1,y-1);
	gtk_widget_show(c.win);
	g_signal_connect(G_OBJECT(c.win),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,&c,id,HuiKindButton);
}

void CHuiWindow::AddColorButton(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
	ScanOptions(id, title);
#ifdef HUI_API_WIN
	int bw=(width>25)?25:width;
	if (hui_option_alpha){
		bw=(width>50)?50:width;
		c.hWnd=CreateWindow(_T("BUTTON"),_T("rgb"),
							WS_VISIBLE | WS_CHILD | (HuiUseFlatButtons?BS_FLAT:0),
							x+width-bw+cdx,y+cdy,bw/2,height,
							hWnd,NULL,hui_win_instance,NULL);
		c.hWnd3=CreateWindow(	_T("BUTTON"),_T("a"),
								WS_VISIBLE | WS_CHILD | (HuiUseFlatButtons?BS_FLAT:0),
								x+width-bw/2+cdx,y+cdy,bw/2,height,
								hWnd,NULL,hui_win_instance,NULL);
	}else{
		c.hWnd=CreateWindow(_T("BUTTON"),_T("rgb"),
							WS_VISIBLE | WS_CHILD | (HuiUseFlatButtons?BS_FLAT:0),
							x+width-bw+cdx,y+cdy,bw,height,
							hWnd,NULL,hui_win_instance,NULL);
		c.hWnd3=NULL;
	}
	c.hWnd2=CreateWindow(	_T("STATIC"),_T(""),//get_lang_sys(id,title),
							WS_CHILD | WS_VISIBLE,
							x+cdx,y+cdy,width-bw,height,
							hWnd,NULL,hui_win_instance,NULL);
	c.Color[0]=0;
	c.Color[1]=0;
	c.Color[2]=0;
	c.Color[3]=255;
#endif
#ifdef HUI_API_GTK
	c.win=gtk_color_button_new();
	if (hui_option_alpha)
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(c.win),true);
	gtk_widget_set_size_request(c.win,width+2,height+2);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x-1,y-1);
	gtk_widget_show(c.win);
	g_signal_connect(G_OBJECT(c.win),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,&c,id,HuiKindColorButton);
}

void CHuiWindow::AddDefButton(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
#ifdef HUI_API_WIN
	c.hWnd=CreateWindow(_T("BUTTON"),get_lang_sys(id,title),
						WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | (HuiUseFlatButtons?BS_FLAT:0),
						x+cdx,y+cdy,width,height,
						hWnd,NULL,hui_win_instance,NULL);
#endif
#ifdef HUI_API_GTK
	c.win=gtk_button_new_with_label(get_lang_sys(id,title));
	gtk_widget_set_size_request(c.win,width+2,height+2);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x-1,y-1);
	gtk_widget_show(c.win);
	g_signal_connect(G_OBJECT(c.win),"clicked",G_CALLBACK(CallbackControl),this);
	gtk_widget_set_can_default(c.win, true);
	//gtk_widget_set_receives_default(c.win, true);
	gtk_widget_grab_default(c.win);
#endif
	AddControl(this,&c,id,HuiKindButton);
}

void CHuiWindow::AddCheckBox(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
#ifdef HUI_API_WIN
	c.hWnd=CreateWindow(_T("BUTTON"),get_lang_sys(id,title),
						WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX| (HuiUseFlatButtons?BS_FLAT:0),
						x+cdx,y+cdy,width,height,
						hWnd,NULL,hui_win_instance,NULL);
#endif
#ifdef HUI_API_GTK
	c.win=gtk_check_button_new_with_label(get_lang_sys(id,title));
	gtk_widget_set_size_request(c.win,width,height);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x,y);
	gtk_widget_show(c.win);
	g_signal_connect(G_OBJECT(c.win),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,&c,id,HuiKindCheckBox);
}

void CHuiWindow::AddText(const char *title,int x,int y,int width,int height,int id)
{
	const char *title2 = ScanOptions(id, title);
	sHuiControl c;
#ifdef HUI_API_WIN
	c.hWnd=CreateWindow(_T("STATIC"),sys_str(title2),
						WS_CHILD | WS_VISIBLE,
						x+cdx,y+cdy,width,height,
						hWnd,NULL,hui_win_instance,NULL);
#endif
#ifdef HUI_API_GTK
	c.win=gtk_label_new(sys_str(title2));
	if (hui_option_bold){
		char *markup = g_markup_printf_escaped ("<b>%s</b>", sys_str(title2));
		gtk_label_set_markup (GTK_LABEL(c.win), markup);
		g_free (markup);
	}else if (hui_option_italic){
		char *markup = g_markup_printf_escaped ("<i>%s</i>", sys_str(title2));
		gtk_label_set_markup (GTK_LABEL(c.win), markup);
		g_free (markup);
	}
	gtk_label_set_line_wrap(GTK_LABEL(c.win),true);
	gtk_widget_set_size_request(c.win,width,height+8);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x,y-4);
	gtk_widget_show(c.win);
#endif
	AddControl(this,&c,id,HuiKindText);
}

void CHuiWindow::AddEdit(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
#ifdef HUI_API_WIN
	if (HuiUseFlatButtons){
		c.hWnd=CreateWindow(_T("EDIT"),sys_str(title),//get_lang_sys(id,title),
							WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER | (HuiMultiline?(ES_MULTILINE | ES_AUTOVSCROLL):0),
							x+cdx,y+cdy+2,width,height-4,
							hWnd,NULL,hui_win_instance,NULL);
	}else{
		c.hWnd=CreateWindowEx(	WS_EX_CLIENTEDGE,_T("EDIT"),sys_str(title),//get_lang_sys(id,title),
								//WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
								WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | (HuiMultiline?(ES_MULTILINE | ES_AUTOVSCROLL):0),// | ES_PASSWORD | ES_NUMBER,
								x+cdx,y+cdy,width,height,
								hWnd,NULL,hui_win_instance,NULL);
	}
#endif
#ifdef HUI_API_GTK
	if (HuiMultiline){
		GtkTextBuffer *tb=gtk_text_buffer_new(NULL);
		c.win=gtk_text_view_new_with_buffer(tb);
		GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_widget_show(scroll);
		gtk_container_add(GTK_CONTAINER(scroll), c.win);
		GtkWidget *frame=gtk_frame_new(NULL);
		gtk_widget_set_size_request(frame,width,height);
		gtk_fixed_put(GTK_FIXED(cur_cnt),frame,x,y);
		gtk_widget_show(frame);
		gtk_container_add(GTK_CONTAINER(frame),scroll);
		gtk_widget_show(c.win);
	}else{
		c.win=gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(c.win),sys_str(title));//get_lang_sys(id,title));
		gtk_widget_set_size_request(c.win,width,height);
		gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x,y);
		gtk_widget_show(c.win);
		gtk_entry_set_activates_default(GTK_ENTRY(c.win), true);

		// dumb but usefull test
		if (height>30){
			GtkStyle* style=gtk_widget_get_style(c.win);
			PangoFontDescription *font_desc=pango_font_description_copy(style->font_desc);
			pango_font_description_set_absolute_size(font_desc,height*PANGO_SCALE*0.95);
			gtk_widget_modify_font(c.win,font_desc);
		}
	}
	g_signal_connect(G_OBJECT(c.win),"changed",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,&c,id,HuiKindEdit);

#ifdef HUI_API_WIN
	if ((height>30)&&(!HuiMultiline)){
		HFONT f=CreateFont(height,0,0,0,FW_NORMAL,FALSE,FALSE,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,_T("MS Sans Serif"));
		//WinStandartFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

		SendMessage(Control[Control.size()-1].hWnd,(UINT)WM_SETFONT,(WPARAM)f,(LPARAM)TRUE);
	}
#endif
}

void CHuiWindow::AddGroup(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
#ifdef HUI_API_WIN
	c.hWnd=CreateWindow(_T("BUTTON"),get_lang_sys(id,title),
						WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
						x+cdx,y+cdy,width,height,
						hWnd,NULL,hui_win_instance,NULL);
#endif
#ifdef HUI_API_GTK
	c.win=gtk_frame_new(get_lang_sys(id,title));
	gtk_widget_set_size_request(c.win,width,height);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x,y);
	gtk_widget_show(c.win);
#endif
	AddControl(this,&c,id,HuiKindGroup);
}

void CHuiWindow::AddComboBox(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
#ifdef HUI_API_WIN
	c.hWnd=CreateWindow(_T("COMBOBOX"),get_lang_sys(id,title),
						WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | CBS_DISABLENOSCROLL,
						x+cdx,y+cdy+2,width,/*height*/500,
						hWnd,NULL,hui_win_instance,NULL);
	SendMessage(c.hWnd,(UINT)CB_RESETCONTENT,(WPARAM)0,(LPARAM)0);
#endif
#ifdef HUI_API_GTK
	c.win=gtk_combo_box_new_text();
	//gtk_combo_box_append_text(GTK_COMBO_BOX(c),title);

	gtk_widget_set_size_request(c.win,width+2,height+4);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x-1,y-2);
	gtk_widget_show(c.win);
	g_signal_connect(G_OBJECT(c.win),"changed",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,&c,id,HuiKindComboBox);
	GetPartStrings(id,title);
	for (int i=0;i<NumPartStrings;i++)
		AddString(id, PartString[i]);
	SetInt(id, 0);
}

void CHuiWindow::AddTabControl(const char *title,int x,int y,int width,int height,int id)
{
	//const char *title2 = ScanOptions(id, title);
	sHuiControl c;
#ifdef HUI_API_WIN
	if (HuiUseFlatButtons){
		// tabcontrol itself
		c.hWnd=CreateWindow(_T("SysTabControl32"),_T(""),
							WS_CHILD | WS_VISIBLE,// | TCS_BUTTONS | TCS_FLATBUTTONS,
							x+cdx,y+cdy,width,height,
							hWnd,NULL,hui_win_instance,NULL);
		/*c.hWnd2=CreateWindow(_T("BUTTON"),_T(""),
												WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
												x+cdx,y+cdy+16,width,height-16,
												hWnd,NULL,HuihInstance,NULL);*/
		// rectangle
		/*c.hWnd2=CreateWindow(_T("STATIC"),_T(""),
												WS_CHILD | WS_VISIBLE | SS_SUNKEN,
												x+cdx,y+cdy+24,width,height-24,
												hWnd,NULL,hui_win_instance,NULL);*/
	}else{
		c.hWnd=CreateWindow(	_T("SysTabControl32"),_T(""),
												WS_CHILD | WS_VISIBLE,
												x+cdx,y+cdy,width,height,
												hWnd,NULL,hui_win_instance,NULL);
	}
	GetPartStrings(id,title);
	for (int i=0;i<NumPartStrings;i++){
		TCITEM item;
		item.mask=TCIF_TEXT;
		item.pszText=(win_str)sys_str(PartString[i]);//const_cast<LPSTR>(PartString[i]);
		//msg_write(PartString[i]);
		TabCtrl_InsertItem(c.hWnd,i,&item);
	}
#endif
#ifdef HUI_API_GTK
	c.win=gtk_notebook_new();

	gtk_widget_set_size_request(c.win,width,height);
	gtk_fixed_put(GTK_FIXED(fixed),c.win,x,y);
	gtk_widget_show(c.win);

	GetPartStrings(id,title);
	for (int i=0;i<NumPartStrings;i++){
		GtkWidget *_fixed=gtk_fixed_new();
		gtk_widget_show(_fixed);	
		GtkWidget *label = gtk_label_new(sys_str(PartString[i]));
		gtk_notebook_append_page (GTK_NOTEBOOK(c.win), _fixed, label);
    }
	c.Selected=0;
	g_signal_connect(G_OBJECT(c.win),"switch-page",G_CALLBACK(CallbackTabControl),this);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(c.win), true);
	if (hui_option_nobar)
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(c.win), false);
#endif
#ifdef HUI_API_WIN
	c.x=x+cdx;
	c.y=y+cdy;
#endif
	//SetControlSelection(id,0);
	AddControl(this,&c,id,HuiKindTabControl);
}

void CHuiWindow::SetTabCreationPage(int id,int page)
{
	TabCreationID=id;
	TabCreationPage=page;
#ifdef HUI_API_WIN
	cdx=0;
	cdy=0;
	if (tool_bar[HuiToolBarTop].Enabled){
		RECT ToolBarRect;
		SendMessage(tool_bar[HuiToolBarTop].hWnd,TB_AUTOSIZE,0,0);
		GetWindowRect(tool_bar[HuiToolBarTop].hWnd,&ToolBarRect);
		int tbh=ToolBarRect.bottom-ToolBarRect.top;
		cdy=tbh;
	}
	if (id >= 0)
		for (int i=0;i<Control.size();i++)
			if (id==Control[i].ID){
				RECT r;
				memset(&r,0,sizeof(r));
				TabCtrl_AdjustRect(Control[i].hWnd,FALSE,&r);
				cdx=Control[i].x+r.left-4;
				cdy=Control[i].y+r.top-2;
			}
#endif
#ifdef HUI_API_GTK
	cur_cnt=fixed;
	if (id >= 0)
		for (int i=0;i<Control.size();i++)
			if (id==Control[i].ID){
				cur_cnt=gtk_notebook_get_nth_page(GTK_NOTEBOOK(Control[i].win),page);
			}
#endif
}

enum
{
   TITLE_COLUMN,
   AUTHOR_COLUMN,
   CHECKED_COLUMN,
   N_COLUMNS
};

#if 0
// debug!!!!
void CHuiWindow::AddListView_Test(const char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_API_WIN
#endif
#ifdef HUI_API_GTK

   GtkTreeStore *store;
   GtkWidget *tree;
   GtkTreeViewColumn *column;
   GtkCellRenderer *renderer;

   /* Create a model.  We are using the store model for now, though we
    * could use any other GtkTreeModel */
   store = gtk_tree_store_new (N_COLUMNS,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_BOOLEAN);

   /* custom function to fill the model with data */
	GtkTreeIter iter,par;
	gtk_tree_store_insert (store, &iter,NULL, 0);
	gtk_tree_store_set (store, &iter, 0,"hallo",1,"test",2,true,-1);	par=iter;
	gtk_tree_store_insert (store, &iter,&par, 1);
	gtk_tree_store_set (store, &iter, 0,"sdfsdf",1,"Michi",2,false,-1);
	gtk_tree_store_insert (store, &iter,NULL, 2);
	gtk_tree_store_set (store, &iter, 0,"hallo",1,"test",2,true,-1);
	gtk_tree_store_insert (store, &iter,NULL, 3);
	gtk_tree_store_set (store, &iter, 0,"sdfsdf",1,"Michi",2,false,-1);

   /* Create a view */
   tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   g_object_set (G_OBJECT (tree),
                 "border-width", 51,
                 NULL);

   /* The view now holds a reference.  We can get rid of our own
    * reference */
   //g_object_unref (G_OBJECT (store));

   /* Create a cell render and arbitrarily make it red for demonstration
    * purposes */
   renderer = gtk_cell_renderer_text_new ();
   /*g_object_set (G_OBJECT (renderer),
                 "foreground", "red",
                 NULL);*/

   /* Create a column, associating the "text" attribute of the
    * cell_renderer to the first column of the model */
   column = gtk_tree_view_column_new_with_attributes ("Author", renderer,
                                                      "text", AUTHOR_COLUMN,
                                                      NULL);

   /* Add the column to the view. */
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

   /* Second column.. title of the book. */
   renderer = gtk_cell_renderer_text_new ();
   g_object_set (G_OBJECT (renderer),
                 "editable", true,
                 "editable-set", true,
                 NULL);
   column = gtk_tree_view_column_new_with_attributes ("Title",
                                                      renderer,
                                                      "text", TITLE_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

   /* Last column.. whether a book is checked out. */
   renderer = gtk_cell_renderer_toggle_new ();
   column = gtk_tree_view_column_new_with_attributes ("Checked out",
                                                      renderer,
                                                      "active", CHECKED_COLUMN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

   /* Now we can manipulate the view just like any other GTK widget */
	c.win=tree;


//c.win=gtk_frame_new(get_lang_sys(id,title));

	gtk_widget_set_size_request(c.win,width,height);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x,y);
	gtk_widget_show(c.win);

#endif
	AddControl(this,id,HuiKindListView);
}
#endif

void CHuiWindow::AddListView(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
	GetPartStrings(id,title);
#ifdef HUI_API_WIN

	if (hui_option_icons){
	}else if (hui_option_tree){
		DWORD style=WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS;
		c.hWnd=CreateWindowEx((HuiUseFlatButtons?0:WS_EX_CLIENTEDGE),WC_TREEVIEW,_T(""),
												style | (HuiUseFlatButtons?( BS_FLAT ):0),
												x+cdx,y+cdy,width,height,
												hWnd,NULL,hui_win_instance,NULL);
	}else{
		DWORD style=WS_CHILD | WS_VISIBLE | LVS_REPORT | (HuiMultiline?LVS_SHOWSELALWAYS:LVS_SINGLESEL);
		c.hWnd=CreateWindowEx((HuiUseFlatButtons?0:WS_EX_CLIENTEDGE),WC_LISTVIEW,_T(""),
												style | (HuiUseFlatButtons?( WS_BORDER | BS_FLAT ):0),
												x+cdx,y+cdy,width,height,
												hWnd,NULL,hui_win_instance,NULL);

		for (int i=0;i<NumPartStrings;i++){
			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt = LVCFMT_LEFT;
			lvc.iSubItem = i;//num_cols;
			lvc.pszText = (win_str)sys_str(PartString[i]);
			lvc.cx = 100;
			ListView_InsertColumn(c.hWnd,i,&lvc);
		}
		ListView_SetExtendedListViewStyleEx(c.hWnd,0,LVS_EX_FULLROWSELECT /*| LVS_EX_GRIDLINES*/ | LVS_EX_HEADERDRAGDROP | (HuiUseFlatButtons?LVS_EX_FLATSB:0));
		//ListView_SetExtendedListViewStyleEx(c.hWnd,0,LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP | (HuiUseFlatButtons?LVS_EX_FLATSB:0));
		for (int i=0;i<NumPartStrings;i++)
			ListView_SetColumnWidth(c.hWnd,i,LVSCW_AUTOSIZE_USEHEADER);
	}
#endif
#ifdef HUI_API_GTK

	GtkWidget *sw=gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

	if (hui_option_icons){
		GtkListStore *model;
		model = gtk_list_store_new (2, G_TYPE_STRING,GDK_TYPE_PIXBUF);
		GtkWidget *iv=gtk_icon_view_new_with_model(GTK_TREE_MODEL(model));
		gtk_icon_view_set_text_column(GTK_ICON_VIEW(iv),0);
		gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(iv),1);
		gtk_icon_view_set_item_width(GTK_ICON_VIEW(iv),130);
		gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(iv),GTK_SELECTION_SINGLE);
		c.win=iv;
		// react on double click
		g_signal_connect(G_OBJECT(c.win),"item-activated",G_CALLBACK(CallbackControl2),this);
	}else if (hui_option_tree){
		GtkTreeStore *store;
		GtkWidget *tree;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		// "model"
		GType TypeList[64];
		for (int i=0;i<NumPartStrings;i++)
			TypeList[i]=G_TYPE_STRING;
		store = gtk_tree_store_newv(NumPartStrings,TypeList);
		// "view"
		tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
		//g_object_set(G_OBJECT(tree),"border-width",51,NULL);
		g_object_unref(G_OBJECT(store));
		for (int i=0;i<NumPartStrings;i++){
			renderer = gtk_cell_renderer_text_new ();
			column = gtk_tree_view_column_new_with_attributes(sys_str(PartString[i]),renderer,"text",i,NULL);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
		}
		c.win=tree;
		g_signal_connect(G_OBJECT(c.win),"row-activated",G_CALLBACK(CallbackControl3),this);
	}else{
		GtkListStore *store;
		GtkWidget *tree;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		// "model"
		GType TypeList[64];
		for (int i=0;i<NumPartStrings;i++)
			TypeList[i]=G_TYPE_STRING;
		store = gtk_list_store_newv(NumPartStrings,TypeList);
		// "view"
		tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
		//g_object_set(G_OBJECT(list),"border-width",51,NULL);
		g_object_unref(G_OBJECT(store));
		for (int i=0;i<NumPartStrings;i++){
			renderer = gtk_cell_renderer_text_new ();
			column = gtk_tree_view_column_new_with_attributes(sys_str(PartString[i]),renderer,"text",i,NULL);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
		}
		c.win=tree;
		g_signal_connect(G_OBJECT(c.win),"row-activated",G_CALLBACK(CallbackControl3),this);
	}
	if (hui_option_nobar)
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(c.win), false);

	GtkWidget *frame=gtk_frame_new(NULL);
	gtk_widget_set_size_request(frame,width-4,height-4);
	gtk_fixed_put(GTK_FIXED(cur_cnt),frame,x,y);
	gtk_widget_show(frame);
	//gtk_container_add(GTK_CONTAINER(frame),c.win);
	gtk_container_add(GTK_CONTAINER(frame),sw);
	gtk_container_add(GTK_CONTAINER(sw),c.win);
	gtk_widget_show(sw);
	gtk_widget_show(c.win);
#endif
	AddControl(this, &c, id, hui_option_icons ? HuiKindListViewIcons : (hui_option_tree ? HuiKindListViewTree : HuiKindListView));
}

void CHuiWindow::AddProgressBar(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
#ifdef HUI_API_WIN
	c.hWnd=CreateWindowEx(0,PROGRESS_CLASS,_T("test"),
											WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
											x+cdx,y+cdy,width,height,
											hWnd,NULL,hui_win_instance,NULL);
	// set range to (int)0 - (int)65535 ....   "int"  (-_-')
	//SendMessage(c.hWnd, PBM_SETRANGE, 0, MAKEPARAM(0,65535));
//	SendMessage(c.hWnd, PBM_SETRANGE, 0, 65535);

#endif
#ifdef HUI_API_GTK
	c.win=gtk_progress_bar_new();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(c.win),sys_str(title));
	gtk_widget_set_size_request(c.win,width,height);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x,y);
	gtk_widget_show(c.win);
	//g_signal_connect(G_OBJECT(c.win),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,&c,id,HuiKindProgressBar);
}

void CHuiWindow::AddSlider(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
#ifdef HUI_API_WIN
	//???
	c.hWnd=CreateWindowEx(0,PROGRESS_CLASS,_T("test"),
											WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
											x+cdx,y+cdy,width,height,
											hWnd,NULL,hui_win_instance,NULL);
	// set range to (int)0 - (int)65535 ....   "int"  (-_-')
	//SendMessage(c.hWnd, PBM_SETRANGE, 0, MAKEPARAM(0,65535));
//	SendMessage(c.hWnd, PBM_SETRANGE, 0, 65535);

#endif
#ifdef HUI_API_GTK
	if (height > width){
		c.win=gtk_vscale_new_with_range(0.0, 1.0, 0.0001);
		gtk_range_set_inverted(GTK_RANGE(c.win),true);
	}else
		c.win=gtk_hscale_new_with_range(0.0, 1.0, 0.0001);
	gtk_scale_set_draw_value(GTK_SCALE(c.win),false);
	gtk_widget_set_size_request(c.win,width,height);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x,y);
	gtk_widget_show(c.win);
	g_signal_connect(G_OBJECT(c.win),"value-changed",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,&c,id,HuiKindSlider);
}

void CHuiWindow::AddImage(const char *title,int x,int y,int width,int height,int id)
{
	sHuiControl c;
#ifdef HUI_API_WIN
	c.hWnd=CreateWindow(	_T("STATIC"),_T(""),
											WS_CHILD | WS_VISIBLE | SS_BITMAP,
											x+cdx,y+cdy,width,height,
											hWnd,NULL,hui_win_instance,NULL);
	HBITMAP bmpname = (HBITMAP)LoadImage(NULL, sys_str_f(title), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	SendMessage(c.hWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bmpname);
#endif
#ifdef HUI_API_GTK
	if (title[0] == '/')
		c.win=gtk_image_new_from_file(SysFileName(title));
	else
		c.win=gtk_image_new_from_file(SysFileName(string(HuiAppDirectory,title)));
	gtk_widget_set_size_request(c.win,width+2,height+2);
	gtk_fixed_put(GTK_FIXED(cur_cnt),c.win,x-1,y-1);
	gtk_widget_show(c.win);
#endif
	AddControl(this,&c,id,HuiKindImage);
}




//----------------------------------------------------------------------------------
// data exchanging functions for control items



// replace all the text
//    for all
void CHuiWindow::SetString(int id, const char *str)
{
	allow_signal_level++;
	//char *str2=sys_str(str);
#ifdef HUI_API_WIN
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
			if ((Control[i].Kind==HuiKindEdit)||(Control[i].Kind==HuiKindCheckBox)||(Control[i].Kind==HuiKindButton)||(Control[i].Kind==HuiKindText)||(Control[i].Kind==HuiKindGroup))
				SendMessage(Control[i].hWnd,WM_SETTEXT,(WPARAM)0,(LPARAM)sys_str(str));
			else if (Control[i].Kind==HuiKindComboBox)
				SendMessage(Control[i].hWnd,CB_ADDSTRING,(WPARAM)0,(LPARAM)sys_str(str));
			else if ((Control[i].Kind==HuiKindListView)||(Control[i].Kind==HuiKindListViewTree)){
				AddString(id,str);
			}
		}
#endif
#ifdef HUI_API_GTK
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
			if (Control[i].Kind==HuiKindText)
				gtk_label_set_text(GTK_LABEL(Control[i].win),sys_str(str));
			else if (Control[i].Kind==HuiKindEdit){
				if (GTK_CHECK_TYPE(Control[i].win,GTK_TYPE_TEXT_VIEW)){
					GtkTextBuffer *tb=gtk_text_view_get_buffer(GTK_TEXT_VIEW(Control[i].win));
					const char *str2=sys_str(str);
					gtk_text_buffer_set_text(tb,str2,strlen(str2));
				}else
					gtk_entry_set_text(GTK_ENTRY(Control[i].win),sys_str(str));
			}else if ((Control[i].Kind==HuiKindListView)||(Control[i].Kind==HuiKindListViewTree)){
				AddString(id, str);
			}else if (Control[i].Kind==HuiKindComboBox){
				gtk_combo_box_append_text(GTK_COMBO_BOX(Control[i].win),sys_str(str));
				Control[i]._item_.push_back(dummy_iter);
			}else if (Control[i].Kind==HuiKindProgressBar)
				gtk_progress_bar_set_text(GTK_PROGRESS_BAR(Control[i].win),sys_str(str));
			else if (Control[i].Kind==HuiKindButton)
				gtk_button_set_label(GTK_BUTTON(Control[i].win),sys_str(str));
		}
#endif
	if (ID==id)
		SetTitle(str);
	allow_signal_level--;
}

// replace all the text with a numerical value (int)
//    for all
// select an item
//    for ComboBox, TabControl, ListView?
void CHuiWindow::SetInt(int id, int n)
{
	allow_signal_level++;
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
			if ((Control[i].Kind==HuiKindEdit) || (Control[i].Kind==HuiKindText))
				SetString(id, i2s(n));
#ifdef HUI_API_WIN
			if (Control[i].Kind==HuiKindTabControl){
				TabCtrl_SetCurSel(Control[i].hWnd,n);
				UpdateTabPages(this);
			}else if (Control[i].Kind==HuiKindListView)
				SendMessage(Control[i].hWnd,(UINT)LVM_SETSELECTIONMARK,(WPARAM)n,(LPARAM)0);
			else if (Control[i].Kind==HuiKindComboBox)
				SendMessage(Control[i].hWnd,(UINT)CB_SETCURSEL,(WPARAM)n,(LPARAM)0);
#endif
#ifdef HUI_API_GTK
			if (Control[i].Kind==HuiKindTabControl){
				gtk_notebook_set_current_page(GTK_NOTEBOOK(Control[i].win),n);
				Control[i].Selected=n;
			}else if ((Control[i].Kind==HuiKindListView) || (Control[i].Kind==HuiKindListViewTree)){
				GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(Control[i].win));
				if (n >= 0){
					gtk_tree_selection_select_iter(sel, &Control[i]._item_[n]);
					GtkTreePath *path = gtk_tree_path_new_from_indices(n, -1);
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(Control[i].win), path, NULL, false, 0, 0);
					gtk_tree_path_free(path);
				}else
					gtk_tree_selection_unselect_all(sel);
			}else if (Control[i].Kind==HuiKindComboBox){
				gtk_combo_box_set_active(GTK_COMBO_BOX(Control[i].win), n);
			}
#endif
		}
	allow_signal_level--;
}

// replace all the text with a float
//    for all
void CHuiWindow::SetFloat(int id, float f)
{
	allow_signal_level++;
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
#ifdef HUI_API_WIN
			if (Control[i].Kind==HuiKindProgressBar)
				SendMessage(Control[i].hWnd, PBM_SETPOS, int(100.0f*f), 0);
			else
				SetString(id, f2s(f, NumFloatDecimals));
#endif
#ifdef HUI_API_GTK
			if (Control[i].Kind==HuiKindProgressBar)
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(Control[i].win),f);
			else if (Control[i].Kind==HuiKindSlider)
				gtk_range_set_value(GTK_RANGE(Control[i].win),f);
			else
				SetString(id, f2s(f, NumFloatDecimals));
#endif
		}
	allow_signal_level--;
}

void CHuiWindow::SetImage(int id,int image)
{
	allow_signal_level++;
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
#ifdef HUI_API_GTK
			if (Control[i].Kind==HuiKindButton){
				GtkWidget *im=(GtkWidget*)get_gtk_image(image,false);
				gtk_button_set_image(GTK_BUTTON(Control[i].win),im);
			}
#endif
		}
	allow_signal_level--;
}

#ifdef HUI_API_WIN
	static LVCOLUMN _col_;
	static LVITEM _lvI_;
	static TVITEM _tvi_;
#endif

// add a single line/string
//    for ComboBox, ListView, ListViewTree, ListViewIcons
void CHuiWindow::AddString(int id, const char *str)
{
	allow_signal_level++;
	//char *str2=sys_str(str);
#ifdef HUI_API_WIN
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
			if ((Control[i].Kind==HuiKindEdit)||(Control[i].Kind==HuiKindCheckBox)||(Control[i].Kind==HuiKindButton)||(Control[i].Kind==HuiKindText)||(Control[i].Kind==HuiKindGroup))
				SendMessage(Control[i].hWnd,WM_SETTEXT,(WPARAM)0,(LPARAM)sys_str(str));
			else if (Control[i].Kind==HuiKindComboBox)
				SendMessage(Control[i].hWnd,CB_ADDSTRING,(WPARAM)0,(LPARAM)sys_str(str));
			else if (Control[i].Kind==HuiKindListView){
				int line=ListView_GetItemCount(Control[i].hWnd),j;
				GetPartStrings(-1,str);
				for (j=0;j<NumPartStrings;j++){
					if (j==0){
						_lvI_.iItem = line;
						_lvI_.iSubItem = 0;
						_lvI_.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
						_lvI_.state = 0;
						_lvI_.stateMask = 0;
						_lvI_.pszText = (win_str)sys_str(PartString[j]);
						ListView_InsertItem(Control[i].hWnd, &_lvI_);
					}else
						ListView_SetItemText(Control[i].hWnd,line,j,(win_str)sys_str(PartString[j]));
				}
				if ((line<5)||(NumPartStrings==0)){
					for (j=0;j<128;j++)
						if (ListView_GetColumn(Control[i].hWnd,j,&_col_)){
							if (!ListView_SetColumnWidth(Control[i].hWnd,j,LVSCW_AUTOSIZE_USEHEADER))
								break;
						}else
							break;
					ShowWindow(Control[i].hWnd,SW_HIDE);
					ShowWindow(Control[i].hWnd,SW_SHOW);
				}
			}else if (Control[i].Kind==HuiKindListViewTree){
				GetPartStrings(-1,str);
				char tt[1024];
				strcpy(tt, PartString[0]);
				for (int j=1;j<NumPartStrings;j++)
					strcat(tt, string(" - ", PartString[j]));
				_tvi_.mask = TVIF_TEXT;
				_tvi_.pszText = (win_str)sys_str(tt);
				_tvi_.cchTextMax = sizeof(_tvi_.pszText)/sizeof(_tvi_.pszText[0]);

				TVINSERTSTRUCT tvins;
				tvins.item = _tvi_;
				tvins.hInsertAfter = TVI_ROOT;
				tvins.hParent = TVI_ROOT;
				Control[i]._item_.push_back((HWND)SendMessage(Control[i].hWnd,TVM_INSERTITEM,0,(LPARAM)&tvins));
			}
		}
#endif
#ifdef HUI_API_GTK
	GtkTreeIter iter;
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
			if (Control[i].Kind==HuiKindEdit)
				{}//gtk_entry_set_text(GTK_ENTRY(Control[i].win),sys_str(str));
			else if (Control[i].Kind==HuiKindComboBox){
				gtk_combo_box_append_text(GTK_COMBO_BOX(Control[i].win),sys_str(str));
				Control[i]._item_.push_back(dummy_iter);
			}else if (Control[i].Kind==HuiKindListViewTree){
				GetPartStrings(-1,str);
				GtkTreeStore *store=GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				gtk_tree_store_append(store, &iter,NULL);
				for (int j=0;j<NumPartStrings;j++)
					gtk_tree_store_set(store, &iter, j,sys_str(PartString[j]),-1);
				Control[i]._item_.push_back(iter);
			}else if (Control[i].Kind==HuiKindListView){
				GetPartStrings(-1,str);
				GtkListStore *store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				gtk_list_store_append(store, &iter);
				for (int j=0;j<NumPartStrings;j++)
					gtk_list_store_set(store, &iter, j,sys_str(PartString[j]),-1);
				Control[i]._item_.push_back(iter);
			}else if (Control[i].Kind==HuiKindListViewIcons){
				GetPartStrings(-1,str);
				GtkTreeModel *model=gtk_icon_view_get_model(GTK_ICON_VIEW(Control[i].win));
				GtkWidget *im=gtk_image_new_from_file(PartString[1]);
				gtk_list_store_append(GTK_LIST_STORE(model), &iter);
				gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0,sys_str(PartString[0]),1,gtk_image_get_pixbuf(GTK_IMAGE(im)),-1);
				Control[i]._item_.push_back(iter);
			}
		}
#endif
	allow_signal_level--;
}

// add a single line as a child in the tree of a ListViewTree
//    for ListViewTree
void CHuiWindow::AddChildString(int id, int parent_row, const char *str)
{
	allow_signal_level++;
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindListViewTree){
#ifdef HUI_API_WIN
				GetPartStrings(-1,str);
				char tt[1024];
				strcpy(tt, PartString[0]);
				for (int j=1;j<NumPartStrings;j++)
					strcat(tt, string(" - ", PartString[j]));
				TVITEM tvi;
				tvi.mask = TVIF_TEXT;
				tvi.pszText = (win_str)sys_str(tt);
				tvi.cchTextMax = sizeof(tvi.pszText)/sizeof(tvi.pszText[0]);

				TVINSERTSTRUCT tvins;
				tvins.item = tvi;
				tvins.hInsertAfter = (HTREEITEM)Control[i]._item_[Control[i]._item_.size()-1];
				tvins.hParent = (HTREEITEM)Control[i]._item_[parent_row];
				Control[i]._item_.push_back((HWND)SendMessage(Control[i].hWnd,TVM_INSERTITEM,0,(LPARAM)&tvins));
#endif
#ifdef HUI_API_GTK
				GtkTreeIter iter;
				GetPartStrings(-1,str);
				GtkTreeStore *store=GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				gtk_tree_store_append(store, &iter, &Control[i]._item_[parent_row]);
				for (int j=0;j<NumPartStrings;j++)
					gtk_tree_store_set(store, &iter, j,sys_str(PartString[j]),-1);
				Control[i]._item_.push_back(iter);
#endif
			}
	allow_signal_level--;
}

// change a single line in the tree of a ListViewTree
//    for ListViewTree
void CHuiWindow::ChangeString(int id,int row,const char *str)
{
	allow_signal_level++;
#ifdef HUI_API_WIN
#endif
#ifdef HUI_API_GTK
	for (int i=0;i<Control.size();i++)
		if (id == Control[i].ID){
			if (Control[i].Kind == HuiKindListViewTree){
				GetPartStrings(-1, str);
				GtkTreeStore *store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				for (int j=0;j<NumPartStrings;j++)
					gtk_tree_store_set(store, &Control[i]._item_[row], j, sys_str(PartString[j]), -1);
			}else if (Control[i].Kind == HuiKindListView){
				GetPartStrings(-1, str);
				GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				for (int j=0;j<NumPartStrings;j++)
					if (gtk_list_store_iter_is_valid(store, &Control[i]._item_[row]))
						gtk_list_store_set(store, &Control[i]._item_[row], j, sys_str(PartString[j]), -1);
			}
		}
#endif
	allow_signal_level--;
}

void CHuiWindow::SetColor(int id,int *c,bool use_alpha)
{
	allow_signal_level++;
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindColorButton){
#ifdef HUI_API_WIN
				for (int j=0;j<(use_alpha?4:3);j++)
					Control[i].Color[j]=c[j];
				// redraw color field...
				ShowWindow(Control[i].hWnd2,SW_HIDE);
				ShowWindow(Control[i].hWnd2,SW_SHOW);
#endif
#ifdef HUI_API_GTK
				GdkColor col;
				col.red=c[0]<<8;
				col.green=c[1]<<8;
				col.blue=c[2]<<8;
				gtk_color_button_set_color(GTK_COLOR_BUTTON(Control[i].win),&col);
				if (use_alpha)
					gtk_color_button_set_alpha(GTK_COLOR_BUTTON(Control[i].win),c[3]<<8);
#endif
			}
	allow_signal_level--;
}

static char ControlText[2048];//,ControlLine[2048];

// retrieve the text
//    for edit
const char *CHuiWindow::GetString(int id)
{
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindEdit){
#ifdef HUI_API_WIN
				/*strcpy(ControlText,"");
				int nl=SendMessage(Control[i].hWnd,(UINT)EM_GETLINECOUNT,(WPARAM)0,(LPARAM)0);
				for (int j=0;j<nl;j++){
					ControlLine[0]=0; // 2048 bytes Groesse...
					ControlLine[1]=8;
					SendMessage(Control[i].hWnd,(UINT)EM_GETLINE,(WPARAM)j,(LPARAM)ControlLine);
					strcat(ControlText,ControlLine);
					if (j<nl-1)
						strcat(ControlText,"\n");
				}*/
				TCHAR _temp_[2048];
				ZeroMemory(_temp_,sizeof(_temp_));
				SendMessage(Control[i].hWnd,(UINT)WM_GETTEXT,(WPARAM)2048,(LPARAM)_temp_);//ControlText);
				return de_sys_str(_temp_);//DeSysStr(ControlText);
#endif
#ifdef HUI_API_GTK
				if (GTK_CHECK_TYPE(Control[i].win,GTK_TYPE_TEXT_VIEW)){
					GtkTextBuffer *tb=gtk_text_view_get_buffer(GTK_TEXT_VIEW(Control[i].win));
					GtkTextIter is,ie;
					gtk_text_buffer_get_iter_at_offset(tb,&is,0);
					gtk_text_buffer_get_iter_at_offset(tb,&ie,-1);
					strcpy(ControlText,gtk_text_buffer_get_text(tb,&is,&ie,false));
				}else
					strcpy(ControlText,gtk_entry_get_text(GTK_ENTRY(Control[i].win)));
				return de_sys_str(ControlText);
#endif
			}
	strcpy(ControlText,"");
	return ControlText;
}

// retrieve the text as a numerical value (int)
//    for edit
// which item/line is selected?
//    for ComboBox, TabControl, ListView
int CHuiWindow::GetInt(int id)
{
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
			if ((Control[i].Kind==HuiKindEdit) || (Control[i].Kind==HuiKindText))
				return s2i(GetString(id));
#ifdef HUI_API_WIN
			if (Control[i].Kind==HuiKindTabControl)
				return TabCtrl_GetCurSel(Control[i].hWnd);
			else if (Control[i].Kind==HuiKindListView)
				return ListView_GetSelectionMark(Control[i].hWnd);
			else if (Control[i].Kind==HuiKindComboBox)
				return (int)SendMessage(Control[i].hWnd,(UINT)/*CB_GETCOUNT*/CB_GETCURSEL,(WPARAM)0,(LPARAM)0);
#endif
#ifdef HUI_API_GTK
			if (Control[i].Kind==HuiKindTabControl)
				return Control[i].Selected;//gtk_notebook_get_current_page(GTK_NOTEBOOK(Control[i].win));
			else if (Control[i].Kind==HuiKindComboBox)
				return gtk_combo_box_get_active(GTK_COMBO_BOX(Control[i].win));
			else if ((Control[i].Kind==HuiKindListView)||(Control[i].Kind==HuiKindListViewTree)){
				GtkTreeSelection *sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(Control[i].win));
				for (int j=0;j<Control[i]._item_.size();j++)
					if (gtk_tree_selection_iter_is_selected(sel,&Control[i]._item_[j]))
						return j;
				return -1;
			}else if (Control[i].Kind==HuiKindListViewIcons){
				GList *l=gtk_icon_view_get_selected_items(GTK_ICON_VIEW(Control[i].win));
				if (l){
					GtkTreePath *p=(GtkTreePath*)(l->data);
					g_list_free(l);
					int *indices=gtk_tree_path_get_indices(p),r;
					if (indices){
						r=indices[0];
						delete[](indices);
						return r;
					}else
						return -1;
				}else
					return -1;
			}
#endif
		}
	return -1;
}

// retrieve the text as a numerical value (float)
//    for edit
float CHuiWindow::GetFloat(int id)
{
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindSlider){
#ifdef HUI_API_GTK
				return gtk_range_get_value(GTK_RANGE(Control[i].win));
#endif
			}
	return s2f(GetString(id));
}

void CHuiWindow::GetColor(int id,int *c,bool use_alpha)
{
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindColorButton){
#ifdef HUI_API_WIN
				for (int j=0;j<(use_alpha?4:3);j++)
					c[j]=Control[i].Color[j];
#endif
#ifdef HUI_API_GTK
				GdkColor col;
				gtk_color_button_get_color(GTK_COLOR_BUTTON(Control[i].win),&col);
				c[0]=col.red>>8;
				c[1]=col.green>>8;
				c[2]=col.blue>>8;
				if (use_alpha)
					c[3]=gtk_color_button_get_alpha(GTK_COLOR_BUTTON(Control[i].win))>>8;
#endif
			}
}

// switch control to usable/unusable
//    for all
void CHuiWindow::Enable(int id,bool enabled)
{
	if (id<0)
		return;
	allow_signal_level++;
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
		    Control[i].Enabled=enabled;
#ifdef HUI_API_WIN
			EnableWindow(Control[i].hWnd,enabled);
			if (Control[i].Kind==HuiKindColorButton){
				if (Control[i].hWnd3)
					EnableWindow(Control[i].hWnd3,enabled);
				ShowWindow(Control[i].hWnd2,SW_HIDE);
				ShowWindow(Control[i].hWnd2,SW_SHOW);
			}
#endif
#ifdef HUI_API_GTK
			gtk_widget_set_sensitive(Control[i].win,enabled);
#endif
		}
	for (int t=0;t<4;t++)
		for (int i=0;i<tool_bar[t].Item.size();i++)
			if (id==tool_bar[t].Item[i].ID){
			    tool_bar[t].Item[i].Enabled=enabled;
#ifdef HUI_API_WIN
				if (t==HuiToolBarTop)
					SendMessage(tool_bar[t].hWnd,TB_ENABLEBUTTON,(WPARAM)id,(LPARAM)(enabled?TRUE:FALSE));
#endif
#ifdef HUI_API_GTK
				gtk_widget_set_sensitive(GTK_WIDGET(tool_bar[t].Item[i].item),enabled);
#endif
			}
	allow_signal_level--;
}

//    for all
bool CHuiWindow::IsEnabled(int id)
{
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			return Control[i].Enabled;
	for (int t=0;t<4;t++)
		for (int i=0;i<tool_bar[t].Item.size();i++)
			if (id==tool_bar[t].Item[i].ID)
				return tool_bar[t].Item[i].Enabled;
	return false;
}

// mark as "checked"
//    for CheckBox, ToolBarItemCheckable
void CHuiWindow::Check(int id,bool checked)
{
	allow_signal_level++;
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindCheckBox)
#ifdef HUI_API_WIN
				SendMessage(Control[i].hWnd,BM_SETCHECK,(WPARAM)(checked?BST_CHECKED:BST_UNCHECKED),(LPARAM)0);
	// BST_INDETERMINATE
#endif
#ifdef HUI_API_GTK
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Control[i].win),checked);
#endif
	for (int t=0;t<4;t++)
		for (int i=0;i<tool_bar[t].Item.size();i++)
			if (id==tool_bar[t].Item[i].ID)
				if (tool_bar[t].Item[i].Kind==HuiToolCheckable){
#ifdef HUI_API_WIN
					if (t==HuiToolBarTop)
						SendMessage(tool_bar[t].hWnd,TB_CHECKBUTTON,(WPARAM)id,(LPARAM)(checked?TRUE:FALSE));
#endif
#ifdef HUI_API_GTK
					gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(tool_bar[t].Item[i].item),checked);
#endif
				}
	allow_signal_level--;
}

// is marked as "checked"?
//    for CheckBox
bool CHuiWindow::IsChecked(int id)
{
#ifdef HUI_API_WIN
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindCheckBox)
				return SendMessage(Control[i].hWnd,BM_GETCHECK,(WPARAM)0,(LPARAM)0)==BST_CHECKED;
#endif
#ifdef HUI_API_GTK
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindCheckBox)
				return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Control[i].win));
#endif
	for (int t=0;t<4;t++)
		for (int i=0;i<tool_bar[t].Item.size();i++)
			if (id==tool_bar[t].Item[i].ID)
				if (tool_bar[t].Item[i].Kind==HuiToolCheckable){
#ifdef HUI_API_WIN
					if (t==HuiToolBarTop)
						return (SendMessage(tool_bar[t].hWnd,TB_ISBUTTONCHECKED,(WPARAM)i,(LPARAM)0)!=0);
#endif
#ifdef HUI_API_GTK
					return gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(tool_bar[t].Item[i].item));
#endif
				}
	return false;
}

// which lines are selected?
//    for ListView
int CHuiWindow::GetMultiSelection(int id,int *indices)
{
	int num_marked=0;
#ifdef HUI_API_WIN
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindListView){
				int num_lines=ListView_GetItemCount(Control[i].hWnd);

				for (int j=0;j<num_lines;j++){
					LVITEM lvI;
   					lvI.iItem = j;
					lvI.iSubItem = 0;
					lvI.mask = LVIF_STATE;
					lvI.state = 0;
					lvI.stateMask = LVIS_SELECTED;
					ListView_GetItem(Control[i].hWnd,&lvI);
					if (lvI.state>0){
						indices[num_marked]=j;
						num_marked++;
					}
				}
			}
#endif
#ifdef HUI_API_GTK
	indices[0] = GetInt(id);
	num_marked = (indices[0] >= 0) ? 1 : 0;
		//msg_write>Write("Todo:  CHuiWindow::GetControlSelectionM (Linux)");
#endif
	return num_marked;
}

// delete all the content
//    for ComboBox, ListView
void CHuiWindow::Reset(int id)
{
	allow_signal_level++;
	for (int i=0;i<Control.size();i++)
		if (id==Control[i].ID){
#ifdef HUI_API_WIN
			if (Control[i].Kind==HuiKindListView)
				ListView_DeleteAllItems(Control[i].hWnd);
			else if (Control[i].Kind==HuiKindComboBox)
				SendMessage(Control[i].hWnd,CB_RESETCONTENT,(WPARAM)0,(LPARAM)0);
#endif
#ifdef HUI_API_GTK
			if (Control[i].Kind==HuiKindListViewTree){
				GtkTreeStore *store=GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				gtk_tree_store_clear(store);
			}else if (Control[i].Kind==HuiKindListView){
				GtkListStore *store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				gtk_list_store_clear(store);
			}else if (Control[i].Kind==HuiKindListViewIcons){
				GtkTreeModel *model=gtk_icon_view_get_model(GTK_ICON_VIEW(Control[i].win));
				//gtk_tree_store_clear(store);
				msg_write("Todo:  CHuiWindow::ResetControl (ListViewIcon)  (Linux)");
			}else if (Control[i].Kind==HuiKindComboBox){
				for (int j=Control[i]._item_.size()-1;j>=0;j--)
					gtk_combo_box_remove_text(GTK_COMBO_BOX(Control[i].win),j);
			}
			Control[i]._item_.clear();
#endif
		}
	allow_signal_level--;
}


//----------------------------------------------------------------------------------
// easy window creation functions


void HuiWindowAddControl(CHuiWindow *win,int control_type,const char *title,int x,int y,int width,int height,int id)
{
	msg_db_m(string2("HuiWindowAddControl %d  %s  %d  %d  %d  %d  %d",control_type,title,x,y,width,height,id),2);
	if (control_type==HuiKindButton)		win->AddButton(title,x,y,width,height,id);
	if (control_type==HuiKindColorButton)	win->AddColorButton(title,x,y,width,height,id);
	if (control_type==HuiKindDefButton)		win->AddDefButton(title,x,y,width,height,id);
	if (control_type==HuiKindText)			win->AddText(title,x,y,width,height,id);
	if (control_type==HuiKindEdit)			win->AddEdit(title,x,y,width,height,id);
	if (control_type==HuiKindGroup)			win->AddGroup(title,x,y,width,height,id);
	if (control_type==HuiKindCheckBox)		win->AddCheckBox(title,x,y,width,height,id);
	if (control_type==HuiKindComboBox)		win->AddComboBox(title,x,y,width,height,id);
	if (control_type==HuiKindTabControl)	win->AddTabControl(title,x,y,width,height,id);
	if (control_type==HuiKindListView)		win->AddListView(title,x,y,width,height,id);
	if (control_type==HuiKindProgressBar)	win->AddProgressBar(title,x,y,width,height,id);
	if (control_type==HuiKindSlider)		win->AddSlider(title,x,y,width,height,id);
	if (control_type==HuiKindImage)			win->AddImage(title,x,y,width,height,id);
}

CHuiWindow *HuiCreateWindow(const char *title,int x,int y,int width,int height,message_function *mf)
{
	return new CHuiWindow(	title,x,y,width,height,NULL,true,
							HuiWinModeWindow,
							true,mf);
}

CHuiWindow *HuiCreateNixWindow(const char *title,int x,int y,int width,int height,message_function *mf)
{
	return new CHuiWindow(	title,x,y,width,height,NULL,true,
							HuiWinModeWindow | HuiWinModeNix,
							true,mf);
}

CHuiWindow *HuiCreateDummyWindow(const char *title,int x,int y,int width,int height,message_function *mf)
{
	return new CHuiWindow(	title, x, y, width, height, mf);
}

CHuiWindow *HuiCreateDialog(const char *title,int width,int height,CHuiWindow *root,bool allow_root,message_function *mf)
{
	return new CHuiWindow(	title,-1,-1,width,height,root,allow_root,
							HuiWinModeDialog,
							true,mf);
}

// mainly for script usage...
void HuiCloseWindow(CHuiWindow *win)
{
	delete(win);
}

// use this as message_function for a window, that should not respond to any message
void HuiMessageFunctionNone(int)
{
}
