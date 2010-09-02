#include "file.h"


#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <vector>

#ifdef FILE_OS_WINDOWS
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <stdarg.h>
	#include <windows.h>
	#include <winbase.h>
   #include <winnt.h>
#endif
#ifdef FILE_OS_LINUX
	#include <unistd.h>
	#include <dirent.h>
	#include <stdarg.h>
	#include <sys/timeb.h>
	#include <sys/stat.h>
#endif


int _file_current_stack_pos_=0;
char _file_stack_str_[FILE_STR_STACK_DEPTH][2048];



char FileTempName[8][1024];
int CurrentFileTempName=0;

inline char *get_temp_name()
{
	CurrentFileTempName ++;
	if (CurrentFileTempName >= 8)
		CurrentFileTempName = 0;
	return FileTempName[CurrentFileTempName];
}

// transposes path-strings to the current operating system
// accepts windows and linux paths ("/" and "\\")
char *SysFileName(const char *filename)
{
	char *str = get_temp_name();
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
char *dir_from_filename(const char *filename)
{
	char *str = get_temp_name();
	strcpy(str,filename);
	for (int i=strlen(str)-1;i>=0;i--){
		if ((str[i]=='/')||(str[i]=='\\')){
			str[i+1]=0;
			break;
		}
		if (i==0)
			str[i]=0;
	}
	return str;
}

char *file_from_filename(const char *filename)
{
	char *str = get_temp_name();
	strcpy(str, filename);
	for (int i=strlen(str)-1;i>=0;i--){
		if ((str[i]=='/')||(str[i]=='\\')){
			strcpy(str, &filename[i+1]);
			str[i+1]=0;
			break;
		}
	}
	return str;
}

// make sure the name ends with (or without) a shlash
void dir_ensure_ending(char *dir,bool slash)
{
	char lc=dir[strlen(dir)-1];
	if ((slash)&&(lc!='/')&&(lc!='\\'))
		strcat(dir,"/");
	if ((!slash)&&((lc=='/')||(lc=='\\')))
		dir[strlen(dir)-1]=0;
}

// remove "/../"
char *filename_no_recursion(const char *filename)
{
	char *str = get_temp_name();
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

char *file_extension(const char *filename)
{
	char *str = get_temp_name();
	strcpy(str,"");
	int pie=-1;
	int length = strlen(filename);
	for (int i=0;i<length;i++){
		if (pie >= 0){
			str[pie] = filename[i];
			str[pie+1] = 0;
			pie ++;
		}
		if (filename[i] == '.')
			pie=0;
	}
	return str;
}




// connecting strings
/*char *string(char *str,...)
{
	char *tmp=_file_get_str_();
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

char *string(const char *str,const char *str2)
{
	char *tmp=_file_get_str_();
	strcpy(tmp,str);
	strcat(tmp,str2);
	return tmp;
}

char *string(const char *str,const char *str2,const char *str3)
{
	char *tmp=_file_get_str_();
	strcpy(tmp,str);
	strcat(tmp,str2);
	strcat(tmp,str3);
	return tmp;
}

char *string(const char *str,const char *str2,const char *str3,const char *str4)
{
	char *tmp=_file_get_str_();
	strcpy(tmp,str);
	strcat(tmp,str2);
	strcat(tmp,str3);
	strcat(tmp,str4);
	return tmp;
}

char *string(const char *str,const char *str2,const char *str3,const char *str4,const char *str5)
{
	char *tmp=_file_get_str_();
	strcpy(tmp,str);
	strcat(tmp,str2);
	strcat(tmp,str3);
	strcat(tmp,str4);
	strcat(tmp,str5);
	return tmp;
}

char *string(const char *str,const char *str2,const char *str3,const char *str4,const char *str5,const char *str6)
{
	char *tmp=_file_get_str_();
	strcpy(tmp,str);
	strcat(tmp,str2);
	strcat(tmp,str3);
	strcat(tmp,str4);
	strcat(tmp,str5);
	strcat(tmp,str6);
	return tmp;
}

// connecting strings
char *string2(const char *str,...)
{
	char *tmp=_file_get_str_();
	va_list args;

    // retrieve the variable arguments
    va_start( args, str );
 
    vsprintf( tmp, str, args ); // C4996
    // Note: vsprintf is deprecated; consider using vsprintf_s instead
	return tmp;
#if 0
	char *tmp=_file_get_str_();
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
void strcut(char *str,const char *dstr)
{
	if (strstr(str,dstr))
		strstr(str,dstr)[0]=0;
}

// convert an integer to a string (with a given number of decimals)
char *i2s2(int i,int l)
{
	char *str=_file_get_str_();
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
	char *str=_file_get_str_();
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
	char *str=_file_get_str_();
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

// convert a float to a string
char *f2sf(float f)
{
	char *str=_file_get_str_();
	sprintf(str,"%f",f);
	return str;
}

// convert a bool to a string
char *b2s(bool b)
{
	char *str = _file_get_str_();
	strcpy(str, b ? "true" : "false");
	return str;
}

// convert a vector to a string
char *fff2s(float *f)
{
	char *str = _file_get_str_();
	sprintf(str,"(%f  %f  %f)", f[0], f[1], f[2]);
	return str;
}
char *ff2s(float *f)
{
	char *str = _file_get_str_();
	sprintf(str,"(%f  %f)", f[0], f[1]);
	return str;
}
char *ffff2s(float *f)
{
	char *str = _file_get_str_();
	sprintf(str,"(%f  %f  %f  %f)", f[0], f[1], f[2], f[3]);
	return str;
}
char *p2s(void *p)
{
	char *str = _file_get_str_();
	sprintf(str,"%p", p);
	return str;
}

// convert binary data to a hex-code-string
// inverted:
//    false:   12.34.56.78
//    true:    0x78.56.34.12
char *d2h(const void *data,int bytes,bool inverted)
{
	char *str=_file_get_str_();
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
int s2i(const char *str)
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
float s2f(const char *str)
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


char *regex_out_match[REGEX_MAX_MATCHES];
int regex_out_pos[REGEX_MAX_MATCHES],regex_out_length[REGEX_MAX_MATCHES];

int regex_match(char *rex,char *str,int max_matches)
{
	int ss=strlen(str);
	int rs=strlen(rex);

	if ((max_matches<=0)||(max_matches>REGEX_MAX_MATCHES))
		max_matches=REGEX_MAX_MATCHES;

	int n_matches=0;

	for (int i=0;i<ss;i++){
		bool match=true;
		for (int j=0;j<rs;j++){
			if (i+j>=ss)
				match=false;
			else if (str[i+j]!=rex[j])
				match=false;
		}
		if (match){
			regex_out_pos[n_matches]=i;
			regex_out_length[n_matches]=rs;
			regex_out_match[n_matches]=&str[i];
			n_matches++;
			if (n_matches>=max_matches)
				return n_matches;
		}
	}
	return n_matches;
}

/*char *regex_replace(char *rex,char *str,int max_matches)
{
}*/

