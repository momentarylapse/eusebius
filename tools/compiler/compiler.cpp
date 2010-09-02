#include <stdlib.h>
#include "script/script.h"
//#include "dasm.h"
#include "file/msg.h"

CScript *script;

int main(int narg,char *arg[])
{
	bool ParamExecute=false;
	bool ParamBinary=false;
	bool ParamShow=false;
	bool ParamDasm=false;

	msg_init();
	if (narg<2){
		msg_write("Verwendung:    compiler [-Optionen] script_file [binary_file]");
		msg_write("Optionen:");
		msg_write("    e   Execute");
		msg_write("    o   Create Binary File");
		msg_write("    s   Show");
		msg_write("    d   DisAssemble");
	}else if (narg>=2){
		ScriptInit();
		if ((narg>2)&&(arg[1][0]=='-')){
			//msg_write("Parameter:");
			for (int i=1;i<strlen(arg[1]);i++){
				if (arg[1][i]=='e'){
					ParamExecute=true;
					//msg_write("	-Execute");
				}else if (arg[1][i]=='o'){
					ParamBinary=true;
					//msg_write("	-Binary");
					if (narg < 4){
						msg_error("-o benoetigt die Angabe einer Ausgabedatei!");
						return 1;
					}
				}else if (arg[1][i]=='s'){
					ParamShow=true;
					//msg_write("	-Show");
				}else if (arg[1][i]=='d'){
					ParamDasm=true;
					//msg_write("	-DisAsm");
				}else{
					msg_write("	-UNBEKANNTER PARAMETER!");
					return 1;
				}
			}
		}else{
			script=new CScript(arg[1]);
			msg_end();
			return 0;
		}
		msg_write(arg[2]);
		script=new CScript(arg[2]);
		if (script->Error){
			msg_end();
			return 1;
		}
		if (ParamShow)
			script->pre_script->Show();
		if (ParamDasm)
			msg_write(Opcode2Asm(script->Opcode,script->OpcodeSize));
		if (ParamBinary){
			CFile *f = FileCreate(arg[3]);
			f->SetBinaryMode(true);
			f->WriteStrL(script->Opcode,script->OpcodeSize);
			FileClose(f);
		}
		if (ParamExecute)
			script->Execute();
	}
	msg_end();
	return 0;
}
