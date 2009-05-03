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
| last update: 2008.12.06 (c) by MichiSoft TM                                  |
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
// time/date

struct sDate{
	int time;
	int year,month,day,hour,minute,second,milli_second;
	int day_of_week,day_of_year;
};

sDate get_current_date();




//--------------------------------------------------------------
// file operation class

typedef bool t_file_try_again_func(char *);

extern bool SilentFileAccess;
extern t_file_try_again_func *FileTryAgainFunc;

enum{
	FileDateAccess,
	FileDateModification,
	FileDateCreation
};

void file_set_archive(char *filename);
void file_clean_up_archive();

class CFile
{
public:
	CFile();
	bool Open(char *filename);
	bool Create(char *filename);
	bool Append(char *filename);
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

	void WriteChar(char c);
	void WriteByte(unsigned char c);
	void WriteWord(unsigned short);
	void _cdecl WriteInt(int in);
	void WriteFloat(float f);
	void _cdecl WriteBool(bool b);
	void WriteStr(const char *str);
	void WriteStr(const char *str,int l);
	void WriteComment(char *str);

	void ShiftRight(int s);

	bool Eof,Binary,SilentFileAccess;
	int FloatDecimals;

//private:
	int handle;
	bool Error,ErrorReported,DontReportErrors;
};



//--------------------------------------------------------------
// string operations

extern int _file_current_stack_pos_;
extern char _file_stack_str_[FILE_STR_STACK_DEPTH][2048];
#define _file_get_str_()	_file_stack_str_[(_file_current_stack_pos_++)%FILE_STR_STACK_DEPTH]

//char *string(char *str,...);
char *string(char *str,char *str2);
char *string(char *str,char *str2,char *str3);
char *string(char *str,char *str2,char *str3,char *str4);
char *string(char *str,char *str2,char *str3,char *str4,char *str5);
char *string(char *str,char *str2,char *str3,char *str4,char *str5,char *str6);
char *string2(char *str,...);
void strcut(char *str,char *dstr);
char *i2s(int i);
char *i2s2(int i,int l);
char *f2s(float f,int dez);
int s2i(char *str);
float s2f(char *str);

char *d2h(void *data,int bytes,bool inverted=true);
char *h2d(char *hex_str,int bytes);


char *SysFileName(char *filename);
char *dir_from_filename(char *filename);
void dir_ensure_ending(char *dir,bool slash);
char *filename_no_recursion(char *filename);


//--------------------------------------------------------------
// file/directory operations

bool dir_create(char *dir);
bool dir_delete(char *dir);
char *get_current_dir();
bool file_rename(char *source,char *target);
bool file_copy(char *source,char *target);
bool file_delete(char *filename);
bool file_test_existence(char *filename);


//--------------------------------------------------------------
// searching directories

extern int dir_search_num;
extern char dir_search_name[DIR_SEARCH_MAX_ITEMS][128];
extern char *dir_search_name_p[DIR_SEARCH_MAX_ITEMS];
extern bool dir_search_is_dir[DIR_SEARCH_MAX_ITEMS];

int _cdecl dir_search(char *dir,char *filter,bool show_directories);

int regex_match(char *rex,char *str,int max_matches=1);
char *regex_replace(char *rex,char *str,int max_matches=0);


//--------------------------------------------------------------
// regular expressions

extern char *regex_out_match[REGEX_MAX_MATCHES];
extern int regex_out_pos[REGEX_MAX_MATCHES],regex_out_length[REGEX_MAX_MATCHES];


#endif

