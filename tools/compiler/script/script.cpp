/*----------------------------------------------------------------------------*\
| CScript                                                                      |
| -> C-like scripting system                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2010.02.14 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "script.h"
#include "../file/msg.h"
#include <malloc.h>
#ifdef FILE_OS_WINDOWS
	#include <windows.h>
#endif
#ifdef FILE_OS_LINUX
	#include <stdarg.h>
	#include <sys/mman.h>
#endif
	#include <stdio.h>

#include "../00_config.h"
#ifdef _X_ALLOW_META_
	#include "../x/x.h"
#endif

char ScriptVersion[] = "0.5.3.0 -alpha-";

//#define ScriptDebug

static int GlobalWaitingMode;
static float GlobalTimeToWait;

static char *GlobalOpcode = NULL;
static int GlobalOpcodeSize;




static int shift_right=0;

static void stringout(char *str)
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

static void right()
{
#ifdef ScriptDebug
	msg_right();
	shift_right+=2;
#endif
}

static void left()
{
#ifdef ScriptDebug
	msg_left();
	shift_right-=2;
#endif
}


int NumPublicScripts=0;
struct sPublicScript{
	char *filename;
	CScript *script;
}PublicScript[256];


char ScriptDirectory[512]="";




CScript *LoadScript(char *filename,bool is_public)
{
	//msg_write(string("Lade ",filename));
	if (strlen(filename)<1){
		msg_error("no script file!");
		return NULL;
	}
	CScript *s=NULL;

	// public und private aus dem Speicher versuchen zu laden
	if (is_public){
		for (int i=0;i<NumPublicScripts;i++)
			if (strcmp(PublicScript[i].filename,SysFileName(filename))==0)
				return PublicScript[i].script;
	}
#if 0
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
		am("CScript",sizeof(CScript),s);
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
#endif

	s=new CScript(filename);
	am("CScript",sizeof(CScript),s);
	s->isPrivate=!is_public;

	// nur public speichern
	if (is_public){
		PublicScript[NumPublicScripts].filename=new char[strlen(filename)+1];
		am("PublicScript.filename",strlen(filename)+1,PublicScript[NumPublicScripts].filename);
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
	msg_db_r("LoadAsInclude",4);
	//msg_write(string("Include ",filename));
	// aus dem Speicher versuchen zu laden
	for (int i=0;i<NumPublicScripts;i++)
		if (strcmp(PublicScript[i].filename,SysFileName(filename))==0){
			//msg_write("...pointer");
			msg_db_l(4);
			return PublicScript[i].script;
		}

	//msg_write("nnneu");
	CScript *s=new CScript(filename,just_analyse);
	am("script",sizeof(CScript),s);
	so("geladen....");
	//msg_write("...neu");
	s->isPrivate=false;

	// als public speichern
	PublicScript[NumPublicScripts].filename=new char[strlen(filename)+1];
	am("pub.filename",strlen(filename)+1,PublicScript[NumPublicScripts].filename);
	strcpy(PublicScript[NumPublicScripts].filename,SysFileName(filename));
	PublicScript[NumPublicScripts++].script=s;

	msg_db_l(4);
	return s;
}

void ExecutePublicScripts()
{
}

void DeletePublicScripts(bool even_immortal)
{
	for (int i=NumPublicScripts-1;i>=0;i--)
		if ((!PublicScript[i].script->pre_script->FlagImmortal) || (even_immortal)){
			delete[](PublicScript[i].filename);
			dm("pub.filename",PublicScript[i].filename);
			delete(PublicScript[i].script);
			dm("script",PublicScript[i].script);
			NumPublicScripts--;
			for (int j=i;j<NumPublicScripts;j++)
				PublicScript[j]=PublicScript[j+1];
		}

	ScriptResetSemiExternalVars();

	
	/*msg_write("------------------------------------------------------------------");
	msg_write(mem_used);
	for (int i=0;i<num_ps;i++)
		msg_write(string2("  fehlt:   %s  %p  (%d)",ppn[i],ppp[i],pps[i]));
	*/
	om();
}

CScript::CScript(char *filename,bool just_analyse)
{
	msg_db_r("CScript",4);
	memset(this,0,sizeof(CScript));
	cur_script = this;
	strcpy(Filename,filename);
	Error = false;
	isCopy = false;
	JustAnalyse = just_analyse;
	msg_write(string("loading script: ",Filename));
	msg_right();

	WaitingMode = WaitingModeFirst;

	pre_script=new CPreScript(Filename,just_analyse);
	am("pre_script",sizeof(CPreScript),pre_script);
	if (pre_script->FlagShow)
		pre_script->Show();
	ParserError=Error=pre_script->Error;
	LinkerError=pre_script->IncludeLinkerError;
	ErrorLine=pre_script->ErrorLine;
	ErrorColumn=pre_script->ErrorColumn;
	strcpy(ErrorMsg,pre_script->ErrorMsg);
	strcpy(ErrorMsgExt[0],pre_script->ErrorMsgExt[0]);
	strcpy(ErrorMsgExt[1],pre_script->ErrorMsgExt[1]);

	if ((!Error)&&(!JustAnalyse))
		Compiler();
	/*if (pre_script->FlagShow)
		pre_script->Show();*/
	if (pre_script->FlagDisassemble){
		printf("%s\n\n", Opcode2Asm(ThreadOpcode,ThreadOpcodeSize));
		printf("%s\n\n", Opcode2Asm(Opcode,OpcodeSize));
		//msg_write(Opcode2Asm(Opcode,OpcodeSize));
	}

	msg_ok();
	msg_left();
	msg_db_l(4);
}

void CScript::SetVariable(char *name, void *data)
{
	for (int i=0;i<pre_script->RootOfAllEvil.Var.size();i++)
		if (strcmp(pre_script->RootOfAllEvil.Var[i].Name, name) == 0){
			memcpy(g_var[i], data, pre_script->RootOfAllEvil.Var[i].Type->Size);
			return;
		}
	msg_error(string("CScript::SetVariable: variable ", name, " not found"));
}

#define MAX_JUMPS			64
#define MAX_JUMP_LEVELS		128

int LOffset,LOffsetMax,OCOffset[MAX_JUMP_LEVELS][MAX_JUMP_LEVELS],TaskReturnOffset;
int NumJumps[MAX_JUMP_LEVELS],JumpSourceOffset[MAX_JUMP_LEVELS][MAX_JUMPS],JumpDest[MAX_JUMP_LEVELS][MAX_JUMPS],JumpCode[MAX_JUMPS][MAX_JUMPS];
int NumNewJumps,NewJumpSourceLevel[MAX_JUMPS],NewJumpSource[MAX_JUMPS],NewJumpDest[MAX_JUMPS],NewJumpDestLevel[MAX_JUMPS],NewJumpCode[MAX_JUMPS];
inline void add_jump(int source_level, int source, int dest_level, int dest, int code)
{
	NewJumpSourceLevel[NumNewJumps] = source_level;
	NewJumpSource[NumNewJumps] = source;
	NewJumpDestLevel[NumNewJumps] = dest_level;
	NewJumpDest[NumNewJumps] = dest;
	NewJumpCode[NumNewJumps] = code;
	NumNewJumps ++;
}
inline void insert_new_jump(int n, int offset)
{
	int level = NewJumpDestLevel[n];
	JumpSourceOffset[level][NumJumps[level]] = offset;
	JumpDest[level][NumJumps[level]] = NewJumpDest[n];
	JumpCode[level][NumJumps[level]] = NewJumpCode[n];
	for (int i=n;i<NumNewJumps;i++){
		NewJumpSourceLevel[i] = NewJumpSourceLevel[i + 1];
		NewJumpSource[i] = NewJumpSource[i + 1];
		NewJumpDestLevel[i] = NewJumpDestLevel[i + 1];
		NewJumpDest[i] = NewJumpDest[i + 1];
		NewJumpCode[i] = NewJumpCode[i + 1];
	}
	NumNewJumps --;
	NumJumps[level] ++;
}

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



void OCAddChar(char *oc,int &ocs,int c)
{	oc[ocs]=(char)c;	ocs++;	}

void OCAddWord(char *oc,int &ocs,int i)
{	*(short*)&oc[ocs]=i;	ocs+=2;	}

void OCAddInt(char *oc,int &ocs,int i)
{	*(int*)&oc[ocs]=i;	ocs+=4;	}

int OCOParam;
void OCAddInstruction(char *oc,int &ocs,int inst,int kind,void *param=NULL,int offset=0,int insert_at=-1)
{
	if (insert_at<0)
		insert_at=ocs;
	int insert_length=0;
	//char insert_oc[128];
	if ((kind!=KindRefToLocal)&&(kind!=KindRefToGlobal))
		param = (void*)((long)param + offset);
	int l=8;
	if (((long)param>125)||((long)param<-125))	l=32; // l=16;
	//if (((int)param>65530)||((int)param<-65530))	l=32;
	short *p_oc=&AsmInstruction[inst].oc_g[0];
	if ((kind==KindVarLocal)&&(l==8))
		p_oc=&AsmInstruction[inst].oc_l8[0];
	if ((kind==KindVarLocal)&&(l==32))
		p_oc=&AsmInstruction[inst].oc_l32[0];
	if ((kind==KindConstant)||(kind==KindRefToConst)){
		if (AsmInstruction[inst].oc_c[0]>=0)
			p_oc=&AsmInstruction[inst].oc_c[0];
		else
			kind=KindVarGlobal;
	}
	if (kind<0)
		p_oc=&AsmInstruction[inst].oc_l8[0];
	if ((kind==KindRefToLocal)||(kind==KindRefToGlobal)){
		if (kind==KindRefToLocal)
			OCAddInstruction(oc,ocs,inMovEdxM,KindVarLocal,param);
		if (kind==KindRefToGlobal)
			OCAddInstruction(oc,ocs,inMovEdxM,KindVarGlobal,param);
		if (offset!=0)
			OCAddInstruction(oc,ocs,inAddEdxM,KindConstant,(char*)offset);
		p_oc=&AsmInstruction[inst].oc_dr[0];
	}

	if (p_oc[0]>=0)	OCAddChar(oc,ocs,(char)p_oc[0]);
	if (p_oc[1]>=0)	OCAddChar(oc,ocs,(char)p_oc[1]);
	if (p_oc[2]>=0)	OCAddChar(oc,ocs,(char)p_oc[2]);
	OCOParam=ocs;
	if (kind==KindVarLocal){ // local
		if (l==8)
			OCAddChar(oc,ocs,(long)param);
//		else if (l==16)
//			OCAddWord(oc,ocs,(long)param);
		else if (l==32)
			OCAddInt(oc,ocs,(long)param);
	}else if (kind==KindVarGlobal)
		OCAddInt(oc,ocs,(long)param); // global
	else if (kind==KindConstant){
		if (AsmInstruction[inst].const_size==8)
			OCAddChar(oc,ocs,(long)param);
		else if (AsmInstruction[inst].const_size==16)
			OCAddWord(oc,ocs,(long)param);
		else if (AsmInstruction[inst].const_size==32)
			OCAddInt(oc,ocs,(long)param);
	}else if (kind==KindRefToConst){
		if (AsmInstruction[inst].const_size==8)
			OCAddChar(oc,ocs,*(int*)param);
		else if (AsmInstruction[inst].const_size==16)
			OCAddWord(oc,ocs,*(int*)param);
		else if (AsmInstruction[inst].const_size==32)
			OCAddInt(oc,ocs,*(int*)param);
	}

	p_oc=&AsmInstruction[inst].oc_p;
	if (p_oc[0]>=0)	OCAddChar(oc,ocs,(char)p_oc[0]);
	/*if (p_oc[0]>=0){	insert_oc[insert_length]=(char)p_oc[0];	insert_length++;	}
	if (p_oc[1]>=0){	insert_oc[insert_length]=(char)p_oc[1];	insert_length++;	}
	if (p_oc[2]>=0){	insert_oc[insert_length]=(char)p_oc[2];	insert_length++;	}
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

	p_oc=&AsmInstruction[inst].oc_p;
	if (p_oc[0]>=0){	insert_oc[insert_length]=(char)p_oc[0];	insert_length++;	}

	int i;
	for (i=s->OpcodeSize-1;i>=insert_at;i--)
		oc[i+insert_length]=oc[i];
	for (i=0;i<insert_length;i++)
		oc[insert_at+i]=insert_oc[i];
	ocs+=insert_length;*/
}

void OCAddEspAdd(char *oc,int &ocs,int d)
{
	if (d>0){
		if (d>120){
			OCAddChar(oc,ocs,(char)0x81);
			OCAddChar(oc,ocs,(char)0xc4);
			OCAddInt(oc,ocs,d);
		}else{
			OCAddChar(oc,ocs,(char)0x83);
			OCAddChar(oc,ocs,(char)0xc4);
			OCAddChar(oc,ocs,(char)d);
		}
	}else if (d<0){
		if (d<-120){
			OCAddChar(oc,ocs,(char)0x81);
			OCAddChar(oc,ocs,(char)0xec);
			OCAddInt(oc,ocs,-d);
		}else{
			OCAddChar(oc,ocs,(char)0x83);
			OCAddChar(oc,ocs,(char)0xec);
			OCAddChar(oc,ocs,(char)-d);
		}
	}
}

// create data for a (function) parameter
//   and compile its command if the parameter is executable itself
char *CScript::OCAddParameter(sLinkData *link, int n_func, int level, int index, int &pk, bool allow_auto_ref)
{
	msg_db_r("OCAddParameter", 4);
	pk = link->Kind;
	char *ret = NULL;
	//sType *rt=link->;
	//msg_write(Kind2Str(link->Kind));
	if (link->Kind == KindVarFunction){
		so(" -var-func");
		if (pre_script->FlagCompileOS)
			ret = (char*)((long)func[link->Nr] - (long)&Opcode[0] + ((sAsmMetaInfo*)pre_script->AsmMetaInfo)->CodeOrigin);
		else
			ret = (char*)func[link->Nr];
		pk = KindVarGlobal;
	}else if (link->Kind == KindVarGlobal){
		so(" -global");
		if (link->script)
			ret = link->script->g_var[link->Nr];
		else
			ret = g_var[link->Nr];
	}else if (link->Kind == KindVarLocal){
		so(" -local");
		ret = (char*)(int)pre_script->Function[n_func].Var[link->Nr].Offset;
	}else if (link->Kind == KindVarExternal){
		so(" -external-var");
		ret=(char*)PreExternalVar[link->Nr].Pointer;
		pk=KindVarGlobal;
		if (!ret){
			DoErrorLink(string("externe Variable nicht linkbar: ",PreExternalVar[link->Nr].Name));
			_return_(4, ret);
			/*msg_write("OCAddParameter  nur ein Verweis auf eine externe Variable?");
			ret=(char*)&PreExternalVar[link->Nr].Pointer;
			pk=KindRefToGlobal;*/
		}
	}else if (link->Kind==KindConstant){
		so(" -const");
		if ((UseConstAsGlobalVar)||(pre_script->FlagCompileOS))
			pk=KindVarGlobal;
		else
			pk=KindRefToConst;
		ret=cnst[link->Nr];
	}else if (link->Kind==KindCommand){
		pk=KindVarLocal;
		ret=OCAddCommand(&pre_script->Command[link->Nr],n_func,level,index);
		if (Error)	_return_(4, NULL);
	}else if (link->Kind==KindPointerShift){
		so(" -p.shift");
		char *param=OCAddParameter(link->Meta,n_func,level,index,pk);
		if (Error)	_return_(4, NULL);
		if ((pk==KindVarLocal)||(pk==KindVarGlobal)){
			so("  ->const");
			ret=param+link->Nr;
		}else{
			so("  ->lea");
			if (pk!=KindRefToLocal){
				DoErrorLink(string("unexpected meta type for pointer shift: ",Kind2Str(pk)));
				_return_(4, NULL);
			}
			ret=param;
			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,KindConstant,(char*)(int)link->Nr);
			OCAddInstruction(Opcode,OpcodeSize,inAddMEax,KindVarLocal,ret);
			if (Error)	_return_(4, NULL);
			pk=KindRefToLocal;
		}
	}else if (link->Kind==KindDerefPointerShift){
		so(" -deref-shift");
		char *param=OCAddParameter(link->Meta,n_func,level,index,pk);
		if (Error)	_return_(4, NULL);
		ret=(char*)(-LOffset-pre_script->Function[n_func].VarSize-4);
		LOffset+=4;
		if (LOffset>LOffsetMax)
			LOffsetMax=LOffset;
		OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk,param);
		OCAddInstruction(Opcode,OpcodeSize,inAddEaxM,KindConstant,(char*)(int)link->Nr);
		OCAddInstruction(Opcode,OpcodeSize,inMovMEax,KindVarLocal,ret);
		if (Error)	_return_(4, NULL);
		pk=KindRefToLocal;
	}else if (link->Kind==KindArray){
		so(" -array");
		char *param=OCAddParameter(link->Meta,n_func,level,index,pk,false);
		if (Error) _return_(4, NULL);
		if ((link->ParamLink->Kind==KindConstant)&&( (pk==KindVarLocal)||(pk==KindVarGlobal) )){
			so("  ->const");
			ret=param+(*(int*)pre_script->Constant[link->ParamLink->Nr].data)*link->type->Size;
		}else{
			so("  ->lea");
			if (pk!=KindRefToLocal){
				so("    ->neu");
				OCAddInstruction(Opcode,OpcodeSize,inLeaEaxM,pk,param);
				ret=(char*)(-LOffset-pre_script->Function[n_func].VarSize-4);
				LOffset+=4;
				if (LOffset>LOffsetMax)
					LOffsetMax=LOffset;
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,KindVarLocal,ret);
			}else
				ret=param;
			param=OCAddParameter(link->ParamLink,n_func,level,index,pk,false);
			if (Error)	_return_(4, NULL);
			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk,param);
			OCAddInstruction(Opcode,OpcodeSize,inMulEaxM,KindConstant,(char*)link->type->Size);
			OCAddInstruction(Opcode,OpcodeSize,inAddMEax,KindVarLocal,ret);
			if (Error)	_return_(4, NULL);
			pk=KindRefToLocal;
		}
	}else if (link->Kind==KindPointerAsArray){
		so(" -pointer-array");
		char *param=OCAddParameter(link->Meta,n_func,level,index,pk,false);
		if (Error)	_return_(4, NULL);
		//so("->lea");
		//if (pk!=KindRefToLocal){
			so("  ->neu");
			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk,param);
			ret=(char*)(-LOffset-pre_script->Function[n_func].VarSize-4);
			LOffset+=4;
			if (LOffset>LOffsetMax)
				LOffsetMax=LOffset;
			OCAddInstruction(Opcode,OpcodeSize,inMovMEax,KindVarLocal,ret);
		//}else
		//	ret=param;
		param=OCAddParameter(link->ParamLink,n_func,level,index,pk,false);
		if (Error)	_return_(4, NULL);
		OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk,param);
		OCAddInstruction(Opcode,OpcodeSize,inMulEaxM,KindConstant,(char*)link->type->Size);
		OCAddInstruction(Opcode,OpcodeSize,inAddMEax,KindVarLocal,ret);
		if (Error)	_return_(4, NULL);
		pk=KindRefToLocal;
	}else if (link->Kind==KindReference){
		//msg_write(Kind2Str(link->Meta->Kind));
		so(" -ref");
		char *param=OCAddParameter(link->Meta,n_func,level,index,pk,false);
		if (Error)	_return_(4, NULL);
		//printf("%d  -  %s\n",pk,Kind2Str(pk));
		if ((pk==KindConstant)||(pk==KindVarGlobal)||(pk==KindRefToConst)){
			so("  Reference-Const");
			pk=KindConstant;
			ret=param;
		}else{
			ret=(char*)(-LOffset-pre_script->Function[n_func].VarSize-4);
			LOffset+=4;
			if (LOffset>LOffsetMax)
				LOffsetMax=LOffset;
			OCAddInstruction(Opcode,OpcodeSize,inLeaEaxM,pk,param);
			pk=KindVarLocal;
			OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk,ret);
			if (Error)	_return_(4, NULL);
		}
	}else if (link->Kind==KindDereference){
		so(" -deref...");
		char *param=OCAddParameter(link->Meta,n_func,level,index,pk);
		if (Error)	_return_(4, NULL);
		if ((pk==KindVarLocal)||(pk==KindVarGlobal)){
			if (pk==KindVarLocal)	pk=KindRefToLocal;
			if (pk==KindVarGlobal)	pk=KindRefToGlobal;
			ret=param;
		}
	}else
		_do_error_(string("unexpected type of parameter: ",Kind2Str(link->Kind)), 4, NULL);
	//if ((!link->type->IsPointer)&&(link->type->SubType)){
	if ((allow_auto_ref)&&(link->type->ArrayLength>0)){
		so("Array: c referenziert automatisch!!");
		char *param=ret;
		ret=(char*)(-LOffset-pre_script->Function[n_func].VarSize-4);
		LOffset+=4;
		if (LOffset>LOffsetMax)
			LOffsetMax=LOffset;
		OCAddInstruction(Opcode,OpcodeSize,inLeaEaxM,pk,param);
		pk=KindVarLocal;
		OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk,ret);
		if (Error)	_return_(4, NULL);
		link->type=TypePointer;
	}
	msg_db_l(4);
	return ret;
}

static int while_level = 0;
static int while_index = 0;

char *CScript::OCAddCommand(sCommand *com,int n_func,int level,int index)
{
	msg_db_r("OCAddCommand", 4);
	//msg_write(Kind2Str(com->Kind));
	sFunction *p_func = &pre_script->Function[n_func];
	char *param[SCRIPT_MAX_PARAMS];
	int s = mem_align(com->ReturnType->Size);
	char *ret = (char*)( -LOffset - p_func->VarSize - s);
	//so(d2h((char*)&ret,4,false));
	so(string2("return: %d/%d/%d", com->ReturnType->Size, LOffset, LOffset + s));
	LOffset += s;
	int pk[SCRIPT_MAX_PARAMS], rk = KindVarLocal; // param_kind, return_kind

	// compile parameters
	if (LOffset > LOffsetMax)
		LOffsetMax = LOffset;
	for (int p=0;p<com->NumParams;p++){
		param[p] = OCAddParameter(&com->ParamLink[p],n_func,level,index,pk[p]);
		if (Error) _return_(4, NULL);
	}

	// class function -> compile instance
	char *instance_param = NULL;
	int instance_kind;
	if (com->Kind == KindCompilerFunction){
		if (PreCommand[com->LinkNr].Instance == f_class){
			so("member");
			sLinkData link;
			link.script = NULL;
			link.Kind = pre_script->Command[com->SubLink1].Kind;
			link.Nr = pre_script->Command[com->SubLink1].LinkNr;
			link.type = pre_script->Command[com->SubLink1].ReturnType;
			link.Meta = NULL;
			instance_param = OCAddParameter(&link,n_func,level,index,instance_kind);
		}
	}

	    
	if (com->Kind == KindOperator){
		//msg_write("---operator");
		
		switch(com->LinkNr){
			case OperatorIntAssign:
			case OperatorFloatAssign:
			case OperatorPointerAssign:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk[0],param[0]);
				break;
			case OperatorCharAssign:
			case OperatorBoolAssign:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,pk[0],param[0]);
				break;
			case OperatorStructAssign:
				for (int i=0;i<signed(com->ParamLink[0].type->Size)/4;i++){
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[1],param[1],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk[0],param[0],i*4);
				}
				for (int i=4*signed(com->ParamLink[0].type->Size/4);i<signed(com->ParamLink[0].type->Size);i++){
					OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[1],param[1],i);
					OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,pk[0],param[0],i);
				}
				break;
// string
			case OperatorStringAssignAA:
			case OperatorStringAssignAP:
				OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LOffset);
				OCAddInstruction(Opcode,OpcodeSize,inPush,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inPush,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcpy-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LOffset+8);
				break;
			case OperatorStringAddAAS:
			case OperatorStringAddAPS:
				OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LOffset);
				OCAddInstruction(Opcode,OpcodeSize,inPush,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inPush,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcat-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LOffset+8);
				break;
			case OperatorStringAddAA:
			case OperatorStringAddAP:
			case OperatorStringAddPA:
			case OperatorStringAddPP:
				OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LOffset);
				so(d2h((char*)&param[0],4,false));
				so(d2h((char*)&ret,4,false));
				OCAddInstruction(Opcode,OpcodeSize,inLeaEdxM,rk,ret);
				OCAddInstruction(Opcode,OpcodeSize,inPush,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inPushEdx,-1);
				OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcpy-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(Opcode,OpcodeSize,8);
				OCAddInstruction(Opcode,OpcodeSize,inLeaEdxM,rk,ret);
				OCAddInstruction(Opcode,OpcodeSize,inPush,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inPushEdx,-1);
				OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcat-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LOffset+8);
				break;
			case OperatorStringEqualAA:
			case OperatorStringEqualAP:
			case OperatorStringEqualPA:
			case OperatorStringEqualPP:
			case OperatorStringNotEqualAA:
			case OperatorStringNotEqualAP:
			case OperatorStringNotEqualPA:
			case OperatorStringNotEqualPP:
				OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LOffset);
				OCAddInstruction(Opcode,OpcodeSize,inPush,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inPush,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcmp-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
				OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LOffset+8);
				OCAddInstruction(Opcode,OpcodeSize,inCmpAlM8,KindConstant,NULL);
				if ((com->LinkNr==OperatorStringEqualAA)||(com->LinkNr==OperatorStringEqualAP)||(com->LinkNr==OperatorStringEqualPA)||(com->LinkNr==OperatorStringEqualPP))
					OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				if ((com->LinkNr==OperatorStringNotEqualAA)||(com->LinkNr==OperatorStringNotEqualAP)||(com->LinkNr==OperatorStringNotEqualPA)||(com->LinkNr==OperatorStringNotEqualPP))
					OCAddInstruction(Opcode,OpcodeSize,inSetnzAl,-1);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
// int
			case OperatorIntAddS:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inAddMEax,pk[0],param[0]);
				break;
			case OperatorIntSubtractS:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSubMEax,pk[0],param[0]);
				break;
			case OperatorIntMultiplyS:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMulEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk[0],param[0]);
				break;
			case OperatorIntDivideS:
				OCAddInstruction(Opcode,OpcodeSize,inMovEdxM,pk[0],param[0]);
				OCAddChar(Opcode,OpcodeSize,(char)0x89);
				OCAddChar(Opcode,OpcodeSize,(char)0xd0); // MOV eAX,eDX			// TODO
				OCAddChar(Opcode,OpcodeSize,(char)0xc1);
				OCAddChar(Opcode,OpcodeSize,(char)0xfa);
				OCAddChar(Opcode,OpcodeSize,0x1f); // SAR eDX,<1f>
				OCAddInstruction(Opcode,OpcodeSize,inDivEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk[0],param[0]);
				break;
			case OperatorIntAdd:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inAddEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
			case OperatorIntSubtract:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inSubEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
			case OperatorIntMultiply:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMulEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
			case OperatorIntDivide:
				OCAddInstruction(Opcode,OpcodeSize,inMovEdxM,pk[0],param[0]);
				OCAddChar(Opcode,OpcodeSize,(char)0x89);
				OCAddChar(Opcode,OpcodeSize,(char)0xd0); // MOV eAX,eDX			// TODO
				OCAddChar(Opcode,OpcodeSize,(char)0xc1);
				OCAddChar(Opcode,OpcodeSize,(char)0xfa);
				OCAddChar(Opcode,OpcodeSize,(char)0x1f); // SAR eDX,<1f>
				OCAddInstruction(Opcode,OpcodeSize,inDivEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
			case OperatorIntModulo:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddChar(Opcode,OpcodeSize,(char)0x99);
				if (pk[1]==KindConstant){
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[1],param[1]);
					OCAddInstruction(Opcode,OpcodeSize,inDivEaxEbx,-1,NULL);
				}else
					OCAddInstruction(Opcode,OpcodeSize,inDivEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEdx,rk,ret);
				break;
			case OperatorIntEqual:
			case OperatorIntNotEqual:
			case OperatorIntGreater:
			case OperatorIntGreaterEqual:
			case OperatorIntSmaller:
			case OperatorIntSmallerEqual:
			case OperatorPointerEqual:
			case OperatorPointerNotEqual:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inCmpEaxM,pk[1],param[1]);
				if (com->LinkNr==OperatorIntEqual)			OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				if (com->LinkNr==OperatorIntNotEqual)		OCAddInstruction(Opcode,OpcodeSize,inSetnzAl,-1);
				if (com->LinkNr==OperatorIntGreater)		OCAddInstruction(Opcode,OpcodeSize,inSetnleAl,-1);
				if (com->LinkNr==OperatorIntGreaterEqual)	OCAddInstruction(Opcode,OpcodeSize,inSetnlAl,-1);
				if (com->LinkNr==OperatorIntSmaller)		OCAddInstruction(Opcode,OpcodeSize,inSetlAl,-1);
				if (com->LinkNr==OperatorIntSmallerEqual)	OCAddInstruction(Opcode,OpcodeSize,inSetleAl,-1);
				if (com->LinkNr==OperatorPointerEqual)		OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				if (com->LinkNr==OperatorPointerNotEqual)	OCAddInstruction(Opcode,OpcodeSize,inSetnzAl,-1);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorIntBitAnd:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inAndEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
			case OperatorIntBitOr:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inOrEaxM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
			case OperatorIntShiftRight:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMovClM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inShrEaxCl,-1);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
			case OperatorIntShiftLeft:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMovClM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inShlEaxCl,-1);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
			case OperatorIntNegate:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,KindConstant,NULL);
				OCAddInstruction(Opcode,OpcodeSize,inSubEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
			case OperatorIntIncrease:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inAddEaxM,KindConstant,(char*)0x00000001);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk[0],param[0]);
				break;
			case OperatorIntDecrease:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inSubEaxM,KindConstant,(char*)0x00000001);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk[0],param[0]);
				break;
// float
			case OperatorFloatAddS:
			case OperatorFloatSubtractS:
			case OperatorFloatMultiplyS:
			case OperatorFloatDivideS:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				if (com->LinkNr==OperatorFloatAddS)			OCAddInstruction(Opcode,OpcodeSize,inAddfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatSubtractS)	OCAddInstruction(Opcode,OpcodeSize,inSubfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatMultiplyS)	OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatDivideS)		OCAddInstruction(Opcode,OpcodeSize,inDivfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,pk[0],param[0]);
				break;
			case OperatorFloatAdd:
			case OperatorFloatSubtract:
			case OperatorFloatMultiply:
			case OperatorFloatDivide:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				if (com->LinkNr==OperatorFloatAdd)		OCAddInstruction(Opcode,OpcodeSize,inAddfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatSubtract)	OCAddInstruction(Opcode,OpcodeSize,inSubfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatMultiply)	OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatDivide)	OCAddInstruction(Opcode,OpcodeSize,inDivfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
				break;
			case OperatorFloatMultiplyFI:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfiM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
				break;
			case OperatorFloatMultiplyIF:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfiM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
				break;
			case OperatorFloatEqual:
			case OperatorFloatNotEqual:
			case OperatorFloatGreater:
			case OperatorFloatGreaterEqual:
			case OperatorFloatSmaller:
			case OperatorFloatSmallerEqual:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[1],param[1]);
				if (com->LinkNr==OperatorFloatEqual){
					OCAddChar(Opcode,OpcodeSize,0xd9);	OCAddChar(Opcode,OpcodeSize,0xc9);
					OCAddChar(Opcode,OpcodeSize,0xda);	OCAddChar(Opcode,OpcodeSize,0xe9);
					OCAddChar(Opcode,OpcodeSize,0xdf);	OCAddChar(Opcode,OpcodeSize,0xe0);
					OCAddChar(Opcode,OpcodeSize,0x80);	OCAddChar(Opcode,OpcodeSize,0xe4);	OCAddChar(Opcode,OpcodeSize,0x45);
					OCAddChar(Opcode,OpcodeSize,0x80);	OCAddChar(Opcode,OpcodeSize,0xfc);	OCAddChar(Opcode,OpcodeSize,0x40);
				}else if (com->LinkNr==OperatorFloatNotEqual){
					OCAddChar(Opcode,OpcodeSize,0xd9);	OCAddChar(Opcode,OpcodeSize,0xc9);
					OCAddChar(Opcode,OpcodeSize,0xda);	OCAddChar(Opcode,OpcodeSize,0xe9);
					OCAddChar(Opcode,OpcodeSize,0xdf);	OCAddChar(Opcode,OpcodeSize,0xe0);
					OCAddChar(Opcode,OpcodeSize,0x80);	OCAddChar(Opcode,OpcodeSize,0xe4);	OCAddChar(Opcode,OpcodeSize,0x45);
					OCAddChar(Opcode,OpcodeSize,0x80);	OCAddChar(Opcode,OpcodeSize,0xf4);	OCAddChar(Opcode,OpcodeSize,0x40);
				}else if (com->LinkNr==OperatorFloatSmaller){
					OCAddChar(Opcode,OpcodeSize,0xda);	OCAddChar(Opcode,OpcodeSize,0xe9);
					OCAddChar(Opcode,OpcodeSize,0xdf);	OCAddChar(Opcode,OpcodeSize,0xe0);
					OCAddChar(Opcode,OpcodeSize,0xf6);	OCAddChar(Opcode,OpcodeSize,0xc4);	OCAddChar(Opcode,OpcodeSize,0x45);
				}else if (com->LinkNr==OperatorFloatSmallerEqual){
					OCAddChar(Opcode,OpcodeSize,0xda);	OCAddChar(Opcode,OpcodeSize,0xe9);
					OCAddChar(Opcode,OpcodeSize,0xdf);	OCAddChar(Opcode,OpcodeSize,0xe0);
					OCAddChar(Opcode,OpcodeSize,0xf6);	OCAddChar(Opcode,OpcodeSize,0xc4);	OCAddChar(Opcode,OpcodeSize,0x05);
				}else if (com->LinkNr==OperatorFloatGreater){
					OCAddChar(Opcode,OpcodeSize,0xd9);	OCAddChar(Opcode,OpcodeSize,0xc9);
					OCAddChar(Opcode,OpcodeSize,0xda);	OCAddChar(Opcode,OpcodeSize,0xe9);
					OCAddChar(Opcode,OpcodeSize,0xdf);	OCAddChar(Opcode,OpcodeSize,0xe0);
					OCAddChar(Opcode,OpcodeSize,0xf6);	OCAddChar(Opcode,OpcodeSize,0xc4);	OCAddChar(Opcode,OpcodeSize,0x45);
				}else if (com->LinkNr==OperatorFloatGreaterEqual){
					OCAddChar(Opcode,OpcodeSize,0xd9);	OCAddChar(Opcode,OpcodeSize,0xc9);
					OCAddChar(Opcode,OpcodeSize,0xda);	OCAddChar(Opcode,OpcodeSize,0xe9);
					OCAddChar(Opcode,OpcodeSize,0xdf);	OCAddChar(Opcode,OpcodeSize,0xe0);
					OCAddChar(Opcode,OpcodeSize,0xf6);	OCAddChar(Opcode,OpcodeSize,0xc4);	OCAddChar(Opcode,OpcodeSize,0x05);
				}
				OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorFloatNegate:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inXorEaxM,KindConstant,(char*)0x80000000);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
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
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inCmpAlM8,pk[0],param[0]);
				if ((com->LinkNr==OperatorCharEqual)||(com->LinkNr==OperatorBoolEqual))
					OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				if ((com->LinkNr==OperatorCharNotEqual)||(com->LinkNr==OperatorBoolNotEqual))
					OCAddInstruction(Opcode,OpcodeSize,inSetnzAl,-1);
				if (com->LinkNr==OperatorBoolGreater)		OCAddInstruction(Opcode,OpcodeSize,inSetnleAl,-1);
				if (com->LinkNr==OperatorBoolGreaterEqual)	OCAddInstruction(Opcode,OpcodeSize,inSetnlAl,-1);
				if (com->LinkNr==OperatorBoolSmaller)		OCAddInstruction(Opcode,OpcodeSize,inSetlAl,-1);
				if (com->LinkNr==OperatorBoolSmallerEqual)	OCAddInstruction(Opcode,OpcodeSize,inSetleAl,-1);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorBoolAnd:
			case OperatorBoolOr:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[0],param[0]);
				if (com->LinkNr==OperatorBoolAnd)	OCAddInstruction(Opcode,OpcodeSize,inAndAlM8,pk[1],param[1]);
				if (com->LinkNr==OperatorBoolOr)	OCAddInstruction(Opcode,OpcodeSize,inOrAlM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorCharAddS:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inAddM8Al,pk[0],param[0]);
				break;
			case OperatorCharSubtractS:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSubM8Al,pk[0],param[0]);
				break;
			case OperatorCharAdd:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inAddAlM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorCharSubtract:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inSubAlM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorCharBitAnd:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inAndAlM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorCharBitOr:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inOrAlM8,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorBoolNegate:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inXorAlM8,KindConstant,(char*)0x01);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorCharNegate:
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inXorAlM8,KindConstant,(char*)0xff);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
// vector
			case OperatorVectorAddS:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inAddfM,pk[1],param[1],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,pk[0],param[0],i*4);
				}
				break;
			case OperatorVectorMultiplyS:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,pk[0],param[0],i*4);
				}
				break;
			case OperatorVectorDivideS:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inDivfM,pk[1],param[1]);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,pk[0],param[0],i*4);
				}
				break;
			case OperatorVectorSubtractS:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inSubfM,pk[1],param[1],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,pk[0],param[0],i*4);
				}
				break;
			case OperatorVectorAdd:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inAddfM,pk[1],param[1],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret,i*4);
				}
				break;
			case OperatorVectorSubtract:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inSubfM,pk[1],param[1],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret+i*4);
				}
				break;
			case OperatorVectorMultiplyVF:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret,i*4);
				}
				break;
			case OperatorVectorMultiplyFV:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
					OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret,i*4);
				}
				break;
			case OperatorVectorDivideVF:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inDivfM,pk[1],param[1]);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret,i*4);
				}
				break;
			case OperatorVectorNegate:
				for (int i=0;i<3;i++){
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0],i*4);
					OCAddInstruction(Opcode,OpcodeSize,inXorEaxM,KindConstant,(char*)0x80000000);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret,i*4);
				}
				break;
			default:
				_do_error_(string("unimplemented operator (call Michi!): ",Operator2Str(pre_script,com->LinkNr)), 4, NULL);
		}
	}else if ((com->Kind == KindCompilerFunction) || (com->Kind == KindFunction)){
		//msg_write("---func");
		t_func *f = NULL;
		void *instance = NULL;
		char name[128];
		if (com->Kind == KindFunction){ // own script Function
			so("Funktion!!!");
			if (com->script){
				so("    extern!!!");
				f = com->script->func[com->LinkNr];
				so("   -ok");
			}else
				f = func[com->LinkNr];
		}else{ // compiler function
			f = (t_func*)PreCommand[com->LinkNr].Func;
			instance = PreCommand[com->LinkNr].Instance;
			strcpy(name, PreCommand[com->LinkNr].Name);
		}
		if ((unsigned long)f > (unsigned long)f_cp){ // a real function
			if ((com->ReturnType->Size > 4) && (com->ReturnType->ArrayLength <= 0))
				OCAddInstruction(Opcode,OpcodeSize,inLeaEaxM,rk,ret);

			// Stack um die lokalen Variablen der bisherigen Funktion erniedrigen
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LOffset-8);
			int dp=0;

			for (int p=com->NumParams-1;p>=0;p--){
				int s = mem_align(com->ParamLink[p].type->Size);
				// push parameters onto stack
				for (int j=0;j<s/4;j++)
					OCAddInstruction(Opcode, OpcodeSize, inPush, pk[p], param[p], s - 4 - j * 4);
				dp += s;
			}

#ifdef NIX_IDE_VCS
			// muessen mehr als 4byte zurueckgegeben werden, muss die Rueckgabe-Adresse als allererster Parameter mitgegeben werden!
			if ((com->ReturnType->Size>4)&&(com->ReturnType->ArrayLength<=0))
				OCAddInstruction(Opcode,OpcodeSize,inPushEax,-1); // nachtraegliche eSP-Korrektur macht die Funktion
#endif
			// _cdecl: Klassen-Instanz als ersten Parameter push'en
			if (instance){
				if ((unsigned long)instance>100)	OCAddInstruction(Opcode, OpcodeSize, inPush, KindConstant,*(void**)instance);
				else if (instance == f_class)		OCAddInstruction(Opcode, OpcodeSize, inPush, instance_kind, instance_param);
				else								OCAddInstruction(Opcode, OpcodeSize, inPush, KindVarGlobal,instance);
				dp+=4;
			}
#ifndef NIX_IDE_VCS
			// muessen mehr als 4byte zurueckgegeben werden, muss die Rueckgabe-Adresse als allererster Parameter mitgegeben werden!
			if ((com->ReturnType->Size>4)&&(com->ReturnType->ArrayLength<=0))
				OCAddInstruction(Opcode,OpcodeSize,inPushEax,-1); // nachtraegliche eSP-Korrektur macht die Funktion
#endif
			long d=(long)f-(long)&Opcode[OpcodeSize]-5;
			//so(d);
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)d); // der eigentliche Aufruf
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LOffset+dp+8);

			// Rueckgabewert > 4b ist schon von der Funktion nach [ret] kopiert worden!
			if (com->ReturnType != TypeVoid)
				if ((com->ReturnType->Size <= 4) || (com->ReturnType->ArrayLength > 0)){
					if (com->ReturnType == TypeFloat)
						OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
					else
						OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				}
		}else if ((unsigned long)f==(unsigned long)f_cp){
			switch(com->LinkNr){
				case CommandIf:
					OCAddInstruction(Opcode,OpcodeSize,inCmpM80,pk[0],param[0]);
					//OCAddInstruction(Opcode,OpcodeSize,inJzC8,KindConstant,NULL);
					OCAddInstruction(Opcode,OpcodeSize,inJzC32,KindConstant,NULL);
					add_jump(level, index + 1, level, index + 2, OCOParam);
					break;
				case CommandIfElse:
					OCAddInstruction(Opcode,OpcodeSize,inCmpM80,pk[0],param[0]);
					//OCAddInstruction(Opcode,OpcodeSize,inJzC8,KindConstant,NULL);
					OCAddInstruction(Opcode,OpcodeSize,inJzC32,KindConstant,NULL);
					add_jump(level, index + 1, level, index + 2, OCOParam);
					add_jump(level, index + 2, level, index + 3, -1);
					break;
				case CommandWhile:
				case CommandFor:
					OCAddInstruction(Opcode,OpcodeSize,inCmpM80,pk[0],param[0]);
					//OCAddInstruction(Opcode,OpcodeSize,inJzC8,KindConstant,NULL);
					OCAddInstruction(Opcode,OpcodeSize,inJzC32,KindConstant,NULL);
					while_level = level;
					while_index = index;
					add_jump(level, index + 1, level, index + 2, OCOParam);
					add_jump(level, index + 2, level, index    , -1);
					break;
				case CommandBreak:
					add_jump(level, index + 1, while_level, while_index + 2, -1);
					break;
				case CommandContinue:
					add_jump(level, index + 1, while_level, while_index, -1);
					break;
				case CommandReturn:
					if (com->NumParams > 0){
						if (com->ParamLink[0].type->Size > 4){ // Return in erhaltener Return-Adresse speichern (> 4 byte)
							OCAddInstruction(Opcode,OpcodeSize,inMovEdxM,KindVarLocal,(char*)8);
							int s = mem_align(com->ParamLink[0].type->Size);
							for (int j=0;j<s/4;j++){
								OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0],j*4);
								OCAddInstruction(Opcode,OpcodeSize,inMovEdxpi8Eax,KindConstant,(char*)(j*4));
							}
						}else{ // Return direkt in eAX speichern (4 byte)
							if (com->ParamLink[0].type == TypeFloat)
								OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
							else
								OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
						}
					}
					OCAddInstruction(Opcode,OpcodeSize,inLeave,-1);
					if (p_func->Type->Size > 4)
						OCAddInstruction(Opcode,OpcodeSize,inRet,-1); // inRet4
					else
						OCAddInstruction(Opcode,OpcodeSize,inRet,-1);
					break;
				case CommandWaitOneFrame:
				case CommandWait:
				case CommandWaitRT:
					// Warte-Zustand setzen
					if (com->LinkNr==CommandWaitOneFrame)	OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,KindConstant,(char*)WaitingModeRT);
					if (com->LinkNr==CommandWait)			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,KindConstant,(char*)WaitingModeGT);
					if (com->LinkNr==CommandWaitRT)			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,KindConstant,(char*)WaitingModeRT);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,KindVarGlobal,(char*)&GlobalWaitingMode);
					if (com->LinkNr==CommandWaitOneFrame)	OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,KindConstant,NULL);
					if (com->LinkNr==CommandWait)			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
					if (com->LinkNr==CommandWaitRT)			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,KindVarGlobal,(char*)&GlobalTimeToWait);
					// Script-Zustand speichern
					OCAddInstruction(Opcode,OpcodeSize,inMovEspM,KindConstant,(char*)&Stack[ScriptStackSize-4]); // zum Anfang des Script-Stacks
					OCAddInstruction(Opcode,OpcodeSize,inPushEbp,-1);
					OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)0); // PUSH eIP
					// Return laden
					OCAddInstruction(Opcode,OpcodeSize,inMovEspM,KindConstant,(char*)&Stack[ScriptStackSize-4]); // zum Anfang des Script-Stacks
					OCAddInstruction(Opcode,OpcodeSize,inPopEsp,-1); // alter StackPointer (echtes Programm)
					OCAddInstruction(Opcode,OpcodeSize,inMovEbpEsp,-1);
					OCAddInstruction(Opcode,OpcodeSize,inLeave,-1);
					OCAddInstruction(Opcode,OpcodeSize,inRet,-1);
					// hier ist die Unterbrechung!
					OCAddInstruction(Opcode,OpcodeSize,inMovEspM,KindConstant,(char*)&Stack[ScriptStackSize-8]); // zum eBP
					OCAddInstruction(Opcode,OpcodeSize,inPopEsp,-1); // Script-StackPointer
					OCAddInstruction(Opcode,OpcodeSize,inMovEbpEsp,-1);
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,KindConstant,(char*)WaitingModeNone);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,KindVarGlobal,(char*)&GlobalWaitingMode);
					break;
				case CommandIntToFloat:
					OCAddInstruction(Opcode,OpcodeSize,inLoadfiM,pk[0],param[0]);
					OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
					break;
				/*case CommandFloatToInt:
					// tut nicht... habe es vorerst lieber als echte Funktion implementiert...
					/ *OCAddChar(0x89);	OCAddChar(0xe5);
					OCAddChar(0x83);	OCAddChar(0xec);	OCAddChar(0x04);
					OCAddChar(0xd9);	OCAddChar(0x05);	OCAddInt((int)param[0]);
					OCAddChar(0xd9);	OCAddChar(0x7d);	OCAddChar(0xfe);
					OCAddChar(0x66);	OCAddChar(0x8b);	OCAddChar(0x45);	OCAddChar(0xfe);
					OCAddChar(0xb4);	OCAddChar(0x0c);
					OCAddChar(0x66);	OCAddChar(0x89);	OCAddChar(0x45);	OCAddChar(0xfc);
					OCAddChar(0xd9);	OCAddChar(0x6d);	OCAddChar(0xfc);
					OCAddChar(0xdb);	OCAddChar(0x1d);	OCAddInt((int)ret);
					OCAddChar(0xd9);	OCAddChar(0x6d);	OCAddChar(0xfe);
					OCAddChar(0xc9);* /
					/ *OCAddChar(0x51);
					OCAddChar(0xd9);    OCAddChar(0x05);	OCAddInt((int)param[0]);
					OCAddChar(0xd9);    OCAddChar(0x7d);    OCAddChar(0xfe);
					OCAddChar(0x66);	OCAddChar(0x8b);	OCAddChar(0x45);	OCAddChar(0xfe);
					OCAddChar(0xb4);	OCAddChar(0x0c);
					OCAddChar(0x66);	OCAddChar(0x89);	OCAddChar(0x45);	OCAddChar(0xfc);
					OCAddChar(0xd9);	OCAddChar(0x6d);	OCAddChar(0xfc);
					OCAddChar(0xdb);	OCAddChar(0x1d);	OCAddInt((int)ret);
					OCAddChar(0xd9);	OCAddChar(0x6d);	OCAddChar(0xfe);
					OCAddChar(0x89);    OCAddChar(0xec);
					break;*/
				case CommandIntToChar:
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
					OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
					break;
				case CommandCharToInt:
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,KindConstant,NULL);
					OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[0],param[0]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
					break;
				case CommandPointerToBool:
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
					OCAddInstruction(Opcode,OpcodeSize,inCmpEaxM,KindConstant,0);
					OCAddInstruction(Opcode,OpcodeSize,inSetnzAl,-1);
					OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
					break;
				case CommandAsm:
					{
//#if 0
						so("a");
						CreateAsmMetaInfo(pre_script);
						so("b");
						((sAsmMetaInfo*)pre_script->AsmMetaInfo)->CurrentOpcodePos = OpcodeSize;
						((sAsmMetaInfo*)pre_script->AsmMetaInfo)->PreInsertionLength = OpcodeSize;
						so("c");
						((sAsmMetaInfo*)pre_script->AsmMetaInfo)->LineOffset=pre_script->AsmBlock[0].Line;
						so("d");
						CurrentAsmMetaInfo=(sAsmMetaInfo*)pre_script->AsmMetaInfo;
						so("e");
				//		so("Asm------------------");
						char *pac;
						
						pac=Asm2Opcode(pre_script->AsmBlock[0].block);
						so("f");
						if (pac){
							so("g");
							char *ac=new char[AsmCodeLength+20];
							am("ac", AsmCodeLength+20, ac);
							memcpy(ac,pac,AsmCodeLength);
							so("h");
				/*			msg_write(AsmCodeLength);
							msg_write(d2h(ac,AsmCodeLength,false));
							msg_write(GetAsm(ac,AsmCodeLength));*/
							for (int i=0;i<AsmCodeLength;i++)
								OCAddChar(Opcode,OpcodeSize,ac[i]);
							dm("ac", ac);
							delete[](ac);
							so("i");
						}else{
							_do_error_("error in assembler code...", 4, ret);
						}
						so("j");
						dm("AsmBlock",pre_script->AsmBlock[0].block);
						delete[](pre_script->AsmBlock[0].block);
						pre_script->AsmBlock.erase(pre_script->AsmBlock.begin());
						so("k");
				//		msg_write("msA------------------");
//#endif
//						msg_error("zu coden: OCAddCommand   Asm");
					}
					break;
				case CommandRectSet:
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[3],param[3]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret,12);
				case CommandVectorSet:
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[1],param[1]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret,4);
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[2],param[2]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret,8);
					break;
				case CommandColorSet:
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret,12);
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[1],param[1]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[2],param[2]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret,4);
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[3],param[3]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret,8);
					break;
				default:
 					_do_error_(string("compiler function unimplemented (call Michi!): ",PreCommand[com->LinkNr].Name), 4, ret);
			}
		}else{
 			DoErrorLink(string("compiler function not linkable: ",PreCommand[com->LinkNr].Name));
			_return_(4, ret);
		}
	}else if (com->Kind==KindBlock){
		//msg_write("---block");
		OCAddBlock(&pre_script->Block[com->LinkNr],n_func,level+1);
		if (Error)	_return_(4, NULL);
	}else{
		//msg_write("---???");
		_do_error_(string("type of command is unimplemented (call Michi!): ",Kind2Str(com->Kind)), 4, NULL);
	}
	//msg_write(Kind2Str(com->Kind));
	//msg_ok();
	msg_db_l(4);
	return ret;
}

void CScript::OCAddBlock(sBlock *block,int n_func,int level)
{
	msg_db_r("OCAddBlock", 4);
	NumJumps[level]=0;
	for (int i=0;i<block->Command.size();i++){
		//msg_write(string2("%d - %d",i,block->NumCommands));
		LOffset=0;
		OCOffset[level][i] = OpcodeSize;
		OCAddCommand(&pre_script->Command[block->Command[i]],n_func,level,i);
		if (Error)	_return_(4,);
		//int offset_after_cmd = OpcodeSize;

		// create dummy code for jumps from this level (if, while,..)
		for (int j=0;j<NumNewJumps;j++){
			if ((NewJumpSourceLevel[j] == level) && (NewJumpSource[j] == i + 1)){ // put it here! (jump before the next command)
				if (NewJumpCode[j]<0){
					//int d=OCOffset[level][JumpDest[level][j]]-OpcodeSize-5;//-2;
					//so(string("Jump....",i2s(d)));
					//OCAddChar(Opcode,OpcodeSize,(char)0xeb);	OCAddChar(Opcode,OpcodeSize,char(0));	NewJumpCode[j]=OpcodeSize-1;
					OCAddChar(Opcode,OpcodeSize,(char)0xe9);	OCAddInt(Opcode,OpcodeSize,0);        	NewJumpCode[j]=OpcodeSize-4;
				}
				insert_new_jump(j, OpcodeSize); // offet = after jump instruction
				j --;
			}
		}
	}
	OCOffset[level][block->Command.size()]=OpcodeSize;

	// link the jump codes to this level
	for (int i=0;i<NumJumps[level];i++){
		//if (JumpCode[level][i]>=0)
		    int d = OCOffset[level][JumpDest[level][i]] - JumpSourceOffset[level][i];
			//Opcode[JumpCode[level][i]]=char(d);
			*(int*)&Opcode[JumpCode[level][i]]=int(d);
			so(string2("Jump.... %d -> %d : %d",JumpSourceOffset[level][i],JumpDest[level][i],d));
	}
	msg_db_l(4);
}

// Opcode generieren
void CScript::Compiler()
{
	int nf, OCORA;
	if (Error)	return;
	msg_db_r("Compiler",2);

	// get memory size needed
	MemorySize = 0;
	for (int i=0;i<pre_script->RootOfAllEvil.Var.size();i++)
		MemorySize += mem_align(pre_script->RootOfAllEvil.Var[i].Type->Size);
	sConstant *c;
	foreach(pre_script->Constant, c, i){
		int s = c->type->Size;
		if (c->type == TypeString)
			// const string -> variable length
			s = strlen(c->data) + 1;
		MemorySize += mem_align(s);
	}
	Memory = new char[MemorySize];
	am("Memory",MemorySize,Memory);

	// use your own stack if needed
	//   wait() used -> needs to switch stacks ("tasks")
	Stack = NULL;
	sCommand *cmd;
	foreach(pre_script->Command, cmd, i)
		if (cmd->Kind == KindCompilerFunction)
			if ((cmd->LinkNr == CommandWait) || (cmd->LinkNr == CommandWaitRT) || (cmd->LinkNr == CommandWaitOneFrame)){
				Stack=new char[ScriptStackSize];
				am("Stack",ScriptStackSize,Stack);
				break;
			}

	MemorySize = 0;
	// global variables -> into Memory
	so("glob.Var.");
	g_var.resize(pre_script->RootOfAllEvil.Var.size());
	for (int i=0;i<pre_script->RootOfAllEvil.Var.size();i++){
		if (pre_script->FlagOverwriteVariablesOffset)
			g_var[i] = (char*)(MemorySize + pre_script->VariablesOffset);
		else
			g_var[i] = &Memory[MemorySize];
		so(string(i2s(MemorySize),":  ",pre_script->RootOfAllEvil.Var[i].Name));
		MemorySize += mem_align(pre_script->RootOfAllEvil.Var[i].Type->Size);
	}
	memset(Memory, 0, MemorySize); // reset all global variables to 0

	// constants -> Memory
	so("Konstanten");
	cnst.resize(pre_script->Constant.size());
	foreach(pre_script->Constant, c, i){
		cnst[i] = &Memory[MemorySize];
		int s = c->type->Size;
		if (c->type == TypeString)
			// const string -> variable Laenge
			s = strlen(pre_script->Constant[i].data) + 1;
		memcpy(&Memory[MemorySize], (void*)c->data, s);
		MemorySize += mem_align(s);
	}

	// allocate some memory for the opcode......    has to be executable!!!   (important on amd64)
#ifdef FILE_OS_WINDOWS
	Opcode=(char*)VirtualAlloc(NULL,SCRIPT_MAX_OPCODE,MEM_COMMIT | MEM_RESERVE,PAGE_EXECUTE_READWRITE);
	ThreadOpcode=(char*)VirtualAlloc(NULL,SCRIPT_MAX_THREAD_OPCODE,MEM_COMMIT | MEM_RESERVE,PAGE_EXECUTE_READWRITE);
#else
	Opcode=(char*)mmap(0,SCRIPT_MAX_OPCODE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED|MAP_ANON,0,0);
	ThreadOpcode=(char*)mmap(0,SCRIPT_MAX_THREAD_OPCODE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED|MAP_ANON,0,0);
#endif
	am("Opcode",SCRIPT_MAX_OPCODE,Opcode);
	am("ThreadOpcode",SCRIPT_MAX_THREAD_OPCODE,ThreadOpcode);
	if (((long)Opcode==-1)||((long)ThreadOpcode==-1))
		_do_error_("CScript:  could not allocate executable memory", 2,);
	OpcodeSize=0;
	ThreadOpcodeSize=0;


	
// compiling an operating system?
//   -> create an entry point for execution... so we can just call Opcode like a function
	sFunction *ff;
	if ((pre_script->FlagCompileOS)||(pre_script->FlagCompileInitialRealMode)){
		nf=-1;
		foreach(pre_script->Function, ff, f)
			if (strcmp(ff->Name,"main")==0)
				nf=f;
		// call
		if (nf>=0)
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,NULL);
		TaskReturnOffset=OpcodeSize;
		OCORA=OCOParam;

		// put strings into Opcode!
		foreach(pre_script->Constant, c, i){
			if ((pre_script->FlagCompileOS) || (c->type == TypeString)){
				int offset=0;
				if (pre_script->AsmMetaInfo)
					offset=((sAsmMetaInfo*)pre_script->AsmMetaInfo)->CodeOrigin;
				cnst[i]=(char*)(OpcodeSize+offset);
				int s=c->type->Size;
				if (pre_script->Constant[i].type == TypeString)
					s=strlen(c->data)+1;
				memcpy(&Opcode[OpcodeSize],(void*)c->data,s);
				OpcodeSize+=s;
			}
		}
	}


// compile functions into Opcode
	so("Funktionen");
	func.resize(pre_script->Function.size());
	foreach(pre_script->Function, ff, f){
		right();
		for (int i=0;i<ff->Var.size();i++)
			so(ff->Var[i].Offset);
		so(ff->Name);
		func[f]=(t_func*)&Opcode[OpcodeSize];
		LOffset=LOffsetMax=0;

		// intro
		OCAddInstruction(Opcode,OpcodeSize,inPushEbp,-1);
		OCAddInstruction(Opcode,OpcodeSize,inMovEbpEsp,-1);

		// function
		NumNewJumps = 0;
		OCAddBlock(ff->Block,f,0);
		if (Error)	_return_(2,);

		if (NumNewJumps > 0)
			_do_error_(string2("unlinked jump instructions in function %s, call Michi!", ff->Name), 2,);

		// outro
		OCAddInstruction(Opcode,OpcodeSize,inLeave,-1);
		if (ff->Type->Size>4)
			OCAddInstruction(Opcode,OpcodeSize,inRet4,-1);
		else
			OCAddInstruction(Opcode,OpcodeSize,inRet,-1);

		left();
	}
	if (!Error)
		if (pre_script->AsmMetaInfo)
			if (((sAsmMetaInfo*)pre_script->AsmMetaInfo)->WantedLabel.size() > 0)
				_do_error_(string2("unknown name in assembler code:  \"%s\"",((sAsmMetaInfo*)pre_script->AsmMetaInfo)->WantedLabel[0].Name), 2,);


// "task" for the first execution of main() -> ThreadOpcode
	if (!pre_script->FlagCompileOS){
		first_execution=(t_func*)&ThreadOpcode[ThreadOpcodeSize];
		// intro
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPushEbp,-1); // within the actual program
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEbpEsp,-1);
		if (Stack){
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEspM,KindConstant,(char*)&Stack[ScriptStackSize]); // zum Anfang des Script-Stacks
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPushEbp,-1); // adress of the old stack
			OCAddEspAdd(ThreadOpcode,ThreadOpcodeSize,-8); // space for wait() task data
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEbpEsp,-1);
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEaxM,KindConstant,(char*)WaitingModeNone); // "reset"
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovMEax,KindVarGlobal,(char*)&WaitingMode);
		}
		// call
		nf=-1;
		foreach(pre_script->Function, ff, f)
			if (strcmp(ff->Name,"main")==0)
				nf=f;
		if (nf>=0){
			// call main() ...correct adress will be put here later!
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inCallRel32,KindConstant,NULL);
			*(int*)&ThreadOpcode[OCOParam]=((long)func[nf]-(long)&ThreadOpcode[ThreadOpcodeSize]);
		}
		// outro
		if (Stack){
			OCAddEspAdd(ThreadOpcode,ThreadOpcodeSize,8); // make space for wait() task data
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPopEsp,-1);
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEbpEsp,-1);
		}
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inLeave,-1);
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inRet,-1);

	// "task" for execution after some wait()
		continue_execution=(t_func*)&ThreadOpcode[ThreadOpcodeSize];
		// Intro
		if (Stack){
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPushEbp,-1); // within the actual program
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEbpEsp,-1);
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovMEbp,KindVarGlobal,&Stack[ScriptStackSize-4]);
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEspM,KindConstant,&Stack[ScriptStackSize-12]); // to the eIP of the script
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPopEax,-1);
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inAddEaxM,KindConstant,(char*)AfterWaitOCSize);
			OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inJmpEax,-1);
			//OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inLeave,-1);
			//OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inRet,-1);
			/*OCAddChar(0x90);
			OCAddChar(0x90);
			OCAddChar(0x90);*/
		}
	}




	if ((pre_script->FlagCompileOS)&&(nf>=0)){
		int lll=((long)func[nf]-(long)&Opcode[TaskReturnOffset]);
		if (pre_script->FlagCompileInitialRealMode)
			lll+=5;
		else
			lll+=3;
		//printf("insert   %d  an %d\n", lll, OCORA);
		//msg_write(lll);
		//msg_write(d2h(&lll,4,false));
		*(int*)&Opcode[OCORA]=lll;
	}

	//msg_db_out(1,GetAsm(Opcode,OpcodeSize));

	//_expand(Opcode,OpcodeSize);

	WaitingMode=WaitingModeFirst;
	msg_write("--------------------------------");
	msg_write(string2("Opcode: %db, Memory: %db",OpcodeSize,MemorySize));
	om();
	msg_db_l(2);
}

CScript::CScript()
{
	so("creating empty script (for console)");
	right();
	memset(this,0, sizeof(CScript));

	Error=false;

	MemorySize=0;
	Memory=new char[4096];
		am("Memory",4096,Memory);

	pre_script=new CPreScript();
		am("PreScript",sizeof(CPreScript),pre_script);

	so("-ok");
	left();
}

CScript::~CScript()
{
	msg_db_r("~CScript", 4);
	if ((Memory)&&(!JustAnalyse)){
		delete[](Memory);
		dm("Memory",Memory);
	}
	if (Opcode){
		#ifdef FILE_OS_WINDOWS
			VirtualFree(Opcode,0,MEM_RELEASE);
		#else
			int r=munmap(Opcode,SCRIPT_MAX_OPCODE);
		#endif
		dm("Opcode",Opcode);
	}
	if (ThreadOpcode){
		#ifdef FILE_OS_WINDOWS
			VirtualFree(ThreadOpcode,0,MEM_RELEASE);
		#else
			int r=munmap(ThreadOpcode,SCRIPT_MAX_THREAD_OPCODE);
		#endif
		dm("ThreadOpcode",ThreadOpcode);
	}
	if (Stack){
		delete[](Stack);
		dm("Stack", Stack);
	}
	//msg_write(string2("-----------            Memory:         %p",Memory));
	delete(pre_script);
	dm("pre_script",pre_script);
	msg_db_l(4);
}



static char single_command[1024];


// bad:  should clean up in case of errors!
void CScript::ExecuteSingleCommand(char *cmd)
{
	if (strlen(cmd)<1)
		return;
	strcpy(single_command,cmd);
	msg_write(string("script command: ",single_command));

	pre_script->Error=Error=false;
// find expressions
	pre_script->Analyse(single_command, false);
	if (pre_script->Exp.line[0].exp.size()<1){
		clear_exp_buffer(&pre_script->Exp);
		return;
	}

	// analyse syntax
	//Error=false;
	int nc=pre_script->Command.size();
	int ocs=OpcodeSize;
	int mms=MemorySize;
	int ncs=pre_script->Constant.size();
	shift_right=0;
	pre_script->Exp.cur_line = &pre_script->Exp.line[0];
	pre_script->Exp.cur_exp=0;

	sFunction f;
	f.Var.clear();
	pre_script->GetCommand(&f);
	//pre_script->GetCompleteCommand((pre_script->Exp->ExpNr,0,0,&f);
	Error|=pre_script->Error;

	// minimally compile
	if ((!Error) && (nc != pre_script->Command.size())){
		sConstant *c;
		foreach(pre_script->Constant, c, i){
			memcpy(&Memory[MemorySize], (void*)c->data, c->type->Size);
			cnst[i] = &Memory[MemorySize];
			MemorySize += mem_align(c->type->Size);
		}

		if (!Opcode){
			#ifdef FILE_OS_WINDOWS
				Opcode=(char*)VirtualAlloc(0,SCRIPT_MAX_OPCODE,MEM_COMMIT | MEM_RESERVE,PAGE_EXECUTE_READWRITE);
			#else
				Opcode=(char*)mmap(0,SCRIPT_MAX_OPCODE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED|MAP_ANON,0,0);
			#endif
			am("Opcode",sizeof(SCRIPT_MAX_OPCODE),Opcode);
			if ((long)Opcode==-1){
				Opcode=NULL;
				DoError("CScript:  could not allocate executable memory (single command)");
				// clean up!!!! (T_T)
				return;
			}
		}
		func[pre_script->Function.size()]=(t_func*)&Opcode[OpcodeSize];
		LOffset=LOffsetMax=0;

		// intro
		OCAddInstruction(Opcode,OpcodeSize,inPushEbp,-1);
		OCAddInstruction(Opcode,OpcodeSize,inMovEbpEsp,-1);

		// command
		OCAddCommand(&pre_script->Command[nc],pre_script->Function.size(),0,0);

		// outro
		OCAddInstruction(Opcode,OpcodeSize,inLeave,-1);
		OCAddInstruction(Opcode,OpcodeSize,inRet,-1);
	}

	// execute
	if ((!Error)&&(nc!=pre_script->Command.size())){
		t_func *_f_=(t_func*)&Opcode[ocs];
		_f_();
	}

	// clean up
	clear_exp_buffer(&pre_script->Exp);
	pre_script->Command.resize(nc);
	OpcodeSize=ocs;
	MemorySize=mms;
	for (int i=ncs;i<pre_script->Constant.size();i++)
		delete[](pre_script->Constant[i].data);
	pre_script->Constant.resize(nc);
}

bool CScript::ExecuteScriptFunction(char *name,...)
{
	msg_db_m("-ExecuteScriptFunction",2);
	msg_db_m(name,2);
	//msg_db_m(FileName,2);

	if ((pre_script->GetExistence(name,&pre_script->RootOfAllEvil))&&(pre_script->GetExistenceLink.Kind==KindFunction)){

		sFunction *f=&pre_script->Function[pre_script->GetExistenceLink.Nr];

		if (f->NumParams==0)
			// no arguments -> directly execute function
			func[pre_script->GetExistenceLink.Nr]();
		else{

			// compile a function (with no arguments) that calls the function we actually want
			// to call and that pushes the given arguments onto the stack before

			// process argument list
			va_list marker;
			va_start(marker,name);
			char *param[SCRIPT_MAX_PARAMS];
			int pk[SCRIPT_MAX_PARAMS];
			for (int p=0;p<f->NumParams;p++){
				param[p]=va_arg(marker,char*);
				pk[p]=KindVarGlobal;
				if (f->Var[p].Type->IsPointer)
					pk[p]=KindConstant;
			}
			va_end(marker);

			if (!GlobalOpcode){
				#ifdef FILE_OS_WINDOWS
					GlobalOpcode=(char*)VirtualAlloc(0,SCRIPT_MAX_OPCODE,MEM_COMMIT | MEM_RESERVE,PAGE_EXECUTE_READWRITE);
				#else
					GlobalOpcode=(char*)mmap(0,SCRIPT_MAX_OPCODE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED|MAP_ANON,0,0);
				#endif
				am("GlobalOpcode",sizeof(SCRIPT_MAX_OPCODE),GlobalOpcode);
				if ((long)GlobalOpcode==-1){
					GlobalOpcode=NULL;
					DoError("CScript:  could not allocate executable memory (single command)");
					return false;
				}
			}

			GlobalOpcodeSize=0;
			t_func *code=(t_func*)GlobalOpcode;

			// intro
			OCAddInstruction(GlobalOpcode,GlobalOpcodeSize,inPushEbp,-1);
			OCAddInstruction(GlobalOpcode,GlobalOpcodeSize,inMovEbpEsp,-1);

			// decrease stack by the size of the local variables/parameters of our function
			int dp=0;
			for (int p=f->NumParams-1;p>=0;p--){
				int s = mem_align(f->Var[p].Type->Size);
				// push parameters onto the stack
				for (int j=0;j<s/4;j++)
					OCAddInstruction(GlobalOpcode,GlobalOpcodeSize,inPush,pk[p],param[p],s-4-j*4);
				dp += s;
			}

			// the actual call
			OCAddInstruction(GlobalOpcode,GlobalOpcodeSize,inCallRel32,KindConstant,(char*)((long)func[pre_script->GetExistenceLink.Nr]-(long)&GlobalOpcode[GlobalOpcodeSize]-5));
			OCAddEspAdd(GlobalOpcode,GlobalOpcodeSize,dp+8);

			OCAddInstruction(GlobalOpcode,GlobalOpcodeSize,inLeave,-1);
			OCAddInstruction(GlobalOpcode,GlobalOpcodeSize,inRet,-1);

			// finally execute!
			code();
		}
		return true;
	}
	//msg_ok();
	return false;
}

void CScript::ShowVars(bool include_consts)
{	
	int ss=0;
	int i;
	char name[12];
	sType *t;
	int n=pre_script->RootOfAllEvil.Var.size();
	if (include_consts)
		n+=pre_script->Constant.size();
	for (i=0;i<n;i++){
		char *add=(char*)&Stack[ss];
		if (i<pre_script->RootOfAllEvil.Var.size()){
			strcpy(name,pre_script->RootOfAllEvil.Var[i].Name);
			t=pre_script->RootOfAllEvil.Var[i].Type;
		}else{
			strcpy(name,"---const---");
			t=pre_script->Constant[i-pre_script->RootOfAllEvil.Var.size()].type;
		}
		if (t == TypeInt)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  ",i2s(*(int*)&Stack[ss])));
		else if (t==TypeFloat)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  ",f2s(*(float*)&Stack[ss],3)));
		else if (t==TypeBool)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =(bool)  ",i2s(Stack[ss])));
		else if (t==TypeVector)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  (",string(f2s(*(float*)&Stack[ss],3)," , ",f2s(*(float*)&Stack[ss+4],3)," , ",f2s(*(float*)&Stack[ss+8],3),")")));
		else if ((t==TypeColor)||(t==TypeRect)||(t==TypeQuaternion))
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  (",string(f2s(*(float*)&Stack[ss],3)," , ",f2s(*(float*)&Stack[ss+4],3),string(" , ",f2s(*(float*)&Stack[ss+8],3)," , ",f2s(*(float*)&Stack[ss+12],3),")"))));
		else if (t->IsPointer)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  ",d2h(&Stack[ss],4,false)));
		else if (t==TypeString)
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  \"",&Stack[ss],"\""));
		else
			msg_write(string(d2h((char*)&add,4,false),":  ",name,"  =  ??? (unbekannter Typ)"));
		ss+=t->Size;
	}
}

void CScript::Execute()
{
	if (Error)	return;
	if (WaitingMode==WaitingModeNone)	return;
	#ifdef ScriptDebug
		//so("\n\n\n################### fuehre aus ######################\n\n\n");
	#endif
	shift_right=0;
	msg_db_r(string("Execute ",Filename),1);

	// handle wait-commands
	if (WaitingMode==WaitingModeFirst){
		GlobalWaitingMode=WaitingModeNone;
		msg_db_r("->First",1);
		//msg_right();
		first_execution();
		msg_db_l(1);
		//msg_left();
	}else{
#ifdef _X_ALLOW_META_
		if (WaitingMode==WaitingModeRT)
			TimeToWait-=ElapsedRT;
		else
			TimeToWait-=Elapsed;
		if (TimeToWait>0){
			msg_db_l(1);
			return;
		}
#endif
		GlobalWaitingMode=WaitingModeNone;
		//msg_write(ThisObject);
		msg_db_r("->Continue",1);
		//msg_write(">---");
		//msg_right();
		continue_execution();
		//msg_write("---<");
		msg_db_l(1);
		//msg_write("ok");
		//msg_left();
	}
	WaitingMode=GlobalWaitingMode;
	TimeToWait=GlobalTimeToWait;

	msg_db_l(1);
}
