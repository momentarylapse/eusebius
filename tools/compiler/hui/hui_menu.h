/*----------------------------------------------------------------------------*\
| Hui menu                                                                     |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2010.01.31 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _HUI_MENU_EXISTS_
#define _HUI_MENU_EXISTS_




class CHuiWindow;
class CHuiMenu;

struct sHuiMenuItem
{
	CHuiMenu *SubMenu;
	int ID, Image;
	char Name[64];
	bool Enabled, IsSeparator, Checked, Checkable;
#ifdef HUI_API_GTK
	GtkWidget* g_item;
#endif
};

class CHuiMenu
{
public:
	CHuiMenu();
	~CHuiMenu();
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
	CHuiMenu *GetSubMenuByID(int id);
	
#ifdef HUI_API_GTK
	void gtk_realize();
	void gtk_unrealize();
	GtkWidget* g_menu;
#endif

#ifdef HUI_API_WIN
	HMENU hMenu;
#endif
	std::vector<sHuiMenuItem> Item;
};

CHuiMenu *_cdecl HuiCreateMenu();

#endif
