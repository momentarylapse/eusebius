/*----------------------------------------------------------------------------*\
| CScript                                                                      |
| -> C-like scripting system                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2009.10.04 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(SCRIPT_H__INCLUDED_)
#define SCRIPT_H__INCLUDED_



extern char ScriptVersion[32];

class CScript;

#include <list>
#include <vector>
#include "../nix/nix.h"
#include "dasm.h"
#include "script_data.h"
#include "pre_script.h"


static bool UseConstAsGlobalVar=false;


enum
{
	WaitingModeNone,
	WaitingModeFirst,
	WaitingModeGT,
	WaitingModeRT
};
#define WaitingModeFinished		WaitingModeNone


// executable (compiled) data
class CScript
{
public:
	// don't call yourself.... better use LoadScript(...)
	CScript(const char *filename, bool just_analyse = false);
	CScript();
	~CScript();

	void Compiler();

	void SetVariable(const char *name, void *data);

	// operational code
	char *OCAddParameter(sCommand *link,int n_func,int level,int index,int &pk,bool allow_auto_ref=true);
	void OCAddBlock(sBlock *block,int n_func,int level);
	char *OCAddCommand(sCommand *com,int n_func,int level,int index);

	void DoError(const char *msg, int overwrite_line = -1);
	void DoErrorLink(const char *msg);

	// execution
	void Execute();
	bool ExecuteScriptFunction(const char *name,...);

	//debug displaying
	void ShowVars(bool include_consts=false);

// data

	CPreScript *pre_script;

	int ReferenceCounter;
	void *user_data; // to associate additional data with the script

	char *Opcode; // executable code
	int OpcodeSize;
	char *ThreadOpcode; // executable code
	int ThreadOpcodeSize;
	char *Memory; // memory for global variables, constants etc
	int MemorySize;
	char *Stack; // stack for local variables etc

	std::vector<t_func*> func;
	t_func *first_execution, *continue_execution;

	bool Error, ParserError, LinkerError, isCopy, isPrivate, JustAnalyse, ShowCompilerStats;
	char ErrorMsg[256], ErrorMsgExt[2][256];
	int ErrorLine, ErrorColumn;
	int WaitingMode;
	float TimeToWait;

	std::vector<char*> g_var, cnst;

	int MemoryUsed;
};

extern CScript *LoadScript(const char *filename, bool is_public = true, bool just_analyse = false);
extern void RemoveScript(CScript *s);
extern char ScriptDirectory[512];
extern void ExecutePublicScripts();
extern void DeleteAllScripts(bool even_immortal = false, bool force = false);
extern void ExecuteSingleScriptCommand(const char *cmd);



// memory debugging!
#if 0
	int mem_used=0;
	int num_ps=0;
	void *ppp[102400];
	int pps[102400];
	char ppn[102400][32];
	void am(char *str,int size,void *p)
	{
		msg_write(string2("----------  add   (%p) %s  (+ %d)",p,str,size));
		mem_used+=size;
		ppp[num_ps]=p;
		strcpy(ppn[num_ps],str);
		pps[num_ps++]=size;
	}
	void dm(char *str,void *p)
	{
		//return;
		int size=-1;
		for (int i=0;i<num_ps;i++)
			if (p==ppp[i]){
				size=pps[i];
				for (int j=i;j<num_ps;j++){
					ppp[j]=ppp[j+1];
					strcpy(ppn[j],ppn[j+1]);
					pps[j]=pps[j+1];
				}
				num_ps--;
				break;
			}
		if (size>=0){
			//msg_write(string2("----------  dec   %s  (- %d)",str,size));
			mem_used-=size;
		}else
			msg_error(string2("mem_used kaputt  %s  %p",str,p));
	}
	void om()
	{
		msg_write("--------------- Memory Leakage -------------");
		msg_write(mem_used);
		for (int i=0;i<num_ps;i++)
			msg_write(string2("  fehlt:   %s  %p  (%d)",ppn[i],ppp[i],pps[i]));
	}
#else
	#define am(a,b,c)
	#define dm(a,b)
	#define om()
#endif


#endif
