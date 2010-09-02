#if !defined(STRINGS_H__INCLUDED_)
#define STRINGS_H__INCLUDED_


#define REGEX_MAX_MATCHES			128
#define FILE_STR_STACK_DEPTH		64


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
char *f2sf(float f);
char *b2s(bool b);
char *ffff2s(float *f);
char *fff2s(float *f);
char *ff2s(float *f);
char *p2s(void *p);
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
// regular expressions

extern char *regex_out_match[REGEX_MAX_MATCHES];
extern int regex_out_pos[REGEX_MAX_MATCHES],regex_out_length[REGEX_MAX_MATCHES];


int regex_match(char *rex,char *str,int max_matches=1);
char *regex_replace(char *rex,char *str,int max_matches=0);


#endif