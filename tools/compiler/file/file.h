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
| last update: 2010.07.01 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(FILE_H)
#define FILE_H



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

#include "msg.h"
#include "array.h"
#include "file_op.h"
#include "strings.h"



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

	// opening
	bool Open(const char *filename);
	bool Create(const char *filename);
	bool Append(const char *filename);
	bool _cdecl Close();

	// meta
	void SetBinaryMode(bool bm);
	void SetPos(int pos,bool absolute);
	int GetSize();
	int GetPos();
	sDate _cdecl GetDate(int type);

	// file format version
	int ReadFileFormatVersion();
	void WriteFileFormatVersion(bool binary,int fvv);

	// really low level
	int ReadBuffer(void *buffer,int size);
	void ReadComplete(void *buffer,int &size);
	int WriteBuffer(const void *buffer,int size);

	// medium level
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
	void WriteStrL(const char *str,int l);
	void WriteComment(const char *str);
	void WriteVector(const void *v);

	// high level
	void Int(int &i);
	void Float(float &f);
	void Bool(bool &b);
	void String(char *str);
	void Vector(float *v);
	void Struct(const char *format, void *data);
	void StructN(const char *format, int &num, void *data, int shift);

	void ShiftRight(int s);

	bool Eof, Binary, SilentFileAccess, Reading;
	int FloatDecimals;

//private:
	int handle;
	bool Error,ErrorReported,DontReportErrors;
};

extern CFile *FileOpen(const char *filename);
extern CFile *FileCreate(const char *filename);
extern CFile *FileAppend(const char *filename);
extern void FileClose(CFile *f);




#endif

