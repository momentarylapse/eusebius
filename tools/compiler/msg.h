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
#if !defined(MSG_H)
#define MSG_H

#include "file.h"




void msg_init(bool verbose=true);
void msg_set_verbose(bool verbose);
void msg_write(int i);
void msg_write(char *str);
void msg_write(char *str,int l);
void msg_write2(char *str,...);
void msg_error(char *str);
void msg_right();
void msg_left();
void msg_trace_r(char *str);
void msg_trace_m(char *str);
void msg_trace_l();
void msg_ok();
void msg_end(bool del_file=true);
void msg_db_out(int dl,char *str);
char *msg_get_str(int index);
char *msg_get_trace();
void msg_todo(char *str);

extern bool msg_inited;


#endif

