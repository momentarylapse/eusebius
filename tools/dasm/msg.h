/*----------------------------------------------------------------------------*\
| msg                                                                          |
| -> debug logfile system                                                      |
| -> "message.txt" used for output                                             |
|                                                                              |
| vital properties:                                                            |
|  - stores the latest messages in memory for direct access                    |
|                                                                              |
| last updated: 2008.06.19 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(MSG_H)
#define MSG_H

#include "file.h"


#define MSG_DEBUG_ALLOW_LEVEL		9

#define msg_db_r(text,level)\
	{if (level<=MSG_DEBUG_ALLOW_LEVEL)\
		msg_trace_r(text,level);}
#define msg_db_m(text,level)\
	{if (level<=MSG_DEBUG_ALLOW_LEVEL)\
		msg_trace_m(text,level);}
#define msg_db_l(level)\
	{if (level<=MSG_DEBUG_ALLOW_LEVEL)\
		msg_trace_l(level);}


// administration
void msg_init(bool verbose=true,char *force_filename=NULL);
void msg_set_verbose(bool verbose);
void msg_end(bool del_file=true);
// pure output
void msg_write(int i);
void msg_write(char *str);
void msg_write(char *str,int l);
void msg_write2(char *str,...);
void msg_error(char *str);
void msg_ok();
void msg_right();
void msg_left();
void msg_reset_shift();
// structured output
void msg_trace_r(char *str,int level=0);
void msg_trace_m(char *str,int level=0);
void msg_trace_l(int level=0);
char *msg_get_str(int index);
char *msg_get_trace();
//void msg_db_out(int dl,char *str);
// output only once
void msg_todo(char *str);

extern bool msg_inited;


#endif

