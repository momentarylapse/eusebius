
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#ifdef FILE_OS_WINDOWS
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <stdarg.h>
#endif
#ifdef FILE_OS_LINUX
	#include <unistd.h>
	#include <dirent.h>
	#include <stdarg.h>
#endif

#include <stdlib.h>
#include "dasm.h"
#include "msg.h"

static char buffer[1024*1024];

int Str2Int2(char *str)
{
	if ((str[0]=='0')&&(str[1]=='x')){
		int r=0;
		str+=2;
		while(str[0]!=0){ 
			r*=16;
			if ((str[0]>='0')&&(str[0]<='9'))
				r+=str[0]-48;
			if ((str[0]>='a')&&(str[0]<='f'))
				r+=str[0]-'a'+10;
			if ((str[0]>='A')&&(str[0]<='F'))
				r+=str[0]-'A'+10;
			str++;
		}
		return r;
	}else
		return	s2i(str);
}

int main(int narg,char *arg[])
{
	bool ParamNoComments=false;
	msg_init();
	if (narg<2){
		msg_write("Verwendung:    Dasm [-Optionen] <Datei> [ <Start> [ <Ende> ] ]");
		msg_write("Optionen:");
		msg_write("    r   16bit RealMode");
		msg_write("    n   Keine Kommentare");
	}else if (narg>=2){
		int nf=1;
		if ((narg>2)&&(arg[1][0]=='-')){
			msg_write("Parameter:");
			for (int i=1;i<strlen(arg[1]);i++){
				if (arg[1][i]=='r'){
					CurrentAsmMetaInfo=new sAsmMetaInfo;
					CurrentAsmMetaInfo->Mode16=true;
					msg_write("	-RealMode");
				}else if (arg[1][i]=='n'){
					ParamNoComments=true;
					msg_write("	-NoComments");
				}else
					msg_write("	-UNBEKANNT!");
			}
			nf++;
		}
		CFile *f=new CFile();
		f->Open(arg[nf]);
		f->SetBinaryMode(true);
		int start=0;
		int end=f->GetSize();
		if (narg>nf+1)
			start=Str2Int2(arg[nf+1]);
		if (narg>nf+2)
			end=Str2Int2(arg[nf+2]);
		f->SetPos(start,true);
		f->ReadBuffer(buffer,end-start);
		f->Close();


	/*	int h=open(arg[nf],0);
		int start=0;
		struct stat stat;
		fstat(h, &stat);
		int end=stat.st_size;
		if (narg>nf+1)
			start=Str2Int2(arg[nf+1]);
		if (narg>nf+2)
			end=Str2Int2(arg[nf+2]);
		read(h,buffer,start);
		read(h,buffer,end-start);
		close(h);*/
		msg_write(Opcode2Asm(buffer,end-start,!ParamNoComments));
	}
	msg_end();
	return 0;
}
