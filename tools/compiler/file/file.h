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
| last update: 2009.11.22 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(FILE_H)
#define FILE_H



#define DIR_SEARCH_MAX_ITEMS		1024
#define REGEX_MAX_MATCHES			128
#define FILE_STR_STACK_DEPTH		64

// which operating system?

#ifdef WIN32
	#define FILE_OS_WINDOWS
#else
	#define FILE_OS_LINUX
#endif



// which developing environment?

#ifdef _MSC_VER
	#if _MSC_VER >= 1400
		#define FILE_IDE_VCS8
	#else
		#define FILE_IDE_VCS6
	#endif
#else
	#define FILE_IDE_DEVCPP
#endif
//#define FILE_IDE_KDEVELOP ...?



// ANSI:
//#include <stdarg.h>
// UNIX:
//#include <varargs.h>




#include <string.h>
#ifndef __cplusplus
	typedef unsigned char bool;
	enum{
		false,
		true
	};
	#define int(x)		(int)(x)
	#define float(x)	(float)(x)
#endif

#ifdef FILE_OS_LINUX
	#define _cdecl
	#include <stdlib.h>

	#define max(a,b)	((a>b)?a:b)
	#define min(a,b)	((a<b)?a:b)
#endif



//--------------------------------------------------------------
// michi-array

#define ma_size(var)						(*(int*)((char*)var - sizeof(int)))
#define ma_new(var, type, num_elements)		var = (type*)(new char[num_elements * sizeof(type) + sizeof(int)] + sizeof(int)); ma_size(var) = num_elements
#define ma_delete(var)						delete[]((char*)var - sizeof(int))



//#define foreach(array, pointer, loop_var)	typeof(array[0]) *pointer=&array[0]; for (int loop_var=0;loop_var<array.size();pointer=&array[++loop_var])
#define foreach(array, pointer, loop_var)	if (array.size()>0)pointer=&array[0]; for (int loop_var=0;loop_var<array.size();++loop_var,pointer=(loop_var<array.size())?&array[loop_var]:NULL)


//--------------------------------------------------------------
// time/date

struct sDate{
	int time;
	int year,month,day,hour,minute,second,milli_second;
	int day_of_week,day_of_year;
};

sDate get_current_date();




//--------------------------------------------------------------
// file operation class

typedef bool t_file_try_again_func(const char *);

extern bool SilentFileAccess;
extern t_file_try_again_func *FileTryAgainFunc;

enum{
	FileDateAccess,
	FileDateModification,
	FileDateCreation
};

void file_set_archive(const char *filename);
void file_clean_up_archive();

class CFile
{
public:
	CFile();
	~CFile();
	bool Open(const char *filename);
	bool Create(const char *filename);
	bool Append(const char *filename);
	bool _cdecl Close();

	void SetBinaryMode(bool bm);
	void SetPos(int pos,bool absolute);
	int GetSize();
	int GetPos();
	sDate _cdecl GetDate(int type);

	int ReadFileFormatVersion();
	void WriteFileFormatVersion(bool binary,int fvv);

	int ReadBuffer(void *buffer,int size);
	void ReadComplete(void *buffer,int &size);
	int WriteBuffer(const void *buffer,int size);

	void ReadComment();
	char ReadChar();
	unsigned char ReadByte();
	unsigned char ReadByteC();
	unsigned short ReadWord();
	unsigned short ReadWordC();
	unsigned short ReadReversedWord(); // for antique versions!!
	int _cdecl ReadInt();
	int _cdecl ReadIntC();
	float ReadFloat();
	float ReadFloatC();
	bool _cdecl ReadBool();
	bool _cdecl ReadBoolC();
	char *_cdecl ReadStr();
	char *ReadStrNT();
	char *_cdecl ReadStrC();
	char *ReadStrRW(); // for antique versions!!
	void ReadVector(void *v);

	void WriteChar(char c);
	void WriteByte(unsigned char c);
	void WriteWord(unsigned short);
	void _cdecl WriteInt(int in);
	void WriteFloat(float f);
	void _cdecl WriteBool(bool b);
	void WriteStr(const char *str);
	void WriteStr(const char *str,int l);
	void WriteComment(const char *str);
	void WriteVector(const void *v);

	void ShiftRight(int s);

	bool Eof,Binary,SilentFileAccess;
	int FloatDecimals;

//private:
	int handle;
	bool Error,ErrorReported,DontReportErrors;
};

extern CFile *FileOpen(const char *filename);
extern CFile *FileCreate(const char *filename);
extern CFile *FileAppend(const char *filename);
extern void FileClose(CFile *f);



//--------------------------------------------------------------
// string operations

extern int _file_current_stack_pos_;
extern char _file_stack_str_[FILE_STR_STACK_DEPTH][2048];
#define _file_get_str_()	_file_stack_str_[(_file_current_stack_pos_++)%FILE_STR_STACK_DEPTH]

//char *string(char *str,...);
char *string(const char *str,const char *str2);
char *string(const char *str,const char *str2,const char *str3);
char *string(const char *str,const char *str2,const char *str3,const char *str4);
char *string(const char *str,const char *str2,const char *str3,const char *str4,const char *str5);
char *string(const char *str,const char *str2,const char *str3,const char *str4,const char *str5,const char *str6);
char *string2(const char *str,...);
void strcut(char *str,const char *dstr);
char *i2s(int i);
char *i2s2(int i,int l);
char *f2s(float f,int dez);
int s2i(const char *str);
float s2f(const char *str);

char *d2h(const void *data,int bytes,bool inverted=true);
char *h2d(const char *hex_str,int bytes);


char *SysFileName(const char *filename);
char *dir_from_filename(const char *filename);
char *file_from_filename(const char *filename);
void dir_ensure_ending(char *dir,bool slash);
char *filename_no_recursion(const char *filename);
char *file_extension(const char *filename);


//--------------------------------------------------------------
// file/directory operations

bool dir_create(const char *dir);
bool dir_delete(const char *dir);
char *get_current_dir();
bool file_rename(const char *source,const char *target);
bool file_copy(const char *source,const char *target);
bool file_delete(const char *filename);
bool file_test_existence(const char *filename);


//--------------------------------------------------------------
// searching directories

extern int dir_search_num;
extern char dir_search_name[DIR_SEARCH_MAX_ITEMS][128];
extern char *dir_search_name_p[DIR_SEARCH_MAX_ITEMS];
extern bool dir_search_is_dir[DIR_SEARCH_MAX_ITEMS];

int _cdecl dir_search(const char *dir,const char *filter,bool show_directories);

int regex_match(char *rex,char *str,int max_matches=1);
char *regex_replace(char *rex,char *str,int max_matches=0);


//--------------------------------------------------------------
// regular expressions

extern char *regex_out_match[REGEX_MAX_MATCHES];
extern int regex_out_pos[REGEX_MAX_MATCHES],regex_out_length[REGEX_MAX_MATCHES];


#endif

