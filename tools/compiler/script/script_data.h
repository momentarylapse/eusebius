/*----------------------------------------------------------------------------*\
| Script Data                                                                  |
| -> "standart library" for the scripting system                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.10.26 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(SCRIPT_DATA_H__INCLUDED_)
#define SCRIPT_DATA_H__INCLUDED_



#define SCRIPT_MAX_FILE_SIZE			2097152
#define SCRIPT_MAX_LINES				16384
#define SCRIPT_MAX_LINE_SIZE			1024
#define SCRIPT_MAX_EXPRESSIONS			65536
#define SCRIPT_MAX_EXPRESSIONS_PER_LINE	1024
#define SCRIPT_MAX_DEFINE_DESTS			64
#define SCRIPT_MAX_DEFINE_RECURSIONS	128
#define SCRIPT_MAX_NAME					42		// variables' name length (+1)
#define SCRIPT_MAX_PARAMS				16		// number of possible parameters per function/command
#define SCRIPT_MAX_OPCODE				(2*65536)	// max. amount of opcode
#define SCRIPT_MAX_THREAD_OPCODE		1024
#define SCRIPT_DEFAULT_STACK_SIZE		32768	// max. amount of stack memory
#define SCRIPT_MAX_ASMS					256		// number of asm{...} blocks
extern int ScriptStackSize;

#define PointerSize (sizeof(char*))


//#define mem_align(x)	((x) + (4 - (x) % 4) % 4)
#define mem_align(x)	((((x) + 3) / 4 ) * 4)

extern char ScriptDataVersion[];


//--------------------------------------------------------------------------------------------------
// types

struct sType{
	char Name[SCRIPT_MAX_NAME];
	int Size; // complete size of type
	int ArrayLength;
	bool IsPointer, IsSilent; // pointer silent (&)
	sType *SubType;
	void *Owner;
};
extern std::vector<sType*> PreType;
extern sType *TypeUnknown;
extern sType *TypeVoid;
extern sType *TypePointer;
extern sType *TypeStruct;
extern sType *TypeBool;
extern sType *TypeInt;
extern sType *TypeFloat;
extern sType *TypeChar;
extern sType *TypeString;

extern sType *TypeVector;
extern sType *TypeRect;
extern sType *TypeColor;
extern sType *TypeQuaternion;




//--------------------------------------------------------------------------------------------------
// operators

struct sPrimitiveOperator{
	char Name[4];
	int ID;
	bool LeftModifiable;
	unsigned char Level; // order of operators ("Punkt vor Strich")
};
extern int NumPrimitiveOperators;
extern sPrimitiveOperator PrimitiveOperator[];
struct sPreOperator{
	int PrimitiveID;
	sType *ReturnType, *ParamType1, *ParamType2;
};
extern std::vector<sPreOperator> PreOperator;



enum{
	OperatorPointerAssign,
	OperatorPointerEqual,
	OperatorPointerNotEqual,
	OperatorCharAssign,
	OperatorCharEqual,
	OperatorCharNotEqual,
	OperatorCharAdd,
	OperatorCharSubtract,
	OperatorCharAddS,
	OperatorCharSubtractS,
	OperatorCharBitAnd,
	OperatorCharBitOr,
	OperatorCharNegate,
	OperatorBoolAssign,
	OperatorBoolEqual,
	OperatorBoolNotEqual,
	OperatorBoolGreater,
	OperatorBoolGreaterEqual,
	OperatorBoolSmaller,
	OperatorBoolSmallerEqual,
	OperatorBoolAnd,
	OperatorBoolOr,
	OperatorBoolNegate,
	OperatorIntAssign,
	OperatorIntAdd,
	OperatorIntSubtract,
	OperatorIntMultiply,
	OperatorIntDivide,
	OperatorIntAddS,
	OperatorIntSubtractS,
	OperatorIntMultiplyS,
	OperatorIntDivideS,
	OperatorIntModulo,
	OperatorIntEqual,
	OperatorIntNotEqual,
	OperatorIntGreater,
	OperatorIntGreaterEqual,
	OperatorIntSmaller,
	OperatorIntSmallerEqual,
	OperatorIntBitAnd,
	OperatorIntBitOr,
	OperatorIntShiftRight,
	OperatorIntShiftLeft,
	OperatorIntNegate,
	OperatorIntIncrease,
	OperatorIntDecrease,
	OperatorFloatAssign,
	OperatorFloatAdd,
	OperatorFloatSubtract,
	OperatorFloatMultiply,
	OperatorFloatMultiplyFI,
	OperatorFloatMultiplyIF,
	OperatorFloatDivide,
	OperatorFloatAddS,
	OperatorFloatSubtractS,
	OperatorFloatMultiplyS,
	OperatorFloatDivideS,
	OperatorFloatEqual,
	OperatorFloatNotEqual,
	OperatorFloatGreater,
	OperatorFloatGreaterEqual,
	OperatorFloatSmaller,
	OperatorFloatSmallerEqual,
	OperatorFloatNegate,
	OperatorStringAssignAA,
	OperatorStringAssignAP,
	OperatorStringAddAA,
	OperatorStringAddAP,
	OperatorStringAddPA,
	OperatorStringAddPP,
	OperatorStringAddAAS,
	OperatorStringAddAPS,
	OperatorStringEqualAA,
	OperatorStringEqualPA,
	OperatorStringEqualAP,
	OperatorStringEqualPP,
	OperatorStringNotEqualAA,
	OperatorStringNotEqualPA,
	OperatorStringNotEqualAP,
	OperatorStringNotEqualPP,
	OperatorStructAssign,
	OperatorStructEqual,
	OperatorStructNotEqual,
	OperatorVectorAdd,
	OperatorVectorSubtract,
	OperatorVectorMultiplyVV,
	OperatorVectorMultiplyVF,
	OperatorVectorMultiplyFV,
	OperatorVectorDivide,
	OperatorVectorDivideVF,
	OperatorVectorAddS,
	OperatorVectorSubtractS,
	OperatorVectorMultiplyS,
	OperatorVectorDivideS,
	OperatorVectorNegate
};



//--------------------------------------------------------------------------------------------------
// structures

struct sStructElement{
	char Name[SCRIPT_MAX_NAME];
	sType *Type;
	int Offset;
};
struct sStructFunction{
	char Name[SCRIPT_MAX_NAME];
	int cmd; // PreCommand index
	// _func_(x)  ->  p.func(x)
};
struct sStruct{
	sType *RootType;
	std::vector<sStructElement> Element;
	std::vector<sStructFunction> Function;
	void *Owner;
};
extern std::vector<sStruct> PreStruct;




//--------------------------------------------------------------------------------------------------
// constants

struct sPreConstant{
	char *Name;
	sType *Type;
	void *Value;
};
extern std::vector<sPreConstant> PreConstant;





//--------------------------------------------------------------------------------------------------
// pre defined global variables
struct sPreGlobalVar{
	char *Name;
	sType *Type;
};
extern std::vector<sPreGlobalVar> PreGlobalVar;




//--------------------------------------------------------------------------------------------------
// external variables (in the surrounding program...)

struct sPreExternalVar{
	char *Name;
	sType *Type;
	void *Pointer;
};
extern std::vector<sPreExternalVar> PreExternalVar;
extern int NumTruePreExternalVars;

//--------------------------------------------------------------------------------------------------
// semi external variables (in the surrounding program...but has to be defined "extern")



//--------------------------------------------------------------------------------------------------
// commands

struct sPreCommandParam{
	char *Name;
	//char Name[SCRIPT_MAX_NAME];
	sType *Type;
};
struct sPreCommand{
	char *Name;
	//char Name[SCRIPT_MAX_NAME];
	void *Instance, *Func;
	sType *ReturnType;
	std::vector<sPreCommandParam> Param;
};
extern std::vector<sPreCommand> PreCommand;


extern void *f_cp;
extern void *f_class;

enum{
	// structural commands
	CommandReturn,
	CommandIf,
	CommandIfElse,
	CommandWhile,
	CommandFor,
	CommandBreak,
	CommandContinue,
	CommandSizeof,
	CommandWait,
	CommandWaitRT,
	CommandWaitOneFrame,
	CommandFloatToInt,
	CommandIntToFloat,
	CommandIntToChar,
	CommandCharToInt,
	CommandPointerToBool,
	CommandVectorSet,
	CommandRectSet,
	CommandColorSet,
	CommandAsm,
	NUM_INTERN_PRE_COMMANDS
};





//--------------------------------------------------------------------------------------------------
// type casting

typedef void *t_cast_func(void*);
struct sTypeCast{
	int Penalty;
	sType *Source,*Dest;
	int Command;
	t_cast_func *Func;
};
extern std::vector<sTypeCast> TypeCast;


typedef void t_func();

extern void ScriptInit();
extern void ScriptResetSemiExternalVars();
extern void ScriptLinkSemiExternalVar(char *name, void *pointer);
extern void ScriptAddPreGlobalVar(char *name, sType *type);
extern sType *ScriptGetPreType(char *name);




//--------------------------------------------------------------------------------------------------
// other stuff

struct sScriptLocation{
	char Name[SCRIPT_MAX_NAME];
	int Location;
};

enum{
	ScriptLocationCalcMovePrae,
	ScriptLocationCalcMovePost,
	ScriptLocationRenderPrae,
	ScriptLocationRenderPost1,
	ScriptLocationRenderPost2,
	ScriptLocationGetInputPrae,
	ScriptLocationGetInputPost,
	ScriptLocationNetworkSend,
	ScriptLocationNetworkRecieve,
	ScriptLocationNetworkAddClient,
	ScriptLocationNetworkRemoveClient,
	ScriptLocationOnKillObject,
	ScriptLocationOnCollision,
	/*ScriptLocationOnKeyDown,
	ScriptLocationOnKeyUp,
	ScriptLocationOnKey,*/
	NumScriptLocations
};

extern sScriptLocation ScriptLocation[NumScriptLocations];


#endif
