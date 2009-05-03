/*----------------------------------------------------------------------------*\
| msg                                                                          |
| -> debug logfile system                                                      |
| -> "message.txt" used for output                                             |
|                                                                              |
| vital properties:                                                            |
|  - stores the latest messages in memory for direct access                    |
|                                                                              |
| last updated: 2007.03.25 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "msg.h"
#include "file.h"
#include <stdio.h>
#include <stdarg.h>

bool msg_inited=false;


// choose the level of debugging here
//    0 = very few messages
//   10 = cruel amount of messages
#define DebugLevel		0

//#define LogTimings

#ifdef FILE_OS_WINDOWS
#include <windows.h>
#endif
#ifdef LogTimings
	#include <io.h>
	#include <unistd.h>
#endif


static CFile *file=NULL;
static int Shift;

static bool Verbose=false;
static bool ErrorOccured;
static char ErrorMsg[256];

// the last messages
#define MSG_NUM_MESSAGES_SAVED		1024
#define MSG_MAX_MESSAGE_LENGTH		128

static char MessageStr[MSG_NUM_MESSAGES_SAVED][MSG_MAX_MESSAGE_LENGTH];
static int CurrentMessageStr=0;

// tracing system
#define MSG_NUM_TRACES_SAVED		256
#define MSG_MAX_TRACE_LENGTH		32

static char TraceStr[MSG_NUM_TRACES_SAVED][MSG_MAX_TRACE_LENGTH];
static int CurrentTraceLevel=0;
static char TraceBuffer[MSG_NUM_TRACES_SAVED*MSG_MAX_TRACE_LENGTH/2];



void msg_init(bool verbose)
{
	Verbose=false;
	file=new CFile();
	Shift=0;
	CurrentMessageStr=0;
	if (!verbose)
		return;
	file->Create("message.txt");
	Verbose=verbose;
#ifdef LogTimings
	file->WriteStr("[hh:mm:ss, ms]");
#endif
	ErrorOccured=false;
	msg_inited=true;
}

void msg_set_verbose(bool verbose)
{
	msg_end(false);
	Verbose=verbose;
	if (!Verbose)
		return;
	file->Create("message.txt");
	Shift=0;
	CurrentMessageStr=0;
}

void msg_add_str(char *str)
{
	if (!Verbose)	return;
	strcpy(MessageStr[CurrentMessageStr%MSG_NUM_MESSAGES_SAVED],"");
	for (int i=0;i<Shift;i++)
		strcat(MessageStr[CurrentMessageStr%MSG_NUM_MESSAGES_SAVED],"    ");
	strcat(MessageStr[CurrentMessageStr%MSG_NUM_MESSAGES_SAVED],str);
	CurrentMessageStr++;
	printf("%s\n",str);
}

void write_date()
{
#ifdef LogTimings
	sDate t=get_current_date();
	char tstr[128];
	sprintf(tstr,"[%d%d:%d%d:%d%d,%d%d%d]\t",	t.hour/10,t.hour%10,
												t.minute/10,t.minute%10,
												t.second/10,t.second%10,
												t.milli_second/100,(t.milli_second/10)%10,t.milli_second%10);
	write(msg_write>file->handle,tstr,strlen(tstr));
	printf(tstr);
#endif
}

void msg_write(int i)
{
	if (!Verbose)	return;
	write_date();
	file->ShiftRight(Shift);
	file->WriteInt(i);

	msg_add_str(i2s(i));
}

void msg_write(char *str)
{
	if (!Verbose)	return;
	write_date();
	file->ShiftRight(Shift);
	file->WriteStr(str);

	msg_add_str(str);
}

void msg_write(char *str,int l)
{
	if (!Verbose)	return;
	write_date();
	file->ShiftRight(Shift);
	file->WriteStr(str,l);

	msg_add_str(str);
}

void msg_write2(char *str,...)
{
#if 1
	va_list arg;
	va_start(arg,str);
	msg_write(string2(str,arg));
#else
#ifdef FILE_OS_WINDOWS
	char tmp[256];
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
	write(tmp);
#endif
#endif
}

void msg_error(char *str)
{
	if (!Verbose)	return;
	int s=Shift;
	Shift=0;
	msg_write("");
	msg_write("----------------------------- Error! -----------------------------");
	msg_write(str);
	msg_write("------------------------------------------------------------------");
	msg_write("");
	Shift=s;
	if (!ErrorOccured)
		strcpy(ErrorMsg,str);
	ErrorOccured=true;
}

void msg_right()
{
	if (!Verbose)	return;
	Shift++;
}

void msg_left()
{
	if (!Verbose)	return;
	Shift--;
}

void msg_ok()
{
	msg_write("-ok");
}

void msg_trace_r(char *str)
{
	strcpy(TraceStr[CurrentTraceLevel++],str);
	strcpy(TraceStr[CurrentTraceLevel],"");
	if (DebugLevel>CurrentTraceLevel){
		msg_write(string("<",str,">"));
		msg_right();
	}
}

void msg_trace_m(char *str)
{
	strcpy(TraceStr[CurrentTraceLevel],str);
	if (DebugLevel>CurrentTraceLevel)
		msg_write(str);
}

void msg_trace_l()
{
	strcpy(TraceStr[CurrentTraceLevel--],"");
	if (CurrentTraceLevel<0){
		msg_error("msg_trace_l(): level below 0!");
		CurrentTraceLevel=0;
	}
	if (DebugLevel>CurrentTraceLevel){
		msg_left();
		msg_write(string("</",TraceStr[CurrentTraceLevel],">"));
	}
}

char *msg_get_trace()
{
	strcpy(TraceBuffer,"");
	for (int i=0;i<CurrentTraceLevel;i++){
		strcat(TraceBuffer,TraceStr[i]);
		if (i<CurrentTraceLevel-1)
			strcat(TraceBuffer,"  ->  ");
	}
	if (strlen(TraceStr[CurrentTraceLevel])>0)
		strcat(TraceBuffer,string("  ->  (",TraceStr[CurrentTraceLevel],")"));
	return TraceBuffer;
}

void msg_end(bool del_file)
{
	if (!file)
		return;
	if (!Verbose)	return;
	file->WriteStr("\n\n\n\n"\
" #                       # \n"\
"###                     ###\n"\
" #     natural death     # \n"\
" #                       # \n"\
" #        _              # \n"\
"       * / b   *^| _       \n"\
"______ |/ ______ |/ * _____\n");
	file->Close();
	if (del_file){
		delete(file);
		file=NULL;
	}
	Verbose=false;
}

void msg_db_out(int dl,char *str)
{
	if (!Verbose)	return;
	if (dl<=DebugLevel)
		msg_write(str);
}

char *msg_get_str(int index)
{
	if (!Verbose)	return "";
	index=(CurrentMessageStr-1-index)%MSG_NUM_MESSAGES_SAVED;
	if (index<0)
		return "";
	return MessageStr[index];
}

static int NumTodos=0;
static char TodoStr[128][128];
void msg_todo(char *str)
{
	for (int i=0;i<NumTodos;i++)
		if (strcmp(str,TodoStr[i])==0)
			return;
	strcpy(TodoStr[NumTodos],str);
	NumTodos++;
	msg_error(string("TODO (Engine): ",str));
}

