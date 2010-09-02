/*----------------------------------------------------------------------------*\
| msg                                                                          |
| -> debug logfile system                                                      |
| -> "message.txt" used for output                                             |
|                                                                              |
| vital properties:                                                            |
|  - stores the latest messages in memory for direct access                    |
|                                                                              |
| last updated: 2010.07.14 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "file.h"
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>

#ifdef FILE_OS_WINDOWS
	#include <windows.h>
#endif



// choose the level of debugging here
//    0 = very few messages
//  >10 = cruel amount of messages
#define MSG_DEBUG_OUTPUT_LEVEL		0

//#define MSG_LOG_TIMIGS
//#define MSG_LIKE_HTML
#define MSG_TRACE_REF



static std::string log_buffer;
static std::vector<int> log_pos;
static std::vector<int> log_length;

bool msg_inited = false;

static CFile *file = NULL;
static int Shift;

static bool Verbose=false;
static bool ErrorOccured;
static char ErrorMsg[4096];

// for msg_get_str
#define MSG_MAX_MESSAGE_LENGTH		512

// tracing system
#define MSG_NUM_TRACES_SAVED		256
#define MSG_MAX_TRACE_LENGTH		96

#ifdef MSG_TRACE_REF
	static const char *TraceStr[MSG_NUM_TRACES_SAVED];
#else
	static char TraceStr[MSG_NUM_TRACES_SAVED][MSG_MAX_TRACE_LENGTH];
#endif
static int CurrentTraceLevel=0;
static char TraceBuffer[MSG_NUM_TRACES_SAVED*MSG_MAX_TRACE_LENGTH/2];



void msg_init(bool verbose,const char *force_filename)
{
	Verbose=false;
	file=new CFile();
	Shift=0;
	ErrorOccured=false;
	msg_inited=true;
	if (!verbose)
		return;
	if (force_filename)
		file->Create(force_filename);
	else
		file->Create("message.txt");
	Verbose=verbose;
#if MSG_LOG_TIMIGS>0
	file->WriteStr("[hh:mm:ss, ms]");
#endif
}

void msg_set_verbose(bool verbose)
{
	if (Verbose==verbose)
		return;
	if (verbose){
		file->Create("message.txt");
		Shift=0;
	}else{
		msg_end(false);
	}
	Verbose=verbose;
}

static void _strcpy_save_(char *a, const char *b,int max_length)
{
	int l=strlen(b);
	if (l>max_length-1)
		l=max_length-1;
	memcpy(a,b,l);
	a[l]=0;
}

void msg_add_str(const char *str)
{
	if (!Verbose)	return;
	int l = strlen(str);
	log_pos.push_back(log_buffer.size());
	log_length.push_back(Shift * 4 + l);
	log_buffer.append(Shift * 4, ' ');
	log_buffer.append(str, l);
	log_buffer.append(1, '\n');
	for (int i=0;i<Shift;i++)
		printf("    ");
	printf("%s\n", str);
}

void write_date()
{
#if MSG_LOG_TIMIGS>0
	sDate t=get_current_date();
	char tstr[128];
	sprintf(tstr,"[%d%d:%d%d:%d%d,%d%d%d]\t",	t.hour/10,t.hour%10,
												t.minute/10,t.minute%10,
												t.second/10,t.second%10,
												t.milli_second/100,(t.milli_second/10)%10,t.milli_second%10);
	file->WriteBuffer(tstr,strlen(tstr));
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

void msg_write(const char *str)
{
	if (!Verbose)	return;
	write_date();
	file->ShiftRight(Shift);
	file->WriteStr(str);

	msg_add_str(str);
}

void msg_write(const char *str,int l)
{
	if (!Verbose)	return;
	write_date();
	file->ShiftRight(Shift);
	file->WriteStrL(str,l);

	msg_add_str(str);
}

void msg_write2(const char *str,...)
{
#if 1
	va_list arg;
	va_start(arg,str);
	msg_write(string2(str,arg));
#else
#ifdef FILE_OS_WINDOWS
	char tmp[1024];
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

void msg_error(const char *str)
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
	if (Shift<0)
		Shift=0;
}

void msg_reset_shift()
{
	if (!Verbose)	return;
	Shift=0;
}

void msg_ok()
{
	msg_write("-ok");
}

void msg_trace_r(const char *str,int level)
{
	if (CurrentTraceLevel >= MSG_NUM_TRACES_SAVED)
		return;
#ifdef MSG_TRACE_REF
	TraceStr[CurrentTraceLevel ++] = str;
#else
	char *mstr=TraceStr[CurrentTraceLevel++];
	_strcpy_save_(mstr,str,MSG_MAX_TRACE_LENGTH);
	strcpy(TraceStr[CurrentTraceLevel],"");
#endif
	if (MSG_DEBUG_OUTPUT_LEVEL>=level){//CurrentTraceLevel){
#ifdef MSG_LIKE_HTML
		msg_write(string("<",str,">"));
#else
		msg_write(str);
#endif
		msg_right();
	}
}

void msg_trace_m(const char *str,int level)
{
	if (CurrentTraceLevel >= MSG_NUM_TRACES_SAVED)
		return;
#ifdef MSG_TRACE_REF
	TraceStr[CurrentTraceLevel] = str;
#else
	_strcpy_save_(TraceStr[CurrentTraceLevel],str,MSG_MAX_TRACE_LENGTH);
#endif
	if (MSG_DEBUG_OUTPUT_LEVEL >= level)//CurrentTraceLevel)
		msg_write(str);
}

void msg_trace_l(int level)
{
#ifdef MSG_TRACE_REF
	TraceStr[CurrentTraceLevel--] = NULL;
#else
	strcpy(TraceStr[CurrentTraceLevel--],"");
#endif
	if (CurrentTraceLevel<0){
		msg_error("msg_trace_l(): level below 0!");
		CurrentTraceLevel=0;
	}
	if (MSG_DEBUG_OUTPUT_LEVEL>=level){//CurrentTraceLevel){
#ifdef MSG_LIKE_HTML
		msg_left();
		msg_write(string("</",TraceStr[CurrentTraceLevel],">"));
#else
		msg_ok();
		msg_left();
#endif
	}
}

const char *msg_get_trace()
{
	strcpy(TraceBuffer,"");
	for (int i=0;i<CurrentTraceLevel;i++){
		strcat(TraceBuffer,TraceStr[i]);
		if (i<CurrentTraceLevel-1)
			strcat(TraceBuffer,"  ->  ");
	}
#ifdef MSG_TRACE_REF
	if (TraceStr[CurrentTraceLevel])
		strcat(TraceBuffer,string(" ( ->  ",TraceStr[CurrentTraceLevel],")"));
#else
	if (strlen(TraceStr[CurrentTraceLevel])>0)
		strcat(TraceBuffer,string(" ( ->  ",TraceStr[CurrentTraceLevel],")"));
#endif
	return TraceBuffer;
}

void msg_end(bool del_file)
{
	//if (!msg_inited)	return;
	if (!file)		return;
	if (!Verbose)	return;
	file->WriteStr("\n\n\n\n"\
" #                       # \n"\
"###                     ###\n"\
" #     natural death     # \n"\
" #                       # \n"\
" #        _              # \n"\
"       * / b   *^| _       \n"\
"______ |/ ______ |/ * _____\n");
	Verbose=false;
	msg_inited=false;
	file->Close();
	if (del_file){
		delete(file);
		file=NULL;
	}
}

void msg_db_out(int dl,const char *str)
{
	if (!Verbose)	return;
	if (dl<=MSG_DEBUG_OUTPUT_LEVEL)
		msg_write(str);
}

static char MsgTempStr[MSG_MAX_MESSAGE_LENGTH + 1];
// index = 0   -> latest log
const char *msg_get_str(int index)
{
	if (!Verbose)
		return "";
	index = (log_pos.size() - 1 - index);
	if (index < 0)
		return "";
	int l = log_length[index];
	if (l > MSG_MAX_MESSAGE_LENGTH)
		l = MSG_MAX_MESSAGE_LENGTH;
	for (int i=0;i<l;i++)
		MsgTempStr[i] = log_buffer[log_pos[index] + i];
	MsgTempStr[l] = 0;
	return MsgTempStr;
}

void msg_get_buffer(char *buffer, int &size, int max_size)
{
	if (log_buffer.size() < max_size){
		strcpy(buffer, log_buffer.c_str());
		size = log_buffer.size();
	}else{
		// not all -> use only the latest log
		int i0 = log_buffer.size() - max_size;
		for (int i=0;i<max_size - 1;i++)
			buffer[i] = log_buffer[i0 + i];
		buffer[max_size - 1] = 0;
		size = max_size - 1;
	}
}

int msg_get_buffer_size()
{
	return log_buffer.size();
}

std::vector<std::string> TodoStr;
void msg_todo(const char *str)
{
	for (int i=0;i<TodoStr.size();i++)
		if (TodoStr[i].compare(str) == 0)
			return;
	TodoStr.push_back(str);
	msg_error(string("TODO (Engine): ",str));
}

