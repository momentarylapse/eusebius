/*----------------------------------------------------------------------------*\
| CHui                                                                         |
| -> Heroic User Interface                                                     |
| -> abstraction layer for GUI                                                 |
|   -> Windows (default api) or Linux (Gtk+)                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2010.01.31 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/


char HuiVersion[32]="0.3.6.1";

#include <string>
#include "hui.h"
#include "../file/file.h"
#include "../file/msg.h"
#include <stdio.h>
#include <signal.h>
#ifdef HUI_OS_WINDOWS
	#include <shlobj.h>
	#include <winuser.h>
	#include <direct.h>
	#include <commctrl.h>
	#include <tchar.h>
	#pragma comment(lib,"winmm.lib")
	#ifdef HUI_API_GTK
		#pragma comment(lib,"gtk-win32-2.0.lib")
		#pragma comment(lib,"glib-2.0.lib")
		#pragma comment(lib,"pango-1.0.lib")
		#pragma comment(lib,"pangowin32-1.0.lib")
		#pragma comment(lib,"gdk-win32-2.0.lib")
		#pragma comment(lib,"gdk_pixbuf-2.0.lib")
		#pragma comment(lib,"gobject-2.0.lib")
	#endif
	#pragma warning(disable : 4995)
#endif
#ifdef HUI_OS_LINUX
	//#include <string.h>
	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/timeb.h>
	#include <time.h>
	#include <gdk/gdkx.h>
#endif

#ifdef HUI_OS_WINDOWS
	HINSTANCE hui_win_instance;
#endif
#ifdef HUI_API_WIN
	HFONT hui_win_default_font;
	HICON hui_win_main_icon;
#endif
#ifdef HUI_OS_LINUX
	void *_hui_x_display_;
#endif

void _so(char *str)
{
	printf("%s\n",str);
}

void _so(int i)
{
	printf("%d\n",i);
}

static char FileTempName[8][1024];
static int CurrentFileTempName=0;




static void SaveConfigFile();

void_function *HuiIdleFunction = NULL, *HuiErrorFunction = NULL;
bool HuiHaveToExit;
bool HuiRunning;
bool HuiEndKeepMsgAlive=false;

std::vector<CHuiWindow*> _HuiWindow_;
std::vector<sHuiClosedWindow> _HuiClosedWindow_;

// file dialogs
char HuiFileDialogPath[1024],HuiFileDialogFile[256],HuiFileDialogCompleteName[1024];
int HuiColor[4];

// language
bool HuiLanguaged;
std::vector<std::string> HuiLanguageName;
struct sHuiLanguageTranslation
{
	std::string Orig;
	std::string Lang; // pre defined translation of Orig
};
struct sHuiLanguage
{
	char Name[128];
	std::vector<std::string> Text; // text associated to ids
	std::vector<sHuiLanguageTranslation> Translation;
};
std::vector<sHuiLanguage> HuiLanguage;
sHuiLanguage *cur_lang = NULL;
char HuiCurLanguageName[128];



// resources
std::vector<sHuiResource> _HuiResource_;

// HUI configuration
char HuiComboBoxSeparator,_Hui_Pseudo_Byte_,*HuiSeparator;
bool HuiUseFlatButtons;
bool HuiMultiline;
bool HuiCreateHiddenWindows;

std::vector<sHuiKeyCode> HuiKeyCode;

char HuiAppFilename[256],HuiAppDirectory[256], HuiInitialWorkingDirectory[256];
char HuiSingleParam[512];




#ifdef HUI_API_WIN
	unsigned char HuiKeyID[256];
#endif
#ifdef HUI_OS_WINDOWS
	char *Argument=NULL;
	void HuiSetArgs(char *arg)
	{
		Argument=arg;
		strcpy(HuiSingleParam,"");
		if (arg)
			strcpy(HuiSingleParam,arg);
	}
#endif
#ifdef HUI_API_GTK
	int HuiKeyID[256],KeyID2[256];
	void *invisible_cursor;
#endif
#ifdef HUI_OS_LINUX
	int HuiNumArguments=0;
	char *HuiArgument[32];
	void HuiSetArgs(int num_args,char *args[])
	{
		HuiNumArguments=num_args;
		if (num_args>32)
			HuiNumArguments=32;
		for (int i=0;i<HuiNumArguments;i++)
			HuiArgument[i]=args[i];
		strcpy(HuiSingleParam,"");
		if (HuiNumArguments>1)
			strcpy(HuiSingleParam,args[1]);
	}
#endif

// timers
#ifdef HUI_OS_WINDOWS
	LONGLONG perf_cnt;
	bool perf_flag=false;
	float time_scale;
#endif
struct sHuiTimer
{
#ifdef HUI_OS_WINDOWS
	LONGLONG CurTime;
	LONGLONG LastTime;
	float time_scale;
#endif
#ifdef HUI_OS_LINUX
	struct timeval CurTime, LastTime;
#endif
};
std::vector<sHuiTimer> HuiTimer;


std::vector<std::string> hui_image_file;




// character set....
#ifdef HUI_OS_WINDOWS

	#define NUM_TCHAR_STRINGS				32
	#define TCHAR_STRING_LENGTH			1024
	static int cur_tchar_str=0;
	static TCHAR _tchar_str_[NUM_TCHAR_STRINGS][TCHAR_STRING_LENGTH];
	#define _get_tchar_str_()		_tchar_str_[(cur_tchar_str++)%NUM_TCHAR_STRINGS]

	TCHAR *hui_tchar_str(const char *str)
	{
		#ifdef _UNICODE
			TCHAR *w=_get_tchar_str_();
			MultiByteToWideChar(CP_UTF8,0,(LPCSTR)str_m2utf8(str),-1,w,TCHAR_STRING_LENGTH);
			return w;
		#else
			return str_m2ascii(str);
		#endif
	}

	TCHAR *hui_tchar_str_f(const char *str)
	{
		char *t=_file_get_str_();
		strcpy(t,str);
		int sl=(int)strlen(t);
		for (int i=0;i<sl;i++)
			if (t[i]=='/')	t[i]='\\';
		#ifdef _UNICODE
			TCHAR *w=_get_tchar_str_();
			MultiByteToWideChar(CP_UTF8,0,(LPCSTR)t,-1,w,TCHAR_STRING_LENGTH);
			return w;
		#else
			return str_m2ascii(t);
		#endif
	}

	char *hui_de_tchar_str(const TCHAR *str)
	{
		#ifdef _UNICODE
			char *t=_file_get_str_();
			WideCharToMultiByte(CP_UTF8,0,str,-1,(LPSTR)t,sizeof(_file_stack_str_[0]),NULL,NULL);
			return str_utf82m(t);
		#else
			return str_ascii2m(str);
		#endif
	}

	char *de_sys_str_f(const TCHAR *str)
	{
		#ifdef _UNICODE
			char *t1=_file_get_str_();
			WideCharToMultiByte(CP_UTF8,0,str,-1,(LPSTR)t1,sizeof(_file_stack_str_[0]),NULL,NULL);
			return t1;
		#else
			return (char*)str;
		#endif
		/*char *t2=_file_get_str_();
		strcpy(t2,t1);
		int sl=strlen(t2);
		for (int i=0;i<sl;i++)
			if (t2[i]=='/')	t2[i]='\\';
		return t2;*/
	}

	int _tchar_str_size_(TCHAR *str)
	{
		return _tcslen(str)*sizeof(TCHAR);
	}
#endif

#ifdef HUI_API_GTK

	char *sys_str(const char *str)
	{	return str_m2utf8(str);}

	char *sys_str_f(const char *str)
	{
		char *t=_file_get_str_();
		strcpy(t,str);
		int sl=(int)strlen(t);
		for (int i=0;i<sl;i++)
			if (t[i]=='/')	t[i]='\\';
		return str_m2utf8(t);
	}

	char *de_sys_str(const char *str)
	{	return str_utf82m(str);	}

	char *de_sys_str_f(const char *str)
	{	return (char*)str;	}

	int _sys_str_size_(char *str)
	{	return strlen(str);	}
#endif


	char *str_m2utf8(const char *str)
	{
		int l=0,s=(int)strlen(str);
		unsigned char *us=(unsigned char*)_file_get_str_();
		for (int i=0;i<s;i++){
			us[l]=str[i];
			if (str[i]=='&'){
				if      (str[i+1]=='a'){	us[l]=0xc3;	us[l+1]=0xa4;	i++;	l++;	}
				else if (str[i+1]=='o'){	us[l]=0xc3;	us[l+1]=0xb6;	i++;	l++;	}
				else if (str[i+1]=='u'){	us[l]=0xc3;	us[l+1]=0xbc;	i++;	l++;	}
				else if (str[i+1]=='s'){	us[l]=0xc3;	us[l+1]=0x9f;	i++;	l++;	}
				else if (str[i+1]=='A'){	us[l]=0xc3;	us[l+1]=0x84;	i++;	l++;	}
				else if (str[i+1]=='O'){	us[l]=0xc3;	us[l+1]=0x96;	i++;	l++;	}
				else if (str[i+1]=='U'){	us[l]=0xc3;	us[l+1]=0x9c;	i++;	l++;	}
				else if (str[i+1]=='&'){	us[l]='&';					i++;			}
			}
			/*if (str[i]=='\n'){
#ifdef HUI_OS_WINDOWS
				_temp_[l]='\r';		_temp_[l+1]='\n';	l++;
#endif
			}*/
			l++;
		}
		us[l]=0;
		return (char*)us;
	}

	// Umlaute zu Vokalen mit & davor zerlegen
	char *str_utf82m(const char *str)
	{
		unsigned char *us=(unsigned char *)str;
		char *ss=_file_get_str_();

		int l=0;
		for (unsigned int i=0;i<strlen(str)+1;i++){
			ss[l]=str[i];
			// testen!!!!
			if ((us[i]==0xc3)&&(us[i+1]==0xa4)){	ss[l]='&';	ss[l+1]='a';	l++;	i++;	}
			if ((us[i]==0xc3)&&(us[i+1]==0xb6)){	ss[l]='&';	ss[l+1]='o';	l++;	i++;	}
			if ((us[i]==0xc3)&&(us[i+1]==0xbc)){	ss[l]='&';	ss[l+1]='u';	l++;	i++;	}
			if ((us[i]==0xc3)&&(us[i+1]==0x9f)){	ss[l]='&';	ss[l+1]='s';	l++;	i++;	}
			if ((us[i]==0xc3)&&(us[i+1]==0x84)){	ss[l]='&';	ss[l+1]='A';	l++;	i++;	}
			if ((us[i]==0xc3)&&(us[i+1]==0x96)){	ss[l]='&';	ss[l+1]='O';	l++;	i++;	}
			if ((us[i]==0xc3)&&(us[i+1]==0x9c)){	ss[l]='&';	ss[l+1]='U';	l++;	i++;	}
			if (us[i]=='&'){						ss[l]='&';	ss[l+1]='&';	l++;	i++;	}
			l++;
		}
		return ss;
	}

	// Umlaute zu Vokalen mit & davor zerlegen
	char *str_ascii2m(const char *str)
	{
		unsigned char *us=(unsigned char *)str;
		char *ss=_file_get_str_();

		int l=0;
		for (unsigned int i=0;i<strlen(str)+1;i++){
			ss[l]=str[i];
			if (us[i]==0xe4){	ss[l]='&';	ss[l+1]='a';	l++;	}
			if (us[i]==0xf6){	ss[l]='&';	ss[l+1]='o';	l++;	}
			if (us[i]==0xfc){	ss[l]='&';	ss[l+1]='u';	l++;	}
			if (us[i]==0xdf){	ss[l]='&';	ss[l+1]='s';	l++;	}
			if (us[i]==0xc4){	ss[l]='&';	ss[l+1]='A';	l++;	}
			if (us[i]==0xd6){	ss[l]='&';	ss[l+1]='O';	l++;	}
			if (us[i]==0xdc){	ss[l]='&';	ss[l+1]='U';	l++;	}
			if (us[i]=='&'){	ss[l]='&';	ss[l+1]='&';	l++;	}
			if (us[i]=='\r')	continue;
			l++;
		}
		return ss;
	}

	char *str_m2ascii(const char *str)
	{
		char *ss=_file_get_str_();
		unsigned char *us=(unsigned char *)ss;

		int l=0;
		for (unsigned int i=0;i<strlen(str)+1;i++){
			ss[l]=str[i];
			if (str[i]=='&'){
				if      (str[i+1]=='a'){	us[l]=0xe4;	i++;	}
				else if (str[i+1]=='o'){	us[l]=0xf6;	i++;	}
				else if (str[i+1]=='u'){	us[l]=0xfc;	i++;	}
				else if (str[i+1]=='s'){	us[l]=0xdf;	i++;	}
				else if (str[i+1]=='A'){	us[l]=0xc4;	i++;	}
				else if (str[i+1]=='O'){	us[l]=0xd6;	i++;	}
				else if (str[i+1]=='U'){	us[l]=0xdc;	i++;	}
				else if (str[i+1]=='&'){	us[l]='&';	i++;	}
			}
			l++;
		}
		return ss;
	}


#ifdef HUI_API_GTK
	int idle_id = -1;
	gboolean GtkIdleFunction(void*)
	{
		if (HuiIdleFunction)
			HuiIdleFunction();
		else
			HuiSleep(10);
		return TRUE;
	}

	gboolean GtkRunLaterFunction(gpointer data)
	{
		if (data){
			void_function *function = (void_function*)data;
			function();
		}
		return false;
	}
#endif

void HuiSetIdleFunction(void_function *idle_function)
{
	#ifdef HUI_API_GTK
		if ((idle_function) && (!HuiIdleFunction))
			idle_id = g_idle_add_full(300,GtkIdleFunction,NULL,NULL);
		if ((!idle_function) && (HuiIdleFunction) && (idle_id >= 0)){
			gtk_idle_remove(idle_id);
			idle_id = -1;
		}
	#endif
	HuiIdleFunction = idle_function;
}

void HuiRunLater(int time_ms, void_function *function)
{
	#ifdef HUI_API_WIN
		msg_todo("HuiRunLater");
	#endif
	#ifdef HUI_API_GTK
		g_timeout_add_full(300, time_ms, &GtkRunLaterFunction, (void*)function, NULL);
	#endif
}

void HuiInit()
{
	#ifdef HUI_OS_WINDOWS
		strcpy(HuiAppFilename,_pgmptr);
		strcpy(HuiAppDirectory,dir_from_filename(HuiAppFilename));
	#endif
	#ifdef HUI_OS_LINUX
		if (HuiNumArguments>0){
			if (HuiArgument[0][0]=='/'){
				strcpy(HuiAppFilename,HuiArgument[0]);
			}else{
				char *r=getcwd(HuiAppFilename,sizeof(HuiAppFilename));
				if (HuiAppDirectory[strlen(HuiAppFilename)-1]!='/')
					strcat(HuiAppFilename,"/");
				if (HuiArgument[0][0]=='.')
					strcat(HuiAppFilename,&HuiArgument[0][2]);
				else
					strcat(HuiAppFilename,HuiArgument[0]);
			}
			strcpy(HuiAppDirectory,dir_from_filename(HuiAppFilename));
		}
	#endif

	if (!msg_inited)
		msg_init(true,string(HuiAppDirectory,"message.txt"));
	HuiRunning=false;

	msg_db_r("Hui",1);
	msg_db_m(string("[",HuiVersion,"]"),1);


	#ifdef HUI_API_WIN

		//InitCommonControls(); comctl32.lib
		CoInitialize(NULL);
		//WinStandartFont=CreateFont(8,0,0,0,FW_NORMAL,FALSE,FALSE,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,"MS Sans Serif");
		hui_win_default_font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

		hui_win_main_icon=ExtractIcon(hui_win_instance,sys_str(_pgmptr),0);

		for (int k=0;k<256;k++)
			HuiKeyID[k]=255;
		HuiKeyID[VK_LCONTROL	]=KEY_LCONTROL;
		HuiKeyID[VK_RCONTROL	]=KEY_RCONTROL;
		HuiKeyID[VK_LSHIFT		]=KEY_LSHIFT;
		HuiKeyID[VK_RSHIFT		]=KEY_RSHIFT;
		HuiKeyID[VK_LMENU		]=KEY_LALT;
		HuiKeyID[VK_RMENU		]=KEY_RALT;
		HuiKeyID[187			]=KEY_ADD;
		HuiKeyID[189			]=KEY_SUBTRACT;
		HuiKeyID[191			]=KEY_FENCE;
		HuiKeyID[220			]=KEY_GRAVE;
		HuiKeyID[VK_END			]=KEY_END;
		HuiKeyID[VK_NEXT		]=KEY_NEXT;
		HuiKeyID[VK_PRIOR		]=KEY_PRIOR;
		HuiKeyID[VK_UP			]=KEY_UP;
		HuiKeyID[VK_DOWN		]=KEY_DOWN;
		HuiKeyID[VK_LEFT		]=KEY_LEFT;
		HuiKeyID[VK_RIGHT		]=KEY_RIGHT;
		HuiKeyID[VK_RETURN		]=KEY_RETURN;
		HuiKeyID[VK_ESCAPE		]=KEY_ESCAPE;
		HuiKeyID[VK_INSERT		]=KEY_INSERT;
		HuiKeyID[VK_DELETE		]=KEY_DELETE;
		HuiKeyID[VK_SPACE		]=KEY_SPACE;
		HuiKeyID[VK_F1			]=KEY_F1;
		HuiKeyID[VK_F2			]=KEY_F2;
		HuiKeyID[VK_F3			]=KEY_F3;
		HuiKeyID[VK_F4			]=KEY_F4;
		HuiKeyID[VK_F5			]=KEY_F5;
		HuiKeyID[VK_F6			]=KEY_F6;
		HuiKeyID[VK_F7			]=KEY_F7;
		HuiKeyID[VK_F8			]=KEY_F8;
		HuiKeyID[VK_F9			]=KEY_F9;
		HuiKeyID[VK_F10			]=KEY_F10;
		HuiKeyID[VK_F11			]=KEY_F11;
		HuiKeyID[VK_F12			]=KEY_F12;
		HuiKeyID['1'			]=KEY_1;
		HuiKeyID['2'			]=KEY_2;
		HuiKeyID['3'			]=KEY_3;
		HuiKeyID['4'			]=KEY_4;
		HuiKeyID['5'			]=KEY_5;
		HuiKeyID['6'			]=KEY_6;
		HuiKeyID['7'			]=KEY_7;
		HuiKeyID['8'			]=KEY_8;
		HuiKeyID['9'			]=KEY_9;
		HuiKeyID['0'			]=KEY_0;
		HuiKeyID['A'			]=KEY_A;
		HuiKeyID['B'			]=KEY_B;
		HuiKeyID['C'			]=KEY_C;
		HuiKeyID['D'			]=KEY_D;
		HuiKeyID['E'			]=KEY_E;
		HuiKeyID['F'			]=KEY_F;
		HuiKeyID['G'			]=KEY_G;
		HuiKeyID['H'			]=KEY_H;
		HuiKeyID['I'			]=KEY_I;
		HuiKeyID['J'			]=KEY_J;
		HuiKeyID['K'			]=KEY_K;
		HuiKeyID['L'			]=KEY_L;
		HuiKeyID['M'			]=KEY_M;
		HuiKeyID['N'			]=KEY_N;
		HuiKeyID['O'			]=KEY_O;
		HuiKeyID['P'			]=KEY_P;
		HuiKeyID['Q'			]=KEY_Q;
		HuiKeyID['R'			]=KEY_R;
		HuiKeyID['S'			]=KEY_S;
		HuiKeyID['T'			]=KEY_T;
		HuiKeyID['U'			]=KEY_U;
		HuiKeyID['V'			]=KEY_V;
		HuiKeyID['W'			]=KEY_W;
		HuiKeyID['X'			]=KEY_X;
		HuiKeyID['Y'			]=KEY_Y;
		HuiKeyID['Z'			]=KEY_Z;
		HuiKeyID[VK_BACK		]=KEY_BACKSPACE;
		HuiKeyID[VK_TAB			]=KEY_TAB;
		HuiKeyID[VK_HOME		]=KEY_HOME;
		HuiKeyID[VK_NUMPAD0		]=KEY_NUM_0;
		HuiKeyID[VK_NUMPAD1		]=KEY_NUM_1;
		HuiKeyID[VK_NUMPAD2		]=KEY_NUM_2;
		HuiKeyID[VK_NUMPAD3		]=KEY_NUM_3;
		HuiKeyID[VK_NUMPAD4		]=KEY_NUM_4;
		HuiKeyID[VK_NUMPAD5		]=KEY_NUM_5;
		HuiKeyID[VK_NUMPAD6		]=KEY_NUM_6;
		HuiKeyID[VK_NUMPAD7		]=KEY_NUM_7;
		HuiKeyID[VK_NUMPAD8		]=KEY_NUM_8;
		HuiKeyID[VK_NUMPAD9		]=KEY_NUM_9;
		HuiKeyID[VK_ADD			]=KEY_NUM_ADD;
		HuiKeyID[VK_SUBTRACT	]=KEY_NUM_SUBTRACT;
		HuiKeyID[VK_MULTIPLY	]=KEY_NUM_MULTIPLY;
		HuiKeyID[VK_DIVIDE		]=KEY_NUM_DIVIDE;
		HuiKeyID[VK_DECIMAL		]=KEY_NUM_COMMA;
		//HuiKeyID[VK_RETURN	]=KEY_NUM_ENTER;
		HuiKeyID[188			]=KEY_COMMA;
		HuiKeyID[190			]=KEY_DOT;
		HuiKeyID[226			]=KEY_SMALLER;
		HuiKeyID[219			]=KEY_SZ;
		HuiKeyID[222			]=KEY_AE;
		HuiKeyID[192			]=KEY_OE;
		HuiKeyID[186			]=KEY_UE;
		

		// timers
		if (QueryPerformanceFrequency((LARGE_INTEGER *) &perf_cnt)){
			perf_flag=true;
			time_scale=1.0f/perf_cnt;
		}else 
			time_scale=0.001f;

	#endif
	#ifdef HUI_API_GTK
		gtk_init(NULL, NULL);
		#ifdef HUI_OS_LINUX
			_hui_x_display_ = XOpenDisplay(0);
		#endif


		for (int k=0;k<HUI_NUM_KEYS;k++)
			HuiKeyID[k]=KeyID2[k]=0;

		HuiKeyID[KEY_LCONTROL]=GDK_Control_L;
		HuiKeyID[KEY_RCONTROL]=GDK_Control_R;
		HuiKeyID[KEY_LSHIFT]=GDK_Shift_L;
		HuiKeyID[KEY_RSHIFT]=GDK_Shift_R;
		HuiKeyID[KEY_LALT]=GDK_Alt_L;
		HuiKeyID[KEY_RALT]=GDK_Alt_R;
		HuiKeyID[KEY_ADD]=GDK_plus;
		HuiKeyID[KEY_SUBTRACT]=GDK_minus;
		HuiKeyID[KEY_FENCE]=GDK_numbersign;
		HuiKeyID[KEY_GRAVE]=GDK_asciicircum;
		HuiKeyID[KEY_HOME]=GDK_Home;
		HuiKeyID[KEY_END]=GDK_End;
		HuiKeyID[KEY_NEXT]=GDK_Page_Up;
		HuiKeyID[KEY_PRIOR]=GDK_Page_Down;
		HuiKeyID[KEY_UP]=GDK_Up;
		HuiKeyID[KEY_DOWN]=GDK_Down;
		HuiKeyID[KEY_LEFT]=GDK_Left;
		HuiKeyID[KEY_RIGHT]=GDK_Right;
		HuiKeyID[KEY_RETURN]=GDK_Return;
		HuiKeyID[KEY_ESCAPE]=GDK_Escape;
		HuiKeyID[KEY_INSERT]=GDK_Insert;
		HuiKeyID[KEY_DELETE]=GDK_Delete;
		HuiKeyID[KEY_SPACE]=GDK_space;
		HuiKeyID[KEY_F1]=GDK_F1;
		HuiKeyID[KEY_F2]=GDK_F2;
		HuiKeyID[KEY_F3]=GDK_F3;
		HuiKeyID[KEY_F4]=GDK_F4;
		HuiKeyID[KEY_F5]=GDK_F5;
		HuiKeyID[KEY_F6]=GDK_F6;
		HuiKeyID[KEY_F7]=GDK_F7;
		HuiKeyID[KEY_F8]=GDK_F8;
		HuiKeyID[KEY_F9]=GDK_F9;
		HuiKeyID[KEY_F10]=GDK_F10;
		HuiKeyID[KEY_F11]=GDK_F11;
		HuiKeyID[KEY_F12]=GDK_F12;
		HuiKeyID[KEY_1]=GDK_1;
		HuiKeyID[KEY_2]=GDK_2;
		HuiKeyID[KEY_3]=GDK_3;
		HuiKeyID[KEY_4]=GDK_4;
		HuiKeyID[KEY_5]=GDK_5;
		HuiKeyID[KEY_6]=GDK_6;
		HuiKeyID[KEY_7]=GDK_7;
		HuiKeyID[KEY_8]=GDK_8;
		HuiKeyID[KEY_9]=GDK_9;
		HuiKeyID[KEY_0]=GDK_0;
		HuiKeyID[KEY_A]=GDK_a;		KeyID2[KEY_A]=GDK_A;
		HuiKeyID[KEY_B]=GDK_b;		KeyID2[KEY_B]=GDK_B;
		HuiKeyID[KEY_C]=GDK_c;		KeyID2[KEY_C]=GDK_C;
		HuiKeyID[KEY_D]=GDK_d;		KeyID2[KEY_D]=GDK_D;
		HuiKeyID[KEY_E]=GDK_e;		KeyID2[KEY_E]=GDK_E;
		HuiKeyID[KEY_F]=GDK_f;		KeyID2[KEY_F]=GDK_F;
		HuiKeyID[KEY_G]=GDK_g;		KeyID2[KEY_G]=GDK_G;
		HuiKeyID[KEY_H]=GDK_h;		KeyID2[KEY_H]=GDK_H;
		HuiKeyID[KEY_I]=GDK_i;		KeyID2[KEY_I]=GDK_I;
		HuiKeyID[KEY_J]=GDK_j;		KeyID2[KEY_J]=GDK_J;
		HuiKeyID[KEY_K]=GDK_k;		KeyID2[KEY_K]=GDK_K;
		HuiKeyID[KEY_L]=GDK_l;		KeyID2[KEY_L]=GDK_L;
		HuiKeyID[KEY_M]=GDK_m;		KeyID2[KEY_M]=GDK_M;
		HuiKeyID[KEY_N]=GDK_n;		KeyID2[KEY_N]=GDK_N;
		HuiKeyID[KEY_O]=GDK_o;		KeyID2[KEY_O]=GDK_O;
		HuiKeyID[KEY_P]=GDK_p;		KeyID2[KEY_P]=GDK_P;
		HuiKeyID[KEY_Q]=GDK_q;		KeyID2[KEY_Q]=GDK_Q;
		HuiKeyID[KEY_R]=GDK_r;		KeyID2[KEY_R]=GDK_R;
		HuiKeyID[KEY_S]=GDK_s;		KeyID2[KEY_S]=GDK_S;
		HuiKeyID[KEY_T]=GDK_t;		KeyID2[KEY_T]=GDK_T;
		HuiKeyID[KEY_U]=GDK_u;		KeyID2[KEY_U]=GDK_U;
		HuiKeyID[KEY_V]=GDK_v;		KeyID2[KEY_V]=GDK_V;
		HuiKeyID[KEY_W]=GDK_w;		KeyID2[KEY_W]=GDK_W;
		HuiKeyID[KEY_X]=GDK_x;		KeyID2[KEY_X]=GDK_X;
		HuiKeyID[KEY_Y]=GDK_y;		KeyID2[KEY_Y]=GDK_Y;
		HuiKeyID[KEY_Z]=GDK_z;		KeyID2[KEY_Z]=GDK_Z;
		HuiKeyID[KEY_BACKSPACE]=GDK_BackSpace;
		HuiKeyID[KEY_TAB]=GDK_Tab;
		HuiKeyID[KEY_NUM_0]=GDK_KP_0;
		HuiKeyID[KEY_NUM_1]=GDK_KP_1;
		HuiKeyID[KEY_NUM_2]=GDK_KP_2;
		HuiKeyID[KEY_NUM_3]=GDK_KP_3;
		HuiKeyID[KEY_NUM_4]=GDK_KP_4;
		HuiKeyID[KEY_NUM_5]=GDK_KP_5;
		HuiKeyID[KEY_NUM_6]=GDK_KP_6;
		HuiKeyID[KEY_NUM_7]=GDK_KP_7;
		HuiKeyID[KEY_NUM_8]=GDK_KP_8;
		HuiKeyID[KEY_NUM_9]=GDK_KP_9;
		HuiKeyID[KEY_NUM_ADD]=GDK_KP_Add;
		HuiKeyID[KEY_NUM_SUBTRACT]=GDK_KP_Subtract;
		HuiKeyID[KEY_NUM_MULTIPLY]=GDK_KP_Multiply;
		HuiKeyID[KEY_NUM_DIVIDE]=GDK_KP_Divide;
		HuiKeyID[KEY_NUM_COMMA]=GDK_KP_Decimal;
		HuiKeyID[KEY_NUM_ENTER]=GDK_KP_Enter;
		HuiKeyID[KEY_COMMA]=GDK_comma;				KeyID2[KEY_COMMA]=GDK_semicolon;
		HuiKeyID[KEY_DOT]=GDK_period;
		HuiKeyID[KEY_SMALLER]=GDK_less;			KeyID2[KEY_AE]=GDK_greater;
		HuiKeyID[KEY_SZ]=GDK_ssharp;
		HuiKeyID[KEY_AE]=GDK_adiaeresis;			KeyID2[KEY_AE]=GDK_ae;
		HuiKeyID[KEY_OE]=GDK_odiaeresis;					KeyID2[KEY_AE]=GDK_oe;
		HuiKeyID[KEY_UE]=GDK_udiaeresis;
		HuiKeyID[KEY_WINDOWS_L]=GDK_Super_L;
		HuiKeyID[KEY_WINDOWS_R]=GDK_Super_R;


		GdkPixmap *pm=gdk_pixmap_new(NULL,1,1,1);
		GdkColor ca;
		ca.pixel=0;
		ca.red=ca.green=ca.blue=0;
		invisible_cursor=gdk_cursor_new_from_pixmap(pm,pm,&ca,&ca,0,0);
		/*invisible_cursor=gdk_cursor_new_from_pixbuf(gdk_display_get_default(),
                                          GdkPixbuf *pixbuf,
                                          0,0);*/

	#endif


	if (HuiSingleParam[0]=='\"'){
		char temp[512];
		strcpy(temp,&HuiSingleParam[1]);
		temp[strlen(temp)-1]=0;
		strcpy(HuiSingleParam,temp);
	}

	HuiComboBoxSeparator='\\';
	_Hui_Pseudo_Byte_=0;
	HuiSeparator=&HuiComboBoxSeparator;
	HuiUseFlatButtons=true;
	HuiMultiline=false;
	HuiIdleFunction=NULL;
	HuiErrorFunction=NULL;
	HuiLanguaged=false;
	HuiCreateHiddenWindows=false;

	// make random numbers...well...random
	sDate d=get_current_date();
	for (int j=0;j<d.milli_second+d.second;j++)
		rand();

	msg_db_l(1);
}

void HuiInitExtended(char *program, char *version, void_function *error_cleanup_function, void *send_bug_report_function, bool load_res, char *def_lang)
{
	char *r = getcwd(HuiInitialWorkingDirectory, sizeof(HuiInitialWorkingDirectory));
	dir_ensure_ending(HuiInitialWorkingDirectory, true);

	#ifdef HUI_OS_LINUX
		if (HuiNumArguments>0){
			if (HuiArgument[0][0]=='/'){
				if (HuiArgument[0][1]=='u'){ // /usr/...
					strcpy(HuiAppFilename,HuiArgument[0]);
					strcpy(HuiAppDirectory, string2("%s/.%s/", getenv("HOME"), program));
				}else{
					strcpy(HuiAppFilename,HuiArgument[0]);
					strcpy(HuiAppDirectory,dir_from_filename(HuiAppFilename));
				}
			}else{
				if (HuiAppDirectory[strlen(HuiAppFilename)-1]!='/')
					strcat(HuiAppFilename,"/");
				if (HuiArgument[0][0]=='.'){
					strcat(HuiAppFilename,&HuiArgument[0][2]);
					strcpy(HuiAppDirectory,HuiInitialWorkingDirectory);
				}else{
					strcat(HuiAppFilename,HuiArgument[0]);
					strcpy(HuiAppDirectory, string2("%s/.%s/", getenv("HOME"), program));
				}
			}
		}
		char temp[1024];
		strcpy(temp, HuiAppDirectory);
	#else
		strcpy(HuiAppDirectory, HuiInitialWorkingDirectory);
	#endif

	if (!msg_inited)
		msg_init(true, string(HuiAppDirectory, "message.txt"));

	//msg_write(string("HuiAppDirectory", HuiAppDirectory));

	HuiInit();
#ifdef HUI_OS_LINUX
	strcpy(HuiAppDirectory, temp);
#endif
	HuiSetDefaultErrorHandler(program, version, error_cleanup_function, send_bug_report_function);
	//msg_write("");

	
	//msg_write(string("HuiAppDirectory", HuiAppDirectory));
	//msg_write(string("HuiInitialWorkingDirectory", HuiInitialWorkingDirectory));

	if (load_res)
		HuiLoadResource(string(HuiAppDirectory, "Data/00_hui_resources.txt"));

	if (strlen(def_lang) > 0){
		char lang[128];
		HuiConfigReadStr("Language", lang, def_lang);
		HuiSetLanguage(lang);
	}

	// at this point:
	//   HuiAppDirectory -> dir to run binary in (binary dir or ~/.my_app/)
	//   HuiAppFilename -> binary file (no dir)
	//   HuiInitialWorkingDirectory -> working dir before running this program
	//   working dir -> ?
}




// die System-Schleife ausfuehren, Verwendung:
// int main(...)
// {
//     HuiInit();
//     ...
//     return HuiRun();
// }

int HuiRun()
{
	msg_db_r("HuiRun",1);
	HuiRunning=true;
#ifdef HUI_API_WIN
	MSG messages;
	messages.message=0;
	HuiHaveToExit=false;
	bool got_message;
	while ((!HuiHaveToExit)&&(WM_QUIT!=messages.message)){
		bool allow=true;
		if (HuiIdleFunction)
			got_message=(PeekMessage(&messages,NULL,0U,0U,PM_REMOVE)!=0);
		else
			got_message=(GetMessage(&messages,NULL,0,0)!=0);
		if (got_message){
			allow=false;
			TranslateMessage(&messages);
			DispatchMessage(&messages);
			for (int i=0;i<_HuiWindow_.size();i++)
				if (_HuiWindow_[i]->hWnd==messages.hwnd){
					allow=true;
					break;
				}
		}
		if ((HuiIdleFunction)&&(allow))
			HuiIdleFunction();
	}
#endif
#ifdef HUI_API_GTK
	gtk_main();
#endif
	msg_db_l(1);
	return 0;
}

void HuiDoSingleMainLoop()
{
	msg_db_r("HuiDoSingleMainLoop",1);
#ifdef HUI_API_WIN
	MSG messages;
	messages.message=0;
	HuiHaveToExit=false;
	bool got_message;

	bool allow=true;
	if (HuiIdleFunction)
		got_message=(PeekMessage(&messages,NULL,0U,0U,PM_REMOVE)!=0);
	else
		got_message=(GetMessage(&messages,NULL,0,0)!=0);
	if (got_message){
		allow=false;
		TranslateMessage(&messages);
		DispatchMessage(&messages);
		for (int i=0;i<_HuiWindow_.size();i++)
			if (_HuiWindow_[i]->hWnd==messages.hwnd){
				allow=true;
				return;
			}
	}
	/*if ((HuiIdleFunction)&&(allow))
		HuiIdleFunction();*/
#endif
#ifdef HUI_API_GTK
	gtk_main_iteration_do(false);
#endif
	msg_db_l(1);
}

// ends the system loop of the HuiRun() command
void HuiEnd()
{
	//_so("<End>");
	msg_db_r("HuiEnd",1);
	HuiSetErrorFunction(NULL);
#ifdef HUI_API_WIN
	PostQuitMessage(0);
#endif
#ifdef HUI_API_GTK
	gtk_main_quit();
#endif
	SaveConfigFile();
	msg_db_l(1);
	if ((msg_inited)&&(!HuiEndKeepMsgAlive))
		msg_end();
}

void HuiSleep(int duration_ms)
{
	if (duration_ms<=0)
		return;
#ifdef HUI_OS_WINDOWS
	Sleep(duration_ms);
#endif
#ifdef HUI_OS_LINUX
	usleep(duration_ms*1000);
#endif
}

// set the default directory
void HuiSetDirectory(char *dir)
{
#ifdef HUI_OS_WINDOWS
	_chdir(SysFileName(dir));
#endif
#ifdef HUI_OS_LINUX
	int r=chdir(SysFileName(dir));
#endif
}

// apply a function to be executed when a critical error occures
void HuiSetErrorFunction(void_function *error_function)
{
	HuiErrorFunction=error_function;
	signal(SIGSEGV,(void (*)(int))HuiErrorFunction);
	/*signal(SIGINT,(void (*)(int))HuiErrorFunction);
	signal(SIGILL,(void (*)(int))HuiErrorFunction);
	signal(SIGTERM,(void (*)(int))HuiErrorFunction);
	signal(SIGABRT,(void (*)(int))HuiErrorFunction);*/
	/*signal(SIGFPE,(void (*)(int))HuiErrorFunction);
	signal(SIGBREAK,(void (*)(int))HuiErrorFunction);
	signal(SIGABRT_COMPAT,(void (*)(int))HuiErrorFunction);*/
}



static char *_eh_program_,*_eh_version_;
static CHuiWindow *ErrorDialog,*ReportDialog;
static void_function *_eh_cleanup_function_;
typedef void bug_report_function(char*,char*,char*,char*);
static bug_report_function *_eh_bug_report_function_;

// IDs
enum{
	HMM_ERROR_HEADER=-5000,
	HMM_MESSAGE_LIST,
	HMM_OK,
	HMM_CANCEL,
	HMM_OPEN_MESSAGE_TXT,
	HMM_SEND_REPORT,
	HMM_COMMENT,
	HMM_REPORT_SENDER
};

char sender[512], comment[2048];

void ReportDialogFunction(int message)
{
	switch (message){
		case HMM_OK:
			if (_eh_bug_report_function_){
				strcpy(sender, ReportDialog->GetControlText(HMM_REPORT_SENDER));
				strcpy(comment, ReportDialog->GetControlText(HMM_COMMENT));
				_eh_bug_report_function_(sender, _eh_program_,_eh_version_, comment);
			}
		case HMM_CANCEL:
		case HUI_WIN_CLOSE:
			delete(ReportDialog);
			return;
	}
}

void HuiSendBugReport()
{
	HuiMultiline=true;

	// dialog
	ReportDialog=HuiCreateDialog(_("Fehlerbericht"),400,295,ErrorDialog,false,&ReportDialogFunction);
	ReportDialog->AddText(_("Name:"),5,5,100,25,-1);
	ReportDialog->AddEdit("",5,35,385,25,HMM_REPORT_SENDER);
	ReportDialog->AddButton(_("OK"),140,255,120,25,HMM_OK);
	ReportDialog->SetControlImage(HMM_OK,HuiImageOk);
	ReportDialog->AddButton(_("Abbrechen"),265,255,120,25,HMM_CANCEL);
	ReportDialog->SetControlImage(HMM_CANCEL,HuiImageCancel);
	ReportDialog->AddText(_("Kommentar/Geschehnisse:"),5,65,200,25,-1);
	ReportDialog->AddEdit("",5,95,385,110,HMM_COMMENT);
	ReportDialog->AddText(_("Neben diesen Angaben wird noch der Inhalt der Datei message.txt geschickt"),5,210,390,35,-1);

	ReportDialog->SetControlText(HMM_REPORT_SENDER,_("(anonym)"));
	ReportDialog->SetControlText(HMM_COMMENT,_("Ist halt irgendwie passiert..."));

	ReportDialog->Update();

	HuiWaitTillWindowClosed(ReportDialog);
}

void ErrorDialogFunction(int message)
{
	switch (message){
		case HMM_SEND_REPORT:
			if (_eh_bug_report_function_)
				HuiSendBugReport();
			break;
		case HMM_OPEN_MESSAGE_TXT:
			HuiOpenDocument("message.txt");
			break;
		case HMM_OK:
		case HUI_WIN_CLOSE:
			delete(ErrorDialog);
			return;
	}
}

void hui_default_error_handler()
{
	HuiIdleFunction=NULL;

	msg_reset_shift();
	msg_write("");
	msg_write("==============================================================================================");
	msg_write(_("program has crashed, error handler has been called... maybe SegFault... m(-_-)m"));
	//msg_write("---");
	msg_write("      trace:");
	msg_write(msg_get_trace());

	if (_eh_cleanup_function_){
		msg_write(_("i'm now going to clean up..."));
		_eh_cleanup_function_();
		msg_write(_("...done"));
	}
	msg_write(_("                  Close dialog box to exit program."));

	//HuiMultiline=true;
	HuiComboBoxSeparator='$';

	// dialog
	ErrorDialog=HuiCreateDialog(_("Fehler"),600,500,NULL,false,&ErrorDialogFunction);
	ErrorDialog->AddText(string2(("%s ist abgest&urzt!		Die letzten Zeilen der Datei message.txt:"),_eh_version_),5,5,590,20,HMM_ERROR_HEADER);
	ErrorDialog->AddListView(_("Nachrichten"),5,30,590,420,HMM_MESSAGE_LIST);
	//ErrorDialog->AddEdit("",5,30,590,420,HMM_MESSAGE_LIST);
	ErrorDialog->AddButton(_("OK"),5,460,100,25,HMM_OK);
	ErrorDialog->SetControlImage(HMM_OK,HuiImageOk);
	ErrorDialog->AddButton(_("message.txt &offnen"),115,460,200,25,HMM_OPEN_MESSAGE_TXT);
	if (_eh_bug_report_function_)
		ErrorDialog->AddButton(_("Fehlerbericht an Michi senden"),325,460,265,25,HMM_SEND_REPORT);
	for (int i=1023;i>=0;i--){
		char temp[256];
		strcpy(temp,msg_get_str(i));
		if (strlen(temp)>0)
			ErrorDialog->AddControlText(HMM_MESSAGE_LIST,msg_get_str(i));
	}
	ErrorDialog->Update();

	HuiWaitTillWindowClosed(ErrorDialog);

	//HuiErrorBox(NULL,"Fehler","Fehler");
	//HuiEnd();
	exit(0);
}

void HuiSetDefaultErrorHandler(char *program,char *version,void_function *error_cleanup_function,void *send_bug_report_function)
{
	_eh_bug_report_function_=(bug_report_function*)send_bug_report_function;
	_eh_cleanup_function_=error_cleanup_function;
	_eh_program_=program;
	_eh_version_=version;
	HuiSetErrorFunction(&hui_default_error_handler);
}

void HuiRaiseError(char *message)
{
	msg_error(string(message," (HuiRaiseError)"));
	/*int *p_i=NULL;
	*p_i=4;*/
	hui_default_error_handler();
}

// wartet, bis das Fenster sich geschlossen hat
void HuiWaitTillWindowClosed(CHuiWindow *win)
{
	msg_db_r("HuiWaitTillWindowClosed",1);
	int uid=win->uid;
	/*msg_write((int)win);
	msg_write(win->uid);*/

#ifdef HUI_API_WIN
	MSG messages;
	messages.message=0;
	bool got_message;
	//while ((WM_QUIT!=messages.message)&&(!WindowClosed[win_no])){
	while (WM_QUIT!=messages.message){
		bool br=false;
		for (int i=0;i<_HuiClosedWindow_.size();i++)
			if (_HuiClosedWindow_[i].UID==uid)
				br=true;
		if (br)
			break;
		bool allow=true;
		if (HuiIdleFunction)
			got_message=(PeekMessage(&messages,NULL,0U,0U,PM_REMOVE)!=0);
		else
			got_message=(GetMessage(&messages,NULL,0,0)!=0);
		if (got_message){
			allow=false;
			TranslateMessage(&messages);
			DispatchMessage(&messages);
		}
		if ((HuiIdleFunction)&&(allow))
			HuiIdleFunction();
	}
	if (WM_QUIT==messages.message){
		HuiHaveToExit=true;
		//msg_write("EXIT!!!!!!!!!!");
	}
#endif
#ifdef HUI_API_GTK
	gtk_dialog_run(GTK_DIALOG(win->window));
#endif
	//msg_write("cleanup");

	// clean up
	for (int i=0;i<_HuiClosedWindow_.size();i++)
		if (_HuiClosedWindow_[i].UID == uid){
			_HuiClosedWindow_.erase(_HuiClosedWindow_.begin() + i);
			i--;
		}
	msg_db_l(1);
}

#ifdef HUI_API_WIN
//#define __T(x)  L ## x
static int CALLBACK FileDialogDirCallBack(HWND hWnd, UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	if (uMsg==BFFM_INITIALIZED){
		SendMessage(hWnd,BFFM_SETSELECTION,TRUE,lpData);
		HWND tree=FindWindowEx(hWnd,NULL,_T("SysTreeView32"),NULL);
		if (tree){
			HuiInfoBox(NULL,"","");
			RECT r;
			GetWindowRect(tree,&r);
			ScreenToClient(hWnd,(LPPOINT)&r);
			ScreenToClient(hWnd,((LPPOINT)&r)+1);
			r.top-=5;
			r.left-=5;
			r.right+=5;
			r.bottom+=5;
			MoveWindow(tree,r.left,r.top,(r.left-r.right),(r.bottom-r.top),FALSE);
		}
	}
	return 0;
}
#endif

#ifdef HUI_API_WIN
	static TCHAR _filename_[512],_complete_name_[512],_path_[512];
#endif

// Dialog zur Wahl eines Verzeichnisses (<dir> ist das anfangs ausgewaehlte)
bool HuiFileDialogDir(CHuiWindow *win,char *title,char *dir/*,char *root_dir*/)
{
#ifdef HUI_API_WIN

	BROWSEINFO bi;
	ZeroMemory(&bi,sizeof(bi));
	bi.hwndOwner=win?win->hWnd:NULL;
	/*if (root_dir)
		bi.pidlRoot=*lpItemIdList;
	else*/
		bi.pidlRoot=NULL;
	bi.lpszTitle=sys_str(title);
#if _MSC_VER > 1000
	bi.ulFlags= BIF_EDITBOX | 64 | BIF_RETURNONLYFSDIRS;
#else
	bi.ulFlags=BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
#endif
	bi.lpfn=FileDialogDirCallBack;
	_tcscpy(_path_,sys_str_f(dir));
	bi.lParam=(LPARAM)_path_;//sys_str_f(dir);
	LPITEMIDLIST pidl=SHBrowseForFolder(&bi);
	if (pidl){
		SHGetPathFromIDList(pidl,_path_);//sys_str_f(FileDialogPath));
		IMalloc *imalloc=0;
		if (SUCCEEDED(SHGetMalloc(&imalloc))){
			imalloc->Free(pidl);
			imalloc->Release();
		}
	}
	strcpy(HuiFileDialogPath,de_sys_str_f(_path_));

	return strlen(HuiFileDialogPath)>0;
#endif
#ifdef HUI_API_GTK
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget* dlg=gtk_file_chooser_dialog_new(	sys_str(title),
												w,
												GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
												GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
												GTK_STOCK_OPEN,		GTK_RESPONSE_ACCEPT,
												NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg),dir);
	int r=gtk_dialog_run (GTK_DIALOG (dlg));
	if (r==GTK_RESPONSE_ACCEPT){
		strcpy(HuiFileDialogPath,gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (dlg)));
		dir_ensure_ending(HuiFileDialogPath,true);
	}
	gtk_widget_destroy(dlg);
	return (r==GTK_RESPONSE_ACCEPT);
#endif
	return false;
}

// Datei-Auswahl zum Oeffnen (filter in der Form "*.txt")
bool HuiFileDialogOpen(CHuiWindow *win,char *title,char *dir,char *show_filter,char *filter)
{
	msg_db_r("HuiFileDialogOpen",1);
#ifdef HUI_API_WIN
	HWND hWnd=win?win->hWnd:NULL;
	TCHAR _filter_[256];
	// filter = $show_filter\0$filter\0\0
	ZeroMemory(_filter_,sizeof(_filter_));
	_tcscpy(_filter_,sys_str(show_filter));
	_tcscpy(&_filter_[_tcslen(_filter_)+1],sys_str(filter));
	_tcscpy(_path_,sys_str_f(dir));
	_tcscpy(_filename_,_T(""));
	_tcscpy(_complete_name_,_T(""));
	OPENFILENAME ofn={	sizeof(OPENFILENAME),
						hWnd,NULL,
						_filter_,
						NULL,0,1,_complete_name_,TCHAR_STRING_LENGTH,
						_filename_,TCHAR_STRING_LENGTH,_path_,sys_str(title),OFN_FILEMUSTEXIST,0,1,_T("????"),0,NULL,NULL};
	bool done=(GetOpenFileName(&ofn)==TRUE);
	strcpy(HuiFileDialogCompleteName,de_sys_str_f(_complete_name_));
	strcpy(HuiFileDialogFile,de_sys_str_f(_filename_));
	strcpy(HuiFileDialogPath,HuiFileDialogCompleteName);
	strstr(HuiFileDialogPath,HuiFileDialogFile)[0]=0;
	msg_db_l(1);
	return done;
#endif
#ifdef HUI_API_GTK
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	msg_db_m("dialog_new",1);
	GtkWidget *dlg=gtk_file_chooser_dialog_new(	sys_str(title),
												w,
												GTK_FILE_CHOOSER_ACTION_OPEN,
												GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
												GTK_STOCK_OPEN,		GTK_RESPONSE_ACCEPT,
												NULL);
	gtk_window_set_modal(GTK_WINDOW(dlg),true);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg),SysFileName(dir));
	GtkFileFilter *gtk_filter=gtk_file_filter_new();
	gtk_file_filter_set_name(gtk_filter,sys_str(show_filter));
	char *ff=filter,_ff[128];
	while(strstr(ff,";")){
		strcpy(_ff,ff);
		strstr(_ff,";")[0]=0;
		gtk_file_filter_add_pattern(gtk_filter,sys_str(_ff));
		ff+=strlen(_ff)+1;
	}
	gtk_file_filter_add_pattern(gtk_filter,sys_str(ff));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg),gtk_filter);
	gtk_widget_show_all(dlg);
	msg_db_m("dialog_run",1);
	int r=gtk_dialog_run(GTK_DIALOG(dlg));
	msg_db_m("ok",1);
	if (r==GTK_RESPONSE_ACCEPT){
		strcpy(HuiFileDialogCompleteName,gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg)));
		strcpy(HuiFileDialogPath,gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dlg)));
		strcpy(HuiFileDialogFile,HuiFileDialogCompleteName+strlen(HuiFileDialogPath)+1);
		dir_ensure_ending(HuiFileDialogPath,true);
	}
	gtk_widget_destroy(dlg);
	msg_db_l(1);
	return (r==GTK_RESPONSE_ACCEPT);
#endif
	msg_db_l(1);
	return false;
}

static void try_to_ensure_extension(char *filename, char *filter)
{
	// multiple choices -> ignore
	if (strstr(filter, ";"))
		return;
	int ln = strlen(filename);
	int lf = strlen(filter);
	// not the wanted extension -> add
	if (strcmp(&filename[ln-lf+1], &filter[1]) != 0)
		strcat(filename, &filter[1]);
}

// Datei-Auswahl zum Speichern
bool HuiFileDialogSave(CHuiWindow *win,char *title,char *dir,char *show_filter,char *filter)
{
#ifdef HUI_API_WIN
	HWND hWnd = win ? win->hWnd : NULL;
	TCHAR _filter_[256];
	// filter = $show_filter\0$filter\0\0
	ZeroMemory(_filter_,sizeof(_filter_));
	_tcscpy(_filter_,sys_str(show_filter));
	_tcscpy(&_filter_[_tcslen(_filter_)+1],sys_str(filter));
	_tcscpy( _path_,sys_str_f( dir ) );
	_tcscpy( _filename_ , _T( "" ) );
	_tcscpy( _complete_name_ , _T( "" ) );
	OPENFILENAME ofn={	sizeof( OPENFILENAME ),
						hWnd, NULL,
						_filter_,
						NULL, 0, 1, _complete_name_, TCHAR_STRING_LENGTH,
						_filename_, TCHAR_STRING_LENGTH, _path_, sys_str(title),
						OFN_FILEMUSTEXIST, 0, 1, _T( "????" ), 0, NULL, NULL };
	if ( GetSaveFileName( &ofn ) == TRUE ){
		strcpy( HuiFileDialogCompleteName, de_sys_str_f( _complete_name_ ) );
		strcpy( HuiFileDialogFile, de_sys_str_f( _filename_ ) );
		strcpy( HuiFileDialogPath, HuiFileDialogCompleteName );
		strstr( HuiFileDialogPath, HuiFileDialogFile )[0] = 0;
		return true;
	}
#endif
#ifdef HUI_API_GTK
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget* dlg=gtk_file_chooser_dialog_new(	sys_str(title),
												w,
												GTK_FILE_CHOOSER_ACTION_SAVE,
												GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
												GTK_STOCK_SAVE,		GTK_RESPONSE_ACCEPT,
												NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg),SysFileName(dir));
	GtkFileFilter *gtk_filter=gtk_file_filter_new();
	gtk_file_filter_set_name(gtk_filter,sys_str(show_filter));
	char *ff=filter,_ff[128];
	while(strstr(ff,";")){
		strcpy(_ff,ff);
		strstr(_ff,";")[0]=0;
		gtk_file_filter_add_pattern(gtk_filter,sys_str(_ff));
		ff+=strlen(_ff)+1;
	}
	gtk_file_filter_add_pattern(gtk_filter,sys_str(ff));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg),gtk_filter);
	int r=gtk_dialog_run(GTK_DIALOG(dlg));
	if (r==GTK_RESPONSE_ACCEPT){
		strcpy(HuiFileDialogCompleteName,gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg)));
		strcpy(HuiFileDialogPath,gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dlg)));
		strcpy(HuiFileDialogFile,HuiFileDialogCompleteName+strlen(HuiFileDialogPath)+1);
		dir_ensure_ending(HuiFileDialogPath,true);
		try_to_ensure_extension(HuiFileDialogCompleteName, filter);
	}
	gtk_widget_destroy (dlg);
	return (r==GTK_RESPONSE_ACCEPT);
#endif
	return false;
}

bool HuiSelectColor(CHuiWindow *win,int r,int g,int b)
{
#ifdef HUI_API_WIN
	HWND hWnd = win ? win->hWnd : NULL;
	CHOOSECOLOR cc;
	static COLORREF cust_color[16];
	ZeroMemory( &cc, sizeof( cc ) );
	cc.lStructSize = sizeof( cc );
	cc.hwndOwner = hWnd;
	cc.lpCustColors = cust_color;
	cc.rgbResult = RGB( r, g, b );
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;
	if ( ChooseColor( &cc ) == TRUE ){
		HuiColor[0] = ( cc.rgbResult & 0xff ); // red
		HuiColor[1] = ( ( cc.rgbResult >> 8 ) & 0xff ); // green
		HuiColor[2] = ( ( cc.rgbResult >> 16 ) & 0xff ); // blue
		HuiColor[3] = 255; // alpha.... :---(
		return true;
	}
#endif
#ifdef HUI_API_GTK
	msg_todo("HuiSelectColor (GTK)");
#endif
	return false;
}

int HuiQuestionBox(CHuiWindow *win,char *title,char *text,bool allow_cancel)
{
#ifdef HUI_API_WIN
	HWND hWnd = win ? win->hWnd : NULL;
	int r = MessageBox(	hWnd,
						sys_str( text ),
						sys_str( title ),
						( allow_cancel ? MB_YESNOCANCEL : MB_YESNO ) | MB_ICONQUESTION );
	if ( r == IDYES )	return HUI_YES;
	if ( r == IDNO )	return HUI_NO;
#endif
#ifdef HUI_API_GTK
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget *dlg=gtk_message_dialog_new(w,GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,"%s",sys_str(text));
	gtk_window_set_modal(GTK_WINDOW(dlg),true);
	gtk_window_resize(GTK_WINDOW(dlg),300,100);
	gtk_window_set_title(GTK_WINDOW(dlg),sys_str(title));
	gtk_widget_show_all(dlg);
	gint result = gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy(dlg);
	switch (result){
		case GTK_RESPONSE_YES:
			return HUI_YES;
		case GTK_RESPONSE_NO:
			return HUI_NO;
    }
#endif
	return HUI_CANCEL;
}

void HuiInfoBox(CHuiWindow *win,char *title,char *text)
{
#ifdef HUI_API_WIN
	HWND hWnd = win ? win->hWnd : NULL;
	MessageBox(	hWnd,
				sys_str( text ),
				sys_str( title ),
				MB_OK | MB_ICONINFORMATION );// | MB_RTLREADING);
#endif
#ifdef HUI_API_GTK
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget *dlg=gtk_message_dialog_new( w, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", sys_str(text) );
	gtk_window_set_modal(GTK_WINDOW(dlg),true);
	gtk_window_resize(GTK_WINDOW(dlg),300,100);
	gtk_window_set_title(GTK_WINDOW(dlg),sys_str(title));
	gtk_widget_show_all(dlg);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
#endif
}

void HuiErrorBox(CHuiWindow *win,char *title,char *text)
{
#ifdef HUI_API_WIN
	HWND hWnd = win ? win->hWnd : NULL;
	MessageBox(	hWnd,
				sys_str( text ),
				sys_str( title ),
				MB_OK | MB_ICONERROR);
#endif
#ifdef HUI_API_GTK
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget *dlg=gtk_message_dialog_new( w, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", sys_str(text) );
	gtk_window_set_modal(GTK_WINDOW(dlg),true);
	gtk_window_resize(GTK_WINDOW(dlg),300,100);
	gtk_window_set_title(GTK_WINDOW(dlg),sys_str(title));
	gtk_widget_show_all(dlg);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
#endif
}

int HuiLoadImage(char *filename)
{
	for (int i=0;i<hui_image_file.size();i++)
		if (strcmp(hui_image_file[i].c_str(), filename) == 0)
			return i + 1024;
	std::string f;
	f = filename;
	hui_image_file.push_back(f);
	return hui_image_file.size() - 1 + 1024;
}

char RegistryString[1024];

static bool ConfigLoaded=false;
static int NumConfigs=0;
static char ConfigName[128][64],ConfigStr[128][256];

static void LoadConfigFile()
{
	CFile *f = FileOpen(string(HuiAppDirectory, "Data/config.txt"));
	NumConfigs = 0;
	if (f){
		NumConfigs = f->ReadIntC();
		for (int i=0;i<NumConfigs;i++){
			strcpy(ConfigName[i], &f->ReadStr()[3]);
			strcpy(ConfigStr[i], f->ReadStr());
		}
		FileClose(f);
	}
	ConfigLoaded = true;
}

static void SaveConfigFile()
{
	dir_create(string(HuiAppDirectory, "Data"));
	CFile *f = FileCreate(string(HuiAppDirectory,"Data/config.txt"));
	f->WriteStr("// NumConfigs");
	f->WriteInt(NumConfigs);
	for (int i=0;i<NumConfigs;i++){
		f->WriteStr(string("// ", ConfigName[i]));
		f->WriteStr(ConfigStr[i]);
	}
	f->WriteStr("#");
	FileClose(f);
	ConfigLoaded = true;
}

void HuiConfigWriteInt(char *name,int val)
{
	HuiConfigWriteStr(name,i2s(val));
}

void HuiConfigWriteBool(char *name,bool val)
{
	HuiConfigWriteStr(name,(val?(char*)"1":(char*)"0"));
}

void HuiConfigWriteStr(char *name,char *str)
{
	for (int i=0;i<NumConfigs;i++)
		if (strcmp(ConfigName[i],name)==0){
			strcpy(ConfigStr[i],str);
			//SaveConfigFile();
			return;
		}
	strcpy(ConfigStr[NumConfigs],str);
	strcpy(ConfigName[NumConfigs],name);
	NumConfigs++;
	//SaveConfigFile();
}

void HuiConfigReadInt(char *name,int &val,int default_val)
{
	/*if (!ConfigLoaded)
		LoadConfigFile();
	for (int i=0;i<NumConfigs;i++)
		if (strcmp(ConfigName[i],name)==0){
			val=s2i(ConfigStr[i]);
			return;
		}
	val=default_val;*/
	char *temp=_file_get_str_();
	HuiConfigReadStr(name,temp,i2s(default_val));
	val=s2i(temp);
}

void HuiConfigReadBool(char *name,bool &val,bool default_val)
{
	/*if (!ConfigLoaded)
		LoadConfigFile();
	for (int i=0;i<NumConfigs;i++)
		if (strcmp(ConfigName[i],name)==0){
			val=(strcmp(ConfigStr[i],"1")==0);
			return;
		}
	val=default_val;*/
	int ttt;
	HuiConfigReadInt(name,ttt,(default_val?1:0));
	val=(ttt==1);
}

void HuiConfigReadStr(char *name,char *str,char *default_str)
{
	if (!ConfigLoaded)
		LoadConfigFile();
	for (int i=0;i<NumConfigs;i++)
		if (strcmp(ConfigName[i],name)==0){
			strcpy(str,ConfigStr[i]);
			return;
		}
	if (default_str)
		strcpy(str,default_str);
	else
		strcpy(str,"");
}

#ifdef HUI_OS_WINDOWS
	TCHAR t_dot_end[256],t_desc[256],t_dot_end_desc[256],t_desc_shell[256],t_cmd[256],t_desc_shell_cmd[256],t_desc_shell_cmd_cmd[256],t_cmd_line[256],t_desc_icon[256],t_icon_0[256];
#endif

void HuiRegisterFileType(char *ending,char *description,char *icon_path,char *open_with,char *command_name,bool set_default)
{
#ifdef HUI_OS_WINDOWS
	_tcscpy(t_dot_end,hui_tchar_str(string(".",ending)));
	_tcscpy(t_desc,hui_tchar_str(description));
	_tcscpy(t_dot_end_desc,hui_tchar_str(string(".",ending,"\\",description)));
	_tcscpy(t_desc_shell,hui_tchar_str(string(description,"\\shell")));
	_tcscpy(t_cmd,hui_tchar_str(command_name));
	_tcscpy(t_desc_shell_cmd,hui_tchar_str(string(description,"\\shell\\",command_name)));
	_tcscpy(t_desc_shell_cmd_cmd,hui_tchar_str(string(description,"\\shell\\",command_name,"\\command")));
	_tcscpy(t_cmd_line,hui_tchar_str(string("\"",open_with,"\" %1")));
	_tcscpy(t_desc_icon,hui_tchar_str(string(description,"\\DefaultIcon")));
	HKEY hkey;

	// $ending -> $description
	RegCreateKeyEx(HKEY_CLASSES_ROOT,t_dot_end,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
	RegSetValueEx(hkey,_T(""),0,REG_SZ,(BYTE*)t_desc,_tchar_str_size_(t_desc));

	// $ending\$description -> $description
	RegCreateKeyEx(HKEY_CLASSES_ROOT,t_dot_end_desc,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
	RegSetValueEx(hkey,_T(""),0,REG_SZ,(BYTE*)t_desc,_tchar_str_size_(t_desc));

	// $description -> $description
	RegCreateKeyEx(HKEY_CLASSES_ROOT,t_desc,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
	RegSetValueEx(hkey,_T(""),0,REG_SZ,(BYTE*)t_desc,_tchar_str_size_(t_desc));

	if (strlen(open_with)>0){
		// $description\shell -> $command_name
		RegCreateKeyEx(HKEY_CLASSES_ROOT,t_desc_shell,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
		if (set_default)
			RegSetValueEx(hkey,_T(""),0,REG_SZ,(BYTE*)t_cmd,_tchar_str_size_(t_cmd));

		// $description\shell\$command_name\command -> "$open_with" %1
		RegCreateKeyEx(HKEY_CLASSES_ROOT,t_desc_shell_cmd,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
		RegCreateKeyEx(HKEY_CLASSES_ROOT,t_desc_shell_cmd_cmd,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
		RegSetValueEx(hkey,_T(""),0,REG_SZ,(BYTE*)t_cmd_line,_tchar_str_size_(t_cmd_line));
	}
	if (icon_path)
		if (strlen(icon_path)>0){
			_tcscpy(t_icon_0,hui_tchar_str(string(icon_path,",0")));
			// $description\DefaultIcon -> $icon_path,0
			RegCreateKeyEx(HKEY_CLASSES_ROOT,t_desc_icon,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
			RegSetValueEx(hkey,_T(""),0,REG_SZ,(BYTE*)t_icon_0,_tchar_str_size_(t_icon_0));
	}
#endif
}

void HuiCopyToClipBoard(char *buffer,int length)
{
#ifdef HUI_API_WIN
	if ((!buffer)||(length<1))	return;
	if (!OpenClipboard(NULL))
		return;

	int i,nn=0; // Anzahl der Zeilenumbrueche
	for (i=0;i<length;i++)
		if (buffer[i]=='\n')
			nn++;

	char *str=new char[length+nn+1];
	HGLOBAL hglbCopy;
	EmptyClipboard();

	// Pointer vorbereiten
	hglbCopy=GlobalAlloc(GMEM_MOVEABLE,sizeof(WCHAR)*(length+nn+1));
	if (!hglbCopy){
		CloseClipboard();
		return;
	}
	WCHAR *wstr=(WCHAR*)GlobalLock(hglbCopy);

	// befuellen
	int l=0;
	for (i=0;i<length;i++){
		if (buffer[i]=='\n'){
			str[l]='\r';
			l++;
		}
		str[l]=buffer[i];
		l++;
	}
	str[l+1]=0;

	MultiByteToWideChar(CP_UTF8,0,(LPCSTR)str,-1,wstr,length+nn+1);
	delete(str);

	GlobalUnlock(hglbCopy);
	SetClipboardData(CF_UNICODETEXT,wstr);
	CloseClipboard();
#endif
#ifdef HUI_API_GTK
	GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(cb,buffer,length);
#endif
}

void HuiPasteFromClipBoard(char **buffer,int &length)
{
	if (*buffer)	delete[](*buffer);
	length=0;
#ifdef HUI_API_WIN

	if (!OpenClipboard(NULL))
		return;
	int nn=0;
	WCHAR *wstr=(WCHAR*)GetClipboardData(CF_UNICODETEXT);
	//char *str=(char*)GetClipboardData(CF_TEXT);
	CloseClipboard();

	int lll=WideCharToMultiByte(CP_UTF8,0,wstr,-1,NULL,0,NULL,NULL)+4;
		//HuiInfoBox(NULL,i2s(lll),"");
	char *str=new char[lll];
	WideCharToMultiByte(CP_UTF8,0,wstr,-1,(LPSTR)str,lll,NULL,NULL);
	delete[](wstr);

	// doppelte Zeilenumbrueche finden
	int len=(int)strlen(str);
	for (int i=0;i<len;i++)
		if (str[i]=='\r')
			nn++;

	(*buffer)=new char[len-nn+5];
	length=0;

	for (int i=0;i<len;i++){
		if (str[i]=='\r')
			continue;
		(*buffer)[length]=str[i];
		length++;
	}
	(*buffer)[length]=0;
#endif
#ifdef HUI_API_GTK
	//msg_write("--------a");
	GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
	//msg_write("--------b");
	(*buffer)=gtk_clipboard_wait_for_text(cb);
	//msg_write((int)(long)*buffer);
	length=0;
	//msg_write(*buffer);
	if (*buffer)
		length=strlen(*buffer);
	//msg_write(length);
#endif
}

void HuiOpenDocument(char *filename)
{
#ifdef HUI_OS_WINDOWS
	ShellExecute(NULL,_T(""),hui_tchar_str(filename),_T(""),_T(""),SW_SHOW);
#endif
#ifdef HUI_OS_LINUX
	int r=system(string("gnome-open ",filename));
#endif
}


static void UpdateMenuLanguage(CHuiMenu *m)
{
	msg_db_r("UpdateMenuLanguage", 1);
	for (int i=0;i<m->Item.size();i++){
		sHuiMenuItem *it = &m->Item[i];
		if (it->SubMenu)
			UpdateMenuLanguage(it->SubMenu);
		if ((it->ID < 0) || (it->IsSeparator))
			continue;
		bool enabled = it->Enabled;
		#ifdef HUI_API_WIN
			strcpy(it->Name, HuiGetLanguage(it->ID));
			ModifyMenu(m->hMenu, i, MF_STRING | MF_BYPOSITION, it->ID, get_lang_sys(it->ID, "", true));
		#endif
		#ifdef HUI_API_GTK
			msg_todo("HuiUpdateMenuLanguage (Linux)");
			//gtk_menu_item_set_label(GTK_MENU_ITEM(it->g_item), get_lang_sys(it->ID, "", true));
		#endif
		m->EnableItem(it->ID, enabled);
	}
	msg_db_l(1);
}

void HuiUpdateAll()
{
	msg_db_r("HuiUpdateAll", 1);
	// update windows
	for (int i=0;i<_HuiWindow_.size();i++){
		for (int j=0;j<_HuiWindow_[i]->Control.size();j++){
			int id=_HuiWindow_[i]->Control[j].ID;
			if (!cur_lang->Text[id].empty())
				_HuiWindow_[i]->SetControlText(id,HuiGetLanguage(id));
		}

		// update menu
		if (_HuiWindow_[i]->Menu)
			UpdateMenuLanguage(_HuiWindow_[i]->Menu);
	}
	msg_db_l(1);
}

char *ConvertReturns(char *str);

void HuiLoadResource(char *filename)
{
	msg_db_r("HuiLoadResource", 1);
	// dirty...
	_HuiResource_.clear();
	HuiLanguage.clear();
	HuiLanguageName.clear();

	CFile *f = FileOpen(filename);
	if (f){
		int ffv = f->ReadFileFormatVersion();
		int nres = f->ReadIntC();
		for (int i=0;i<nres;i++){
			sHuiResource res;
			res.cmd.clear();
			res.type = f->ReadIntC();
			res.id = f->ReadInt();
			res.i_param[0] = f->ReadInt();
			res.i_param[1] = f->ReadInt();
			res.i_param[2] = f->ReadInt();
			res.i_param[3] = f->ReadInt();
			res.i_param[4] = f->ReadInt();
			res.i_param[5] = f->ReadInt();
			res.i_param[6] = f->ReadInt();
			res.b_param[0] = f->ReadBool();
			res.b_param[1] = f->ReadBool();
			int n = f->ReadInt();
			for (int j=0;j<n;j++){
				sHuiResourceCommand cmd;
				cmd.type = f->ReadInt();
				cmd.id = f->ReadInt();
				cmd.i_param[0] = f->ReadInt();
				cmd.i_param[1] = f->ReadInt();
				cmd.i_param[2] = f->ReadInt();
				cmd.i_param[3] = f->ReadInt();
				cmd.i_param[4] = f->ReadInt();
				cmd.i_param[5] = f->ReadInt();
				cmd.i_param[6] = f->ReadInt();
				cmd.b_param[0] = f->ReadBool();
				cmd.b_param[1] = f->ReadBool();
				res.cmd.push_back(cmd);
			}
			_HuiResource_.push_back(res);
		}

		// languages
		int nl = f->ReadIntC();
		for (int l=0;l<nl;l++){
			sHuiLanguage hl;
			hl.Text.clear();
			hl.Translation.clear();

			// Language
			strcpy(hl.Name, f->ReadStrC());
			HuiLanguageName.push_back(hl.Name);

			//  NumIDs
			std::string t;
			int n = f->ReadIntC();
			f->ReadComment(); // Text
			for (int i=0;i<n;i++){
				t = ConvertReturns(f->ReadStr());
				hl.Text.push_back(t);
			}
			// Num Language Strings
			n = f->ReadIntC();
			// Text
			f->ReadComment();
			for (int i=0;i<n;i++){
				sHuiLanguageTranslation s;
				s.Orig = ConvertReturns(f->ReadStr());
				s.Lang = ConvertReturns(f->ReadStr());
				hl.Translation.push_back(s);
			}
			HuiLanguage.push_back(hl);
		}
	}
	FileClose(f);
	msg_db_l(1);
}

char ReturnStr[2048];

char *ConvertReturns(char *str)
{
	int l=0;
	int sl=(int)strlen(str);
	for (int i=0;i<sl;i++){
		ReturnStr[l++]=str[i];
		if ((str[i]=='\\')&&(str[i+1]=='n')){
			ReturnStr[l - 1]='\n';
			i++;
		}else if ((str[i]=='\\')&&(str[i+1]=='\\')){
			ReturnStr[l - 1]='\\';
			i++;
		}else if ((str[i]=='\\')&&(str[i+1]=='?')){
			ReturnStr[l - 1]='?';
			i++;
		}else if ((str[i]=='\\')&&(str[i+1]=='t')){
			ReturnStr[l - 1]='\t';
			i++;
		}else if ((str[i]=='\\')&&(str[i+1]=='"')){
			ReturnStr[l - 1]='"';
			i++;
		}
	}
	ReturnStr[l]=0;
	return ReturnStr;
}

void HuiSetLanguage(char *language)
{
	msg_db_r("HuiSetLang", 1);
	cur_lang = NULL;
	HuiLanguaged=false;
	for (int i=0;i<HuiLanguage.size();i++)
		if (strcmp(HuiLanguage[i].Name, language) == 0){
			cur_lang = &HuiLanguage[i];
			HuiLanguaged = true;
			strcpy(HuiCurLanguageName, language);
		}

	HuiUpdateAll();
	msg_db_l(1);
}

char *HuiGetLanguage(int id)
{
	if ((!HuiLanguaged)||(id<0)||(id>=cur_lang->Text.size()))
		return "";
	if (cur_lang->Text[id].empty())
		return "";//"???";
	return (char*)cur_lang->Text[id].c_str();
}

// pre-translated...translations
char *HuiGetLanguageS(char *str)
{
	if (!HuiLanguaged)
		return str;
	for (int i=0;i<cur_lang->Translation.size();i++)
		if (strcmp(str, cur_lang->Translation[i].Orig.c_str()) == 0)
			return (char*)cur_lang->Translation[i].Lang.c_str();
	return str;
}


char *get_lang(int id,char *text,bool allow_keys)
{
	if (strlen(text)>0)
		return text;
	if ((!HuiLanguaged)||(id<0)||(id>=cur_lang->Text.size()))
		return text;
	if (cur_lang->Text[id].empty())
		return text;
#ifdef HUI_API_WIN
	if (allow_keys)
		for (int i=0;i<HuiKeyCode.size();i++)
			if (id==HuiKeyCode[i].ID)
				return string(cur_lang->Text[id].c_str(), "\t", HuiGetKeyCodeName(HuiKeyCode[i].Code));
#endif
	return (char*)cur_lang->Text[id].c_str();
}

#ifdef HUI_API_WIN
	TCHAR *get_lang_sys(int id,char *text,bool allow_keys)
#else
	char *get_lang_sys(int id,char *text,bool allow_keys)
#endif
{
	return sys_str(get_lang(id,text,allow_keys));
}

void HuiAddKeyCode(int id, int key_code)
{
	sHuiKeyCode k;
	k.Code = key_code;
	k.ID = id;
	HuiKeyCode.push_back(k);
}

char *HuiGetKeyName(int k)
{
	if (k==KEY_LCONTROL)	return "ControlL";
	if (k==KEY_RCONTROL)	return "ControlR";
	if (k==KEY_LSHIFT)		return "ShiftL";
	if (k==KEY_RSHIFT)		return "ShiftR";
	if (k==KEY_LALT)		return "AltL";
	if (k==KEY_RALT)		return "AltR";
	if (k==KEY_ADD)			return "Add";
	if (k==KEY_SUBTRACT)	return "Subtract";
	if (k==KEY_FENCE)		return "Fence";
	if (k==KEY_GRAVE)		return "Grave";
	if (k==KEY_END)			return "End";
	if (k==KEY_NEXT)		return "Next";
	if (k==KEY_PRIOR)		return "Prior";
	if (k==KEY_UP)			return "ArrowUp";
	if (k==KEY_DOWN)		return "ArrowDown";
	if (k==KEY_LEFT)		return "ArrowLeft";
	if (k==KEY_RIGHT)		return "ArrowRight";
	if (k==KEY_RETURN)		return "Return";
	if (k==KEY_ESCAPE)		return "Escape";
	if (k==KEY_INSERT)		return "Insert";
	if (k==KEY_DELETE)		return "Delete";
	if (k==KEY_SPACE)		return "Space";
	if (k==KEY_F1)			return "F1";
	if (k==KEY_F2)			return "F2";
	if (k==KEY_F3)			return "F3";
	if (k==KEY_F4)			return "F4";
	if (k==KEY_F5)			return "F5";
	if (k==KEY_F6)			return "F6";
	if (k==KEY_F7)			return "F7";
	if (k==KEY_F8)			return "F8";
	if (k==KEY_F9)			return "F9";
	if (k==KEY_F10)			return "F10";
	if (k==KEY_F11)			return "F11";
	if (k==KEY_F12)			return "F12";
	if (k==KEY_1)			return "1";
	if (k==KEY_2)			return "2";
	if (k==KEY_3)			return "3";
	if (k==KEY_4)			return "4";
	if (k==KEY_5)			return "5";
	if (k==KEY_6)			return "6";
	if (k==KEY_7)			return "7";
	if (k==KEY_8)			return "8";
	if (k==KEY_9)			return "9";
	if (k==KEY_0)			return "0";
	if (k==KEY_A)			return "A";
	if (k==KEY_B)			return "B";
	if (k==KEY_C)			return "C";
	if (k==KEY_D)			return "D";
	if (k==KEY_E)			return "E";
	if (k==KEY_F)			return "F";
	if (k==KEY_G)			return "G";
	if (k==KEY_H)			return "H";
	if (k==KEY_I)			return "I";
	if (k==KEY_J)			return "J";
	if (k==KEY_K)			return "K";
	if (k==KEY_L)			return "L";
	if (k==KEY_M)			return "M";
	if (k==KEY_N)			return "N";
	if (k==KEY_O)			return "O";
	if (k==KEY_P)			return "P";
	if (k==KEY_Q)			return "Q";
	if (k==KEY_R)			return "R";
	if (k==KEY_S)			return "S";
	if (k==KEY_T)			return "T";
	if (k==KEY_U)			return "U";
	if (k==KEY_V)			return "V";
	if (k==KEY_W)			return "W";
	if (k==KEY_X)			return "X";
	if (k==KEY_Y)			return "Y";
	if (k==KEY_Z)			return "Z";
	if (k==KEY_BACKSPACE)	return "Backspace";
	if (k==KEY_TAB)			return "Tab";
	if (k==KEY_HOME)		return "Home";
	if (k==KEY_NUM_0)		return "Num 0";
	if (k==KEY_NUM_1)		return "Num 1";
	if (k==KEY_NUM_2)		return "Num 2";
	if (k==KEY_NUM_3)		return "Num 3";
	if (k==KEY_NUM_4)		return "Num 4";
	if (k==KEY_NUM_5)		return "Num 5";
	if (k==KEY_NUM_6)		return "Num 6";
	if (k==KEY_NUM_7)		return "Num 7";
	if (k==KEY_NUM_8)		return "Num 8";
	if (k==KEY_NUM_9)		return "Num 9";
	if (k==KEY_NUM_ADD)		return "Num Add";
	if (k==KEY_NUM_SUBTRACT)return "Num Subtract";
	if (k==KEY_NUM_MULTIPLY)return "Num Multiply";
	if (k==KEY_NUM_DIVIDE)	return "Num Divide";
	if (k==KEY_NUM_ENTER)	return "Num Enter";
	if (k==KEY_NUM_COMMA)	return "Num Comma";
	if (k==KEY_COMMA)		return "Comma";
	if (k==KEY_DOT)			return "Dot";
	if (k==KEY_SMALLER)		return "<";
	if (k==KEY_SZ)			return "&s";
	if (k==KEY_AE)			return "&A";
	if (k==KEY_OE)			return "&O";
	if (k==KEY_UE)			return "&U";
	if (k==KEY_WINDOWS_R)	return "WindowsR";
	if (k==KEY_WINDOWS_L)	return "WindowsL";
	return "";
}

int HuiCreateTimer()
{
	sHuiTimer t;
	HuiTimer.push_back(t);
	HuiGetTime(HuiTimer.size() - 1); // reset...
	return HuiTimer.size() - 1;
}

float HuiGetTime(int index)
{
	/*if (index<0)
		return 0;*/
	float elapsed = 1;
	sHuiTimer *t = &HuiTimer[index];
	#ifdef HUI_OS_WINDOWS
		if (perf_flag)
			QueryPerformanceCounter((LARGE_INTEGER *)&t->CurTime);
		else
			t->CurTime = timeGetTime();
		elapsed = (t->CurTime - t->LastTime) * time_scale;
		t->LastTime = t->CurTime;
	#endif
	#ifdef HUI_OS_LINUX
		gettimeofday(&t->CurTime,NULL);
		elapsed = float(t->CurTime.tv_sec - t->LastTime.tv_sec) + float(t->CurTime.tv_usec - t->LastTime.tv_usec) * 0.000001f;
		t->LastTime = t->CurTime;
	#endif
	return elapsed;
}

char KeyCodeName[128];
char *HuiGetKeyCodeName(int k)
{
	if (k<0)
		strcpy(KeyCodeName,"");
	else{
		strcpy(KeyCodeName,"");
		if ((k&256)==256)
			strcat(KeyCodeName,"Ctrl+");
		if ((k&512)==512)
			strcat(KeyCodeName,"Shift+");
		strcat(KeyCodeName,HuiGetKeyName(k%256));
	}
	return KeyCodeName;
}


//----------------------------------------------------------------------------------
// resource functions



CHuiWindow *HuiCreateResourceDialog(int id,CHuiWindow *root,message_function *mf)
{
	//return HuiCreateDialog("-dialog not found in resource-",200,100,root,true,mf);
	msg_db_r("HuiCreateResourceDialog",1);
	msg_db_m(i2s(id),2);
	if (id<0){
		msg_db_l(1);
		return NULL;
	}
	for (int r=0;r<_HuiResource_.size();r++){
		sHuiResource *res = &_HuiResource_[r];
		msg_db_m(i2s(res->id),1);
		if (res->id==id){
			msg_db_m("HuiResDialog",2);
			msg_db_m(i2s(res->i_param[2]),2);
			msg_db_m(i2s(res->i_param[3]),2);
			CHuiWindow *dlg=HuiCreateDialog(HuiGetLanguage(res->id),res->i_param[2],res->i_param[3],root,res->b_param[0],mf);
			if (res->i_param[4]>=0)
				dlg->Menu=HuiCreateResourceMenu(res->i_param[4]);
			if (res->i_param[5]>=0)
				dlg->ToolBarSetByID(res->i_param[5]);
			for (int j=0;j<res->cmd.size();j++){
				sHuiResourceCommand *cmd = &res->cmd[j];
				msg_db_m(string2("%d:  %d / %d",j,(cmd->type & 1023),(cmd->type >> 10)),4);
				if ((cmd->type & 1023)==HuiCmdDialogAddControl){
					if (cmd->i_param[5]>=0)
						dlg->SetTabCreationPage( cmd->i_param[5] >> 10, cmd->i_param[5] & 1023 );
					else
						dlg->SetTabCreationPage(-1,-1);
					HuiWindowAddControl( dlg, cmd->type >> 10, HuiGetLanguage(cmd->id),
								cmd->i_param[0],cmd->i_param[1],
								cmd->i_param[2],cmd->i_param[3],
								cmd->id);
					if (!cmd->b_param[0])
						dlg->EnableControl(cmd->id,false);
					if (cmd->i_param[4]>=0)
						dlg->SetControlImage(cmd->id,cmd->i_param[4]);
				}
			}
			msg_db_m("  \\(^_^)/",1);
			msg_db_l(1);
			return dlg;
		}
	}
	msg_error(string2("HuiCreateResourceDialog  (id=%d)  m(-_-)m",id));
	CHuiWindow *d=HuiCreateDialog(string2("-dialog (id=%d) not found in resource-",id),300,200,root,true,mf);
	msg_db_l(1);
	return d;
}

CHuiMenu *_create_res_menu_(sHuiResource *res, int &index, int num)
{
	msg_db_r("_create_res_menu_",2);
	CHuiMenu *menu = new CHuiMenu();
	//msg_db_out(2,i2s(n));
	for (int i=0;i<num;i++){
		//msg_db_out(2,i2s(j));
		sHuiResourceCommand *cmd = &res->cmd[index];
		if (cmd->type == HuiCmdMenuAddItem)
			menu->AddEntry(get_lang(cmd->id, "", true), cmd->id);
		if (cmd->type == HuiCmdMenuAddItemImage)
			menu->AddEntryImage(get_lang(cmd->id, "", true), cmd->i_param[1], cmd->id);
		if (cmd->type == HuiCmdMenuAddItemCheckable)
			menu->AddEntryCheckable(get_lang(cmd->id, "", true), cmd->id);
		if (cmd->type == HuiCmdMenuAddItemSeparator)
			menu->AddSeparator();
		if (cmd->type == HuiCmdMenuAddItemPopup){
			index ++;
			CHuiMenu *sub = _create_res_menu_(res, index, cmd->i_param[0]);
			menu->AddSubMenu(get_lang(cmd->id, "", true), cmd->id, sub);
			index --;
		}
		if (!cmd->b_param[0])
			menu->EnableItem(cmd->id, false);
		index ++;
	}
	msg_db_l(2);
	return menu;
}

CHuiMenu *HuiCreateResourceMenu(int id)
{
	msg_db_r("HuiCreateResourceMenu",1);
	msg_db_m(i2s(id),2);
	if (id<0){
		msg_db_l(1);
		return NULL;
	}
	for (int r=0;r<_HuiResource_.size();r++){
		sHuiResource *res = &_HuiResource_[r];
//		msg_write>Write(res->id);
		if (res->id == id){
			int i = 0;
			msg_db_m("  \\(^_^)/",1);
			CHuiMenu *m = _create_res_menu_(res, i, res->i_param[0]);
			msg_db_l(1);
			return m;
		}
	}
	msg_error(string2("HuiCreateResourceMenu  (id=%d)  m(-_-)m", id));
	msg_db_l(1);
	return NULL;
}



