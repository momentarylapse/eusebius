/*----------------------------------------------------------------------------*\
| msg                                                                          |
| -> debug logfile system                                                      |
| -> "message.txt" used for output                                             |
|                                                                              |
| vital properties:                                                            |
|  - stores the latest messages in memory for direct access                    |
|                                                                              |
| last updated: 2010.03.06 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(MSG_H)
#define MSG_H



#define MSG_DEBUG_ALLOW_LEVEL		9

#define msg_db_r(text, level)\
	{if (level <= MSG_DEBUG_ALLOW_LEVEL)\
		msg_trace_r(text, level);}
#define msg_db_m(text, level)\
	{if (level <= MSG_DEBUG_ALLOW_LEVEL)\
		msg_trace_m(text, level);}
#define msg_db_l(level)\
	{if (level <= MSG_DEBUG_ALLOW_LEVEL)\
		msg_trace_l(level);}


// administration
void msg_init(bool verbose=true, const char *force_filename = NULL);
void msg_set_verbose(bool verbose);
void msg_end(bool del_file = true);

// pure output
void msg_write(int i);
void msg_write(const char *str);
void msg_write(const char *str, int l);
void msg_write2(const char *str,...);
void msg_error(const char *str);
void msg_ok();
void msg_right();
void msg_left();
void msg_reset_shift();

// structured output   (str may only be global consts...saved as reference)
void msg_trace_r(const char *str, int level = 0);
void msg_trace_m(const char *str, int level = 0);
void msg_trace_l(const int level = 0);

// reading log
const char *msg_get_str(int index);
void msg_get_buffer(char *buffer, int &size, int max_size);
int msg_get_buffer_size();
const char *msg_get_trace();
//void msg_db_out(int dl,char *str);

// output only once
void msg_todo(const char *str);

extern bool msg_inited;


#endif

