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
	CScript(char *filename,bool just_analyse=false);
	CScript();
	~CScript();

	void Compiler();

	void SetVariable(char *name, void *data);

	// operational code
	char *OCAddParameter(sLinkData *link,int n_func,int level,int index,int &pk,bool allow_auto_ref=true);
	void OCAddBlock(sBlock *block,int n_func,int level);
	char *OCAddCommand(sCommand *com,int n_func,int level,int index);

	void DoError(char *msg);
	void DoErrorLink(char *msg);

	// execution
	void Execute();
	void ExecuteSingleCommand(char *cmd);
	bool ExecuteScriptFunction(char *name,...);

	//debug displaying
	void ShowVars(bool include_consts=false);

// data

	CPreScript *pre_script;

	char Filename[256];
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

	bool Error, ParserError, LinkerError, isCopy, isPrivate, JustAnalyse;
	char ErrorMsg[256], ErrorMsgExt[2][256];
	int ErrorLine, ErrorColumn;
	int WaitingMode;
	float TimeToWait;

	std::vector<char*> g_var, cnst;

	int MemoryUsed;
};

extern CScript *LoadScript(char *filename,bool is_public=true);
extern CScript *LoadScriptAsInclude(char *filename,bool just_analyse);
extern int NumPublicScripts;
extern char ScriptDirectory[512];
extern void ExecutePublicScripts();
extern void DeletePublicScripts(bool even_immortal=false);



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
