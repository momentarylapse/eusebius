/*----------------------------------------------------------------------------*\
| Script Data                                                                  |
| -> "standart library" for the scripting system                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2007.03.19 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(SCRIPT_DATA_H__INCLUDED_)
#define SCRIPT_DATA_H__INCLUDED_



#define SCRIPT_MAX_FILE_SIZE		2097152
#define SCRIPT_MAX_EXPRESSIONS		65536
#define SCRIPT_MAX_INCLUDES			64
#define SCRIPT_MAX_DEFINES			1024
#define SCRIPT_MAX_DEFINE_DESTS		64
#define SCRIPT_MAX_RULES			32
#define SCRIPT_MAX_TYPES			256		// number of possible types
#define SCRIPT_MAX_STRUCTS			128
#define SCRIPT_MAX_STRUCT_ELEMENTS	32
#define SCRIPT_MAX_NAME				32		// variables' name length (+1)
#define SCRIPT_MAX_VARS				128		// number of variables per function (global=separate function)
#define SCRIPT_MAX_CONSTANTS		2048	// number of constants per file
#define SCRIPT_MAX_FUNCS			256		// own functions
#define SCRIPT_MAX_COMMAND_BLOCKS	256		// obsolete....
#define SCRIPT_MAX_BLOCK_COMMANDS	256		// commands per block (not including subcommands)
#define SCRIPT_MAX_PARAMS			16		// number of possible parameters per function/command
#define SCRIPT_MAX_COMMANDS			4096	// complete number of commands per file (including subcommands)
#define SCRIPT_MAX_OPCODE			(2*65536)	// max. amount of opcode
#define SCRIPT_STACK_SIZE			8192	// max. amount of stack memory
#define SCRIPT_MAX_ASMS				256		// number of asm{...} blocks

#define PointerSize (sizeof(char*))

extern char ScriptDataVersion[128];


//--------------------------------------------------------------------------------------------------
// types

struct sType{
	char Name[SCRIPT_MAX_NAME];
	unsigned int Size; // complete size of type
	unsigned int ArrayLength;
	bool IsPointer;
	sType *SubType;
};
extern int NumPreTypes;
extern sType TypeUnknown;
extern sType TypeVoid;
extern sType TypePointer;
extern sType TypeStruct;
extern sType TypeBool;
extern sType TypeInt;
extern sType TypeFloat;
extern sType TypeString;

extern sType TypeVector;
extern sType TypeRect;
extern sType TypeColor;
extern sType TypeQuaternion;
extern sType *PreType[SCRIPT_MAX_TYPES];




//--------------------------------------------------------------------------------------------------
// operators

struct sPrimitiveOperator{
	char Name[4];
	unsigned int ID;
	// order of operators ("Punkt vor Strich")
	unsigned char Level; // 1=Zuweisung, 2=bool'sches, 3="Strich", 4="Punkt"
};
extern int NumPrimitiveOperators;
extern sPrimitiveOperator PrimitiveOperator[];
struct sPreOperator{
	unsigned int ID,PrimitiveID;
	sType *ReturnType,*ParamType1,*ParamType2;
};
extern int NumPreOperators;
extern sPreOperator PreOperator[];


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
	OperatorVectorNegate,
	NUM_PRE_OPERATORS
};



//--------------------------------------------------------------------------------------------------
// structures

struct sStructElement{
	char *Name;
	sType *Type;
	unsigned int Shift;
};
struct sStruct{
	sType *RootType;
	int NumElements;
	sStructElement Element[SCRIPT_MAX_STRUCT_ELEMENTS];
};
extern int NumPreStructs;
extern sStruct PreStruct[];




//--------------------------------------------------------------------------------------------------
// constants

struct sPreConstant{
	char *Name;
	sType *Type;
	char *Value;
};
extern int NumPreConstants;
extern sPreConstant PreConstant[];





//--------------------------------------------------------------------------------------------------
// pre defined global variables
struct sPreGlobalVar{
	char Name[SCRIPT_MAX_NAME];
	sType *type;
};
extern int NumPreGlobalVars;
extern sPreGlobalVar PreGlobalVar[];




//--------------------------------------------------------------------------------------------------
// external variables (in the surrounding program...)

struct sPreExternalVar{
	char *Name;
	unsigned int Nr;
	sType *Type;
	char *Pointer;
};
extern int NumPreExternalVars;
extern sPreExternalVar PreExternalVar[];




//--------------------------------------------------------------------------------------------------
// commands

struct sPCParam{
	sType *Type;
	char *Name;
};
struct sPreCommand{
	char *Name;
	unsigned int Nr;
	char *Instance,*Func;
	sType *ReturnType;
	unsigned char NumParams;
	sPCParam Param[SCRIPT_MAX_PARAMS];
};
extern int NumPreCommands;
extern sPreCommand PreCommand[];


extern char *f_cp;

enum{
	// structural commands
	CommandReturn,
	CommandIf,
	CommandIfElse,
	CommandWhile,
	CommandFor,
	CommandWait,
	CommandWaitRT,
	CommandWaitOneFrame,
	CommandIntFromFloat,
	CommandFloatFromInt,
	CommandCharFromInt,
	CommandIntFromChar,
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
	sType *Source,*Dest;
	int Command;
	t_cast_func *Func;
};
extern int NumTypeCasts;
extern sTypeCast TypeCast[];


typedef void t_func();

extern void ScriptLinkExternalData();
extern void ScriptLinkDynamicExternalData();




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
	NumScriptLocations
};

extern sScriptLocation ScriptLocation[NumScriptLocations];


#endif
