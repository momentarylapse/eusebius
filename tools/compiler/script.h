/*----------------------------------------------------------------------------*\
| CScript                                                                      |
| -> C-like scripting system                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2007.03.19 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(SCRIPT_H__INCLUDED_)
#define SCRIPT_H__INCLUDED_



extern char ScriptVersion[32];


#include "script_data.h"


class CScript;

static bool UseConstAsGlobalVar=false;


struct exp_buffer{
	char Buffer[SCRIPT_MAX_FILE_SIZE];
	char *Name[SCRIPT_MAX_EXPRESSIONS];
	int Line[SCRIPT_MAX_EXPRESSIONS],Column[SCRIPT_MAX_EXPRESSIONS];
	int NumExps,ExpNr,TempLine,TempColumn,BufferUsed;
};

struct sDefine{
	char Source[SCRIPT_MAX_NAME];
	char Dest[SCRIPT_MAX_DEFINE_DESTS][SCRIPT_MAX_NAME];
	int NumDests;
};

struct sPreScriptRule{
	int Location,Level;
	char Name[SCRIPT_MAX_NAME];
};


struct sVariable{
	char *data;
	sType *type;
	sVariable *meta;
	bool OwnData;
	int StackPos;
};

struct sLinkData{
	unsigned char Kind;
	unsigned int Nr; // Parameter fuer die Link-Art (auch Verschiebung des Pointers)
	sType *type;
	sLinkData *Meta,*ParamLink;
	CScript *script;
};

enum{
	KindUnknown,
	KindVarLocal,
	KindVarGlobal,
	KindVarFunction,
	KindVarExternal,		// = variable from surrounding program
	KindVarTemp,
	KindConstant,
	KindFunction,			// = eigene Funktionen
	KindCompilerFunction,	// = Compiler-Funktionen
	KindCommand,			// = Script-Befehl (durchnummeriert)
	KindBlock,				// = Befehls-Block {...}
	KindOperator,
	KindPrimitiveOperator,
	KindPointerShift,		// = . "struct"
	KindArray,				// = []
	KindPointerAsArray,		// = []
	KindReference,			// = &
	KindDereference,		// = *
	KindDerefPointerShift,	// = ->
	KindRefToLocal,
	KindRefToGlobal,
	KindRefToConst
};


enum{
	ExpKindNumber,
	ExpKindLetters,
	ExpKindSpacing,
	ExpKindSign
};


// {...}-Block
struct sBlock{
	short RootNr;
	unsigned short Nr;
	unsigned short NumCommands;
	unsigned short Command[SCRIPT_MAX_BLOCK_COMMANDS]; // ID des Befehls im globalen Befehl-Array
};

// selbst definierte Funktionen
struct sFunction{
	char Name[SCRIPT_MAX_NAME];
	// Parameter (interne Variablen-Verknuepfung)
	unsigned char NumParams;
	// Code-Block
	sBlock *Block;
	// lokale Variablen
	sType *VarType[SCRIPT_MAX_VARS]; // zum Erstellen der Instanzen
	char VarName[SCRIPT_MAX_VARS][SCRIPT_MAX_NAME];
	int VarOffset[SCRIPT_MAX_VARS];
	int NumVars,VarSize,ParamSize;
	// return-Wert
	sType *Type;
};


// einzelner Befehl
struct sCommand{
	unsigned char Kind;
	unsigned short LinkNr;
	CScript *script;
	// Parameter
	unsigned char NumParams;
	sLinkData ParamLink[SCRIPT_MAX_PARAMS];
	// Verlinkung von if's und while's
	unsigned short SubLink1,SubLink2,SubLinkEnd;
	// Return-Wert
	sLinkData ReturnLink;
};

enum{
	WaitingModeNone,
	WaitingModeGT,
	WaitingModeRT
};


class CPreScript
{
public:
	CPreScript(char *filename,bool just_analyse=false);
	CPreScript();
	bool LoadToBuffer(char *filename,bool just_analyse);
	void Parser();
	void PreCompiler();
	void AddIncludeData(CScript *s);

	bool Error;
	char ErrorMsg[256],ErrorMsgExt[2][256];
	int ErrorLine,ErrorColumn;
	void DoError(char *msg,int nr);

	// syntax analysis
	bool isNumber(char c);
	bool isLetter(char c);
	bool isSpacing(char c);
	bool isSign(char c);
	int GetKind(char c);
	void NextExp(char *Buffer);
	void MakeExps(char *Buffer,bool just_analyse);
	int WhichPrimitiveOperator(char *name);
	int WhichCompilerFunction(char *name);
	void SetCompilerFunction(int CF,sCommand *Com);
	int WhichExternalVariable(char *name);
	void SetExternalVariable(int gv,sLinkData &link);
	void AddType();

	// syntax analysis
	sType *GetConstantType(char *name);
	sType *GetType(int &ie,bool force);
	void AddType(sType **type);
	void TestArrayDefinition(int &ie,sType **type,bool is_pointer);
	bool GetExistence(char *name,sFunction *f);
	void LinkMostImportantOperator(int &NumOperators,sLinkData *Operand,sLinkData *Operator);
	void GetOperandExtension(int &ie,sLinkData *Operand,sFunction *f);
	sLinkData GetCommand(int &ie,int bracket_level,int shift,sFunction *f);
	void GetCompleteCommand(sBlock *block);
	sLinkData GetOperand(int &ie,int bracket_level,sFunction *f);
	bool GetOperator(int &ie,sFunction *f);

	// data creation
	int AddVar(char *name,sType *type,sFunction *f);
	int AddConstant(sType *type);
	int AddBlock();
	int AddFunction(char *name);
	void AddCommand();

	// debug displaying
	void ShowLink(sLinkData *link);
	void ShowCommand(int c);
	void ShowFunction(int f);
	void ShowBlock(int b);
	void Show();

// data

	char Filename[256];
	char *Buffer;
	int BufferLength,BufferPos;
	exp_buffer *Exp;

	// compiler options
	bool FlagShow,FlagDisassemble,FlagCompileOS,FlagCompileInitialRealMode,FlagOverwriteVariablesOffset;
	int VariablesOffset;

	int NumTypes;
	sType *Type[SCRIPT_MAX_TYPES];

	int NumStructs;
	sStruct Struct[SCRIPT_MAX_STRUCTS];

	int NumIncludes;
	CScript *Include[SCRIPT_MAX_INCLUDES];

	int NumDefines;
	sDefine *Define[SCRIPT_MAX_DEFINES];
	bool OwnDefine[SCRIPT_MAX_DEFINES];
	int NumPreScriptRules;
	sPreScriptRule PreScriptRule[SCRIPT_MAX_RULES];

	void *AsmMetaInfo;
	int NumAsms;
	char *AsmBlock[SCRIPT_MAX_ASMS];
	int AsmLine[SCRIPT_MAX_ASMS];

	sLinkData GetExistenceLink;

	int NumConstants;
	sVariable *Constant[SCRIPT_MAX_CONSTANTS];

	int NumBlocks;
	sBlock *Block[SCRIPT_MAX_COMMAND_BLOCKS];

	sFunction RootOfAllEvil;
	sFunction *Function[SCRIPT_MAX_FUNCS];
	int NumFunctions;

	sCommand *Command[SCRIPT_MAX_COMMANDS];
	int NumCommands;
};

// executable (compiled) data
class CScript
{
public:
	CScript(char *filename,bool just_analyse=false);
	CScript();

	void Compiler();

	// operational code
	void OCAddChar(int c);
	void OCAddWord(int i);
	void OCAddInt(int i);
	void OCAddBlock(sBlock *block,int n_func,int level);
	char *OCAddCommand(sCommand *com,int n_func,int level,int index);

	void DoError(char *msg,int nr);
	void DoErrorLink(char *msg,int nr);

	// execution
	void Execute();
	void ExecuteSingleCommand(char *cmd);
	bool ExecuteScriptFunction(char *name,...);

	//debug displaying
	void ShowVars(bool include_consts=false);

// data

	CPreScript *pre_script;

	char Filename[256];
	int ThisObject;

	char Opcode[SCRIPT_MAX_OPCODE]; // executable code
	int OpcodeSize;
	char *Memory; // memory for global variables, constants etc
	int MemorySize;
	char Stack[SCRIPT_STACK_SIZE]; // stack for local variables etc

	t_func *func[SCRIPT_MAX_FUNCS];
	t_func *FirstExecution,*ContinueExecution;

	bool Error,ParserError,LinkerError,isCopy,isPrivate,JustAnalyse;
	char ErrorMsg[256],ErrorMsgExt[2][256];
	int ErrorLine,ErrorColumn;
	int WaitingMode;
	float TimeToWait;

	char *g_var[SCRIPT_MAX_VARS],*cnst[SCRIPT_MAX_CONSTANTS];

	int MemoryUsed;
};

extern CScript *LoadScript(char *filename,bool is_public=true);
extern CScript *LoadScriptAsInclude(char *filename,bool just_analyse);
extern int NumPublicScripts;
extern char ScriptDirectory[512];


#endif
