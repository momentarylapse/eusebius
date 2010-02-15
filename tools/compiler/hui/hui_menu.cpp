/*----------------------------------------------------------------------------*\
| Hui menu                                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2010.01.31 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "hui.h"


#include "../file/file.h"
#include "../file/msg.h"
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
	gboolean CallbackMenu(GtkWidget *widget,gpointer data);
	GtkAccelGroup *accel_group = NULL;

	void try_add_accel(GtkWidget *item, int id)
	{
		for (int i=0;i<HuiKeyCode.size();i++)
			if ((id == HuiKeyCode[i].ID) && (HuiKeyCode[i].Code >= 0)){
				int k = HuiKeyCode[i].Code;
				int mod = (((k&512)>0) ? GDK_SHIFT_MASK : 0) | (((k&256)>0) ? GDK_CONTROL_MASK : 0);
				gtk_widget_add_accelerator(item, "activate", accel_group, HuiKeyID[k & 255], (GdkModifierType)mod, GTK_ACCEL_VISIBLE);
		}
	}
#endif

CHuiMenu::CHuiMenu()
{
	msg_db_r("CHuiMenu()", 1);

#ifdef HUI_API_WIN
	hMenu = CreateMenu();
#endif
#ifdef HUI_API_GTK
	//g_menu = NULL;
	g_menu = gtk_menu_new();
	if (accel_group == NULL)
		accel_group = gtk_accel_group_new();
#endif
	msg_db_l(1);
}

CHuiMenu::~CHuiMenu()
{
}

#ifdef HUI_API_GTK
void CHuiMenu::gtk_realize()
{
	g_menu = gtk_menu_new();
}
#endif

// window coordinate system!
void CHuiMenu::OpenPopup(CHuiWindow *win, int x, int y)
{
	msg_db_r("CHuiMenu::OpenPopup", 1);
#ifdef HUI_API_WIN
	tagPOINT pt;
	pt.x = pt.y = 0;
	ClientToScreen(win->hWnd, &pt);
	HMENU pm = CreateMenu();
	AppendMenu(pm, MF_STRING|MF_POPUP, (UINT)hMenu, _T(""));
	TrackPopupMenu(hMenu, 0, pt.x + x, pt.y + y, 0, win->hWnd, NULL);
	//win->Popup=this;
#endif
#ifdef HUI_API_GTK
	gtk_widget_show(g_menu);
	gtk_menu_popup(GTK_MENU(g_menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	win->Popup = this;
	msg_db_l(1);
}

// stupid function for HuiBui....
void CHuiMenu::SetID(int id)
{
}

void CHuiMenu::AddEntry(char *name, int id)
{
	sHuiMenuItem i;
#ifdef HUI_API_WIN
	AppendMenu(hMenu, MF_STRING, id, get_lang_sys(id, name, true));
#endif
#ifdef HUI_API_GTK
	i.g_item = gtk_menu_item_new_with_label(get_lang_sys(id,name,false));
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu), i.g_item);
	gtk_widget_show(i.g_item);
	g_signal_connect(G_OBJECT(i.g_item), "activate", G_CALLBACK(CallbackMenu), (void*)id);
	try_add_accel(i.g_item, id);
#endif
	i.SubMenu = NULL;
	strcpy(i.Name, get_lang(id, name, false));
	i.ID = id;
	i.Enabled = true;
	i.IsSeparator = false;
	i.Checkable = false;
	i.Checked = false;
	Item.push_back(i);
}


#ifdef HUI_API_WIN
int get_image_id(int image)
{
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

#ifdef HUI_API_GTK
const char *get_stock_id(int image)
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

void *get_gtk_image(int image,bool large)
{
	if (image<1024)
		return gtk_image_new_from_stock(get_stock_id(image),large?GTK_ICON_SIZE_LARGE_TOOLBAR:GTK_ICON_SIZE_MENU);
	else{
		if (hui_image_file[image-1024][0] == '/')
			return gtk_image_new_from_file(hui_image_file[image-1024].c_str());
		return gtk_image_new_from_file(string(HuiAppDirectory,hui_image_file[image-1024].c_str()));
	}
}
#endif

void CHuiMenu::AddEntryImage(char *name,int image,int id)
{
	sHuiMenuItem i;
#ifdef HUI_API_WIN
	AppendMenu(hMenu,MF_STRING,id,get_lang_sys(id,name,true));
#endif
#ifdef HUI_API_GTK
	/*g_item[NumItems]=gtk_image_menu_item_new();
	char str[256];
	strcpy(str,get_lang(id,name,true));
	//if (strstr(
	GtkWidget *l=gtk_label_new("test");//str);
	gtk_widget_add_mnemonic_label(g_item[NumItems],l);*/
	i.g_item=gtk_image_menu_item_new_with_label(get_lang_sys(id,name,false));
	GtkWidget *im=(GtkWidget*)get_gtk_image(image,false);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i.g_item),im);
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu),i.g_item);
	gtk_widget_show(i.g_item);
	g_signal_connect(G_OBJECT(i.g_item),"activate",G_CALLBACK(CallbackMenu),(void*)id);

	try_add_accel(i.g_item, id);
#endif
	i.SubMenu=NULL;
	strcpy(i.Name,get_lang(id,name,false));
	i.ID=id;
	i.Enabled=true;
	i.IsSeparator=false;
	i.Checkable=false;
	i.Checked=false;
	Item.push_back(i);
}

void CHuiMenu::AddEntryCheckable(char *name,int id)
{
	sHuiMenuItem i;
#ifdef HUI_API_WIN
	AppendMenu(hMenu,MF_STRING,id,get_lang_sys(id,name,true));
#endif
#ifdef HUI_API_GTK
	i.g_item=gtk_check_menu_item_new_with_label(get_lang_sys(id,name,false));
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu),i.g_item);
	gtk_widget_show(i.g_item);
	g_signal_connect(G_OBJECT(i.g_item),"activate",G_CALLBACK(CallbackMenu),(void*)id);

	try_add_accel(i.g_item, id);
#endif
	i.SubMenu=NULL;
	strcpy(i.Name,get_lang(id,name,false));
	i.ID=id;
	i.Enabled=true;
	i.IsSeparator=false;
	i.Checkable=true;
	i.Checked=false;
	Item.push_back(i);
}

void CHuiMenu::AddSeparator()
{
	sHuiMenuItem i;
#ifdef HUI_API_WIN
	AppendMenu(hMenu,MF_SEPARATOR,0,_T(""));
#endif
#ifdef HUI_API_GTK
	i.g_item=gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu),i.g_item);
	gtk_widget_show(i.g_item);
#endif
	i.SubMenu=NULL;
	i.ID=-1;
	i.Enabled=true;
	i.IsSeparator=true;
	Item.push_back(i);
}

void CHuiMenu::AddSubMenu(char *name,int id,CHuiMenu *menu)
{
	sHuiMenuItem i;
#ifdef HUI_API_WIN
	AppendMenu(hMenu,MF_STRING|MF_POPUP,(UINT)menu->hMenu,get_lang_sys(id,name));
#endif
#ifdef HUI_API_GTK
	i.g_item=gtk_menu_item_new_with_label(get_lang_sys(id,name));
	gtk_widget_show(i.g_item);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (i.g_item),menu->g_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(g_menu),i.g_item);
#endif
	i.SubMenu=menu;
	strcpy(i.Name,get_lang(id,name));
	i.ID=id;
	i.Enabled=true;
	i.IsSeparator=false;
	Item.push_back(i);
}

// only allow menu callback, if we are in layer 0 (if we don't edit it ourself)
int allow_signal_level=0;

void CHuiMenu::CheckItem(int id,bool checked)
{
#ifdef HUI_API_WIN
	CheckMenuItem(hMenu,id,checked?MF_CHECKED:MF_UNCHECKED);
#endif
#ifdef HUI_API_GTK
	allow_signal_level++;
	for (int i=0;i<Item.size();i++){
		if (Item[i].SubMenu)
			Item[i].SubMenu->CheckItem(id,checked);
		else if (Item[i].ID==id){
			if (Item[i].Checkable)
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(Item[i].g_item),checked);
		}
	}
	allow_signal_level--;
#endif
}

bool CHuiMenu::IsItemChecked(int id)
{
	//CheckMenuItem(hMenu,id,checked?MF_CHECKED:MF_UNCHECKED);
#ifdef HUI_API_GTK
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
	for (int i=0;i<Item.size();i++){
		if (Item[i].SubMenu)
			Item[i].SubMenu->EnableItem(id,enabled);
		if (Item[i].ID==id){
			Item[i].Enabled=enabled;
#ifdef HUI_API_WIN
			// would be recursive by itself,....but who cares.
			if (enabled)
				EnableMenuItem(hMenu,id,MF_ENABLED);
			else{
				EnableMenuItem(hMenu,id,MF_DISABLED);
				EnableMenuItem(hMenu,id,MF_GRAYED);
			}
#endif
#ifdef HUI_API_GTK
			gtk_widget_set_sensitive(Item[i].g_item,enabled);
#endif
		}
	}
}

void CHuiMenu::SetText(int id,char *text)
{
	for (int i=0;i<Item.size();i++){
		if (Item[i].SubMenu)
			Item[i].SubMenu->SetText(id,text);
		else if (Item[i].ID==id){
			strcpy(Item[i].Name,text);
#ifdef HUI_API_WIN
			ModifyMenu(hMenu,i,MF_STRING | MF_BYPOSITION,id,sys_str(text));
#endif
#ifdef HUI_API_GTK
			msg_todo("CHuiMenu::SetText (Linux)");
#endif
		}
	}
}


CHuiMenu *CHuiMenu::GetSubMenuByID(int id)
{
	for (int i=0;i<Item.size();i++)
		if (Item[i].SubMenu){
			if (Item[i].ID == id)
				return Item[i].SubMenu;
			CHuiMenu *m = Item[i].SubMenu->GetSubMenuByID(id);
			if (m)
				return m;
		}
	return NULL;
}

CHuiMenu *HuiCreateMenu()
{
	return new CHuiMenu();
}
