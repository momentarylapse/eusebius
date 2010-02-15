
// character buffer and expressions (syntax analysis)

struct ps_exp_t
{
	char* name; // points into Exp.buffer
	int pos;
};

struct ps_line_t
{
	int physical_line, length, indent;
	std::vector<ps_exp_t> exp;
};

struct ps_exp_buffer_t
{
	char *buffer; // holds ALL expressions of the file (0 separated)
	char *buf_cur; // pointer to the latest one
	std::vector<ps_line_t> line;
	ps_line_t temp_line;
	ps_line_t *cur_line;
	int cur_exp;
	int comment_level;
};

// macros
struct sDefine
{
	char Source[SCRIPT_MAX_NAME];
	char Dest[SCRIPT_MAX_DEFINE_DESTS][SCRIPT_MAX_NAME];
	int NumDests;
};

// special macro for execution rules
struct sPreScriptRule
{
	int Location, Level;
	char Name[SCRIPT_MAX_NAME];
};

// single enum entries
struct sEnum
{
	char Name[SCRIPT_MAX_NAME];
	int Value;
};

// for any type of constant used in the script
struct sConstant
{
	char *data;
	sType *type;
};

// links commands and variables
struct sLinkData
{
	int Kind;
	int Nr; // parameter for the link type (also shift for pointer)
	sType *type;
	sLinkData *Meta,*ParamLink;
	CScript *script;
};

enum
{
	KindUnknown,
	KindVarLocal,
	KindVarGlobal,
	KindVarFunction,
	KindVarExternal,		// = variable from surrounding program
	KindVarTemp,
	KindEnum,				// = single enum entry
	KindConstant,
	KindFunction,			// = user defined functions
	KindCompilerFunction,	// = compiler functions
//	KindClassFunction,		// = compiler functions from a class
	KindCommand,			// = script commands (indexed)
	KindBlock,				// = block of commands {...}
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
	KindRefToConst,
	KindType
};

// type of expression (syntax)
enum
{
	ExpKindNumber,
	ExpKindLetter,
	ExpKindSpacing,
	ExpKindSign
};


// {...}-block
struct sBlock
{
	int RootNr;
	int Nr;
	std::vector<int> Command; // ID of command in global command array
};

struct sLocalVariable
{
	sType *Type; // for creating instances
	char Name[SCRIPT_MAX_NAME];
	int Offset;
};

// user defined functions
struct sFunction
{
	char Name[SCRIPT_MAX_NAME];
	// parameters (linked to intern variables)
	int NumParams;
	// block of code
	sBlock *Block;
	// local variables
	std::vector<sLocalVariable> Var;
	int VarSize, ParamSize;
	// return value
	sType *Type;
};


// single command
struct sCommand
{
	int Kind;
	int LinkNr;
	CScript *script;
	// parameters
	int NumParams;
	sLinkData ParamLink[SCRIPT_MAX_PARAMS];
	// linking of [if]s, [while]s and class function instances
	int SubLink1, SubLink2, SubLinkEnd;
	// return value
	sType *ReturnType;
};

struct sAsmBlock
{
	char *block;
	int Line;
};


// data structures (uncompiled)
class CPreScript
{
public:
	CPreScript(char *filename, bool just_analyse = false);
	CPreScript();
	~CPreScript();
	bool LoadToBuffer(char *filename, bool just_analyse);
	void AddIncludeData(CScript *s);

	bool Error, IncludeLinkerError;
	char ErrorMsg[256],ErrorMsgExt[2][256];
	int ErrorLine,ErrorColumn;
	void DoError(char *msg);
	bool ExpectNoNewline();
	bool ExpectNewline();
	bool ExpectIndent();

	// lexical analysis
	int GetKind(char c);
	void Analyse(char *buffer, bool just_analyse);
	bool AnalyseExpression(char *buffer, int &pos, ps_line_t *l, int &line_no, bool just_analyse);
	bool AnalyseLine(char *buffer, ps_line_t *l, int &line_no, bool just_analyse);
	void AnalyseLogicalLine(char *buffer, ps_line_t *l, int &line_no, bool just_analyse);
	
	// syntax analysis
	void Parser();
	void ParseEnum();
	void ParseStruct();
	void ParseFunction();
	sType *ParseVariableDefSingle(sType *type, sFunction *f, bool as_param = false);
	void ParseVariableDef(bool single, sFunction *f);
	int WhichPrimitiveOperator(char *name);
	int WhichCompilerFunction(char *name);
	void SetCompilerFunction(int CF,sCommand *Com);
	int WhichExternalVariable(char *name);
	int WhichType(char *name);
	void SetExternalVariable(int gv,sLinkData &link);
	void AddType();

	// pre compiler
	void PreCompiler(bool just_analyse);
	void HandleMacro(ps_line_t *l, int &line_no, int &NumIfDefs, bool *IfDefed, bool just_analyse);

	// syntax analysis
	sType *GetConstantType(char *name);
	void *GetConstantValue(char *name);
	sType *GetType(int &ie, bool force);
	void AddType(sType **type);
	sType *CreateNewType(char *name, int size, bool is_pointer, bool is_silent, int array_size, sType *sub);
	void TestArrayDefinition(sType **type, bool is_pointer);
	bool GetExistence(char *name, sFunction *f);
	void LinkMostImportantOperator(int &NumOperators, sLinkData *Operand, sLinkData *Operator, int *op_exp);
	void GetOperandExtension(sLinkData *Operand, sFunction *f);
	sLinkData GetCommand(sFunction *f);
	void GetCompleteCommand(sBlock *block, sFunction *f);
	sLinkData GetOperand(sFunction *f);
	bool GetOperator(sFunction *f);
	void FindFunctionParameters(int &np, sType **WantedType, sFunction *f, int cmd, int fnc);
	void FindFunctionSingleParameter(int p, sType **WantedType, sFunction *f, int cmd, int fnc);
	void GetFunctionCall(char *f_name, sLinkData *Operand, sLinkData *link, sFunction *f);
	bool GetSpecialFunctionCall(char *f_name, sLinkData *Operand, sLinkData *link, sFunction *f);
	void CheckParamLink(sLinkData *link, sType *type, char *f_name = "", int param_no = -1);
	void GetSpecialCommand(sBlock *block, sFunction *f);

	// data creation
	int AddVar(char *name,sType *type,sFunction *f);
	int AddConstant(sType *type);
	int AddBlock();
	int AddFunction(char *name);
	int AddCommand();

	// debug displaying
	void ShowLink(sLinkData *link);
	void ShowCommand(int c);
	void ShowFunction(int f);
	void ShowBlock(int b);
	void Show();

// data

	char Filename[256];
	char *Buffer;
	int BufferLength, BufferPos;
	ps_exp_buffer_t Exp;
	sLinkData GetExistenceLink;

	// compiler options
	bool FlagShow;
	bool FlagDisassemble;
	bool FlagImmortal;
	bool FlagCompileOS;
	bool FlagCompileInitialRealMode;
	bool FlagOverwriteVariablesOffset;
	int VariablesOffset;

	std::vector<sType*> Type;
	std::vector<sStruct> Struct;
	std::vector<sEnum> Enum;
	std::vector<CScript*> Include;
	std::vector<sDefine> Define;
	std::vector<sPreScriptRule> PreScriptRule;
	char *AsmMetaInfo;
	std::vector<sAsmBlock> AsmBlock;
	std::vector<sConstant> Constant;
	std::vector<sBlock> Block;
	std::vector<sFunction> Function;
	std::vector<sCommand> Command;
	std::vector<sLinkData> LinkData;

	sFunction RootOfAllEvil;
};

#define _do_error_(str,n,r)	{DoError(str);msg_db_l(n);return r;}
#define _return_(n,r)		{msg_db_l(n);return r;}

char *Kind2Str(int kind);
char *Operator2Str(CPreScript *s,int cmd);
void clear_exp_buffer(ps_exp_buffer_t *e);
void CreateAsmMetaInfo(CPreScript* ps);
extern CScript *cur_script;
