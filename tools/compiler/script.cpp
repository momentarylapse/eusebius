/*----------------------------------------------------------------------------*\
| CScript                                                                      |
| -> C-like scripting system                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2007.03.19 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "script.h"
#include "msg.h"
#include "dasm.h"
#include <malloc.h>
#ifdef FILE_OS_WINDOWS
	#include <windows.h>
#endif
#ifdef FILE_OS_LINUX
	#include <stdarg.h>
#endif
	#include <stdio.h>

#include "00_config.h"
#ifdef _X_ALLOW_META_
	#include "meta.h"
#endif

char ScriptVersion[32]="0.4.1.4";

//#define ScriptDebug


int NumPublicScripts=0;
struct sPublicScript{
	char *filename;
	CScript *script;
}PublicScript[256];


char ScriptDirectory[512]="";


static int PreConstantNr,RTConstantNr;
// RunTime-Konstanten
#define MAX_RT_CONSTANTS	1024
static sPreConstant *RTConstant[MAX_RT_CONSTANTS];
static int NumRTConstants;



// ################################################################################################
//                                        Syntax-Analyse
// ################################################################################################

char Temp[1024];
int ExpKind;

int shift_right=0;

void stringout(char *str)
{
	msg_write(str);
}

static void so(char *str)
{
#ifdef ScriptDebug
	if (strlen(str)>256)
		str[256]=0;
	msg_write(str);
#endif
}

static void so(int i)
{
#ifdef ScriptDebug
	msg_write(i);
#endif
}

void right()
{
#ifdef ScriptDebug
	msg_right();
	shift_right+=2;
#endif
}

void left()
{
#ifdef ScriptDebug
	msg_left();
	shift_right-=2;
#endif
}

int s2i2(char *str)
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

static int SCurrentStack=-1;
#define SCRIPT_STR_STACK_DEPTH		64
static char SStackStr[SCRIPT_STR_STACK_DEPTH][4096];

char *Type2Str(CPreScript *s,sType *type)
{
	SCurrentStack=(SCurrentStack+1)%SCRIPT_STR_STACK_DEPTH;
	char *str=SStackStr[SCurrentStack];
	if (type)
		strcpy(str,string2("UNBEKANNTER TYP (%s)",type->Name));
	else
		strcpy(str,string2("UNBEKANNTER TYP (%d)",(int)type));
	for (int i=0;i<s->NumTypes;i++)
		if (type==s->Type[i])		strcpy(str,s->Type[i]->Name);
	if (type==&TypeUnknown)			strcpy(str,"[absichtlich unbekannter Typ]");
	return str;
}

char *Kind2Str(int kind)
{
	SCurrentStack=(SCurrentStack+1)%SCRIPT_STR_STACK_DEPTH;
	char *str=SStackStr[SCurrentStack];
	strcpy(str,string("UNBEKANNTE ART: ",i2s(kind)));
	if (kind==KindVarLocal)			strcpy(str,"lokale Variable");
	if (kind==KindVarGlobal)		strcpy(str,"globale Variable");
	if (kind==KindVarFunction)		strcpy(str,"Funktion als Variable");
	if (kind==KindVarExternal)		strcpy(str,"externe Programm-Variable");
	if (kind==KindConstant)			strcpy(str,"Konstante");
	if (kind==KindRefToConst)		strcpy(str,"Ref. auf Konstante");
	if (kind==KindFunction)			strcpy(str,"Funktion");
	if (kind==KindCompilerFunction)	strcpy(str,"Compiler-Funktion");
	if (kind==KindOperator)			strcpy(str,"Operator");
	if (kind==KindPrimitiveOperator)strcpy(str,"PRIMITIVER Operator");
	if (kind==KindCommand)			strcpy(str,"Befehl");
	if (kind==KindBlock)			strcpy(str,"Befehls-Block");
	if (kind==KindPointerShift)		strcpy(str,"Pointer-Verschiebung");
	if (kind==KindArray)			strcpy(str,"Array-Element");
	if (kind==KindPointerAsArray)	strcpy(str,"Pointer-Array-Element");
	if (kind==KindReference)		strcpy(str,"Adress-Operator");
	if (kind==KindDereference)		strcpy(str,"Dereferenzierung");
	if (kind==KindDerefPointerShift)strcpy(str,"deref. Pointer-Verschiebung");
	return str;
}

char *Operator2Str(CPreScript *s,int cmd)
{
	SCurrentStack=(SCurrentStack+1)%SCRIPT_STR_STACK_DEPTH;
	char *str=SStackStr[SCurrentStack];
	strcpy(str,string("UNBEKANNTER OPERATOR: ",i2s(cmd)));
	strcpy(str,string(	string("(",Type2Str(s,PreOperator[cmd].ParamType1),") "),
						PrimitiveOperator[PreOperator[cmd].PrimitiveID].Name,
						string(" (",Type2Str(s,PreOperator[cmd].ParamType2),")")));
	return str;
}

char *PrimitiveOperator2Str(int cmd)
{
	SCurrentStack=(SCurrentStack+1)%SCRIPT_STR_STACK_DEPTH;
	char *str=SStackStr[SCurrentStack];
	strcpy(str,string("UNBEKANNTER PRIMITIVER OPERATOR: ",i2s(cmd)));
	strcpy(str,PrimitiveOperator[cmd].Name);
	return str;
}

char *CompilerFunction2Str(int cmd)
{
	SCurrentStack=(SCurrentStack+1)%SCRIPT_STR_STACK_DEPTH;
	char *str=SStackStr[SCurrentStack];
	strcpy(str,string("UNBEKANNTE COMPILER-FUNKTION.: ",i2s(cmd)));
	for (int i=0;i<NumPreCommands;i++)
		if (PreCommand[i].Nr==(unsigned)cmd){
			strcpy(str,PreCommand[i].Name);
			break;
		}
	return str;
}

char *LinkNr2Str(CPreScript *s,int kind,int nr)
{
	SCurrentStack=(SCurrentStack+1)%SCRIPT_STR_STACK_DEPTH;
	char *str=SStackStr[SCurrentStack];
	strcpy(str,"UNBRAUCHBARE ART");
	if (kind==KindVarLocal)			strcpy(str,i2s(nr));
	if (kind==KindVarGlobal)		strcpy(str,i2s(nr));
	if (kind==KindVarFunction)		strcpy(str,i2s(nr));
	if (kind==KindVarExternal)		strcpy(str,PreExternalVar[nr].Name);
	if (kind==KindConstant)			strcpy(str,i2s(nr));
	if (kind==KindFunction)			strcpy(str,i2s(nr));
	if (kind==KindCompilerFunction)	strcpy(str,CompilerFunction2Str(nr));
	if (kind==KindOperator)			strcpy(str,Operator2Str(s,nr));
	if (kind==KindPrimitiveOperator)strcpy(str,PrimitiveOperator2Str(nr));
	if (kind==KindCommand)			strcpy(str,i2s(nr));
	if (kind==KindBlock)			strcpy(str,i2s(nr));
	if (kind==KindPointerShift)		strcpy(str,i2s(nr));
	if (kind==KindArray)			strcpy(str,i2s(nr));
	if (kind==KindPointerAsArray)	strcpy(str,i2s(nr));
	if (kind==KindReference)		strcpy(str,"(keine Parameter)");
	if (kind==KindDereference)		strcpy(str,"(keine Parameter)");
	if (kind==KindDerefPointerShift)strcpy(str,i2s(nr));
	return str;
}

void CPreScript::DoError(char *str,int nr)
{
	stringout("\n\n\n");
	stringout("------------------------       Error       -----------------------");
	Error=true;
	if (nr>=0)
		strcpy(ErrorMsg,string2("\"%s\" %s",Exp->Name[nr],str));
	else
		strcpy(ErrorMsg,str);
	strcpy(ErrorMsgExt[0],str);
	if (nr>=0)
		strcpy(ErrorMsgExt[1],string2("\"%s\" , line %d:%d",Exp->Name[nr],Exp->Line[nr],Exp->Column[nr]));
	else
		strcpy(ErrorMsgExt[1],"");
	ErrorLine=ErrorColumn=0;
	if (nr>=0){
		ErrorLine=Exp->Line[nr]-1;
		ErrorColumn=Exp->Column[nr]-1;
	}
	stringout(str);
	stringout(ErrorMsgExt[1]);
	stringout("------------------------------------------------------------------");
	stringout(Filename);
	stringout("\n\n\n");
}

void CScript::DoError(char *str,int nr)
{
	pre_script->DoError(str,nr);
	Error=true;
	ErrorLine=pre_script->ErrorLine;
	ErrorColumn=pre_script->ErrorColumn;
	/*stringout("\n\n\n");
	stringout("------------------------       Error       -----------------------");
	ParserError=true;
	Error=true;
	if (nr>=0)
		strcpy(ErrorMsg,string("\"",pre_script->Exp->Name[nr],"\" ",str));
	else
		strcpy(ErrorMsg,str);
	strcpy(ErrorMsgExt[0],str);
	if (nr>=0)
		strcpy(ErrorMsgExt[1],string("\"",pre_script->Exp->Name[nr],"\" , line ",i2s(pre_script->Exp->Line[nr])));
	else
		strcpy(ErrorMsgExt[1],"");
	ErrorLine=0;
	if (nr>=0){
		ErrorLine=Exp->Line[nr]-1;
		ErrorColumn=Exp->Column[nr]-1;
	}
	stringout(str);
	stringout(ErrorMsgExt[1]);
	stringout("------------------------------------------------------------------");
	stringout("\n\n\n");*/
}

void CScript::DoErrorLink(char *str,int nr)
{
	pre_script->DoError(str,nr);
	LinkerError=true;
	ErrorLine=pre_script->ErrorLine;
	ErrorColumn=pre_script->ErrorColumn;
	/*if (Error)	return;
	stringout("\n\n\n");
	stringout("------------------------       Error       -----------------------");
	LinkerError=true;
	Error=true;
	if (nr>=0)
		strcpy(ErrorMsg,string("\"",Exp->Name[nr],"\" ",str));
	else
		strcpy(ErrorMsg,str);
	strcpy(ErrorMsgExt[0],str);
	if (nr>=0)
		strcpy(ErrorMsgExt[1],string("\"",Exp->Name[nr],"\" , line ",i2s(Exp->Line[nr])));
	else
		strcpy(ErrorMsgExt[1],"");
	ErrorLine=0;
	if (nr>=0)
		ErrorLine=Exp->Line[nr]-1;
	stringout(str);
	stringout(ErrorMsgExt[1]);
	stringout("------------------------------------------------------------------");
	stringout("\n\n\n");*/
}

bool CPreScript::isNumber(char c)
{
	if ((c>=48)&&(c<=57))
		return true;
	return false;
}

bool CPreScript::isLetter(char c)
{
	if ((c>='a')&&(c<='z'))
		return true;
	if ((c>='A')&&(c<='Z'))
		return true;
	if ((c=='_'))
		return true;
	// Umlaute
#ifdef HUI_OS_WINDOWS
	// Windows-Zeichensatz
	if ((c=='_')||(c==-28)||(c==-10)||(c==-4)||(c==-33)||(c==-60)||(c==-42)||(c==-36))
		return true;
#endif
#ifdef HUI_OS_LINUX
	// Linux-Zeichensatz??? testen!!!!
#endif
	return false;
}

bool CPreScript::isSpacing(char c)
{
	if ((c==' ')||(c=='\t')||(c=='\n'))
		return true;
	return false;
}

bool CPreScript::isSign(char c)
{
	if ((c=='.')||(c==':')||(c==',')||(c==';')||(c=='+')||(c=='-')||(c=='*')||(c=='%')||(c=='/')||(c=='=')||(c=='<')||(c=='>')||(c=='\''))
		return true;
	if ((c=='(')||(c==')')||(c=='{')||(c=='}')||(c=='&')||(c=='|')||(c=='!')||(c=='[')||(c==']')||(c=='\"')||(c=='\\')||(c=='#')||(c=='?')||(c=='$'))
		return true;
	return false;
}

int CPreScript::GetKind(char c)
{
	if (isNumber(c))
		return ExpKindNumber;
	else if (isLetter(c))
		return ExpKindLetters;
	else if (isSpacing(c))
		return ExpKindSpacing;
	else if (isSign(c))
		return ExpKindSign;
	else if (c==0)
		return -1;

	char str[8];
	str[0]=c;
	str[1]=0;
	strcpy(Exp->Name[Exp->NumExps],string("   ",str," (",d2h(str,1),")   "));
	Exp->Line[Exp->NumExps]=Exp->TempLine;
	Exp->Column[Exp->NumExps]=Exp->TempColumn;
	DoError("evil character found!",Exp->NumExps);
	return -1;
}

static int CommentLevel;

void CPreScript::NextExp(char *Buffer)
{
	if (Exp->NumExps==0)
		Exp->BufferUsed=0;
	else
		Exp->BufferUsed+=strlen(Exp->Name[Exp->NumExps-1])+1;
	Exp->Name[Exp->NumExps]=&Exp->Buffer[Exp->BufferUsed];

	int i;
	// Platzhalter ueberspringen bis zum ersten wichtigen Zeichen
	for (i=0;i<1024;i++){
		if (Buffer[BufferPos]==0){
			strcpy(Temp,"");
			return;
		}
		ExpKind=GetKind(Buffer[BufferPos]);
		if (Error)	return;
		if (Buffer[BufferPos]=='\n'){ // Zeilenumbruch
			Exp->TempLine++;
			Exp->TempColumn=0;
		}
		if ((Buffer[BufferPos]=='/')&&(Buffer[BufferPos+1]=='/')){ // einzeiliger Kommentar
			for (int c=0;c<SCRIPT_MAX_FILE_SIZE;c++)
				if ((Buffer[BufferPos+c]=='\n')||(Buffer[BufferPos+c]==0)){
					Exp->TempLine++;
					Exp->TempColumn=0;
					BufferPos+=c;
					break;
				}
			ExpKind=ExpKindSpacing;
		}
		if ((Buffer[BufferPos]=='/')&&(Buffer[BufferPos+1]=='*')){ // mehrzeiliger Kommentar
			CommentLevel=1;
			int c=1;
			while(true){
				if (Buffer[BufferPos+c]=='\n'){
					Exp->TempLine++;
					Exp->TempColumn=0;
				}
				if ((Buffer[BufferPos+c]=='/')&&(Buffer[BufferPos+c+1]=='*'))
					CommentLevel++;
				if ((Buffer[BufferPos+c]=='*')&&(Buffer[BufferPos+c+1]=='/')){
					CommentLevel--;
					if (CommentLevel==0){
						BufferPos+=c+1;
						break;
					}
				}
				if ((Buffer[BufferPos+c]==0)||(BufferPos>=BufferLength)){
					DoError("comment exceeds line",-1);
					return;
				}
				c++;
				Exp->TempColumn++;
			}
			ExpKind=ExpKindSpacing;
		}
		if (ExpKind!=ExpKindSpacing)
			break;
		BufferPos++;
		Exp->TempColumn++;
	}
	int TempLength=0;
	int kind;
	//int ExpStart=BufferPos;
	bool StartedString=false;
	for (i=0;i<1024;i++){
		Temp[TempLength]=Buffer[BufferPos];
		/*char s[2];
		s[0]=Buffer[BufferPos];
		s[1]=0;
		msg_write(string(s," ",i2s(BufferPos)));*/
		kind=GetKind(Buffer[BufferPos]);
		// Ausdruecke, die mit Buchstaben anfangen, duerfen Zahlen enthalten
		if (ExpKind==ExpKindLetters)
			if (kind==ExpKindNumber){
				BufferPos++;
				TempLength++;
				Exp->TempColumn=0;
				continue;
			}
		// Doppel-Symbole:
		if (TempLength==0)
			if ((Buffer[BufferPos]=='=')&&(Buffer[BufferPos+1]=='=')|| // ==
				(Buffer[BufferPos]=='!')&&(Buffer[BufferPos+1]=='=')|| // !=
				(Buffer[BufferPos]=='<')&&(Buffer[BufferPos+1]=='=')|| // <=
				(Buffer[BufferPos]=='>')&&(Buffer[BufferPos+1]=='=')|| // >=
				(Buffer[BufferPos]=='+')&&(Buffer[BufferPos+1]=='=')|| // +=
				(Buffer[BufferPos]=='-')&&(Buffer[BufferPos+1]=='=')|| // -=
				(Buffer[BufferPos]=='*')&&(Buffer[BufferPos+1]=='=')|| // *=
				(Buffer[BufferPos]=='/')&&(Buffer[BufferPos+1]=='=')|| // /=
				(Buffer[BufferPos]=='+')&&(Buffer[BufferPos+1]=='+')|| // ++
				(Buffer[BufferPos]=='-')&&(Buffer[BufferPos+1]=='-')|| // --
				(Buffer[BufferPos]=='&')&&(Buffer[BufferPos+1]=='&')|| // &&
				(Buffer[BufferPos]=='|')&&(Buffer[BufferPos+1]=='|')|| // ||
				(Buffer[BufferPos]=='<')&&(Buffer[BufferPos+1]=='<')|| // <<
				(Buffer[BufferPos]=='>')&&(Buffer[BufferPos+1]=='>')|| // >>
				(Buffer[BufferPos]=='+')&&(Buffer[BufferPos+1]=='+')|| // ++
				(Buffer[BufferPos]=='-')&&(Buffer[BufferPos+1]=='-')|| // --
				(Buffer[BufferPos]=='-')&&(Buffer[BufferPos+1]=='>')){ // ->
				BufferPos++;
				TempLength++;
				Exp->TempColumn++;
				Temp[TempLength]=Buffer[BufferPos];
				BufferPos++;
				TempLength++;
				Exp->TempColumn++;
				break;
			}
		// strings in "" einschliessen!!!
		if (StartedString){
			if (Buffer[BufferPos]=='"'){
				Temp[TempLength]=Buffer[BufferPos];
				TempLength++;
				BufferPos++;
				Exp->TempColumn++;
				break;
			}else if (Buffer[BufferPos]=='\n'){
				DoError("string exceeds line",Exp->NumExps-1);
				return;
			}else{
				if (Buffer[BufferPos]=='\\'){
					BufferPos++;
					Exp->TempColumn++;
					if (Buffer[BufferPos]=='\\')
						Temp[TempLength]='\\';
					else if (Buffer[BufferPos]=='"')
						Temp[TempLength]='\"';
					else if (Buffer[BufferPos]=='n')
						Temp[TempLength]='\n';
					else if (Buffer[BufferPos]=='r')
						Temp[TempLength]='\r';
					else if (Buffer[BufferPos]=='t')
						Temp[TempLength]='\t';
					else if (Buffer[BufferPos]=='0')
						Temp[TempLength]='\0';
					else{
						DoError("unknown escape in string",Exp->NumExps-1);
						return;
					}
				}
				BufferPos++;
				TempLength++;
				Exp->TempColumn++;
				continue;
			}
		}
		if ((Buffer[BufferPos]=='"')&&(TempLength==0)){
			StartedString=true;
			BufferPos++;
			TempLength++;
			Exp->TempColumn++;
			continue;
		}
		// - vor Zahlen erlauben nach ( , [ ] = < >
		if (Buffer[BufferPos]=='-')
			if (GetKind(Buffer[BufferPos+1])==ExpKindNumber){
				char c=Buffer[BufferPos-1];
				if ((c=='(')||(c==',')||(c=='[')||(c==']')||(c=='=')||(c=='<')||(c=='>')){
					ExpKind=ExpKindNumber;
					BufferPos++;
					TempLength++;
					Exp->TempColumn++;
					continue;
				}
			}
		if (ExpKind==ExpKindNumber){
			// float nicht , oder . oder f abbrechen
			if (/*(Buffer[BufferPos]==',')||*/(Buffer[BufferPos]=='.')||(Buffer[BufferPos]=='f')){
				BufferPos++;
				TempLength++;
				Exp->TempColumn++;
				continue;
			}
			// Hex darf mit 0x... anfangen
			if (((TempLength==1)&&(Buffer[BufferPos]=='x')) || (strstr(Temp,"0x")&&(Buffer[BufferPos]>='a')&&(Buffer[BufferPos]<='f'))){
				BufferPos++;
				TempLength++;
				Exp->TempColumn++;
				continue;
			}
		}
		if (kind!=ExpKind) // bis zum Arten-Wechsel scannen
			break;
		if (kind==ExpKindSign){ // Zeichen sind nur 1 char lang!
			BufferPos++;
			TempLength++;
			Exp->TempColumn++;
			break;
		}
		BufferPos++;
		TempLength++;
	}
	Temp[TempLength]=0;
	
	if (strcmp(Temp,"asm")==0){
		if (Buffer[BufferPos]!='{'){
			DoError("\"{\" expected at beginning of assembler block",Exp->NumExps-1);
			return;
		}
		BufferPos++;
		int asm_start=BufferPos;
		int asm_end=-1;
		AsmLine[NumAsms]=Exp->TempLine;
		for (int i=0;i<SCRIPT_MAX_FILE_SIZE;i++){
			if (Buffer[BufferPos+i]=='}'){
				asm_end=asm_start+i-1;
				break;
			}
			if (Buffer[BufferPos+i]==0)
				break;
			if (Buffer[BufferPos+i]=='\n'){
				Exp->TempLine++;
				Exp->TempColumn=0;
			}
			Exp->TempColumn++;
		}
		if (asm_end<0){
			DoError("\"}\" expected after assembler block",Exp->NumExps-1);
			return;
		}
		AsmBlock[NumAsms]=new char[asm_end-asm_start+1];
		memcpy(AsmBlock[NumAsms],&Buffer[asm_start],asm_end-asm_start);
		AsmBlock[NumAsms][asm_end-asm_start]=0;
		//msg_error(AsmBlock[NumAsms]);
		NumAsms++;
		BufferPos=asm_end+2;
	}
	//msg_write(Temp);
}

bool IsIfDefed(int &num_ifdefs,bool *defed)
{
	for (int i=0;i<num_ifdefs;i++)
		if (!defed[i])
			return false;
	return true;
}

static CScript *cur_script;
void CreateAsmMetaInfo(CPreScript* ps)
{
	//msg_error("zu coden: CreateAsmMetaInfo");
	if (!ps->AsmMetaInfo){
		ps->AsmMetaInfo=new sAsmMetaInfo;
		sAsmMetaInfo *m=(sAsmMetaInfo*)ps->AsmMetaInfo;
		m->NumLabels=0;
		m->NumWantedLabels=0;
		m->NumDatas=0;
		m->NumBitChanges=0;
		m->Opcode=&cur_script->Opcode[0];
		m->Mode16=false;
		m->CodeOrigin=0;
		m->NumGlobalVars=&(ps->RootOfAllEvil.NumVars);
		if (ps->FlagOverwriteVariablesOffset)
			m->GlobalVarsOffset=ps->VariablesOffset;
		else
			m->GlobalVarsOffset=int(&cur_script->Stack[0]);
		m->GlobalVarNames=&ps->RootOfAllEvil.VarName[0][0];
		m->GlobalVarOffset=&ps->RootOfAllEvil.VarOffset[0];
		m->GlobalVarNameSize=SCRIPT_MAX_NAME;
	}
}

void CPreScript::AddIncludeData(CScript *s)
{
	Include[NumIncludes++]=s;
	CPreScript *ps=s->pre_script;
	int i;
	for (i=0;i<ps->NumDefines;i++){
		Define[NumDefines+i]=ps->Define[i];
		OwnDefine[NumDefines+i]=false;
	}
	NumDefines+=ps->NumDefines;
	for (i=0;i<ps->NumStructs-NumPreStructs;i++)
		Struct[NumStructs+i]=ps->Struct[i+NumPreStructs];
	NumStructs+=ps->NumStructs-NumPreStructs;
	for (i=0;i<ps->NumTypes-NumPreTypes;i++)
		Type[NumTypes+i]=ps->Type[i+NumPreTypes];
	NumTypes+=ps->NumTypes-NumPreTypes;
}

enum{
	MacroInclude,
	MacroDefine,
	MacroIfdef,
	MacroIfndef,
	MacroEndif,
	MacroElse,
	MacroDisasm,
	MacroShow,
	MacroRule,
	MacroOs,
	MacroInitialRealmode,
	MacroVariablesOffset,
	MacroCodeOrigin,
	NumMacroNames
};

char MacroName[NumMacroNames][SCRIPT_MAX_NAME]=
{
	"include",
	"define",
	"ifdef",
	"ifndef",
	"endif",
	"else",
	"disasm",
	"show",
	"rule",
	"os",
	"initial_realmode",
	"variables_offset",
	"code_origin"
};

void CPreScript::MakeExps(char *Buffer,bool just_analyse)
{
	int i,NumIfDefs=0,l1,l2,l3,ln;
	bool IfDefed[1024];
	Exp=new exp_buffer;
	Exp->BufferUsed=0;
	BufferPos=0;
	Exp->TempLine=1;
	Exp->TempColumn=0;
	Exp->NumExps=0;
	char filename[256];
	CScript *include;
	while(true){
		int l=(Exp->NumExps==0)?0:Exp->TempLine;
		NextExp(Buffer);
		if (Error)	return;
		if (Temp[0]==0)
			break;

		if ((Exp->TempLine>l)&&(strcmp(Temp,"#")==0)){
			l=Exp->TempLine;
			so("# -Makro");
			NextExp(Buffer);

			int macro_no=-1;
			for (i=0;i<NumMacroNames;i++)
				if (strcmp(Temp,MacroName[i])==0)
					macro_no=i;

			switch(macro_no){
				case MacroInclude:
					NextExp(Buffer);
					if (!IsIfDefed(NumIfDefs,IfDefed))
						continue;
//					strcpy(filename,&Temp[1]);
					strcpy(filename,dir_from_filename(Filename));
					strcat(filename,&Temp[1]);
					filename[strlen(filename)-1]=0; // remove "
/*					for (l=strlen(Filename)-1;l>=0;l--)
						if ((Filename[l]=='/')||(Filename[l]=='\\')){
							char ttt[256];
							strcpy(ttt,filename);
							strcpy(filename,Filename);
							filename[l+1]=0;
							strcat(filename,ttt);
							//so(filename);
							break;
						}*/
					// ".." herauskuerzen
					strcpy(filename,filename_no_recursion(filename));
					/*for (l1=strlen(filename)-2;l1>=0;l1--)
						if ((filename[l1]=='.')&&(filename[l1+1]=='.')){
							for (l2=l1-2;l2>=0;l2--)
								if ((filename[l2]=='/')||(filename[l2]=='\\')){
									int ss=strlen(filename)+l2-l1-2;
									for (l3=l2;l3<ss;l3++)
										filename[l3]=filename[l3+l1-l2+2];
									filename[ss]=0;
									l1=l2;
									break;
								}
						}*/
					//msg_write(filename);

					so("lade Include-Datei");
					right();

					include=LoadScriptAsInclude(filename,just_analyse);

					left();
					if ((!include)||(include->Error)){
						DoError(string2("error in inluded file \"%s\":\n[ %s (line %d:) ]",filename,include->ErrorMsg,include->ErrorLine,include->ErrorColumn),Exp->ExpNr);
						return;
					}
					AddIncludeData(include);
					Exp->ExpNr++;
					break;
				case MacroDefine:
					Define[NumDefines]=new sDefine;
					OwnDefine[NumDefines]=true;
					// Source
					NextExp(Buffer);
					strcpy(Define[NumDefines]->Source,Temp);
					Define[NumDefines]->NumDests=0;
					// Dests
					int t;
					for (i=0;i<SCRIPT_MAX_DEFINE_DESTS;i++){
						t=BufferPos;
						NextExp(Buffer);
						if (Exp->TempLine>l){
							BufferPos=t;
							break;
						}
						strcpy(Define[NumDefines]->Dest[Define[NumDefines]->NumDests],Temp);
						Define[NumDefines]->NumDests++;
					}
					Exp->TempLine=l;
					NumDefines++;
					break;
				case MacroIfdef:
					NextExp(Buffer);
					IfDefed[NumIfDefs]=false;
					for (i=0;i<NumDefines;i++)
						if (strcmp(Temp,Define[i]->Source)==0){
							IfDefed[NumIfDefs]=true;
							break;
						}
					NumIfDefs++;
					break;
				case MacroIfndef:
					NextExp(Buffer);
					IfDefed[NumIfDefs]=true;
					for (i=0;i<NumDefines;i++)
						if (strcmp(Temp,Define[i]->Source)==0){
							IfDefed[NumIfDefs]=false;
							break;
						}
					NumIfDefs++;
					break;
				case MacroElse:
					if (NumIfDefs<1){
						strcpy(Exp->Name[Exp->NumExps],Temp);
						Exp->Line[Exp->NumExps]=Exp->TempLine;
						Exp->Column[Exp->NumExps]=Exp->TempColumn;
						DoError("\"#else\" found but no matching \"#ifdef\"",Exp->NumExps);
						return;
					}
					IfDefed[NumIfDefs-1]=!IfDefed[NumIfDefs-1];
					break;
				case MacroEndif:
					if (NumIfDefs<1){
						strcpy(Exp->Name[Exp->NumExps],Temp);
						Exp->Line[Exp->NumExps]=Exp->TempLine;
						Exp->Column[Exp->NumExps]=Exp->TempColumn;
						DoError("\"#endif\" found but no matching \"#ifdef\"",Exp->NumExps);
						return;
					}
					NumIfDefs--;
					break;
				case MacroRule:
					NextExp(Buffer);
					ln=-1;
					for (i=0;i<NumScriptLocations;i++)
						if (strcmp(ScriptLocation[i].Name,Temp)==0)
							ln=i;
					if (ln<0){
						strcpy(Exp->Name[Exp->NumExps],Temp);
						Exp->Line[Exp->NumExps]=Exp->TempLine;
						Exp->Column[Exp->NumExps]=Exp->TempColumn;
						DoError("unknown location in script rule",Exp->NumExps);
						return;
					}
					PreScriptRule[NumPreScriptRules].Location=ScriptLocation[ln].Location;
					NextExp(Buffer);
					PreScriptRule[NumPreScriptRules].Level=s2i(Temp);
					NextExp(Buffer);
					Temp[strlen(Temp)-1]=0;
					strcpy(PreScriptRule[NumPreScriptRules].Name,&Temp[1]);
					NumPreScriptRules++;
					break;
				case MacroDisasm:
					FlagDisassemble=true;
					break;
				case MacroShow:
					FlagShow=true;
					break;
				case MacroOs:
					FlagCompileOS=true;
					break;
				case MacroInitialRealmode:
					FlagCompileInitialRealMode=true;
					break;
				case MacroVariablesOffset:
					FlagOverwriteVariablesOffset=true;
					NextExp(Buffer);
					VariablesOffset=s2i2(Temp);
					break;
				case MacroCodeOrigin:
					NextExp(Buffer);
					CreateAsmMetaInfo(this);
					((sAsmMetaInfo*)AsmMetaInfo)->CodeOrigin=s2i2(Temp);
					break;
				default:
					strcpy(Exp->Name[Exp->NumExps],Temp);
					Exp->Line[Exp->NumExps]=Exp->TempLine;
					Exp->Column[Exp->NumExps]=Exp->TempColumn;
					DoError("unknown makro atfer \"#\"",Exp->NumExps);
					return;
			}
			continue;
		}

		bool defed=false;
		for (i=0;i<NumDefines;i++)
			if (strcmp(Temp,Define[i]->Source)==0){
				defed=true;
				for (int j=0;j<Define[i]->NumDests;j++){
					strcpy(Exp->Name[Exp->NumExps],Define[i]->Dest[j]);
					Exp->BufferUsed+=strlen(Define[i]->Dest[j])+1;
					Exp->Name[Exp->NumExps+1]=&Exp->Buffer[Exp->BufferUsed];
					Exp->Line[Exp->NumExps]=Exp->TempLine;
					Exp->Column[Exp->NumExps]=Exp->TempColumn;
					Exp->NumExps++;
				}
				break;
			}
		if (defed)
			continue;

		if (!IsIfDefed(NumIfDefs,IfDefed))
			continue;

		strcpy(Exp->Name[Exp->NumExps],Temp);
		Exp->Line[Exp->NumExps]=Exp->TempLine;
		Exp->Column[Exp->NumExps]=Exp->TempColumn;
		Exp->NumExps++;
	}
	if (NumIfDefs>0){
		DoError("\"#ifdef\" found but no matching \"#endif\"",Exp->NumExps);
		return;
	}
	Exp->ExpNr=0;
}

bool next_extern;

int CPreScript::AddVar(char *name,sType *type,sFunction *f)
{
	if ((next_extern)&&(f==&RootOfAllEvil)){
		for (int i=0;i<NumPreGlobalVars;i++)
			if (strcmp(name,PreGlobalVar[i].Name)==0){
				f->VarType[i]=type;
				return i;
			}
	}
	if (f->NumVars>=SCRIPT_MAX_VARS){
		if (f==&RootOfAllEvil)
			DoError("too many global variables",Exp->ExpNr);
		else
			DoError("too many local variabels",Exp->ExpNr);
		return -1;
	}
	so("                                AddVar");
	strcpy(f->VarName[f->NumVars],name);
	int s=type->Size+(4-type->Size%4)%4;
	if (f->VarSize>=0){ // als "echte" lokale Variable
		f->VarOffset[f->NumVars]=-f->VarSize-s;
		f->VarSize+=s;
	}else{ // als Parameter
		f->VarOffset[f->NumVars]=f->ParamSize;
		f->ParamSize+=s;
	}
	f->VarType[f->NumVars]=type;
	f->NumVars++;
	return f->NumVars-1;
}

// Konstanten

int CPreScript::AddConstant(sType *type)
{
	if (NumConstants>=SCRIPT_MAX_CONSTANTS){
		DoError("too many constants",Exp->ExpNr);
		return -1;
	}
	so("                                AddConstant");
	Constant[NumConstants]=new sVariable;
	Constant[NumConstants]->type=type;
	Constant[NumConstants]->data=new char[type->Size];
	Constant[NumConstants]->meta=NULL;
	NumConstants++;
	return NumConstants;
}


int CPreScript::AddBlock()
{
	if (NumBlocks>=SCRIPT_MAX_COMMAND_BLOCKS){
		DoError("too many command blocks",Exp->ExpNr);
		return -1;
	}
	Block[NumBlocks]=new sBlock;
	Block[NumBlocks]->Nr=NumBlocks;
	Block[NumBlocks]->NumCommands=0;
	Block[NumBlocks]->RootNr=-1;
	NumBlocks++;
	return NumBlocks-1;
}

// function

int CPreScript::AddFunction(char *name)
{
	if (NumFunctions>=SCRIPT_MAX_FUNCS){
		DoError("too many functions",Exp->ExpNr);
		return -1;
	}
	Function[NumFunctions]=new sFunction;
	strcpy(Function[NumFunctions]->Name,name);
	Function[NumFunctions]->Block=Block[AddBlock()];
	if (Error)	return-1;
	Function[NumFunctions]->NumParams=0;
	Function[NumFunctions]->NumVars=0;
	Function[NumFunctions]->VarSize=-1;
	Function[NumFunctions]->ParamSize=8; // Speicher-Platz fuer Ruecksprungadresse und eBP
	Function[NumFunctions]->Type=&TypeVoid;
	NumFunctions++;
	return NumFunctions-1;
}

void CPreScript::AddCommand()
{
	if (NumCommands>=SCRIPT_MAX_COMMANDS){
		DoError("too many commands in whole file",Exp->ExpNr);
		return;
	}
	Command[NumCommands]=new sCommand;
	Command[NumCommands]->ReturnLink.type=&TypeVoid;
	Command[NumCommands]->Kind=KindUnknown;
	Command[NumCommands]->NumParams=0;
	NumCommands++;
}

int CPreScript::WhichPrimitiveOperator(char *name)
{
	for (int i=0;i<NumPrimitiveOperators;i++)
		if (strcmp(name,PrimitiveOperator[i].Name)==0)	return i;
	return -1;
}

int CPreScript::WhichExternalVariable(char *name)
{
	for (int i=0;i<NumPreExternalVars;i++)
		if (strcmp(name,PreExternalVar[i].Name)==0)
			return PreExternalVar[i].Nr;

	return -1;
}

bool CPreScript::GetExistence(char *name,sFunction *f)
{
	int i;
	sFunction *lf=f;
	GetExistenceLink.Meta=NULL;
	GetExistenceLink.script=NULL;

	// erst die lokalen Variablen
	if (lf){
		for (i=0;i<lf->NumVars;i++)
			if (strcmp(lf->VarName[i],name)==0){
				GetExistenceLink.type=lf->VarType[i];
				GetExistenceLink.Nr=i;
				GetExistenceLink.Kind=KindVarLocal;
				return true;
			}
	}

	// dann die globalen Variablen (=lokale Variable in "RootOfAllEvil")
	lf=&RootOfAllEvil;
	for (i=0;i<lf->NumVars;i++)
		if (strcmp(lf->VarName[i],name)==0){
			GetExistenceLink.type=lf->VarType[i];
			GetExistenceLink.Nr=i;
			GetExistenceLink.Kind=KindVarGlobal;
			return true;
		}

	// dann die Funktionen
	for (i=0;i<NumFunctions;i++)
		if (strcmp(Function[i]->Name,name)==0){
			GetExistenceLink.type=Function[i]->Type;
			GetExistenceLink.Nr=i;
			GetExistenceLink.Kind=KindFunction;
			return true;
		}

	// dann die Compiler-Funktionen
	i=WhichCompilerFunction(name);
	if (i>=0){
		//GetExistenceLink.type=;
		GetExistenceLink.Nr=i;
		GetExistenceLink.Kind=KindCompilerFunction;
		return true;
	}

	// schliesslich die externen-Variablen
	i=WhichExternalVariable(name);
	if (i>=0){
		SetExternalVariable(i,GetExistenceLink);
		return true;
	}

	// die Operatoren
	i=WhichPrimitiveOperator(name);
	if (i>=0){
		GetExistenceLink.Nr=i;
		GetExistenceLink.Kind=KindPrimitiveOperator;
		return true;
	}

	// ganz, ganz am Schluss in den Include-Dateien (nur global)...
	for ( i=0;i<NumIncludes;i++){
		if (Include[i]->pre_script->GetExistence(name,NULL)){
			if (Include[i]->pre_script->GetExistenceLink.script) // nicht rekursiv!!!
				continue;
			//msg_error(string("\"",name,"\" in Include gefunden!"));
			memcpy(&GetExistenceLink,&(Include[i]->pre_script->GetExistenceLink),sizeof(sLinkData));
			GetExistenceLink.script=Include[i];
			return true;
		}
	}

	// Name nicht bekannt
	GetExistenceLink.type=&TypeUnknown;
	GetExistenceLink.Nr=0;
	GetExistenceLink.Kind=0;
	return false;
}

int CPreScript::WhichCompilerFunction(char *name)
{
	for (int i=0;i<NumPreCommands;i++)
		if (strcmp(name,PreCommand[i].Name)==0)
			return PreCommand[i].Nr;
	return -1;
}

void CPreScript::SetCompilerFunction(int CF,sCommand *Com)
{
// dem "Compiler" bekannte Funktionen
	Com->Kind=KindCompilerFunction;
	Com->LinkNr=CF;
	Com->NumParams=0;
	Com->ReturnLink.type=&TypeVoid;

	for (int i=0;i<NumPreCommands;i++)
		if (PreCommand[i].Nr==(unsigned)CF){
			Com->NumParams=PreCommand[i].NumParams;
			for (int p=0;p<Com->NumParams;p++)
				Com->ParamLink[p].type=PreCommand[i].Param[p].Type;
			Com->ReturnLink.type=PreCommand[i].ReturnType;
			break;
		}

}

void CPreScript::SetExternalVariable(int gv, sLinkData &link)
{
	link.Kind=KindVarExternal;
	link.Nr=gv;

	for (int i=0;i<NumPreExternalVars;i++)
		if ((unsigned)gv==PreExternalVar[i].Nr){
			link.type=PreExternalVar[i].Type;
			break;
		}
}

sType *CPreScript::GetConstantType(char *name)
{
	PreConstantNr=-1;
	RTConstantNr=-1;
	// vordefinierte Konstanten
	for (PreConstantNr=0;PreConstantNr<NumPreConstants;PreConstantNr++)
		if (strcmp(name,PreConstant[PreConstantNr].Name)==0)
			return PreConstant[PreConstantNr].Type;
	PreConstantNr=-1;
	// neu definierte Konstanten
	for (RTConstantNr=0;RTConstantNr<NumRTConstants;RTConstantNr++)
		if (strcmp(name,RTConstant[RTConstantNr]->Name)==0)
			return RTConstant[RTConstantNr]->Type;
	RTConstantNr=-1;
	if ((name[0]=='"')&&(name[strlen(name)-1]=='"'))
		return &TypeString;
	sType *type=&TypeInt;
	bool hex=(name[0]=='0')&&(name[1]=='x');
	for (unsigned int c=0;c<strlen(name);c++)
		if ((name[c]<'0')||(name[c]>'9'))
			if (hex){
				if ((c>=2)&&(name[c]<'a')&&(name[c]>'f'))
					return &TypeUnknown;
			}else if (name[c]=='.')
				type=&TypeFloat;
			else{
				if ((type!=&TypeFloat)||(name[c]!='f')) // f in floats erlauben
					if ((c!=0)||(name[c]!='-')) // Vorzeichen erlauben
						return &TypeUnknown;
			}
	return type;
}

sType *CPreScript::GetType(int &ie,bool force)
{
	sType *type=NULL;
	for (int i=0;i<NumTypes;i++)
		if (strcmp(Exp->Name[ie],Type[i]->Name)==0)
			type=Type[i];
	if (force){
		if (!type)
			DoError("unknown type",ie);
	}
	if (type)
		ie++;
	return type;
}

void CPreScript::AddType(sType **type)
{
	for (int i=0;i<NumTypes;i++)
		if (strcmp((*type)->Name,Type[i]->Name)==0){
			(*type)=Type[i];
			return;
		}
	Type[NumTypes]=new sType;
	(*Type[NumTypes])=(**type);
	strcpy(Type[NumTypes]->Name,(*type)->Name);
	so(string("AddType: ",Type[NumTypes]->Name));
	(*type)=Type[NumTypes];
	NumTypes++;
}

void CPreScript::GetOperandExtension(int &ie,sLinkData *Operand,sFunction *f)
{
	so("GetOperandExtension");
	int op=WhichPrimitiveOperator(Exp->Name[ie]);
	if ((strcmp(Exp->Name[ie],".")!=0)&&(strcmp(Exp->Name[ie],"[")!=0)&&(strcmp(Exp->Name[ie],"->")!=0)&&(op<0))
		return;
	right();
	sLinkData link,temp;
	if ((strcmp(Exp->Name[ie],".")==0)||(strcmp(Exp->Name[ie],"->")==0)){
		so("->Struktur");
		ie++;
		link.Kind=KindPointerShift;
		sType *type=Operand->type;
		if (strcmp(Exp->Name[ie-1],"->")==0){
			so("(dereferenzierende Struktur)");
			link.Kind=KindDerefPointerShift;
			if (!type->IsPointer){
				DoError("expression in front of \"->\" needs to be a pointer",ie);
				return;
			}
			type=type->SubType;
		}

		bool ok=false;
		for (int i=0;i<NumStructs;i++)
			if (type==Struct[i].RootType){
				for (int e=0;e<Struct[i].NumElements;e++)
					if (strcmp(Exp->Name[ie],Struct[i].Element[e].Name)==0){
						link.Nr=Struct[i].Element[e].Shift;
						link.type=Struct[i].Element[e].Type;
						ok=true;
						break;
					}
				break;
			}

		if (ok){
			ie++;
			temp=(*Operand);
			(*Operand)=link;
			Operand->Meta=new sLinkData;
			(*Operand->Meta)=temp;
		}else
			DoError(string("unknown element of ",Type2Str(this,type)),ie);
	}else if (strcmp(Exp->Name[ie],"[")==0){
		so("->Array");
		if ((Operand->type->ArrayLength<1)&&(!Operand->type->IsPointer)){
			DoError(string2("type \"%s\" is neither an array nor a pointer",Operand->type->Name),ie);
			return;
		}
		ie++;
		so(Operand->type->Name);
		if (Operand->type->IsPointer){
			link.Kind=KindPointerAsArray;
			so("  ->Pointer-Array");
		}else
			link.Kind=KindArray;
		// den Index aufloesen...
		sLinkData index=GetCommand(ie,0,-1,f);
		if (Error)	return;
		if (index.type!=&TypeInt){
			DoError("type of index for an array needs to be (int)",ie-1);
			return;
		}
		if (strcmp(Exp->Name[ie],"]")!=0){
			DoError("\"]\" expected after array index",ie);
			return;
		}
		ie++;
		temp=(*Operand);
		so(temp.type->Name);
		//so(temp.type->Name);
		(*Operand)=link;
		Operand->Meta=new sLinkData;
		Operand->type=temp.type->SubType;
		(*Operand->Meta)=temp;
		Operand->ParamLink=new sLinkData;
		(*Operand->ParamLink)=index;
	}else if (op>=0){
		for (int i=0;i<NumPreOperators;i++)
			if (PreOperator[i].PrimitiveID==(unsigned)op)
				if ((PreOperator[i].ParamType1==Operand->type)&&(PreOperator[i].ParamType2==&TypeVoid)){
					//DoError("Unaerer Operator",ie);
					so("  => unaerer Operator");
					so(LinkNr2Str(this,KindOperator,i));
					AddCommand();
					Command[NumCommands-1]->Kind=KindOperator;
					Command[NumCommands-1]->LinkNr=i;
					Command[NumCommands-1]->NumParams=1;
					Command[NumCommands-1]->ParamLink[0]=*Operand;
					Command[NumCommands-1]->ReturnLink.type=PreOperator[i].ReturnType;
					Operand->Kind=KindCommand;
					Operand->Nr=NumCommands-1;
					Operand->type=PreOperator[i].ReturnType;
					ie++;
					left();
					return;
				}
		left();
		return;
	}
	GetOperandExtension(ie,Operand,f);
	left();
}

sLinkData CPreScript::GetOperand(int &ie,int bracket_level,sFunction *f)
{
	sLinkData Operand;
	Operand.Meta=NULL;
	so("GetOperand");
	right();
	so(Exp->Name[ie]);
	int p;

	// ( -> eine Ebene tiefer und als Befehl zusammenfassen
	if (strcmp(Exp->Name[ie],"(")==0){
		ie++;
		Operand=GetCommand(ie,bracket_level+1,-1,f);
	}else if (strcmp(Exp->Name[ie],"&")==0){ // & -> Adress-Operator
		so("<Adress-Operator &>");
		ie++;
		Operand.Meta=new sLinkData;
		(*Operand.Meta)=GetOperand(ie,bracket_level,f);
		if (Error)	return Operand;
		Operand.Kind=KindReference;
		// neuen Typ als Pointer auf den Meta-Typ erstellen
		sType nt,*pt=&nt;
		nt.ArrayLength=0;
		nt.IsPointer=true;
		strcpy(nt.Name,string(Operand.Meta->type->Name,"*"));
		nt.Size=4;
		nt.SubType=Operand.Meta->type;
		AddType(&pt);
		Operand.type=pt;
		ie--;
	}else if (strcmp(Exp->Name[ie],"*")==0){ // * -> Dereferenzierung
		so("<Dereferenzierung *>");
		ie++;
		Operand.Meta=new sLinkData;
		(*Operand.Meta)=GetOperand(ie,bracket_level,f);
		if (Error)	return Operand;
		if (!Operand.Meta->type->IsPointer){
			DoError("only pointers can be dereferenced using \"*\"",ie-1);
			return Operand;
		}
		Operand.Kind=KindDereference;
		Operand.type=Operand.Meta->type->SubType;
		ie--;
	}else{

		// direkter Operand
		if (GetExistence(Exp->Name[ie],f)){
			char f_name[256];
			strcpy(f_name,Exp->Name[ie]);
			so(string("=> ",Kind2Str(GetExistenceLink.Kind)));
			Operand=GetExistenceLink;

			// Variablen werden direkt gelinkt

			// Operand ist selbst auszufuehren
			if ((GetExistenceLink.Kind==KindFunction)||(GetExistenceLink.Kind==KindCompilerFunction)){
				ie++;
				// Funktions-Variable?
				if (strcmp(Exp->Name[ie],"(")!=0){
					if (GetExistenceLink.Kind==KindFunction){
						so("Funktion als Variable!");
						Operand.Kind=KindVarFunction;
						Operand.type=&TypeVoid;
						Operand.Nr=GetExistenceLink.Nr;
					}else
						DoError("\"(\" expected in front of parameter list",ie);
					return Operand;
				}

				/*bool is_for=((GetExistenceLink.Kind==KindCompilerFunction)&&(GetExistenceLink.Nr==CommandFor));
				int cmd_for_1;

				if (is_for){
					AddCommand();
					cmd_for_1=NumCommands-1;
				}*/

				// Befehl erstellen
				AddCommand();
				int cmd=NumCommands-1;
				Command[cmd]->Kind=GetExistenceLink.Kind;
				Command[cmd]->LinkNr=GetExistenceLink.Nr;
				Command[cmd]->script=GetExistenceLink.script;
				Command[cmd]->ReturnLink.type=GetExistenceLink.type;
				int fnc=-1;
				if (GetExistenceLink.Kind==KindFunction){
					fnc=GetExistenceLink.Nr;
					if (Command[cmd]->script)
						Command[cmd]->NumParams=Command[cmd]->script->pre_script->Function[GetExistenceLink.Nr]->NumParams;
					else
						Command[cmd]->NumParams=Function[GetExistenceLink.Nr]->NumParams;
				}else{
					SetCompilerFunction(Command[cmd]->LinkNr,Command[cmd]);
				}

				so(Type2Str(this,Command[cmd]->ReturnLink.type));
				ie++;
				// Operand auf den Befehl linken
				Operand.Kind=KindCommand;
				Operand.Nr=cmd;
				Operand.type=Command[cmd]->ReturnLink.type;

				so(Command[cmd]->NumParams);

				// Parameter-Liste
				int np=0;
				sType *WantedType[SCRIPT_MAX_PARAMS];
				for (p=0;p<SCRIPT_MAX_PARAMS;p++){
					if (strcmp(Exp->Name[ie],")")==0)
						break;
					np++;
					// Parameter finden
					sLinkData Param;
					Param=GetCommand(ie,0,-1,f);
					if (Error)	return Operand;

					if ((Command[cmd]->Kind!=KindCompilerFunction)||(Command[cmd]->LinkNr!=CommandReturn))
					if (Param.type->ArrayLength>0){

						sLinkData Meta;
						Meta=Param;
						//memcpy(&Meta,&Param,sizeof(sLinkData));
						sType t,*pt=&t;
						strcpy(t.Name,string(Param.type->Name,"*"));
						t.Size=PointerSize;
						t.IsPointer=true;
						t.SubType=Param.type;
						AddType(&pt);
						Param.type=pt;
						Param.Kind=KindReference;
						Param.Meta=new sLinkData;
						(*Param.Meta)=Meta;
						so("C-Standart:  Arrays als Parameter werden referenziert!");
					}

					WantedType[p]=Command[cmd]->ParamLink[p].type;
					if (fnc>=0){
						if (Command[cmd]->script)
							WantedType[p]=Command[cmd]->script->pre_script->Function[fnc]->VarType[p];
						else
							WantedType[p]=Function[fnc]->VarType[p];
					}
					// Parameter linken
					Command[cmd]->ParamLink[p]=Param;

					if (strcmp(Exp->Name[ie],",")!=0){
						if (strcmp(Exp->Name[ie],")")==0)
							break;
						DoError("\",\" or \")\" expected after parameter for function",ie);
						return Operand;
					}
					ie++;
				}

				// Kompatibilitaet testen
				if (np!=Command[cmd]->NumParams){
					DoError(string2("function \"%s\" expects %d parameters, %d were found",f_name,Command[cmd]->NumParams,np),--ie);
					return Operand;
				}
				for (p=0;p<np;p++){

					// return-Typ der aktuellen Funktion anpassen
					if ((Command[cmd]->Kind==KindCompilerFunction)&&(Command[cmd]->LinkNr==CommandReturn))
						WantedType[p]=f->Type;

					// Type-Cast erforderlich und moeglich?
					sType *pt=Command[cmd]->ParamLink[p].type;
					sType *wt=WantedType[p];
					if ((pt!=wt)&&( (!wt->IsPointer)||(!pt->IsPointer) )){
						int tc=-1;
						for (int i=0;i<NumTypeCasts;i++)
							if ((TypeCast[i].Source==pt)&&(TypeCast[i].Dest==wt))
								tc=i;
						if (tc>=0){
							so("TypeCast");
							so(string2("Benoetige automatischen TypeCast (%s) -> (%s)",pt->Name,wt->Name));
							if (Command[cmd]->ParamLink[p].Kind==KindConstant){
								int n=Command[cmd]->ParamLink[p].Nr;
								memcpy( Constant[n]->data, TypeCast[tc].Func(Constant[n]->data), wt->Size );
								Constant[n]->type=wt;
								so("  ...Konstante wurde direkt gewandelt!");
							}else{
								//so("  ...keine Konstante: Wandel-Befehl wurde hinzugefuegt!");
								DoError(string2("parameter %d for function \"%s\" has type (%s), (%s) expected - type casting within commands not implemented yet (call Michi!)",p+1,f_name,pt->Name,wt->Name),--ie);
								return Operand;
							}
						}else{
							DoError(string2("parameter %d for function \"%s\" has type (%s), (%s) expected",p+1,f_name,pt->Name,wt->Name),--ie);
							return Operand;
						}
					}
				}
			}else if(GetExistenceLink.Kind==KindPrimitiveOperator){
				// unaerer Operator
				int _ie=ie;
				so("  => unitaerer Operator");
				ie++;
				sLinkData sub_command=GetOperand(ie,bracket_level,f);
				if (Error)	return Operand;
				bool found=false;
				for (int i=0;i<NumPreOperators;i++)
					if ((PreOperator[i].ParamType1==&TypeVoid)&&(PreOperator[i].ParamType2==sub_command.type)){
						AddCommand();
						Command[NumCommands-1]->Kind=KindOperator;
						Command[NumCommands-1]->LinkNr=i;
						Command[NumCommands-1]->NumParams=1;
						Command[NumCommands-1]->ParamLink[0]=sub_command;
						Command[NumCommands-1]->ReturnLink.type=PreOperator[i].ReturnType;
						Operand.Kind=KindCommand;
						Operand.Nr=NumCommands-1;
						Operand.type=PreOperator[i].ReturnType;
						so(Operator2Str(this,i));
						found=true;
						break;
					}
				if (!found){
					DoError("unknown unitary operator",_ie);
					return Operand;
				}
				return Operand;
			}
		}else{
			sType *t=GetConstantType(Exp->Name[ie]);
			if (t!=&TypeUnknown){
				so("=> Konstante");
				Operand.Kind=KindConstant;
				// Konstante als Parameter (ueber Umwege als Variable)
				Operand.type=t;
				AddConstant(t);
				Operand.Nr=NumConstants-1;
				if (PreConstantNr>=0){
					if (t->Size<=TypePointer.Size) // <= 4
						(*(char**)(Constant[NumConstants-1]->data))=PreConstant[PreConstantNr].Value; // direkter Inhalt
					else
						memcpy(Constant[NumConstants-1]->data,PreConstant[PreConstantNr].Value,t->Size); // verlinkter Inhalt
				}else{
					if (RTConstantNr>=0){
						if (t==&TypeInt)	(*(int*)(Constant[NumConstants-1]->data))=(int)RTConstant[RTConstantNr]->Value;
					}else{
						if (t==&TypeInt)	(*(int*)(Constant[NumConstants-1]->data))=s2i2(Exp->Name[ie]);
						if (t==&TypeFloat){
							for (unsigned int s=0;s<strlen(Exp->Name[ie]);s++)
								if (Exp->Name[ie][s]=='f')
									Exp->Name[ie][s]=0;
							(*(float*)(Constant[NumConstants-1]->data))=s2f(Exp->Name[ie]);
						}
						if (t==&TypeString){
							char *str=(char*)Constant[NumConstants-1]->data;
							unsigned int ui;
							for (ui=0;ui<strlen(Exp->Name[ie])-2;ui++)
								str[ui]=Exp->Name[ie][ui+1];
							str[ui]=0;
						}
					}
				}
			}else{
				DoError("unknown operand",ie);
				Operand.Kind=0;
				return Operand;
			}
		}

	}
	if (Error)
		return Operand;

	ie++;

	// Arrays, Strukturen aufloessen...
	GetOperandExtension(ie,&Operand,f);

	so(string("Operand endet mit ",Exp->Name[ie-1]));
	left();
	return Operand;
}

static sLinkData LastOperator;

// nur "primitiver" Operator -> keine Typen-Angaben
bool CPreScript::GetOperator(int &ie,sFunction *f)
{
	so("GetOperator");
	right();
	so(Exp->Name[ie]);
	int op=WhichPrimitiveOperator(Exp->Name[ie]);
	if (op>=0){

		// Befehl aus Operator
		AddCommand();
		Command[NumCommands-1]->Kind=KindPrimitiveOperator;
		Command[NumCommands-1]->LinkNr=op;
		// nur provisorisch (nur die Art des Zeichens, Parameter und deren Art erst am Ende von GetCommand!!!)

		// Befehl
		LastOperator.Kind=KindCommand;
		LastOperator.Nr=NumCommands-1;
		LastOperator.type=Command[NumCommands-1]->ReturnLink.type;

		ie++;
		left();
		return true;
	}
	left();
	return false;
}

void CPreScript::LinkMostImportantOperator(int &NumOperators,sLinkData *Operand,sLinkData *Operator)
{
	so("LinkMostImportantOperator");
	right();
// den wichtigsten Operator finden
	int i,mio=0;
	for (i=0;i<NumOperators;i++){
		so(string(i2s(PrimitiveOperator[Command[Operator[i].Nr]->LinkNr].Level),", ",i2s(Command[Operator[i].Nr]->LinkNr)));

		if (PrimitiveOperator[Command[Operator[i].Nr]->LinkNr].Level>PrimitiveOperator[Command[Operator[mio].Nr]->LinkNr].Level)
			mio=i;
	}
	so(mio);

// ihn linken
	// die Parameter linken
	Command[Operator[mio].Nr]->NumParams=2;
	Command[Operator[mio].Nr]->ParamLink[0]=Operand[mio];
	Command[Operator[mio].Nr]->ParamLink[1]=Operand[mio+1];
	// ihn selbst linken
	int po=Command[Operator[mio].Nr]->LinkNr,o=-1;
	sType *r=&TypeVoid;
	sType *p1=Operand[mio].type;
	sType *p2=Operand[mio+1].type;
	bool sa=false;
	if (p1==p2){
		for (i=0;i<NumStructs;i++)
			if (Struct[i].RootType==p1)
				sa=true;
	}
	bool ok=false;
	for (i=0;i<NumPreOperators;i++){
		bool sa2=((sa)&&(PreOperator[i].ParamType1==&TypeStruct)&&(PreOperator[i].ParamType2==&TypeStruct));
		// beide Operand-Typen muessen mit denen des Operators uebereinstimmen
		//   (fordert der Operator einen Pointer sind alle Arten von Pointern erlaubt!!!)
		//   (gleiches gilt fuer Strukturen gleicher Art)
		if ((unsigned)po==PreOperator[i].PrimitiveID)
			if ( (p1==PreOperator[i].ParamType1) || ((PreOperator[i].ParamType1==&TypePointer)&&(p1->IsPointer)) || sa2 )
				if ( (p2==PreOperator[i].ParamType2) || ((PreOperator[i].ParamType2==&TypePointer)&&(p2->IsPointer)) || sa2 ){
					o=i;
					r=PreOperator[i].ReturnType;
					ok=true;
					break;
				}
	}
	if (!ok){
		int tc1=-1,tc2=-1;
		for (i=0;i<NumPreOperators;i++){
			if ((unsigned)po==PreOperator[i].PrimitiveID){
				// 1.Parameter echt, 2.Parameter gewandelt
				for (int j=0;j<NumTypeCasts;j++)
					if (TypeCast[j].Source==p2)
						if ( (p1==PreOperator[i].ParamType1) || ((PreOperator[i].ParamType1==&TypePointer)&&(p1->IsPointer)) )
							if ( (TypeCast[j].Dest==PreOperator[i].ParamType2) || ((PreOperator[i].ParamType2==&TypePointer)&&(p2->IsPointer)) ){
								o=i;
								r=PreOperator[i].ReturnType;
								tc1=-1;
								tc2=j;
								ok=true;
								break;
							}
			}
		}
		if (ok){
			so("TypeCast");
			if (tc1>=0){
				sLinkData *Param=&Command[Operator[mio].Nr]->ParamLink[0];
				so(string("Benoetige automatischen TypeCast: ",TypeCast[tc1].Source->Name," -> ",TypeCast[tc1].Dest->Name));
				if (Param->Kind==KindConstant){
					memcpy(	Constant[Param->Nr]->data,TypeCast[tc1].Func(Constant[Param->Nr]->data),TypeCast[tc1].Dest->Size);
					Constant[Param->Nr]->type=TypeCast[tc1].Dest;
					so("  ...Konstante wurde direkt gewandelt!");
				}else{
					//so("  ...keine Konstante: Wandel-Befehl wurde hinzugefuegt!");
					DoError("type casting within commands not implemented yet (call Michi!)",Exp->ExpNr);
					return;
				}
			}
			if (tc2>=0){
				sLinkData *Param=&Command[Operator[mio].Nr]->ParamLink[1];
				so(string("Benoetige automatischen TyepCast: ",TypeCast[tc2].Source->Name," -> ",TypeCast[tc2].Dest->Name));
				if (Param->Kind==KindConstant){
					memcpy(	Constant[Param->Nr]->data,TypeCast[tc2].Func(Constant[Param->Nr]->data),TypeCast[tc2].Dest->Size);
					Constant[Param->Nr]->type=TypeCast[tc2].Dest;
					so("  ...Konstante wurde direkt gewandelt!");
				}else{
					//so("  ...keine Konstante: Wandel-Befehl wurde hinzugefuegt!");
					DoError("type casting within commands not implemented yet (call Michi!)",Exp->ExpNr);
					return;
				}
			}
		}else{
			DoError(string2("not an operator: (%s) %s (%s)",Type2Str(this,p1),PrimitiveOperator2Str(po),Type2Str(this,p2)),Exp->ExpNr-3);
			return;
		}
	}
	if (ok){
		Command[Operator[mio].Nr]->Kind=KindOperator;
		Command[Operator[mio].Nr]->LinkNr=o;
		Command[Operator[mio].Nr]->ReturnLink.type=Operator[mio].type=r;
	}

// ihn aus der Liste herauskuerzen
	Operand[mio]=Operator[mio];
	for (i=mio;i<NumOperators-1;i++){
		Operator[i]=Operator[i+1];
		Operand[i+1]=Operand[i+2];
	}
	NumOperators--;
	left();
}

sLinkData CPreScript::GetCommand(int &ie,int bracket_level,int shift,sFunction *f)
{
	so("GetCommand");
	right();
	int i,NumOperands=0;
	sLinkData Operand[SCRIPT_MAX_BLOCK_COMMANDS];
	sLinkData Operator[SCRIPT_MAX_BLOCK_COMMANDS];

	// ersten Operanden finden
	Operand[0]=GetOperand(ie,bracket_level,f);
	if (Error)	return Operand[0];
	NumOperands++;

	// nach if und while kein ";"
	bool ignore_rest=false;
	if (Operand[0].Kind==KindCommand)
		if (Command[Operand[0].Nr]->Kind==KindCompilerFunction)
			if ((Command[Operand[0].Nr]->LinkNr==CommandIf)||(Command[Operand[0].Nr]->LinkNr==CommandWhile)){
				ignore_rest=true;
				so("---------kein Rest-------------");
			}

	// je einen Operator und einen Operanden finden
	if (!ignore_rest)
		for (i=0;i<SCRIPT_MAX_BLOCK_COMMANDS;i++){
			if (GetOperator(ie,f)){
				Operator[i]=LastOperator;
				Operand[i+1]=GetOperand(ie,bracket_level,f);
				if (Error) return Operand[0];
				NumOperands++;
			}else{
				if (Error) return Operand[0];
				so("(kein weiterer Operator)");
				so(Exp->Name[ie]);
				ie++;
				break;
			}
		}


	// in jedem Schritt den wichtigsten Operator finden und herauskrzen
	int NumOperators=NumOperands-1;
	for (i=0;i<NumOperands-1;i++){
		LinkMostImportantOperator(NumOperators,Operand,Operator);
		if (Error) return Operand[0];
	}

	// der gesammte Befehl hat sich im dann im Operand[0] gesammelt
	// (ohne Operator war Operand[0] schon das einzig wichtige)

	so("-fertig");
	ie+=shift;
	so(string("Command endet mit ",Exp->Name[ie]));
	left();
	return Operand[0];
}

void CPreScript::GetCompleteCommand(sBlock *block)
{
	so("GetCompleteCommand");
	right();

	sType *tType=GetType(Exp->ExpNr,false);

	// Block?
	if (strcmp(Exp->Name[Exp->ExpNr],"{")==0){
		Exp->ExpNr++;
		so("<Block>");
		right();
		int NewBlock=AddBlock();
		if (Error)	return;

		block->NumCommands++;
		AddCommand();
		int cmd=NumCommands-1;
		block->Command[block->NumCommands-1]=cmd;
		Command[cmd]->Kind=KindBlock;
		Command[cmd]->LinkNr=NewBlock;

		Block[NewBlock]->RootNr=block->Nr;

		for (int i=0;i<SCRIPT_MAX_BLOCK_COMMANDS;i++){
			if (strcmp(Exp->Name[Exp->ExpNr],"}")==0){
				Exp->ExpNr++;
				left();
				so("</Block>");
				break;
			}
			GetCompleteCommand(Block[NewBlock]);
			if (Error)	return;
		}

	// Assembler-Block
	}else if (strcmp(Exp->Name[Exp->ExpNr],"asm")==0){
		Exp->ExpNr++;
		so("<Asm-Block>");
		block->NumCommands++;
		AddCommand();
		int cmd=NumCommands-1;
		block->Command[block->NumCommands-1]=cmd;
		Command[cmd]->Kind=KindCompilerFunction;
		Command[cmd]->LinkNr=CommandAsm;

	// lokale (Variablen-)Definitionen...
	// Art der Variablen
	}else if (tType){
		for (int l=0;l<SCRIPT_MAX_VARS;l++){
			bool is_pointer=false;
			sType *type=tType;
			if (strcmp(Exp->Name[Exp->ExpNr],"*")==0){
				so("Pointer");
				Exp->ExpNr++;
				is_pointer=true;
			}
			// Name der Variablen
			so("Lokale Variable:");
			char name[128];
			strcpy(name,Exp->Name[Exp->ExpNr]);
			Exp->ExpNr++;
			so(name);
			TestArrayDefinition(Exp->ExpNr,&type,is_pointer);
			if ((strcmp(Exp->Name[Exp->ExpNr],",")==0)||(strcmp(Exp->Name[Exp->ExpNr],";")==0)||(strcmp(Exp->Name[Exp->ExpNr],"=")==0))
				AddVar(name,type,Function[NumFunctions-1]);
			if (strcmp(Exp->Name[Exp->ExpNr],"=")==0){
				Exp->ExpNr--;
				sLinkData link=GetCommand(Exp->ExpNr,0,0,Function[NumFunctions-1]);
				if (Error)	return;
				if (link.Kind==KindCommand){
					block->NumCommands++;
					block->Command[block->NumCommands-1]=link.Nr;
				}
				Exp->ExpNr--;
			}
			if (strcmp(Exp->Name[Exp->ExpNr],";")==0){
				Exp->ExpNr++;
				break;
			}
			if ((strcmp(Exp->Name[Exp->ExpNr],",")!=0)&&(strcmp(Exp->Name[Exp->ExpNr],";")!=0)){
				DoError("\",\", \"=\" or \";\" expected after definition of local variable",Exp->ExpNr);
				return;
			}
			Exp->ExpNr++;
		}
		left();
		return;
	}else{

	// Befehle (der eigentliche Code!)
		sLinkData link=GetCommand(Exp->ExpNr,0,0,Function[NumFunctions-1]);
		if (Error)	return;
		if (link.Kind==KindCommand){
			block->Command[block->NumCommands++]=link.Nr;
			if (block->NumCommands>SCRIPT_MAX_BLOCK_COMMANDS)
				DoError("too many commands in current block",Exp->ExpNr);
			if (Command[link.Nr]->Kind==KindCompilerFunction){
				if (Command[link.Nr]->LinkNr==CommandWhile){
					Command[link.Nr]->SubLink1=block->NumCommands-1;
					GetCompleteCommand(block);
					Command[link.Nr]->SubLinkEnd=block->NumCommands;
				}
				if (Command[link.Nr]->LinkNr==CommandIf){
					Command[link.Nr]->SubLink1=block->NumCommands;
					GetCompleteCommand(block);
					if (strcmp(Exp->Name[Exp->ExpNr],"else")==0){
						Exp->ExpNr++;
						Command[link.Nr]->LinkNr=CommandIfElse;
						Command[link.Nr]->SubLink2=block->NumCommands;
						GetCompleteCommand(block);

						// Ende nachkorregieren
						sCommand *c=Command[block->Command[Command[link.Nr]->SubLink2]];
						if (c->Kind==KindCompilerFunction)
							if ((c->LinkNr==CommandIf)||(c->LinkNr==CommandIfElse)){
								c->SubLinkEnd=block->NumCommands;
							}
					}
					// Ende nachkorregieren
					sCommand *c=Command[block->Command[Command[link.Nr]->SubLink1]];
					if (c->Kind==KindCompilerFunction)
						if ((c->LinkNr==CommandIf)||(c->LinkNr==CommandIfElse)){
							c->SubLinkEnd=block->NumCommands;
						}
					Command[link.Nr]->SubLinkEnd=block->NumCommands;
				}
			}
		}
	}
	left();
}

// look for array definitions and correct pointers
void CPreScript::TestArrayDefinition(int &ie,sType **type,bool is_pointer)
{
	if (is_pointer){
		sType nt,*pt=&nt;
		nt.ArrayLength=0;
		nt.IsPointer=true;
		strcpy(nt.Name,string((*type)->Name,"*"));
		nt.Size=4;
		nt.SubType=(*type);
		AddType(&pt);
		(*type)=pt;
	}
	if (strcmp(Exp->Name[ie],"[")==0){
		sType nt,*pt=&nt;
		int array_size;
		int or_name_length=strlen((*type)->Name);
		so("-Array-");
		right();
		ie++;
		if (GetConstantType(Exp->Name[ie])==&TypeInt){
			array_size=s2i(Exp->Name[ie]);
			/*nt.IsPointer=false;
			nt.SubType=(*type);
			nt.ArrayLength=s2i(Exp->Name[ie]);
			nt.Size=(*type)->Size*nt.ArrayLength;
			strcpy(nt.Name,string((*type)->Name,"[",Exp->Name[ie],"]"));
			AddType(&pt);
			(*type)=pt;*/
		}else{
			DoError("only constants of type \"int\" allowed for size of arrays",ie);
			return;
		}
		ie++;
		if (strcmp(Exp->Name[ie],"]")!=0){
			DoError("\"]\" expected after array size",ie);
			return;
		}
		ie++;
		// recursion
		TestArrayDefinition(ie,type,false); // is_pointer=false, since pointers have been handled
		// create array
		nt.IsPointer=false;
		nt.SubType=(*type);
		nt.ArrayLength=array_size;
		nt.Size=(*type)->Size*array_size;
		strcpy(nt.Name,(*type)->Name);
		nt.Name[or_name_length]=0;
		strcat(nt.Name,string("[",i2s(array_size),"]",&(*type)->Name[or_name_length]));
		AddType(&pt);
		(*type)=pt;

		left();
	}
}

CScript *LoadScript(char *filename,bool is_public)
{
	//msg_write(string("Lade ",filename));
	if (strlen(filename)<1){
		msg_error("Keine Scriptdatei!");
		return NULL;
	}
	CScript *s=NULL;

	// public und private aus dem Speicher versuchen zu laden
	int ae=-1;
	for (int i=0;i<NumPublicScripts;i++)
		if (strcmp(PublicScript[i].filename,SysFileName(filename))==0)
			ae=i;
	if (ae>=0){
		if (is_public){
			s=PublicScript[ae].script;
			//msg_write("...pointer");
		}else{
			s=new CScript();
			memcpy(s,PublicScript[ae].script,sizeof(CScript));
			s->WaitingMode=WaitingModeNone;
			s->isCopy=true;
			s->OpcodeSize=0;
			s->Compiler();
			s->isPrivate=!is_public;
			s->ThisObject=-1;
			//msg_write("...kopiert (private)");
			//msg_error(string("Script existiert schon!!! ",filename));
		}
		return s;
	}

	s=new CScript(filename);
	s->isPrivate=!is_public;

	// nur public speichern
	if (is_public){
		PublicScript[NumPublicScripts].filename=new char[strlen(filename)+1];
		strcpy(PublicScript[NumPublicScripts].filename,SysFileName(filename));
		PublicScript[NumPublicScripts].script=s;
		NumPublicScripts++;
		//msg_write("...neu (public)");
	}//else
		//msg_write("...neu (private)");
	//msg_error(i2s(NumPublicScripts));
	return s;
}

CScript *LoadScriptAsInclude(char *filename,bool just_analyse)
{
	//msg_write(string("Include ",filename));
	// aus dem Speicher versuchen zu laden
	for (int i=0;i<NumPublicScripts;i++)
		if (strcmp(PublicScript[i].filename,SysFileName(filename))==0){
			//msg_write("...pointer");
			return PublicScript[i].script;
		}

	CScript *s=new CScript(filename,just_analyse);
	so("geladen....");
	//msg_write("...neu");
	s->isPrivate=false;

	// als public speichern
	PublicScript[NumPublicScripts].filename=new char[strlen(filename)+1];
	strcpy(PublicScript[NumPublicScripts].filename,SysFileName(filename));
	PublicScript[NumPublicScripts].script=s;
	NumPublicScripts++;

	return s;
}

CPreScript::CPreScript(char *filename,bool just_analyse)
{
	strcpy(Filename,filename);
	Error=false;

	Buffer=new char[SCRIPT_MAX_FILE_SIZE];
	FlagShow=FlagDisassemble=false;
	FlagCompileOS=FlagCompileInitialRealMode=FlagOverwriteVariablesOffset=false;
	AsmMetaInfo=NULL;
	NumIncludes=0;
	NumDefines=0;
	NumAsms=0;

	int i;
	NumTypes=NumPreTypes;
	for (i=0;i<NumPreTypes;i++)
		Type[i]=PreType[i];
	NumStructs=NumPreStructs;
	for (i=0;i<NumPreStructs;i++)
		Struct[i]=PreStruct[i];
	NumPreScriptRules=0;

	Error=!LoadToBuffer(string(ScriptDirectory,Filename),just_analyse);

	if (!Error)
		PreCompiler();

	if (!Error)
		Parser();

	delete(Buffer);
}

CScript::CScript(char *filename,bool just_analyse)
{
	cur_script=this;
	strcpy(Filename,filename);
	Error=false;
	isCopy=false;
	JustAnalyse=just_analyse;
	msg_write(string("loading script: ",Filename));
	msg_right();

	WaitingMode=WaitingModeNone;
	ThisObject=-1;

	pre_script=new CPreScript(Filename,just_analyse);
	if (pre_script->FlagShow)
		pre_script->Show();
	ParserError=Error=pre_script->Error;
	ErrorLine=pre_script->ErrorLine;
	ErrorColumn=pre_script->ErrorColumn;
	strcpy(ErrorMsg,pre_script->ErrorMsg);
	strcpy(ErrorMsgExt[0],pre_script->ErrorMsgExt[0]);
	strcpy(ErrorMsgExt[1],pre_script->ErrorMsgExt[1]);

	if ((!Error)&&(!JustAnalyse))
		Compiler();
	/*if (pre_script->FlagShow)
		pre_script->Show();*/
	if (pre_script->FlagDisassemble)
		msg_write(GetAsm(Opcode,OpcodeSize));

	msg_ok();
	msg_left();
}

// Datei auslesen (und Kommentare auslesen)
bool CPreScript::LoadToBuffer(char *filename,bool just_analyse)
{
	so("lese Skript aus");
	right();
// auslesen aus Datei
	CFile *f=new CFile();
	if (!f->Open(filename)){
		DoError("script file not loadable",-1);
		delete(f);
		so("...Abbruch");
		left();
		return false;
	}
	BufferLength=0;
	while (!f->Eof){
		Buffer[BufferLength]=f->ReadChar();
		BufferLength+=1;
	}
	Buffer[BufferLength]=0;
	BufferLength-=1;
	f->Close();
	delete(f);

// nach Ausdruecken durchsuchen
	MakeExps(Buffer,just_analyse);
	if (Error)	return false;

	left();
	return true;
}

// ... irgendwann einmal
void CPreScript::PreCompiler()
{
	if (Error)	return;
	so("PreCompiler");
	right();
	left();
}

// Text in Script-Daten wandeln
void CPreScript::Parser()
{
	if (Error)	return;
	so("Parser");
	right();
	int i,j,k;

	// Start-Werte erzeugen
	NumCommands=NumFunctions=NumConstants=0;
	RootOfAllEvil.NumVars=NumPreGlobalVars;
	for (i=0;i<NumPreGlobalVars;i++){
		strcpy(RootOfAllEvil.VarName[i],PreGlobalVar[i].Name);
		RootOfAllEvil.VarType[i]=PreGlobalVar[i].type;
	}
	NumBlocks=0;
	NumCommands=0;
	strcpy(RootOfAllEvil.Name,"RootOfAllEvil");

	// Syntax-Analyse
	sType *tType;
	Error=false;
	shift_right=0;
	Exp->ExpNr=0;
	for (i=0;i<1024;i++){
		next_extern=false;
		if (Exp->ExpNr>=Exp->NumExps)
			break;
		// globale Definitionen (enum, Variablen und Funktionen)

		// enum
		if (strcmp(Exp->Name[Exp->ExpNr],"enum")==0){
			so("enum");
			Exp->ExpNr++;
			if (strcmp(Exp->Name[Exp->ExpNr],"{")!=0){
				DoError("\"{\" expected after \"enum\"",Exp->ExpNr);
				return;
			}
			for (int num=0;num<256;num++){
				Exp->ExpNr++;
				RTConstant[NumRTConstants]=new sPreConstant;
				RTConstant[NumRTConstants]->Name=new char[SCRIPT_MAX_NAME];
				strcpy(RTConstant[NumRTConstants]->Name,Exp->Name[Exp->ExpNr]);
				RTConstant[NumRTConstants]->Type=&TypeInt;
				RTConstant[NumRTConstants]->Value=(char*)num;
				NumRTConstants++;
				Exp->ExpNr++;
				if ((strcmp(Exp->Name[Exp->ExpNr],",")!=0)&&(strcmp(Exp->Name[Exp->ExpNr],"}")!=0)){
					DoError("\",\" or \"}\" expected within enum",Exp->ExpNr);
					return;
				}
				if (strcmp(Exp->Name[Exp->ExpNr],"}")==0)
					break;
			}
			Exp->ExpNr++;
			if (strcmp(Exp->Name[Exp->ExpNr],";")!=0){
				DoError("\";\" expected after enum block",Exp->ExpNr);
				return;
			}
			Exp->ExpNr++;
			continue;
		}

		// struct
		if (strcmp(Exp->Name[Exp->ExpNr],"struct")==0){
			so("struct");
			Struct[NumStructs].NumElements=0;
			int _shift=0;
			Exp->ExpNr++;
			char name[256];
			strcpy(name,Exp->Name[Exp->ExpNr]);
			Exp->ExpNr++;
			if (strcmp(Exp->Name[Exp->ExpNr],":")==0){
				so("vererbung der struktur");
				Exp->ExpNr++;
				sType *ancestor=GetType(Exp->ExpNr,true);
				if (Error)	return;
				bool found=false;
				for (int ss=0;ss<NumStructs;ss++)
					if (Struct[ss].RootType==ancestor){
						// Elemente erben
						Struct[NumStructs].NumElements=Struct[ss].NumElements;
						for (int e=0;e<Struct[ss].NumElements;e++)
							Struct[NumStructs].Element[e]=Struct[ss].Element[e];
						_shift=Struct[ss].RootType->Size;
						found=true;
						break;
					}
				if (!found){
					DoError(string2("parental type in structure definition after \":\" has to be a structure, but (%s) is not",ancestor->Name),Exp->ExpNr);
					return;
				}
				//Exp->ExpNr++;
			}
			if (strcmp(Exp->Name[Exp->ExpNr],"{")!=0){
				DoError("\"{\" expected after struct",Exp->ExpNr);
				return;
			}
			for (int num=0;num<SCRIPT_MAX_STRUCT_ELEMENTS;num++){
				Exp->ExpNr++;
				if (strcmp(Exp->Name[Exp->ExpNr],"}")==0)
					break;
				tType=GetType(Exp->ExpNr,true);
				if (Error)	return;
				for (j=0;j<SCRIPT_MAX_VARS;j++){
					sStructElement *el=&Struct[NumStructs].Element[Struct[NumStructs].NumElements];
					el->Shift=_shift;
					bool is_pointer=false;
					sType *type=tType;
					if (strcmp(Exp->Name[Exp->ExpNr],"*")==0){
						Exp->ExpNr++;
						is_pointer=true;
					}
					el->Name=new char[strlen(Exp->Name[Exp->ExpNr])+1];
					strcpy(el->Name,Exp->Name[Exp->ExpNr]);
					Exp->ExpNr++;
					so(string2("Struct-Element: %s %s  Offset: %d",type->Name,el->Name,_shift));
					TestArrayDefinition(Exp->ExpNr,&type,is_pointer);
					el->Type=type;
					if ((strcmp(Exp->Name[Exp->ExpNr],",")!=0)&&(strcmp(Exp->Name[Exp->ExpNr],";")!=0)){
						DoError("\",\" or \";\" expected after struct element",Exp->ExpNr);
						return;
					}
					el->Shift=_shift;
					_shift+=type->Size;
					if (type->Size>=4)
						_shift=((_shift+3)/4)*4;
					Struct[NumStructs].NumElements++;
					if (strcmp(Exp->Name[Exp->ExpNr],";")==0)
						break;
					Exp->ExpNr++;
				}
			}
			Exp->ExpNr++;
			if (strcmp(Exp->Name[Exp->ExpNr],";")!=0){
				DoError("\";\" expected after struct block",Exp->ExpNr);
				return;
			}
			sType nt,*pt=&nt;
			nt.ArrayLength=0;
			nt.IsPointer=false;
			strcpy(nt.Name,name);
			nt.Size=_shift;
			nt.SubType=NULL;
			AddType(&pt);
			Struct[NumStructs].RootType=pt;
			NumStructs++;

			Exp->ExpNr++;
			continue;
		}

		// extern
		if (strcmp(Exp->Name[Exp->ExpNr],"extern")==0){
			next_extern=true;
			Exp->ExpNr++;
		}

		// Art der Definition
		tType=GetType(Exp->ExpNr,true);
		if (Error)	return;
		int te=Exp->ExpNr+1;
		if (strcmp(Exp->Name[Exp->ExpNr],"*")==0)
			te++;

		// Globale Variablen?
		if ((strcmp(Exp->Name[te],",")==0)||(strcmp(Exp->Name[te],";")==0)||(strcmp(Exp->Name[te],"[")==0)){
			for (j=0;j<SCRIPT_MAX_VARS;j++){
				bool is_pointer=false;
				sType *type=tType;
				char name[128];
				if (strcmp(Exp->Name[Exp->ExpNr],"*")==0){
					Exp->ExpNr++;
					is_pointer=true;
				}
				strcpy(name,Exp->Name[Exp->ExpNr]);
				Exp->ExpNr++;
				so(string("Globale Variable: ",name));
				TestArrayDefinition(Exp->ExpNr,&type,is_pointer);
				AddVar(name,type,&RootOfAllEvil);
				if ((strcmp(Exp->Name[Exp->ExpNr],",")!=0)&&(strcmp(Exp->Name[Exp->ExpNr],";")!=0)){
					DoError("\",\" or \";\" expected after definition of a global variable",Exp->ExpNr);
					return;
				}
				if (strcmp(Exp->Name[Exp->ExpNr],";")==0){
					Exp->ExpNr++;
					break;
				}
				Exp->ExpNr++;
			}
			continue;
		}

		// eigene Funktion?
		if (strcmp(Exp->Name[te],"(")==0){

			//msg_write("Funktion");
			//msg_write(Exp->Name[Exp->ExpNr]);

			so("-----------------------------------------------------");
			so("|  eigene Funktion:                                 |");
			so(Exp->Name[Exp->ExpNr]);
			right();
			int CurrentFunction=AddFunction(Exp->Name[Exp->ExpNr]);
			if (Error)	return;
			sBlock *CurrentBlock=Block[Function[CurrentFunction]->Block->Nr];
			Function[CurrentFunction]->Type=tType;
			if (tType->Size>4)
				Function[CurrentFunction]->ParamSize+=4;
			//Function[CurrentFunction]->VarSize+=tType->Size;
			Exp->ExpNr+=2;

			// Parameter...
			if (strcmp(Exp->Name[Exp->ExpNr],")")!=0)
			for (k=0;k<SCRIPT_MAX_PARAMS;k++){
				// aehnlich Variablen-Definition

				Function[NumFunctions-1]->NumParams++;

				// Parameter-Variablen-Art
				tType=GetType(Exp->ExpNr,true);
				if (Error)	return;
				bool is_pointer=false;
				char name[128];
				if (strcmp(Exp->Name[Exp->ExpNr],"*")==0){
					Exp->ExpNr++;
					is_pointer=true;
				}
				strcpy(name,Exp->Name[Exp->ExpNr]);
				so("Parameter-Variable:");
				so(name);
				Exp->ExpNr++;
				TestArrayDefinition(Exp->ExpNr,&tType,is_pointer);
				if (tType->ArrayLength>0){
					//DoError("Keine Arrays als Parameter erlaubt, benutzen Sie Pointer!",Exp->ExpNr);
					//return;
					TestArrayDefinition(Exp->ExpNr,&tType,true);
					so("C-Standart:   Array wurde in Pointer umgewandelt!!!!");
				}
				Function[NumFunctions-1]->VarType[Function[NumFunctions-1]->NumVars]=tType;
				AddVar(name,tType,Function[NumFunctions-1]);

				if (strcmp(Exp->Name[Exp->ExpNr],")")==0)
					break;

				if (strcmp(Exp->Name[Exp->ExpNr],",")!=0){
					DoError("\",\" or \")\" expected after parameter",Exp->ExpNr);
					return;
				}
				Exp->ExpNr++;
			}
			Exp->ExpNr++;
			Function[NumFunctions-1]->VarSize=0;

			if (strcmp(Exp->Name[Exp->ExpNr],"{")!=0){
				DoError("\"{\" expected after parameter list",Exp->ExpNr);
				return;
			}
			Exp->ExpNr++;

			// Anweisungen...
			for (k=0;k<2048;k++){

				// Ende der Funktion
				if (strcmp(Exp->Name[Exp->ExpNr],"}")==0)
					break;

				// normaler Befehl oder lokale Definition
				GetCompleteCommand(CurrentBlock);
				if (Error)	return;
			}

			// nur eine Funktion auf einmal definieren!
			left();
			so("|  eigene Funktion                                  |");
			so("-----------------------------------------------------");
			Exp->ExpNr++;
			continue;
		}
	}

	delete(Exp);
	so("-ok");
	left();
}

int LOffset,LOffsetMax,OCOffset[32][128],TaskReturnOffset;
int NumJumps[32],JumpSource[32][128],JumpDest[32][128],JumpCode[32][128];

enum{
	inNop,
	inPushEbp,
	inMovEbpEsp,
	inMovEspM,
	inMovEdxpi8Eax,
	inLeave,
	inRet,
	inRet4,
	inMovEaxM,
	inMovMEax,
	inMovEdxM,
	inMovMEdx,
	inMovAlM8,
	inMovAhM8,
	inMovBlM8,
	inMovBhM8,
	inMovClM8,
	inMovM8Al,
	inMovMEbp,
	inMovMEsp,
	inLeaEaxM,
	inLeaEdxM,
	inPush,
	inPushEax,
	inPushEdx,
	inPopEax,
	inPopEsp,
	inAndEaxM,
	inOrEaxM,
	inXorEaxM,
	inAddEaxM,
	inAddEdxM,
	inAddMEax,
	inSubEaxM,
	inSubMEax,
	inMulEaxM,
	inDivEaxM,
	inDivEaxEbx,
	inCmpEaxM,
	inCmpAlM8,
	inCmpM80,
	inSetzAl,
	inSetnzAl,
	inSetnleAl,
	inSetnlAl,
	inSetlAl,
	inSetleAl,
	inAndAlM8,
	inOrAlM8,
	inXorAlM8,
	inAddAlM8,
	inAddM8Al,
	inSubAlM8,
	inSubM8Al,
	inCallRel32,
	inJmpEax,
	inJmpC32,
	inJzC8,
	inJzC32,
	inLoadfM,
	inSavefM,
	inLoadfiM,
	inAddfM,
	inSubfM,
	inMulfM,
	inDivfM,
	inShrEaxCl,
	inShlEaxCl,
	NumAsmInstructions
};

struct sAsmInstruction{
	short oc_l8[3];		// fuer lokale Variablen (1b Offset)
//	short oc_l16[3];	// lokal (2b Offset)
	short oc_l32[3];	// lokal (4b Offset)
	short oc_g[3];		// global
	short oc_c[3];		// const
	short oc_dr[3];		// zu dereferenzierende Variable
	short oc_p;			// Abschluss
	short const_size;	// falls const
};


sAsmInstruction AsmInstruction[NumAsmInstructions]={
	//	local +8b		local +32b		global			const			reference[edx]	ending	const_size
	{	0x90,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // NOP
	{	0x55,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // PUSH eBP
	{	0x89,0xe5,-1,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // MOV eBP,eSP
	{	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		0xbc,-1,-1,		-1,-1,-1,		-1,		32	}, // MOV eSP,c32
	{	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		0x89,0x42,-1,	-1,-1,-1,		-1,		8	}, // MOV [eDX+c8],eAX
	{	0xc9,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // LEAVE
	{	0xc3,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // RET
	{	0xc2,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		0x04,	-1	}, // RET 4
	{	0x8b,0x45,-1,	0x8b,0x85,-1,	0xa1,-1,-1,		0xb8,-1,-1,		0x8b,0x02,-1,	-1,		32	}, // MOV eAX,m
	{	0x89,0x45,-1,	0x89,0x85,-1,	0xa3,-1,-1,		-1,-1,-1,		0x89,0x02,-1,	-1,		32	}, // MOV m,eAX
	{	0x8b,0x55,-1,	0x8b,0x95,-1,	0x8b,0x15,-1,	0xba,-1,-1,		-1,-1,-1,		-1,		32	}, // MOV eDX,m
	{	0x89,0x55,-1,	0x89,0x95,-1,	0x89,0x15,-1,	-1,-1,-1,		-1,-1,-1,		-1,		32	}, // MOV m,eDX
	{	0x8a,0x45,-1,	0x8a,0x85,-1,	0xa0,-1,-1,		0xb0,-1,-1,		0x8a,0x02,-1,	-1,		8	}, // MOV.b AL,m
	{	0x8a,0x65,-1,	0x8a,0xa5,-1,	0x8a,0x25,-1,	0xb4,-1,-1,		0x8a,0x02,-1,	-1,		8	}, // MOV.b AH,m
	{	0x8a,0x5d,-1,	0x8a,0x9d,-1,	0x8a,0x1d,-1,	0xb3,-1,-1,		0x8a,0x02,-1,	-1,		8	}, // MOV.b BL,m
	{	0x8a,0x7d,-1,	0x8a,0xbd,-1,	0x8a,0x3d,-1,	0xb7,-1,-1,		0x8a,0x02,-1,	-1,		8	}, // MOV.b BH,m
	{	0x8a,0x4d,-1,	0x8a,0x8d,-1,	0x8a,0x0d,-1,	0xb1,-1,-1,		0x8a,0x0a,-1,	-1,		8	}, // MOV.b CL,m
	{	0x88,0x45,-1,	0x88,0x85,-1,	0xa2,-1,-1,		-1,-1,-1,		0x88,0x02,-1,	-1,		8	}, // MOV.b m,AL
	{	0x89,0x6d,-1,	0x89,0xad,-1,	0x89,0x2d,-1,	-1,-1,-1,		-1,-1,-1,		-1,		32	}, // MOV eAX,eBP
	{	0x89,0x65,-1,	0x89,0xa5,-1,	0x89,0x25,-1,	-1,-1,-1,		-1,-1,-1,		-1,		32	}, // MOV eAX,eSP
	{	0x8d,0x45,-1,	0x8d,0x85,-1,	0x8d,0x05,-1,	-1,-1,-1,		0x8d,0x02,-1,	-1,		32	}, // LEA eAX,m
	{	0x8d,0x55,-1,	0x8d,0x95,-1,	0x8d,0x15,-1,	-1,-1,-1,		0x8d,0x12,-1,	-1,		32	}, // LEA eDX,m
	{	0xff,0x75,-1,	0xff,0xb5,-1,	0xff,0x35,-1,	0x68,-1,-1,		0xff,0x32,-1,	-1,		32	}, // PUSH m
	{	0x50,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // PUSH eAX
	{	0x52,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // PUSH eDX
	{	0x58,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // POP eAX
	{	0x5c,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // POP eSP
	{	0x23,0x45,-1,	0x23,0x85,-1,	0x23,0x05,-1,	0x25,-1,-1,		0x23,0x02,-1,	-1,		32	}, // AND eAX,m
	{	0x0b,0x45,-1,	0x0b,0x85,-1,	0x0b,0x05,-1,	0x0d,-1,-1,		0x0b,0x02,-1,	-1,		32	}, // OR eAX,m
	{	0x33,0x45,-1,	0x33,0x85,-1,	0x33,0x05,-1,	0x35,-1,-1,		0x33,0x02,-1,	-1,		32	}, // XOR eAX,m
	{	0x03,0x45,-1,	0x03,0x85,-1,	0x03,0x05,-1,	0x05,-1,-1,		0x03,0x02,-1,	-1,		32	}, // ADD eAX,m
	{	0x03,0x55,-1,	0x03,0x95,-1,	0x03,0x15,-1,	0x81,0xc2,-1,	-1,-1,-1,		-1,		32	}, // ADD edX,m
	{	0x01,0x45,-1,	0x01,0x85,-1,	0x01,0x05,-1,	-1,-1,-1,		0x01,0x02,-1,	-1,		-1	}, // ADD m,eAX
	{	0x2b,0x45,-1,	0x2b,0x85,-1,	0x2b,0x05,-1,	0x2d,-1,-1,		0x2b,0x02,-1,	-1,		32	}, // SUB eAX,m
	{	0x29,0x45,-1,	0x29,0x85,-1,	0x29,0x05,-1,	-1,-1,-1,		0x29,0x02,-1,	-1,		-1	}, // SUB m,eAX
	{	0x0f,0xaf,0x45,	0x0f,0xaf,0x85,	0x0f,0xaf,0x05,	0x69,0xc0,-1,	0x0f,0xaf,0x02,	-1,		32	}, // MUL eAX,m
	{	0xf7,0x7d,-1,	0xf7,0xbd,-1,	0xf7,0x3d,-1,	-1,-1,-1,		0xf7,0x3a,-1,	-1,		32	}, // DIV eAX,m
	{	0xf7,0xfb,-1,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // DIV eAX,eBX
	{	0x3b,0x45,-1,	0x3b,0x85,-1,	0x3b,0x05,-1,	0x3d,-1,-1,		0x3b,0x02,-1,	-1,		32	}, // CMP eAX,m
	{	0x3a,0x45,-1,	0x3a,0x85,-1,	0x3a,0x05,-1,	0x3c,-1,-1,		0x3a,0x02,-1,	-1,		8	}, // CMP.b AL,m
	{	0x80,0x7d,-1,	0x80,0xbd,-1,	0x80,0x3d,-1,	-1,-1,-1,		0x80,0x3a,-1,	0x00,	8	}, // CMP.b m,0
	{	0x0f,0x94,0xc0,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // SETZ AL
	{	0x0f,0x95,0xc0,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // SETNZ AL
	{	0x0f,0x9f,0xc0,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // SETNLE AL
	{	0x0f,0x9d,0xc0,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // SETNL AL
	{	0x0f,0x9c,0xc0,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // SETL AL
	{	0x0f,0x9e,0xc0,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // SETLE AL
	{	0x22,0x45,-1,	0x22,0x85,-1,	0x22,0x05,-1,	0x24,-1,-1,		0x22,0x02,-1,	-1,		8	}, // AND.b AL,m
	{	0x0a,0x45,-1,	0x0a,0x85,-1,	0x0a,0x05,-1,	0x0c,-1,-1,		0x0a,0x02,-1,	-1,		8	}, // OR.b AL,m
	{	0x32,0x45,-1,	0x32,0x85,-1,	0x32,0x05,-1,	0x34,-1,-1,		0x32,0x02,-1,	-1,		8	}, // XOR.b AL,m
	{	0x02,0x45,-1,	0x02,0x85,-1,	0x02,0x05,-1,	0x04,-1,-1,		0x02,0x02,-1,	-1,		8	}, // ADD.b AL,m
	{	0x00,0x45,-1,	0x00,0x85,-1,	0x00,0x05,-1,	-1,-1,-1,		0x00,0x02,-1,	-1,		-1	}, // ADD.b m,AL
	{	0x2a,0x45,-1,	0x2a,0x85,-1,	0x2a,0x05,-1,	0x2c,-1,-1,		0x2a,0x02,-1,	-1,		8	}, // SUB.b AL,m
	{	0x28,0x45,-1,	0x28,0x85,-1,	0x28,0x05,-1,	-1,-1,-1,		0x28,0x02,-1,	-1,		-1	}, // SUB.b m,AL
	{	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		0xe8,-1,-1,		-1,-1,-1,		-1,		32	}, // CALL const
	{	0xff,0xe0,-1,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // JMP eAX
	{	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		0xe9,-1,-1,		-1,-1,-1,		-1,		-1	}, // JMP const
	{	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		0x74,-1,-1,		-1,-1,-1,		-1,		8	}, // JZ.b const
	{	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		0x0f,0x84,-1,	-1,-1,-1,		-1,		32	}, // JZ.d const
	{	0xd9,0x45,-1,	0xd9,0x85,-1,	0xd9,0x05,-1,	-1,-1,-1,		0xd9,0x02,-1,	-1,		32	}, // LOAD.f m
	{	0xd9,0x5d,-1,	0xd9,0x9d,-1,	0xd9,0x1d,-1,	-1,-1,-1,		0xd9,0x1a,-1,	-1,		32	}, // SAVE.f m
	{	0xdb,0x45,-1,	0xdb,0x85,-1,	0xdb,0x05,-1,	-1,-1,-1,		0xdb,0x02,-1,	-1,		32	}, // LOAD.fi m
	{	0xd8,0x45,-1,	0xd8,0x85,-1,	0xd8,0x05,-1,	-1,-1,-1,		0xd8,0x02,-1,	-1,		32	}, // ADD.f m
	{	0xd8,0x65,-1,	0xd8,0xa5,-1,	0xd8,0x25,-1,	-1,-1,-1,		0xd8,0x22,-1,	-1,		32	}, // SUB.f m
	{	0xd8,0x4d,-1,	0xd8,0x8d,-1,	0xd8,0x0d,-1,	-1,-1,-1,		0xd8,0x0a,-1,	-1,		32	}, // MUL.f m
	{	0xd8,0x75,-1,	0xd8,0xb5,-1,	0xd8,0x35,-1,	-1,-1,-1,		0xd8,0x32,-1,	-1,		32	}, // DIV.f m
	{	0xd3,0xe8,-1,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // SHR eAX, CL
	{	0xd3,0xe0,-1,	-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,-1,-1,		-1,		-1	}, // SHL eAX, CL
};
#define CallRel32OCSize			5
#define AfterWaitOCSize			10

int OCOParam;
void OCAddInstruction(CScript *s,int inst,int kind,char *param=NULL,int offset=0,int insert_at=-1)
{
	if (insert_at<0)
		insert_at=s->OpcodeSize;
	int insert_length=0;
	//char insert_oc[128];
	if ((kind!=KindRefToLocal)&&(kind!=KindRefToGlobal))
		param+=offset;
	int l=8;
	if (((int)param>125)||((int)param<-125))	l=32; // l=16;
	//if (((int)param>65530)||((int)param<-65530))	l=32;
	short *oc=&AsmInstruction[inst].oc_g[0];
	if ((kind==KindVarLocal)&&(l==8))
		oc=&AsmInstruction[inst].oc_l8[0];
	if ((kind==KindVarLocal)&&(l==32))
		oc=&AsmInstruction[inst].oc_l32[0];
	if ((kind==KindConstant)||(kind==KindRefToConst)){
		if (AsmInstruction[inst].oc_c[0]>=0)
			oc=&AsmInstruction[inst].oc_c[0];
		else
			kind=KindVarGlobal;
	}
	if (kind<0)
		oc=&AsmInstruction[inst].oc_l8[0];
	if ((kind==KindRefToLocal)||(kind==KindRefToGlobal)){
		if (kind==KindRefToLocal)
			OCAddInstruction(s,inMovEdxM,KindVarLocal,param);
		if (kind==KindRefToGlobal)
			OCAddInstruction(s,inMovEdxM,KindVarGlobal,param);
		if (offset!=0)
			OCAddInstruction(s,inAddEdxM,KindConstant,(char*)offset);
		oc=&AsmInstruction[inst].oc_dr[0];
	}

	if (oc[0]>=0)	s->OCAddChar((char)oc[0]);
	if (oc[1]>=0)	s->OCAddChar((char)oc[1]);
	if (oc[2]>=0)	s->OCAddChar((char)oc[2]);
	OCOParam=s->OpcodeSize;
	if (kind==KindVarLocal){ // local
		if (l==8)
			s->OCAddChar((int)param);
//		else if (l==16)
//			s->OCAddWord((int)param);
		else if (l==32)
			s->OCAddInt((int)param);
	}else if (kind==KindVarGlobal)
		s->OCAddInt((int)param); // global
	else if (kind==KindConstant){
		if (AsmInstruction[inst].const_size==8)
			s->OCAddChar((int)param);
		else if (AsmInstruction[inst].const_size==16)
			s->OCAddWord((int)param);
		else if (AsmInstruction[inst].const_size==32)
			s->OCAddInt((int)param);
	}else if (kind==KindRefToConst){
		if (AsmInstruction[inst].const_size==8)
			s->OCAddChar(*(int*)param);
		else if (AsmInstruction[inst].const_size==16)
			s->OCAddWord(*(int*)param);
		else if (AsmInstruction[inst].const_size==32)
			s->OCAddInt(*(int*)param);
	}

	oc=&AsmInstruction[inst].oc_p;
	if (oc[0]>=0)	s->OCAddChar((char)oc[0]);
	/*if (oc[0]>=0){	insert_oc[insert_length]=(char)oc[0];	insert_length++;	}
	if (oc[1]>=0){	insert_oc[insert_length]=(char)oc[1];	insert_length++;	}
	if (oc[2]>=0){	insert_oc[insert_length]=(char)oc[2];	insert_length++;	}
	OCOParam=insert_at+insert_length;
	if (kind==KindVarLocal){ // local
		if (l==8)
			insert_oc[insert_length]=(char)(int)param;
//		else if (l==16)
//			*(short*)&insert_oc[insert_length]=(short)param);
		else if (l==32)
			*(int*)&insert_oc[insert_length]=(int)param;
		insert_length+=l/8;
	}else if (kind==KindVarGlobal){ // global
		*(int*)&insert_oc[insert_length]=(int)param;
		insert_length+=4;
	}else if (kind==KindConstant){
		if (AsmInstruction[inst].const_size==8)
			insert_oc[insert_length]=(char)(int)param;
		else if (AsmInstruction[inst].const_size==16)
			*(short*)&insert_oc[insert_length]=(short)(int)param;
		else if (AsmInstruction[inst].const_size==32)
			*(int*)&insert_oc[insert_length]=(int)param;
		insert_length+=AsmInstruction[inst].const_size/8;
	}else if (kind==KindRefToConst){
		if (AsmInstruction[inst].const_size==8)
			insert_oc[insert_length]=*(char*)param;
		else if (AsmInstruction[inst].const_size==16)
			*(short*)&insert_oc[insert_length]=*(short*)param;
		else if (AsmInstruction[inst].const_size==32)
			*(int*)&insert_oc[insert_length]=*(int*)param;
		insert_length+=AsmInstruction[inst].const_size/8;
	}

	oc=&AsmInstruction[inst].oc_p;
	if (oc[0]>=0){	insert_oc[insert_length]=(char)oc[0];	insert_length++;	}

	int i;
	for (i=s->OpcodeSize-1;i>=insert_at;i--)
		s->Opcode[i+insert_length]=s->Opcode[i];
	for (i=0;i<insert_length;i++)
		s->Opcode[insert_at+i]=insert_oc[i];
	s->OpcodeSize+=insert_length;*/
}

void OCAddEspAdd(CScript *s,int d)
{
	if (d>0){
		if (d>120){		s->OCAddChar((char)0x81);	s->OCAddChar((char)0xc4);	s->OCAddInt(d);	}
		else{			s->OCAddChar((char)0x83);	s->OCAddChar((char)0xc4);	s->OCAddChar((char)d);	}
	}else if (d<0){
		if (d<-120){	s->OCAddChar((char)0x81);	s->OCAddChar((char)0xec);	s->OCAddInt(-d);	}
		else{			s->OCAddChar((char)0x83);	s->OCAddChar((char)0xec);	s->OCAddChar((char)-d);	}
	}
}

char *OCAddParameter(CScript *s,sLinkData *link,int n_func,int level,int index,int &pk,bool allow_auto_ref=true)
{
	so("param");
	CPreScript *ps=s->pre_script;
	right();
	pk=link->Kind;
	char *ret=NULL;
	//sType *rt=link->;
	//msg_write(Kind2Str(link->Kind));
	if (link->Kind==KindVarFunction){
		so(" -var-func");
		if (ps->FlagCompileOS)
			ret=(char*)((int)s->func[link->Nr]-(int)&s->Opcode[0]+((sAsmMetaInfo*)ps->AsmMetaInfo)->CodeOrigin);
		else
			ret=(char*)s->func[link->Nr];
		pk=KindVarGlobal;
	}else if (link->Kind==KindVarGlobal){
		so(" -global");
		if (link->script)
			ret=link->script->g_var[link->Nr];
		else
			ret=s->g_var[link->Nr];
	}else if (link->Kind==KindVarLocal){
		so(" -local");
		ret=(char*)(int)ps->Function[n_func]->VarOffset[link->Nr];
	}else if (link->Kind==KindVarExternal){
		so(" -external-var");
		ret=PreExternalVar[link->Nr].Pointer;
		pk=KindVarGlobal;
		if (!ret){
			msg_write("OCAddParameter  nur ein Verweis auf eine externe Variable?");
			ret=(char*)&PreExternalVar[link->Nr].Pointer;
			pk=KindRefToGlobal;
		}
	}else if (link->Kind==KindConstant){
		so(" -const");
		if ((UseConstAsGlobalVar)||(ps->FlagCompileOS))
			pk=KindVarGlobal;
		else
			pk=KindRefToConst;
		ret=s->cnst[link->Nr];
	}else if (link->Kind==KindCommand){
		pk=KindVarLocal;
		ret=s->OCAddCommand(ps->Command[link->Nr],n_func,level,index);
	}else if (link->Kind==KindPointerShift){
		so(" -p.shift");
		char *param=OCAddParameter(s,link->Meta,n_func,level,index,pk);
		if ((pk==KindVarLocal)||(pk==KindVarGlobal)){
			so("  ->const");
			ret=param+link->Nr;
		}else{
			so("  ->lea");
			if (pk!=KindRefToLocal){
				s->DoErrorLink(string("unexpected meta type for pointer shift: ",Kind2Str(pk)),-1);
				return NULL;
			}
			ret=param;
			OCAddInstruction(s,inMovEaxM,KindConstant,(char*)(int)link->Nr);
			OCAddInstruction(s,inAddMEax,KindVarLocal,ret);
			pk=KindRefToLocal;
		}
	}else if (link->Kind==KindDerefPointerShift){
		so(" -deref-shift");
		char *param=OCAddParameter(s,link->Meta,n_func,level,index,pk);
		ret=(char*)(-LOffset-ps->Function[n_func]->VarSize-4);
		LOffset+=4;
		if (LOffset>LOffsetMax)
			LOffsetMax=LOffset;
		OCAddInstruction(s,inMovEaxM,pk,param);
		OCAddInstruction(s,inAddEaxM,KindConstant,(char*)(int)link->Nr);
		OCAddInstruction(s,inMovMEax,KindVarLocal,ret);
		pk=KindRefToLocal;
	}else if (link->Kind==KindArray){
		so(" -array");
		char *param=OCAddParameter(s,link->Meta,n_func,level,index,pk,false);
		if ((link->ParamLink->Kind==KindConstant)&&( (pk==KindVarLocal)||(pk==KindVarGlobal) )){
			so("  ->const");
			ret=param+(*(int*)ps->Constant[link->ParamLink->Nr]->data)*link->type->Size;
		}else{
			so("  ->lea");
			if (pk!=KindRefToLocal){
				so("    ->neu");
				OCAddInstruction(s,inLeaEaxM,pk,param);
				ret=(char*)(-LOffset-ps->Function[n_func]->VarSize-4);
				LOffset+=4;
				if (LOffset>LOffsetMax)
					LOffsetMax=LOffset;
				OCAddInstruction(s,inMovMEax,KindVarLocal,ret);
			}else
				ret=param;
			param=OCAddParameter(s,link->ParamLink,n_func,level,index,pk,false);
			OCAddInstruction(s,inMovEaxM,pk,param);
			OCAddInstruction(s,inMulEaxM,KindConstant,(char*)link->type->Size);
			OCAddInstruction(s,inAddMEax,KindVarLocal,ret);
			pk=KindRefToLocal;
		}
	}else if (link->Kind==KindPointerAsArray){
		so(" -pointer-array");
		char *param=OCAddParameter(s,link->Meta,n_func,level,index,pk,false);
		//so("->lea");
		//if (pk!=KindRefToLocal){
			so("  ->neu");
			OCAddInstruction(s,inMovEaxM,pk,param);
			ret=(char*)(-LOffset-ps->Function[n_func]->VarSize-4);
			LOffset+=4;
			if (LOffset>LOffsetMax)
				LOffsetMax=LOffset;
			OCAddInstruction(s,inMovMEax,KindVarLocal,ret);
		//}else
		//	ret=param;
		param=OCAddParameter(s,link->ParamLink,n_func,level,index,pk,false);
		OCAddInstruction(s,inMovEaxM,pk,param);
		OCAddInstruction(s,inMulEaxM,KindConstant,(char*)link->type->Size);
		OCAddInstruction(s,inAddMEax,KindVarLocal,ret);
		pk=KindRefToLocal;
	}else if (link->Kind==KindReference){
		//msg_write(Kind2Str(link->Meta->Kind));
		so(" -ref");
		char *param=OCAddParameter(s,link->Meta,n_func,level,index,pk,false);
		//printf("%d  -  %s\n",pk,Kind2Str(pk));
		if ((pk==KindConstant)||(pk==KindVarGlobal)||(pk==KindRefToConst)){
			so("  Reference-Const");
			pk=KindConstant;
			ret=param;
		}else{
			ret=(char*)(-LOffset-ps->Function[n_func]->VarSize-4);
			LOffset+=4;
			if (LOffset>LOffsetMax)
				LOffsetMax=LOffset;
			OCAddInstruction(s,inLeaEaxM,pk,param);
			pk=KindVarLocal;
			OCAddInstruction(s,inMovMEax,pk,ret);
		}
	}else if (link->Kind==KindDereference){
		so(" -deref...");
		char *param=OCAddParameter(s,link->Meta,n_func,level,index,pk);
		if ((pk==KindVarLocal)||(pk==KindVarGlobal)){
			if (pk==KindVarLocal)	pk=KindRefToLocal;
			if (pk==KindVarGlobal)	pk=KindRefToGlobal;
			ret=param;
		}
	}else
		s->DoError(string("unexpected type of parameter: ",Kind2Str(link->Kind)),-1);
	//if ((!link->type->IsPointer)&&(link->type->SubType)){
	if ((allow_auto_ref)&&(link->type->ArrayLength)){
		so("Array: c referenziert automatisch!!");
		char *param=ret;
		ret=(char*)(-LOffset-ps->Function[n_func]->VarSize-4);
		LOffset+=4;
		if (LOffset>LOffsetMax)
			LOffsetMax=LOffset;
		OCAddInstruction(s,inLeaEaxM,pk,param);
		pk=KindVarLocal;
		OCAddInstruction(s,inMovMEax,pk,ret);
		link->type=&TypePointer;
	}
	//msg_ok();
	left();
	return ret;
}

char *CScript::OCAddCommand(sCommand *com,int n_func,int level,int index)
{
	so("Befehl");
	//msg_write(Kind2Str(com->Kind));
	sFunction *p_func=pre_script->Function[n_func];
	right();
	int i;
	char *param[SCRIPT_MAX_PARAMS];
	int s=com->ReturnLink.type->Size+(4-com->ReturnLink.type->Size%4)%4;
	char *ret=(char*)(-LOffset-p_func->VarSize-s);
	//so(d2h((char*)&ret,4,false));
	so(string("return: ",i2s(com->ReturnLink.type->Size),"/",i2s(LOffset),"/",i2s(LOffset+s)));
	LOffset+=s;
	int pk[SCRIPT_MAX_PARAMS],rk=KindVarLocal;
	if (LOffset>LOffsetMax)
		LOffsetMax=LOffset;
	for (int p=0;p<com->NumParams;p++)
		param[p]=OCAddParameter(this,&com->ParamLink[p],n_func,level,index,pk[p]);
	if (com->Kind==KindOperator){
		//msg_write("---operator");
		switch(com->LinkNr){
			case OperatorIntAssign:
			case OperatorFloatAssign:
			case OperatorPointerAssign:
				OCAddInstruction(this,inMovEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEax,pk[0],param[0]);
				break;
			case OperatorCharAssign:
			case OperatorBoolAssign:
				OCAddInstruction(this,inMovAlM8,pk[1],param[1]);
				OCAddInstruction(this,inMovM8Al,pk[0],param[0]);
				break;
			case OperatorStructAssign:
				for (i=0;i<signed(com->ParamLink[0].type->Size+3)/4;i++){
					OCAddInstruction(this,inMovEaxM,pk[1],param[1],i*4);
					OCAddInstruction(this,inMovMEax,pk[0],param[0],i*4);
				}
				break;
// string
			case OperatorStringAssignAA:
			case OperatorStringAssignAP:
				OCAddEspAdd(this,-p_func->VarSize-LOffset);
				OCAddInstruction(this,inPush,pk[1],param[1]);
				OCAddInstruction(this,inPush,pk[0],param[0]);
				OCAddInstruction(this,inCallRel32,KindConstant,(char*)((int)&strcpy-(int)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(this,p_func->VarSize+LOffset+8);
				break;
			case OperatorStringAddAAS:
			case OperatorStringAddAPS:
				OCAddEspAdd(this,-p_func->VarSize-LOffset);
				OCAddInstruction(this,inPush,pk[1],param[1]);
				OCAddInstruction(this,inPush,pk[0],param[0]);
				OCAddInstruction(this,inCallRel32,KindConstant,(char*)((int)&strcat-(int)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(this,p_func->VarSize+LOffset+8);
				break;
			case OperatorStringAddAA:
			case OperatorStringAddAP:
			case OperatorStringAddPA:
			case OperatorStringAddPP:
				OCAddEspAdd(this,-p_func->VarSize-LOffset);
				so(d2h((char*)&param[0],4,false));
				so(d2h((char*)&ret,4,false));
				OCAddInstruction(this,inLeaEdxM,rk,ret);
				OCAddInstruction(this,inPush,pk[0],param[0]);
				OCAddInstruction(this,inPushEdx,-1);
				OCAddInstruction(this,inCallRel32,KindConstant,(char*)((int)&strcpy-(int)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(this,8);
				OCAddInstruction(this,inLeaEdxM,rk,ret);
				OCAddInstruction(this,inPush,pk[1],param[1]);
				OCAddInstruction(this,inPushEdx,-1);
				OCAddInstruction(this,inCallRel32,KindConstant,(char*)((int)&strcat-(int)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(this,p_func->VarSize+LOffset+8);
				break;
			case OperatorStringEqualAA:
			case OperatorStringEqualAP:
			case OperatorStringEqualPA:
			case OperatorStringEqualPP:
			case OperatorStringNotEqualAA:
			case OperatorStringNotEqualAP:
			case OperatorStringNotEqualPA:
			case OperatorStringNotEqualPP:
				OCAddEspAdd(this,-p_func->VarSize-LOffset);
				OCAddInstruction(this,inPush,pk[1],param[1]);
				OCAddInstruction(this,inPush,pk[0],param[0]);
				OCAddInstruction(this,inCallRel32,KindConstant,(char*)((int)&strcmp-(int)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(this,p_func->VarSize+LOffset+8);
				OCAddInstruction(this,inCmpAlM8,KindConstant,NULL);
				if ((com->LinkNr==OperatorStringEqualAA)||(com->LinkNr==OperatorStringEqualAP)||(com->LinkNr==OperatorStringEqualPA)||(com->LinkNr==OperatorStringEqualPP))
					OCAddInstruction(this,inSetzAl,-1);
				if ((com->LinkNr==OperatorStringNotEqualAA)||(com->LinkNr==OperatorStringNotEqualAP)||(com->LinkNr==OperatorStringNotEqualPA)||(com->LinkNr==OperatorStringNotEqualPP))
					OCAddInstruction(this,inSetnzAl,-1);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
// int
			case OperatorIntAddS:
				OCAddInstruction(this,inMovEaxM,pk[1],param[1]);
				OCAddInstruction(this,inAddMEax,pk[0],param[0]);
				break;
			case OperatorIntSubtractS:
				OCAddInstruction(this,inMovEaxM,pk[1],param[1]);
				OCAddInstruction(this,inSubMEax,pk[0],param[0]);
				break;
			case OperatorIntMultiplyS:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inMulEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEax,pk[0],param[0]);
				break;
			case OperatorIntDivideS:
				OCAddInstruction(this,inMovEdxM,pk[0],param[0]);
				OCAddChar((char)0x89);	OCAddChar((char)0xd0); // MOV eAX,eDX			// TODO
				OCAddChar((char)0xc1);	OCAddChar((char)0xfa);	OCAddChar(0x1f); // SAR eDX,<1f>
				OCAddInstruction(this,inDivEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEax,pk[0],param[0]);
				break;
			case OperatorIntAdd:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inAddEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
			case OperatorIntSubtract:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inSubEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
			case OperatorIntMultiply:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inMulEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
			case OperatorIntDivide:
				OCAddInstruction(this,inMovEdxM,pk[0],param[0]);
				OCAddChar((char)0x89);	OCAddChar((char)0xd0); // MOV eAX,eDX			// TODO
				OCAddChar((char)0xc1);	OCAddChar((char)0xfa);	OCAddChar((char)0x1f); // SAR eDX,<1f>
				OCAddInstruction(this,inDivEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
			case OperatorIntModulo:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddChar((char)0x99);
				if (pk[1]==KindConstant){
					OCAddInstruction(this,inMovEaxM,pk[1],param[1]);
					OCAddInstruction(this,inDivEaxEbx,-1,NULL);
				}else
					OCAddInstruction(this,inDivEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEdx,rk,ret);
				break;
			case OperatorIntEqual:
			case OperatorIntNotEqual:
			case OperatorIntGreater:
			case OperatorIntGreaterEqual:
			case OperatorIntSmaller:
			case OperatorIntSmallerEqual:
			case OperatorPointerEqual:
			case OperatorPointerNotEqual:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inCmpEaxM,pk[1],param[1]);
				if (com->LinkNr==OperatorIntEqual)			OCAddInstruction(this,inSetzAl,-1);
				if (com->LinkNr==OperatorIntNotEqual)		OCAddInstruction(this,inSetnzAl,-1);
				if (com->LinkNr==OperatorIntGreater)		OCAddInstruction(this,inSetnleAl,-1);
				if (com->LinkNr==OperatorIntGreaterEqual)	OCAddInstruction(this,inSetnlAl,-1);
				if (com->LinkNr==OperatorIntSmaller)		OCAddInstruction(this,inSetlAl,-1);
				if (com->LinkNr==OperatorIntSmallerEqual)	OCAddInstruction(this,inSetleAl,-1);
				if (com->LinkNr==OperatorPointerEqual)		OCAddInstruction(this,inSetzAl,-1);
				if (com->LinkNr==OperatorPointerNotEqual)	OCAddInstruction(this,inSetnzAl,-1);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
			case OperatorIntBitAnd:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inAndEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
			case OperatorIntBitOr:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inOrEaxM,pk[1],param[1]);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
			case OperatorIntShiftRight:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inMovClM8,pk[1],param[1]);
				OCAddInstruction(this,inShrEaxCl,-1);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
			case OperatorIntShiftLeft:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inMovClM8,pk[1],param[1]);
				OCAddInstruction(this,inShlEaxCl,-1);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
			case OperatorIntNegate:
				OCAddInstruction(this,inMovEaxM,KindConstant,NULL);
				OCAddInstruction(this,inSubEaxM,pk[0],param[0]);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
			case OperatorIntIncrease:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inAddEaxM,KindConstant,(char*)0x00000001);
				OCAddInstruction(this,inMovMEax,pk[0],param[0]);
				break;
			case OperatorIntDecrease:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inSubEaxM,KindConstant,(char*)0x00000001);
				OCAddInstruction(this,inMovMEax,pk[0],param[0]);
				break;
// float
			case OperatorFloatAddS:
			case OperatorFloatSubtractS:
			case OperatorFloatMultiplyS:
			case OperatorFloatDivideS:
				OCAddInstruction(this,inLoadfM,pk[0],param[0]);
				if (com->LinkNr==OperatorFloatAddS)			OCAddInstruction(this,inAddfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatSubtractS)	OCAddInstruction(this,inSubfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatMultiplyS)	OCAddInstruction(this,inMulfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatDivideS)		OCAddInstruction(this,inDivfM,pk[1],param[1]);
				OCAddInstruction(this,inSavefM,pk[0],param[0]);
				break;
			case OperatorFloatAdd:
			case OperatorFloatSubtract:
			case OperatorFloatMultiply:
			case OperatorFloatDivide:
				OCAddInstruction(this,inLoadfM,pk[0],param[0]);
				if (com->LinkNr==OperatorFloatAdd)		OCAddInstruction(this,inAddfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatSubtract)	OCAddInstruction(this,inSubfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatMultiply)	OCAddInstruction(this,inMulfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatDivide)	OCAddInstruction(this,inDivfM,pk[1],param[1]);
				OCAddInstruction(this,inSavefM,rk,ret);
				break;
			case OperatorFloatMultiplyFI:
				OCAddInstruction(this,inLoadfiM,pk[1],param[1]);
				OCAddInstruction(this,inMulfM,pk[0],param[0]);
				OCAddInstruction(this,inSavefM,rk,ret);
				break;
			case OperatorFloatEqual:
			case OperatorFloatNotEqual:
			case OperatorFloatGreater:
			case OperatorFloatGreaterEqual:
			case OperatorFloatSmaller:
			case OperatorFloatSmallerEqual:
				OCAddInstruction(this,inLoadfM,pk[0],param[0]);
				OCAddInstruction(this,inLoadfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatEqual){
					OCAddChar(0xd9);	OCAddChar(0xc9);
					OCAddChar(0xda);	OCAddChar(0xe9);
					OCAddChar(0xdf);	OCAddChar(0xe0);
					OCAddChar(0x80);	OCAddChar(0xe4);	OCAddChar(0x45);
					OCAddChar(0x80);	OCAddChar(0xfc);	OCAddChar(0x40);
				}else if (com->LinkNr==OperatorFloatNotEqual){
					OCAddChar(0xd9);	OCAddChar(0xc9);
					OCAddChar(0xda);	OCAddChar(0xe9);
					OCAddChar(0xdf);	OCAddChar(0xe0);
					OCAddChar(0x80);	OCAddChar(0xe4);	OCAddChar(0x45);
					OCAddChar(0x80);	OCAddChar(0xf4);	OCAddChar(0x40);
				}else if (com->LinkNr==OperatorFloatSmaller){
					OCAddChar(0xda);	OCAddChar(0xe9);
					OCAddChar(0xdf);	OCAddChar(0xe0);
					OCAddChar(0xf6);	OCAddChar(0xc4);	OCAddChar(0x45);
				}else if (com->LinkNr==OperatorFloatSmallerEqual){
					OCAddChar(0xda);	OCAddChar(0xe9);
					OCAddChar(0xdf);	OCAddChar(0xe0);
					OCAddChar(0xf6);	OCAddChar(0xc4);	OCAddChar(0x05);
				}else if (com->LinkNr==OperatorFloatGreater){
					OCAddChar(0xd9);	OCAddChar(0xc9);
					OCAddChar(0xda);	OCAddChar(0xe9);
					OCAddChar(0xdf);	OCAddChar(0xe0);
					OCAddChar(0xf6);	OCAddChar(0xc4);	OCAddChar(0x45);
				}else if (com->LinkNr==OperatorFloatGreaterEqual){
					OCAddChar(0xd9);	OCAddChar(0xc9);
					OCAddChar(0xda);	OCAddChar(0xe9);
					OCAddChar(0xdf);	OCAddChar(0xe0);
					OCAddChar(0xf6);	OCAddChar(0xc4);	OCAddChar(0x05);
				}
				OCAddInstruction(this,inSetzAl,-1);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
			case OperatorFloatNegate:
				OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(this,inXorEaxM,KindConstant,(char*)0x80000000);
				OCAddInstruction(this,inMovMEax,rk,ret);
				break;
// bool/char
			case OperatorCharEqual:
			case OperatorCharNotEqual:
			case OperatorBoolEqual:
			case OperatorBoolNotEqual:
			case OperatorBoolGreater:
			case OperatorBoolGreaterEqual:
			case OperatorBoolSmaller:
			case OperatorBoolSmallerEqual:
				OCAddInstruction(this,inMovAlM8,pk[1],param[1]);
				OCAddInstruction(this,inCmpAlM8,pk[0],param[0]);
				if ((com->LinkNr==OperatorCharEqual)||(com->LinkNr==OperatorBoolEqual))
					OCAddInstruction(this,inSetzAl,-1);
				if ((com->LinkNr==OperatorCharNotEqual)||(com->LinkNr==OperatorBoolNotEqual))
					OCAddInstruction(this,inSetnzAl,-1);
				if (com->LinkNr==OperatorBoolGreater)		OCAddInstruction(this,inSetnleAl,-1);
				if (com->LinkNr==OperatorBoolGreaterEqual)	OCAddInstruction(this,inSetnlAl,-1);
				if (com->LinkNr==OperatorBoolSmaller)		OCAddInstruction(this,inSetlAl,-1);
				if (com->LinkNr==OperatorBoolSmallerEqual)	OCAddInstruction(this,inSetleAl,-1);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
			case OperatorBoolAnd:
			case OperatorBoolOr:
				OCAddInstruction(this,inMovAlM8,pk[0],param[0]);
				if (com->LinkNr==OperatorBoolAnd)	OCAddInstruction(this,inAndAlM8,pk[1],param[1]);
				if (com->LinkNr==OperatorBoolOr)	OCAddInstruction(this,inOrAlM8,pk[1],param[1]);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
			case OperatorCharAddS:
				OCAddInstruction(this,inMovAlM8,pk[1],param[1]);
				OCAddInstruction(this,inAddM8Al,pk[0],param[0]);
				break;
			case OperatorCharSubtractS:
				OCAddInstruction(this,inMovAlM8,pk[1],param[1]);
				OCAddInstruction(this,inSubM8Al,pk[0],param[0]);
				break;
			case OperatorCharAdd:
				OCAddInstruction(this,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(this,inAddAlM8,pk[1],param[1]);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
			case OperatorCharSubtract:
				OCAddInstruction(this,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(this,inSubAlM8,pk[1],param[1]);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
			case OperatorCharBitAnd:
				OCAddInstruction(this,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(this,inAndAlM8,pk[1],param[1]);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
			case OperatorCharBitOr:
				OCAddInstruction(this,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(this,inOrAlM8,pk[1],param[1]);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
			case OperatorBoolNegate:
				OCAddInstruction(this,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(this,inXorAlM8,KindConstant,(char*)0x01);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
			case OperatorCharNegate:
				OCAddInstruction(this,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(this,inXorAlM8,KindConstant,(char*)0xff);
				OCAddInstruction(this,inMovM8Al,rk,ret);
				break;
// vector
			case OperatorVectorAddS:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(this,inAddfM,pk[1],param[1],i*4);
					OCAddInstruction(this,inSavefM,pk[0],param[0],i*4);
				}
				break;
			case OperatorVectorMultiplyS:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(this,inMulfM,pk[1],param[1]);
					OCAddInstruction(this,inSavefM,pk[0],param[0],i*4);
				}
				break;
			case OperatorVectorDivideS:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(this,inDivfM,pk[1],param[1]);
					OCAddInstruction(this,inSavefM,pk[0],param[0],i*4);
				}
				break;
			case OperatorVectorSubtractS:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(this,inSubfM,pk[1],param[1],i*4);
					OCAddInstruction(this,inSavefM,pk[0],param[0],i*4);
				}
				break;
			case OperatorVectorAdd:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(this,inAddfM,pk[1],param[1],i*4);
					OCAddInstruction(this,inSavefM,rk,ret,i*4);
				}
				break;
			case OperatorVectorSubtract:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(this,inSubfM,pk[1],param[1],i*4);
					OCAddInstruction(this,inSavefM,rk,ret+i*4);
				}
				break;
			case OperatorVectorMultiplyVF:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(this,inMulfM,pk[1],param[1]);
					OCAddInstruction(this,inSavefM,rk,ret,i*4);
				}
				break;
			case OperatorVectorMultiplyFV:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inLoadfM,pk[0],param[0]);
					OCAddInstruction(this,inMulfM,pk[1],param[1],i*4);
					OCAddInstruction(this,inSavefM,rk,ret,i*4);
				}
				break;
			case OperatorVectorDivideVF:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(this,inDivfM,pk[1],param[1]);
					OCAddInstruction(this,inSavefM,rk,ret,i*4);
				}
				break;
			case OperatorVectorNegate:
				for (i=0;i<3;i++){
					OCAddInstruction(this,inMovEaxM,pk[0],param[0],i*4);
					OCAddInstruction(this,inXorEaxM,KindConstant,(char*)0x80000000);
					OCAddInstruction(this,inMovMEax,rk,ret,i*4);
				}
				break;
			default:
				DoError(string("unimplemented operator (call Michi!): ",Operator2Str(pre_script,com->LinkNr)),-1);
				break;
		}
	}else if ((com->Kind==KindCompilerFunction)||(com->Kind==KindFunction)){
		//msg_write("---func");
		t_func *f=NULL;
		char *instance=NULL;
		char name[128];
		if (com->Kind==KindFunction){ // eigene Script-Funktion
			so("Funktion!!!");
			if (com->script){
				so("    extern!!!");
				f=com->script->func[com->LinkNr];
				so("   -ok");
			}else
				f=func[com->LinkNr];
		}else // Compiler-Funktion
			for (int c=0;c<NumPreCommands;c++)
				if (PreCommand[c].Nr==com->LinkNr){
					f=(t_func*)PreCommand[c].Func;
					instance=PreCommand[c].Instance;
					strcpy(name,PreCommand[c].Name);
				}
		if ((int)f>(int)f_cp){ // echte Funktion
			if ((com->ReturnLink.type->Size>4)&&(com->ReturnLink.type->ArrayLength<=0))
				OCAddInstruction(this,inLeaEaxM,rk,ret);

			// Stack um die lokalen Variablen der bisherigen Funktion erniedrigen
			OCAddEspAdd(this,-p_func->VarSize-LOffset-8);
			int dp=0;

			for (int p=com->NumParams-1;p>=0;p--){
				int s=com->ParamLink[p].type->Size+(4-com->ParamLink[p].type->Size%4)%4;
				// Parameter auf den Stack pushen
				for (int j=0;j<s/4;j++)
					OCAddInstruction(this,inPush,pk[p],param[p],s-4-j*4);
				dp+=s;
			}

#ifdef NIX_IDE_VCS
			// muessen mehr als 4byte zurueckgegeben werden, muss die Rueckgabe-Adresse als allererster Parameter mitgegeben werden!
			if ((com->ReturnLink.type->Size>4)&&(com->ReturnLink.type->ArrayLength<=0))
				OCAddInstruction(this,inPushEax,-1); // nachtraegliche eSP-Korrektur macht die Funktion
#endif
			// _cdecl: Klassen-Instanz als ersten Parameter push'en
			if (instance){
				if ((int)instance>100)	OCAddInstruction(this,inPush,KindConstant,*(char**)instance);
				else					OCAddInstruction(this,inPush,KindVarGlobal,instance);
				dp+=4;
			}
#ifndef NIX_IDE_VCS
			// muessen mehr als 4byte zurueckgegeben werden, muss die Rueckgabe-Adresse als allererster Parameter mitgegeben werden!
			if ((com->ReturnLink.type->Size>4)&&(com->ReturnLink.type->ArrayLength<=0))
				OCAddInstruction(this,inPushEax,-1); // nachtraegliche eSP-Korrektur macht die Funktion
#endif
			int d=(int)f-(int)&Opcode[OpcodeSize]-5;
			//so(d);
			OCAddInstruction(this,inCallRel32,KindConstant,(char*)d); // der eigentliche Aufruf
			OCAddEspAdd(this,p_func->VarSize+LOffset+dp+8);

			// Rueckgabewert > 4b ist schon von der Funktion nach [ret] kopiert worden!
			if (com->ReturnLink.type!=&TypeVoid)
				if ((com->ReturnLink.type->Size<=4)||(com->ReturnLink.type->ArrayLength>0)){
					if (com->ReturnLink.type==&TypeFloat)
						OCAddInstruction(this,inSavefM,rk,ret);
					else
						OCAddInstruction(this,inMovMEax,rk,ret);
				}
		}else if ((int)f==(int)f_cp){
			switch(com->LinkNr){
				case CommandIf:
					OCAddInstruction(this,inCmpM80,pk[0],param[0]);
					//OCAddInstruction(this,inJzC8,KindConstant,NULL);
					OCAddInstruction(this,inJzC32,KindConstant,NULL);
					JumpSource[level][NumJumps[level]]=index+1;
					JumpDest[level][NumJumps[level]]=index+2;
					JumpCode[level][NumJumps[level]]=OCOParam;
					NumJumps[level]++;
					break;
				case CommandIfElse:
					OCAddInstruction(this,inCmpM80,pk[0],param[0]);
					//OCAddInstruction(this,inJzC8,KindConstant,NULL);
					OCAddInstruction(this,inJzC32,KindConstant,NULL);
					JumpSource[level][NumJumps[level]]=index+1;
					JumpDest[level][NumJumps[level]]=index+2;
					JumpCode[level][NumJumps[level]]=OCOParam;
					NumJumps[level]++;
					JumpSource[level][NumJumps[level]]=index+2;
					JumpDest[level][NumJumps[level]]=index+3;
					JumpCode[level][NumJumps[level]]=-1;
					NumJumps[level]++;
					break;
				case CommandWhile:
					OCAddInstruction(this,inCmpM80,pk[0],param[0]);
					//OCAddInstruction(this,inJzC8,KindConstant,NULL);
					OCAddInstruction(this,inJzC32,KindConstant,NULL);
					JumpSource[level][NumJumps[level]]=index+1;
					JumpDest[level][NumJumps[level]]=index+2;
					JumpCode[level][NumJumps[level]]=OCOParam;
					NumJumps[level]++;
					JumpSource[level][NumJumps[level]]=index+2;
					JumpDest[level][NumJumps[level]]=index;
					JumpCode[level][NumJumps[level]]=-1;
					NumJumps[level]++;
					break;
				case CommandReturn:
					if (com->NumParams>0){
						if (com->ParamLink[0].type->Size>4){ // Return in erhaltener Return-Adresse speichern (> 4 byte)
							OCAddInstruction(this,inMovEdxM,KindVarLocal,(char*)8);
							int s=com->ParamLink[0].type->Size+(4-com->ParamLink[0].type->Size%4)%4;
							for (int j=0;j<s/4;j++){
								OCAddInstruction(this,inMovEaxM,pk[0],param[0],j*4);
								OCAddInstruction(this,inMovEdxpi8Eax,KindConstant,(char*)(j*4));
							}
						}else // Return direkt in eAX speichern (4 byte)
							if (com->ParamLink[0].type==&TypeFloat)
								OCAddInstruction(this,inLoadfM,pk[0],param[0]);
							else
								OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
					}
					OCAddInstruction(this,inLeave,-1);
					if (p_func->Type->Size>4)
						OCAddInstruction(this,inRet,-1); // inRet4
					else
						OCAddInstruction(this,inRet,-1);
					break;
				case CommandWaitOneFrame:
				case CommandWait:
				case CommandWaitRT:
					// Warte-Zustand setzen
					if (com->LinkNr==CommandWaitOneFrame)	OCAddInstruction(this,inMovEaxM,KindConstant,(char*)WaitingModeRT);
					if (com->LinkNr==CommandWait)			OCAddInstruction(this,inMovEaxM,KindConstant,(char*)WaitingModeGT);
					if (com->LinkNr==CommandWaitRT)			OCAddInstruction(this,inMovEaxM,KindConstant,(char*)WaitingModeRT);
					OCAddInstruction(this,inMovMEax,KindVarGlobal,(char*)&WaitingMode);
					if (com->LinkNr==CommandWaitOneFrame)	OCAddInstruction(this,inMovEaxM,KindConstant,NULL);
					if (com->LinkNr==CommandWait)			OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
					if (com->LinkNr==CommandWaitRT)			OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
					OCAddInstruction(this,inMovMEax,KindVarGlobal,(char*)&TimeToWait);
					// Script-Zustand speichern
					OCAddInstruction(this,inMovEspM,KindConstant,(char*)&Stack[SCRIPT_STACK_SIZE-4]); // zum Anfang des Script-Stacks
					OCAddInstruction(this,inPushEbp,-1);
					OCAddInstruction(this,inCallRel32,KindConstant,(char*)0); // PUSH eIP
					// Return laden
					OCAddInstruction(this,inMovEspM,KindConstant,(char*)&Stack[SCRIPT_STACK_SIZE-4]); // zum Anfang des Script-Stacks
					OCAddInstruction(this,inPopEsp,-1); // alter StackPointer (echtes Programm)
					OCAddInstruction(this,inMovEbpEsp,-1);
					OCAddInstruction(this,inLeave,-1);
					OCAddInstruction(this,inRet,-1);
					// hier ist die Unterbrechung!
					OCAddInstruction(this,inMovEspM,KindConstant,(char*)&Stack[SCRIPT_STACK_SIZE-8]); // zum eBP
					OCAddInstruction(this,inPopEsp,-1); // Script-StackPointer
					OCAddInstruction(this,inMovEbpEsp,-1);
					OCAddInstruction(this,inMovEaxM,KindConstant,(char*)WaitingModeNone);
					OCAddInstruction(this,inMovMEax,KindVarGlobal,(char*)&WaitingMode);
					break;
				case CommandFloatFromInt:
					OCAddInstruction(this,inLoadfiM,pk[0],param[0]);
					OCAddInstruction(this,inSavefM,rk,ret);
					break;
				case CommandIntFromFloat:
					/*OCAddChar(0x89);	OCAddChar(0xe5);
					OCAddChar(0x83);	OCAddChar(0xec);	OCAddChar(0x04);
					OCAddChar(0xd9);	OCAddChar(0x05);	OCAddInt((int)param[0]);
					OCAddChar(0xd9);	OCAddChar(0x7d);	OCAddChar(0xfe);
					OCAddChar(0x66);	OCAddChar(0x8b);	OCAddChar(0x45);	OCAddChar(0xfe);
					OCAddChar(0xb4);	OCAddChar(0x0c);
					OCAddChar(0x66);	OCAddChar(0x89);	OCAddChar(0x45);	OCAddChar(0xfc);
					OCAddChar(0xd9);	OCAddChar(0x6d);	OCAddChar(0xfc);
					OCAddChar(0xdb);	OCAddChar(0x1d);	OCAddInt((int)ret);
					OCAddChar(0xd9);	OCAddChar(0x6d);	OCAddChar(0xfe);
					OCAddChar(0xc9);*/
					/*OCAddChar(0x51);
					OCAddChar(0xd9);    OCAddChar(0x05);	OCAddInt((int)param[0]);
					OCAddChar(0xd9);    OCAddChar(0x7d);    OCAddChar(0xfe);
					OCAddChar(0x66);	OCAddChar(0x8b);	OCAddChar(0x45);	OCAddChar(0xfe);
					OCAddChar(0xb4);	OCAddChar(0x0c);
					OCAddChar(0x66);	OCAddChar(0x89);	OCAddChar(0x45);	OCAddChar(0xfc);
					OCAddChar(0xd9);	OCAddChar(0x6d);	OCAddChar(0xfc);
					OCAddChar(0xdb);	OCAddChar(0x1d);	OCAddInt((int)ret);
					OCAddChar(0xd9);	OCAddChar(0x6d);	OCAddChar(0xfe);
					OCAddChar(0x89);    OCAddChar(0xec);*/
					break;
				case CommandAsm:
					{
//#if 0
						CreateAsmMetaInfo(pre_script);
						((sAsmMetaInfo*)pre_script->AsmMetaInfo)->CurrentOpcodePos=OpcodeSize;
						((sAsmMetaInfo*)pre_script->AsmMetaInfo)->LineOffset=pre_script->AsmLine[0];
						if (pre_script->FlagOverwriteVariablesOffset)
							((sAsmMetaInfo*)pre_script->AsmMetaInfo)->GlobalVarsOffset=pre_script->VariablesOffset;
						else
							((sAsmMetaInfo*)pre_script->AsmMetaInfo)->GlobalVarsOffset=int(Stack[0]);
						CurrentAsmMetaInfo=(sAsmMetaInfo*)pre_script->AsmMetaInfo;
				//		msg_write("Asm------------------");
						char ac[1024],*pac;
						pac=SetAsm(pre_script->AsmBlock[0]);
						if (pac){
							int i;
							for (i=0;i<AsmCodeLength;i++)
								ac[i]=pac[i];
				/*			msg_write(AsmCodeLength);
							msg_write(d2h(ac,AsmCodeLength,false));
							msg_write(GetAsm(ac,AsmCodeLength));*/
							for (i=0;i<AsmCodeLength;i++)
								OCAddChar(ac[i]);
						}else{
							DoError("error in assembler code...",-1);
							return ret;
						}
						delete(pre_script->AsmBlock[0]);
						for (int i=0;i<pre_script->NumAsms-1;i++){
							pre_script->AsmBlock[i]=pre_script->AsmBlock[i+1];
							pre_script->AsmLine[i]=pre_script->AsmLine[i+1];
						}
						pre_script->NumAsms--;
				//		msg_write("msA------------------");
//#endif
//						msg_error("zu coden: OCAddCommand   Asm");
					}
					break;
				case CommandRectSet:
				case CommandColorSet:
					OCAddInstruction(this,inMovEaxM,pk[3],param[3]);
					OCAddInstruction(this,inMovMEax,rk,ret,12);
				case CommandVectorSet:
					OCAddInstruction(this,inMovEaxM,pk[0],param[0]);
					OCAddInstruction(this,inMovMEax,rk,ret);
					OCAddInstruction(this,inMovEaxM,pk[1],param[1]);
					OCAddInstruction(this,inMovMEax,rk,ret,4);
					OCAddInstruction(this,inMovEaxM,pk[2],param[2]);
					OCAddInstruction(this,inMovMEax,rk,ret,8);
					break;
				default:
 					DoError(string("compiler function unimplemented (call Michi!): ",CompilerFunction2Str(com->LinkNr)),-1);
					return ret;
			}
		}else{
 			DoErrorLink(string("compiler function not linkable: ",CompilerFunction2Str(com->LinkNr)),-1);
			return ret;
		}
	}else if (com->Kind==KindBlock){
		//msg_write("---block");
		OCAddBlock(pre_script->Block[com->LinkNr],n_func,level+1);
	}else{
		//msg_write("---???");
		DoError(string("type of command is unimplemented (call Michi!): ",Kind2Str(com->Kind)),-1);
	}
	//msg_write(Kind2Str(com->Kind));
	//msg_ok();
	left();
	return ret;
}

void CScript::OCAddBlock(sBlock *block,int n_func,int level)
{
	so("Block");
	right();
	int i;
	NumJumps[level]=0;
	for (i=0;i<block->NumCommands;i++){
		//msg_write(string2("%d - %d",i,block->NumCommands));
		LOffset=0;
		OCOffset[level][i]=OpcodeSize;
		OCAddCommand(pre_script->Command[block->Command[i]],n_func,level,i);
		if (Error)	return;
		for (int j=0;j<NumJumps[level];j++)
			if ((JumpCode[level][j]<0)&&(JumpSource[level][j]==i+1)){
				//int d=OCOffset[level][JumpDest[level][j]]-OpcodeSize-5;//-2;
				//so(string("Jump....",i2s(d)));
				//OCAddChar((char)0xeb);	OCAddChar(char(0));	JumpCode[level][j]=OpcodeSize-1;
				OCAddChar((char)0xe9);	OCAddInt(0);        	JumpCode[level][j]=OpcodeSize-4;
			}
	}
	OCOffset[level][block->NumCommands]=OpcodeSize;
	
	for (i=0;i<NumJumps[level];i++){
		//if (JumpCode[level][i]>=0)
		    int d=int(OCOffset[level][JumpDest[level][i]])-int(OCOffset[level][JumpSource[level][i]]);
			//Opcode[JumpCode[level][i]]=char(d);
			*(int*)&Opcode[JumpCode[level][i]]=int(d);
			so(string("Jump.... ",i2s(JumpSource[level][i]),"->",i2s(JumpDest[level][i]),": ",i2s(d)));
	}
	left();
}

// Opcode generieren
void CScript::Compiler()
{
	int nf,OCORA;
	if (Error)	return;
	so("Compiler");
	right();
	int i,f;

	// Speicherbedarf ermitteln
	MemorySize=0;
	for (i=0;i<pre_script->RootOfAllEvil.NumVars;i++)
		MemorySize+=pre_script->RootOfAllEvil.VarType[i]->Size+(4-pre_script->RootOfAllEvil.VarType[i]->Size%4)%4;
	for (i=0;i<pre_script->NumConstants;i++){
		int s=pre_script->Constant[i]->type->Size;
		if (pre_script->Constant[i]->type==&TypeString)
			// variable Laenge
			s=strlen(pre_script->Constant[i]->data)+1;
		MemorySize+=s+(4-s%4)%4;
	}
	Memory=new char[MemorySize];

	MemorySize=0;
	// globale Variablen in den Speicher...
	so("glob.Var.");
	for (i=0;i<pre_script->RootOfAllEvil.NumVars;i++){
		if (pre_script->FlagOverwriteVariablesOffset)
			g_var[i]=(char*)(MemorySize+pre_script->VariablesOffset);
		else
			g_var[i]=&Memory[MemorySize];
		so(string(i2s(MemorySize),":  ",pre_script->RootOfAllEvil.VarName[i]));
		MemorySize+=pre_script->RootOfAllEvil.VarType[i]->Size+(4-pre_script->RootOfAllEvil.VarType[i]->Size%4)%4;
	}
	ZeroMemory(Memory,MemorySize); // reset all global variables
	// Konstanten ebenso
	so("Konstanten");
	for (i=0;i<pre_script->NumConstants;i++){
		cnst[i]=&Memory[MemorySize];
		int s=pre_script->Constant[i]->type->Size;
		if (pre_script->Constant[i]->type==&TypeString)
			// variable Laenge
			s=strlen(pre_script->Constant[i]->data)+1;
		memcpy(&Memory[MemorySize],(void*)pre_script->Constant[i]->data,s);
		MemorySize+=s+(4-s%4)%4;
	}


	OpcodeSize=0;
	//Opcode=new char[SCRIPT_MAX_OPCODE];
// "Task" fuer das erste Ausfuehren der main-Funktion
	if ((!pre_script->FlagCompileOS)&&(!pre_script->FlagCompileInitialRealMode)){
		FirstExecution=(t_func*)&Opcode[OpcodeSize];
		// Intro
		OCAddInstruction(this,inPushEbp,-1); // im eigentlichen Programm
		OCAddInstruction(this,inMovEbpEsp,-1);
		OCAddInstruction(this,inMovEspM,KindConstant,(char*)&Stack[SCRIPT_STACK_SIZE]); // zum Anfang des Script-Stacks
		OCAddInstruction(this,inPushEbp,-1); // Adresse des alten Stacks
		OCAddEspAdd(this,-8); // Platz fuer Wait-Task-Daten lassen
		OCAddInstruction(this,inMovEbpEsp,-1);
		OCAddInstruction(this,inMovEaxM,KindConstant,(char*)WaitingModeNone); // "Reset"
		OCAddInstruction(this,inMovMEax,KindVarGlobal,(char*)&WaitingMode);
		// Aufruf
		nf=-1;
		for (f=0;f<pre_script->NumFunctions;f++)
			if (strcmp(pre_script->Function[f]->Name,"main")==0)
				nf=f;
		if (nf>=0)
			OCAddInstruction(this,inCallRel32,KindConstant,NULL);
		OCORA=OCOParam;
		TaskReturnOffset=OpcodeSize;
		// Outro
		OCAddEspAdd(this,8); // Platz fuer Wait-Task-Daten lassen
		OCAddInstruction(this,inPopEsp,-1);
		OCAddInstruction(this,inMovEbpEsp,-1);
		OCAddInstruction(this,inLeave,-1);
		OCAddInstruction(this,inRet,-1);

	// "Task" fuer das Ausfuehren nach einem Wait
		ContinueExecution=(t_func*)&Opcode[OpcodeSize];
		// Intro
		OCAddInstruction(this,inPushEbp,-1); // im eigentlichen Programm
		OCAddInstruction(this,inMovEbpEsp,-1);
		OCAddInstruction(this,inMovMEbp,KindVarGlobal,&Stack[SCRIPT_STACK_SIZE-4]);
		OCAddInstruction(this,inMovEspM,KindConstant,&Stack[SCRIPT_STACK_SIZE-12]); // zum eIP des Scriptes
		OCAddInstruction(this,inPopEax,-1);
		OCAddInstruction(this,inAddEaxM,KindConstant,(char*)AfterWaitOCSize);
		OCAddInstruction(this,inJmpEax,-1);
		//OCAddInstruction(this,inLeave,-1);
		//OCAddInstruction(this,inRet,-1);
		/*OCAddChar(0x90);
		OCAddChar(0x90);
		OCAddChar(0x90);*/
	}else{
		nf=-1;
		for (f=0;f<pre_script->NumFunctions;f++)
			if (strcmp(pre_script->Function[f]->Name,"main")==0)
				nf=f;
		// Aufruf
		if (nf>=0)
			OCAddInstruction(this,inCallRel32,KindConstant,NULL);
		OCORA=OCOParam;

		// Strings in den Opcode setzen!
		for (i=0;i<pre_script->NumConstants;i++){
			if ((pre_script->FlagCompileOS)||(pre_script->Constant[i]->type==&TypeString)){
				int offset=0;
				if (pre_script->AsmMetaInfo)
					offset=((sAsmMetaInfo*)pre_script->AsmMetaInfo)->CodeOrigin;
				cnst[i]=(char*)(OpcodeSize+offset);
				int s=pre_script->Constant[i]->type->Size;
				if (pre_script->Constant[i]->type==&TypeString)
					s=strlen(pre_script->Constant[i]->data)+1;
				memcpy(&Opcode[OpcodeSize],(void*)pre_script->Constant[i]->data,s);
				OpcodeSize+=s;
			}
		}
	}


// Code der einzelnen Funktionen
	so("Funktionen");
	for (f=0;f<pre_script->NumFunctions;f++){
		right();
		for (i=0;i<pre_script->Function[f]->NumVars;i++)
			so(pre_script->Function[f]->VarOffset[i]);
		so(pre_script->Function[f]->Name);
		func[f]=(t_func*)&Opcode[OpcodeSize];
		LOffset=LOffsetMax=0;

		// Intro
		OCAddInstruction(this,inPushEbp,-1);
		OCAddInstruction(this,inMovEbpEsp,-1);

		// Funktion
		OCAddBlock(pre_script->Function[f]->Block,f,0);

		// Outro
		OCAddInstruction(this,inLeave,-1);
		if (pre_script->Function[f]->Type->Size>4)
			OCAddInstruction(this,inRet4,-1);
		else
			OCAddInstruction(this,inRet,-1);

		left();
	}
	if (!Error)
		if (pre_script->AsmMetaInfo)
			if (((sAsmMetaInfo*)pre_script->AsmMetaInfo)->NumWantedLabels>0){
				DoError(string("unknown name in assembler code:  \"",((sAsmMetaInfo*)pre_script->AsmMetaInfo)->WantedLabelName[0],"\""),-1);
				return;
			}
	if (nf>=0){
		int lll=((int)func[nf]-(int)&Opcode[TaskReturnOffset]);
		if (pre_script->FlagCompileOS)
			if (pre_script->FlagCompileInitialRealMode)
				lll+=0;
			else
				lll-=2;
		else
			lll+=0;
		//msg_write(lll);
		//msg_write(d2h(&lll,4,false));
		*(int*)&Opcode[OCORA]=lll;
	}

	//msg_db_out(1,GetAsm(Opcode,OpcodeSize));

	//_expand(Opcode,OpcodeSize);

	WaitingMode=WaitingModeNone;
	msg_write("--------------------------------");
	msg_write(string2("Opcode: %db, Memory: %db",OpcodeSize,MemorySize));
	left();
}

void CScript::OCAddChar(int c)
{	Opcode[OpcodeSize]=(char)c;	OpcodeSize++;	}

void CScript::OCAddWord(int i)
{	*(short*)&Opcode[OpcodeSize]=i;	OpcodeSize+=2;	}

void CScript::OCAddInt(int i)
{	*(int*)&Opcode[OpcodeSize]=i;	OpcodeSize+=4;	}

CPreScript::CPreScript()
{
	int i;
	ZeroMemory(this,sizeof(CPreScript));
	strcpy(RootOfAllEvil.Name,"RootOfAllEvil");

	NumTypes=NumPreTypes;
	for (i=0;i<NumPreTypes;i++)
		Type[i]=PreType[i];
	NumStructs=NumPreStructs;
	for (i=0;i<NumPreStructs;i++)
		Struct[i]=PreStruct[i];
	NumPreScriptRules=0;
}

CScript::CScript()
{
	so("creating empty script (for console)");
	right();

	Error=false;

	MemorySize=0;
	Memory=new char[4096];

	pre_script=new CPreScript();

	so("-ok");
	left();
}

/*static char ConsoleOpcode[SCRIPT_MAX_OPCODE/8];

void CScript::ExecuteSingleCommand(char *cmd)
{
	if (strlen(cmd)<1)
		return;
	msg_write(string("script command: ",cmd));

	Error=false;
// find all expressions
	pre_script->MakeExps(cmd,false);
	if (Exp->NumExps<1)
		return;

	// syntax analysis
	Error=false;
	int i;
	int nc=pre_script->NumCommands;
	int ncs=pre_script->NumConstants;
	char *opcode=Opcode;
	char *memory;
	int memory_size=0;
	shift_right=0;
	Exp->ExpNr=0;

	// retieve script data
	int nf=pre_script->NumFunctions;
	int nb=pre_script->NumBlocks;
	pre_script->AddFunction("_no_name_");
	if (pre_script->Error)
		return;
	//pre_script->GetCommand(Exp->ExpNr,0,";",0,pre_script->Function[nf]);
	pre_script->GetCompleteCommand(pre_script->Function[nf]->Block);
	if (nc==pre_script->NumCommands){
		delete(pre_script->Function[nf]->Block);
		delete(pre_script->Function[nf]);
		return;
	}

	// minimally compile
	if (!Error){

		// memory usage (only constants)
		for (i=ncs;i<pre_script->NumConstants;i++)
			memory_size+=int(int(pre_script->Constant[i]->type->Size+3)/4)*4;
		memory=new char[memory_size];
		memory_size=0;
		for (i=ncs;i<pre_script->NumConstants;i++){
			memcpy(&memory[memory_size],(void*)pre_script->Constant[i]->data,pre_script->Constant[i]->type->Size);
			cnst[i]=&memory[memory_size];
			memory_size+=int(int(pre_script->Constant[i]->type->Size+3)/4)*4;
		}
		//func[nf]=(t_func*)&Opcode[OpcodeSize];
		LOffset=LOffsetMax=0;

		// switch buffers to save script's opcode
		opcode=Opcode;
		OpcodeSize=0;
		Opcode=ConsoleOpcode;

		// intro
		OCAddInstruction(this,inPushEbp,-1);
		OCAddInstruction(this,inMovEbpEsp,-1);

		// command
		OCAddBlock(pre_script->Function[nf]->Block,nf,0);
		//OCAddCommand(pre_script->Command[nc],nf,0,0);

		// outro
		OCAddInstruction(this,inLeave,-1);
		OCAddInstruction(this,inRet,-1);

		// reswitch buffers
		Opcode=opcode;

		left();
	}

	// execute
	if (!Error){
		t_func *f=(t_func*)&ConsoleOpcode[0];
		f();
	}

	// clean up
	for (i=ncs;i<pre_script->NumConstants;i++){
		delete(pre_script->Constant[i]->data);
		delete(pre_script->Constant[i]);
	}
	for (i=nc;i<pre_script->NumCommands;i++)
		delete(pre_script->Command[i]);
	delete(pre_script->Function[pre_script->NumFunctions]);
	pre_script->NumCommands=nc;
	pre_script->NumConstants=nc;
}*/


void CScript::ExecuteSingleCommand(char *cmd)
{
	if (strlen(cmd)<1)
		return;
	msg_write(string("script command: ",cmd));

	pre_script->Error=Error=false;
// find expressions
	pre_script->MakeExps(cmd,false);
	if (pre_script->Exp->NumExps<1)
		return;

	// analyse syntax
	//Error=false;
	int i;
	int nc=pre_script->NumCommands;
	int ocs=OpcodeSize;
	int mms=MemorySize;
	int ncs=pre_script->NumConstants;
	shift_right=0;
	pre_script->Exp->ExpNr=0;

	pre_script->Function[pre_script->NumFunctions]=new sFunction;
	pre_script->Function[pre_script->NumFunctions]->NumVars=0;
	pre_script->GetCommand(pre_script->Exp->ExpNr,0,0,pre_script->Function[pre_script->NumFunctions]);
	//pre_script->GetCompleteCommand((pre_script->Exp->ExpNr,0,0,pre_script->Function[pre_script->NumFunctions]);
	Error|=pre_script->Error;

	// minimally compile
	if ((!Error)&&(nc!=pre_script->NumCommands)){
		for (i=ncs;i<pre_script->NumConstants;i++){
			memcpy(&Memory[MemorySize],(void*)pre_script->Constant[i]->data,pre_script->Constant[i]->type->Size);
			cnst[i]=&Memory[MemorySize];
			MemorySize+=pre_script->Constant[i]->type->Size+(4-pre_script->Constant[i]->type->Size%4)%4;
		}
		func[pre_script->NumFunctions]=(t_func*)&Opcode[OpcodeSize];
		LOffset=LOffsetMax=0;

		// intro
		OCAddInstruction(this,inPushEbp,-1);
		OCAddInstruction(this,inMovEbpEsp,-1);

		// command
		OCAddCommand(pre_script->Command[nc],pre_script->NumFunctions,0,0);

		// outro
		OCAddInstruction(this,inLeave,-1);
		OCAddInstruction(this,inRet,-1);

		left();
	}
	if ((!Error)&&(nc!=pre_script->NumCommands)){
		t_func *f=(t_func*)&Opcode[ocs];
		f();
	}

	// clean
	for (i=ncs;i<pre_script->NumConstants;i++){
		delete(pre_script->Constant[i]->data);
		delete(pre_script->Constant[i]);
	}
	for (i=nc;i<pre_script->NumCommands;i++)
		delete(pre_script->Command[i]);
	delete(pre_script->Function[pre_script->NumFunctions]);
	pre_script->NumCommands=nc;
	OpcodeSize=ocs;
	MemorySize=mms;
	pre_script->NumConstants=nc;
}

bool CScript::ExecuteScriptFunction(char *name,...)
{
	/*msg_db_out(2,"-ExecuteScriptFunction");
	msg_db_out(2,name);
	msg_db_out(2,FileName);*/
//#ifdef FILE_OS_WINDOWS

	if ((pre_script->GetExistence(name,&pre_script->RootOfAllEvil))&&(pre_script->GetExistenceLink.Kind==KindFunction)){

		sFunction *f=pre_script->Function[pre_script->GetExistenceLink.Nr];

		if (f->NumParams>0){
			int p;

			va_list marker;
			va_start(marker,name);
			char *param[SCRIPT_MAX_PARAMS];
			int pk[SCRIPT_MAX_PARAMS];
			for (p=0;p<f->NumParams;p++){
				param[p]=va_arg(marker,char*);
				pk[p]=KindVarGlobal;
				if (f->VarType[p]->IsPointer)
					pk[p]=KindConstant;
			}
			va_end(marker);

			int oc_size=OpcodeSize;
			t_func *code=(t_func*)&Opcode[OpcodeSize];
			OCAddInstruction(this,inPushEbp,-1);
			OCAddInstruction(this,inMovEbpEsp,-1);

			// Stack um die lokalen Variablen der bisherigen Funktion erniedrigen
			int dp=0;
			for (p=f->NumParams-1;p>=0;p--){
				int s=f->VarType[p]->Size;
				s=s+(4-s%4)%4;
				// Parameter auf den Stack pushen
				for (int j=0;j<s/4;j++)
					OCAddInstruction(this,inPush,pk[p],param[p],s-4-j*4);
				dp+=s;
			}

			OCAddInstruction(this,inCallRel32,KindConstant,(char*)((int)func[pre_script->GetExistenceLink.Nr]-(int)&Opcode[OpcodeSize]-5));
			OCAddEspAdd(this,dp+8);

			OCAddInstruction(this,inLeave,-1);
			OCAddInstruction(this,inRet,-1);
			code();
			OpcodeSize=oc_size;
		}else
			func[pre_script->GetExistenceLink.Nr]();
		return true;
	}
//#endif
	//msg_ok();
	return false;
}

void CPreScript::ShowLink(sLinkData *link)
{
	msg_right();
	msg_write(string(Kind2Str(link->Kind),", ",LinkNr2Str(this,link->Kind,link->Nr)));
	msg_write(Type2Str(this,link->type));
	if ((link->Kind==KindPointerShift)||(link->Kind==KindArray)||(link->Kind==KindPointerAsArray)||(link->Kind==KindReference)||(link->Kind==KindDereference)){
		msg_write("Meta-Link");
		ShowLink(link->Meta);
	}
	if ((link->Kind==KindArray)||(link->Kind==KindPointerAsArray)){
		msg_write("Param-Link");
		ShowLink(link->ParamLink);
	}
	if (link->Kind==KindCommand){
		ShowCommand(link->Nr);
	}
	msg_left();
}

void CPreScript::ShowCommand(int c)
{
	msg_write(string(i2s(c),": ",Kind2Str(Command[c]->Kind),", ",LinkNr2Str(this,Command[c]->Kind,Command[c]->LinkNr)));
	msg_right();
	msg_write(string("Return: ",Type2Str(this,Command[c]->ReturnLink.type)));
	for (int p=0;p<Command[c]->NumParams;p++){
		msg_write("Parameter");
		ShowLink(&Command[c]->ParamLink[p]);
	}
	msg_left();
	msg_write("");
}

void CPreScript::ShowBlock(int b)
{
	msg_right();
	for (int c=0;c<Block[b]->NumCommands;c++){
		//msg_write(Block[b]->Command[c]);
		if (Command[Block[b]->Command[c]]->Kind==KindBlock)
			ShowBlock(Command[Block[b]->Command[c]]->LinkNr);
		else
			ShowCommand(Block[b]->Command[c]);
	}
	msg_left();
}

void CPreScript::ShowFunction(int f)
{
	msg_write(string(i2s(f),": ",Function[f]->Name,"  --------------------------------------------"));
	ShowBlock(Function[f]->Block->Nr);
}

void CPreScript::Show()
{
	//if (Error)	return;
	msg_write("\n\n\n################### Darstellung ######################\n\n\n");
	msg_write(Filename);
	/*msg_write("Befehle:\n");
	msg_right();
	for (int c=0;c<NumCommands;c++)
		ShowCommand(c);
	msg_left();*/
	msg_write("\nFunktionen:\n");
	msg_right();
	for (int f=0;f<NumFunctions;f++)
		ShowFunction(f);
	msg_left();
	msg_write("\n\n");
}

void CScript::ShowVars(bool include_consts)
{
	
	int ss=0;
	int i;
	char name[12];
	sType *t;
	int n=pre_script->RootOfAllEvil.NumVars;
	if (include_consts)
		n+=pre_script->NumConstants;
	for (i=0;i<n;i++){
		char *add=(char*)&Stack[ss];
		if (i<pre_script->RootOfAllEvil.NumVars){
			strcpy(name,pre_script->RootOfAllEvil.VarName[i]);
			t=pre_script->RootOfAllEvil.VarType[i];
		}else{
			strcpy(name,"---const---");
			t=pre_script->Constant[i-pre_script->RootOfAllEvil.NumVars]->type;
		}
		if (t==&TypeInt)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  ",i2s(*(int*)&Stack[ss])));
		else if (t==&TypeFloat)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  ",f2s(*(float*)&Stack[ss],3)));
		else if (t==&TypeBool)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =(bool)  ",i2s(Stack[ss])));
		else if (t==&TypeVector)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  (",string(f2s(*(float*)&Stack[ss],3)," , ",f2s(*(float*)&Stack[ss+4],3)," , ",f2s(*(float*)&Stack[ss+8],3),")")));
		else if ((t==&TypeColor)||(t==&TypeRect)||(t==&TypeQuaternion))
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  (",string(f2s(*(float*)&Stack[ss],3)," , ",f2s(*(float*)&Stack[ss+4],3),string(" , ",f2s(*(float*)&Stack[ss+8],3)," , ",f2s(*(float*)&Stack[ss+12],3),")"))));
		else if (t->IsPointer)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  ",d2h(&Stack[ss],4,false)));
		else if (t==&TypeString)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  \"",&Stack[ss],"\""));
		else
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  ??? (unbekannter Typ)"));
		ss+=t->Size;
	}
}

void CScript::Execute()
{
	if (Error)	return;
	#ifdef ScriptDebug
		//so("\n\n\n################### fuehre aus ######################\n\n\n");
	#endif
	shift_right=0;
	msg_db_out(1,string("Execute ",Filename));

	// handle wait-commands
	if (WaitingMode!=WaitingModeNone){
#ifdef _X_ALLOW_META_
		if (WaitingMode==WaitingModeRT)
			TimeToWait-=ElapsedRT;
		else
			TimeToWait-=Elapsed;
		if (TimeToWait>0)
			return;
#endif
		WaitingMode=WaitingModeNone;
		//msg_write(ThisObject);
		msg_db_out(1,"->Continue");
		//msg_write(">---");
		//msg_right();
		ContinueExecution();
		//msg_write("---<");
		msg_db_out(1,"/continue");
		//msg_write("ok");
		//msg_left();
	}else{
		msg_db_out(1,"->First");
		//msg_right();
		FirstExecution();
		msg_db_out(1,"/first");
		//msg_left();
	}
}
