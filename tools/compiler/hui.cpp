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


char HuiVersion[32]="0.2.9.4";

#define HUI_MAX_LANGUAGE_IDS			1024
#define HUI_MAX_LANGUAGE_TEXT_LENGTH	256

#include "hui.h"
#include "file.h"
#include "msg.h"
#include <stdio.h>
#ifdef HUI_OS_WINDOWS
	#include <shlobj.h>
	#include <winuser.h>
	#include <direct.h>
	#include <commctrl.h>
	#pragma comment(lib,"winmm.lib")
#endif
#ifdef HUI_OS_LINUX
	#include <string.h>
	#include <unistd.h>
	#include <signal.h>
	#include <sys/time.h>
	#include <sys/timeb.h>
	#include <time.h>
	#include <gdk/gdkx.h>
#endif

#ifdef HUI_OS_WINDOWS
	HFONT StandartFont;
#endif

//#define UseFakeTexts

// transform small windows coordinates into big linux ones
int w2l(int i)
{
	return int(float(i)*hui->GtkCorrectionFactor);
}

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


//########################################################################################
// HUI
//########################################################################################





static bool Languaged;
static int NumLanguageIDs;
static char LanguageText[HUI_MAX_LANGUAGE_IDS][HUI_MAX_LANGUAGE_TEXT_LENGTH];



#ifdef HUI_OS_WINDOWS
	static unsigned char KeyID[256];
	HINSTANCE HuiInstance;
	char *Argument=NULL;
	void HuiSetArgs(char *arg)
	{	Argument=arg;	}
#endif
#ifdef HUI_OS_LINUX
	static int KeyID[HUI_NUM_KEYS],KeyID2[HUI_NUM_KEYS];
	GdkCursor *invisible_cursor;
	int NumArguments=0;
	char *Argument[32];
	void HuiSetArgs(int num_args,char *args[])
	{
		NumArguments=num_args;
		if (num_args>32)
			NumArguments=32;
		for (int i=0;i<NumArguments;i++)
			Argument[i]=args[i];
	}
#endif

// timers
static int NumTimers;
#ifdef HUI_OS_WINDOWS
	static LONGLONG CurTime[HUI_MAX_TIMERS];
	static LONGLONG perf_cnt;
	static bool perf_flag=false;
	static LONGLONG LastTime[HUI_MAX_TIMERS];
	static float time_scale;
#endif
#ifdef HUI_OS_LINUX
	static struct timeval CurTime[HUI_MAX_TIMERS],LastTime[HUI_MAX_TIMERS];
#endif

static int NumImages;
char image_file[128][128];
/*#ifdef HUI_OS_LINUX
#endif*/


// resource files
static int NumResources=0;
static sHuiResource HuiResource[128];



#ifdef HUI_OS_LINUX
	gboolean GTKIdleFunction(void*)
	{
		if (hui->IdleFunction)
			hui->IdleFunction();
		return TRUE;
	}
#endif

CHui *hui;

CHui::CHui()
{
	if (!msg_inited)
		msg_init();
	Running=false;
	strcpy(SingleParam,"");
	strcpy(AppFilename,"");
	strcpy(AppDirectory,"");
	#ifdef HUI_OS_WINDOWS
		//InitCommonControls(); comctl32.lib
		CoInitialize(NULL);
		//StandartFont=CreateFont(8,0,0,0,FW_NORMAL,FALSE,FALSE,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,"MS Sans Serif");
		StandartFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

		main_icon=ExtractIcon(HuiInstance,_pgmptr,0);

		strcpy(AppFilename,_pgmptr);
		strcpy(AppDirectory,_pgmptr);
		for (unsigned int i=strlen(AppDirectory)-1;i>=0;i--)
			if (AppDirectory[i]=='\\'){
				AppDirectory[i+1]=0;
				break;
			}

		if (SingleParam)
			strcpy(SingleParam,Argument);

		for (int k=0;k<256;k++)
			KeyID[k]=255;
		KeyID[VK_LCONTROL	]=KEY_LCONTROL;
		KeyID[VK_RCONTROL	]=KEY_RCONTROL;
		KeyID[VK_LSHIFT		]=KEY_LSHIFT;
		KeyID[VK_RSHIFT		]=KEY_RSHIFT;
		KeyID[VK_LMENU		]=KEY_LALT;
		KeyID[VK_RMENU		]=KEY_RALT;
		KeyID[187			]=KEY_ADD;
		KeyID[189			]=KEY_SUBTRACT;
		KeyID[191			]=KEY_FENCE;
		KeyID[220			]=KEY_GRAVE;
		KeyID[VK_END		]=KEY_END;
		KeyID[VK_NEXT		]=KEY_NEXT;
		KeyID[VK_PRIOR		]=KEY_PRIOR;
		KeyID[VK_UP			]=KEY_UP;
		KeyID[VK_DOWN		]=KEY_DOWN;
		KeyID[VK_LEFT		]=KEY_LEFT;
		KeyID[VK_RIGHT		]=KEY_RIGHT;
		KeyID[VK_RETURN		]=KEY_RETURN;
		KeyID[VK_ESCAPE		]=KEY_ESCAPE;
		KeyID[VK_INSERT		]=KEY_INSERT;
		KeyID[VK_DELETE		]=KEY_DELETE;
		KeyID[VK_SPACE		]=KEY_SPACE;
		KeyID[VK_F1			]=KEY_F1;
		KeyID[VK_F2			]=KEY_F2;
		KeyID[VK_F3			]=KEY_F3;
		KeyID[VK_F4			]=KEY_F4;
		KeyID[VK_F5			]=KEY_F5;
		KeyID[VK_F6			]=KEY_F6;
		KeyID[VK_F7			]=KEY_F7;
		KeyID[VK_F8			]=KEY_F8;
		KeyID[VK_F9			]=KEY_F9;
		KeyID[VK_F10		]=KEY_F10;
		KeyID[VK_F11		]=KEY_F11;
		KeyID[VK_F12		]=KEY_F12;
		KeyID['1'			]=KEY_1;
		KeyID['2'			]=KEY_2;
		KeyID['3'			]=KEY_3;
		KeyID['4'			]=KEY_4;
		KeyID['5'			]=KEY_5;
		KeyID['6'			]=KEY_6;
		KeyID['7'			]=KEY_7;
		KeyID['8'			]=KEY_8;
		KeyID['9'			]=KEY_9;
		KeyID['0'			]=KEY_0;
		KeyID['A'			]=KEY_A;
		KeyID['B'			]=KEY_B;
		KeyID['C'			]=KEY_C;
		KeyID['D'			]=KEY_D;
		KeyID['E'			]=KEY_E;
		KeyID['F'			]=KEY_F;
		KeyID['G'			]=KEY_G;
		KeyID['H'			]=KEY_H;
		KeyID['I'			]=KEY_I;
		KeyID['J'			]=KEY_J;
		KeyID['K'			]=KEY_K;
		KeyID['L'			]=KEY_L;
		KeyID['M'			]=KEY_M;
		KeyID['N'			]=KEY_N;
		KeyID['O'			]=KEY_O;
		KeyID['P'			]=KEY_P;
		KeyID['Q'			]=KEY_Q;
		KeyID['R'			]=KEY_R;
		KeyID['S'			]=KEY_S;
		KeyID['T'			]=KEY_T;
		KeyID['U'			]=KEY_U;
		KeyID['V'			]=KEY_V;
		KeyID['W'			]=KEY_W;
		KeyID['X'			]=KEY_X;
		KeyID['Y'			]=KEY_Y;
		KeyID['Z'			]=KEY_Z;
		KeyID[VK_BACK		]=KEY_BACKSPACE;
		KeyID[VK_TAB		]=KEY_TAB;
		KeyID[VK_HOME		]=KEY_HOME;
		KeyID[VK_NUMPAD0	]=KEY_NUM_0;
		KeyID[VK_NUMPAD1	]=KEY_NUM_1;
		KeyID[VK_NUMPAD2	]=KEY_NUM_2;
		KeyID[VK_NUMPAD3	]=KEY_NUM_3;
		KeyID[VK_NUMPAD4	]=KEY_NUM_4;
		KeyID[VK_NUMPAD5	]=KEY_NUM_5;
		KeyID[VK_NUMPAD6	]=KEY_NUM_6;
		KeyID[VK_NUMPAD7	]=KEY_NUM_7;
		KeyID[VK_NUMPAD8	]=KEY_NUM_8;
		KeyID[VK_NUMPAD9	]=KEY_NUM_9;
		KeyID[VK_ADD		]=KEY_NUM_ADD;
		KeyID[VK_SUBTRACT	]=KEY_NUM_SUBTRACT;
		KeyID[VK_MULTIPLY	]=KEY_NUM_MULTIPLY;
		KeyID[VK_DIVIDE		]=KEY_NUM_DIVIDE;
		KeyID[VK_DECIMAL	]=KEY_NUM_COMMA;
		//KeyID[VK_RETURN		]=KEY_NUM_ENTER;
		KeyID[188			]=KEY_COMMA;
		KeyID[190			]=KEY_DOT;
		KeyID[226			]=KEY_SMALLER;
		KeyID[219			]=KEY_SZ;
		KeyID[222			]=KEY_AE;
		KeyID[192			]=KEY_OE;
		KeyID[186			]=KEY_UE;
		

		// timers
		if (QueryPerformanceFrequency((LARGE_INTEGER *) &perf_cnt)){
			perf_flag=true;
			time_scale=1.0f/perf_cnt;
		}else 
			time_scale=0.001f;

	#endif
	#ifdef HUI_OS_LINUX
		gtk_init (NULL,NULL);
		gtk_idle_add(GTKIdleFunction,NULL);
		GtkCorrectionFactor=1.0f;


		if (NumArguments>1)
			strcpy(SingleParam,Argument[1]);

		if (NumArguments>0){
			if (Argument[0][0]=='/'){
				strcpy(AppFilename,Argument[0]);
			}else{
				getcwd(AppFilename,sizeof(AppFilename));
				if (AppDirectory[strlen(AppFilename)-1]!='/')
					strcat(AppFilename,"/");
				if (Argument[0][0]=='.')
					strcat(AppFilename,&Argument[0][2]);
				else
					strcat(AppFilename,Argument[0]);
			}
			strcpy(AppDirectory,dir_from_filename(AppFilename));
		}

		for (int k=0;k<HUI_NUM_KEYS;k++)
			KeyID[k]=KeyID2[k]=0;

		KeyID[KEY_LCONTROL]=GDK_Control_L;
		KeyID[KEY_RCONTROL]=GDK_Control_R;
		KeyID[KEY_LSHIFT]=GDK_Shift_L;
		KeyID[KEY_RSHIFT]=GDK_Shift_R;
		KeyID[KEY_LALT]=GDK_Alt_L;
		KeyID[KEY_RALT]=GDK_Alt_R;
		KeyID[KEY_ADD]=GDK_plus;
		KeyID[KEY_SUBTRACT]=GDK_minus;
		KeyID[KEY_FENCE]=0;
		KeyID[KEY_GRAVE]=0;
		KeyID[KEY_HOME]=GDK_Home;
		KeyID[KEY_END]=GDK_End;
		KeyID[KEY_NEXT]=GDK_Page_Up;
		KeyID[KEY_PRIOR]=GDK_Page_Down;
		KeyID[KEY_UP]=GDK_Up;
		KeyID[KEY_DOWN]=GDK_Down;
		KeyID[KEY_LEFT]=GDK_Left;
		KeyID[KEY_RIGHT]=GDK_Right;
		KeyID[KEY_RETURN]=GDK_Return;
		KeyID[KEY_ESCAPE]=GDK_Escape;
		KeyID[KEY_INSERT]=GDK_Insert;
		KeyID[KEY_DELETE]=GDK_Delete;
		KeyID[KEY_SPACE]=GDK_space;
		KeyID[KEY_F1]=GDK_F1;
		KeyID[KEY_F2]=GDK_F2;
		KeyID[KEY_F3]=GDK_F3;
		KeyID[KEY_F4]=GDK_F4;
		KeyID[KEY_F5]=GDK_F5;
		KeyID[KEY_F6]=GDK_F6;
		KeyID[KEY_F7]=GDK_F7;
		KeyID[KEY_F8]=GDK_F8;
		KeyID[KEY_F9]=GDK_F9;
		KeyID[KEY_F10]=GDK_F10;
		KeyID[KEY_F11]=GDK_F11;
		KeyID[KEY_F12]=GDK_F12;
		KeyID[KEY_1]=GDK_1;
		KeyID[KEY_2]=GDK_2;
		KeyID[KEY_3]=GDK_3;
		KeyID[KEY_4]=GDK_4;
		KeyID[KEY_5]=GDK_5;
		KeyID[KEY_6]=GDK_6;
		KeyID[KEY_7]=GDK_7;
		KeyID[KEY_8]=GDK_8;
		KeyID[KEY_9]=GDK_9;
		KeyID[KEY_0]=GDK_0;
		KeyID[KEY_A]=GDK_a;		KeyID2[KEY_A]=GDK_A;
		KeyID[KEY_B]=GDK_b;		KeyID2[KEY_B]=GDK_B;
		KeyID[KEY_C]=GDK_c;		KeyID2[KEY_C]=GDK_C;
		KeyID[KEY_D]=GDK_d;		KeyID2[KEY_D]=GDK_D;
		KeyID[KEY_E]=GDK_e;		KeyID2[KEY_E]=GDK_E;
		KeyID[KEY_F]=GDK_f;		KeyID2[KEY_F]=GDK_F;
		KeyID[KEY_G]=GDK_g;		KeyID2[KEY_G]=GDK_G;
		KeyID[KEY_H]=GDK_h;		KeyID2[KEY_H]=GDK_H;
		KeyID[KEY_I]=GDK_i;		KeyID2[KEY_I]=GDK_I;
		KeyID[KEY_J]=GDK_j;		KeyID2[KEY_J]=GDK_J;
		KeyID[KEY_K]=GDK_k;		KeyID2[KEY_K]=GDK_K;
		KeyID[KEY_L]=GDK_l;		KeyID2[KEY_L]=GDK_L;
		KeyID[KEY_M]=GDK_m;		KeyID2[KEY_M]=GDK_M;
		KeyID[KEY_N]=GDK_n;		KeyID2[KEY_N]=GDK_N;
		KeyID[KEY_O]=GDK_o;		KeyID2[KEY_O]=GDK_O;
		KeyID[KEY_P]=GDK_p;		KeyID2[KEY_P]=GDK_P;
		KeyID[KEY_Q]=GDK_q;		KeyID2[KEY_Q]=GDK_Q;
		KeyID[KEY_R]=GDK_r;		KeyID2[KEY_R]=GDK_R;
		KeyID[KEY_S]=GDK_s;		KeyID2[KEY_S]=GDK_S;
		KeyID[KEY_T]=GDK_t;		KeyID2[KEY_T]=GDK_T;
		KeyID[KEY_U]=GDK_u;		KeyID2[KEY_U]=GDK_U;
		KeyID[KEY_V]=GDK_v;		KeyID2[KEY_V]=GDK_V;
		KeyID[KEY_W]=GDK_w;		KeyID2[KEY_W]=GDK_W;
		KeyID[KEY_X]=GDK_x;		KeyID2[KEY_X]=GDK_X;
		KeyID[KEY_Y]=GDK_y;		KeyID2[KEY_Y]=GDK_Y;
		KeyID[KEY_Z]=GDK_z;		KeyID2[KEY_Z]=GDK_Z;
		KeyID[KEY_BACKSPACE]=GDK_BackSpace;
		KeyID[KEY_TAB]=GDK_Tab;
		KeyID[KEY_NUM_0]=GDK_KP_0;
		KeyID[KEY_NUM_1]=GDK_KP_1;
		KeyID[KEY_NUM_2]=GDK_KP_2;
		KeyID[KEY_NUM_3]=GDK_KP_3;
		KeyID[KEY_NUM_4]=GDK_KP_4;
		KeyID[KEY_NUM_5]=GDK_KP_5;
		KeyID[KEY_NUM_6]=GDK_KP_6;
		KeyID[KEY_NUM_7]=GDK_KP_7;
		KeyID[KEY_NUM_8]=GDK_KP_8;
		KeyID[KEY_NUM_9]=GDK_KP_9;
		KeyID[KEY_NUM_ADD]=GDK_KP_Add;
		KeyID[KEY_NUM_SUBTRACT]=GDK_KP_Subtract;
		KeyID[KEY_NUM_MULTIPLY]=GDK_KP_Multiply;
		KeyID[KEY_NUM_DIVIDE]=GDK_KP_Divide;
		KeyID[KEY_NUM_COMMA]=GDK_KP_Decimal;
		KeyID[KEY_NUM_ENTER]=GDK_KP_Enter;
		KeyID[KEY_COMMA]=GDK_colon;				KeyID2[KEY_COMMA]=GDK_semicolon;
		KeyID[KEY_DOT]=GDK_period;
		KeyID[KEY_SMALLER]=GDK_less;			KeyID2[KEY_AE]=GDK_greater;
		KeyID[KEY_SZ]=0;
		KeyID[KEY_AE]=GDK_AE;					KeyID2[KEY_AE]=GDK_ae;
		KeyID[KEY_OE]=GDK_OE;					KeyID2[KEY_AE]=GDK_oe;
		KeyID[KEY_UE]=0;//GDK_UE;
		KeyID[KEY_WINDOWS_L]=GDK_Super_L;
		KeyID[KEY_WINDOWS_R]=GDK_Super_R;


		GdkPixmap *pm=gdk_pixmap_new(NULL,1,1,1);
		GdkColor ca;
		ca.pixel=0;
		ca.red=ca.green=ca.blue=0;
		invisible_cursor=gdk_cursor_new_from_pixmap(pm,pm,&ca,&ca,0,0);
		/*invisible_cursor=gdk_cursor_new_from_pixbuf(gdk_display_get_default(),
                                          GdkPixbuf *pixbuf,
                                          0,0);*/

	#endif


	if (SingleParam[0]=='\"'){
		char temp[512];
		strcpy(temp,&SingleParam[1]);
		temp[strlen(temp)-2]=0;
		strcpy(SingleParam,temp);
	}

	ComboBoxSeparator='\\';
	_Pseudo_Byte_=0;
	Separator=&ComboBoxSeparator;
	UseFlatButtons=true;
	Multiline=false;
	NumWindows=0;
	IdleFunction=NULL;
	ErrorFunction=NULL;
	Languaged=false;
	NumTimers=0;
	CreateHiddenWindows=false;
	NumImages=0;

	sDate d=get_current_date();
	for (int j=0;j<d.milli_second+d.second;j++)
		rand();
}


CHui::~CHui()
{
}


// die System-Schleife ausfuehren, Verwendung:
// int main(...)
// {
//     hui=new CHui();
//     ...
//     return hui->Run();
// }
int CHui::Run()
{
	Running=true;
#ifdef HUI_OS_WINDOWS
	MSG messages;
	messages.message=0;
	HaveToExit=false;
	bool got_message;
	while ((!HaveToExit)&&(WM_QUIT!=messages.message)){
		bool allow=true;
		if (IdleFunction)
			got_message=(PeekMessage(&messages,NULL,0U,0U,PM_REMOVE)!=0);
		else
			got_message=(GetMessage(&messages,NULL,0,0)!=0);
		if (got_message){
			allow=false;
			TranslateMessage(&messages);
			DispatchMessage(&messages);
			for (int i=0;i<NumWindows;i++)
				if (Window[i]->hWnd==messages.hwnd){
					allow=true;
					break;
				}
		}
		if ((IdleFunction)&&(allow))
			IdleFunction();
	}
#endif
#ifdef HUI_OS_LINUX
    gtk_main();
#endif
	return 0;
}

void CHui::DoSingleMainLoop()
{
#ifdef HUI_OS_WINDOWS
	MSG messages;
	messages.message=0;
	HaveToExit=false;
	bool got_message;

		bool allow=true;
		if (IdleFunction)
			got_message=(PeekMessage(&messages,NULL,0U,0U,PM_REMOVE)!=0);
		else
			got_message=(GetMessage(&messages,NULL,0,0)!=0);
		if (got_message){
			allow=false;
			TranslateMessage(&messages);
			DispatchMessage(&messages);
			for (int i=0;i<NumWindows;i++)
				if (Window[i]->hWnd==messages.hwnd){
					allow=true;
					return;
				}
		}
		/*if ((IdleFunction)&&(allow))
			IdleFunction();*/
#endif
#ifdef HUI_OS_LINUX
	gtk_main_iteration_do(false);
#endif
}

// beendet die System-Schleife des Run()-Befehls
void CHui::End()
{
	_so("<End>");
	if (msg_inited)
		msg_end();
#ifdef HUI_OS_WINDOWS
	PostQuitMessage(0);
#endif
#ifdef HUI_OS_LINUX
    gtk_main_quit();
#endif
}

void CHui::Sleep(int duration_ms)
{
	if (duration_ms<=0)
		return;
#ifdef HUI_OS_WINDOWS
	::Sleep(duration_ms);
#endif
#ifdef HUI_OS_LINUX
	usleep(duration_ms*1000);
#endif
}

// set the default directory
void CHui::SetDirectory(char *dir)
{
#ifdef HUI_OS_WINDOWS
	_chdir(SysFileName(dir));
#endif
#ifdef HUI_OS_LINUX
	chdir(SysFileName(dir));
#endif
}

// apply a function to be executed when a critical error occures
void CHui::SetErrorFunction(void_function *error_function)
{
	ErrorFunction=error_function;
#ifdef HUI_OS_WINDOWS
	atexit(ErrorFunction);
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); // TEST ME!
#endif
#ifdef HUI_OS_LINUX
	signal(SIGSEGV,(void (*)(int))ErrorFunction);
#endif
}

// wartet, bis das Fenster sich geschlossen hat
void CHui::WaitTillWindowClosed(CHuiWindow *win)
{
	int win_no=-1;
	for (int i=0;i<hui->NumWindows;i++)
		if (hui->Window[i]==win){
			win_no=i;
			break;
		}
	if (win_no<0)
		return;

#ifdef HUI_OS_WINDOWS
	MSG messages;
	messages.message=0;
	bool got_message;
	while ((WM_QUIT!=messages.message)&&(!WindowClosed[win_no])){
		bool allow=true;
		if (IdleFunction)
			got_message=(PeekMessage(&messages,NULL,0U,0U,PM_REMOVE)!=0);
		else
			got_message=(GetMessage(&messages,NULL,0,0)!=0);
		if (got_message){
			allow=false;
			TranslateMessage(&messages);
			DispatchMessage(&messages);
		}
		if ((IdleFunction)&&(allow))
			IdleFunction();
	}
	if (WM_QUIT==messages.message)
		HaveToExit=true;
#endif
#ifdef HUI_OS_LINUX
	gtk_dialog_run(GTK_DIALOG(win->window));
	//Sleep(10000);
#endif
}

// genaue Pixel-Angaben benutzen oder besser aussehen?
void CHui::UseCorrectValues(bool correct)
{
#ifdef HUI_OS_LINUX
	if (correct)
		GtkCorrectionFactor=1.0f;
	else
		GtkCorrectionFactor=1.20f;
#endif
}

#ifdef HUI_OS_WINDOWS
//#define __T(x)  L ## x
static int CALLBACK FileDialogDirCallBack(HWND hWnd, UINT uMsg,LPARAM lParam,LPARAM lpData)
{
	if (uMsg==BFFM_INITIALIZED){
		SendMessage(hWnd,BFFM_SETSELECTION,TRUE,lpData);
		//HWND tree=FindWindowEx(hWnd,NULL,__T("SysTreeView32"),NULL);
		HWND tree=FindWindowEx(hWnd,NULL,"SysTreeView32",NULL);
		if (tree){
			hui->InfoBox(NULL,"","");
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

// Dialog zur Wahl eines Verzeichnisses (dir ist das anfangs ausgewaehlte)
bool CHui::FileDialogDir(CHuiWindow *win,char *title,char *dir/*,char *root_dir*/)
{
#ifdef HUI_OS_WINDOWS

	BROWSEINFO bi;
	ZeroMemory(&bi,sizeof(bi));
	bi.hwndOwner=win?win->hWnd:NULL;
	/*if (root_dir)
		bi.pidlRoot=*lpItemIdList;
	else*/
		bi.pidlRoot=NULL;
	bi.lpszTitle=title;
#if _MSC_VER > 1000
	bi.ulFlags= BIF_EDITBOX | 64 | BIF_RETURNONLYFSDIRS;
#else
	bi.ulFlags=BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
#endif
	bi.lpfn=FileDialogDirCallBack;
	bi.lParam=(LPARAM)SysFileName(dir);
	LPITEMIDLIST pidl=SHBrowseForFolder(&bi);
	if (pidl){
		SHGetPathFromIDList(pidl,FileDialogPath);
		IMalloc *imalloc=0;
		if (SUCCEEDED(SHGetMalloc(&imalloc))){
			imalloc->Free(pidl);
			imalloc->Release();
		}
	}

	return strlen(FileDialogPath)>0;
#endif
#ifdef HUI_OS_LINUX
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget* dlg=gtk_file_chooser_dialog_new(	SysStr(title),
												w,
												GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
												GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
												GTK_STOCK_OPEN,		GTK_RESPONSE_ACCEPT,
												NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg),dir);
	int r=gtk_dialog_run (GTK_DIALOG (dlg));
	if (r==GTK_RESPONSE_ACCEPT){
		strcpy(FileDialogPath,gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER (dlg)));
		if (FileDialogPath[strlen(FileDialogPath)-1]!='/')
			strcat(FileDialogPath,"/");
	}
	gtk_widget_destroy(dlg);
	return (r==GTK_RESPONSE_ACCEPT);
#endif
	return false;
}

// Datei-Auswahl zum Oeffnen (filter in der Form "*.txt")
bool CHui::FileDialogOpen(CHuiWindow *win,char *title,char *dir,char *show_filter,char *filter)
{
#ifdef HUI_OS_WINDOWS
	HWND hWnd=win?win->hWnd:NULL;
	char Filter[128];
	strcpy(Filter,show_filter);
	memcpy((char*)(int(Filter)+strlen(show_filter)+1),filter,strlen(filter)+1);
	Filter[strlen(show_filter)+strlen(filter)+1]=0;
	strcpy(FileDialogPath,SysFileName(dir));
	strcpy(FileDialogFile,"");
	strcpy(FileDialogCompleteName,"");
	OPENFILENAME ofn={	sizeof(OPENFILENAME),
						hWnd,NULL,
						Filter,
						NULL,0,1,FileDialogCompleteName,sizeof(FileDialogCompleteName),
						FileDialogFile,sizeof(FileDialogFile),FileDialogPath,SysStr(title),OFN_FILEMUSTEXIST,0,1,"????",0,NULL,NULL};
	bool done=(GetOpenFileName(&ofn)==TRUE);
	strcpy(FileDialogPath,FileDialogCompleteName);
	strstr(FileDialogPath,FileDialogFile)[0]=0;
	return done;
#endif
#ifdef HUI_OS_LINUX
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget* dlg=gtk_file_chooser_dialog_new(	SysStr(title),
												w,
												GTK_FILE_CHOOSER_ACTION_OPEN,
												GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
												GTK_STOCK_OPEN,		GTK_RESPONSE_ACCEPT,
												NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg),SysFileName(dir));
	GtkFileFilter *gtk_filter=gtk_file_filter_new();
	gtk_file_filter_set_name(gtk_filter,show_filter);
	char *ff=filter,_ff[128];
	while(strstr(ff,";")){
		strcpy(_ff,ff);
		strstr(_ff,";")[0]=0;
		gtk_file_filter_add_pattern(gtk_filter,_ff);
		ff+=strlen(_ff)+1;
	}
	gtk_file_filter_add_pattern(gtk_filter,ff);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg),gtk_filter);
	int r=gtk_dialog_run(GTK_DIALOG(dlg));
	if (r==GTK_RESPONSE_ACCEPT){
		strcpy(FileDialogCompleteName,gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg)));
		strcpy(FileDialogPath,gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dlg)));
		if (FileDialogPath[strlen(FileDialogPath)-1]!='/')
			strcat(FileDialogPath,"/");
		strcpy(FileDialogFile,FileDialogCompleteName+strlen(FileDialogPath)+1);
	}
	gtk_widget_destroy (dlg);
	return (r==GTK_RESPONSE_ACCEPT);
#endif
	return false;
}

// Datei-Auswahl zum Speichern
bool CHui::FileDialogSave(CHuiWindow *win,char *title,char *dir,char *show_filter,char *filter)
{
#ifdef HUI_OS_WINDOWS
	HWND hWnd=win?win->hWnd:NULL;
	char Filter[128];
	strcpy(Filter,show_filter);
	memcpy((char*)(int(Filter)+strlen(show_filter)+1),filter,strlen(filter)+1);
	Filter[strlen(show_filter)+strlen(filter)+1]=0;
	strcpy(FileDialogPath,SysFileName(dir));
	strcpy(FileDialogFile,"");
	strcpy(FileDialogCompleteName,"");
	OPENFILENAME ofn={	sizeof(OPENFILENAME),
						hWnd,NULL,
						Filter,
						NULL,0,1,FileDialogCompleteName,sizeof(FileDialogCompleteName),
						FileDialogFile,sizeof(FileDialogFile),FileDialogPath,SysStr(title),OFN_FILEMUSTEXIST,0,1,"????",0,NULL,NULL};
	bool done=(GetSaveFileName(&ofn)==TRUE);
	strcpy(FileDialogPath,FileDialogCompleteName);
	strstr(FileDialogPath,FileDialogFile)[0]=0;
	return done;
#endif
#ifdef HUI_OS_LINUX
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget* dlg=gtk_file_chooser_dialog_new(	SysStr(title),
												w,
												GTK_FILE_CHOOSER_ACTION_SAVE,
												GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
												GTK_STOCK_OPEN,		GTK_RESPONSE_ACCEPT,
												NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg),SysFileName(dir));
	GtkFileFilter *gtk_filter=gtk_file_filter_new();
	gtk_file_filter_set_name(gtk_filter,show_filter);
	gtk_file_filter_add_pattern(gtk_filter,filter);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg),gtk_filter);
	int r=gtk_dialog_run(GTK_DIALOG(dlg));
	if (r==GTK_RESPONSE_ACCEPT){
		strcpy(FileDialogCompleteName,gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg)));
		strcpy(FileDialogPath,gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dlg)));
		if (FileDialogPath[strlen(FileDialogPath)-1]!='/')
			strcat(FileDialogPath,"/");
		strcpy(FileDialogFile,FileDialogCompleteName+strlen(FileDialogPath)+1);
	}
	gtk_widget_destroy (dlg);
	return (r==GTK_RESPONSE_ACCEPT);
#endif
	return false;
}

int CHui::QuestionBox(CHuiWindow *win,char *title,char *text,bool allow_cancel)
{
#ifdef HUI_OS_WINDOWS
	HWND hWnd=win?win->hWnd:NULL;
	int r=MessageBox(hWnd,SysStr(text),SysStr(title),(allow_cancel?MB_YESNOCANCEL:MB_YESNO) | MB_ICONQUESTION);
	if (r==IDYES)	return HUI_YES;
	if (r==IDNO)	return HUI_NO;
#endif
#ifdef HUI_OS_LINUX
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget* dlg=gtk_message_dialog_new(w,GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,SysStr(text));
	gtk_window_resize(GTK_WINDOW(dlg),300,100);
	gtk_window_set_title(GTK_WINDOW(dlg),SysStr(title));
	gtk_widget_show_all (dlg);
	gint result = gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (dlg);
	switch (result){
		case GTK_RESPONSE_YES:
			return HUI_YES;
		case GTK_RESPONSE_NO:
			return HUI_NO;
    }
#endif
	return HUI_CANCEL;
}

void CHui::InfoBox(CHuiWindow *win,char *title,char *text)
{
#ifdef HUI_OS_WINDOWS
	HWND hWnd=win?win->hWnd:NULL;
	MessageBox(hWnd,SysStr(text),SysStr(title),MB_OK | MB_ICONINFORMATION);// | MB_RTLREADING);
#endif
#ifdef HUI_OS_LINUX
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget*  dlg=gtk_message_dialog_new( w, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, SysStr(text) );
	gtk_window_set_modal(GTK_WINDOW(dlg),true);
	gtk_window_resize(GTK_WINDOW(dlg),300,100);
	gtk_window_set_title(GTK_WINDOW(dlg),SysStr(title));
	gtk_widget_show_all(dlg);
	gtk_dialog_run(GTK_DIALOG (dlg));
	gtk_widget_destroy(dlg);
#endif
}

void CHui::ErrorBox(CHuiWindow *win,char *title,char *text)
{
#ifdef HUI_OS_WINDOWS
	HWND hWnd=win?win->hWnd:NULL;
	MessageBox(hWnd,SysStr(text),SysStr(title),MB_OK | MB_ICONERROR);
#endif
#ifdef HUI_OS_LINUX
	GtkWindow *w=win?GTK_WINDOW(win->window):NULL;
	GtkWidget*  dlg=gtk_message_dialog_new( w, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, SysStr(text) );
	gtk_window_set_modal(GTK_WINDOW(dlg),true);
	gtk_window_resize(GTK_WINDOW(dlg),300,100);
	gtk_window_set_title(GTK_WINDOW(dlg),SysStr(title));
	gtk_widget_show_all(dlg);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
#endif
}

int CHui::LoadImage(char *filename)
{
	for (int i=0;i<NumImages;i++)
		if (strcmp(image_file[i],filename)==0)
			return i+1024;
	strcpy(image_file[NumImages++],filename);
	return NumImages-1+1024;
}

char RegistryString[1024];

static bool ConfigLoaded=false;
static int NumConfigs=0;
static char ConfigName[128][64],ConfigStr[128][256];

static void LoadConfigFile()
{
	CFile *f=new CFile();
	f->SilentFileAccess=true;
	NumConfigs=0;
	if (f->Open(string(hui->AppDirectory,"Data/config.txt"))){
		NumConfigs=f->ReadIntC();
		for (int i=0;i<NumConfigs;i++){
			strcpy(ConfigName[i],&f->ReadStr()[3]);
			strcpy(ConfigStr[i],f->ReadStr());
		}
		f->Close();
	}
	delete(f);
	ConfigLoaded=true;
}

static void SaveConfigFile()
{
	dir_create(string(hui->AppDirectory,"Data"));
	CFile *f=new CFile();
	f->SilentFileAccess=true;
	f->Create(string(hui->AppDirectory,"Data/config.txt"));
	f->WriteStr("// NumConfigs");
	f->WriteInt(NumConfigs);
	for (int i=0;i<NumConfigs;i++){
		f->WriteStr(string("// ",ConfigName[i]));
		f->WriteStr(ConfigStr[i]);
	}
	f->WriteStr("#");
	f->Close();
	delete(f);
	ConfigLoaded=true;
}

void CHui::ConfigWriteInt(char *name,int val)
{
	ConfigWriteStr(name,i2s(val));
}

void CHui::ConfigWriteStr(char *name,char *str)
{
	for (int i=0;i<NumConfigs;i++)
		if (strcmp(ConfigName[i],name)==0){
			strcpy(ConfigStr[i],str);
			SaveConfigFile();
			return;
		}
	strcpy(ConfigStr[NumConfigs],str);
	strcpy(ConfigName[NumConfigs],name);
	NumConfigs++;
	SaveConfigFile();
}

void CHui::ConfigReadInt(char *name,int &val,int default_val)
{
	if (!ConfigLoaded)
		LoadConfigFile();
	for (int i=0;i<NumConfigs;i++)
		if (strcmp(ConfigName[i],name)==0){
			val=s2i(ConfigStr[i]);
			return;
		}
	val=default_val;
}

void CHui::ConfigReadStr(char *name,char *str,char *default_str)
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

void CHui::RegisterFileType(char *ending,char *description,char *icon_path,char *open_with,char *command_name,bool set_default)
{
#ifdef HUI_OS_WINDOWS
	HKEY hkey;
	RegCreateKeyEx(HKEY_CLASSES_ROOT,string(".",ending),0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
	RegSetValueEx(hkey,"",0,REG_SZ,(BYTE*)description,strlen(description));
	RegCreateKeyEx(HKEY_CLASSES_ROOT,string(".",ending,"\\",description),0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
	RegSetValueEx(hkey,"",0,REG_SZ,(BYTE*)description,strlen(description));
	RegCreateKeyEx(HKEY_CLASSES_ROOT,description,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
	RegSetValueEx(hkey,"",0,REG_SZ,(BYTE*)description,strlen(description));
	if (strlen(open_with)>0){
		RegCreateKeyEx(HKEY_CLASSES_ROOT,string(description,"\\shell"),0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
		if (set_default)
			RegSetValueEx(hkey,"",0,REG_SZ,(BYTE*)command_name,strlen(command_name));
		RegCreateKeyEx(HKEY_CLASSES_ROOT,string(description,"\\shell\\",command_name),0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
		RegCreateKeyEx(HKEY_CLASSES_ROOT,string(description,"\\shell\\",command_name,"\\command"),0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
		RegSetValueEx(hkey,"",0,REG_SZ,(BYTE*)string("\"",open_with,"\" %1"),strlen(open_with)+5);
	}
	if (icon_path)
		if (strlen(icon_path)>0){
			RegCreateKeyEx(HKEY_CLASSES_ROOT,string(description,"\\DefaultIcon"),0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hkey,NULL);
			RegSetValueEx(hkey,"",0,REG_SZ,(BYTE*)string(icon_path,",0"),strlen(icon_path)+2);
	}
#endif
}

void CHui::CopyToClipBoard(char *buffer,int length)
{
#ifdef HUI_OS_WINDOWS
	if ((!buffer)||(length<1))	return;
	if (!OpenClipboard(NULL))
		return;

	int i,nn=0; // Anzahl der Zeilenumbrueche
	for (i=0;i<length;i++)
		if (buffer[i]=='\n')
			nn++;

	char *str;
	HGLOBAL hglbCopy;
	EmptyClipboard();

	// Pointer vorbereiten
	hglbCopy=GlobalAlloc(GMEM_MOVEABLE,length+nn+1);
	if (!hglbCopy){
		CloseClipboard();
		return;
	}
	str=(char*)GlobalLock(hglbCopy);

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

	GlobalUnlock(hglbCopy);
	SetClipboardData(CF_TEXT,str);
	CloseClipboard();
#endif
#ifdef HUI_OS_LINUX
	GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(cb,buffer,length);
#endif
}

void CHui::PasteFromClipBoard(char **buffer,int &length)
{
	if (*buffer)	delete(*buffer);
	length=0;
#ifdef HUI_OS_WINDOWS

	if (!OpenClipboard(NULL))
		return;
	int i,nn=0;
	char *str=(char*)GetClipboardData(CF_TEXT);
	CloseClipboard();

	// doppelte Zeilenumbrueche finden
	int len=strlen(str);
	for (i=0;i<len;i++)
		if (str[i]=='\r')
			nn++;

	(*buffer)=new char[len-nn+1];
	length=0;

	for (i=0;i<len;i++){
		if (str[i]=='\r')
			continue;
		(*buffer)[length]=str[i];
		length++;
	}
	(*buffer)[length]=0;
#endif
#ifdef HUI_OS_LINUX
	msg_write("--------a");
	GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
	msg_write("--------b");
	(*buffer)=gtk_clipboard_wait_for_text(cb);
	msg_write((int)*buffer);
	length=0;
	msg_write(*buffer);
	if (*buffer)
		length=strlen(*buffer);
	msg_write(length);
#endif
}

void CHui::OpenDocument(char *filename)
{
#ifdef HUI_OS_WINDOWS
	ShellExecute(NULL,"",SysStr(filename),"","",SW_SHOW);
#endif
}

char *GetLanguaged(int id,char *text,bool allow_keys=false);

static void UpdateMenuLanguage(CHuiMenu *m)
{
	for (int i=0;i<m->NumItems;i++){
		if (m->SubMenu[i])
			UpdateMenuLanguage(m->SubMenu[i]);
		if ((m->ItemID[i]<0)||(m->ItemIsSeparator[i]))
			continue;
		bool enabled=m->ItemEnabled[i];
		#ifdef HUI_OS_WINDOWS
			strcpy(m->ItemName[i],hui->GetLanguage(m->ItemID[i]));
			ModifyMenu(m->hMenu,i,MF_STRING | MF_BYPOSITION,m->ItemID[i],SysStr(GetLanguaged(m->ItemID[i],"",true)));
		#endif
		#ifdef HUI_OS_LINUX
			msg_write("Todo:  CHui::UpdateMenuLanguage (Linux)");
		#endif
		m->EnableItem(m->ItemID[i],enabled);
	}
}

void CHui::UpdateAll()
{
	// update windows
	int i;
	for (i=0;i<NumWindows;i++){
		for (int j=0;j<Window[i]->NumControls;j++){
			int id=Window[i]->Control[j].ID;
			if (strlen(LanguageText[id])>0)
				Window[i]->SetControlText(id,GetLanguage(id));
		}

		// update menu
		if (Window[i]->Menu)
			UpdateMenuLanguage(Window[i]->Menu);
	}
}

void CHui::LoadResource(char *filename)
{
	NumResources=0;
	CFile *f=new CFile();
	if (f->Open(filename)){
		int ffv=f->ReadFileFormatVersion();
		NumResources=f->ReadIntC();
		for (int i=0;i<NumResources;i++){
			sHuiResource *res=&HuiResource[i];
			res->type=f->ReadIntC();
			res->id=f->ReadInt();
			res->i_param[0]=f->ReadInt();
			res->i_param[1]=f->ReadInt();
			res->i_param[2]=f->ReadInt();
			res->i_param[3]=f->ReadInt();
			res->i_param[4]=f->ReadInt();
			res->i_param[5]=f->ReadInt();
			res->i_param[6]=f->ReadInt();
			res->b_param[0]=f->ReadBool();
			res->b_param[1]=f->ReadBool();
			res->num_cmds=f->ReadInt();
			res->cmd=new sHuiResourceCommand[(res->num_cmds>64)?res->num_cmds:64];
			for (int j=0;j<res->num_cmds;j++){
				sHuiResourceCommand *cmd=&res->cmd[j];
				cmd->type=f->ReadInt();
				cmd->id=f->ReadInt();
				cmd->i_param[0]=f->ReadInt();
				cmd->i_param[1]=f->ReadInt();
				cmd->i_param[2]=f->ReadInt();
				cmd->i_param[3]=f->ReadInt();
				cmd->i_param[4]=f->ReadInt();
				cmd->i_param[5]=f->ReadInt();
				cmd->i_param[6]=f->ReadInt();
				cmd->b_param[0]=f->ReadBool();
				cmd->b_param[1]=f->ReadBool();
			}
		}
		f->Close();
	}
	delete(f);
}

char ReturnStr[2048];

char *ConvertReturns(char *str)
{
	int l=0;
	int sl=strlen(str);
	for (int i=0;i<sl;i++){
		ReturnStr[l++]=str[i];
		if ((str[i]=='\\')&&(str[i+1]=='n')){
			if (i>0)
				if (str[i-1]=='\\')
					continue;
			ReturnStr[l-1]='\n';
			i++;
		}
	}
	ReturnStr[l]=0;
	return ReturnStr;
}

void CHui::SetLanguageFile(char *filename)
{
	if (strlen(filename)<1){
		Languaged=false;
		return;
	}
	CFile *f=new CFile();
	f->SilentFileAccess=true;
	if (f->Open(filename)){
		f->ReadStrC();
		NumLanguageIDs=f->ReadIntC();
		f->ReadComment();
		for (int i=0;i<NumLanguageIDs;i++)
			strcpy(LanguageText[i],ConvertReturns(f->ReadStr()));
		f->Close();
		Languaged=true;
	}
	delete(f);

	UpdateAll();
}

char *CHui::GetLanguage(int id)
{
	if ((!Languaged)||(id<0)||(id>=NumLanguageIDs))
		return "";
	if (LanguageText[id][0]==0)
		return "???";
	return SysStr(LanguageText[id]);
}

char *GetLanguaged(int id,char *text,bool allow_keys)
{
	if ((!Languaged)||(id<0)||(id>=NumLanguageIDs))
		return SysStr(text);
	if (LanguageText[id][0]==0)
		return SysStr(text);
	if (allow_keys)
		for (int i=0;i<hui->NumKeyCodes;i++)
			if (id==hui->KeyCodeID[i])
				return string(SysStr(LanguageText[id]),"\t",hui->GetKeyCodeName(hui->KeyCode[i]));
	return SysStr(LanguageText[id]);
}

char *CHui::GetKeyName(int k)
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
	if (k==KEY_SZ)			return SysStr("&s");
	if (k==KEY_AE)			return SysStr("&A");
	if (k==KEY_OE)			return SysStr("&O");
	if (k==KEY_UE)			return SysStr("&U");
	if (k==KEY_WINDOWS_R)	return "WindowsR";
	if (k==KEY_WINDOWS_L)	return "WindowsL";
	return "";
}

int CHui::CreateTimer()
{
	if (NumTimers>=HUI_MAX_TIMERS){
		//msg_write>Write("too many timers");
		return -1;
	}
	GetTime(NumTimers); // Reset...
	NumTimers++;
	return NumTimers-1;
}

float CHui::GetTime(int index)
{
	/*if (index<0)
		return 0;*/
	float Elapsed=1;
	#ifdef HUI_OS_WINDOWS
		if (perf_flag)	QueryPerformanceCounter((LARGE_INTEGER *)&CurTime[index]);
		else			CurTime[index]=timeGetTime();
		Elapsed=(CurTime[index]-LastTime[index])*time_scale;
		LastTime[index]=CurTime[index];
	#endif
	#ifdef HUI_OS_LINUX
		gettimeofday(&CurTime[index],NULL);
		Elapsed=float(CurTime[index].tv_sec-LastTime[index].tv_sec)+float(CurTime[index].tv_usec-LastTime[index].tv_usec)*0.000001f;
		LastTime[index]=CurTime[index];
	#endif
	return Elapsed;
}

char KeyCodeName[128];
char *CHui::GetKeyCodeName(int k)
{
	if (k<0)
		strcpy(KeyCodeName,"");
	else{
		strcpy(KeyCodeName,"");
		if ((k&256)==256)
			strcat(KeyCodeName,"Ctrl+");
		if ((k&512)==512)
			strcat(KeyCodeName,"Shift+");
		strcat(KeyCodeName,hui->GetKeyName(k%256));
	}
	return KeyCodeName;
}


//########################################################################################
// menus
//########################################################################################



#ifdef HUI_OS_LINUX
	gboolean CallbackMenu(GtkWidget *widget,gpointer data);
#endif

CHuiMenu::CHuiMenu()
{
	NumItems=0;
#ifdef HUI_OS_WINDOWS
	hMenu=CreateMenu();
#endif
#ifdef HUI_OS_LINUX
	g_menu=gtk_menu_new ();
#endif
}

CHuiMenu::~CHuiMenu()
{
}

// window coordinate system!
void CHuiMenu::OpenPopup(CHuiWindow *win,int x,int y)
{
	msg_write("OpenPopup");
#ifdef HUI_OS_WINDOWS
	tagPOINT pt;
	pt.x=pt.y=0;
	ClientToScreen(win->hWnd,&pt);
	HMENU pm=CreateMenu();
	AppendMenu(pm,MF_STRING|MF_POPUP,(UINT)hMenu,"");
	TrackPopupMenu(hMenu,0,pt.x+x,pt.y+y,0,win->hWnd,NULL);
	//win->Popup=this;
#endif
#ifdef HUI_OS_LINUX
	msg_write("OpenPopup 2");
	gtk_widget_show(g_menu);
	gtk_menu_popup(GTK_MENU(g_menu),NULL,NULL,NULL,NULL,0,gtk_get_current_event_time());
	msg_write("OpenPopup 3");
#endif
	win->Popup=this;
}

// stupid function for HuiBui....
void CHuiMenu::SetID(int id)
{
}

void CHuiMenu::AddEntry(char *name,int id)
{
#ifdef HUI_OS_WINDOWS
	AppendMenu(hMenu,MF_STRING,id,GetLanguaged(id,name,true));
#endif
#ifdef HUI_OS_LINUX
	g_item[NumItems]=gtk_menu_item_new_with_label(GetLanguaged(id,name,true));
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu),g_item[NumItems]);
	gtk_widget_show(g_item[NumItems]);
	g_signal_connect(G_OBJECT(g_item[NumItems]),"activate",G_CALLBACK(CallbackMenu),(void*)id);

#endif
	SubMenu[NumItems]=NULL;
	strcpy(ItemName[NumItems],GetLanguaged(id,name,true));
	ItemID[NumItems]=id;
	ItemEnabled[NumItems]=true;
	ItemIsSeparator[NumItems]=false;
	ItemCheckable[NumItems]=false;
	ItemChecked[NumItems]=false;
	NumItems++;
}

const gchar *get_stock_id(int image)
{
	if (image==HuiImageOpen)	return GTK_STOCK_OPEN;
	if (image==HuiImageNew)		return GTK_STOCK_NEW;
	if (image==HuiImageSave)	return GTK_STOCK_SAVE;
	if (image==HuiImageSaveAs)	return GTK_STOCK_SAVE_AS;
	if (image==HuiImageQuit)	return GTK_STOCK_QUIT;

	if (image==HuiImageCopy)	return GTK_STOCK_COPY;
	if (image==HuiImagePaste)	return GTK_STOCK_PASTE;
	if (image==HuiImageCut)		return GTK_STOCK_CUT;
	if (image==HuiImageDelete)	return GTK_STOCK_DELETE;
	if (image==HuiImageClose)	return GTK_STOCK_CLOSE;
	if (image==HuiImageEdit)	return GTK_STOCK_EDIT;
	if (image==HuiImageFind)	return GTK_STOCK_FIND;

	if (image==HuiImageNo)		return GTK_STOCK_NO;
	if (image==HuiImageYes)		return GTK_STOCK_YES;
	if (image==HuiImageOk)		return GTK_STOCK_OK;
	if (image==HuiImageCancel)	return GTK_STOCK_CANCEL;
	if (image==HuiImageApply)	return GTK_STOCK_APPLY;

	if (image==HuiImageRedo)	return GTK_STOCK_REDO;
	if (image==HuiImageUndo)	return GTK_STOCK_UNDO;
	if (image==HuiImageRefresh)	return GTK_STOCK_REFRESH;
	if (image==HuiImagePreferences)	return GTK_STOCK_PREFERENCES;

	if (image==HuiImageClear)	return GTK_STOCK_CLEAR;
	if (image==HuiImageAdd)		return GTK_STOCK_ADD;
	if (image==HuiImageRemove)	return GTK_STOCK_REMOVE;
	if (image==HuiImageExecute)	return GTK_STOCK_EXECUTE;
	if (image==HuiImageStop)	return GTK_STOCK_STOP;

	if (image==HuiImageUp)		return GTK_STOCK_GO_UP;
	if (image==HuiImageDown)	return GTK_STOCK_GO_DOWN;
	if (image==HuiImageBack)	return GTK_STOCK_GO_BACK;
	if (image==HuiImageForward)	return GTK_STOCK_GO_FORWARD;

	if (image==HuiImageHelp)	return GTK_STOCK_HELP;
	if (image==HuiImageInfo)	return GTK_STOCK_INFO;
	if (image==HuiImagePrint)	return GTK_STOCK_PRINT;
	if (image==HuiImageFont)	return GTK_STOCK_SELECT_FONT;
	if (image==HuiImageSelectAll)	return "gtk-select-all";//GTK_STOCK_SELECT_ALL;

	if (image==HuiImageZoomIn)	return GTK_STOCK_ZOOM_IN;
	if (image==HuiImageZoomOut)	return GTK_STOCK_ZOOM_OUT;
	if (image==HuiImageFullscreen)	return GTK_STOCK_FULLSCREEN;
	return "";
}

GtkWidget *get_gtk_image(int image,bool large)
{
	if (image<1024)
		return gtk_image_new_from_stock(get_stock_id(image),large?GTK_ICON_SIZE_LARGE_TOOLBAR:GTK_ICON_SIZE_MENU);
	else{
		return gtk_image_new_from_file(image_file[image-1024]);
	}
}

void CHuiMenu::AddEntryImage(char *name,int image,int id)
{
#ifdef HUI_OS_WINDOWS
	AppendMenu(hMenu,MF_STRING,id,GetLanguaged(id,name,true));
#endif
#ifdef HUI_OS_LINUX
	/*g_item[NumItems]=gtk_image_menu_item_new();
	char str[256];
	strcpy(str,GetLanguaged(id,name,true));
	//if (strstr(
	GtkWidget *l=gtk_label_new("test");//str);
	gtk_widget_add_mnemonic_label(g_item[NumItems],l);*/
	g_item[NumItems]=gtk_image_menu_item_new_with_label(GetLanguaged(id,name,true));
	GtkWidget *im=get_gtk_image(image,false);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(g_item[NumItems]),im);
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu),g_item[NumItems]);
	gtk_widget_show(g_item[NumItems]);
	g_signal_connect(G_OBJECT(g_item[NumItems]),"activate",G_CALLBACK(CallbackMenu),(void*)id);

#endif
	SubMenu[NumItems]=NULL;
	strcpy(ItemName[NumItems],GetLanguaged(id,name,true));
	ItemID[NumItems]=id;
	ItemEnabled[NumItems]=true;
	ItemIsSeparator[NumItems]=false;
	ItemCheckable[NumItems]=false;
	ItemChecked[NumItems]=false;
	NumItems++;
}

void CHuiMenu::AddEntryCheckable(char *name,int id)
{
#ifdef HUI_OS_WINDOWS
	AppendMenu(hMenu,MF_STRING,id,GetLanguaged(id,name,true));
#endif
#ifdef HUI_OS_LINUX
	g_item[NumItems]=gtk_check_menu_item_new_with_label(GetLanguaged(id,name,true));
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu),g_item[NumItems]);
	gtk_widget_show(g_item[NumItems]);
	g_signal_connect(G_OBJECT(g_item[NumItems]),"activate",G_CALLBACK(CallbackMenu),(void*)id);

#endif
	SubMenu[NumItems]=NULL;
	strcpy(ItemName[NumItems],GetLanguaged(id,name,true));
	ItemID[NumItems]=id;
	ItemEnabled[NumItems]=true;
	ItemIsSeparator[NumItems]=false;
	ItemCheckable[NumItems]=true;
	ItemChecked[NumItems]=false;
	NumItems++;
}

void CHuiMenu::AddSeparator()
{
#ifdef HUI_OS_WINDOWS
	AppendMenu(hMenu,MF_SEPARATOR,0,"");
#endif
#ifdef HUI_OS_LINUX
	g_item[NumItems]=gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu),g_item[NumItems]);
	gtk_widget_show(g_item[NumItems]);
#endif
	SubMenu[NumItems]=NULL;
	ItemID[NumItems]=-1;
	ItemEnabled[NumItems]=true;
	ItemIsSeparator[NumItems]=true;
	NumItems++;
}

void CHuiMenu::AddSubMenu(char *name,int id,CHuiMenu *menu)
{
#ifdef HUI_OS_WINDOWS
	AppendMenu(hMenu,MF_STRING|MF_POPUP,(UINT)menu->hMenu,GetLanguaged(id,name));
#endif
#ifdef HUI_OS_LINUX
	g_item[NumItems]=gtk_menu_item_new_with_label(GetLanguaged(id,name));
	gtk_widget_show(g_item[NumItems]);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (g_item[NumItems]),menu->g_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu),g_item[NumItems]);
#endif
	SubMenu[NumItems]=menu;
	strcpy(ItemName[NumItems],GetLanguaged(id,name));
	ItemID[NumItems]=id;
	ItemEnabled[NumItems]=true;
	ItemIsSeparator[NumItems]=false;
	NumItems++;
}

// only allow menu callback, if we are in layer 0 (if we don't edit it ourself)
int allow_menu_signal_layer=0;

void CHuiMenu::CheckItem(int id,bool checked)
{
#ifdef HUI_OS_WINDOWS
	CheckMenuItem(hMenu,id,checked?MF_CHECKED:MF_UNCHECKED);
#endif
#ifdef HUI_OS_LINUX
	allow_menu_signal_layer++;
	for (int i=0;i<NumItems;i++){
		if (SubMenu[i])
			SubMenu[i]->CheckItem(id,checked);
		else if (ItemID[i]==id){
			if (ItemCheckable[i])
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_item[i]),checked);
		}
	}
	allow_menu_signal_layer--;
#endif
}

bool CHuiMenu::IsItemChecked(int id)
{
	//CheckMenuItem(hMenu,id,checked?MF_CHECKED:MF_UNCHECKED);
#ifdef HUI_OS_LINUX
	/*for (int i=0;i<NumItems;i++){
		if (SubMenu[i])
			SubMenu[i]->CheckItem(id,checked);
		else if (ItemID[i]==id){
			return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(g_item[i]));
		}
	}*/
#endif
	return false;
}

void CHuiMenu::EnableItem(int id,bool enabled)
{
	for (int i=0;i<NumItems;i++){
		if (SubMenu[i])
			SubMenu[i]->EnableItem(id,enabled);
		if (ItemID[i]==id){
			ItemEnabled[i]=enabled;
#ifdef HUI_OS_WINDOWS
			// would be recursive by itself,....but who cares.
			if (enabled)
				EnableMenuItem(hMenu,id,MF_ENABLED);
			else{
				EnableMenuItem(hMenu,id,MF_DISABLED);
				EnableMenuItem(hMenu,id,MF_GRAYED);
			}
#endif
#ifdef HUI_OS_LINUX
			gtk_widget_set_sensitive(g_item[i],enabled);
#endif
		}
	}
}

void CHuiMenu::SetText(int id,char *text)
{
	for (int i=0;i<NumItems;i++){
		if (SubMenu[i])
			SubMenu[i]->SetText(id,text);
		else if (ItemID[i]==id){
			strcpy(ItemName[i],text);
#ifdef HUI_OS_WINDOWS
			ModifyMenu(hMenu,i,MF_STRING | MF_BYPOSITION,id,SysStr(text));
#endif
#ifdef HUI_OS_LINUX
			msg_write("Todo:  CHuiMenu::SetText (Linux)");
#endif
		}
	}
}

CHuiMenu *HuiCreateMenu()
{
	return new CHuiMenu;
}


//########################################################################################
// windows
//########################################################################################

bool TestMenuID(CHuiMenu *menu,int id,message_function *mf)
{
	if (!menu)
		return false;
	for (int i=0;i<menu->NumItems;i++){
		if (menu->ItemID[i]==id){
			mf(id);
			return true;
		}
		if (menu->SubMenu[i])
			if (TestMenuID(menu->SubMenu[i],id,mf))
				return true;
	}
	return false;
}

#ifdef HUI_OS_WINDOWS
void UpdateTabPages(CHuiWindow *win)
{
	for (int i=0;i<win->NumControls;i++){
		int n_tab=-1;
		int cmd=SW_SHOW;
		// find the tab-control
	    if (win->Control[i].TabID>=0)
			for (int j=0;j<i;j++)
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
		ShowWindow(win->Control[i].hWnd,cmd);
		if (win->Control[i].Kind==HuiKindText)
			win->TextShow[win->Control[i].TextNr]=(cmd==SW_SHOW);
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
				for (i=0;i<hui->NumKeyCodes;i++)
					if (k==hui->KeyCode[i])
						mf(hui->KeyCodeID[i]);
		}
	}
}*/

static LRESULT CALLBACK WindowProcedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	//msg_write>Write("WP");
	int i,j;
#ifdef UseFakeTexts
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT hOldFont;
	RECT rt;
#endif

	CHuiWindow *win=NULL;
	message_function *mf=NULL;
	for (i=0;i<hui->NumWindows;i++){
		if (hui->Window[i]->hWnd==hwnd){
			win=hui->Window[i];
			mf=win->MessageFunction;
			break;
		}
	}
	if (win){
		//msg_write>Write(" w");
		win->CompleteWindowMessage.msg=message;
		win->CompleteWindowMessage.wparam=wParam;
		win->CompleteWindowMessage.lparam=lParam;

		if (GetActiveWindow()!=win->hWnd){
			win->AllowKeys=false;
			win->OwnData.KeyBufferDepth=0;
		}

		//msg_write>Write("-");
		//msg_write>Write((int)message);
		//msg_write>Write((int)(GetActiveWindow()==win->hWnd));


		if (win->UsedByNix)
			if (win->NixGetInputFromWindow)
				win->NixGetInputFromWindow();
		if (!win->Allowed){
			//msg_write>Write(string2("!win->Allowed %d",message));
			switch (message){
				case WM_PAINT:
					DefWindowProc(hwnd,message,wParam,lParam);
					mf(HUI_WIN_RENDER);
					break;
				case WM_ERASEBKGND:
					DefWindowProc(hwnd,message,wParam,lParam);
					mf(HUI_WIN_ERASEBKGND);
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
				default:
					return DefWindowProc(hwnd,message,wParam,lParam);
			}
			return 0;
		}

		//msg_write>Write(string2("win %d",message));
		switch (message){
			case WM_MOUSEMOVE:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
				win->mx=LOWORD(lParam);
				win->my=HIWORD(lParam);
				break;
			case WM_PAINT:
#ifdef UseFakeTexts
				if (win->NumTexts>0){
					InvalidateRect(win->hWnd,NULL,true);
					hdc=BeginPaint(win->hWnd,&ps);
					GetClientRect( win->hWnd, &rt );
					SetBkColor(hdc,GetSysColor(COLOR_3DFACE));
					SetTextColor(hdc,GetSysColor(COLOR_BTNTEXT));
					if (hOldFont = (HFONT)SelectObject(hdc, StandartFont))
						for (i=0;i<win->NumTexts;i++){
							if (win->TextShow[i])
								TextOut(hdc,win->TextX[i],win->TextY[i],win->TextStr[i],strlen(win->TextStr[i]));
						}
					SelectObject(hdc, hOldFont);
					EndPaint( win->hWnd, &ps );
				}
#endif
				break;
		}
	}
	//msg_write>Write(" a");

	/*if (hui->IdleFunction)
		hui->IdleFunction(0);*/

	if (win)
		if ((GetActiveWindow()==win->hWnd)&&(win->AllowKeys)){
			win->OwnDataOld=win->OwnData;
			for (i=0;i<256;i++)
				win->OwnData.key[KeyID[i]]=((GetAsyncKeyState(i)&(1<<15))!=0);
			int k=-1;
			if ((!win->GetKey(KEY_RALT))&&(!win->GetKey(KEY_LALT))){
				for (i=6;i<NUM_KEYS;i++)
					if (win->GetKeyDown(i)){
						k=i;
						if ((win->GetKey(KEY_RCONTROL))||(win->GetKey(KEY_LCONTROL)))	k+=256;
						if ((win->GetKey(KEY_RSHIFT))||(win->GetKey(KEY_LSHIFT)))		k+=512;
					}
				if (k>=0)
					for (i=0;i<hui->NumKeyCodes;i++)
						if (k==hui->KeyCode[i]){
							//msg_write>Write("---------------------------------");
							//msg_write>Write(hui->KeyCodeID[i]);
							mf(hui->KeyCodeID[i]);
						}
			}
		}

	if (mf){
		//msg_write>Write(string2("mf   %d",message));
		switch (message){
			case WM_COMMAND:
				for (j=0;j<win->NumControls;j++)
					if (win->Control[j].hWnd==(HWND)lParam){
						switch (HIWORD(wParam)){
							case CBN_SELCHANGE:
							case TCN_SELCHANGE:
							case BN_CLICKED:
							case EN_CHANGE:
								mf(win->Control[j].ID);
								return 0;
						}
								mf(win->Control[j].ID);
    				}
					//msg_write>Write(string2("hw wP = %d",(int)HIWORD(wParam)));
				if (HIWORD(wParam)==0){//WM_USER){
					if (TestMenuID(win->Menu,LOWORD(wParam),mf))
						return 0;
					if (TestMenuID(win->Popup,LOWORD(wParam),mf))
						return 0;
				}
				break;
			case WM_NOTIFY:
				for (j=0;j<win->NumControls;j++)
					if (((LPNMHDR)lParam)->hwndFrom==win->Control[j].hWnd){
						if ((win->Control[j].Kind==HuiKindListView)&&(((LPNMHDR)lParam)->code==NM_DBLCLK))
							mf(win->Control[j].ID);
						if ((win->Control[j].Kind==HuiKindTabControl)&&(((LPNMHDR)lParam)->code==TCN_SELCHANGE)){
							UpdateTabPages(win);
							mf(win->Control[j].ID);
							return 0;
						}
					}
				break;
			case WM_LBUTTONDOWN:
				mf(HUI_WIN_LBUTTONDOWN);
				break;
			case WM_LBUTTONUP:
				mf(HUI_WIN_LBUTTONUP);
				break;
			case WM_RBUTTONDOWN:
				mf(HUI_WIN_RBUTTONDOWN);
				break;
			case WM_RBUTTONUP:
				mf(HUI_WIN_RBUTTONUP);
				break;
			case WM_KEYDOWN:
				if (GetActiveWindow()==win->hWnd){
					win->AllowKeys=true;
					if (win->OwnData.KeyBufferDepth>=HUI_MAX_KEYBUFFER_DEPTH-1){
						for (i=0;i<win->OwnData.KeyBufferDepth-2;i++)
							win->OwnData.KeyBuffer[i]=win->OwnData.KeyBuffer[i+1];
						win->OwnData.KeyBufferDepth=HUI_MAX_KEYBUFFER_DEPTH-1;
					}
					win->OwnData.KeyBuffer[win->OwnData.KeyBufferDepth]=KeyID[wParam];
					win->OwnData.KeyBufferDepth++;
					mf(HUI_WIN_KEYDOWN);
				}
				break;
			case WM_KEYUP:
				mf(HUI_WIN_KEYUP);
				break;
			case WM_SIZE:
			case WM_SIZING:
				mf(HUI_WIN_SIZE);
				break;
			case WM_MOVE:
			case WM_MOVING:
				mf(HUI_WIN_MOVE);
				break;
			case WM_PAINT:
				//if (!win->UsedByNix)
					DefWindowProc(hwnd,message,wParam,lParam); // sonst funktionieren auch MessageBox'en nicht
				//mf(HUI_WIN_RENDER);
				break;
			case WM_ERASEBKGND:
				if (!win->UsedByNix)
					DefWindowProc(hwnd,message,wParam,lParam);
				mf(HUI_WIN_ERASEBKGND);
				break;
			//case WM_DESTROY:
			//	mf(HUI_WIN_DESTROY);
			//	break;
			case WM_CLOSE:
				mf(HUI_WIN_CLOSE);
				break;
			case WM_NCACTIVATE:
				win->AllowKeys=true;
			default:
				mf(HUI_WIN_EMPTY);
				return DefWindowProc(hwnd,message,wParam,lParam);
		}
	}else{
		//msg_write>Write(string2("!mf   %d",message));

		// default message_function (when none is applied)
		bool allow_exit=true;
		switch (message){
			case WM_NOTIFY:
				for (j=0;j<win->NumControls;j++)
					if (((LPNMHDR)lParam)->hwndFrom==win->Control[j].hWnd){
						if ((win->Control[j].Kind==HuiKindTabControl)&&(((LPNMHDR)lParam)->code==TCN_SELCHANGE)){
							UpdateTabPages(win);
							return 0;
						}
					}
				break;
			case WM_DESTROY:
				for (i=0;i<hui->NumWindows;i++)
					if (hui->Window[i]->MessageFunction)
						allow_exit=false;
				if (allow_exit)
					hui->End();
				break;
			default:
				return DefWindowProc(hwnd,message,wParam,lParam);
		}
	}
	//msg_write>Write(" x");

	/*	//if (GetActiveWindow()!=HuiWindow->hWnd)
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
		//	HuiWindow->OwnData.key[KEY_LCONTROL]=0;

    return 0;
}
#endif

#ifdef HUI_OS_LINUX

gboolean CallbackMenu(GtkWidget *widget,gpointer data)
{
	if (allow_menu_signal_layer>0)
		return FALSE;
	msg_db_out(1,"CallbackMenu");
	if ((int)data<0)
		return FALSE;
	CHuiWindow *win=NULL;
	message_function *mf=NULL;
	int i,j;
	for (int i=0;i<hui->NumWindows;i++){
		//if (gtk_window_has_toplevel_focus(GTK_WINDOW(hui->Window[i]->window))){
			//_so("top_level");
			win=hui->Window[i];
			if ((win->MessageFunction)&&(win->AllowInput))
				win->MessageFunction((int)data);
			return FALSE;
		//}
	}
	return FALSE;
}

gboolean CallbackWindow(GtkWidget *widget,gpointer data)
{
	msg_db_out(1,"CallbackWindow");
	//_so((int)data);
	//_so((char*)data);
	CHuiWindow *win=NULL;
	message_function *mf=NULL;
	int i,j;
	for (int i=0;i<hui->NumWindows;i++){
		if (hui->Window[i]->window==widget){
			win=hui->Window[i];
			if (win->AllowInput)
				mf=win->MessageFunction;
			break;
		}
	}
	if (mf){
		mf(HUI_WIN_CLOSE);//(int)data);
		return TRUE;
	}else{
		//if ((int)data==HUI_WIN_CLOSE)
			hui->End();
		return FALSE;
	}
}

void NotifyWindow(CHuiWindow *win,GtkWidget *control)
{
	int i,j,t;
	//msg_write>Write("NotifyWindow");
	for (i=0;i<hui->NumWindows;i++)
		if (hui->Window[i]==win){
			CHuiWindow *win=hui->Window[i];
			message_function *mf=win->MessageFunction;
			int id=-1;
			if ((mf)&&(win->AllowInput)){
				for (j=0;j<win->NumControls;j++)
					if (win->Control[j].win==control)
						id=win->Control[j].ID;
				for (t=0;t<4;t++)
					for (j=0;j<win->tool_bar[t].NumItems;j++)
						if ((GtkWidget*)win->tool_bar[t].Item[j].item==control)
							id=win->tool_bar[t].Item[j].ID;
				mf(id);
			}
		}
	//msg_write>Write("/NotifyWindow");
}

void CallbackControl(GtkWidget *widget,gpointer data)
{
	msg_db_out(1,"CallbackControl");
	NotifyWindow((CHuiWindow*)data,widget);
}

void CallbackControl2(GtkWidget *widget,void* a,gpointer data)
{
	msg_db_out(1,string2("CallbackControl2 %d",(int)a));
	NotifyWindow((CHuiWindow*)data,widget);
}

void CallbackControl3(GtkWidget *widget,void* a,void* b,gpointer data)
{
	msg_db_out(1,string2("CallbackControl3 %d %d",(int)a,(int)b));
	NotifyWindow((CHuiWindow*)data,widget);
}

void CallbackTabControl(GtkWidget *widget,GtkNotebookPage *page,guint page_num,gpointer data)
{
	msg_db_out(1,"CallbackTabControl");
	int i,j;
	for (i=0;i<hui->NumWindows;i++)
		if (hui->Window[i]==data){
			CHuiWindow *win=hui->Window[i];
			message_function *mf=win->MessageFunction;
			for (j=0;j<win->NumControls;j++)
				if (win->Control[j].win==widget)
					win->Control[j].Selected=page_num;
			if ((mf)&&(win->AllowInput))
				for (j=0;j<win->NumControls;j++)
					if (win->Control[j].win==widget){
						mf(win->Control[j].ID);
						break;
		    		}
		}
}

gint CallbackGDK(GtkWidget *widget,GdkEvent *event,gpointer data)
{
	msg_db_out(1,"CallbackGDK");
	_GdkEventKey *k=(_GdkEventKey*)event;
	int i,key=-1,kv=gdk_keyval_to_lower(k->keyval);
	for (i=0;i<HUI_NUM_KEYS;i++)
		//if ((KeyID[i]==k->keyval)||(KeyID2[i]==k->keyval))
		if (KeyID[i]==kv)
			key=i;
	if (key>=0){
		bool down=((int)data==HUI_WIN_KEYDOWN);
		for (i=0;i<hui->NumWindows;i++)
			if (hui->Window[i]->window==widget){
				CHuiWindow *win=hui->Window[i];
				if ((!down)||(!win->InputData.key[key])){
					msg_db_out(1,string2("%s:  %s",down?"down":"up",hui->GetKeyName(key)));
				}
				win->InputData.key[key]=down;
				message_function *mf=win->MessageFunction;
				if ((mf)&&(win->AllowInput)){
					mf(int(data));

					// key code?
					if (down){
						int key_code=-1;
						if ((!win->InputData.key[KEY_RALT])&&(!win->InputData.key[KEY_LALT])){
							key_code=key;
							if ((win->InputData.key[KEY_RCONTROL])||(win->InputData.key[KEY_LCONTROL]))	key_code+=256;
							if ((win->InputData.key[KEY_RSHIFT])||(win->InputData.key[KEY_LSHIFT]))		key_code+=512;
						}
						//msg_write>Write(key_code);
						if (key_code>=0)
							for (int kk=0;kk<hui->NumKeyCodes;kk++)
								if (key_code==hui->KeyCode[kk]){
									//msg_write>Write("---------------------------------");
									//msg_write>Write(hui->KeyCodeID[kk]);
									mf(hui->KeyCodeID[kk]);
								}
					}
				}
			}
	}else
		msg_db_out(1,string2("Unbekannte Taste: %d",k->keyval));
}

#endif

// allgemeines Fenster
CHuiWindow::CHuiWindow(char *title,int x,int y,int width,int height,CHuiWindow *root,bool allow_root,int bg_mode,bool show,message_function *mf)
{
	hui->Window[hui->NumWindows]=this;
	hui->WindowClosed[hui->NumWindows]=false;
	hui->NumWindows++;

	MessageFunction=mf;
	UsedByNix=false;
	NumControls=0;
	NumTexts=0;
	Allowed=true;
	AllowKeys=true;
	Root=root;
	TerrorChild=NULL;
	if (Root){
		Root->Allowed=allow_root;
		if (!Root->Allowed)
			Root->TerrorChild=this;
		Root->SubWindow[Root->NumSubWindows++]=this;
	}
	Menu=Popup=NULL;
	StatusBarEnabled;
	for (int i=0;i<4;i++){
		tool_bar[i].Enabled=false;
		tool_bar[i].TextEnabled=tool_bar[i].LargeIcons=true;
		tool_bar[i].NumItems=0;
	}
	tb=&tool_bar[HuiToolBarTop];
	NumSubWindows=0;
	InputData.KeyBufferDepth=0;
	memset(&InputData.key,0,sizeof(InputData.key));
	OwnData.KeyBufferDepth=0;
	memset(&OwnData.key,0,sizeof(OwnData.key));
	TabCreationID=TabCreationPage=-1;

	IsHidden=false;
	Menu=NULL;
	ID=-1;
	AllowInput=false; // allow only if ->Update() was called

#ifdef HUI_OS_WINDOWS
	NixGetInputFromWindow=NULL;
	char ClassName[128];
	sprintf(ClassName,"HuiWindowClass %d",hui->NumWindows);
	WNDCLASSEX wincl;
	wincl.hInstance=HuiInstance;
	wincl.lpszClassName=ClassName;
	wincl.lpfnWndProc=WindowProcedure;
	wincl.style=CS_DBLCLKS;
	wincl.cbSize=sizeof(WNDCLASSEX);

	wincl.hIcon=hui->main_icon;
	wincl.hIconSm=hui->main_icon;
	/*wincl.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wincl.hIconSm=LoadIcon(NULL,IDI_APPLICATION);*/

	wincl.hCursor=LoadCursor(NULL,IDC_ARROW);
	wincl.lpszMenuName=NULL;
	wincl.cbClsExtra=0;
	wincl.cbWndExtra=0;
	if (bg_mode==BGModeBlack)
		wincl.hbrBackground=CreateSolidBrush(RGB(0,0,0));
	if (bg_mode==BGModeStyleWindow)
		wincl.hbrBackground=GetSysColorBrush(COLOR_WINDOW);
	if (bg_mode==BGModeStyleDialog)
		wincl.hbrBackground=GetSysColorBrush(COLOR_3DFACE);

	if (!RegisterClassEx(&wincl))
		return;
		
	DWORD style_ex=0;
	if ((root)&&(bg_mode==BGModeStyleDialog))		style_ex=WS_EX_DLGMODALFRAME | WS_EX_NOPARENTNOTIFY;
	DWORD style=WS_OVERLAPPEDWINDOW;
	if ((root)&&(bg_mode==BGModeStyleDialog))		style=DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;


	if (bg_mode==BGModeStyleDialog){
		DEVMODE mode;
		EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&mode);
		x=(mode.dmPelsWidth-width)/2;
		y=(mode.dmPelsHeight-height)/2;
	}

	hWnd=CreateWindowEx(	style_ex,
							ClassName,
							SysStr(title),
							style,
							x<0?CW_USEDEFAULT:x,
							y<0?CW_USEDEFAULT:y,
							width<0?CW_USEDEFAULT:width,
							height<0?CW_USEDEFAULT:height,
							root?root->hWnd:HWND_DESKTOP,
							NULL,
							HuiInstance,
							NULL);
	if ((show)&&(!hui->CreateHiddenWindows))
		ShowWindow(hWnd,SW_SHOW);
#endif
#ifdef HUI_OS_LINUX

	// creation
	if (bg_mode==BGModeStyleDialog){
		window=gtk_dialog_new();
	//	gtk_window_set_modal(GTK_WINDOW(window),false);
		gtk_dialog_set_has_separator(GTK_DIALOG(window),false);
		gtk_container_set_border_width(GTK_CONTAINER(window),0);
	}else
		window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(window),SysStr(title));
	if (bg_mode==BGModeStyleDialog){
		// dialog -> center on screen or root (if given)
		/*if (Root){
			irect r=Root->GetOuterior();
			x = r.x1 + (r.x2-r.x1-w2l(width))/2;
			y = r.y1 + (r.y2-r.y1-w2l(height))/2;
		}else{
			GdkScreen *screen=gtk_window_get_screen(GTK_WINDOW(window));
			x=(gdk_screen_get_width(screen)-w2l(width))/2;
			y=(gdk_screen_get_height(screen)-w2l(height))/2;
		}
		//gtk_window_move(GTK_WINDOW(window),x,y);*/
		gtk_window_set_resizable(GTK_WINDOW(window),false);
		gtk_widget_set_size_request(window,w2l(width),w2l(height));
	}else{
		//gtk_window_move(GTK_WINDOW(window),w2l(x),w2l(y));
		//gtk_window_resize(GTK_WINDOW(window),w2l(width),w2l(height));
		gtk_window_move(GTK_WINDOW(window),x,y);
		gtk_window_resize(GTK_WINDOW(window),width,height);
	}

	if (Root)
		gtk_window_set_transient_for(GTK_WINDOW(window),GTK_WINDOW(Root->window));

	// only catch key events if no controls may want them (we're no dialog)
	if (bg_mode!=BGModeStyleDialog){
		g_signal_connect(G_OBJECT(window),"key_press_event",G_CALLBACK(CallbackGDK),(void*)HUI_WIN_KEYDOWN);
		g_signal_connect(G_OBJECT(window),"key_release_event",G_CALLBACK(CallbackGDK),(void*)HUI_WIN_KEYUP);
	}
	
	//g_signal_connect(G_OBJECT(window),"destroy",G_CALLBACK(CallbackWindow),NULL);
	g_signal_connect(G_OBJECT(window),"delete_event",G_CALLBACK(CallbackWindow),(void*)HUI_WIN_CLOSE);
	//g_signal_connect(G_OBJECT(window),"size-request",G_CALLBACK(CallbackWindow),(void*)HUI_WIN_SIZE);
	//g_signal_connect(G_OBJECT(window),"key-press-event",G_CALLBACK(CallbackWindow),(void*)HUI_WIN_CLOSE);
	//g_signal_connect(G_OBJECT(window),"button-press-event",G_CALLBACK(CallbackWindow),(void*)HUI_WIN_LBUTTONDOWN);
	/*HUI_WIN_SIZE,
	HUI_WIN_MOVE,
	HUI_WIN_RENDER,
	HUI_WIN_ERASEBKGND,
	HUI_WIN_LBUTTONDOWN,
	HUI_WIN_LBUTTONUP,
	HUI_WIN_RBUTTONDOWN,
	HUI_WIN_RBUTTONUP*/

	gtk_container_set_border_width(GTK_CONTAINER(window),0);
	if (bg_mode==BGModeStyleDialog)
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


	GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	//gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	gtk_box_pack_start (GTK_BOX (hbox), tool_bar[HuiToolBarLeft].tool_bar, FALSE, FALSE, 0);

	if (bg_mode==BGModeBlack){
		// "drawable" (for opengl)
		gl_widget=gtk_drawing_area_new();
		gtk_container_add(GTK_CONTAINER(hbox),gl_widget);
		gtk_widget_show(gl_widget);
		gtk_widget_realize(gl_widget);
		GdkRectangle r;
		r.x=0;
		r.y=0;
		r.width=500;
		r.height=500;
		gdk_window_invalidate_region(gl_widget->window,gdk_region_rectangle(&r),false);
		gdk_window_process_all_updates();
	}else{
		// "fixed" (for buttons etc)
		fixed=gtk_fixed_new();
		gtk_container_add(GTK_CONTAINER(hbox),fixed);
		gtk_widget_show(fixed);
		gl_widget=fixed;
		cur_cnt=fixed;
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
	/*if (bg_mode==BGModeBlack){
		GtkWidget *edit=gtk_entry_new();
		gtk_widget_set_size_request(edit,1,1);
		gtk_fixed_put(GTK_FIXED(fixed),edit,0,0);
		gtk_widget_show(edit);
	}*/
	//###########################################################################################
#endif
}

CHuiWindow::~CHuiWindow()
{
	int i,j;
#ifdef HUI_OS_WINDOWS
	int win;
	for (win=0;win<hui->NumWindows;win++)
		if (hui->Window[win]==this){
			hui->WindowClosed[win]=true;
			break;
		}
	if (Root){
		Root->Allowed=true;
		Root->TerrorChild=NULL;
		//Root->Activate();
		/*SetFocus(Root->hWnd);
		UpdateWindow(Root->hWnd);
		ShowWindow(Root->hWnd,SW_SHOW);*/
		//Root->Update();
		for (i=0;i<Root->NumSubWindows;i++)
			if (Root->SubWindow[i]==this){
				for (j=i;j<Root->NumSubWindows-1;j++)
					Root->SubWindow[j]=Root->SubWindow[j+1];
				Root->NumSubWindows--;
				break;
			}
	}
	for (i=0;i<NumControls;i++)
		if (Control[i].hWnd)
			DestroyWindow(Control[i].hWnd);
	DestroyWindow(hWnd);
#endif
#ifdef HUI_OS_LINUX
	gtk_widget_destroy(window);
#endif
	// unregister window
	for (i=0;i<hui->NumWindows;i++)
		if (hui->Window[i]==this){
			for (j=i;j<hui->NumWindows-1;j++)
				hui->Window[j]=hui->Window[j+1];
			hui->NumWindows--;
			break;
		}
}

void CHuiWindow::Update()
{
#ifdef HUI_OS_WINDOWS
	if (Menu)
		SetMenu(hWnd,Menu->hMenu);
	else
		SetMenu(hWnd,NULL);
	int i;
	for (i=0;i<NumControls;i++)
		if (Control[i].Kind==HuiKindTabControl)
			TabCtrl_SetCurSel(Control[i].hWnd,1);
	UpdateTabPages(this);
	for (i=0;i<NumControls;i++)
		if (Control[i].Kind==HuiKindTabControl)
			TabCtrl_SetCurSel(Control[i].hWnd,0);
	UpdateWindow(hWnd);
	UpdateTabPages(this);
	if (!IsHidden)
		ShowWindow(hWnd,SW_SHOW);


	UpdateTabPages(this);
#endif
#ifdef HUI_OS_LINUX
	int i;
	for (i=0;i<gtk_num_menus;i++){
		gtk_container_remove(GTK_CONTAINER(menu_bar),GTK_WIDGET(gtk_menu[i]));
	}
	gtk_num_menus=0;
	if (Menu){
		gtk_widget_show (menu_bar);
		gtk_num_menus=Menu->NumItems;
		for (i=0;i<Menu->NumItems;i++){
			gtk_menu[i] = gtk_menu_item_new_with_label (Menu->ItemName[i]);
			gtk_widget_show (gtk_menu[i]);
			if (Menu->SubMenu[i]){
		//gtk_menu_item_remove_submenu(GTK_MENU_ITEM(Menu->g_item[i]));
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (gtk_menu[i]), Menu->SubMenu[i]->g_menu);//Menu->g_item[i]);
				gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), gtk_menu[i]);
			}else{
				gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),gtk_menu[i]);
				g_signal_connect(G_OBJECT(gtk_menu[i]),"activate",G_CALLBACK(CallbackMenu),(void*)Menu->ItemID[i]);
			}
			gtk_widget_set_sensitive(gtk_menu[i],Menu->ItemEnabled[i]);
		}
	}else
		gtk_widget_hide(menu_bar);
	if (!IsHidden)
		gtk_widget_show(window);
#endif
	AllowInput=true;
}

void CHuiWindow::Hide(bool hide)
{
#ifdef HUI_OS_WINDOWS
	if (hide)
	    ShowWindow(hWnd,SW_HIDE);
	else
	    ShowWindow(hWnd,SW_SHOW);
#endif
#ifdef HUI_OS_LINUX
	if (hide)
		gtk_widget_hide(window);
	else
		gtk_widget_show(window);
#endif
	IsHidden=hide;
}

void CHuiWindow::SetTitle(char *title)
{
#ifdef HUI_OS_WINDOWS
	SetWindowText(hWnd,SysStr(title));
#endif
#ifdef HUI_OS_LINUX
	gtk_window_set_title(GTK_WINDOW(window),SysStr(title));
#endif
}

void CHuiWindow::SetID(int id)
{
	ID=id;
	if ((Languaged)&&(id>=0))
		SetTitle(GetLanguaged(id,""));
}

void CHuiWindow::SetPosition(int x,int y)
{
#ifdef HUI_OS_WINDOWS
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
#ifdef HUI_OS_LINUX
	gtk_window_move(GTK_WINDOW(window),x,y);
#endif
}

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

void CHuiWindow::SetOuterior(irect rect)
{
#ifdef HUI_OS_WINDOWS
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	// nicht maximiert!!
	lpwndpl.showCmd=SW_SHOW;
	lpwndpl.rcNormalPosition.left=rect.x1;
	lpwndpl.rcNormalPosition.top=rect.y1;
	lpwndpl.rcNormalPosition.right=rect.x2;
	lpwndpl.rcNormalPosition.bottom=rect.y2;
	SetWindowPlacement(hWnd,&lpwndpl);
#endif
#ifdef HUI_OS_LINUX
	gtk_window_move(GTK_WINDOW(window),rect.x1,rect.y1);
	gtk_window_resize(GTK_WINDOW(window),rect.x2-rect.x1,rect.y2-rect.y1);
#endif
}

irect CHuiWindow::GetOuterior()
{
	irect r;
#ifdef HUI_OS_WINDOWS
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	r.x1=lpwndpl.rcNormalPosition.left;
	r.y1=lpwndpl.rcNormalPosition.top;
	r.x2=lpwndpl.rcNormalPosition.right;
	r.y2=lpwndpl.rcNormalPosition.bottom;
#endif
#ifdef HUI_OS_LINUX
	gtk_window_get_position(GTK_WINDOW(window),&r.x1,&r.y1);
	gtk_window_get_size(GTK_WINDOW(window),&r.x2,&r.y2);
	r.x2+=r.x1;
	r.y2+=r.y1;
#endif
	return r;
}

int _tool_bar_size(sHuiToolBar *tool_bar)
{
	int s=28;
	if (tool_bar->LargeIcons)
		s+=8;
	if (tool_bar->TextEnabled)
		s+=16;
	return s;
}

irect CHuiWindow::GetInterior()
{
	irect r;
#ifdef HUI_OS_WINDOWS
	RECT WindowClient;
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
#endif
#ifdef HUI_OS_LINUX
	gtk_window_get_position(GTK_WINDOW(window),&r.x1,&r.y1);
	gtk_window_get_size(GTK_WINDOW(window),&r.x2,&r.y2);
//	gdk_window_get_geometry(fixed->window,&r.x1,&r.y1,&r.x2,&r.y2,NULL);
//	msg_write>Write(string2(" %d - %d - %d - %d",r.x1,r.y1,r.x2,r.y2));
	r.x2+=r.x1;
	r.y2+=r.y1;
/*	int x,y;
	gdk_window_get_root_origin(fixed->window,&x,&y);
	msg_write>Write(string2(" %d - %d",x,y));
	gdk_window_get_origin(fixed->window,&x,&y);
	msg_write>Write(string2(" %d - %d",x,y));
	gdk_window_get_position(fixed->window,&x,&y);
	msg_write>Write(string2(" %d - %d",x,y));
	gdk_window_get_deskrelative_origin(fixed->window,&x,&y);
	msg_write>Write(string2(" %d - %d",x,y));*/

	/*gtk_window_get_position(GTK_WINDOW(fixed),&r.x1,&r.y1);
	gtk_window_get_size(GTK_WINDOW(fixed),&r.x2,&r.y2);
	r.x2+=r.x1;
	r.y2+=r.y1;*/

	int w,h;
	if (Menu){
		gdk_window_get_size(menu_bar->window,&w,&h);
//		msg_write>Write(string2("MenuBar:  %d",h));
		r.y1+=h;
	}
	if (tool_bar[HuiToolBarTop].Enabled){
		gdk_window_get_size(tool_bar[HuiToolBarTop].tool_bar->window,&w,&h);
//		msg_write>Write(string2("ToolBar:  %d",h));
		// :--(   wrong size...I'm puzzled
	//	r.y1+=h;
		r.y1+=_tool_bar_size(&tool_bar[HuiToolBarTop]);
	}
	if (tool_bar[HuiToolBarBottom].Enabled)
		r.y2-=_tool_bar_size(&tool_bar[HuiToolBarBottom]);
	if (tool_bar[HuiToolBarLeft].Enabled)
		r.x1+=_tool_bar_size(&tool_bar[HuiToolBarLeft]);
	if (tool_bar[HuiToolBarRight].Enabled)
		r.x2-=_tool_bar_size(&tool_bar[HuiToolBarRight]);
	if (StatusBarEnabled){
		gdk_window_get_size(status_bar->window,&w,&h);
//		msg_write>Write(string2("StatusBar:  %d",h));
		// :--(   wrong size...I'm puzzled
		r.y2-=20;//h;
	}
#endif
	return r;
}

void CHuiWindow::ShowCursor(bool show)
{
#ifdef HUI_OS_WINDOWS
	int s=::ShowCursor(show);
	if (show){
		while(s<0)
			s=::ShowCursor(show);
	}else{
		while(s>=0)
			s=::ShowCursor(show);
	}
#endif
#ifdef HUI_OS_LINUX
	if (show)
		gdk_window_set_cursor(vbox->window,NULL);
	else
		gdk_window_set_cursor(vbox->window,invisible_cursor);
#endif
}

void CHuiWindow::SetCursorPos(int x,int y)
{
	irect ri=GetInterior();
	irect ro=GetOuterior();
#ifdef HUI_OS_WINDOWS
	::SetCursorPos(x+ri.x1,y+ri.y1);
#endif
#ifdef HUI_OS_LINUX
	Display *display=XOpenDisplay(0);
	XWarpPointer(display, None, GDK_WINDOW_XWINDOW(vbox->window), 0, 0, 0, 0, x-ro.x1+ri.x1,y-ro.y1+ri.y1);
	XFlush(display);
	XCloseDisplay(display);
#endif
	InputData.mx=(float)x;
	InputData.my=(float)y;
	InputData.vx=0;
	InputData.vy=0;
}

void CHuiWindow::SetMaximized(bool maximized)
{
#ifdef HUI_OS_WINDOWS
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	lpwndpl.showCmd=maximized?SW_SHOWMAXIMIZED:SW_SHOW;
	SetWindowPlacement(hWnd,&lpwndpl);
#endif
#ifdef HUI_OS_LINUX
	if (maximized)
		gtk_window_maximize(GTK_WINDOW(window));
	else
		gtk_window_unmaximize(GTK_WINDOW(window));
#endif
}

bool CHuiWindow::IsMaximized()
{
#ifdef HUI_OS_WINDOWS
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	return (lpwndpl.showCmd==SW_SHOWMAXIMIZED);
#endif
#ifdef HUI_OS_LINUX
	int state=gdk_window_get_state(window->window);
	return ((state & GDK_WINDOW_STATE_MAXIMIZED)>0);
#endif
}

bool CHuiWindow::IsMinimized()
{
#ifdef HUI_OS_WINDOWS
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(hWnd,&lpwndpl);
	return ((lpwndpl.showCmd==SW_SHOWMINIMIZED)||(lpwndpl.showCmd==SW_MINIMIZE));
#endif
#ifdef HUI_OS_LINUX
	int state=gdk_window_get_state(window->window);
	return ((state & GDK_WINDOW_STATE_ICONIFIED)>0);
#endif
}

void CHuiWindow::SetFullscreen(bool fullscreen)
{
#ifdef HUI_OS_WINDOWS
		// Fenster-Daten speichern
		WindowStyle=GetWindowLong(hWnd,GWL_STYLE);
		//hMenu=GetMenu(hWnd);
		GetWindowRect(hWnd,&WindowBounds);
		GetClientRect(hWnd,&WindowClient);
#endif
#ifdef HUI_OS_LINUX
	if (fullscreen)
		gtk_window_fullscreen(GTK_WINDOW(window));
	else
		gtk_window_unmaximize(GTK_WINDOW(window));
#endif
}

void CHuiWindow::EnableStatusBar(bool enabled)
{
#ifdef HUI_OS_LINUX
	if (enabled)
	    gtk_widget_show(status_bar);
	else
	    gtk_widget_hide(status_bar);
#endif
	StatusBarEnabled=enabled;
}

void CHuiWindow::SetStatusText(char *str)
{
#ifdef HUI_OS_LINUX
	gtk_statusbar_push(GTK_STATUSBAR(status_bar),0,SysStr(str));
#endif
}

void CHuiWindow::EnableToolBar(bool enabled)
{
#ifdef HUI_OS_LINUX
	if (enabled)
		gtk_widget_show(tb->tool_bar);
	else
		gtk_widget_hide(tb->tool_bar);
#endif
	tb->Enabled=enabled;
}

void CHuiWindow::ToolBarSetCurrent(int index)
{
	tb=&tool_bar[index];
}

void CHuiWindow::ToolBarConfigure(bool text_enabled,bool large_icons)
{
#ifdef HUI_OS_LINUX
	gtk_toolbar_set_style(GTK_TOOLBAR(tb->tool_bar),text_enabled?GTK_TOOLBAR_BOTH:GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(tb->tool_bar),large_icons?GTK_ICON_SIZE_LARGE_TOOLBAR:GTK_ICON_SIZE_SMALL_TOOLBAR);
#endif
	tb->TextEnabled=text_enabled;
	tb->LargeIcons=large_icons;
}

void CHuiWindow::ToolBarAddItem(char *title,char *tool_tip,int image,int id)
{
#ifdef HUI_OS_LINUX
	GtkWidget *im=get_gtk_image(image,true);
	gtk_widget_show(im);
	GtkToolItem *it=gtk_tool_button_new(im,SysStr(title));
	gtk_tool_item_set_tooltip(it,gtk_tooltips_new(),SysStr(tool_tip),SysStr(tool_tip));
	gtk_tool_item_set_homogeneous(it,true);
	gtk_widget_show(GTK_WIDGET(it));
	gtk_toolbar_insert(GTK_TOOLBAR(tb->tool_bar),it,-1);
	tb->Item[tb->NumItems].ID=id;
	tb->Item[tb->NumItems].Kind=HuiToolButton;
	tb->Item[tb->NumItems].Enabled=true;
	tb->Item[tb->NumItems].menu=NULL;
	tb->Item[tb->NumItems++].item=it;
	g_signal_connect(G_OBJECT(it),"clicked",G_CALLBACK(CallbackControl),this);
#endif
}

void CHuiWindow::ToolBarAddItemCheckable(char *title,char *tool_tip,int image,int id)
{
#ifdef HUI_OS_LINUX
	GtkWidget *im=get_gtk_image(image,true);
	gtk_widget_show(im);
	GtkToolItem *it=gtk_toggle_tool_button_new();
	gtk_tool_item_set_tooltip(it,gtk_tooltips_new(),SysStr(tool_tip),SysStr(tool_tip));
	gtk_tool_item_set_homogeneous(it,true);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(it),SysStr(title));
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(it),im);
	gtk_widget_show(GTK_WIDGET(it));
	gtk_toolbar_insert(GTK_TOOLBAR(tb->tool_bar),it,-1);
	tb->Item[tb->NumItems].ID=id;
	tb->Item[tb->NumItems].Kind=HuiToolCheckable;
	tb->Item[tb->NumItems].Enabled=true;
	tb->Item[tb->NumItems].menu=NULL;
	tb->Item[tb->NumItems++].item=it;
	g_signal_connect(G_OBJECT(it),"clicked",G_CALLBACK(CallbackControl),this);
#endif
}

void CHuiWindow::ToolBarAddItemMenu(char *title,char *tool_tip,int image,CHuiMenu *menu,int id)
{
	if (!menu)
		return;
#ifdef HUI_OS_LINUX
	GtkWidget *im=get_gtk_image(image,true);
	gtk_widget_show(im);
	GtkToolItem *it=gtk_menu_tool_button_new(im,SysStr(title));
	gtk_tool_item_set_tooltip(it,gtk_tooltips_new(),SysStr(tool_tip),SysStr(tool_tip));
	gtk_tool_item_set_homogeneous(it,true);
	gtk_widget_show(GTK_WIDGET(it));
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(it),menu->g_menu);
	gtk_toolbar_insert(GTK_TOOLBAR(tb->tool_bar),it,-1);
	tb->Item[tb->NumItems].ID=id;
	tb->Item[tb->NumItems].Kind=HuiToolCheckable;
	tb->Item[tb->NumItems].Enabled=true;
	tb->Item[tb->NumItems].menu=menu;
	tb->Item[tb->NumItems++].item=it;
	g_signal_connect(G_OBJECT(it),"clicked",G_CALLBACK(CallbackControl),this);
	//g_signal_connect(G_OBJECT(menu->g_menu),"activate",G_CALLBACK(CallbackMenu),(void*)Menu->ItemID[i]);
#endif
}

void CHuiWindow::ToolBarAddItemMenuByID(char *title,char *tool_tip,int image,int menu_id,int id)
{
	CHuiMenu *menu=HuiCreateResourceMenu(menu_id);
	ToolBarAddItemMenu(title,tool_tip,image,menu,id);
}

void CHuiWindow::ToolBarAddSeparator()
{
#ifdef HUI_OS_LINUX
	GtkToolItem *it=gtk_separator_tool_item_new();
	gtk_widget_show(GTK_WIDGET(it));
	gtk_toolbar_insert(GTK_TOOLBAR(tb->tool_bar),it,-1);
	tb->Item[tb->NumItems].ID=-1;
	tb->Item[tb->NumItems].Kind=HuiToolSeparator;
	tb->Item[tb->NumItems].Enabled=true;
	tb->Item[tb->NumItems].menu=NULL;
	tb->Item[tb->NumItems++].item=it;
#endif
}

void CHuiWindow::ToolBarReset()
{
#ifdef HUI_OS_LINUX
	for (int i=0;i<tb->NumItems;i++)
		gtk_widget_destroy(GTK_WIDGET(tb->Item[i].item));
	tb->NumItems=0;
#endif
}

void CHuiWindow::ToolBarSetByID(int id)
{
	msg_db_out(1,"ToolBarSetByID");
	msg_db_out(1,i2s(id));
	if (id<0)
		return;
	for (int r=0;r<NumResources;r++){
		sHuiResource *res=&HuiResource[r];
//		msg_write>Write(res->id);
		if (res->id==id){
			ToolBarReset();
			ToolBarConfigure(res->b_param[0],res->b_param[1]);
			for (int i=0;i<res->num_cmds;i++){
				sHuiResourceCommand *cmd=&res->cmd[i];
				if (cmd->type==HuiCmdMenuAddItem)
					ToolBarAddItem(GetLanguaged(cmd->id,"",false),"",cmd->i_param[1],cmd->id);
				else if (cmd->type==HuiCmdMenuAddItemCheckable)
					ToolBarAddItemCheckable(GetLanguaged(cmd->id,"",false),"",cmd->i_param[1],cmd->id);
				else if (cmd->type==HuiCmdMenuAddItemSeparator)
					ToolBarAddSeparator();
				else if (cmd->type==HuiCmdMenuAddItemPopup){
					char title[256];
					strcpy(title,GetLanguaged(cmd->id,"",false));
					ToolBarAddItemMenuByID(title,"",cmd->i_param[1],cmd->i_param[2],cmd->id);
				}
			}
			EnableToolBar(true);
			msg_db_out(1,":)");
			return;
		}
	}
	msg_error("ToolBarSetByID  :~~(");
	return;
}

void CHuiWindow::Activate(int control_id)
{
#ifdef HUI_OS_WINDOWS
	SetFocus(hWnd);
	WindowStyle=GetWindowLong(hWnd,GWL_STYLE);
	if (WindowStyle&WS_MINIMIZE>0)
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
#ifdef HUI_OS_LINUX
	gtk_widget_grab_focus(window);
	gtk_window_present(GTK_WINDOW(window));
	if (control_id>=0)
		for (int i=0;i<NumControls;i++)
			if (control_id==Control[i].ID)
				gtk_widget_grab_focus(Control[i].win);
#endif
}

bool CHuiWindow::IsActive(bool include_sub_windows)
{
	bool ia=false;
#ifdef HUI_OS_WINDOWS
	ia=(GetActiveWindow()==hWnd);
#endif
#ifdef HUI_OS_LINUX
	/*ghjghjghj
	// TODO!!!
	gtk_widget_grab_focus(window);
	gtk_window_present(GTK_WINDOW(window));*/
#endif
	if ((!ia)&&(include_sub_windows)){
		for (int i=0;i<NumSubWindows;i++)
			if (SubWindow[i]->IsActive(true))
				return true;
	}
	return ia;
}

bool CHuiWindow::GetKey(int k)
{	return OwnData.key[k];	}

bool CHuiWindow::GetKeyDown(int k)
{	return ((OwnData.key[k])&&(!OwnDataOld.key[k]));	}

bool CHuiWindow::GetKeyUp(int k)
{	return ((!OwnData.key[k])&&(OwnDataOld.key[k]));	}

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
		if (key==KEY_AE)		return SysStr("&A")[0];
		if (key==KEY_OE)		return SysStr("&O")[0];
		if (key==KEY_UE)		return SysStr("&U")[0];
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
	if (key==KEY_SZ)			return SysStr("&s")[0];
	if (key==KEY_AE)			return SysStr("&a")[0];
	if (key==KEY_OE)			return SysStr("&o")[0];
	if (key==KEY_UE)			return SysStr("&u")[0];
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
	if (HuiWindow->InputData.KeyBufferDepth>0){
		int k=HuiWindow->InputData.KeyBuffer[0];
		//msg_write>Write(GetKeyName(k));
		for (int i=0;i<HuiWindow->InputData.KeyBufferDepth-2;i++)
			HuiWindow->InputData.KeyBuffer[i]=HuiWindow->InputData.KeyBuffer[i+1];
		HuiWindow->InputData.KeyBufferDepth--;
		return k;
	}else
		return -1;*/
	for (int i=6;i<HUI_NUM_KEYS;i++)
		if (GetKeyDown(i))
			return i;
	return -1;
}

void AddControl(CHuiWindow *win,int id,int kind)
{
#ifdef HUI_OS_WINDOWS
	if (win->Control[win->NumControls].hWnd)
		SendMessage(win->Control[win->NumControls].hWnd,(UINT)WM_SETFONT,(WPARAM)StandartFont,(LPARAM)TRUE);
	/*if (win->NumControls==0)
		SetFocus(win->Control[win->NumControls]);*/
#endif
	win->Control[win->NumControls].ID=id;
	win->Control[win->NumControls].Kind=kind;
	win->Control[win->NumControls].Enabled=true;
	win->Control[win->NumControls].TabID=win->TabCreationID;
	win->Control[win->NumControls].TabPage=win->TabCreationPage;
	win->NumControls++;
}

void CHuiWindow::AddButton(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
	Control[NumControls].hWnd=CreateWindow(	"BUTTON",GetLanguaged(id,title),
											WS_VISIBLE | WS_CHILD | (hui->UseFlatButtons?BS_FLAT:0),
											x,y,width,height,
											hWnd,NULL,HuiInstance,NULL);
#endif
#ifdef HUI_OS_LINUX
	Control[NumControls].win=gtk_button_new_with_label(GetLanguaged(id,title));
	gtk_widget_set_size_request(Control[NumControls].win,w2l(width)+2,w2l(height)+2);
	gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x)-1,w2l(y)-1);
	gtk_widget_show(Control[NumControls].win);
	g_signal_connect(G_OBJECT(Control[NumControls].win),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,id,HuiKindButton);
}

void CHuiWindow::AddDefButton(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
	Control[NumControls].hWnd=CreateWindow(	"BUTTON",GetLanguaged(id,title),
											WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | (hui->UseFlatButtons?BS_FLAT:0),
											x,y,width,height,
											hWnd,NULL,HuiInstance,NULL);
#endif
#ifdef HUI_OS_LINUX
	Control[NumControls].win=gtk_button_new_with_label(GetLanguaged(id,title));
	gtk_widget_set_size_request(Control[NumControls].win,w2l(width)+2,w2l(height)+2);
	gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x)-1,w2l(y)-1);
	gtk_widget_show(Control[NumControls].win);
	g_signal_connect(G_OBJECT(Control[NumControls].win),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,id,HuiKindButton);
}

void CHuiWindow::AddCheckBox(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
	Control[NumControls].hWnd=CreateWindow(	"BUTTON",GetLanguaged(id,title),
											WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX| (hui->UseFlatButtons?BS_FLAT:0),
											x,y,width,height,
											hWnd,NULL,HuiInstance,NULL);
#endif
#ifdef HUI_OS_LINUX
	Control[NumControls].win=gtk_check_button_new_with_label(GetLanguaged(id,title));
	gtk_widget_set_size_request(Control[NumControls].win,w2l(width),w2l(height));
	gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x),w2l(y));
	gtk_widget_show(Control[NumControls].win);
	g_signal_connect(G_OBJECT(Control[NumControls].win),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,id,HuiKindCheckBox);
}

void CHuiWindow::AddText(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
#ifdef UseFakeTexts
	Control[NumControls].hWnd=NULL;
	Control[NumControls].TextNr=NumTexts;
	TextX[NumTexts]=x;
	TextY[NumTexts]=y;
	TextStr[NumTexts]=new char[GetLanguaged(id,title)+1];
	strcpy(TextStr[NumTexts],GetLanguaged(id,title));
	TextShow[NumTexts]=true;
	NumTexts++;
#else
	Control[NumControls].hWnd=CreateWindow(	"STATIC",GetLanguaged(id,title),
											WS_CHILD | WS_VISIBLE,
											x,y,width,height,
											hWnd,NULL,HuiInstance,NULL);
	Control[NumControls].TextNr=NumTexts;
#endif
#endif
#ifdef HUI_OS_LINUX
	Control[NumControls].win=gtk_label_new(GetLanguaged(id,title));
	gtk_widget_set_size_request(Control[NumControls].win,w2l(width),w2l(height)+8);
	gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x),w2l(y)-4);
	gtk_widget_show(Control[NumControls].win);
#endif
	AddControl(this,id,HuiKindText);
}

void CHuiWindow::AddEdit(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
	if (hui->UseFlatButtons){
		Control[NumControls].hWnd=CreateWindow(	"EDIT",SysStr(title),//GetLanguaged(id,title),
												WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | WS_BORDER | (hui->Multiline?(ES_MULTILINE | ES_AUTOVSCROLL):0),
												x,y,width,height,
												hWnd,NULL,HuiInstance,NULL);
	}else{
		Control[NumControls].hWnd=CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT",SysStr(title),//GetLanguaged(id,title),
												//WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
												WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | (hui->Multiline?(ES_MULTILINE | ES_AUTOVSCROLL):0),// | ES_PASSWORD | ES_NUMBER,
												x,y,width,height,
												hWnd,NULL,HuiInstance,NULL);
	}
#endif
#ifdef HUI_OS_LINUX
	if (hui->Multiline){
		GtkTextBuffer *tb=gtk_text_buffer_new(NULL);
		Control[NumControls].win=gtk_text_view_new_with_buffer(tb);
		GtkWidget *frame=gtk_frame_new(NULL);
		gtk_widget_set_size_request(frame,w2l(width),w2l(height));
		gtk_fixed_put(GTK_FIXED(cur_cnt),frame,w2l(x),w2l(y));
		gtk_widget_show(frame);
		gtk_container_add(GTK_CONTAINER(frame),Control[NumControls].win);
		gtk_widget_show(Control[NumControls].win);
	}else{
		Control[NumControls].win=gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(Control[NumControls].win),SysStr(title));//GetLanguaged(id,title));
		gtk_widget_set_size_request(Control[NumControls].win,w2l(width),w2l(height)-2);
		gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x),w2l(y)+1);
		gtk_widget_show(Control[NumControls].win);
		g_signal_connect(G_OBJECT(Control[NumControls].win),"changed",G_CALLBACK(CallbackControl),this);

		// dumb but usefull test
		if (height>30){
			GtkStyle* style=gtk_widget_get_style(Control[NumControls].win);
			PangoFontDescription *font_desc=pango_font_description_copy(style->font_desc);
			pango_font_description_set_size(font_desc,height*PANGO_SCALE*2/3);
			gtk_widget_modify_font(Control[NumControls].win,font_desc);
		}
	}
#endif
	AddControl(this,id,HuiKindEdit);
}

void CHuiWindow::AddGroup(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
	Control[NumControls].hWnd=CreateWindow(	"BUTTON",GetLanguaged(id,title),
											WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
											x,y,width,height,
											hWnd,NULL,HuiInstance,NULL);
#endif
#ifdef HUI_OS_LINUX
	Control[NumControls].win=gtk_frame_new(GetLanguaged(id,title));
	gtk_widget_set_size_request(Control[NumControls].win,w2l(width),w2l(height));
	gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x),w2l(y));
	gtk_widget_show(Control[NumControls].win);
#endif
	AddControl(this,id,HuiKindGroup);
}

static int NumPartStrings;
static char PartString[128][256];

void GetPartStrings(int id,char *title)
{
	char str[2048],*title2=GetLanguaged(id,title);
	int l=0;
	NumPartStrings=0;
	for (unsigned int i=0;i<strlen(title2);i++){
		str[l]=title2[i];
		str[l+1]=0;
		if ((title2[i]==hui->ComboBoxSeparator)||(i==strlen(title2)-1)){
			if (title2[i]==hui->ComboBoxSeparator)
				str[l]=0;
			l=-1;
			strcpy(PartString[NumPartStrings++],str);
		}
		l++;
	}
}

void CHuiWindow::AddComboBox(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
	Control[NumControls].hWnd=CreateWindow(	"COMBOBOX",GetLanguaged(id,title),
											WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | CBS_DISABLENOSCROLL,
											x,y,width,/*height*/500,
											hWnd,NULL,HuiInstance,NULL);
	SendMessage(Control[NumControls].hWnd,(UINT)CB_RESETCONTENT,(WPARAM)0,(LPARAM)0);
#endif
#ifdef HUI_OS_LINUX
	Control[NumControls].win=gtk_combo_box_new_text();
	//gtk_combo_box_append_text(GTK_COMBO_BOX(Control[NumControls]),title);

	gtk_widget_set_size_request(Control[NumControls].win,w2l(width)+2,w2l(height)+4);
	gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x)-1,w2l(y)-2);
	gtk_widget_show(Control[NumControls].win);
	g_signal_connect(G_OBJECT(Control[NumControls].win),"changed",G_CALLBACK(CallbackControl),this);
	Control[NumControls]._num_items_=0;
#endif
	AddControl(this,id,HuiKindComboBox);
	GetPartStrings(id,title);
	for (int i=0;i<NumPartStrings;i++)
		AddControlText(id,PartString[i]);
	SetControlSelection(id,0);
}

void CHuiWindow::AddTabControl(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
	if (hui->UseFlatButtons){
		Control[NumControls].hWnd=CreateWindow(	"SysTabControl32",GetLanguaged(id,title),
												WS_CHILD | WS_VISIBLE | TCS_BUTTONS | TCS_FLATBUTTONS,
												x,y,width,height,
												hWnd,NULL,HuiInstance,NULL);
		/*Control[NumControls].hWnd2=CreateWindow("BUTTON","",
												WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
												x,y+16,width,height-16,
												hWnd,NULL,hui->hInstance,NULL);*/
		Control[NumControls].hWnd2=CreateWindow("STATIC","",
												WS_CHILD | WS_VISIBLE | SS_SUNKEN,
												x,y+24,width,height-24,
												hWnd,NULL,HuiInstance,NULL);
	}else{
		Control[NumControls].hWnd=CreateWindow(	"SysTabControl32",GetLanguaged(id,title),
												WS_CHILD | WS_VISIBLE,
												x,y,width,height,
												hWnd,NULL,HuiInstance,NULL);
	}
	AddControl(this,id,HuiKindComboBox);
	GetPartStrings(id,title);
	for (int i=0;i<NumPartStrings;i++){
		TCITEM item;
		item.mask=TCIF_TEXT;
		item.pszText=const_cast<LPSTR>(PartString[i]);
		TabCtrl_InsertItem(Control[NumControls].hWnd,i,&item);
	}
#endif
#ifdef HUI_OS_LINUX
	Control[NumControls].win=gtk_notebook_new();

	gtk_widget_set_size_request(Control[NumControls].win,w2l(width),w2l(height));
	gtk_fixed_put(GTK_FIXED(fixed),Control[NumControls].win,w2l(x),w2l(y));
	gtk_widget_show(Control[NumControls].win);

	GetPartStrings(id,title);
	for (int i=0;i<NumPartStrings;i++){
		GtkWidget *_fixed=gtk_fixed_new();
		gtk_widget_show(_fixed);	
		GtkWidget *label = gtk_label_new (PartString[i]);
		gtk_notebook_append_page (GTK_NOTEBOOK(Control[NumControls].win), _fixed, label);
    }
	Control[NumControls].Selected=0;
	g_signal_connect(G_OBJECT(Control[NumControls].win),"switch-page",G_CALLBACK(CallbackTabControl),this);
#endif
	AddControl(this,id,HuiKindTabControl);
	//SetControlSelection(id,0);
}

void CHuiWindow::SetTabCreationPage(int id,int page)
{
	TabCreationID=id;
	TabCreationPage=page;
	cur_cnt=fixed;
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
			cur_cnt=gtk_notebook_get_nth_page(GTK_NOTEBOOK(Control[i].win),page);
		}
}

enum
{
   TITLE_COLUMN,
   AUTHOR_COLUMN,
   CHECKED_COLUMN,
   N_COLUMNS
};

void CHuiWindow::AddListView_Test(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
#endif
#ifdef HUI_OS_LINUX

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
	Control[NumControls].win=tree;


//Control[NumControls].win=gtk_frame_new(GetLanguaged(id,title));

	gtk_widget_set_size_request(Control[NumControls].win,w2l(width),w2l(height));
	gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x),w2l(y));
	gtk_widget_show(Control[NumControls].win);

#endif
	AddControl(this,id,HuiKindListView);
}

void CHuiWindow::AddListView(char *title,int x,int y,int width,int height,int id)
{
	GetPartStrings(id,title);
	int i;
	bool lv_tree=false,lv_icons=false,lv_extended=false;
	if (PartString[0][0]=='!'){
		lv_tree=strstr(PartString[0],"tree");
		lv_icons=strstr(PartString[0],"icons");
		lv_extended=strstr(PartString[0],"extended");
		for (i=0;i<NumPartStrings-1;i++)
			strcpy(PartString[i],PartString[i+1]);
		NumPartStrings--;
	}
#ifdef HUI_OS_WINDOWS
	DWORD style=WS_CHILD | WS_VISIBLE | LVS_REPORT | (hui->Multiline?LVS_SHOWSELALWAYS:LVS_SINGLESEL);
	if (hui->UseFlatButtons){
		Control[NumControls].hWnd=CreateWindow(	WC_LISTVIEW,"",
												style | WS_BORDER | BS_FLAT,
												x,y,width,height,
												hWnd,NULL,HuiInstance,NULL);
	}else{
		Control[NumControls].hWnd=CreateWindowEx(WS_EX_CLIENTEDGE,WC_LISTVIEW,"",
												style,
												x,y,width,height,
												hWnd,NULL,HuiInstance,NULL);
	}

	for (i=0;i<NumPartStrings;i++){
		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;
		lvc.iSubItem = num_cols;
		lvc.pszText = PartString[i];
		lvc.cx = 100;
		ListView_InsertColumn(Control[NumControls].hWnd,i,&lvc);
	}
	ListView_SetExtendedListViewStyleEx(Control[NumControls].hWnd,0,LVS_EX_FULLROWSELECT /*| LVS_EX_GRIDLINES*/ | LVS_EX_HEADERDRAGDROP | (hui->UseFlatButtons?LVS_EX_FLATSB:0));
	//ListView_SetExtendedListViewStyleEx(Control[NumControls].hWnd,0,LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP | (hui->UseFlatButtons?LVS_EX_FLATSB:0));
	for (i=0;i<NumPartStrings;i++)
		ListView_SetColumnWidth(Control[NumControls].hWnd,i,LVSCW_AUTOSIZE_USEHEADER);
#endif
#ifdef HUI_OS_LINUX

	GtkWidget *sw=gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

	Control[NumControls]._item_=new GtkTreeIter[4096];
	Control[NumControls]._num_items_=0;

	if (lv_icons){
		GtkListStore *model;
		model = gtk_list_store_new (2, G_TYPE_STRING,GDK_TYPE_PIXBUF);
		GtkWidget *iv=gtk_icon_view_new_with_model(GTK_TREE_MODEL(model));
		gtk_icon_view_set_text_column(GTK_ICON_VIEW(iv),0);
		gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(iv),1);
		gtk_icon_view_set_item_width(GTK_ICON_VIEW(iv),130);
		gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(iv),GTK_SELECTION_SINGLE);
		Control[NumControls].win=iv;
		// react on double click
		g_signal_connect(G_OBJECT(Control[NumControls].win),"item-activated",G_CALLBACK(CallbackControl2),this);
	}else if (lv_tree){
		GtkTreeStore *store;
		GtkWidget *tree;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		// "model"
		GType TypeList[64];
		for (i=0;i<NumPartStrings;i++)
			TypeList[i]=G_TYPE_STRING;
		store = gtk_tree_store_newv(NumPartStrings,TypeList);
		// "view"
		tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
		//g_object_set(G_OBJECT(tree),"border-width",51,NULL);
		g_object_unref(G_OBJECT(store));
		for (i=0;i<NumPartStrings;i++){
			renderer = gtk_cell_renderer_text_new ();
			column = gtk_tree_view_column_new_with_attributes(PartString[i],renderer,"text",i,NULL);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
		}
		Control[NumControls].win=tree;
		g_signal_connect(G_OBJECT(Control[NumControls].win),"row-activated",G_CALLBACK(CallbackControl3),this);
	}else{
		GtkListStore *store;
		GtkWidget *tree;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		// "model"
		GType TypeList[64];
		for (i=0;i<NumPartStrings;i++)
			TypeList[i]=G_TYPE_STRING;
		store = gtk_list_store_newv(NumPartStrings,TypeList);
		// "view"
		tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
		//g_object_set(G_OBJECT(list),"border-width",51,NULL);
		g_object_unref(G_OBJECT(store));
		for (i=0;i<NumPartStrings;i++){
			renderer = gtk_cell_renderer_text_new ();
			column = gtk_tree_view_column_new_with_attributes(PartString[i],renderer,"text",i,NULL);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tree),column);
		}
		Control[NumControls].win=tree;
		g_signal_connect(G_OBJECT(Control[NumControls].win),"row-activated",G_CALLBACK(CallbackControl3),this);
	}

	GtkWidget *frame=gtk_frame_new(NULL);
	gtk_widget_set_size_request(frame,w2l(width)-4,w2l(height)-4);
	gtk_fixed_put(GTK_FIXED(cur_cnt),frame,w2l(x),w2l(y));
	gtk_widget_show(frame);
	//gtk_container_add(GTK_CONTAINER(frame),Control[NumControls].win);
	gtk_container_add(GTK_CONTAINER(frame),sw);
	gtk_container_add(GTK_CONTAINER(sw),Control[NumControls].win);
	gtk_widget_show(sw);
	gtk_widget_show(Control[NumControls].win);
#endif
	AddControl(this,id,lv_icons?HuiKindListViewIcons:(lv_tree?HuiKindListViewTree:HuiKindListView));
}

void CHuiWindow::AddProgressBar(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
#endif
#ifdef HUI_OS_LINUX
	Control[NumControls].win=gtk_progress_bar_new();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(Control[NumControls].win),SysStr(title));
	gtk_widget_set_size_request(Control[NumControls].win,w2l(width),w2l(height));
	gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x),w2l(y));
	gtk_widget_show(Control[NumControls].win);
	//g_signal_connect(G_OBJECT(Control[NumControls].win),"clicked",G_CALLBACK(CallbackControl),this);
#endif
	AddControl(this,id,HuiKindProgressBar);
}

void CHuiWindow::AddImage(char *title,int x,int y,int width,int height,int id)
{
#ifdef HUI_OS_WINDOWS
#endif
#ifdef HUI_OS_LINUX
	Control[NumControls].win=gtk_image_new_from_file(SysFileName(title));
	gtk_widget_set_size_request(Control[NumControls].win,w2l(width)+2,w2l(height)+2);
	gtk_fixed_put(GTK_FIXED(cur_cnt),Control[NumControls].win,w2l(x)-1,w2l(y)-1);
	gtk_widget_show(Control[NumControls].win);
#endif
	AddControl(this,id,HuiKindImage);
}

// replace all the text
//    for all
void CHuiWindow::SetControlText(int id,char *str)
{
	char *str2=SysStr(str);
#ifdef HUI_OS_WINDOWS
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
			if ((Control[i].Kind==HuiKindEdit)||(Control[i].Kind==HuiKindCheckBox)||(Control[i].Kind==HuiKindButton)||(Control[i].Kind==HuiKindText)||(Control[i].Kind==HuiKindGroup))
				SendMessage(Control[i].hWnd,WM_SETTEXT,(WPARAM)0,(LPARAM)str2);
			else if (Control[i].Kind==HuiKindComboBox)
				SendMessage(Control[i].hWnd,CB_ADDSTRING,(WPARAM)0,(LPARAM)str2);
			else if (Control[i].Kind==HuiKindListView){
				int line=ListView_GetItemCount(Control[i].hWnd);
				GetPartStrings(-1,str);
				for (j=0;j<NumPartStrings;j++){
					if (num_cols==0){
						LVITEM lvI;
						lvI.iItem = line;
						lvI.iSubItem = 0;
						lvI.mask = LVIF_TEXT  | LVIF_PARAM | LVIF_STATE;
						lvI.state = 0;
						lvI.stateMask = 0;
						lvI.pszText=PartString[j];
						ListView_InsertItem(Control[i].hWnd, &lvI);
					}else
						ListView_SetItemText(Control[i].hWnd,line,num_cols,PartString[j]);
					num_cols++;
				}
				for (j=0;j<NumPartStrings;j++)
					ListView_SetColumnWidth(Control[i].hWnd,j,LVSCW_AUTOSIZE_USEHEADER);
			}
		}
#endif
#ifdef HUI_OS_LINUX
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
			if (Control[i].Kind==HuiKindEdit){
				if (GTK_CHECK_TYPE(Control[i].win,GTK_TYPE_TEXT_VIEW)){
					GtkTextBuffer *tb=gtk_text_view_get_buffer(GTK_TEXT_VIEW(Control[i].win));
					gtk_text_buffer_set_text(tb,str2,strlen(str2));
				}else
					gtk_entry_set_text(GTK_ENTRY(Control[i].win),str2);
			}else if (Control[i].Kind==HuiKindComboBox){
				gtk_combo_box_append_text(GTK_COMBO_BOX(Control[i].win),str2);
				Control[i]._num_items_++;
			}else if (Control[i].Kind==HuiKindProgressBar)
				gtk_progress_bar_set_text(GTK_PROGRESS_BAR(Control[i].win),str2);
		}
#endif
	if (ID==id)
		SetTitle(str);
}

// replace all the text with a numerical value (int)
//    for all
void CHuiWindow::SetControlInt(int id,int i)
{
	SetControlText(id,i2s(i));
}

// replace all the text with a float
//    for all
void CHuiWindow::SetControlFloat(int id,float f,int dec)
{
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
#ifdef HUI_OS_LINUX
			if (Control[i].Kind==HuiKindProgressBar)
				gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(Control[i].win),f);
			else
#endif
				SetControlText(id,f2s(f,dec));
			return;
		}
}

void CHuiWindow::SetControlImage(int id,int image)
{
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
#ifdef HUI_OS_LINUX
			if (Control[i].Kind==HuiKindButton){
				GtkWidget *im=get_gtk_image(image,false);
				gtk_button_set_image(GTK_BUTTON(Control[i].win),im);
			}
#endif
		}
}

// add a single line/string
//    for ComboBox, ListView, ListViewTree, ListViewIcons
void CHuiWindow::AddControlText(int id,char *str)
{
	char *str2=SysStr(str);
#ifdef HUI_OS_WINDOWS
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
			if ((Control[i].Kind==HuiKindEdit)||(Control[i].Kind==HuiKindCheckBox)||(Control[i].Kind==HuiKindButton)||(Control[i].Kind==HuiKindText)||(Control[i].Kind==HuiKindGroup))
				SendMessage(Control[i].hWnd,WM_SETTEXT,(WPARAM)0,(LPARAM)str2);
			else if (Control[i].Kind==HuiKindComboBox)
				SendMessage(Control[i].hWnd,CB_ADDSTRING,(WPARAM)0,(LPARAM)str2);
			else if (Control[i].Kind==HuiKindListView){
				int line=ListView_GetItemCount(Control[i].hWnd);
				char ttt[2048];
				int l=0,num_cols=0,j;
				for (j=0;j<(signed)strlen(str2);j++){
					ttt[l]=str2[j];
					ttt[l+1]=0;
					if ((str2[j]==hui->ComboBoxSeparator)||(j==(signed)strlen(str2)-1)){
						if (str2[j]==hui->ComboBoxSeparator)
							ttt[l]=0;
						l=-1;
						if (num_cols==0){
							LVITEM lvI;
   							lvI.iItem = line;
							lvI.iSubItem = 0;
							lvI.mask = LVIF_TEXT  | LVIF_PARAM | LVIF_STATE;
							lvI.state = 0;
							lvI.stateMask = 0;
							lvI.pszText=ttt;
							ListView_InsertItem(Control[i].hWnd, &lvI);
						}else
							ListView_SetItemText(Control[i].hWnd,line,num_cols,ttt);
						num_cols++;
					}
					l++;
				}
				for (j=0;j<num_cols;j++)
					ListView_SetColumnWidth(Control[i].hWnd,j,LVSCW_AUTOSIZE_USEHEADER);
			}
		}
#endif
#ifdef HUI_OS_LINUX
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
			if (Control[i].Kind==HuiKindEdit)
				{}//gtk_entry_set_text(GTK_ENTRY(Control[i].win),str2);
			else if (Control[i].Kind==HuiKindComboBox){
				gtk_combo_box_append_text(GTK_COMBO_BOX(Control[i].win),str2);
				Control[i]._num_items_++;
			}else if (Control[i].Kind==HuiKindListViewTree){
				GetPartStrings(-1,str);
				GtkTreeStore *store=GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				gtk_tree_store_append(store, &Control[i]._item_[Control[i]._num_items_],NULL);
				for (int j=0;j<NumPartStrings;j++)
					gtk_tree_store_set(store, &Control[i]._item_[Control[i]._num_items_], j,PartString[j],-1);
				Control[i]._num_items_++;
			}else if (Control[i].Kind==HuiKindListView){
				GetPartStrings(-1,str);
				GtkListStore *store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				gtk_list_store_append(store, &Control[i]._item_[Control[i]._num_items_]);
				for (int j=0;j<NumPartStrings;j++)
					gtk_list_store_set(store, &Control[i]._item_[Control[i]._num_items_], j,PartString[j],-1);
				Control[i]._num_items_++;
			}else if (Control[i].Kind==HuiKindListViewIcons){
				GetPartStrings(-1,str);
				GtkTreeModel *model=gtk_icon_view_get_model(GTK_ICON_VIEW(Control[i].win));
				GtkWidget *im=gtk_image_new_from_file(PartString[1]);
				gtk_list_store_append(GTK_LIST_STORE(model), &Control[i]._item_[Control[i]._num_items_]);
				gtk_list_store_set(GTK_LIST_STORE(model), &Control[i]._item_[Control[i]._num_items_], 0,PartString[0],1,gtk_image_get_pixbuf(GTK_IMAGE(im)),-1);
				Control[i]._num_items_++;
			}
		}
#endif
}

// add a single line as a child in the tree of a ListViewTree
//    for ListViewTree
void CHuiWindow::AddControlChildText(int id,int parent_row,char *str)
{
#ifdef HUI_OS_WINDOWS
#endif
#ifdef HUI_OS_LINUX
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindListViewTree){
				GetPartStrings(-1,str);
				GtkTreeStore *store=GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				gtk_tree_store_append(store, &Control[i]._item_[Control[i]._num_items_], &Control[i]._item_[parent_row]);
				for (int j=0;j<NumPartStrings;j++)
					gtk_tree_store_set(store, &Control[i]._item_[Control[i]._num_items_], j,PartString[j],-1);
				Control[i]._num_items_++;
			}
#endif
}

// change a single line in the tree of a ListViewTree
//    for ListViewTree
void CHuiWindow::ChangeControlText(int id,int row,char *str)
{
#ifdef HUI_OS_WINDOWS
#endif
#ifdef HUI_OS_LINUX
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
			if (Control[i].Kind==HuiKindListViewTree){
				GetPartStrings(-1,str);
				GtkTreeStore *store=GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Control[i].win)));
				for (int j=0;j<NumPartStrings;j++)
					gtk_tree_store_set(store, &Control[i]._item_[row], j,PartString[j],-1);
			}
		}
#endif
}

static char ControlText[2048];//,ControlLine[2048];

// retrieve the text
//    for edit
char *CHuiWindow::GetControlText(int id)
{
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindEdit){
#ifdef HUI_OS_WINDOWS
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
				SendMessage(Control[i].hWnd,(UINT)WM_GETTEXT,(WPARAM)2048,(LPARAM)ControlText);
				return DeSysStr(ControlText);
#endif
#ifdef HUI_OS_LINUX
				if (GTK_CHECK_TYPE(Control[i].win,GTK_TYPE_TEXT_VIEW)){
					GtkTextBuffer *tb=gtk_text_view_get_buffer(GTK_TEXT_VIEW(Control[i].win));
					GtkTextIter is,ie;
					gtk_text_buffer_get_iter_at_offset(tb,&is,0);
					gtk_text_buffer_get_iter_at_offset(tb,&ie,-1);
					strcpy(ControlText,gtk_text_buffer_get_text(tb,&is,&ie,false));
				}else
					strcpy(ControlText,gtk_entry_get_text(GTK_ENTRY(Control[i].win)));
				return DeSysStr(ControlText);
#endif
			}
	strcpy(ControlText,"");
	return ControlText;
}

// retrieve the text as a numerical value (int)
//    for edit
int CHuiWindow::GetControlInt(int id)
{
	return s2i(GetControlText(id));
}

// retrieve the text as a numerical value (float)
//    for edit
float CHuiWindow::GetControlFloat(int id)
{
	return s2f(GetControlText(id));
}

// switch control to usable/unusable
//    for all
void CHuiWindow::EnableControl(int id,bool enabled)
{
	if (id<0)
		return;
	int i,t;
	for (i=0;i<NumControls;i++)
		if (id==Control[i].ID){
		    Control[i].Enabled=enabled;
#ifdef HUI_OS_WINDOWS
			EnableWindow(Control[i].hWnd,enabled);
#endif
#ifdef HUI_OS_LINUX
			gtk_widget_set_sensitive(Control[i].win,enabled);
#endif
		}
	for (t=0;t<4;t++)
		for (i=0;i<tool_bar[t].NumItems;i++)
			if (id==tool_bar[t].Item[i].ID){
			    tool_bar[t].Item[i].Enabled=enabled;
#ifdef HUI_OS_WINDOWS
//				EnableWindow(ToolBarItem[i].hWnd,enabled);
#endif
#ifdef HUI_OS_LINUX
				gtk_widget_set_sensitive(GTK_WIDGET(tool_bar[t].Item[i].item),enabled);
#endif
			}
}

//    for all
bool CHuiWindow::IsControlEnabled(int id)
{
	int i,t;
	for (i=0;i<NumControls;i++)
		if (id==Control[i].ID)
			return Control[i].Enabled;
	for (t=0;t<4;t++)
		for (i=0;i<tool_bar[t].NumItems;i++)
			if (id==tool_bar[t].Item[i].ID)
				return tool_bar[t].Item[i].Enabled;
	return false;
}

// mark as "checked"
//    for CheckBox, ToolBarItemCheckable
void CHuiWindow::CheckControl(int id,bool checked)
{
	int i,t;
	for (i=0;i<NumControls;i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindCheckBox)
#ifdef HUI_OS_WINDOWS
				SendMessage(Control[i].hWnd,BM_SETCHECK,(WPARAM)(checked?BST_CHECKED:BST_UNCHECKED),(LPARAM)0);
	// BST_INDETERMINATE
#endif
#ifdef HUI_OS_LINUX
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Control[i].win),checked);
#endif
#ifdef HUI_OS_LINUX
	for (t=0;t<4;t++)
		for (i=0;i<tool_bar[t].NumItems;i++)
			if (id==tool_bar[t].Item[i].ID)
				if (tool_bar[t].Item[i].Kind==HuiToolCheckable)
					gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(tool_bar[t].Item[i].item),checked);
#endif
}

// is marked as "checked"?
//    for CheckBox
bool CHuiWindow::IsControlChecked(int id)
{
	int i,t;
#ifdef HUI_OS_WINDOWS
	for (i=0;i<NumControls;i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindCheckBox)
				return SendMessage(Control[i].hWnd,BM_GETCHECK,(WPARAM)0,(LPARAM)0)==BST_CHECKED;
#endif
#ifdef HUI_OS_LINUX
	for (i=0;i<NumControls;i++)
		if (id==Control[i].ID)
			if (Control[i].Kind==HuiKindCheckBox)
				return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Control[i].win));
#endif
#ifdef HUI_OS_LINUX
	for (t=0;t<4;t++)
		for (i=0;i<tool_bar[t].NumItems;i++)
			if (id==tool_bar[t].Item[i].ID)
				if (tool_bar[t].Item[i].Kind==HuiToolCheckable)
					return gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(tool_bar[t].Item[i].item));
#endif
	return false;
}

// which item/line is selected?
//    for ComboBox, TabControl, ListView
int CHuiWindow::GetControlSelection(int id)
{
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
#ifdef HUI_OS_WINDOWS
			if (Control[i].Kind==HuiKindTabControl)
				return TabCtrl_GetCurSel(Control[i].hWnd);
			else if (Control[i].Kind==HuiKindListView)
				return ListView_GetSelectionMark(Control[i].hWnd);
			else if (Control[i].Kind==HuiKindComboBox)
				return SendMessage(Control[i].hWnd,(UINT)/*CB_GETCOUNT*/CB_GETCURSEL,(WPARAM)0,(LPARAM)0);
#endif
#ifdef HUI_OS_LINUX
			if (Control[i].Kind==HuiKindTabControl)
				return Control[i].Selected;//gtk_notebook_get_current_page(GTK_NOTEBOOK(Control[i].win));
			else if (Control[i].Kind==HuiKindComboBox)
				return gtk_combo_box_get_active(GTK_COMBO_BOX(Control[i].win));
			else if ((Control[i].Kind==HuiKindListView)||(Control[i].Kind==HuiKindListViewTree)){
				GtkTreeSelection *sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(Control[i].win));
				for (int j=0;j<Control[i]._num_items_;j++)
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
						delete(indices);
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

// which lines are selected?
//    for ListView
int CHuiWindow::GetControlSelectionM(int id,int *indices)
{
	int num_marked=0;
#ifdef HUI_OS_WINDOWS
	for (int i=0;i<NumControls;i++)
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
#ifdef HUI_OS_LINUX
	indices[0]=GetControlSelection(id);
	num_marked=(indices[0]>=0)?1:0;
		//msg_write>Write("Todo:  CHuiWindow::GetControlSelectionM (Linux)");
#endif
	return num_marked;
}

// select an item
//    for ComboBox, TabControl, ListView?
void CHuiWindow::SetControlSelection(int id,int index)
{
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
#ifdef HUI_OS_WINDOWS
			if (Control[i].Kind==HuiKindTabControl){
				TabCtrl_SetCurSel(Control[i].hWnd,index);
				UpdateTabPages(this);
			}else if (Control[i].Kind==HuiKindListView)
				SendMessage(Control[i].hWnd,(UINT)LVM_SETSELECTIONMARK,(WPARAM)index,(LPARAM)0);
			else if (Control[i].Kind==HuiKindComboBox)
				SendMessage(Control[i].hWnd,(UINT)CB_SETCURSEL,(WPARAM)index,(LPARAM)0);
#endif
#ifdef HUI_OS_LINUX
			if (Control[i].Kind==HuiKindTabControl){
				gtk_notebook_set_current_page(GTK_NOTEBOOK(Control[i].win),index);
				Control[i].Selected=index;
			}else if (Control[i].Kind==HuiKindListView){
			}else if (Control[i].Kind==HuiKindComboBox){
				gtk_combo_box_set_active(GTK_COMBO_BOX(Control[i].win),index);
			}
#endif
		}
}

// delete all the content
//    for ComboBox, ListView
void CHuiWindow::ResetControl(int id)
{
	for (int i=0;i<NumControls;i++)
		if (id==Control[i].ID){
			Control[i]._num_items_=0;
#ifdef HUI_OS_WINDOWS
			if (Control[i].Kind==HuiKindListView)
				ListView_DeleteAllItems(Control[i].hWnd);
			else if (Control[i].Kind==HuiKindComboBox)
				SendMessage(Control[i].hWnd,CB_RESETCONTENT,(WPARAM)0,(LPARAM)0);
#endif
#ifdef HUI_OS_LINUX
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
				for (int j=Control[i]._num_items_-1;j>=0;j--)
					gtk_combo_box_remove_text(GTK_COMBO_BOX(Control[i].win),j);
				Control[i]._num_items_=0;
				//msg_write>Write("Todo:  CHuiWindow::ResetControl (ComboBox)  (Linux)");
			}
#endif
		}
}

void HuiWindowAddControl(CHuiWindow *win,int control_type,char *title,int x,int y,int width,int height,int id)
{
	msg_db_out(1,string2("HuiWindowAddControl %d  %s  %d  %d  %d  %d  %d",control_type,title,x,y,width,height,id));
	if (control_type==HuiKindButton)		win->AddButton(title,x,y,width,height,id);
	if (control_type==HuiKindDefButton)		win->AddDefButton(title,x,y,width,height,id);
	if (control_type==HuiKindText)			win->AddText(title,x,y,width,height,id);
	if (control_type==HuiKindEdit)			win->AddEdit(title,x,y,width,height,id);
	if (control_type==HuiKindGroup)			win->AddGroup(title,x,y,width,height,id);
	if (control_type==HuiKindCheckBox)		win->AddCheckBox(title,x,y,width,height,id);
	if (control_type==HuiKindComboBox)		win->AddComboBox(title,x,y,width,height,id);
	if (control_type==HuiKindTabControl)	win->AddTabControl(title,x,y,width,height,id);
	if (control_type==HuiKindListView)		win->AddListView(title,x,y,width,height,id);
	if (control_type==HuiKindProgressBar)	win->AddProgressBar(title,x,y,width,height,id);
	if (control_type==HuiKindImage)			win->AddImage(title,x,y,width,height,id);
}

CHuiWindow *HuiCreateWindow(char *title,int x,int y,int width,int height,message_function *mf)
{
	return new CHuiWindow(	title,x,y,width,height,NULL,true,
							BGModeStyleWindow,
							true,mf);
}

CHuiWindow *HuiCreateNixWindow(char *title,int x,int y,int width,int height,message_function *mf)
{
	return new CHuiWindow(	title,x,y,width,height,NULL,true,
							BGModeBlack,
							true,mf);
}

CHuiWindow *HuiCreateDialog(char *title,int width,int height,CHuiWindow *root,bool allow_root,message_function *mf)
{
	return new CHuiWindow(	title,-1,-1,width,height,root,allow_root,
							BGModeStyleDialog,
							true,mf);
}




CHuiWindow *HuiCreateResourceDialog(int id,CHuiWindow *root,message_function *mf)
{
	//return HuiCreateDialog("-dialog not found in resource-",200,100,root,true,mf);
	msg_db_out(1,"HuiCreateResourceDialog");
	msg_db_out(1,i2s(id));
	if (id<0)
		return NULL;
	for (int r=0;r<NumResources;r++){
		sHuiResource *res=&HuiResource[r];
		msg_db_out(1,i2s(res->id));
		if (res->id==id){
			CHuiWindow *dlg=HuiCreateDialog(GetLanguaged(res->id,"",true),res->i_param[2],res->i_param[3],root,res->b_param[0],mf);
			for (int j=0;j<res->num_cmds;j++){
				sHuiResourceCommand *cmd=&res->cmd[j];
				if ((cmd->type & 1023)==HuiCmdDialogAddControl){
					if (cmd->i_param[5]>=0){
						dlg->SetTabCreationPage( cmd->i_param[5]/1024, cmd->i_param[5]&1023 );
					}else
						dlg->SetTabCreationPage(-1,-1);
					HuiWindowAddControl(dlg,cmd->type>>10,GetLanguaged(cmd->id,"",true),
								cmd->i_param[0],cmd->i_param[1],
								cmd->i_param[2],cmd->i_param[3],
								cmd->id);
					if (!cmd->b_param[0])
						dlg->EnableControl(cmd->id,false);
					if (cmd->i_param[4]>=0)
						dlg->SetControlImage(cmd->id,cmd->i_param[4]);
				}
			}
			if (res->i_param[4]>=0)
				dlg->Menu=HuiCreateResourceMenu(res->i_param[4]);
			if (res->i_param[5]>=0)
				dlg->ToolBarSetByID(res->i_param[5]);
			msg_db_out(1,":)");
			return dlg;
		}
	}
	msg_error(string2("HuiCreateResourceDialog  (id=%d)  :~~(",id));
	return HuiCreateDialog(string2("-dialog (id=%d) not found in resource-",id),200,100,root,true,mf);
}

CHuiMenu *_create_res_menu_(sHuiResource *res,int c_first,int &c_last)
{
	msg_db_out(2,"--------");
	CHuiMenu *menu=new CHuiMenu();
	int n=res->cmd[c_first].i_param[0];
	msg_db_out(2,i2s(n));
	int j=c_first;
	for (int i=0;i<n;i++){
		msg_db_out(2,i2s(j));
		sHuiResourceCommand *cmd=&res->cmd[j];
		if (cmd->type==HuiCmdMenuAddItem)
			menu->AddEntry(GetLanguaged(cmd->id,"",true),cmd->id);
		if (cmd->type==HuiCmdMenuAddItemImage)
			menu->AddEntryImage(GetLanguaged(cmd->id,"",true),cmd->i_param[1],cmd->id);
		if (cmd->type==HuiCmdMenuAddItemCheckable)
			menu->AddEntryCheckable(GetLanguaged(cmd->id,"",true),cmd->id);
		if (cmd->type==HuiCmdMenuAddItemSeparator)
			menu->AddSeparator();
		if (cmd->type==HuiCmdMenuAddItemPopup){
			CHuiMenu *sub=_create_res_menu_(res,j+1,j);
			menu->AddSubMenu(GetLanguaged(cmd->id,"",true),cmd->id,sub);
		}
		if (!cmd->b_param[0])
			menu->EnableItem(cmd->id,false);
		j++;
	}
	c_last=j-1;
	msg_db_out(2,"/--------");
	return menu;
}

CHuiMenu *HuiCreateResourceMenu(int id)
{
	msg_db_out(1,"HuiCreateResourceMenu");
	msg_db_out(1,i2s(id));
	if (id<0)
		return NULL;
	for (int r=0;r<NumResources;r++){
		sHuiResource *res=&HuiResource[r];
//		msg_write>Write(res->id);
		if (res->id==id){
			int i;
			msg_db_out(1,":)");
			return _create_res_menu_(res,0,i);
		}
	}
	msg_error(string2("HuiCreateResourceMenu  (id=%d)  :~~(",id));
	return NULL;
}



int CurrentSystemStr=0;
char SystemString[8][2048];

// Umlaute aus Vokalen mit & davor zusammenbauen
char *SysStr(char *str)
{
	unsigned char *ss=(unsigned char *)SystemString[CurrentSystemStr];
	CurrentSystemStr++;
	if (CurrentSystemStr>=8)	CurrentSystemStr=0;

	int l=0;
	for (unsigned int i=0;i<strlen(str)+1;i++){
		//ss[l]=(unsigned char)str[i];
		ss[l]=str[i];
		if (str[i]=='&'){
#ifdef HUI_OS_WINDOWS
			// Windows-Zeichensatz
			if      (str[i+1]=='a'){	ss[l]=0xe4;	i++;	}
			else if (str[i+1]=='o'){	ss[l]=0xf6;	i++;	}
			else if (str[i+1]=='u'){	ss[l]=0xfc;	i++;	}
			else if (str[i+1]=='s'){	ss[l]=0xdf;	i++;	}
			else if (str[i+1]=='A'){	ss[l]=0xc4;	i++;	}
			else if (str[i+1]=='O'){	ss[l]=0xd6;	i++;	}
			else if (str[i+1]=='U'){	ss[l]=0xdc;	i++;	}
#endif
#ifdef HUI_OS_LINUX
			// Linux-Zeichensatz??? testen!!!!
			if      (str[i+1]=='a'){	ss[l]=0xc3;	ss[l+1]=0xa4;	i++;	l++;	}
			else if (str[i+1]=='o'){	ss[l]=0xc3;	ss[l+1]=0xb6;	i++;	l++;	}
			else if (str[i+1]=='u'){	ss[l]=0xc3;	ss[l+1]=0xbc;	i++;	l++;	}
			else if (str[i+1]=='s'){	ss[l]=0xc3;	ss[l+1]=0x9f;	i++;	l++;	}
			else if (str[i+1]=='A'){	ss[l]=0xc3;	ss[l+1]=0x84;	i++;	l++;	}
			else if (str[i+1]=='O'){	ss[l]=0xc3;	ss[l+1]=0x96;	i++;	l++;	}
			else if (str[i+1]=='U'){	ss[l]=0xc3;	ss[l+1]=0x9c;	i++;	l++;	}
#endif
		}
		if (str[i]=='\n'){
#ifdef HUI_OS_WINDOWS
			ss[l]='\r';		ss[l+1]='\n';	l++;
#endif
		}
		l++;
	}
	return (char *)ss;
}

// Umlaute zu Vokalen mit & davor zerlegen
char *DeSysStr(char *str)
{
	unsigned char *us=(unsigned char *)str;
	char *ss=SystemString[CurrentSystemStr];
	CurrentSystemStr++;
	if (CurrentSystemStr>=8)	CurrentSystemStr=0;

	int l=0;
	for (unsigned int i=0;i<strlen(str)+1;i++){
		ss[l]=str[i];
#ifdef HUI_OS_WINDOWS
		// Windows-Zeichensatz
		if (us[i]==0xe4){	ss[l]='&';	ss[l+1]='a';	l++;	}
		if (us[i]==0xf6){	ss[l]='&';	ss[l+1]='o';	l++;	}
		if (us[i]==0xfc){	ss[l]='&';	ss[l+1]='u';	l++;	}
		if (us[i]==0xdf){	ss[l]='&';	ss[l+1]='s';	l++;	}
		if (us[i]==0xc4){	ss[l]='&';	ss[l+1]='A';	l++;	}
		if (us[i]==0xd6){	ss[l]='&';	ss[l+1]='O';	l++;	}
		if (us[i]==0xdc){	ss[l]='&';	ss[l+1]='U';	l++;	}
		if (us[i]=='\r')	continue;
#endif
#ifdef HUI_OS_LINUX
			// Linux-Zeichensatz??? testen!!!!
		if ((us[i]==0xc3)&&(us[i+1]==0xa4)){	ss[l]='&';	ss[l+1]='a';	l++;	i++;	}
		if ((us[i]==0xc3)&&(us[i+1]==0xb6)){	ss[l]='&';	ss[l+1]='o';	l++;	i++;	}
		if ((us[i]==0xc3)&&(us[i+1]==0xbc)){	ss[l]='&';	ss[l+1]='u';	l++;	i++;	}
		if ((us[i]==0xc3)&&(us[i+1]==0x9f)){	ss[l]='&';	ss[l+1]='s';	l++;	i++;	}
		if ((us[i]==0xc3)&&(us[i+1]==0x84)){	ss[l]='&';	ss[l+1]='A';	l++;	i++;	}
		if ((us[i]==0xc3)&&(us[i+1]==0x96)){	ss[l]='&';	ss[l+1]='O';	l++;	i++;	}
		if ((us[i]==0xc3)&&(us[i+1]==0x9c)){	ss[l]='&';	ss[l+1]='U';	l++;	i++;	}
#endif
		l++;
	}
	return ss;
}


