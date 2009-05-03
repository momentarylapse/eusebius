#include <iostream>
#include <stdlib.h>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

char _if[512],_of[512];
int _bs,_count,_skip,_seek;
bool notrunc;

char buffer[65536];

int Str2Int(char *str)
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

bool strsta(char *str,char *start)
{
	for (int i=0;i<strlen(start);i++)
		if (start[i]!=str[i])
			return false;
	return true;
}

int main(int argc, char *argv[])
{
	int i,ih,oh;
	_bs=512;
	_count=-1;
	_seek=0;
	notrunc=false;
	strcpy(_if,"");
	strcpy(_of,"");

	// Parameter
	for (i=1;i<argc;i++){
		if (strsta(argv[i],"if="))
			strcpy(_if,&argv[i][3]);
		else if (strsta(argv[i],"of="))
			strcpy(_of,&argv[i][3]);
		else if (strsta(argv[i],"bs="))
			_bs=Str2Int(&argv[i][3]);
		else if (strsta(argv[i],"count="))
			_count=Str2Int(&argv[i][6]);
		else if (strsta(argv[i],"seek="))
			_seek=Str2Int(&argv[i][5]);
		else if (strsta(argv[i],"skip="))
			_skip=Str2Int(&argv[i][5]);
		else if (strsta(argv[i],"conv=")){
			if (strcmp(&argv[i][5],"notrunc")==0)
				notrunc=true;
		}
	}

	// Quelle oeffnen
	ih=open(_if,O_RDONLY);
	if (ih<0){
		printf("if=%s nicht gefunden!",_if);
		return 1;
	}
	_setmode(ih,_O_BINARY);

	// Ziel oeffnen
	if (notrunc)
		oh=open(_of,O_RDWR | O_CREAT,_S_IREAD | _S_IWRITE);
	else
		oh=open(_of,O_RDWR | O_CREAT | O_TRUNC,_S_IREAD | _S_IWRITE);
		//oh=creat(_of,_S_IREAD | _S_IWRITE);
	if (oh<0){
		printf("of=%s nicht beschreibbar!",_if);
		return 1;
	}
	_setmode(oh,_O_BINARY);

	// zu ueberspringender Anfang, Quelle
	for (i=0;i<_skip;i++)
		read(ih,buffer,_bs);

	// zu ueberspringender Anfang, Ziel
	//lseek(oh,_bs*_seek,SEEK_SET);
	for (i=0;i<_seek;i++)
		read(oh,buffer,_bs);

	// das eigentliche Kopieren
	int num_blocks_written=0,r;
	while(true){
		_count--;
		if (_count==-1)
			break;
		r=read(ih,buffer,_bs);
		write(oh,buffer,r);
		if (r<_bs)
			break;
		num_blocks_written++;
		r=0;
	}
	close(ih);
	close(oh);

	if (r>0)
		printf("%d * %d + %d byte kopiert",num_blocks_written,_bs,r);
	else
		printf("%d * %d byte kopiert",num_blocks_written,_bs);
	return 0;
}
