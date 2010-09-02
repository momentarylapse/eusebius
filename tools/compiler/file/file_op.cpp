#include "file.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

#define MAX_DIR_SEARCH_NAME		256 // compatibility with scripts....

#ifdef FILE_OS_WINDOWS
	/*
	#include <io.h>
	#include <direct.h>
	#include <stdarg.h>
	#include <windows.h>
	#include <winbase.h>
	#include <winnt.h>*/
#endif
#ifdef FILE_OS_LINUX
	#include <unistd.h>
	#include <dirent.h>
	//#include <sys/timeb.h>
 
	 
	#define _open	open
	#define _read	read
	#define _write	write
	#define _lseek	lseek
	#define _close	close
	#define _rmdir	rmdir
	#define _unlink	unlink
	inline unsigned char to_low(unsigned char c)
	{
		if ((c>='A')&&(c<='Z'))
			return c-'A'+'a';
		return c;
	}
	int _stricmp(const char*a,const char*b)
	{
		unsigned char a_low=to_low(*a);
		unsigned char b_low=to_low(*b);
		while((*a!=0)&&(*b!=0)){
			if (a_low!=b_low)	break;
			a++,++b;
			a_low=to_low(*a);
			b_low=to_low(*b);
		}
		return a_low-b_low;
	}
#endif



static CFile *test_file=NULL;

// just test the existence of a file
bool file_test_existence(const char *filename)
{
	if (!test_file)
		test_file=new CFile();
	test_file->SilentFileAccess=test_file->DontReportErrors=true;
	if (!test_file->Open(filename)){
		//delete(test_file);
		return false;
	}
	test_file->Close();
	return true;
}


bool dir_create(const char *dir)
{
#ifdef FILE_OS_WINDOWS
	return (_mkdir(SysFileName(dir))==0);
#endif
#ifdef FILE_OS_LINUX
	return (mkdir(SysFileName(dir),S_IRWXU | S_IRWXG | S_IRWXO)==0);
#endif
	return false;
}

bool dir_delete(const char *dir)
{
	return (_rmdir(SysFileName(dir))==0);
}

char *get_current_dir()
{
	char *str=_file_get_str_();
#ifdef FILE_OS_WINDOWS
	char *r=_getcwd(str,sizeof(_file_stack_str_[0]));
	strcat(str,"\\");
#endif
#ifdef FILE_OS_LINUX
	char *r=getcwd(str,sizeof(_file_stack_str_[0]));
	strcat(str,"/");
#endif
	return str;
}

bool file_rename(const char *source,const char *target)
{
	char dir[512];
	for (unsigned int i=0;i<strlen(target);i++){
		dir[i]=target[i];
		dir[i+1]=0;
		if (i>3)
			if ((target[i]=='/')||(target[i]=='\\'))
				dir_create(dir);
	}
	return (rename(source,target)==0);
}

bool file_copy(const char *source,const char *target)
{
	char dir[512];
	for (unsigned int i=0;i<strlen(target);i++){
		dir[i]=target[i];
		dir[i+1]=0;
		if (i>3)
			if ((target[i]=='/')||(target[i]=='\\'))
				dir_create(dir);
	}
	int hs=_open(SysFileName(source),O_RDONLY);
	if (hs<0)
		return false;
#ifdef FILE_OS_WINDOWS
	int ht=_creat(SysFileName(target),_S_IREAD | _S_IWRITE);
	_setmode(hs,_O_BINARY);
	_setmode(ht,_O_BINARY);
#endif
#ifdef FILE_OS_LINUX
	int ht=creat(SysFileName(target),S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
	if (ht<0){
		_close(hs);
		return false;
	}
	char buf[10240];
	int r=10;
	while(r>0){
		r=_read(hs,buf,sizeof(buf));
		int rr=_write(ht,buf,r);
	}
	_close(hs);
	_close(ht);
	return true;
}

bool file_delete(const char *filename)
{
	return (_unlink(SysFileName(filename))==0);
}


bool dir_search_inited = false;
CMichiArray DirSearchName, DirSearchNameP;
CMichiArray DirSearchIsDir;

inline void dir_search_add(char *name, bool is_dir)
{
	DirSearchIsDir.append_1_single(is_dir);
	DirSearchName.append_single(name);
}

// seach an directory for files matching a filter
int dir_search(const char *dir,const char *filter,bool show_directories)
{
	msg_db_r("dir_search", 1);
	if (!dir_search_inited){
		DirSearchName.init(sizeof(char) * MAX_DIR_SEARCH_NAME);
		DirSearchNameP.init(sizeof(char*));
		DirSearchIsDir.init(sizeof(bool));
		dir_search_inited = true;
	}
	DirSearchName.clear();
	DirSearchIsDir.clear();


	const char *filter2=filter+1;
	char dir2[256];
	if ((dir[strlen(dir)-1]!='/')&&(dir[strlen(dir)-1]!='\\')){
		strcpy(dir2,string(dir,"/"));
		dir=dir2;
	}

#ifdef FILE_OS_WINDOWS
	static _finddata_t t;
	int handle=_findfirst(string(SysFileName(dir),"*"),&t);
	int e=handle;
	while(e>=0){
		//if ((strcmp(t.name,".")!=0)&&(strcmp(t.name,"..")!=0)&&(strstr(t.name,"~")==NULL)){
		if ((t.name[0]!='.')&&(!strstr(t.name,"~"))){
			if ((strstr(t.name,filter2))|| ((show_directories)&&(t.attrib==_A_SUBDIR)) ){
				dir_search_add(t.name, (t.attrib == _A_SUBDIR));
			}
		}
		e=_findnext(handle,&t);
	}
#endif
#ifdef FILE_OS_LINUX
	DIR *_dir;
	_dir=opendir(SysFileName(dir));
	if (!_dir)
		return 0;
	struct dirent *dn;
	dn=readdir(_dir);
	struct stat s;
	while(dn){
		//if ((strcmp(dn->d_name,".")!=0)&&(strcmp(dn->d_name,"..")!=0)&&(!strstr(dn->d_name,"~"))){
			if ((dn->d_name[0]!='.')&&(!strstr(dn->d_name,"~"))){
			stat(string(SysFileName(dir),dn->d_name),&s);
			bool is_reg=(s.st_mode & S_IFREG)>0;
			bool is_dir=(s.st_mode & S_IFDIR)>0;
			int sss=strlen(dn->d_name)-strlen(filter2);
			if (sss<0)	sss=0;
			if ( ((is_reg)&&(strcmp(&dn->d_name[sss],filter2)==0)) || ((show_directories)&&(is_dir)) ){
				dir_search_add(dn->d_name, is_dir);
			}
		}
		dn=readdir(_dir);
	}
	closedir(_dir);
#endif

	// create pointers
	DirSearchNameP.clear();
	for (int i=0;i<DirSearchName.num;i++){
		char *pp = (char*)DirSearchName.data + i * MAX_DIR_SEARCH_NAME;
		DirSearchNameP.append_single(&pp);
	}
	
	// sorting...
	for (int i=0;i<DirSearchName.num-1;i++)
		for (int j=i+1;j<DirSearchName.num;j++){
			bool ok = true;
			if (dir_search_is_dir[i] == dir_search_is_dir[j]){
				if (_stricmp(dir_search_name[i], dir_search_name[j]) > 0)
					ok = false;
			}else
				if ((!dir_search_is_dir[i]) && (dir_search_is_dir[j]))
					ok = false;
			if (!ok){
				char temp[256];
				strcpy(temp,dir_search_name[i]);
				strcpy(dir_search_name[i],dir_search_name[j]);
				strcpy(dir_search_name[j],temp);
				bool temp2;
				temp2=dir_search_is_dir[i];
				dir_search_is_dir[i]=dir_search_is_dir[j];
				dir_search_is_dir[j]=temp2;
			}
		}

	msg_db_l(1);
	return DirSearchName.num;
}



