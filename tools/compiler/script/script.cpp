/*----------------------------------------------------------------------------*\
| CScript                                                                      |
| -> C-like scripting system                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2010.07.07 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "script.h"
#include "../file/file.h"
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

char ScriptVersion[] = "0.7.0.-2";

//#define ScriptDebug

static int GlobalWaitingMode;
static float GlobalTimeToWait;

static char *GlobalOpcode = NULL;
static int GlobalOpcodeSize;

static std::vector<CScript*> cur_script_stack;
inline void push_cur_script(CScript *s)
{
	cur_script_stack.push_back(s);
	cur_script = s;
}
inline void pop_cur_script()
{
	cur_script_stack.resize(cur_script_stack.size() - 1);
	if (cur_script_stack.size() >= 1)
		cur_script = cur_script_stack[cur_script_stack.size() - 1];
	else
		cur_script = NULL;
}




static int shift_right=0;

static void stringout(const char *str)
{
	msg_write(str);
}

static void so(const char *str)
{
#ifdef ScriptDebug
	if (strlen(str)>256)
		((char*)str)[256]=0;
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



std::vector<CScript*> PublicScript;
std::vector<CScript*> PrivateScript;
std::vector<CScript*> DeadScript;


char ScriptDirectory[512]="";




CScript *LoadScript(const char *filename, bool is_public, bool just_analyse)
{
	//msg_write(string("Lade ",filename));
	CScript *s = NULL;

	// public und private aus dem Speicher versuchen zu laden
	if (is_public){
		for (int i=0;i<PublicScript.size();i++)
			if (strcmp(PublicScript[i]->pre_script->Filename, SysFileName(filename)) == 0)
				return PublicScript[i];
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

	s = new CScript(filename, just_analyse);
	am("CScript",sizeof(CScript),s);
	s->isPrivate = !is_public;

	// nur public speichern
	if (is_public){
		//msg_write("...neu (public)");
		PublicScript.push_back(s);
	}else{
		//msg_write("...neu (private)");
		PrivateScript.push_back(s);
	}
	//msg_error(i2s(NumPublicScripts));
	return s;
}

#if 0
CScript *LoadScriptAsInclude(char *filename, bool just_analyse)
{
	msg_db_r("LoadAsInclude",4);
	//msg_write(string("Include ",filename));
	// aus dem Speicher versuchen zu laden
	for (int i=0;i<ublicScript.size();i++)
		if (strcmp(PublicScript[i].filename, SysFileName(filename)) == 0){
			//msg_write("...pointer");
			msg_db_l(4);
			return PublicScript[i].script;
		}

	//msg_write("nnneu");
	CScript *s = new CScript(filename, just_analyse);
	am("script",sizeof(CScript),s);
	so("geladen....");
	//msg_write("...neu");
	s->isPrivate = false;

	// als public speichern
	PublicScript[NumPublicScripts].filename=new char[strlen(filename)+1];
	am("pub.filename",strlen(filename)+1,PublicScript[NumPublicScripts].filename);
	strcpy(PublicScript[NumPublicScripts].filename,SysFileName(filename));
	PublicScript[NumPublicScripts++].script=s;

	msg_db_l(4);
	return s;
}
#endif

void ExecuteAllScripts()
{
	for (int i=0;i<PrivateScript.size();i++)
		PrivateScript[i]->Execute();
	
	for (int i=0;i<PublicScript.size();i++)
		PublicScript[i]->Execute();
}

void RemoveScript(CScript *s)
{
	msg_db_r("RemoveScript", 0);
	// remove references
	for (int i=0;i<s->pre_script->Include.size();i++)
		s->pre_script->Include[i]->ReferenceCounter --;

	// put on to-delete-list
	DeadScript.push_back(s);

	// remove from normal list
	if (s->isPrivate){
		for (int i=0;i<PrivateScript.size();i++)
			if (PrivateScript[i] == s)
				PrivateScript.erase(PrivateScript.begin() + i);
	}else{
		for (int i=0;i<PublicScript.size();i++)
			if (PublicScript[i] == s)
				PublicScript.erase(PublicScript.begin() + i);
	}

	// delete all deletables
	for (int i=DeadScript.size()-1;i>=0;i--)
		if (DeadScript[i]->ReferenceCounter <= 0){
			dm("script",DeadScript[i]);
			delete(DeadScript[i]);
			DeadScript.erase(DeadScript.begin() + i);
		}
	msg_db_l(0);
}

void DeleteAllScripts(bool even_immortal, bool force)
{
	msg_db_r("DeleteAllScripts", 1);

	// try to erase them...
	for (int i=PublicScript.size()-1;i>=0;i--)
		if ((!PublicScript[i]->pre_script->FlagImmortal) || (even_immortal))
			RemoveScript(PublicScript[i]);
	for (int i=PrivateScript.size()-1;i>=0;i--)
		if ((!PrivateScript[i]->pre_script->FlagImmortal) || (even_immortal))
			RemoveScript(PrivateScript[i]);

	// undead... really KILL!
	if (force){
		for (int i=DeadScript.size()-1;i>=0;i--){
			dm("script",DeadScript[i]);
			delete(DeadScript[i]);
		}
		DeadScript.clear();
	}

	//ScriptResetSemiExternalData();

	
	/*msg_write("------------------------------------------------------------------");
	msg_write(mem_used);
	for (int i=0;i<num_ps;i++)
		msg_write(string2("  fehlt:   %s  %p  (%d)",ppn[i],ppp[i],pps[i]));
	*/
	om();
	msg_db_l(1);
}

void reset_script(CScript *s)
{
	s->ReferenceCounter = 0;
	s->Error = false;
	s->ParserError = false;
	s->LinkerError = false;
	s->isCopy = false;
	s->isPrivate = false;
	
	s->ErrorLine = 0;
	s->ErrorColumn = 0;
	s->WaitingMode = 0;
	s->TimeToWait = 0;
	s->ShowCompilerStats = false;
	
	s->pre_script = NULL;
	s->user_data = NULL;

	s->Opcode = NULL;
	s->OpcodeSize = 0;
	s->ThreadOpcode = NULL;
	s->ThreadOpcodeSize = 0;
	s->Memory = NULL;
	s->MemorySize = 0;
	s->MemoryUsed = 0;
	s->Stack = NULL;

	//func.clear();
	//g_var.clear();
	//cnst.clear();
}

CScript::CScript(const char*filename, bool just_analyse)
{
	msg_db_r("CScript",4);
	reset_script(this);
	push_cur_script(this);
	JustAnalyse = just_analyse;
	msg_write(string("loading script: ",filename));
	msg_right();

	WaitingMode = WaitingModeFirst;
	ShowCompilerStats = true;

	pre_script = new CPreScript(filename,just_analyse);
	am("pre_script",sizeof(CPreScript),pre_script);
	ParserError = Error = pre_script->Error;
	LinkerError = pre_script->IncludeLinkerError;
	ErrorLine = pre_script->ErrorLine;
	ErrorColumn = pre_script->ErrorColumn;
	strcpy(ErrorMsg, pre_script->ErrorMsg);
	strcpy(ErrorMsgExt[0], pre_script->ErrorMsgExt[0]);
	strcpy(ErrorMsgExt[1], pre_script->ErrorMsgExt[1]);

	if ((!Error)&&(!JustAnalyse))
		Compiler();
	/*if (pre_script->FlagShow)
		pre_script->Show();*/
	if (pre_script->FlagDisassemble){
		printf("%s\n\n", Opcode2Asm(ThreadOpcode,ThreadOpcodeSize));
		//printf("%s\n\n", Opcode2Asm(Opcode,OpcodeSize));
		msg_write(Opcode2Asm(Opcode,OpcodeSize));
	}

	pop_cur_script();
	msg_ok();
	msg_left();
	msg_db_l(4);
}

void CScript::DoError(const char *str, int overwrite_line)
{
	pre_script->DoError(str, overwrite_line);
	Error = true;
	ErrorLine = pre_script->ErrorLine;
	ErrorColumn = pre_script->ErrorColumn;
	strcpy(ErrorMsgExt[0], pre_script->ErrorMsgExt[0]);
	strcpy(ErrorMsgExt[1], pre_script->ErrorMsgExt[1]);
	strcpy(ErrorMsg, pre_script->ErrorMsg);
}

void CScript::DoErrorLink(const char *str)
{
	DoError(str);
	LinkerError = true;
}

void CScript::SetVariable(const char*name, void *data)
{
	msg_db_r("SetVariable", 4);
	//msg_write(name);
	for (int i=0;i<pre_script->RootOfAllEvil.Var.size();i++)
		if (strcmp(pre_script->RootOfAllEvil.Var[i].Name, name) == 0){
			/*msg_write("var");
			msg_write(pre_script->RootOfAllEvil.Var[i].Type->Size);
			msg_write((int)g_var[i]);*/
			memcpy(g_var[i], data, pre_script->RootOfAllEvil.Var[i].Type->Size);
			msg_db_l(4);
			return;
		}
	msg_error(string("CScript::SetVariable: variable ", name, " not found"));
	msg_db_l(4);
}

int LocalOffset,LocalOffsetMax;

/*int get_func_temp_size(sFunction *f)
{
}*/

inline int add_temp_var(int size)
{
	LocalOffset += size;
	if (LocalOffset > LocalOffsetMax)
		LocalOffsetMax = LocalOffset;
	return LocalOffset;
}


#define MAX_JUMPS			64
#define MAX_JUMP_LEVELS		128

int OCOffset[MAX_JUMP_LEVELS][MAX_JUMP_LEVELS],TaskReturnOffset;
int NumJumps[MAX_JUMP_LEVELS],JumpSourceOffset[MAX_JUMP_LEVELS][MAX_JUMPS],JumpDest[MAX_JUMP_LEVELS][MAX_JUMPS],JumpCode[MAX_JUMPS][MAX_JUMPS];

struct sNewJump
{
	int SourceLevel, Source;
	int DestLevel, Dest;
	int Code;
};
std::vector<sNewJump> NewJump;
/*struct sJump
{
	int OCOffset[MAX_JUMP_LEVELS][MAX_JUMP_LEVELS],TaskReturnOffset;
int NumJumps[MAX_JUMP_LEVELS],JumpSourceOffset[MAX_JUMP_LEVELS][MAX_JUMPS],JumpDest[MAX_JUMP_LEVELS][MAX_JUMPS],JumpCode[MAX_JUMPS][MAX_JUMPS];
};
std::vector<sJump> Jump;*/
//int NumNewJumps,NewJumpSourceLevel[MAX_JUMPS],NewJumpSource[MAX_JUMPS],NewJumpDest[MAX_JUMPS],NewJumpDestLevel[MAX_JUMPS],NewJumpCode[MAX_JUMPS];

inline void add_jump(int source_level, int source, int dest_level, int dest, int code)
{
	sNewJump n;
	n.SourceLevel = source_level;
	n.Source = source;
	n.DestLevel = dest_level;
	n.Dest = dest;
	n.Code = code;
	NewJump.push_back(n);
}
inline void insert_new_jump(int n, int offset)
{
	int level = NewJump[n].DestLevel;
	JumpSourceOffset[level][NumJumps[level]] = offset;
	JumpDest[level][NumJumps[level]] = NewJump[n].Dest;
	JumpCode[level][NumJumps[level]] = NewJump[n].Code;
	NewJump.erase(NewJump.begin() + n);
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
	inPushM,
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


#define CallRel32OCSize			5
#define AfterWaitOCSize			10



inline void OCAddChar(char *oc,int &ocs,int c)
{	oc[ocs]=(char)c;	ocs++;	}

inline void OCAddWord(char *oc,int &ocs,int i)
{	*(short*)&oc[ocs]=i;	ocs+=2;	}

inline void OCAddInt(char *oc,int &ocs,int i)
{	*(int*)&oc[ocs]=i;	ocs+=4;	}

int OCOParam;

// offset: used to shift addresses   (i.e. mov iteratively to a big local variable)
void OCAddInstruction(char *oc, int &ocs, int inst, int kind, void *param = NULL, int offset = 0)
{
	int code = 0;
	int pk[2] = {PKNone, PKNone};
	void *p[2] = {NULL, NULL};
	int m = -1;
	int size = 32;
	//msg_write(offset);

// corrections
	// lea
	if ((inst == inLeaEaxM) && (kind == KindRefToConst)){
		OCAddInstruction(oc, ocs, inst, KindVarGlobal, param, offset);
		return;
	}

	
	switch(inst){
		case inNop:			code = inst_nop;	break;
		case inPushEbp:		code = inst_push;	pk[0] = PKRegister;	p[0] = (void*)RegEbp;	break;
		case inMovEbpEsp:	code = inst_mov;	pk[0] = PKRegister;	p[0] = (void*)RegEbp;	pk[1] = PKRegister;	p[1] = (void*)RegEsp;	break;
		case inMovEspM:		code = inst_mov;	pk[0] = PKRegister;	p[0] = (void*)RegEsp;	m = 1;	break;
		case inMovEdxpi8Eax:code = inst_mov;	pk[0] = PKEdxRel;	p[0] = param;	pk[1] = PKRegister;	p[1] = (void*)RegEax;	break;
		case inLeave:		code = inst_leave;	break;
		case inRet:			code = inst_ret;	break;
		case inRet4:		code = inst_ret;	pk[0] = PKConstant16;	p[0] = (void*)4;	break;
		case inMovEaxM:		code = inst_mov;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inMovMEax:		code = inst_mov;	pk[1] = PKRegister;	p[1] = (void*)RegEax;	m = 0;	break;
		case inMovEdxM:		code = inst_mov;	pk[0] = PKRegister;	p[0] = (void*)RegEdx;	m = 1;	break;
		case inMovMEdx:		code = inst_mov;	pk[1] = PKRegister;	p[1] = (void*)RegEdx;	m = 0;	break;
		case inMovAlM8:		code = inst_mov_b;	pk[0] = PKRegister;	p[0] = (void*)RegAl;	m = 1;	size = 8;	break;
		case inMovAhM8:		code = inst_mov_b;	pk[0] = PKRegister;	p[0] = (void*)RegAh;	m = 1;	size = 8;	break;
		case inMovBlM8:		code = inst_mov_b;	pk[0] = PKRegister;	p[0] = (void*)RegBl;	m = 1;	size = 8;	break;
		case inMovBhM8:		code = inst_mov_b;	pk[0] = PKRegister;	p[0] = (void*)RegBh;	m = 1;	size = 8;	break;
		case inMovClM8:		code = inst_mov_b;	pk[0] = PKRegister;	p[0] = (void*)RegCl;	m = 1;	size = 8;	break;
		case inMovM8Al:		code = inst_mov_b;	pk[1] = PKRegister;	p[1] = (void*)RegAl;	m = 0;	size = 8;	break;
		case inMovMEbp:		code = inst_mov;	pk[1] = PKRegister;	p[1] = (void*)RegEbp;	m = 0;	break;
		case inMovMEsp:		code = inst_mov;	pk[1] = PKRegister;	p[1] = (void*)RegEsp;	m = 0;	break;
		case inLeaEaxM:		code = inst_lea;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inLeaEdxM:		code = inst_lea;	pk[0] = PKRegister;	p[0] = (void*)RegEdx;	m = 1;	break;
		case inPushM:		code = inst_push;	m = 0;	break;
		case inPushEax:		code = inst_push;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	break;
		case inPushEdx:		code = inst_push;	pk[0] = PKRegister;	p[0] = (void*)RegEdx;	break;
		case inPopEax:		code = inst_pop;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	break;
		case inPopEsp:		code = inst_pop;	pk[0] = PKRegister;	p[0] = (void*)RegEsp;	break;
		case inAndEaxM:		code = inst_and;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inOrEaxM:		code = inst_or;		pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inXorEaxM:		code = inst_xor;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inAddEaxM:		code = inst_add;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inAddEdxM:		code = inst_add;	pk[0] = PKRegister;	p[0] = (void*)RegEdx;	m = 1;	break;
		case inAddMEax:		code = inst_add;	pk[1] = PKRegister;	p[1] = (void*)RegEax;	m = 0;	break;
		case inSubEaxM:		code = inst_sub;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inSubMEax:		code = inst_sub;	pk[1] = PKRegister;	p[1] = (void*)RegEax;	m = 0;	break;
		case inMulEaxM:		code = inst_imul;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inDivEaxM:		code = inst_div;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inDivEaxEbx:	code = inst_div;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	pk[1] = PKRegister;	p[1] = (void*)RegEbx;	break;
		case inCmpEaxM:		code = inst_cmp;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	m = 1;	break;
		case inCmpAlM8:		code = inst_cmp_b;	pk[0] = PKRegister;	p[0] = (void*)RegAl;	m = 1;	size = 8;	break;
		case inCmpM80:		code = inst_cmp_b;	pk[1] = PKConstant8;	p[1] = NULL;	m = 0;	size = 8;	break;
		case inSetzAl:		code = inst_setz_b;	pk[0] = PKRegister;	p[0] = (void*)RegAl;	break;
		case inSetnzAl:		code = inst_setnz_b;pk[0] = PKRegister;	p[0] = (void*)RegAl;	break;
		case inSetnleAl:	code = inst_setnle_b;pk[0] = PKRegister;	p[0] = (void*)RegAl;	break;
		case inSetnlAl:		code = inst_setnl_b;pk[0] = PKRegister;	p[0] = (void*)RegAl;	break;
		case inSetlAl:		code = inst_setl_b;	pk[0] = PKRegister;	p[0] = (void*)RegAl;	break;
		case inSetleAl:		code = inst_setle_b;pk[0] = PKRegister;	p[0] = (void*)RegAl;	break;
		case inAndAlM8:		code = inst_and_b;	pk[0] = PKRegister;	p[0] = (void*)RegAl;	m = 1;	size = 8;	break;
		case inOrAlM8:		code = inst_or_b;	pk[0] = PKRegister;	p[0] = (void*)RegAl;	m = 1;	size = 8;	break;
		case inXorAlM8:		code = inst_xor_b;	pk[0] = PKRegister;	p[0] = (void*)RegAl;	m = 1;	size = 8;	break;
		case inAddAlM8:		code = inst_add_b;	pk[0] = PKRegister;	p[0] = (void*)RegAl;	m = 1;	size = 8;	break;
		case inAddM8Al:		code = inst_add_b;	pk[1] = PKRegister;	p[1] = (void*)RegAl;	m = 0;	size = 8;	break;
		case inSubAlM8:		code = inst_sub_b;	pk[0] = PKRegister;	p[0] = (void*)RegAl;	m = 1;	size = 8;	break;
		case inSubM8Al:		code = inst_sub_b;	pk[1] = PKRegister;	p[1] = (void*)RegAl;	m = 0;	size = 8;	break;
		case inCallRel32:	code = inst_call;	m = 0;	break;
		case inJmpEax:		code = inst_jmp;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	break;
		case inJmpC32:		code = inst_jmp;	m = 0;	break;
		case inJzC8:		code = inst_jz_b;	m = 0;	size = 8;	break;
		case inJzC32:		code = inst_jz;		m = 0;	size = 8;	break;
		case inLoadfM:		code = inst_fld;	m = 0;	break;
		case inSavefM:		code = inst_fstp;	m = 0;	break;
		case inLoadfiM:		code = inst_fild;	m = 0;	break;
		case inAddfM:		code = inst_fadd;	m = 0;	break;
		case inSubfM:		code = inst_fsub;	m = 0;	break;
		case inMulfM:		code = inst_fmul;	m = 0;	break;
		case inDivfM:		code = inst_fdiv;	m = 0;	break;
		case inShrEaxCl:	code = inst_shr;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	pk[1] = PKRegister;	p[1] = (void*)RegCl;	break;
		case inShlEaxCl:	code = inst_shl;	pk[0] = PKRegister;	p[0] = (void*)RegEax;	pk[1] = PKRegister;	p[1] = (void*)RegCl;	break;
		default:
			msg_todo(string2("unhandled instruction: %d", inst));
			cur_script->DoError("internal asm error");
			return;
	}

	
	// const as global var?
	if (kind == KindRefToConst){
		if (!AsmImmediateAllowed(code)){
			//msg_write("evil");
			kind = KindVarGlobal;
		}

		if (inst == inCmpM80){
			kind = KindVarGlobal;
		}
	}
	

	// parameters
	if ((m >= 0) && (kind >= 0)){
	
		if (kind == KindVarLocal){
			pk[m] = PKLocal;
			p[m] = (void*)((long)param + offset);
		}else if (kind == KindVarGlobal){
			pk[m] = PKDerefConstant;
			p[m] = (void*)((long)param + offset);
		}else if (kind == KindConstant){
			pk[m] = (size == 8) ? PKConstant8 : PKConstant32;
			p[m] = param;
		}else if (kind == KindRefToConst){
			kind = KindConstant;
			pk[m] = (size == 8) ? PKConstant8 : PKConstant32;
			p[m] = *(void**)((long)param + offset);
		}else if ((kind == KindRefToLocal) || (kind == KindRefToGlobal)){
			if (kind == KindRefToLocal)
				OCAddInstruction(oc, ocs, inMovEdxM, KindVarLocal, param);
			else if (kind == KindRefToGlobal)
				OCAddInstruction(oc, ocs, inMovEdxM, KindVarGlobal, param);
			if (offset != 0)
				OCAddInstruction(oc, ocs, inAddEdxM, KindConstant, (char*)offset);
			pk[m] = PKDerefRegister;
			p[m] = (void*)RegEdx;
		}else{
			msg_error("kind unhandled");
			msg_write(kind);
		}
	}
	
	// compile
	if (!AsmAddInstruction(oc, ocs, code, pk[0], p[0], pk[1], p[1]))
		cur_script->DoError("internal asm error");

	OCOParam = AsmOCParam;
}

/*enum{
	PKInvalid,
	PKNone,
	PKRegister,			// eAX
	PKDerefRegister,	// [eAX]
	PKLocal,			// [ebp + 0x0000]
	PKStackRel,			// [esp + 0x0000]
	PKConstant32,		// 0x00000000
	PKConstant16,		// 0x0000
	PKConstant8,		// 0x00
	PKConstantDouble,   // 0x00:0x0000   ...
	PKDerefConstant		// [0x0000]
};*/
//bool AsmAddInstruction(char *oc, int &ocs, int inst, int param1_type, void *param1, int param2_type, void *param2, int offset = 0, int insert_at = -1);

void OCAddEspAdd(char *oc,int &ocs,int d)
{
	if (d>0){
		if (d>120)
			AsmAddInstruction(oc, ocs, inst_add, PKRegister, (void*)RegEsp, PKConstant32, (void*)d);
		else
			AsmAddInstruction(oc, ocs, inst_add_b, PKRegister, (void*)RegEsp, PKConstant8, (void*)d);
	}else if (d<0){
		if (d<-120)
			AsmAddInstruction(oc, ocs, inst_sub, PKRegister, (void*)RegEsp, PKConstant32, (void*)(-d));
		else
			AsmAddInstruction(oc, ocs, inst_sub_b, PKRegister, (void*)RegEsp, PKConstant8, (void*)(-d));
	}
}

/*void OCAddCall(char *oc,int &ocs, sFunction *p_func, void *func)
{
	OCAddEspAdd(oc, ocs, - (p_func->VarSize + LocalOffset));
	//OCAddInstruction(oc, ocs, inPushM,pk[1], param[1]);
	//OCAddInstruction(oc, ocs, inPushM,pk[0], param[0]);
	OCAddInstruction(oc, ocs, inCallRel32, KindConstant, (char*)((long)func-(long)&oc[ocs] - CallRel32OCSize));
	OCAddEspAdd(oc, ocs, p_func->VarSize + LocalOffset + 8);
}*/

// create data for a (function) parameter
//   and compile its command if the parameter is executable itself
char *CScript::OCAddParameter(sCommand *link, int n_func, int level, int index, int &pk, bool allow_auto_ref)
{
	msg_db_r("OCAddParameter", 4);
	pk = link->Kind;
	char *ret = NULL;
	//sType *rt=link->;
	//msg_write(Kind2Str(link->Kind));
	if (link->Kind == KindVarFunction){
		so(" -var-func");
		if (pre_script->FlagCompileOS)
			ret = (char*)((long)func[link->LinkNr] - (long)&Opcode[0] + ((sAsmMetaInfo*)pre_script->AsmMetaInfo)->CodeOrigin);
		else
			ret = (char*)func[link->LinkNr];
		pk = KindVarGlobal;
	}else if (link->Kind == KindVarGlobal){
		so(" -global");
		if (link->script)
			ret = link->script->g_var[link->LinkNr];
		else
			ret = g_var[link->LinkNr];
	}else if (link->Kind == KindVarLocal){
		so(" -local");
		ret = (char*)(int)pre_script->Function[n_func].Var[link->LinkNr].Offset;
	}else if (link->Kind == KindVarExternal){
		so(" -external-var");
		ret=(char*)PreExternalVar[link->LinkNr].Pointer;
		pk=KindVarGlobal;
		if (!ret){
			DoErrorLink(string("external variable is not linkable: ",PreExternalVar[link->LinkNr].Name));
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
		ret=cnst[link->LinkNr];
	}else if ((link->Kind==KindOperator) || (link->Kind==KindFunction) || (link->Kind==KindCompilerFunction)){
		pk=KindVarLocal;
		ret=OCAddCommand(link,n_func,level,index);
		if (Error)	_return_(4, NULL);
	}else if (link->Kind==KindAddressShift){
		so(" -p.shift");
		char *param = OCAddParameter(link->Param[0],n_func,level,index,pk,false);
		if (Error)	_return_(4, NULL);
		//if ((pk==KindVarLocal)||(pk==KindVarGlobal)){
		if ((pk==KindVarLocal)||(pk==KindVarGlobal)||(pk==KindRefToConst)){
			so("  ->const");
			ret=param+link->LinkNr;
		}else{
			so("  ->lea");
			if (pk!=KindRefToLocal){
				DoErrorLink(string("unexpected meta type for pointer shift: ",Kind2Str(pk)));
				_return_(4, NULL);
			}
			ret=param;
			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,KindConstant,(char*)(int)link->LinkNr);
			OCAddInstruction(Opcode,OpcodeSize,inAddMEax,KindVarLocal,ret);
			if (Error)	_return_(4, NULL);
			pk=KindRefToLocal;
		}
	}else if (link->Kind==KindDerefAddressShift){
		so(" -deref-shift");
		char *param=OCAddParameter(link->Param[0],n_func,level,index,pk);
		if (Error)	_return_(4, NULL);
		ret=(char*)(-add_temp_var(4) - pre_script->Function[n_func].VarSize);
		OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk,param);
		OCAddInstruction(Opcode,OpcodeSize,inAddEaxM,KindConstant,(char*)(int)link->LinkNr);
		OCAddInstruction(Opcode,OpcodeSize,inMovMEax,KindVarLocal,ret);
		if (Error)	_return_(4, NULL);
		pk=KindRefToLocal;
	}else if (link->Kind==KindArray){
		so(" -array");
		char *param=OCAddParameter(link->Param[0],n_func,level,index,pk,false);
		if (Error) _return_(4, NULL);
		if ((link->Param[1]->Kind==KindConstant)&&( (pk==KindVarLocal)||(pk==KindVarGlobal) )){
			so("  ->const");
			ret=param+(*(int*)pre_script->Constant[link->Param[1]->LinkNr].data)*link->Type->Size;
		}else{
			so("  ->lea");
			if (pk!=KindRefToLocal){
				so("    ->neu");
				OCAddInstruction(Opcode,OpcodeSize,inLeaEaxM,pk,param);
				ret=(char*)(-add_temp_var(4) - pre_script->Function[n_func].VarSize);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,KindVarLocal,ret);
			}else
				ret=param;
			param=OCAddParameter(link->Param[1],n_func,level,index,pk,false);
			if (Error)	_return_(4, NULL);
			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk,param);
			OCAddInstruction(Opcode,OpcodeSize,inMulEaxM,KindConstant,(char*)link->Type->Size);
			OCAddInstruction(Opcode,OpcodeSize,inAddMEax,KindVarLocal,ret);
			if (Error)	_return_(4, NULL);
			pk=KindRefToLocal;
		}
	}else if (link->Kind==KindPointerAsArray){
		so(" -pointer-array");
		char *param=OCAddParameter(link->Param[0],n_func,level,index,pk,false);
		if (Error)	_return_(4, NULL);
		//so("->lea");
		//if (pk!=KindRefToLocal){
			so("  ->neu");
			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk,param);
			ret=(char*)(-add_temp_var(4) - pre_script->Function[n_func].VarSize);
			OCAddInstruction(Opcode,OpcodeSize,inMovMEax,KindVarLocal,ret);
		//}else
		//	ret=param;
		param=OCAddParameter(link->Param[1],n_func,level,index,pk,false);
		if (Error)	_return_(4, NULL);
		OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk,param);
		OCAddInstruction(Opcode,OpcodeSize,inMulEaxM,KindConstant,(char*)link->Type->Size);
		OCAddInstruction(Opcode,OpcodeSize,inAddMEax,KindVarLocal,ret);
		if (Error)	_return_(4, NULL);
		pk=KindRefToLocal;
	}else if (link->Kind==KindReference){
		//msg_write(Kind2Str(link->Meta->Kind));
		so(" -ref");
		char *param=OCAddParameter(link->Param[0],n_func,level,index,pk,false);
		if (Error)	_return_(4, NULL);
		//printf("%d  -  %s\n",pk,Kind2Str(pk));
		if ((pk==KindConstant)||(pk==KindVarGlobal)||(pk==KindRefToConst)){
			so("  Reference-Const");
			pk=KindConstant;
			ret=param;
		}else{
			ret=(char*)(-add_temp_var(4) - pre_script->Function[n_func].VarSize);
			OCAddInstruction(Opcode,OpcodeSize,inLeaEaxM,pk,param);
			pk=KindVarLocal;
			OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk,ret);
			if (Error)	_return_(4, NULL);
		}
	}else if (link->Kind==KindDereference){
		so(" -deref...");
		char *param=OCAddParameter(link->Param[0],n_func,level,index,pk);
		if (Error)	_return_(4, NULL);
		if ((pk==KindVarLocal)||(pk==KindVarGlobal)){
			if (pk==KindVarLocal)	pk=KindRefToLocal;
			if (pk==KindVarGlobal)	pk=KindRefToGlobal;
			ret=param;
		}
	}else
		_do_error_(string("unexpected type of parameter: ",Kind2Str(link->Kind)), 4, NULL);
	//if ((!link->type->IsPointer)&&(link->type->SubType)){
	if ((allow_auto_ref) && ((link->Type->IsArray) || (link->Type->IsSuperArray))){
		so("Array: c referenziert automatisch!!");
		char *param=ret;
		ret=(char*)(-add_temp_var(4) - pre_script->Function[n_func].VarSize);
		OCAddInstruction(Opcode,OpcodeSize,inLeaEaxM,pk,param);
		pk=KindVarLocal;
		OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk,ret);
		if (Error)	_return_(4, NULL);
		link->Type=TypePointer;
	}
	msg_db_l(4);
	return ret;
}

void OCAddOperator(char *Opcode, int &OpcodeSize, CScript *s, sCommand *com, sFunction *p_func, char **param, char *&ret, int *pk, int &rk)
{
	msg_db_r("OCAddOperator", 4);
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
		case OperatorClassAssign:
			for (int i=0;i<signed(com->Param[0]->Type->Size)/4;i++){
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[1],param[1],i*4);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,pk[0],param[0],i*4);
			}
			for (int i=4*signed(com->Param[0]->Type->Size/4);i<signed(com->Param[0]->Type->Size);i++){
				OCAddInstruction(Opcode,OpcodeSize,inMovAlM8,pk[1],param[1],i);
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,pk[0],param[0],i);
			}
			break;
// string   TODO: create own code!
		case OperatorStringAssignAA:
	//	case OperatorStringAssignAP:
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LocalOffset);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[1],param[1]);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[0],param[0]);
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcpy-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LocalOffset+8);
			break;
		case OperatorStringAddAAS:
	//	case OperatorStringAddAPS:
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LocalOffset);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[1],param[1]);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[0],param[0]);
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcat-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LocalOffset+8);
			break;
		case OperatorStringAddAA:
	/*	case OperatorStringAddAP:
		case OperatorStringAddPA:
		case OperatorStringAddPP:*/
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LocalOffset);
			so(d2h((char*)&param[0],4,false));
			so(d2h((char*)&ret,4,false));
			OCAddInstruction(Opcode,OpcodeSize,inLeaEdxM,rk,ret);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[0],param[0]);
			OCAddInstruction(Opcode,OpcodeSize,inPushEdx,-1);
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcpy-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
			OCAddEspAdd(Opcode,OpcodeSize,8);
			OCAddInstruction(Opcode,OpcodeSize,inLeaEdxM,rk,ret);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[1],param[1]);
			OCAddInstruction(Opcode,OpcodeSize,inPushEdx,-1);
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcat-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LocalOffset+8);
			break;
		case OperatorStringEqualAA:
	/*	case OperatorStringEqualAP:
		case OperatorStringEqualPA:
		case OperatorStringEqualPP:*/
		case OperatorStringNotEqualAA:
	/*	case OperatorStringNotEqualAP:
		case OperatorStringNotEqualPA:
		case OperatorStringNotEqualPP:*/
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LocalOffset);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[1],param[1]);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[0],param[0]);
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)((long)&strcmp-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LocalOffset+8);
			OCAddInstruction(Opcode,OpcodeSize,inCmpAlM8,KindConstant,NULL);
			if ((com->LinkNr==OperatorStringEqualAA)/*||(com->LinkNr==OperatorStringEqualAP)||(com->LinkNr==OperatorStringEqualPA)||(com->LinkNr==OperatorStringEqualPP)*/)
				OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
			if ((com->LinkNr==OperatorStringNotEqualAA)/*||(com->LinkNr==OperatorStringNotEqualAP)||(com->LinkNr==OperatorStringNotEqualPA)||(com->LinkNr==OperatorStringNotEqualPP)*/)
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
			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
			AsmAddInstruction(Opcode, OpcodeSize, inst_mov, PKRegister, (void*)RegEdx, PKRegister, (void*)RegEax);
			AsmAddInstruction(Opcode, OpcodeSize, inst_sar, PKRegister, (void*)RegEdx, PKConstant8, (void*)0x1f);
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
			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
			AsmAddInstruction(Opcode, OpcodeSize, inst_mov, PKRegister, (void*)RegEdx, PKRegister, (void*)RegEax);
			AsmAddInstruction(Opcode, OpcodeSize, inst_sar, PKRegister, (void*)RegEdx, PKConstant8, (void*)0x1f);
			OCAddInstruction(Opcode,OpcodeSize,inDivEaxM,pk[1],param[1]);
			OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
			break;
		case OperatorIntModulo:
			OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
			AsmAddInstruction(Opcode, OpcodeSize, inst_mov, PKRegister, (void*)RegEdx, PKRegister, (void*)RegEax);
			AsmAddInstruction(Opcode, OpcodeSize, inst_sar, PKRegister, (void*)RegEdx, PKConstant8, (void*)0x1f);
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
			//		AsmAddInstruction(Opcode, OpcodeSize, inst_fxchg, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fucompp, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fnstsw, PKRegister, (void*)RegAx, PKNone, NULL);
					AsmAddInstruction(Opcode, OpcodeSize, inst_and_b, PKRegister, (void*)RegAh, PKConstant8, (void*)0x45);
					AsmAddInstruction(Opcode, OpcodeSize, inst_cmp_b, PKRegister, (void*)RegAh, PKConstant8, (void*)0x40);
					OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				}else if (com->LinkNr==OperatorFloatNotEqual){
				//	AsmAddInstruction(Opcode, OpcodeSize, inst_fxchg, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fucompp, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fnstsw, PKRegister, (void*)RegAx, PKNone, NULL);
					AsmAddInstruction(Opcode, OpcodeSize, inst_and_b, PKRegister, (void*)RegAh, PKConstant8, (void*)0x45);
					AsmAddInstruction(Opcode, OpcodeSize, inst_cmp_b, PKRegister, (void*)RegAh, PKConstant8, (void*)0x40);
					OCAddInstruction(Opcode,OpcodeSize,inSetnzAl,-1);
				}else if (com->LinkNr==OperatorFloatSmaller){
					AsmAddInstruction(Opcode, OpcodeSize, inst_fucompp, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fnstsw, PKRegister, (void*)RegAx, PKNone, NULL);
					AsmAddInstruction(Opcode, OpcodeSize, inst_test_b, PKRegister, (void*)RegAh, PKConstant8, (void*)0x45);
					OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				}else if (com->LinkNr==OperatorFloatSmallerEqual){
					AsmAddInstruction(Opcode, OpcodeSize, inst_fucompp, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fnstsw, PKRegister, (void*)RegAx, PKNone, NULL);
					AsmAddInstruction(Opcode, OpcodeSize, inst_test_b, PKRegister, (void*)RegAh, PKConstant8, (void*)0x05);
					OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				}else if (com->LinkNr==OperatorFloatGreater){
					AsmAddInstruction(Opcode, OpcodeSize, inst_fxchg, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fucompp, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fnstsw, PKRegister, (void*)RegAx, PKNone, NULL);
					AsmAddInstruction(Opcode, OpcodeSize, inst_test_b, PKRegister, (void*)RegAh, PKConstant8, (void*)0x45);
					OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				}else if (com->LinkNr==OperatorFloatGreaterEqual){
					AsmAddInstruction(Opcode, OpcodeSize, inst_fxchg, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fucompp, PKRegister, (void*)RegSt0, PKRegister, (void*)RegSt1);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fnstsw, PKRegister, (void*)RegAx, PKNone, NULL);
					AsmAddInstruction(Opcode, OpcodeSize, inst_test_b, PKRegister, (void*)RegAh, PKConstant8, (void*)0x05);
					OCAddInstruction(Opcode,OpcodeSize,inSetzAl,-1);
				}
				OCAddInstruction(Opcode,OpcodeSize,inMovM8Al,rk,ret);
				break;
			case OperatorFloatNegate:
				OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inXorEaxM,KindConstant,(char*)0x80000000);
				OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				break;
// complex
			case OperatorComplexAddS:
			case OperatorComplexSubtractS:
			//case OperatorComplexMultiplySCF:
			//case OperatorComplexDivideS:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				if (com->LinkNr==OperatorComplexAddS)		OCAddInstruction(Opcode,OpcodeSize,inAddfM,pk[1],param[1]);
				if (com->LinkNr==OperatorComplexSubtractS)	OCAddInstruction(Opcode,OpcodeSize,inSubfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],4);
				if (com->LinkNr==OperatorComplexAddS)		OCAddInstruction(Opcode,OpcodeSize,inAddfM,pk[1],param[1],4);
				if (com->LinkNr==OperatorComplexSubtractS)	OCAddInstruction(Opcode,OpcodeSize,inSubfM,pk[1],param[1],4);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,pk[0],param[0],4);
				break;
			case OperatorComplexAdd:
			case OperatorComplexSubtract:
//			case OperatorFloatMultiply:
//			case OperatorFloatDivide:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				if (com->LinkNr==OperatorComplexAdd)		OCAddInstruction(Opcode,OpcodeSize,inAddfM,pk[1],param[1]);
				if (com->LinkNr==OperatorComplexSubtract)	OCAddInstruction(Opcode,OpcodeSize,inSubfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],4);
				if (com->LinkNr==OperatorComplexAdd)		OCAddInstruction(Opcode,OpcodeSize,inAddfM,pk[1],param[1],4);
				if (com->LinkNr==OperatorComplexSubtract)	OCAddInstruction(Opcode,OpcodeSize,inSubfM,pk[1],param[1],4);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret,4);
				break;
			case OperatorComplexMultiply:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],4);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1],4);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSubfM,rk,ret);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],4);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret,4);
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1],4);
				OCAddInstruction(Opcode,OpcodeSize,inAddfM,rk,ret,4);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret,4);
				break;
			case OperatorComplexMultiplyFC:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1],4);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret,4);
				break;
			case OperatorComplexMultiplyCF:
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0]);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
				OCAddInstruction(Opcode,OpcodeSize,inLoadfM,pk[0],param[0],4);
				OCAddInstruction(Opcode,OpcodeSize,inMulfM,pk[1],param[1]);
				OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret,4);
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
// super arrays
		case OperatorSuperArrayAssign:
//		case OperatorIntListAssign:
//		case OperatorFloatListAssign:
		case OperatorIntListAssignInt:
		case OperatorFloatListAssignFloat:
		case OperatorIntListAddS:
		case OperatorIntListSubtractS:
		case OperatorIntListMultiplyS:
		case OperatorIntListDivideS:
		case OperatorFloatListAddS:
		case OperatorFloatListSubtractS:
		case OperatorFloatListMultiplyS:
		case OperatorFloatListDivideS:
		case OperatorComplexListAddS:
		case OperatorComplexListSubtractS:
		case OperatorComplexListMultiplyS:
		case OperatorComplexListDivideS:{
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LocalOffset-8);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[1],param[1]);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[0],param[0]);
			long func;
			if (com->LinkNr == OperatorIntListAssignInt)	func = (long)&super_array_assign_4_single;
			if (com->LinkNr == OperatorFloatListAssignFloat)func = (long)&super_array_assign_4_single;
			if (com->LinkNr == OperatorSuperArrayAssign)	func = (long)&super_array_assign;
			if (com->LinkNr == OperatorIntListAddS)			func = (long)&super_array_add_s_int;
			if (com->LinkNr == OperatorIntListSubtractS)	func = (long)&super_array_sub_s_int;
			if (com->LinkNr == OperatorIntListMultiplyS)	func = (long)&super_array_mul_s_int;
			if (com->LinkNr == OperatorIntListDivideS)		func = (long)&super_array_div_s_int;
			if (com->LinkNr == OperatorFloatListAddS)		func = (long)&super_array_add_s_float;
			if (com->LinkNr == OperatorFloatListSubtractS)	func = (long)&super_array_sub_s_float;
			if (com->LinkNr == OperatorFloatListMultiplyS)	func = (long)&super_array_mul_s_float;
			if (com->LinkNr == OperatorFloatListDivideS)	func = (long)&super_array_div_s_float;
			if (com->LinkNr == OperatorComplexListAddS)		func = (long)&super_array_add_s_com;
			if (com->LinkNr == OperatorComplexListSubtractS)	func = (long)&super_array_sub_s_com;
			if (com->LinkNr == OperatorComplexListMultiplyS)	func = (long)&super_array_mul_s_com;
			if (com->LinkNr == OperatorComplexListDivideS)	func = (long)&super_array_div_s_com;
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)(func-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LocalOffset+16);
			}break;
		/*case OperatorIntListAdd:
		case OperatorIntListSubtract:
		case OperatorIntListMultiply:
		case OperatorIntListDivide:{
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LocalOffset-12);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[1],param[1]);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[0],param[0]);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,rk,ret);
			long func;
			if (com->LinkNr == OperatorIntListAdd)			func = (long)&super_array_add_int;
			if (com->LinkNr == OperatorIntListSubtract)		func = (long)&super_array_sub_int;
			if (com->LinkNr == OperatorIntListMultiply)		func = (long)&super_array_mul_int;
			if (com->LinkNr == OperatorIntListDivide)		func = (long)&super_array_div_int;
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)(func-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LocalOffset+20);
		}break;*/
		case OperatorIntListAddSInt:
		case OperatorIntListSubtractSInt:
		case OperatorIntListMultiplySInt:
		case OperatorIntListDivideSInt:
		case OperatorFloatListAddSFloat:
		case OperatorFloatListSubtractSFloat:
		case OperatorFloatListMultiplySFloat:
		case OperatorFloatListDivideSFloat:
		case OperatorComplexListMultiplySFloat:{
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LocalOffset-8);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[1],param[1]);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[0],param[0]);
			long func;
			if (com->LinkNr == OperatorIntListAddSInt)			func = (long)&super_array_add_s_int_int;
			if (com->LinkNr == OperatorIntListSubtractSInt)		func = (long)&super_array_sub_s_int_int;
			if (com->LinkNr == OperatorIntListMultiplySInt)		func = (long)&super_array_mul_s_int_int;
			if (com->LinkNr == OperatorIntListDivideSInt)		func = (long)&super_array_div_s_int_int;
			if (com->LinkNr == OperatorFloatListAddSFloat)		func = (long)&super_array_add_s_float_float;
			if (com->LinkNr == OperatorFloatListSubtractSFloat)	func = (long)&super_array_sub_s_float_float;
			if (com->LinkNr == OperatorFloatListMultiplySFloat)	func = (long)&super_array_mul_s_float_float;
			if (com->LinkNr == OperatorFloatListDivideSFloat)	func = (long)&super_array_div_s_float_float;
			if (com->LinkNr == OperatorComplexListMultiplySFloat)	func = (long)&super_array_mul_s_com_float;
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)(func-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LocalOffset+16);
			}break;
		case OperatorComplexListAssignComplex:
		case OperatorComplexListAddSComplex:
		case OperatorComplexListSubtractSComplex:
		case OperatorComplexListMultiplySComplex:
		case OperatorComplexListDivideSComplex:{
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LocalOffset-8);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[1],param[1],4);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[1],param[1]);
			OCAddInstruction(Opcode,OpcodeSize,inPushM,pk[0],param[0]);
			long func;
			if (com->LinkNr == OperatorComplexListAssignComplex)		func = (long)&super_array_assign_8_single;
			if (com->LinkNr == OperatorComplexListAddSComplex)			func = (long)&super_array_add_s_com_com;
			if (com->LinkNr == OperatorComplexListSubtractSComplex)		func = (long)&super_array_sub_s_com_com;
			if (com->LinkNr == OperatorComplexListMultiplySComplex)		func = (long)&super_array_mul_s_com_com;
			if (com->LinkNr == OperatorComplexListDivideSComplex)		func = (long)&super_array_div_s_com_com;
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)(func-(long)&Opcode[OpcodeSize]-CallRel32OCSize));
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LocalOffset+16);
			}break;
		default:
			s->DoError(string("unimplemented operator (call Michi!): ",Operator2Str(s->pre_script, com->LinkNr)));
	}

	if (AsmError)
			s->DoError("internal asm error");
	msg_db_l(4);
}

static int while_level = 0;
static int while_index = 0;

char *CScript::OCAddCommand(sCommand *com,int n_func,int level,int index)
{
	msg_db_r("OCAddCommand", 4);
	//msg_write(Kind2Str(com->Kind));
	sFunction *p_func = &pre_script->Function[n_func];
	char *param[SCRIPT_MAX_PARAMS];
	int s = mem_align(com->Type->Size);
	char *ret = (char*)( -add_temp_var(s) - p_func->VarSize);
	//so(d2h((char*)&ret,4,false));
	so(string2("return: %d/%d/%d", com->Type->Size, LocalOffset, LocalOffset));
	int pk[SCRIPT_MAX_PARAMS], rk = KindVarLocal; // param_kind, return_kind

	// compile parameters
	for (int p=0;p<com->NumParams;p++){
		param[p] = OCAddParameter(com->Param[p],n_func,level,index,pk[p]);
		if (Error) _return_(4, NULL);
	}

	// class function -> compile instance
	bool is_class_function = false;
	if (com->Kind == KindCompilerFunction)
		if (PreCommand[com->LinkNr].IsClassFunction)
			is_class_function = true;
	if (com->Kind == KindFunction)
		if (strstr(pre_script->Function[com->LinkNr].Name, "."))
			is_class_function = true;
	char *instance_param = NULL;
	int instance_kind;
	if (is_class_function){
		so("member");
		instance_param = OCAddParameter(com->Sub1,n_func,level,index,instance_kind);
		so(Kind2Str(instance_kind));
		// super_array automatically referenced...
	}

	    
	if (com->Kind == KindOperator){
		//msg_write("---operator");
		OCAddOperator(Opcode, OpcodeSize, this, com, p_func, param, ret, pk, rk);
		if (Error)
			_return_(4, NULL);
		
	}else if ((com->Kind == KindCompilerFunction) || (com->Kind == KindFunction)){
		//msg_write("---func");
		t_func *f = NULL;
		bool class_function = false;
		char name[128];
		if (com->Kind == KindFunction){ // own script Function
			so("Funktion!!!");
			if (com->script){
				//msg_write(com->LinkNr);
				so("    extern!!!");
				f = com->script->func[com->LinkNr];
				so("   -ok");
			}else{
				f = func[com->LinkNr];
				if (strstr(pre_script->Function[com->LinkNr].Name, ".")){
					msg_error("class   member--------");
					class_function = true;
				}
			}
		}else{ // compiler function
			f = (t_func*)PreCommand[com->LinkNr].Func;
			class_function = PreCommand[com->LinkNr].IsClassFunction;
			strcpy(name, PreCommand[com->LinkNr].Name);
		}
		if (f){ // a real function
			if ((com->Type->Size > 4) && (!com->Type->IsArray))
				OCAddInstruction(Opcode,OpcodeSize,inLeaEaxM,rk,ret);

			// grow stack (down) for local variables of the calling function
			OCAddEspAdd(Opcode,OpcodeSize,-p_func->VarSize-LocalOffset-8);
			int dp=0;

			for (int p=com->NumParams-1;p>=0;p--){
				int s = mem_align(com->Param[p]->Type->Size);
				// push parameters onto stack
				for (int j=0;j<s/4;j++)
					OCAddInstruction(Opcode, OpcodeSize, inPushM, pk[p], param[p], s - 4 - j * 4);
				dp += s;
			}

#ifdef NIX_IDE_VCS
			// more than 4 byte have to be returned -> give return address as very last parameter!
			if ((com->Type->Size > 4) && (!com->Type->IsArray))
				OCAddInstruction(Opcode,OpcodeSize,inPushEax,-1); // nachtraegliche eSP-Korrektur macht die Funktion
#endif
			// _cdecl: Klassen-Instanz als ersten Parameter push'en
			if (class_function){
				//if ((unsigned long)instance>100)	OCAddInstruction(Opcode, OpcodeSize, inPushM, KindConstant,*(void**)instance);
				/*else if (instance == f_class)*/		OCAddInstruction(Opcode, OpcodeSize, inPushM, instance_kind, instance_param);
				//else								OCAddInstruction(Opcode, OpcodeSize, inPushM, KindVarGlobal,instance);
				dp+=4;
			}
#ifndef NIX_IDE_VCS
			// more than 4 byte have to be returned -> give return address as very first parameter!
			if ((com->Type->Size > 4) && (!com->Type->IsArray))
				OCAddInstruction(Opcode,OpcodeSize,inPushEax,-1); // nachtraegliche eSP-Korrektur macht die Funktion
#endif
			long d=(long)f-(long)&Opcode[OpcodeSize]-CallRel32OCSize;
			//so(d);
			OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,(char*)d); // the actual call
			OCAddEspAdd(Opcode,OpcodeSize,p_func->VarSize+LocalOffset+dp+8);

			// return > 4b already got copied to [ret] by the function!
			if (com->Type != TypeVoid)
				if ((com->Type->Size <= 4) || (com->Type->IsArray)){
					if (com->Type == TypeFloat)
						OCAddInstruction(Opcode,OpcodeSize,inSavefM,rk,ret);
					else
						OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
				}
		}else if (PreCommand[com->LinkNr].IsSpecial){
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
						if (com->Param[0]->Type->Size > 4){ // Return in erhaltener Return-Adresse [ebp+0x08] speichern (> 4 byte)
							OCAddInstruction(Opcode,OpcodeSize,inMovEdxM,KindVarLocal,(char*)8);
							int s = mem_align(com->Param[0]->Type->Size);
							for (int j=0;j<s/4;j++){
								OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0],j*4);
								OCAddInstruction(Opcode,OpcodeSize,inMovEdxpi8Eax,KindConstant,(char*)(j*4));
							}
						}else{ // Return direkt in eAX speichern (4 byte)
							if (com->Param[0]->Type == TypeFloat)
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
				case CommandFloatToInt:
				{
					if (pk[0] == KindVarLocal){
						AsmAddInstruction(Opcode, OpcodeSize, inst_fld, PKLocal, param[0], PKNone, NULL);
					}else{
						int t0 = - add_temp_var(4) - pre_script->Function[n_func].VarSize;
						OCAddInstruction(Opcode, OpcodeSize, inMovEaxM, pk[0], param[0]);
						OCAddInstruction(Opcode, OpcodeSize, inMovMEax, KindVarLocal, (void*)t0);
						AsmAddInstruction(Opcode, OpcodeSize, inst_fld, PKLocal, (void*)t0, PKNone, NULL);
					}
					int t1 = - add_temp_var(2) - pre_script->Function[n_func].VarSize;
					int t2 = - add_temp_var(2) - pre_script->Function[n_func].VarSize;
					AsmAddInstruction(Opcode, OpcodeSize, inst_fnstcw, PKLocal, (void*)t1, PKNone, NULL);
					AsmAddInstruction(Opcode, OpcodeSize, inst_movzx, PKRegister, (void*)RegEax, PKLocal, (void*)t1);
					//AsmAddInstruction(Opcode, OpcodeSize, inst_and, PKRegister, (void*)RegEax, PKConstant32, (void*)0xffffff0c);
					OCAddInstruction(Opcode, OpcodeSize, inMovAhM8, KindConstant, (void*)0x0c);
					//AsmAddInstruction(Opcode, OpcodeSize, inst_mov, PKLocal, (void*)t2, PKRegister, (void*)RegAx);
					OCAddChar(Opcode, OpcodeSize, 0x66); // my asm library is not powerful enough (T_T)
					OCAddChar(Opcode, OpcodeSize, 0x89);
					OCAddChar(Opcode, OpcodeSize, 0x45);
					OCAddChar(Opcode, OpcodeSize, t2);
					AsmAddInstruction(Opcode, OpcodeSize, inst_fldcw, PKLocal, (void*)t2, PKNone, NULL);
					if (rk == KindVarLocal){
						AsmAddInstruction(Opcode, OpcodeSize, inst_fistp, PKLocal, ret, PKNone, NULL);
					}else{
						int t3 = - add_temp_var(4) - pre_script->Function[n_func].VarSize;
						AsmAddInstruction(Opcode, OpcodeSize, inst_fistp, PKLocal, (void*)t3, PKNone, NULL);
						OCAddInstruction(Opcode, OpcodeSize, inMovEaxM, KindVarLocal, (void*)t3);
						OCAddInstruction(Opcode, OpcodeSize, inMovMEax, rk, ret);
					}
					AsmAddInstruction(Opcode, OpcodeSize, inst_fldcw, PKLocal, (void*)t1, PKNone, NULL);
				}
					
				/*	fld [ebp + 0xfc]                                 // d9.45.fc		p
					fnstcw [ebp + 0xee]                              // d9.7d.ee		t1
					movzx eax, [ebp + 0xee]                          // 0f.b7.45.ee		t1
					mov.b ah, 0x0c                                   // b4.0c
					mov word [ebp + 0xec], ax                        // 66.89.45.ec		t2
					fldcw [ebp + 0xec]                               // d9.6d.ec		t2
					fistp [ebp + 0xf8]                               // db.5d.f8		r
					fldcw [ebp + 0xee]                               // d9.6d.ee		t1	*/

					break;
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
						const char *pac;
						
						pac=Asm2Opcode(pre_script->AsmBlock[0].block);
						so("f");
						if (!AsmError){
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
							AsmErrorLine--; // (T_T)
							DoError("error in assembler code...", AsmErrorLine);
							_return_(4, ret);
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
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[2],param[2]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret,8);
				case CommandComplexSet:
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[0],param[0]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret);
					OCAddInstruction(Opcode,OpcodeSize,inMovEaxM,pk[1],param[1]);
					OCAddInstruction(Opcode,OpcodeSize,inMovMEax,rk,ret,4);
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
			if (PreCommand[com->LinkNr].IsSemiExternal)
	 			DoErrorLink(string("external function not linkable: ",PreCommand[com->LinkNr].Name));
			else
	 			DoErrorLink(string("compiler function not linkable: ",PreCommand[com->LinkNr].Name));
			_return_(4, ret);
		}
	}else if (com->Kind==KindBlock){
		//msg_write("---block");
		OCAddBlock(pre_script->Block[com->LinkNr],n_func,level+1);
		if (Error)	_return_(4, NULL);
	}else{
		//msg_write("---???");
		//_do_error_(string("type of command is unimplemented (call Michi!): ",Kind2Str(com->Kind)), 4, NULL);
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
		LocalOffset=0;
		OCOffset[level][i] = OpcodeSize;
		OCAddCommand(block->Command[i],n_func,level,i);
		if (Error)	_return_(4,);
		//int offset_after_cmd = OpcodeSize;

		// create dummy code for jumps from this level (if, while,..)
		for (int j=0;j<NewJump.size();j++){
			if ((NewJump[j].SourceLevel == level) && (NewJump[j].Source == i + 1)){ // put it here! (jump before the next command)
				if (NewJump[j].Code < 0){
					//int d=OCOffset[level][JumpDest[level][j]]-OpcodeSize-5;//-2;
					//so(string("Jump....",i2s(d)));
					//OCAddChar(Opcode,OpcodeSize,(char)0xeb);	OCAddChar(Opcode,OpcodeSize,char(0));	NewJump[j].Code=OpcodeSize-1;
					OCAddChar(Opcode,OpcodeSize,(char)0xe9);	OCAddInt(Opcode,OpcodeSize,0);        	NewJump[j].Code=OpcodeSize-4;
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

void init_sub_super_array(CPreScript *ps, sFunction *f, sType *t, char* g_var, int offset)
{
	// direct
	if (t->IsSuperArray){
		if (g_var)
			((CSuperArray*)(g_var + offset))->init_by_type(t->SubType);
		if (f){}
	}

	// indirect
	if (t->IsArray)
		for (int i=0;i<t->ArrayLength;i++)
			init_sub_super_array(ps, f, t->SubType, g_var, offset + i * t->SubType->Size);
	if (t->Class)
		for (int i=0;i<t->Class->Element.size();i++)
			init_sub_super_array(ps, f, t->Class->Element[i].Type, g_var, offset + t->Class->Element[i].Offset);
}

void find_all_super_arrays(CPreScript *ps, sFunction *f, std::vector<char*> g_var)
{
	for (int i=0;i<f->Var.size();i++)
		init_sub_super_array(ps, f, f->Var[i].Type, g_var[i], 0);
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
	if (MemorySize > 0){
		Memory = new char[MemorySize];
		am("Memory",MemorySize,Memory);
	}

	// use your own stack if needed
	//   wait() used -> needs to switch stacks ("tasks")
	Stack = NULL;
	for (int i=0;i<pre_script->Command.size();i++){
		sCommand *cmd = pre_script->Command[i];
		if (cmd->Kind == KindCompilerFunction)
			if ((cmd->LinkNr == CommandWait) || (cmd->LinkNr == CommandWaitRT) || (cmd->LinkNr == CommandWaitOneFrame)){
				Stack=new char[ScriptStackSize];
				am("Stack",ScriptStackSize,Stack);
				break;
			}
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
	// initialize global super arrays
	find_all_super_arrays(pre_script, &pre_script->RootOfAllEvil, g_var);

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
		LocalOffset=LocalOffsetMax=0;

		// intro
		OCAddInstruction(Opcode,OpcodeSize,inPushEbp,-1);
		OCAddInstruction(Opcode,OpcodeSize,inMovEbpEsp,-1);

		// function
		NewJump.clear();
		OCAddBlock(ff->Block,f,0);
		if (Error)	_return_(2,);

		if (NewJump.size() > 0)
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

	WaitingMode = WaitingModeFirst;
	
	if (ShowCompilerStats){
		msg_write("--------------------------------");
		msg_write(string2("Opcode: %db, Memory: %db",OpcodeSize,MemorySize));
	}
	om();
	msg_db_l(2);
}

CScript::CScript()
{
	so("creating empty script (for console)");
	right();
	reset_script(this);
	WaitingMode = WaitingModeFirst;

	pre_script = new CPreScript();
		am("PreScript",sizeof(CPreScript),pre_script);
	
	strcpy(pre_script->Filename, "-console script-");

	so("-ok");
	left();
}

CScript::~CScript()
{
	msg_db_r("~CScript", 4);
	if ((Memory) && (!JustAnalyse)){
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
	g_var.clear();
	cnst.clear();
	//msg_write(string2("-----------            Memory:         %p",Memory));
	delete(pre_script);
	dm("pre_script",pre_script);
	msg_db_l(4);
}



static char single_command[1024];


// bad:  should clean up in case of errors!
void ExecuteSingleScriptCommand(const char *cmd)
{
	if (strlen(cmd) < 1)
		return;
	msg_db_r("ExecuteSingleScriptCmd", 2);
	strcpy(single_command, cmd);
	msg_write(string("script command: ", single_command));

	// empty script
	CScript *s = new CScript();
	CPreScript *ps = s->pre_script;

// find expressions
	ps->Analyse(single_command, false);
	if (ps->Exp.line[0].exp.size() < 1){
		//clear_exp_buffer(&ps->Exp);
		delete(s);
		msg_db_l(2);
		return;
	}

// analyse syntax

	// create a main() function
	int func = ps->AddFunction("main", TypeVoid);
	sFunction *f = &ps->Function[func];
	f->VarSize = 0; // set to -1...

	// parse
	ps->Exp.cur_line = &ps->Exp.line[0];
	ps->Exp.cur_exp = 0;
	ps->GetCompleteCommand(f->Block, f);
	//pre_script->GetCompleteCommand((pre_script->Exp->ExpNr,0,0,&f);
	s->Error |= ps->Error;

	if (!s->Error)
		ps->ConvertCallByReference();

// compile
	if (!s->Error)
		s->Compiler();

	/*if (true){
		printf("%s\n\n", Opcode2Asm(s->ThreadOpcode,s->ThreadOpcodeSize));
		printf("%s\n\n", Opcode2Asm(s->Opcode,s->OpcodeSize));
		//msg_write(Opcode2Asm(Opcode,OpcodeSize));
	}*/
// execute
	if (!s->Error)
		s->Execute();

	delete(s);
	msg_db_l(2);
}

bool CScript::ExecuteScriptFunction(const char*name,...)
{
	msg_db_m("-ExecuteScriptFunction",2);
	msg_db_m(name,2);
	msg_db_m(pre_script->Filename,2);

	if ((pre_script->GetExistence(name,&pre_script->RootOfAllEvil))&&(pre_script->GetExistenceLink.Kind==KindFunction)){

		sFunction *f=&pre_script->Function[pre_script->GetExistenceLink.LinkNr];

		if (f->NumParams==0)
			// no arguments -> directly execute function
			func[pre_script->GetExistenceLink.LinkNr]();
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
					OCAddInstruction(GlobalOpcode,GlobalOpcodeSize,inPushM,pk[p],param[p],s-4-j*4);
				dp += s;
			}

			// the actual call
			OCAddInstruction(GlobalOpcode,GlobalOpcodeSize,inCallRel32,KindConstant,(char*)((long)func[pre_script->GetExistenceLink.LinkNr]-(long)&GlobalOpcode[GlobalOpcodeSize]-5));
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
	//msg_db_r(string("Execute ",pre_script->Filename),1);
	msg_db_r("Execute", 1);
	msg_db_r(pre_script->Filename,1);

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
	msg_db_l(1);
}
