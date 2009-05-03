/*----------------------------------------------------------------------------*\
| CFile                                                                        |
| -> acces to files (high & low level)                                         |
| -> text mode / binary mode                                                   |
|    -> textmode: numbers as decimal numbers, 1 line per value saved,          |
|                 carriage-return/linefeed 2 characters (windows),...          |
|    -> binary mode: numbers as 4 byte binary coded, carriage-return 1         |
|                    character,...                                             |
| -> opening a missing file can call a callback function (x: used for          |
|    automatically downloading the file)                                       |
| -> files can be stored in an archive file                                    |
|                                                                              |
| vital properties:                                                            |
|  - a single instance per file                                                |
|                                                                              |
| last update: 2007.03.23 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#include "file.h"

#include "msg.h"



#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#ifdef FILE_OS_WINDOWS
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <stdarg.h>
#endif
#ifdef FILE_OS_LINUX
	#include <unistd.h>
	#include <dirent.h>
	#include <stdarg.h>
	#include <sys/timeb.h>
	#include <sys/stat.h>

	void ZeroMemory(void *p,int s)
	{
		int s4=(s+3)/4;
		for (int i=0;i<s4;i++)
			((int*)p)[i]=0;
	}
#endif



#ifdef FILE_OS_WINDOWS
sDate time2date(_SYSTEMTIME t)
{
	sDate d;
	d.time=(int)t.time;   :---(
	d.year=t.wYear;
	d.month=t.wMonth-1;
	d.day=t.wDay-1;
	d.hour=t.wHour;
	d.minute=t.wMinute;
	d.second=t.wSecond;
	d.milli_second=t.wMilliseconds;
	d.day_of_week=t.wDayofweek;   :---(
	d.day_of_year=t.wDayofyear;   :---(
	return d;
}
#endif
#ifdef FILE_OS_LINUX
sDate time2date(time_t t)
{
	sDate d;
	d.time=(int)t;
	tm *tm=localtime(&t);
	d.year=tm->tm_year+1900;
	d.month=tm->tm_mon;
	d.day=tm->tm_mday-1;
	d.hour=tm->tm_hour;
	d.minute=tm->tm_min;
	d.second=tm->tm_sec;
	d.milli_second=0;
	d.day_of_week=tm->tm_wday;
	d.day_of_year=tm->tm_yday;
	return d;
}
#endif

sDate get_current_date()
{
#ifdef FILE_OS_WINDOWS
	_SYSTEMTIME t;
	GetLocalTime(&t);
	return time2date(t);
#endif
#ifdef FILE_OS_LINUX
	time_t t;
	t=time(NULL);
	sDate d;
	d=time2date(t);
	timeb tb;
	ftime(&tb);
	d.milli_second=tb.millitm;
	return d;
#endif
}

//#define StructuredShifts
//#define FILE_COMMENTS_DEBUG



#define STR_STACK_DEPTH		64

static int CurrentStack=-1;
static char StackStr[STR_STACK_DEPTH][2048];

t_file_try_again_func *FileTryAgainFunc;

static int a_num_dirs=-1,a_num_files=-1;
struct s_a_dir{
	char name[256];
	int first_file,num_files;
}*a_dir;
struct s_a_file{
	char name[128];
	int size,offset,dir_no;
	bool decompressed;
}*a_file;
static CFile *a_f=NULL;

static int a_num_created_dirs=0;
static char a_created_dir[256][256];

// set and open the current archive
//   calls to CFile::Open will try to find a file inside this archive
void file_set_archive(char *filename)
{
	int i,j;
	if (a_f)
		file_clean_up_archive();
	a_f=new CFile();
	a_f->SilentFileAccess=true;
	if (!a_f->Open(filename)){
		delete(a_f);
		a_f=NULL;
		return;
	}
	a_f->SetBinaryMode(true);
	a_f->ReadInt(); // "michizip"
	a_f->ReadInt();
	int vers=a_f->ReadInt();
	int comp=a_f->ReadInt();
	if ((vers!=0)&&(comp!=1)){
		msg_error(string2("unsupported archive: version %d, compression %d   (I need v0, c1)",vers,comp));
		a_f->Close();
		delete(a_f);
		a_f=NULL;
		return;
	}
	a_num_dirs=a_f->ReadInt();
	a_dir=new s_a_dir[a_num_dirs];
	a_num_files=a_f->ReadInt();
	a_file=new s_a_file[a_num_files];
	for (i=0;i<a_num_dirs;i++){
		strcpy(a_dir[i].name,SysFileName(a_f->ReadStr()));
		a_dir[i].first_file=a_f->ReadInt();
		a_dir[i].num_files=a_f->ReadInt();
		for (j=a_dir[i].first_file;j<a_dir[i].first_file+a_dir[i].num_files;j++)
			a_file[j].dir_no=i;
	}
	for (i=0;i<a_num_files;i++){
		strcpy(a_file[i].name,a_f->ReadStr());
		a_file[i].size=a_f->ReadInt();
		a_file[i].decompressed=false;
	}
	int offset=a_f->GetPos();
	for (i=0;i<a_num_files;i++){
		a_file[i].offset=offset;
		offset+=a_file[i].size;
	}
	
//	a_f->Close();
//	delete(a_f);
}

struct sMzipTree{
	char data;
	int sub_tree[2],root;
};

static sMzipTree tree[768]; // 3*256 (using boring algorithm...)
static int num_trees;


static void read_tree(char *in_buffer,int &pos,int i)
{
	if (in_buffer[pos++]==0){
		tree[i].sub_tree[0]=num_trees;
		read_tree(in_buffer,pos,num_trees++);
		tree[i].sub_tree[1]=num_trees;
		read_tree(in_buffer,pos,num_trees++);
	}else{
		tree[i].sub_tree[0]=tree[i].sub_tree[1]=-1;
		tree[i].data=in_buffer[pos++];
	}
}

void add_created_dir(char *dir)
{
	for (int i=0;i<a_num_created_dirs;i++)
		if (strcmp(a_created_dir[i],SysFileName(dir))==0)
			return;
	strcpy(a_created_dir[a_num_created_dirs++],SysFileName(dir));
}

// try to find a file inside the archive and create a decompressed copy
//   only relative paths supported!!!
bool file_get_from_archive(char *filename)
{
	if (!a_f)
		return false;
	char dir[512],file[128];
	int i,j;
	// easify the name
	for (i=strlen(filename)-1;i>=0;i--)
		if ((filename[i]=='/')||(filename[i]=='\\')){
			strcpy(dir,SysFileName(filename));
			dir[i+1]=0;
			strcpy(file,&filename[i+1]);
			break;
		}
	msg_write(string("trying to find file in archive: ",filename));
	// find the file inside the archive
	int nf=-1;
	for (i=0;i<a_num_dirs;i++)
		if (strcmp(dir,a_dir[i].name)==0){
			for (j=a_dir[i].first_file;j<a_dir[i].first_file+a_dir[i].num_files;j++)
				if (strcmp(file,a_file[j].name)==0){
					nf=j;
					break;
				}
			break;
		}
	if (nf<0)
		return false;
	// read the raw data
	char *in_buffer=new char[a_file[nf].size];
	a_f->SetPos(a_file[nf].offset,true);
	a_f->ReadBuffer(in_buffer,a_file[nf].size);
	// decompress
	int version=((int*)&in_buffer[0])[0];
	int out_size=((int*)&in_buffer[0])[1];
	char *out_buffer=new char[out_size];
	num_trees=1;
	int pos=8;
	read_tree(in_buffer,pos,0);
	//trees_out();
	int offset=0;
	for (i=0;i<out_size;i++){
		int t=0;
		while(true){
			if (tree[t].sub_tree[0]<0){
				out_buffer[i]=tree[t].data;
				break;
			}
			// read current bit
			t=tree[t].sub_tree[(int)( (in_buffer[pos]&(1<<offset))>0 )];
			offset++;
			if (offset>=8){
				offset=0;
				pos++;
			}
		}
	}
	// test directories
	for (unsigned int n=3;n<strlen(dir);n++){
		if ((dir[n]=='/')||(dir[n]=='\\')){
			char temp[256];
			strcpy(temp,dir);
			temp[n]=0;
			if (dir_create(temp))
				add_created_dir(temp);
		}
	}
	// write the file
	CFile *f=new CFile();
	f->SilentFileAccess=true;
	f->SetBinaryMode(true);
	f->Create(filename);
	f->SetBinaryMode(true);
	f->WriteBuffer(out_buffer,out_size);
	f->Close();
	delete(f);
	delete(in_buffer);
	delete(out_buffer);
	a_file[nf].decompressed=true;
	return true;
}

// remove all decompressed files (and directories...)
void file_clean_up_archive()
{
	if (a_f){
		msg_write("cleaning file archive");
		a_f->Close();
		delete(a_f);
		a_f=NULL;

		int i;
		for (i=0;i<a_num_files;i++)
			if (a_file[i].decompressed)
				file_delete(string(a_dir[a_file[i].dir_no].name,a_file[i].name));
		for (i=a_num_created_dirs-1;i>=0;i--)
			dir_delete(a_created_dir[i]);

		a_num_files=0;
		a_num_dirs=0;
		a_num_created_dirs=0;
		delete(a_file);
		delete(a_dir);
	}
}


CFile::CFile()
{
	SilentFileAccess=false;
}

char FileTempName[8][1024];
int CurrentFileTempName=0;

// transposes path-strings to the current operating system
// accepts windows and linux paths ("/" and "\\")
char *SysFileName(char *filename)
{
	CurrentFileTempName++;
	if (CurrentFileTempName>=8)	CurrentFileTempName=0;
	char *str=FileTempName[CurrentFileTempName];
	strcpy(str,filename);
#ifdef FILE_OS_WINDOWS
	for (unsigned int i=0;i<strlen(str);i++)
		if (str[i]=='/')
			str[i]='\\';
#endif
#ifdef FILE_OS_LINUX
	for (int i=0;i<strlen(str);i++)
		if (str[i]=='\\')
			str[i]='/';
#endif
	return str;
}

// ends with '/' or '\'
char *dir_from_filename(char *filename)
{
	CurrentFileTempName++;
	if (CurrentFileTempName>=8)	CurrentFileTempName=0;
	char *str=FileTempName[CurrentFileTempName];
	strcpy(str,filename);
	for (int i=strlen(str)-1;i>=0;i--)
		if ((str[i]=='/')||(str[i]=='\\')){
			str[i+1]=0;
			break;
		}
	return str;
}

char *filename_no_recursion(char *filename)
{
	CurrentFileTempName++;
	if (CurrentFileTempName>=8)	CurrentFileTempName=0;
	char *str=FileTempName[CurrentFileTempName];
	strcpy(str,filename);
	int l1,l2,l3;
	for (l1=strlen(str)-2;l1>=0;l1--)
		if ((str[l1]=='.')&&(str[l1+1]=='.')){
			for (l2=l1-2;l2>=0;l2--)
				if ((str[l2]=='/')||(str[l2]=='\\')){
					int ss=strlen(str)+l2-l1-2;
					for (l3=l2;l3<ss;l3++)
						str[l3]=str[l3+l1-l2+2];
					str[ss]=0;
					l1=l2;
					break;
				}
		}
	return str;
}

// open a file
bool CFile::Open(char *filename)
{
	if (!SilentFileAccess){
		msg_write(string("loading file: ",SysFileName(filename)));
		msg_right();
	}
	Error=Eof=false;
	handle=open(SysFileName(filename),O_RDONLY);
	SetBinaryMode(false);
	if (handle<=0){
		if (file_get_from_archive(filename)){
			handle=open(SysFileName(filename),O_RDONLY);
			SetBinaryMode(false);
			return true;
		}else if (FileTryAgainFunc){
			if (FileTryAgainFunc(filename)){
				handle=open(SysFileName(filename),O_RDONLY);
				SetBinaryMode(false);
				return true;
			}
		}
		Error=true;
		if (SilentFileAccess)
			msg_error(string("while opening the file: ",filename));
		else
			msg_error("while opening the file");
		msg_left();
	}
	return !Error;
}

// create a new file or reset an existing one
void CFile::Create(char *filename)
{
	if (!SilentFileAccess){
		msg_write(string("creating file: ",SysFileName(filename)));
		msg_right();
	}
	Error=false;
	SetBinaryMode(false);
	FloatDecimals=3;
#ifdef FILE_OS_WINDOWS
	handle=creat(SysFileName(filename),_S_IREAD | _S_IWRITE);
#endif
#ifdef FILE_OS_LINUX
	handle=creat(SysFileName(filename),S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
	if (handle<=0){
		Error=true;
		if (SilentFileAccess)
			msg_error(string("while creating the file: ",filename));
		else
			msg_error("while creating the file");
	}
}

// create a new file or append data to an existing one
void CFile::Append(char *filename)
{
	if (!SilentFileAccess){
		msg_write(string("appending file: ",SysFileName(filename)));
		msg_right();
	}
	Error=false;
	SetBinaryMode(false);
	FloatDecimals=3;
#ifdef FILE_OS_WINDOWS
	handle=open(SysFileName(filename),O_WRONLY | O_APPEND | O_CREAT,_S_IREAD | _S_IWRITE);
#endif
#ifdef FILE_OS_LINUX
	handle=open(SysFileName(filename),O_WRONLY | O_APPEND | O_CREAT,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
	if (handle<=0){
		Error=true;
		if (SilentFileAccess)
			msg_error(string("while appending file: ",filename));
		else
			msg_error("while appending file");
	}
}

// close the file
bool _cdecl CFile::Close()
{
	//flush(handle);
	close(handle);
	if (SilentFileAccess){
		/*if (Error)
			msg_error("during file access");*/
	}else{
		if (Error)
			msg_error("during file access");
		else
			msg_ok();
		msg_left();
	}
	return Error;
}

// switch between text mode and binary mode
void CFile::SetBinaryMode(bool binary)
{
	Binary=binary;
#ifdef FILE_OS_WINDOWS
	if (binary)
		_setmode(handle,_O_BINARY);
	else
		_setmode(handle,_O_TEXT);
#endif
	// ignored by linux
	// (no fake return-characters!!!!!!)
}

// jump to an position in the file or to a position relative to the current
void CFile::SetPos(int pos,bool absolute)
{
	if (absolute)	lseek(handle,pos,SEEK_SET);
	else			lseek(handle,pos,SEEK_CUR);
}

// retrieve the size of the opened(!) file
int CFile::GetSize()
{
#ifdef FILE_OS_WINDOWS
	return (int)_filelength(handle);
#endif
#ifdef FILE_OS_LINUX
	struct stat stat;
	fstat(handle, &stat);
	return stat.st_size;
#endif
}

sDate CFile::GetDate(int type)
{
	time_t t;
	struct stat stat;
	fstat(handle, &stat);
	if (type==FileDateAccess)		t=stat.st_atime;
	if (type==FileDateModification)	t=stat.st_mtime;
	if (type==FileDateCreation)		t=stat.st_ctime;
	return time2date(t);
}

// where is the current reading position in the file?
int CFile::GetPos()
{
	return lseek(handle,0,SEEK_CUR);
}

// read a single character followed by the file-format-version-number
int CFile::ReadFileFormatVersion()
{
	if (Error)	return -1;
	unsigned char a=0;
	if (read(handle,&a,1)<=0){
		Eof=true;
		return -1;
	}
	if (a=='t')
		SetBinaryMode(false);
	else if (a=='b')
		SetBinaryMode(true);
	else{
		msg_error("File Format Version must begin ether with 't' or 'b'!!!");
		Error=true;
		return -1;
	}
	return ReadWord();
}

// write a single character followed by the file-format-version-number
void CFile::WriteFileFormatVersion(bool binary,int fvv)
{
	char a=binary?'b':'t';
	write(handle,&a,1);
	SetBinaryMode(binary);
	WriteWord(fvv);
}

#define chunk_size		2048
char chunk[chunk_size];

// read the complete file into the buffer
void CFile::ReadComplete(char *buffer,int &size)
{
	int t_len=chunk_size;
	size=0;
	while(t_len==chunk_size){
		t_len=read(handle,chunk,chunk_size);
		if (t_len<0){
			size=0;
			return;
		}
#ifdef FILE_OS_WINDOWS
		memcpy(&buffer[size],chunk,t_len);
		size+=t_len;
#else
		if (!Binary){
			for (int i=0;i<t_len;i++)
				if (chunk[i]!='\r')
					buffer[size++]=chunk[i];
		}else{
			memcpy(&buffer[size],chunk,t_len);
			size+=t_len;
		}
#endif
	}
}

// read a part of the file into the buffer
void CFile::ReadBuffer(char *buffer,int size)
{
	read(handle,buffer,size);
/*	int t_len=chunk_size;
	size=0;
	while(t_len==chunk_size){
		t_len=read(handle,chunk,chunk_size);
#ifdef FILE_OS_WINDOWS
		memcpy(&buffer[size],chunk,t_len);
		size+=t_len;
#else
		if (!Binary){
			for (int i=0;i<t_len;i++)
				if (chunk[i]!='\r')
					buffer[size++]=chunk[i];
		}else{
			memcpy(&buffer[size],chunk,t_len);
			size+=t_len;
		}
#endif
	}*/
}

// insert the buffer into the file
void CFile::WriteBuffer(char *buffer,int size)
{
	write(handle,buffer,size);
}


// read a single character (1 byte)
char CFile::ReadChar()
{
	if (Error)	return 0;
	char c=0;
	if (read(handle,&c,1)<1)
		Eof=true;
	return ((!Binary)&&(c==0x0d))?ReadChar():c;
}

// read a single character (1 byte)
unsigned char CFile::ReadByte()
{
	if (Error)	return 0;
	if (Binary){
		unsigned char a=0;
		if (read(handle,&a,1)<1)
			Eof=true;
		return a;
	}
	return s2i(ReadStr());
}

unsigned char CFile::ReadByteC()
{
	ReadComment();
	return ReadByte();
}

// read the rest of the line (only text mode)
void CFile::ReadComment()
{
	if ((Error)||(Binary))	return;
#ifdef FILE_COMMENTS_DEBUG
	msg_write(string("comment: ",ReadStr()));
#else
	unsigned char a=0;
	while (a!='\n'){
		if (read(handle,&a,1)<=0){
			Error=true;
			Eof=true;
			break;
		}
	}
#endif

}

// read a word (2 bytes in binary mode)
unsigned short CFile::ReadWord()
{
	if (Error)	return 0;
	if (Binary){
		unsigned short w=0;
		if (read(handle,&w,2)!=2)
			Eof=true;
		return w;
	}
	return s2i(ReadStr());
}

unsigned short CFile::ReadWordC()
{
	ReadComment();
	return ReadWord();
}

// read a word (2 bytes in binary mode)
unsigned short CFile::ReadReversedWord()
{
	if (Error)	return 0;
	if (Binary){
		int i=0;
		unsigned char a;
		if (read(handle,&a,1)<=0){
			Error=true;
			Eof=true;
			return 0;
		}
		i=(int)(a);
		if (read(handle,&a,1)<=0){
			Error=true;
			Eof=true;
			return 0;
		}
		i=i*256+(int)(a);
		return i;
	}
	return s2i(ReadStr());
}

// read an integer (4 bytes in binary mode)
int _cdecl CFile::ReadInt()
{
	if (Error)	return 0;
	if (Binary){
		int i;
		if (read(handle,&i,4)!=4)
			Eof=true;
		return i;
	}
	return s2i(ReadStr());
}

// ignore this line, then read an integer
int _cdecl CFile::ReadIntC()
{
	ReadComment();
	return ReadInt();
}

// read a float (4 bytes in binary mode)
float CFile::ReadFloat()
{
	if (Error)	return 0;
	if (Binary){
		float f;
		if (read(handle,&f,4)!=4)
			Eof=true;
		return f;
	}
	return s2f(ReadStr());
}

// ignore this line, then read an float
float CFile::ReadFloatC()
{
	ReadComment();
	return ReadFloat();
}

// read a boolean (1 byte in binary mode)
bool _cdecl CFile::ReadBool()
{
	if (Error)	return false;
	unsigned char a=0;
	bool res=false;
	if (read(handle,&a,1)<=0){
		Error=true;
		Eof=true;
		return false;
	}
	if (a=='1')
		res=true;
	if (!Binary){
		unsigned char a=0;
		while (a!='\n'){
			if (read(handle,&a,1)<=0){
				Error=true;
				Eof=true;
				break;
			}
		}
	}
	return res;
}

// ignore this line, then read an boolean
bool _cdecl CFile::ReadBoolC()
{
	ReadComment();
	return ReadBool();
}

// read a string
//   text mode:   complete rest of this line
//   binary mode: length word, then string
char *_cdecl CFile::ReadStr()
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *str=StackStr[CurrentStack];
	Error|=Eof;
	if (Error)	return str;
	if (Binary){
		int l=ReadWord();
		if (read(handle,str,l)<l){
			Error=true;
			Eof=true;
			return str;
		}
		str[l]=0;
	}else{
		char a=-1;
		int i=0;
		while((a!='\n')&&(a!=0)){
			if (read(handle,&a,1)<=0){
				Error=true;
				Eof=true;
				return str;
			}
			#ifdef FILE_OS_LINUX
				if (a=='\r')
					continue;
			#endif
			str[i]=a;
			i++;
		}
		str[i-1]=0;
	}
	return str;
}

// read a null-terminated string
char *CFile::ReadStrNT()
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *str=StackStr[CurrentStack];
	Error|=Eof;
	if (Error)	return str;
	char a=-1;
	int i=0;
	while((a!=0)){
		if (read(handle,&a,1)<=0){
			Error=true;
			Eof=true;
			return str;
		}
		#ifdef FILE_OS_LINUX
			if (a=='\r')
				continue;
		#endif
		str[i]=a;
		i++;
	}
	return str;
}

// ignore this line, then read an string
char *_cdecl CFile::ReadStrC()
{
	ReadComment();
	return ReadStr();
}

// read a string havinf reversed byte as length in binary mode
char *CFile::ReadStrRW()
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *str=StackStr[CurrentStack];
	Error|=Eof;
	if (Error)	return str;
	if (Binary){
		int l=ReadReversedWord();
		if (read(handle,str,l)<l){
			Error=true;
			Eof=true;
			return str;
		}
		str[l]=0;
	}else{
		char a=-1;
		int i=0;
		while((a!='\n')&&(a!=0)){
			if (read(handle,&a,1)<=0){
				Error=true;
				Eof=true;
				return str;
			}
			#ifdef FILE_OS_LINUX
				if (a=='\r')
					continue;
			#endif
			str[i]=a;
			i++;
		}
		str[i-1]=0;
	}
	return str;
}

// write a single character (1 byte)
void CFile::WriteChar(char c)
{
	if (Binary)	write(handle,&c,1);
	else		WriteStr(i2s(c));
}

// write a single character (1 byte)
void CFile::WriteByte(unsigned char c)
{
	if (Binary)	write(handle,&c,1);
	else		WriteStr(i2s(c));
}

// write a word (2 bytes)
void CFile::WriteWord(unsigned short w)
{
	/*char c=(w/256)%256;
	write(handle,&c,1);
	c=w%256;
	write(handle,&c,1);*/
	if (Binary)	write(handle,&w,2);
	else		WriteStr(i2s(w));
}

// write an integer (4 bytes)
void _cdecl CFile::WriteInt(int in)
{
	if (Binary)	write(handle,&in,4);
	else		WriteStr(i2s(in));
}

// write a float (4 bytes)
void CFile::WriteFloat(float f)
{
	if (Binary)	write(handle,&f,4);
	else		WriteStr(f2s(f,FloatDecimals));
}

// write a boolean (1 byte)
void _cdecl CFile::WriteBool(bool b)
{
	if (b)	write(handle,"1",1);
	else	write(handle,"0",1);
	if (!Binary)	write(handle,"\n",1);
}

// write a string
//   text mode:   complete rest of this line
//   binary mode: length word, then string
void CFile::WriteStr(char *str)
{
	if (Binary)		WriteWord(strlen(str));
	write(handle,str,strlen(str));
	if (!Binary)	write(handle,"\n",1);
}

// write a string with a given length
void CFile::WriteStr(char *str,int l)
{
	write(handle,str,l);
	if (!Binary)
		write(handle,"\n",1);
}

// write a comment line
void CFile::WriteComment(char *str)
{
#ifdef FILE_COMMENTS_DEBUG
	msg_write(string("comment: ",str));
#endif
	if (!Binary)
		WriteStr(str);
}

// insert some white spaces
void CFile::ShiftRight(int s)
{
#ifdef StructuredShifts
	for (int i=0;i<s-1;i++)
		write(handle," |\t",3);
	if (s>0)
		write(handle," >-\t",4);
#else
	for (int i=0;i<s;i++)
		write(handle,"\t",1);
#endif
}

static CFile *te_file;

// just test the existence of a file
bool file_test_existence(char *filename)
{
	te_file=new CFile();
	te_file->SilentFileAccess=true;
	if (!te_file->Open(filename)){
		delete(te_file);
		return false;
	}
	te_file->Close();
	delete(te_file);
	return true;
}



// connecting strings
/*char *string(char *str,...)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *tmp=StackStr[CurrentStack];
	char *t_str;
	//msg_write>Write("-erster String:");
	//msg_write>Write(str);
	strcpy(tmp,str);
	va_list marker;
	va_start(marker,str);
	t_str=va_arg(marker,char*);
	//msg_write>Write("-naechster String:");
	msg_write>Write((unsigned int)t_str);
	while ((int)t_str>100000)
	{
		msg_write>Write(t_str);
		strcat(tmp,t_str);
		t_str=va_arg(marker,char*);
		//msg_write>Write("-naechster String:");
		msg_write>Write((unsigned int)t_str);
	}
	msg_write>Write("Ende");
	va_end(marker);

	return tmp;
}*/

char *string(char *str,char *str2)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *tmp=StackStr[CurrentStack];
	strcpy(tmp,str);
	strcat(tmp,str2);
	return tmp;
}

char *string(char *str,char *str2,char *str3)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *tmp=StackStr[CurrentStack];
	strcpy(tmp,str);
	strcat(tmp,str2);
	strcat(tmp,str3);
	return tmp;
}

char *string(char *str,char *str2,char *str3,char *str4)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *tmp=StackStr[CurrentStack];
	strcpy(tmp,str);
	strcat(tmp,str2);
	strcat(tmp,str3);
	strcat(tmp,str4);
	return tmp;
}

char *string(char *str,char *str2,char *str3,char *str4,char *str5)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *tmp=StackStr[CurrentStack];
	strcpy(tmp,str);
	strcat(tmp,str2);
	strcat(tmp,str3);
	strcat(tmp,str4);
	strcat(tmp,str5);
	return tmp;
}

char *string(char *str,char *str2,char *str3,char *str4,char *str5,char *str6)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *tmp=StackStr[CurrentStack];
	strcpy(tmp,str);
	strcat(tmp,str2);
	strcat(tmp,str3);
	strcat(tmp,str4);
	strcat(tmp,str5);
	strcat(tmp,str6);
	return tmp;
}

// connecting strings
char *string2(char *str,...)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *tmp=StackStr[CurrentStack];
	va_list args;

    // retrieve the variable arguments
    va_start( args, str );
 
    vsprintf( tmp, str, args ); // C4996
    // Note: vsprintf is deprecated; consider using vsprintf_s instead
	return tmp;
#if 0
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *tmp=StackStr[CurrentStack];
	tmp[0]=0;

	va_list marker;
	va_start(marker,str);

	int l=0,s=strlen(str);
	for (int i=0;i<s;i++){
		if ((str[i]=='%')&&(str[i+1]=='s')){
			strcat(tmp,va_arg(marker,char*));
			i++;
			l=strlen(tmp);
		}else if ((str[i]=='%')&&(str[i+1]=='d')){
			strcat(tmp,i2s(va_arg(marker,int)));
			i++;
			l=strlen(tmp);
		}else if ((str[i]=='%')&&(str[i+1]=='f')){
			int fl=3;
			if (str[i+2]==':'){
				fl=str[i+3]-'0';
				i+=3;
			}else
				i++;
			strcat(tmp,f2s((float)va_arg(marker,double),fl));
			l=strlen(tmp);
		}else if ((str[i]=='%')&&(str[i+1]=='v')){
			int fl=3;
			if (str[i+2]==':'){
				fl=str[i+3]-'0';
				i+=3;
			}else
				i++;
			/*float *v=(float*)&va_arg(marker,double);
			va_arg(marker,float);
			va_arg(marker,float);
			strcat(tmp,"( ");
			strcat(tmp,f2s(v[0],fl));
			strcat(tmp," , ");
			strcat(tmp,f2s(v[1],fl));
			strcat(tmp," , ");
			strcat(tmp,f2s(v[2],fl));
			strcat(tmp," )");
			l=strlen(tmp);*/
msg_write>Error("Todo:  %v");
		}else{
			tmp[l]=str[i];
			tmp[l+1]=0;
			l++;
		}
	}
	va_end(marker);

	return tmp;
#endif
}

// cut the string at the position of a substring
void strcut(char *str,char *dstr)
{
	if (strstr(str,dstr))
		strstr(str,dstr)[0]=0;
}

// convert an integer to a string (with a given number of decimals)
char *i2s2(int i,int l)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *str=StackStr[CurrentStack];
	for (int n=l-1;n>=0;n--){
		str[n]=i%10+48;
		i/=10;
	}
	str[l]=0;
	return str;
}

// convert an integer to a string
char *i2s(int i)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *str=StackStr[CurrentStack];
	int l=0;
	bool m=false;
	if (i<0){
		i=-i;
		m=true;
	}
	char a[128];
	while (1){
		a[l]=(i%10)+48;
		l++;
		i=(int)(i/10);
		if (i==0)
			break;
	}
	if (m){
		a[l]='-';
		l++;
	}
	for (int j=0;j<l;j++)
		str[l-j-1]=a[j];
	str[l]=0;
	return str;
}

// convert a float to a string
char *f2s(float f,int dez)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *str=StackStr[CurrentStack];
	/*strcpy(str,"");
	if (f<0){
		strcat(str,"-");
		f=-f;
	}
	strcat(str,i2s(int(f)));
	if (dez>0){
		strcat(str,",");
		int e=1;
		for (int i=0;i<dez;i++)
			e*=10;
		strcat(str,i2sl(int(f*(float)e)%e,dez));
	}*/
	sprintf(str,string("%.",i2s(dez),"f"),f);
	return str;
}

// convert binary data to a hex-code-string
// inverted:
//    false:   12.34.56.78
//    true:    0x78.56.34.12
char *d2h(void *data,int bytes,bool inverted)
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *str=StackStr[CurrentStack];
	int pos;
	if (inverted)	strcpy(str,"0x");
	unsigned char *c_data=(unsigned char *)data;
	for (int i=0;i<bytes;i++){
		if (inverted)	pos=(bytes-i-1)*3+2;
		else			pos=i*3;
		int c=c_data[0]%16;
		if (c<10)	str[pos+1]=48+c;
		else		str[pos+1]='a'-10+c;
		c=c_data[0]/16;
		if (c<10)	str[pos]=48+c;
		else		str[pos]='a'-10+c;
		str[pos+2]='.';
		c_data++;
	}
	if (inverted)
		str[bytes*3+1]=0;
	else
		str[bytes*3-1]=0;
	return str;
}

// convert a string to an integer
int s2i(char *str)
{
	bool minus=false;
	int res=0;
	for (unsigned int i=0;i<strlen(str);i++){
		if (str[i]=='-')
			minus=true;
		else res=res*10+(str[i]-48);		
	}
	if (minus)
		res=-res;
	return res;
}

// convert a string to a float
float s2f(char *str)
{
	bool minus=false;
	int e=-1;
	float res=0;
	for (unsigned int i=0;i<strlen(str);i++){
		if (e>0)
			e*=10;
		if (str[i]=='-')
			minus=true;
		else{
			if ((str[i]==',')||(str[i]=='.'))
			e=1;
			else{
				if(str[i]!='\n')
					if (e<0)
						res=res*10+(str[i]-48);
					else
						res+=float(str[i]-48)/(float)e;
			}
		}
	}
	if (minus)
		res=-res;
	return res;
}

bool dir_create(char *dir)
{
#ifdef FILE_OS_WINDOWS
	return (mkdir(SysFileName(dir))==0);
#endif
#ifdef FILE_OS_LINUX
	return (mkdir(SysFileName(dir),S_IRWXU | S_IRWXG | S_IRWXO)==0);
#endif
	return false;
}

bool dir_delete(char *dir)
{
	return (rmdir(SysFileName(dir))==0);
}

char *get_current_dir()
{
	CurrentStack=(CurrentStack+1)%STR_STACK_DEPTH;
	char *str=StackStr[CurrentStack];
#ifdef FILE_OS_WINDOWS
	_getcwd(str,sizeof(StackStr[0]));
	strcat(str,"\\");
#endif
#ifdef FILE_OS_LINUX
	getcwd(str,sizeof(StackStr[0]));
	strcat(str,"/");
#endif
	return str;
}

bool file_rename(char *source,char *target)
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

bool file_copy(char *source,char *target)
{
	char dir[512];
	for (unsigned int i=0;i<strlen(target);i++){
		dir[i]=target[i];
		dir[i+1]=0;
		if (i>3)
			if ((target[i]=='/')||(target[i]=='\\'))
				dir_create(dir);
	}
	int hs=open(SysFileName(source),O_RDONLY);
	if (hs<0)
		return false;
#ifdef FILE_OS_WINDOWS
	int ht=creat(SysFileName(target),_S_IREAD | _S_IWRITE);
	_setmode(hs,_O_BINARY);
	_setmode(ht,_O_BINARY);
#endif
#ifdef FILE_OS_LINUX
	int ht=creat(SysFileName(target),S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
	if (ht<0){
		close(hs);
		return false;
	}
	char buf[10240];
	int r=10;
	while(r>0){
		r=read(hs,buf,sizeof(buf));
		write(ht,buf,r);
	}
	close(hs);
	close(ht);
	return true;
}

bool file_delete(char *filename)
{
	return (unlink(SysFileName(filename))==0);
}



int dir_search_num;
char dir_search_name[DIR_SEARCH_MAX_ITEMS][128],*dir_search_name_p[DIR_SEARCH_MAX_ITEMS];
bool dir_search_is_dir[DIR_SEARCH_MAX_ITEMS];

// seach an directory for files matching a filter
int dir_search(char *dir,char *filter,bool show_directories)
{
	dir_search_num=0;
	char *filter2=filter+1;
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
		if ((strcmp(t.name,".")!=0)&&(strcmp(t.name,"..")!=0)){
			if ((strstr(t.name,filter2))|| ((show_directories)&&(t.attrib==_A_SUBDIR)) ){
				strcpy(dir_search_name[dir_search_num],t.name);
				dir_search_is_dir[dir_search_num]=(t.attrib==_A_SUBDIR);
				dir_search_num++;
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
		if ((strcmp(dn->d_name,".")!=0)&&(strcmp(dn->d_name,"..")!=0)){
			stat(string(SysFileName(dir),dn->d_name),&s);
			bool is_reg=(s.st_mode & S_IFREG)>0;
			bool is_dir=(s.st_mode & S_IFDIR)>0;
			int sss=strlen(dn->d_name)-strlen(filter2);
			if (sss<0)	sss=0;
			if ( ((is_reg)&&(strcmp(&dn->d_name[sss],filter2)==0)) || ((show_directories)&&(is_dir)) ){
				strcpy(dir_search_name[dir_search_num],dn->d_name);
				dir_search_is_dir[dir_search_num]=(is_dir);
				dir_search_num++;
			}
		}
		dn=readdir(_dir);
	}
#endif
	// sorting...
	int i,j;
	for (i=0;i<dir_search_num-1;i++)
		for (j=i+1;j<dir_search_num;j++){
			bool ok=true;
			if (dir_search_is_dir[i]==dir_search_is_dir[j]){
				if (strcmp(dir_search_name[i],dir_search_name[j])>0)
					ok=false;
			}else
				if ((!dir_search_is_dir[i])&&(dir_search_is_dir[j]))
					ok=false;
			if (!ok){
				char temp[128];
				strcpy(temp,dir_search_name[i]);
				strcpy(dir_search_name[i],dir_search_name[j]);
				strcpy(dir_search_name[j],temp);
				bool temp2;
				temp2=dir_search_is_dir[i];
				dir_search_is_dir[i]=dir_search_is_dir[j];
				dir_search_is_dir[j]=temp2;
			}
		}
	for (i=0;i<dir_search_num;i++)
		dir_search_name_p[i]=&dir_search_name[i][0];
	return dir_search_num;
}

