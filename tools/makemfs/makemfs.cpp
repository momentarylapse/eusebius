#ifdef WIN32
	#define MFS_OS_WINDOWS
#else
	#define MFS_OS_LINUX
#endif


#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

#ifdef MFS_OS_WINDOWS
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
#endif
#ifdef MFS_OS_LINUX
	#include <unistd.h>
	#include <dirent.h>
#endif


/*#include <iostream>
#include <stdlib.h>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>*/

using namespace std;

char in_dir[512],out_file[512];

#define ClusterSize		2048
int BufferUsed;
char Buffer[ClusterSize];

#define MAX_FILES		128
int NumFiles;
struct sFile{
	char Name[256];
	char AbsoluteName[1024];
	int Size;
	bool IsDir;
};
sFile File[MAX_FILES];

void FindFiles(char *dir)
{
#ifdef MFS_OS_WINDOWS
	_finddata_t t;
	char _dir[512],str[512];
	strcpy(_dir,dir);
	if (_dir[strlen(_dir)-1]!='\\')
		strcat(_dir,"\\");
	strcpy(str,_dir);
	strcat(str,"*");
	int handle=_findfirst(str,&t);
	int e=handle;
	NumFiles=0;
	while(e>=0){
		if ((strcmp(t.name,".")!=0)&&(strcmp(t.name,"..")!=0)){
			strcpy(File[NumFiles].Name,t.name);
			strcpy(File[NumFiles].AbsoluteName,_dir);
			strcat(File[NumFiles].AbsoluteName,t.name);
			File[NumFiles].IsDir=(t.attrib==_A_SUBDIR);
			File[NumFiles].Size=t.size;
			NumFiles++;
		}
		e=_findnext(handle,&t);
	}
#endif
#ifdef MFS_OS_LINUX
	char _dir[512];
	strcpy(_dir,dir);
	if (_dir[strlen(_dir)-1]!='/')
		strcat(_dir,"/");

	DIR *p_dir;
	p_dir=opendir(_dir);
	struct dirent *dn;
	dn=readdir(p_dir);
	while(dn){
		if ((strcmp(dn->d_name,".")!=0)&&(strcmp(dn->d_name,"..")!=0)){
			strcpy(File[NumFiles].Name,dn->d_name);
			strcpy(File[NumFiles].AbsoluteName,_dir);
			strcat(File[NumFiles].AbsoluteName,dn->d_name);
			File[NumFiles].IsDir=false;
			int handle=open(File[NumFiles].AbsoluteName,0);
			struct stat stat;
			fstat(handle, &stat);
			File[NumFiles].Size=stat.st_size;
			NumFiles++;
		}
		dn=readdir(p_dir);
	}
#endif

	// sortieren
	sFile temp;
	for (int i=0;i<NumFiles-1;i++)
		for (int j=i+1;j<NumFiles;j++)
			if (strcmp(File[i].Name,File[j].Name)>0){
				temp=File[i];
				File[i]=File[j];
				File[j]=temp;
			}
}

void EmptyBuffer()
{
	for (int i=0;i<ClusterSize;i++)
		Buffer[i]=0;
	BufferUsed=0;
}

void WriteBool(bool b)
{
	*(bool*)&Buffer[BufferUsed]=b;
	BufferUsed++;
}

void WriteInt(int i)
{
	*(int*)&Buffer[BufferUsed]=i;
	BufferUsed+=4;
}

void WriteStr(char *str)
{
	WriteInt(strlen(str));
	strcpy(&Buffer[BufferUsed],str);
	BufferUsed+=strlen(str);
}

int main(int argc, char *argv[])
{
	int i;
	if (argc<3)
		printf("Usage:  makemfs <out-file> <in-dir>\n");
	else{
		strcpy(out_file,argv[1]);
		strcpy(in_dir,argv[2]);
		printf("Verzeichnis: %s\n",in_dir);
		printf("Image-Datei: %s\n",out_file);
		FindFiles(in_dir);
		printf("NumFiles: %d\n",NumFiles);

#ifdef MFS_OS_WINDOWS
		int h=creat(out_file,_S_IREAD | _S_IWRITE);
		_setmode(h,_O_BINARY);
#endif
#ifdef MFS_OS_LINUX
		int h=creat(out_file,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif

		// create root directory
		EmptyBuffer();
		WriteInt(NumFiles);
		int FileOffset=1;
  		for (i=0;i<NumFiles;i++){
  			WriteStr(File[i].Name);
  			WriteBool(File[i].IsDir);
			WriteInt(File[i].Size);
  			WriteInt(1); // number of file-parts
  			WriteInt(FileOffset); // offset
  			int s=(File[i].Size-1)/ClusterSize+1;
  			WriteInt(s); // number of clusters
  			printf("\t%s\t\t%d\t\t%d\t\t%d\n",File[i].Name,File[i].Size,FileOffset,s);
  			FileOffset+=s;
		}
		write(h,Buffer,ClusterSize);
		// files
  		for (i=0;i<NumFiles;i++){
			int hf=open(File[i].AbsoluteName,0);
#ifdef MFS_OS_WINDOWS
			_setmode(hf,_O_BINARY);
#endif
  			int s=(File[i].Size-1)/ClusterSize;
			for (int c=0;c<s;c++){
				int r=read(hf,Buffer,ClusterSize);
				if (r<ClusterSize)
					printf("      Fehler beim Lesen %s %d/%d  %d\n",File[i].Name,c,s,r);
				write(h,Buffer,ClusterSize);
			}
			EmptyBuffer();
			read(hf,Buffer,ClusterSize);
			write(h,Buffer,ClusterSize);
			close(hf);
		}
		close(h);
	}
	//system("PAUSE");
	return 0;
}
