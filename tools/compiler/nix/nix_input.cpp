/*----------------------------------------------------------------------------*\
| Nix input                                                                    |
| -> user input (mouse/keyboard                                                |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
//#include "nix_common.h"

#ifdef NIX_OS_LINUX
	#include <X11/keysym.h>
	#include <gdk/gdkx.h>
#endif

sInputData NixInputDataCurrent,NixInputDataLast;

vector NixMouse, NixMouseRel, NixMouseD, NixMouseDRel;

static bool KeyBufferRead;

bool AllowWindowsKeyInput=false;

// Eingaben vonm Fenster entgegennehmen
void NixGetInputFromWindow()
{
	if (!NixWindow)
		return;

	#ifdef HUI_API_WIN
		int i;
		POINT mpos;
		UINT message=NixWindow->CompleteWindowMessage.msg;		NixWindow->CompleteWindowMessage.msg=0;
		WPARAM wParam=NixWindow->CompleteWindowMessage.wparam;	NixWindow->CompleteWindowMessage.wparam=0;
		LPARAM lParam=NixWindow->CompleteWindowMessage.lparam;	NixWindow->CompleteWindowMessage.lparam=0;
		//NixWindow->InputData=NixInputDataCurrent;

		int mycd=0;
		RECT ToolBarRect;

		switch(message){
			case WM_MOUSEMOVE:
				// correction due to toolbar?
				if (NixWindow->tool_bar[HuiToolBarTop].Enabled){
					//SendMessage(NixWindow->tool_bar[HuiToolBarTop].hWnd,TB_AUTOSIZE,0,0);
					GetWindowRect(NixWindow->tool_bar[HuiToolBarTop].hWnd,&ToolBarRect);
					mycd=-ToolBarRect.bottom+ToolBarRect.top;
				}
				if (NixFullscreen){
					GetCursorPos(&mpos);
					NixWindow->InputData.dx+=(float)mpos.x-NixScreenWidth/2.0f;
					NixWindow->InputData.dy+=(float)mpos.y-NixScreenHeight/2.0f;
					SetCursorPos(NixScreenWidth/2,NixScreenHeight/2);
					// korrekte Mausposition
					/*NixWindow->InputData.mx=NixInputDataCurrent.mx+NixWindow->InputData.vx;
					NixWindow->InputData.my=NixInputDataCurrent.my+NixWindow->InputData.vy;*/
					// praktischere Mausposition
					NixWindow->InputData.x=NixInputDataCurrent.x+float(NixWindow->InputData.dx)/NixMouseMappingWidth*float(NixScreenWidth);
					NixWindow->InputData.y=NixInputDataCurrent.y+float(NixWindow->InputData.dy)/NixMouseMappingHeight*float(NixScreenHeight);
				}else{
					NixWindow->InputData.x=(float)LOWORD(lParam);
					NixWindow->InputData.y=(float)HIWORD(lParam)+mycd;
					if (NixWindow->InputData.x>32000)			NixWindow->InputData.x=0;
					if (NixWindow->InputData.y>32000)			NixWindow->InputData.y=0;
				}
				if (NixWindow->InputData.x<0)				NixWindow->InputData.x=0;
				if (NixWindow->InputData.y<0)				NixWindow->InputData.y=0;
				if (NixWindow->InputData.x>NixTargetWidth)	NixWindow->InputData.x=(float)NixTargetWidth;
				if (NixWindow->InputData.y>NixTargetHeight)	NixWindow->InputData.y=(float)NixTargetHeight;
				if (!NixFullscreen){
					NixWindow->InputData.dx=NixWindow->InputData.x-NixInputDataLast.x;
					NixWindow->InputData.dy=NixWindow->InputData.y-NixInputDataLast.y;
				}
				break;
			case WM_MOUSEWHEEL:
				NixWindow->InputData.dz+=(short)HIWORD(wParam);
				break;
			case WM_LBUTTONDOWN:
				SetCapture(NixWindow->hWnd);
				NixWindow->InputData.lb=true;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_LBUTTONUP:
				ReleaseCapture();
				NixWindow->InputData.lb=false;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_MBUTTONDOWN:
				SetCapture(NixWindow->hWnd);
				NixWindow->InputData.mb=true;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_MBUTTONUP:
				ReleaseCapture();
				NixWindow->InputData.mb=false;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_RBUTTONDOWN:
				SetCapture(NixWindow->hWnd);
				NixWindow->InputData.rb=true;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_RBUTTONUP:
				ReleaseCapture();
				NixWindow->InputData.rb=false;
				//NixWindow->InputData.mx=LOWORD(lParam);
				//NixWindow->InputData.my=HIWORD(lParam);
				break;
			case WM_KEYDOWN:
				if (GetActiveWindow()==NixWindow->hWnd){
					AllowWindowsKeyInput=true;
					if (NixWindow->InputData.KeyBufferDepth>=HUI_MAX_KEYBUFFER_DEPTH-1){
						for (i=0;i<NixWindow->InputData.KeyBufferDepth-2;i++)
							NixWindow->InputData.KeyBuffer[i]=NixWindow->InputData.KeyBuffer[i+1];
						NixWindow->InputData.KeyBufferDepth--;
					}
					NixWindow->InputData.KeyBuffer[NixWindow->InputData.KeyBufferDepth]=HuiKeyID[wParam];
					NixWindow->InputData.KeyBufferDepth++;
				}
				break;
			/*case WM_KEYDOWN:
				key[wParam]=true;
				break;
			case WM_KEYUP:
				key[wParam]=false;
				break;*/
			/*case WM_SIZE:
				NixResize();
				break;*/
		}
		NixWindow->InputData.mw=(float)atan2(-NixWindow->InputData.dy-NixInputDataLast.dy,NixWindow->InputData.dx+NixInputDataLast.dx);
		if (NixWindow->InputData.mw<0)
			NixWindow->InputData.mw+=2*pi;

		if (GetActiveWindow()!=NixWindow->hWnd)
			AllowWindowsKeyInput=false;
	
		if (AllowWindowsKeyInput)
			for (i=0;i<256;i++)
				NixWindow->InputData.key[HuiKeyID[i]]=((GetAsyncKeyState(i)&(1<<15))!=0);
		else{
			for (i=0;i<256;i++)
				NixWindow->InputData.key[i]=false;
		}

		// Korrektur (manche Tasten belegen mehrere Array-Elemente) :-S
		if (NixGetKey(KEY_RALT))
			NixWindow->InputData.key[KEY_LCONTROL]=0;
	#endif
}

bool allow_mb=false;
int gdk_mx=0,gdk_my=0;

// Eingaben behandeln
void NixUpdateInput()
{
	NixInputDataLast = NixInputDataCurrent;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		NixInputDataCurrent = NixWindow->InputData;
	}
#endif
#ifdef NIX_API_OPENGL
	#ifdef NIX_OS_WINDOWS
		if (NixApi==NIX_API_OPENGL){
			NixInputDataCurrent = NixWindow->InputData;
		}
	#endif
	#ifdef NIX_OS_LINUX
	
		if (NixFullscreen){
			int mx, my, mod;
			NixInputDataCurrent = NixWindow->InputData;
		//GdkWindow *pw=gdk_window_get_pointer(NixWindow->window->window,&mx,&my,(GdkModifierType*)&mod);
			GdkWindow *pw = gdk_window_get_pointer(NixWindow->gl_widget->window, &mx, &my, (GdkModifierType*)&mod);
			NixInputDataCurrent.dx = mx - gdk_mx;
			NixInputDataCurrent.dy = my - gdk_my;
			NixInputDataCurrent.x=NixInputDataLast.x+NixInputDataCurrent.dx;
			NixInputDataCurrent.y=NixInputDataLast.y+NixInputDataCurrent.dy;
			clampf(NixInputDataCurrent.x, 0, NixScreenWidth-1);
			clampf(NixInputDataCurrent.y, 0, NixScreenHeight-1);
			// reset....
			Window win=GDK_WINDOW_XWINDOW(NixWindow->gl_widget->window);
			XWarpPointer(hui_x_display, None, win, 0, 0, 0, 0, NixScreenWidth/2, NixScreenHeight/2);
			//XFlush(hui_x_display);
			XSync(hui_x_display, FALSE);
			//GdkWindow *pw2=gdk_window_get_pointer(NixWindow->window->window,&gdk_mx,&gdk_my,(GdkModifierType*)&mod);
			GdkWindow *pw2=gdk_window_get_pointer(NixWindow->gl_widget->window,&gdk_mx,&gdk_my,(GdkModifierType*)&mod);
			
			/*int mx,my, x0, x1, y0, y1;
			// old pos
			mx = NixInputDataCurrent.x;
			my = NixInputDataCurrent.y;
			// iterate
			NixInputDataCurrent = NixWindow->InputData;
			// get pos change
			x0 = NixWindow->InputData.x;
			y0 = NixWindow->InputData.y;
			printf("0: %d %d\n", x0, y0);
			NixWindow->SetCursorPos(NixScreenWidth / 2, NixScreenHeight / 2);
			x1 = NixWindow->InputData.x;
			y1 = NixWindow->InputData.y;
			printf("1: %d %d\n", x1, y1);
			NixInputDataCurrent.dx = x0 - x1;
			NixInputDataCurrent.dy = y0 - y1;
			// new pos
			mx += NixInputDataCurrent.dx;
			my += NixInputDataCurrent.dy;
			//clampi(mx, 0, NixScreenWidth);
			//clampi(my, 0, NixScreenHeight);
			NixInputDataCurrent.x = mx;
			NixInputDataCurrent.y = my;*/
		}else{
			NixInputDataCurrent = NixWindow->InputData;
			clampf(NixInputDataCurrent.x, 0, NixTargetWidth-1);
			clampf(NixInputDataCurrent.y, 0, NixTargetHeight-1);
			NixInputDataCurrent.dx = NixInputDataCurrent.x - NixInputDataLast.x;
			NixInputDataCurrent.dy = NixInputDataCurrent.y - NixInputDataLast.y;
		}
	#endif
#endif

	// noch nicht so ganz...
	//NixWindow->InputData.lb=NixWindow->InputData.mb=NixWindow->InputData.rb=false;
	NixWindow->InputData.dx = NixWindow->InputData.dy = NixWindow->InputData.dz = 0;

	if (!KeyBufferRead){
		NixWindow->InputData.KeyBufferDepth = 0;
	}
	KeyBufferRead = false;

	
	NixMouse = vector(NixInputDataCurrent.x, NixInputDataCurrent.y, 0);
	NixMouseRel = vector((float)NixInputDataCurrent.x / (float)NixTargetWidth, (float)NixInputDataCurrent.y / (float)NixTargetHeight, 0);
	NixMouseD = vector(NixInputDataCurrent.dx, NixInputDataCurrent.dy, NixInputDataCurrent.dz);
	NixMouseDRel = vector((float)NixInputDataCurrent.dx / (float)NixTargetWidth, (float)NixInputDataCurrent.dy / (float)NixTargetHeight, NixInputDataCurrent.dz);
}

void NixResetInput()
{
	memset(&NixInputDataCurrent, 0, sizeof(sInputData));
	NixInputDataCurrent.x = gdk_mx = NixScreenWidth/2;
	NixInputDataCurrent.y = gdk_my = NixScreenHeight/2;
	NixUpdateInput();
	NixInputDataLast = NixInputDataCurrent;
}


float NixGetMDir()
{	return NixInputDataCurrent.mw;	}


void NixResetCursor()
{
	if (NixFullscreen){
		#ifdef NIX_OS_WINDOWS
			SetCursorPos(NixScreenWidth/2,NixScreenHeight/2);
		#endif
		NixWindow->InputData.x=NixInputDataCurrent.x=NixScreenWidth/2.0f;
		NixWindow->InputData.y=NixInputDataCurrent.y=NixScreenHeight/2.0f;
	}
}

bool NixGetButton(int but)
{
	if (but == 0)
		return NixInputDataCurrent.lb;
	if (but == 1)
		return NixInputDataCurrent.mb;
	if (but == 2)
		return NixInputDataCurrent.rb;
	return false;
}

bool NixGetButtonDown(int but)
{
	if (but == 0)
		return ((NixInputDataCurrent.lb) && (!NixInputDataLast.lb));
	if (but == 1)
		return ((NixInputDataCurrent.mb) && (!NixInputDataLast.mb));
	if (but == 2)
		return ((NixInputDataCurrent.rb) && (!NixInputDataLast.rb));
	return false;
}

bool NixGetButtonUp(int but)
{
	if (but == 0)
		return ((!NixInputDataCurrent.lb) && (NixInputDataLast.lb));
	if (but == 1)
		return ((!NixInputDataCurrent.mb) && (NixInputDataLast.mb));
	if (but == 2)
		return ((!NixInputDataCurrent.rb) && (NixInputDataLast.rb));
	return false;
}

bool NixGetKey(int Key)
{
	if (Key==KEY_ANY){
		for (int i=0;i<HUI_NUM_KEYS;i++)
			if (NixInputDataCurrent.key[i])
				return true;
		return false;
	}else if (Key==KEY_CONTROL)
		return NixInputDataCurrent.key[KEY_RCONTROL] || NixInputDataCurrent.key[KEY_LCONTROL];
	else if (Key==KEY_SHIFT)
		return NixInputDataCurrent.key[KEY_RSHIFT] || NixInputDataCurrent.key[KEY_LSHIFT];
	else if (Key==KEY_ALT)
		return NixInputDataCurrent.key[KEY_RALT] || NixInputDataCurrent.key[KEY_LALT];
	else
		return NixInputDataCurrent.key[Key];
}

bool NixGetKeyUp(int Key)
{
	if (Key==KEY_ANY){
		for (int i=0;i<HUI_NUM_KEYS;i++)
			if ((!NixInputDataCurrent.key[i])&&(NixInputDataLast.key[i]))
				return true;
		return false;
	}else if (Key==KEY_CONTROL)
		return ((!NixInputDataCurrent.key[KEY_RCONTROL])&&(NixInputDataLast.key[KEY_RCONTROL])) || ((!NixInputDataCurrent.key[KEY_LCONTROL])&&(NixInputDataLast.key[KEY_LCONTROL]));
	else if (Key==KEY_SHIFT)
		return ((!NixInputDataCurrent.key[KEY_RSHIFT])&&(NixInputDataLast.key[KEY_RSHIFT])) || ((!NixInputDataCurrent.key[KEY_LSHIFT])&&(NixInputDataLast.key[KEY_LSHIFT]));
	else if (Key==KEY_ALT)
		return ((!NixInputDataCurrent.key[KEY_RALT])&&(NixInputDataLast.key[KEY_RALT])) || ((!NixInputDataCurrent.key[KEY_LALT])&&(NixInputDataLast.key[KEY_LALT]));
	else
		return ((!NixInputDataCurrent.key[Key])&&(NixInputDataLast.key[Key]));
}

bool NixGetKeyDown(int Key)
{
	if (Key==KEY_ANY){
		for (int i=0;i<HUI_NUM_KEYS;i++)
			if ((NixInputDataCurrent.key[i])&&(!NixInputDataLast.key[i]))
				return true;
		return false;
	}else if (Key==KEY_CONTROL)
		return ((NixInputDataCurrent.key[KEY_RCONTROL])&&(!NixInputDataLast.key[KEY_RCONTROL])) || ((NixInputDataCurrent.key[KEY_LCONTROL])&&(!NixInputDataLast.key[KEY_LCONTROL]));
	else if (Key==KEY_SHIFT)
		return ((NixInputDataCurrent.key[KEY_RSHIFT])&&(!NixInputDataLast.key[KEY_RSHIFT])) || ((NixInputDataCurrent.key[KEY_LSHIFT])&&(!NixInputDataLast.key[KEY_LSHIFT]));
	else if (Key==KEY_ALT)
		return ((NixInputDataCurrent.key[KEY_RALT])&&(!NixInputDataLast.key[KEY_RALT])) || ((NixInputDataCurrent.key[KEY_LALT])&&(!NixInputDataLast.key[KEY_LALT]));
	else
		return ((NixInputDataCurrent.key[Key])&&(!NixInputDataLast.key[Key]));
}

static char key_char_str[16];
const char *NixGetKeyChar(int key)
{
	strcpy(key_char_str,"");
	if (key<0)	return key_char_str;
	if ((NixGetKey(KEY_RCONTROL))||(NixGetKey(KEY_LCONTROL)))
		return key_char_str;
	// shift
	if ((NixGetKey(KEY_RSHIFT))||(NixGetKey(KEY_LSHIFT))){
		for (int i=0;i<26;i++)
			if (key==KEY_A+i){
				key_char_str[0]='A'+i;
				key_char_str[1]=0;
				return key_char_str;
			}
		if (key==KEY_1)			return "!";
		if (key==KEY_2)			return "\"";
		if (key==KEY_3)			return "§";
		if (key==KEY_4)			return "$";
		if (key==KEY_5)			return "%";
		if (key==KEY_6)			return "&&";
		if (key==KEY_7)			return "/";
		if (key==KEY_8)			return "(";
		if (key==KEY_9)			return ")";
		if (key==KEY_0)			return "=";
		if (key==KEY_COMMA)		return ";";
		if (key==KEY_DOT)		return ":";
		if (key==KEY_ADD)		return "*";
		if (key==KEY_SUBTRACT)	return "_";
		if (key==KEY_SMALLER)	return ">";
		if (key==KEY_SZ)		return "?";
		if (key==KEY_AE)		return "&A";
		if (key==KEY_OE)		return "&O";
		if (key==KEY_UE)		return "&U";
		if (key==KEY_FENCE)		return "\'";
		if (key==KEY_GRAVE)		return "°";
		if (key==KEY_SPACE)		return " ";
		return key_char_str;
	}
	// alt
	if ((NixGetKey(KEY_RALT))||(NixGetKey(KEY_LALT))){
		if (key==KEY_Q)			return "@";
		/*if (key==KEY_E)			return -128;
		if (key==KEY_Y)			return -91;
		if (key==KEY_2)			return -78;
		if (key==KEY_3)			return -77;*/
		if (key==KEY_7)			return "{";
		if (key==KEY_8)			return "[";
		if (key==KEY_9)			return "]";
		if (key==KEY_0)			return "}";
		if (key==KEY_ADD)		return "~";
		if (key==KEY_SMALLER)	return "|";
		if (key==KEY_SZ)		return "\\";
		return key_char_str;
	}
	// normal
	for (int i=0;i<26;i++)
		if (key==KEY_A+i){
			key_char_str[0]='a'+i;
			key_char_str[1]=0;
			return key_char_str;
		}
	for (int i=0;i<10;i++)
		if ((key==KEY_0+i)||(key==KEY_NUM_0+i)){
			key_char_str[0]='0'+i;
			key_char_str[1]=0;
			return key_char_str;
		}
	if (key==KEY_COMMA)			return ",";
	if (key==KEY_DOT)			return ".";
	if (key==KEY_ADD)			return "+";
	if (key==KEY_SUBTRACT)		return "-";
	if (key==KEY_FENCE)			return "#";
	if (key==KEY_GRAVE)			return "^";
	if (key==KEY_NUM_ADD)		return "+";
	if (key==KEY_NUM_SUBTRACT)	return "-";
	if (key==KEY_NUM_MULTIPLY)	return "*";
	if (key==KEY_NUM_DIVIDE)	return "/";
	if (key==KEY_SMALLER)		return "<";
	if (key==KEY_SZ)			return "&s";
	if (key==KEY_AE)			return "&a";
	if (key==KEY_OE)			return "&o";
	if (key==KEY_UE)			return "&u";
	if (key==KEY_SPACE)			return " ";
	if (key==KEY_TAB)			return "\t";
	if (key==KEY_RETURN)		return "\n";
	if (key==KEY_NUM_ENTER)		return "\n";
	// unknown:
	return key_char_str;
}

int NixGetKeyRhythmDown()
{
	KeyBufferRead=true;
	if (NixWindow->InputData.KeyBufferDepth>0){
		int k=NixWindow->InputData.KeyBuffer[0];
		//msg_write(GetKeyName(k));
		for (int i=0;i<NixWindow->InputData.KeyBufferDepth-2;i++)
			NixWindow->InputData.KeyBuffer[i]=NixWindow->InputData.KeyBuffer[i+1];
		NixWindow->InputData.KeyBufferDepth--;
		return k;
	}else
		return -1;
}


