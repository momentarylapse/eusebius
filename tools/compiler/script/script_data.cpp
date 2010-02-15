/*----------------------------------------------------------------------------*\
| Script Data                                                                  |
| -> "standart library" for the scripting system                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2010.02.08 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "script.h"

#include <string.h>

//#include "../00_config.h"
#ifdef _X_ALLOW_META_
	#include "../x/x.h"
#endif

#ifdef _X_ALLOW_GOD_
	extern void SetMessage(char *msg);
#endif

char ScriptDataVersion[]="0.5.2.0";

int ScriptStackSize = SCRIPT_DEFAULT_STACK_SIZE;



//------------------------------------------------------------------------------------------------//
//                                             types                                              //
//------------------------------------------------------------------------------------------------//

sType *TypeUnknown;
sType *TypeVoid;
sType *TypePointer;
sType *TypeStruct;
sType *TypeBool;
sType *TypeInt;
sType *TypeFloat;
sType *TypeChar;
sType *TypeString;

sType *TypeVector;
sType *TypeRect;
sType *TypeColor;
sType *TypeQuaternion;


std::vector<sType*> PreType;
sType *add_type(char *name, int size, int array_length = 0, bool is_pointer = false, bool is_silent = false, sType *sub_type = NULL)
{
	sType *t = new sType;
	strcpy(t->Name, name);
	t->Size = size;
	t->ArrayLength = array_length;
	t->IsPointer = is_pointer;
	t->IsSilent = is_silent;
	t->SubType = sub_type;
	PreType.push_back(t);
	return t;
}

sType *ScriptGetPreType(char *name)
{
	for (int i=0;i<PreType.size();i++)
		if (strcmp(name, PreType[i]->Name) == 0)
			return PreType[i];
	return TypeUnknown;
}

//------------------------------------------------------------------------------------------------//
//                                           operators                                            //
//------------------------------------------------------------------------------------------------//

//   without type information ("primitive")
enum{
	OperatorAssign,			//  =
	OperatorAdd,			//  +
	OperatorSubtract,		//  -
	OperatorMultiply,		//  *
	OperatorDivide,			//  /
	OperatorAddS,			// +=
	OperatorSubtractS,		// -=
	OperatorMultiplyS,		// *=
	OperatorDivideS,		// /=
	OperatorEqual,			// ==
	OperatorNotEqual,		// !=
	OperatorNegate,			//  !
	OperatorSmaller,		//  <
	OperatorGreater,		//  >
	OperatorSmallerEqual,	// <=
	OperatorGreaterEqual,	// >=
/*	OperatorAnd,			// &&
	OperatorOr,				// ||  */
	OperatorAndLiteral,		// and
	OperatorOrLiteral,		// or
	OperatorModulo,			//  %
	OperatorBitAnd,			//  &
	OperatorBitOr,			//  |
	OperatorShiftLeft,		// <<
	OperatorShiftRight,		// >>
	OperatorIncrease,		// ++
	OperatorDecrease,		// --
	NUM_PRIMITIVE_OPERATORS
};
int NumPrimitiveOperators = NUM_PRIMITIVE_OPERATORS;

sPrimitiveOperator PrimitiveOperator[NUM_PRIMITIVE_OPERATORS]={
	{"=",	OperatorAssign,			true,	1},
	{"+",	OperatorAdd,			false,	11},
	{"-",	OperatorSubtract,		false,	11},
	{"*",	OperatorMultiply,		false,	12},
	{"/",	OperatorDivide,			false,	12},
	{"+=",	OperatorAddS,			true,	1},
	{"-=",	OperatorSubtractS,		true,	1},
	{"*=",	OperatorMultiplyS,		true,	1},
	{"/=",	OperatorDivideS,		true,	1},
	{"==",	OperatorEqual,			false,	8},
	{"!=",	OperatorNotEqual,		false,	8},
	{"!",	OperatorNegate,			false,	2},
	{"<",	OperatorSmaller,		false,	9},
	{">",	OperatorGreater,		false,	9},
	{"<=",	OperatorSmallerEqual,	false,	9},
	{">=",	OperatorGreaterEqual,	false,	9},
/*	{"-&&",	OperatorAnd,			false,	4},
	{"-||",	OperatorOr,				false,	3},*/
	{"and",	OperatorAndLiteral,		false,	4},
	{"or",	OperatorOrLiteral,		false,	3},
	{"%",	OperatorModulo,			false,	12},
	{"&",	OperatorBitAnd,			false,	7},
	{"|",	OperatorBitOr,			false,	5},
	{"<<",	OperatorShiftLeft,		false,	10},
	{">>",	OperatorShiftRight,		false,	10},
	{"++",	OperatorIncrease,		true,	2},
	{"--",	OperatorDecrease,		true,	2}
// Level = 15 - (official C-operator priority)
// priority from "C als erste Programmiersprache", page 552
};

//   with type information

std::vector<sPreOperator> PreOperator;
int add_operator(int primitive_op, sType *return_type, sType *param_type1, sType *param_type2)
{
	sPreOperator o;
	o.PrimitiveID = primitive_op;
	o.ReturnType = return_type;
	o.ParamType1 = param_type1;
	o.ParamType2 = param_type2;
	PreOperator.push_back(o);
	return PreOperator.size() - 1;
}


//------------------------------------------------------------------------------------------------//
//                                     structures & elements                                      //
//------------------------------------------------------------------------------------------------//

#ifdef _X_ALLOW_GUI_
	static sText *_text;
	#define	GetDAText(x)		long(&_text->x)-long(_text)
	static sPicture *_picture;
	#define	GetDAPicture(x)		long(&_picture->x)-long(_picture)
	static sPicture3D *_picture3d;
	#define	GetDAPicture3D(x)	long(&_picture3d->x)-long(_picture3d)
	static sGrouping *_grouping;
	#define	GetDAGrouping(x)	long(&_grouping->x)-long(_grouping)
#else
	#define	GetDAText(x)		0
	#define	GetDAPicture(x)		0
	#define	GetDAPicture3D(x)	0
	#define	GetDAGrouping(x)	0
#endif
#ifdef _X_ALLOW_FX_
	static sParticle *_particle;
	#define	GetDAParticle(x)	long(&_particle->x)-long(_particle)
	static sEffect *_effect;
	#define	GetDAEffect(x)		long(&_effect->x)-long(_effect)
#else
	#define	GetDAParticle(x)	0
	#define	GetDABeam(x)		0
	#define	GetDAEffect(x)		0
#endif
#ifdef _X_ALLOW_MODEL_
	static sItem *_item;
	#define	GetDAItem(x)		long(&_item->x)-long(_item)
	static CModel *_model;
	#define	GetDAModel(x)		long(&_model->x)-long(_model)
	static sSkin *_skin;
	#define	GetDASkin(x)		long(&_skin->x)-long(_skin)
	static sMaterial *_material;
	#define	GetDAMaterial(x)	long(&_material->x)-long(_material)
#else
	#define	GetDAItem(x)		0
	#define	GetDAModel(x)		0
	#define	GetDASkin(x)		0
	#define	GetDAMaterial(x)	0
#endif
#ifdef _X_ALLOW_GOD_
	static sFog *_fog;
	#define	GetDAFog(x)			long(&_fog->x)-long(_fog)
#else
	#define	GetDAFog(x)			0
#endif
#ifdef _X_ALLOW_OBJECT_
	static CObject *_object;
	#define	GetDAObject(x)		long(&_object->x)-long(_object)
#else
	#define	GetDAObject(x)		0
#endif
#ifdef _X_ALLOW_CAMERA_
	static sView *_view;
	#define	GetDAView(x)		long(&_view->x)-long(_view)
#else
	#define	GetDAView(x)		0
#endif
#ifdef _X_ALLOW_TERRAIN_
	static CTerrain *_terrain;
	#define	GetDATerrain(x)		long(&_terrain->x)-long(_terrain)
#else
	#define	GetDATerrain(x)		0
#endif
static sDate *_date;
#define	GetDADate(x)			long(&_date->x)-long(_date)
static CHuiWindow *_win;
#define GetDAWindow(x)			long(&_win->x)-long(_win)


std::vector<sStruct> PreStruct;
int cur_struct;

void add_struct(sType *root_type)
{
	sStruct s;
	s.RootType = root_type;
	PreStruct.push_back(s);
	cur_struct = PreStruct.size() - 1;
}

void struct_add_element(char *name, sType *type, int offset)
{
	sStructElement e;
	strcpy(e.Name, name);
	e.Type = type;
	e.Offset = offset;
	PreStruct[cur_struct].Element.push_back(e);
}

int add_func(char *name, sType *return_type, void *func, void *instance = NULL);

void struct_add_func(char *name, sType *return_type, void *func)
{
	char *mname = new char[strlen(name) + 2];
	strcpy(mname, string(".", name));
	int cmd = add_func(mname, return_type, func, f_class);
	sStructFunction f;
	strcpy(f.Name, name);
	f.cmd = cmd;
	PreStruct[cur_struct].Function.push_back(f);
}


//------------------------------------------------------------------------------------------------//
//                                           constants                                            //
//------------------------------------------------------------------------------------------------//

std::vector<sPreConstant> PreConstant;
void add_const(char *name, sType *type, void *value)
{
	sPreConstant c;
	c.Name = name;
	c.Type = type;
	c.Value = value;
	PreConstant.push_back(c);
}


//------------------------------------------------------------------------------------------------//
//                                  pre defined global variables                                  //
//------------------------------------------------------------------------------------------------//

std::vector<sPreGlobalVar> PreGlobalVar;

/*#define NUM_PRE_GLOBAL_VARS		1
#define MAX_PRE_GLOBAL_VARS		128

int NumPreGlobalVars=NUM_PRE_GLOBAL_VARS;
sPreGlobalVar PreGlobalVar[MAX_PRE_GLOBAL_VARS]={
	{"this",	&TypeObjectP}
};*/

void ScriptAddPreGlobalVar(char *name, sType *type)
{
	sPreGlobalVar v;
	v.Name = name;
	v.Type = type;
	PreGlobalVar.push_back(v);
}


//------------------------------------------------------------------------------------------------//
//                                    environmental variables                                     //
//------------------------------------------------------------------------------------------------//

std::vector<sPreExternalVar> PreExternalVar;
int NumTruePreExternalVars = 0;

void add_ext_var(char *name, sType *type, void *var)
{
	sPreExternalVar v;
	v.Name = name;
	v.Type = type;
	v.Pointer = var;
	PreExternalVar.push_back(v);
	NumTruePreExternalVars = PreExternalVar.size();
};

//------------------------------------------------------------------------------------------------//
//                                      compiler functions                                        //
//------------------------------------------------------------------------------------------------//



#ifndef FILE_OS_WINDOWS
	//#define _cdecl
	#include <stdlib.h>
#endif

float _cdecl f_sin(float f){	return (float)sin(double(f));	}
float _cdecl f_cos(float f){	return (float)cos(double(f));	}
float _cdecl f_tan(float f){	return (float)tan(double(f));	}
float _cdecl f_asin(float f){	return (float)asin(double(f));	}
float _cdecl f_acos(float f){	return (float)acos(double(f));	}
float _cdecl f_atan(float f){	return (float)atan(double(f));	}
float _cdecl f_atan2(float x,float y){	return (float)atan2((double)x,(double)y);	}
float _cdecl f_sqrt(float f){	return (float)sqrt(float(f));	}
float _cdecl f_sqr(float f){	return f*f;	}
float _cdecl f_pow(float f,float ex){	return (float)pow(float(f),float(ex));	}
float _cdecl f_exp(float f){	return (float)exp(f);	}
float _cdecl f_abs(float f){	return fabs(f);	}
void _cdecl intout(int i){	msg_write(string2("IntOut:    %d",i));	}
void _cdecl floatout(float f){	msg_write(string2("FloatOut:  %.3f",f));	}
void _cdecl boolout(bool b){	if (b)	msg_write("BoolOut:  true");	else	msg_write("BoolOut:  false");	}
void _cdecl _stringout(char *str){	msg_write(string("StringOut: ",str));	}
void _cdecl pointerout(char *p){	msg_write(string("PointerOut:    ",d2h((char*)&p,4,false)));	}
sDate _cdecl FileGetDate(CFile *f,int type){	return f->GetDate(type);	}
void _cdecl FileWriteStr(CFile *f,char *str){	f->WriteStr(str);	}
int _cdecl _Float2Int(float f){	return int(f);	}

#ifdef _X_ALLOW_MODEL_
	vector _cdecl ModelGetVertex(CModel *m,int index,int skin)
	{	return m->GetVertex(index,skin);	}
#else
	vector _cdecl ModelGetVertex(void *m,int index,int skint)
	{	return v0;	}
#endif
typedef void (CFile::*tmf)();
typedef char *tcpa[4];
void *mf(tmf vmf)
{
	tcpa *cpa=(tcpa*)&vmf;
	return (*cpa)[0];
}

void *f_cp = (void*)1; // for fake (compiler-) functions
void *f_class = (void*)2; // for class member functions


std::vector<sPreCommand> PreCommand;

int cur_func;

int add_func(char *name, sType *return_type, void *func, void *instance)
{
	sPreCommand c;
	c.Name = name;
	c.ReturnType = return_type;
	c.Func = func;
	c.Instance = instance;
	PreCommand.push_back(c);
	cur_func = PreCommand.size() - 1;
	return cur_func;
}

void func_add_param(char *name, sType *type)
{
	sPreCommandParam p;
	p.Name = name;
	p.Type = type;
	PreCommand[cur_func].Param.push_back(p);
}




// automatic type casting


char CastTemp[256];
char *CastFloat2Int(float *f)
{
	*(int*)&CastTemp[0]=int(*f);
	return &CastTemp[0];
}
char *CastInt2Float(int *i)
{
	*(float*)&CastTemp[0]=float(*i);
	return &CastTemp[0];
}
char *CastInt2Char(int *i)
{
	*(char*)&CastTemp[0]=char(*i);
	return &CastTemp[0];
}
char *CastChar2Int(char *c)
{
	
	*(int*)&CastTemp[0]=int(*c);
	return &CastTemp[0];
}
char *CastPointer2Bool(void **p)
{
	
	*(bool*)&CastTemp[0]=( (*p) != NULL );
	return &CastTemp[0];
}

std::vector<sTypeCast> TypeCast;
void add_type_cast(int penalty, sType *source, sType *dest, int cmd, void *func)
{
	sTypeCast c;
	c.Penalty = penalty;
	c.Command = cmd;
	c.Source = source;
	c.Dest = dest;
	c.Func = (t_cast_func*) func;
	TypeCast.push_back(c);
}



#ifdef _X_ALLOW_META_
	#define meta_p(p)	(void*)p
#else
	#define meta_p(p)	NULL
#endif
#ifdef _X_ALLOW_GOD_
	#define god_p(p)	(void*)p
#else
	#define god_p(p)	NULL
#endif
#ifdef _X_ALLOW_FX_
	#define fx_p(p)	(void*)p
#else
	#define fx_p(p)	NULL
#endif
#ifdef _X_ALLOW_GUI_
	#define gui_p(p)	(void*)p
#else
	#define gui_p(p)	NULL
#endif
#ifdef _X_ALLOW_CAMERA_
	#define cam_p(p)	(void*)p
#else
	#define cam_p(p)	NULL
#endif


void ScriptInit()
{
	msg_db_r("ScriptInit", 1);

	AsmInit();
	
	msg_db_m("type", 1);


	
	TypeUnknown				= add_type("-\?\?\?-",	0); // should not appear anywhere....or else we're screwed up!
	TypeVoid				= add_type("void",		0);
	TypePointer				= add_type("void*"		,PointerSize,		0,	true); // substitute for all pointer types
	sType *TypePointerPS	= add_type("void*&"		,PointerSize		,0	,true	,true	,TypePointer);
	sType *TypePointerList	= add_type("void*[]"		,0					,1	,false	,false	,TypePointer);
	TypeStruct				= add_type("-struct-"	,0					,0	,false	,false	,NULL); // substitute for all struct types
	TypeBool				= add_type("bool"		,sizeof(bool)		,0	,false	,false	,NULL);
	sType *TypeBoolList		= add_type("bool[]"		,0					,1	,false	,false	,TypeBool);
	TypeInt					= add_type("int"			,sizeof(int)		,0	,false	,false	,NULL);
	sType *TypeIntP			= add_type("int*"		,PointerSize		,0	,true	,false	,TypeInt);
	sType *TypeIntPs		= add_type("int&"		,PointerSize		,0	,true	,true	,TypeInt);
	sType *TypeIntList		= add_type("int[]"		,0					,1	,false	,false	,TypeInt);
	sType *TypeIntList2		= add_type("int[][]"		,0					,1	,false	,false	,TypeIntList);
	TypeFloat				= add_type("float"		,sizeof(float)		,0	,false	,false	,NULL);
	sType *TypeFloatP		= add_type("float*"		,PointerSize		,0	,true	,false	,TypeFloat);
	sType *TypeFloatPs		= add_type("float&"		,PointerSize		,0	,true	,true	,TypeFloat);
	sType *TypeFloatList	= add_type("float[]"		,0					,1	,false	,false	,TypeFloat);
	TypeChar				= add_type("char"		,sizeof(char)		,0	,false	,false	,NULL);
	sType *TypeCharP		= add_type("char*"		,PointerSize		,0	,true	,false	,TypeChar);
	TypeString				= add_type("string"		,256*TypeChar->Size	,256,false	,false	,TypeChar);	// string := char[256]
	sType *TypeStringP		= add_type("string*"		,PointerSize		,0	,true	,false	,TypeString);
	sType *TypeStringPP		= add_type("string**"	,PointerSize		,0	,true	,false	,TypeStringP);
	sType *TypeStringList	= add_type("string[]"	,0					,1	,false	,false	,TypeString);
	sType *TypeStringPList	= add_type("string*[]"	,0					,1	,false	,false	,TypeStringP);
	TypeVector				= add_type("vector"		,sizeof(vector)		,0	,false	,false	,NULL);
	sType *TypeVectorP		= add_type("vector*"		,PointerSize		,0	,true	,false	,TypeVector);
	sType *TypeVectorPs		= add_type("vector&"		,PointerSize		,0	,true	,true	,TypeVector);
	sType *TypeVectorList	= add_type("vector[]"	,0					,1	,false	,false	,TypeVector);
	TypeRect				= add_type("rect"		,sizeof(rect)		,0	,false	,false	,NULL);
	sType *TypeRectP		= add_type("rect*"		,PointerSize		,0	,true	,false	,TypeRect);
	sType *TypeRectPs		= add_type("rect&"		,PointerSize		,0	,true	,true	,TypeRect);
	sType *TypeMatrix		= add_type("matrix"		,sizeof(matrix)		,0	,false	,false	,NULL);
	sType *TypeMatrixP		= add_type("matrix*"		,PointerSize		,0	,true	,false	,TypeMatrix);
	sType *TypeMatrixPs		= add_type("matrix&"		,PointerSize		,0	,true	,true	,TypeMatrix);
	TypeQuaternion			= add_type("quaternion"	,sizeof(quaternion)	,0	,false	,false	,NULL);
	sType *TypeQuaternionP	= add_type("quaternion*"	,PointerSize		,0	,true	,false	,TypeQuaternion);
	sType *TypeQuaternionPs	= add_type("quaternion&"	,PointerSize		,0	,true	,true	,TypeQuaternion);
	sType *TypePlane		= add_type("plane"		,sizeof(plane)		,0	,false	,false	,NULL);
	sType *TypePlaneP		= add_type("plane*"		,PointerSize		,0	,true	,false	,TypePlane);
	TypeColor				= add_type("color"		,sizeof(color)		,0	,false	,false	,NULL);
	sType *TypeColorP		= add_type("color*"		,PointerSize		,0	,true	,false	,TypeColor);
	sType *TypeColorPs		= add_type("color&"		,PointerSize		,0	,true	,true	,TypeColor);
	sType *TypeMatrix3		= add_type("matrix3"	,sizeof(matrix3)	,0	,false	,false	,NULL);
	sType *TypeObject		= add_type("-CObject-"	,0					,0	,false	,false	,NULL);
	sType *TypeObjectP		= add_type("object"		,PointerSize		,0	,true	,false	,TypeObject);
	sType *TypeObjectPP		= add_type("*object"	,PointerSize		,0	,true	,false	,TypeObjectP);
	sType *TypeObjectPList	= add_type("object[]"	,0					,1	,false	,false	,TypeObjectP);
	sType *TypeModel		= add_type("-CModel-"	,0					,0	,false	,false	,NULL);
	sType *TypeModelP		= add_type("model"		,PointerSize		,0	,true	,false	,TypeModel);
	sType *TypeModelPP		= add_type("model*"		,PointerSize		,0	,true	,false	,TypeModelP);
	sType *TypeItem			= add_type("-sItem-"	,0					,0	,false	,false	,NULL);
	sType *TypeItemP		= add_type("item"		,PointerSize		,0	,true	,false	,TypeItem);
	sType *TypeItemPP		= add_type("item*"		,PointerSize		,0	,true	,false	,TypeItemP);
	sType *TypeFile			= add_type("-CFile-"	,0					,0	,false	,false	,NULL);
	sType *TypeFileP		= add_type("file"		,PointerSize		,0	,true	,false	,TypeFile);
	sType *TypeDate			= add_type("date"		,sizeof(sDate)		,0	,false	,false	,NULL);
	sType *TypeText			= add_type("-sText-"	,0					,0	,false	,false	,NULL);
	sType *TypeTextP		= add_type("text"		,PointerSize		,0	,true	,false	,TypeText);
	sType *TypePicture		= add_type("-sPicture-"	,0					,0	,false	,false	,NULL);
	sType *TypePictureP		= add_type("picture"	,PointerSize		,0	,true	,false	,TypePicture);
	sType *TypePicture3D	= add_type("-sPicture3d-",0					,0	,false	,false	,NULL);
	sType *TypePicture3DP	= add_type("picture3d"	,PointerSize		,0	,true	,false	,TypePicture3D);
	sType *TypeGrouping		= add_type("-sGrouping-",0					,0	,false	,false	,NULL);
	sType *TypeGroupingP	= add_type("grouping"	,PointerSize		,0	,true	,false	,TypeGrouping);
	sType *TypeParticle		= add_type("-sParticle-",0					,0	,false	,false	,NULL);
	sType *TypeParticleP	= add_type("particle"	,PointerSize		,0	,true	,false	,TypeParticle);
	sType *TypeBeam			= add_type("-sBeam-"	,0					,0	,false	,false	,NULL);
	sType *TypeBeamP		= add_type("beam"		,PointerSize		,0	,true	,false	,TypeBeam);
	sType *TypeEffect		= add_type("-sEffect-"	,0					,0	,false	,false	,NULL);
	sType *TypeEffectP		= add_type("effect"		,PointerSize		,0	,true	,false	,TypeEffect);
	sType *TypeView			= add_type("-sView-"	,0					,0	,false	,false	,NULL);
	sType *TypeViewP		= add_type("view"		,PointerSize		,0	,true	,false	,TypeView);
	sType *TypeSkin			= add_type("-sSkin-"	,0					,0	,false	,false	,NULL);
	sType *TypeSkinP		= add_type("skin"		,PointerSize		,0	,true	,false	,TypeSkin);
	sType *TypeSkinPList	= add_type("skin[]"		,0					,1	,true	,false	,TypeSkinP);
	sType *TypeMaterial		= add_type("-sMaterial-",0					,0	,false	,false	,NULL);
	sType *TypeMaterialP	= add_type("-sMaterial-*",	PointerSize		,0	,true	,false	,TypeMaterial);
	sType *TypeFog			= add_type("fog",		0					,0	,false	,false	,NULL);
	sType *TypeTerrain		= add_type("-CTerrain-",0					,0	,false	,false	,NULL);
	sType *TypeTerrainP		= add_type("terrain",		PointerSize		,0	,true	,false	,TypeTerrain);
	sType *TypeTerrainPP	= add_type("terrain*",		PointerSize		,0	,true	,false	,TypeTerrainP);
	sType *TypeTerrainPList	= add_type("terrain[]",		0					,1	,false	,false	,TypeTerrainP);


	msg_db_m("struct", 1);
	
	add_struct(TypeVector);
		struct_add_element("x",		TypeFloat,	0);
		struct_add_element("y",		TypeFloat,	4);
		struct_add_element("z",		TypeFloat,	8);
	
	add_struct(TypeQuaternion);
		struct_add_element("x",		TypeFloat,	0);
		struct_add_element("y",		TypeFloat,	4);
		struct_add_element("z",		TypeFloat,	8);
		struct_add_element("w",		TypeFloat,	12);
	
	add_struct(TypeRect);
		struct_add_element("x1",	TypeFloat,	0);
		struct_add_element("x2",	TypeFloat,	4);
		struct_add_element("y1",	TypeFloat,	8);
		struct_add_element("y2",	TypeFloat,	12);
	
	add_struct(TypeColor);
		struct_add_element("a",		TypeFloat,	12);
		struct_add_element("r",		TypeFloat,	0);
		struct_add_element("g",		TypeFloat,	4);
		struct_add_element("b",		TypeFloat,	8);
	
	add_struct(TypePlane);
		struct_add_element("a",		TypeFloat,	0);
		struct_add_element("b",		TypeFloat,	4);
		struct_add_element("c",		TypeFloat,	8);
		struct_add_element("d",		TypeFloat,	12);
	
	add_struct(TypeMatrix);
		struct_add_element("_11",	TypeFloat,	0);
		struct_add_element("_12",	TypeFloat,	4);
		struct_add_element("_13",	TypeFloat,	8);
		struct_add_element("_14",	TypeFloat,	12);
		struct_add_element("_21",	TypeFloat,	16);
		struct_add_element("_22",	TypeFloat,	20);
		struct_add_element("_23",	TypeFloat,	24);
		struct_add_element("_24",	TypeFloat,	28);
		struct_add_element("_31",	TypeFloat,	32);
		struct_add_element("_32",	TypeFloat,	36);
		struct_add_element("_33",	TypeFloat,	40);
		struct_add_element("_34",	TypeFloat,	44);
		struct_add_element("_41",	TypeFloat,	48);
		struct_add_element("_42",	TypeFloat,	52);
		struct_add_element("_43",	TypeFloat,	56);
		struct_add_element("_44",	TypeFloat,	60);
	
	add_struct(TypeMatrix3);
		struct_add_element("_11",	TypeFloat,	0);
		struct_add_element("_12",	TypeFloat,	4);
		struct_add_element("_13",	TypeFloat,	8);
		struct_add_element("_21",	TypeFloat,	12);
		struct_add_element("_22",	TypeFloat,	16);
		struct_add_element("_23",	TypeFloat,	20);
		struct_add_element("_31",	TypeFloat,	24);
		struct_add_element("_32",	TypeFloat,	28);
		struct_add_element("_33",	TypeFloat,	32);
	
	add_struct(TypePicture);
		struct_add_element("Enabled",		TypeBool,		GetDAPicture(Enabled));
		struct_add_element("TCInverted",	TypeBool,		GetDAPicture(TCInverted));
		struct_add_element("Pos",			TypeVector,		GetDAPicture(Pos));
		struct_add_element("Width",			TypeFloat,		GetDAPicture(Width));
		struct_add_element("Height",		TypeFloat,		GetDAPicture(Height));
		struct_add_element("Color",			TypeColor,		GetDAPicture(Color));
		struct_add_element("Texture",		TypeInt,		GetDAPicture(Texture));
		struct_add_element("Source",		TypeRect,		GetDAPicture(Source));
		struct_add_element("ShaderFile",	TypeInt,		GetDAPicture(ShaderFile));
	
	add_struct(TypePicture3D);
		struct_add_element("Enabled",		TypeBool,		GetDAPicture3D(Enabled));
		struct_add_element("Relative",		TypeBool,		GetDAPicture3D(Relative));
		struct_add_element("Lighting",		TypeBool,		GetDAPicture3D(Lighting));
		struct_add_element("z",				TypeFloat,		GetDAPicture3D(z));
		struct_add_element("Matrix",		TypeMatrix,		GetDAPicture3D(Matrix));
		struct_add_element("model",			TypeModelP,		GetDAPicture3D(model));
	
	add_struct(TypeGrouping);
		struct_add_element("Enabled",		TypeBool,		GetDAGrouping(Enabled));
		struct_add_element("Pos",			TypeVector,		GetDAGrouping(Pos));
		struct_add_element("Color",			TypeColor,		GetDAGrouping(Color));
	
	add_struct(TypeText);
		struct_add_element("Enabled",		TypeBool,		GetDAText(Enabled));
		struct_add_element("Centric",		TypeBool,		GetDAText(Centric));
		struct_add_element("Vertical",		TypeBool,		GetDAText(Vertical));
		struct_add_element("Font",			TypeInt,		GetDAText(Font));
		struct_add_element("Pos",			TypeVector,		GetDAText(Pos));
		struct_add_element("Size",			TypeFloat,		GetDAText(Size));
		struct_add_element("Color",			TypeColor,		GetDAText(Color));
		struct_add_element("Str",			TypeString,		GetDAText(Str));
	
	add_struct(TypeParticle);
		struct_add_element("Enabled",		TypeBool,		GetDAParticle(Enabled));
		struct_add_element("Suicidal",		TypeBool,		GetDAParticle(Suicidal));
		struct_add_element("Pos",			TypeVector,		GetDAParticle(Pos));
		struct_add_element("Vel",			TypeVector,		GetDAParticle(Vel));
		struct_add_element("Ang",			TypeVector,		GetDAParticle(Parameter));
		struct_add_element("TimeToLive",	TypeFloat,		GetDAParticle(TimeToLive));
		struct_add_element("Radius",		TypeFloat,		GetDAParticle(Radius));
		struct_add_element("Color",			TypeColor,		GetDAParticle(Color));
		struct_add_element("Texture",		TypeInt,		GetDAParticle(Texture));
		struct_add_element("Source",		TypeRect,		GetDAParticle(Source));
		struct_add_element("FuncDeltaT",	TypeFloat,		GetDAParticle(FuncDeltaT));
		struct_add_element("elapsed",		TypeFloat,		GetDAParticle(elapsed));
		struct_add_element("func",			TypePointer,	GetDAParticle(func));

	add_struct(TypeBeam);
		struct_add_element("Enabled",		TypeBool,		GetDAParticle(Enabled));
		struct_add_element("Suicidal",		TypeBool,		GetDAParticle(Suicidal));
		struct_add_element("Pos",			TypeVector,		GetDAParticle(Pos));
		struct_add_element("Vel",			TypeVector,		GetDAParticle(Vel));
		struct_add_element("Length",		TypeVector,		GetDAParticle(Parameter));
		struct_add_element("TimeToLive",	TypeFloat,		GetDAParticle(TimeToLive));
		struct_add_element("Radius",		TypeFloat,		GetDAParticle(Radius));
		struct_add_element("Color",			TypeColor,		GetDAParticle(Color));
		struct_add_element("Texture",		TypeInt,		GetDAParticle(Texture));
		struct_add_element("Source",		TypeRect,		GetDAParticle(Source));
		struct_add_element("FuncDeltaT",	TypeFloat,		GetDAParticle(FuncDeltaT));
		struct_add_element("elapsed",		TypeFloat,		GetDAParticle(elapsed));
		struct_add_element("func",			TypePointer,	GetDAParticle(func));
	
	add_struct(TypeEffect);
		struct_add_element("Enabled",		TypeBool,		GetDAEffect(Enabled));
		struct_add_element("Used",			TypeBool,		GetDAEffect(Used));
		struct_add_element("Suicidal",		TypeBool,		GetDAEffect(Suicidal));
		struct_add_element("Pos",			TypeVector,		GetDAEffect(Pos));
		struct_add_element("Vel",			TypeVector,		GetDAEffect(Vel));
		struct_add_element("TimeToLive",	TypeFloat,		GetDAEffect(TimeToLive));
		struct_add_element("ScriptVar",		TypeFloatList,	GetDAEffect(ScriptVar));
		struct_add_element("ScriptVarI",	TypeIntList,	GetDAEffect(ScriptVar));
		struct_add_element("ScriptVarP",	TypePointerList,GetDAEffect(ScriptVar));
		struct_add_element("FuncDeltaT",	TypeFloat,		GetDAEffect(FuncDeltaT));
		struct_add_element("elapsed",		TypeFloat,		GetDAEffect(elapsed));
		struct_add_element("func",			TypePointer,	GetDAEffect(func));
		struct_add_element("del_func",		TypePointer,	GetDAEffect(del_func));
		struct_add_element("model",			TypeModelP,		GetDAEffect(model));
		struct_add_element("vertex",		TypeInt,		GetDAEffect(vertex));
	
	add_struct(TypeObject);
		struct_add_element("Pos",			TypeVector,		GetDAObject(Pos));
		struct_add_element("Vel",			TypeVector,		GetDAObject(Vel));
		struct_add_element("VelS",			TypeVector,		GetDAObject(VelS));
		struct_add_element("Ang",			TypeVector,		GetDAObject(Ang));
		struct_add_element("Rot",			TypeVector,		GetDAObject(Rot));
		struct_add_element("Matrix",		TypeMatrixP,	GetDAObject(Matrix));
		struct_add_element("Name",			TypeString,		GetDAObject(Name));
		struct_add_element("OnGround",		TypeBool,		GetDAObject(OnGround));
		struct_add_element("GroundID",		TypeInt,		GetDAObject(GroundID));
		struct_add_element("GroundNormal",	TypeVector,		GetDAObject(GroundNormal));
		struct_add_element("GFactor",		TypeFloat,		GetDAObject(GFactor));
		struct_add_element("ID",			TypeInt,		GetDAObject(ID));
		struct_add_element("Visible",		TypeBool,		GetDAObject(Visible));
		struct_add_element("ActivePhysics",	TypeBool,		GetDAObject(ActivePhysics));
		struct_add_element("PassivePhysics",TypeBool,		GetDAObject(PassivePhysics));
		struct_add_element("AllowShadow",	TypeBool,		GetDAObject(AllowShadow));
		struct_add_element("model",			TypeModelP,		GetDAObject(model));
		struct_add_element("Mass",			TypeFloat,		GetDAObject(Mass));
		struct_add_element("Theta",			TypeMatrix3,	GetDAObject(Theta));
		struct_add_element("Radius",		TypeFloat,		GetDAObject(Radius));
		struct_add_element("Life",			TypeFloat,		GetDAObject(Life));
		struct_add_element("MaxLife",		TypeFloat,		GetDAObject(MaxLife));
		struct_add_element("ScriptVar",		TypeFloatP,		GetDAObject(ScriptVar));
		struct_add_element("ScriptVarI",	TypeIntP,		GetDAObject(ScriptVar));
		struct_add_element("Item",			TypeItemPP,		GetDAObject(items));
		struct_add_element("ItemFilename",	TypeStringP,	GetDAObject(ItemFilename));

	add_struct(TypeModel);
		struct_add_element("Skin",			TypeSkinPList,	GetDAModel(Skin));
		struct_add_element("Skin0",			TypeSkinP,		GetDAModel(Skin[0]));
		struct_add_element("Skin1",			TypeSkinP,		GetDAModel(Skin[1]));
		struct_add_element("Skin2",			TypeSkinP,		GetDAModel(Skin[2]));
		struct_add_element("NumMaterials",	TypeInt,		GetDAModel(NumMaterials));
		struct_add_element("Material",		TypeMaterialP,	GetDAModel(Material));
		struct_add_element("Theta",			TypeMatrix3,	GetDAModel(Theta));
		struct_add_element("NumBones",		TypeInt,		GetDAModel(NumBones));
		struct_add_element("BoneRoot",		TypeIntP,		GetDAModel(BoneRoot));
		struct_add_element("BonePos",		TypeVectorP,	GetDAModel(BonePos));
		struct_add_element("BoneModel",		TypeModelPP,	GetDAModel(BoneModel));
		struct_add_element("BoneDMatrix",	TypeMatrixP,	GetDAModel(BoneDMatrix));
		struct_add_element("BoneCurAng",	TypeQuaternionP,GetDAModel(BoneCurAng));
		struct_add_element("BoneCurPos",	TypeVectorP,	GetDAModel(BoneCurPos));
		struct_add_element("Matrix",		TypeMatrix,		GetDAModel(Matrix));
		struct_add_element("Min",			TypeVector,		GetDAModel(Min));
		struct_add_element("Max",			TypeVector,		GetDAModel(Max));
		struct_add_element("TestCollisions",TypeBool,		GetDAModel(TestCollisions));
		struct_add_element("object",		TypeObjectP,	GetDAModel(object));

	add_struct(TypeSkin);
		struct_add_element("NumVertices",		TypeInt,		GetDASkin(NumVertices));
		struct_add_element("Vertex",			TypeVectorP,	GetDASkin(Vertex));
		struct_add_element("NumSkinVertices",	TypeInt,		GetDASkin(NumSkinVertices));
		struct_add_element("SkinVertex",		TypeFloatP,		GetDASkin(SkinVertex));
		struct_add_element("NumTriangles",		TypeInt,		GetDASkin(NumTriangles));
		struct_add_element("TriangleIndex",		TypeIntP,		GetDASkin(TriangleIndex));
		struct_add_element("TriangleSkinIndex",	TypeIntP,		GetDASkin(TriangleSkinIndex));
		struct_add_element("Normal",			TypeVectorP,	GetDASkin(Normal));

	add_struct(TypeMaterial);
		struct_add_element("NumTextures",	TypeInt,		GetDAMaterial(NumTextures));
		struct_add_element("Texture",		TypeIntList,	GetDAMaterial(Texture));
		struct_add_element("ShaderFile",	TypeInt,		GetDAMaterial(ShaderFile));
		struct_add_element("AlphaFactor",	TypeFloat,		GetDAMaterial(AlphaFactor));
		struct_add_element("Ambient",		TypeColor,		GetDAMaterial(Ambient));
		struct_add_element("Diffuse",		TypeColor,		GetDAMaterial(Diffuse));
		struct_add_element("Specular",		TypeColor,		GetDAMaterial(Specular));
		struct_add_element("Emission",		TypeColor,		GetDAMaterial(Emission));
		struct_add_element("Shininess",		TypeFloat,		GetDAMaterial(Shininess));

	add_struct(TypeItem);
		struct_add_element("Kind",			TypeInt,		GetDAItem(Kind));
		struct_add_element("OID",			TypeInt,		GetDAItem(OID));
		struct_add_element("Quantity",		TypeInt,		GetDAItem(Quantity));
		struct_add_element("QuantityMax",	TypeInt,		GetDAItem(QuantityMax));
		struct_add_element("ScriptVar",		TypeFloatList,	GetDAItem(ScriptVar[0]));
		struct_add_element("model",			TypeModelP,		GetDAItem(model));
		struct_add_element("Name",			TypeString,		GetDAItem(Name[0]));
		struct_add_element("Description",	TypeString,		GetDAItem(Description[0]));

	add_struct(TypeTerrain);
		struct_add_element("Pos",			TypeVector,		GetDATerrain(Pos));
		struct_add_element("NumX",			TypeInt,		GetDATerrain(NumX));
		struct_add_element("NumZ",			TypeInt,		GetDATerrain(NumZ));
		struct_add_element("Height",		TypeFloatP,		GetDATerrain(Height));
		struct_add_element("Pattern",		TypeVector,		GetDATerrain(Pattern));
		struct_add_element("NumTextures",	TypeInt,		GetDATerrain(NumTextures));
		struct_add_element("Texture",		TypeIntList,	GetDATerrain(Texture));
		struct_add_element("TextureScale",	TypeVectorList,	GetDATerrain(TextureScale));

	add_struct(TypeView);
		struct_add_element("Enabled",		TypeBool,		GetDAView(Enabled));
		struct_add_element("Show",			TypeBool,		GetDAView(Show));
		struct_add_element("OutputTexture",	TypeInt,		GetDAView(OutputTexture));
		struct_add_element("InputTexture",	TypeInt,		GetDAView(InputTexture));
		struct_add_element("ShaderFile",	TypeInt,		GetDAView(ShaderFile));
		struct_add_element("ShadedDisplays",TypeBool,		GetDAView(ShadedDisplays));
		struct_add_element("Pos",			TypeVector,		GetDAView(Pos));
		struct_add_element("Ang",			TypeVector,		GetDAView(Ang));
		struct_add_element("Vel",			TypeVector,		GetDAView(Vel));
		struct_add_element("Rot",			TypeVector,		GetDAView(Rot));
		struct_add_element("Zoom",			TypeFloat,		GetDAView(Zoom));
		struct_add_element("Dest",			TypeRect,		GetDAView(Dest));
		struct_add_element("z",				TypeFloat,		GetDAView(z));

	add_struct(TypeFog);
		struct_add_element("Enabled",		TypeBool,		GetDAFog(Enabled));
		struct_add_element("Mode",			TypeInt,		GetDAFog(Mode));
		struct_add_element("Start",			TypeFloat,		GetDAFog(Start));
		struct_add_element("End",			TypeFloat,		GetDAFog(End));
		struct_add_element("Density",		TypeFloat,		GetDAFog(Density));
		struct_add_element("Color",			TypeColor,		GetDAFog(Color));

	add_struct(TypeDate);
		struct_add_element("time",			TypeInt,		GetDADate(time));
		struct_add_element("year",			TypeInt,		GetDADate(year));
		struct_add_element("month",			TypeInt,		GetDADate(month));
		struct_add_element("day",			TypeInt,		GetDADate(day));
		struct_add_element("hour",			TypeInt,		GetDADate(hour));
		struct_add_element("minute",		TypeInt,		GetDADate(minute));
		struct_add_element("second",		TypeInt,		GetDADate(second));
		struct_add_element("milli_second",	TypeInt,		GetDADate(milli_second));
		struct_add_element("day_of_week",	TypeInt,		GetDADate(day_of_week));
		struct_add_element("day_of_year",	TypeInt,		GetDADate(day_of_year));






msg_db_m("operator", 1);



	add_operator(OperatorAssign,		TypeVoid,		TypePointer,	TypePointer);
	add_operator(OperatorEqual,			TypeBool,		TypePointer,	TypePointer);
	add_operator(OperatorNotEqual,		TypeBool,		TypePointer,	TypePointer);
	add_operator(OperatorAssign,		TypeVoid,		TypeChar,		TypeChar);
	add_operator(OperatorEqual,			TypeBool,		TypeChar,		TypeChar);
	add_operator(OperatorNotEqual,		TypeBool,		TypeChar,		TypeChar);
	add_operator(OperatorAdd,			TypeChar,		TypeChar,		TypeChar);
	add_operator(OperatorSubtractS,		TypeChar,		TypeChar,		TypeChar);
	add_operator(OperatorAddS,			TypeChar,		TypeChar,		TypeChar);
	add_operator(OperatorSubtract,		TypeChar,		TypeChar,		TypeChar);
	add_operator(OperatorBitAnd,		TypeChar,		TypeChar,		TypeChar);
	add_operator(OperatorBitOr,			TypeChar,		TypeChar,		TypeChar);
	add_operator(OperatorSubtract,		TypeChar,		TypeVoid,		TypeChar);
	add_operator(OperatorAssign,		TypeVoid,		TypeBool,		TypeBool);
	add_operator(OperatorEqual,			TypeBool,		TypeBool,		TypeBool);
	add_operator(OperatorNotEqual,		TypeBool,		TypeBool,		TypeBool);
	add_operator(OperatorGreater,		TypeBool,		TypeBool,		TypeBool);
	add_operator(OperatorGreaterEqual,	TypeBool,		TypeBool,		TypeBool);
	add_operator(OperatorSmaller,		TypeBool,		TypeBool,		TypeBool);
	add_operator(OperatorSmallerEqual,	TypeBool,		TypeBool,		TypeBool);
/*	add_operator(OperatorAnd,			TypeBool,		TypeBool,		TypeBool);
	add_operator(OperatorOr,			TypeBool,		TypeBool,		TypeBool);*/
	add_operator(OperatorAndLiteral,	TypeBool,		TypeBool,		TypeBool);
	add_operator(OperatorOrLiteral,		TypeBool,		TypeBool,		TypeBool);
	add_operator(OperatorNegate,		TypeBool,		TypeVoid,		TypeBool);
	add_operator(OperatorAssign,		TypeVoid,		TypeInt,		TypeInt);
	add_operator(OperatorAdd,			TypeInt,		TypeInt,		TypeInt);
	add_operator(OperatorSubtract,		TypeInt,		TypeInt,		TypeInt);
	add_operator(OperatorMultiply,		TypeInt,		TypeInt,		TypeInt);
	add_operator(OperatorDivide,		TypeInt,		TypeInt,		TypeInt);
	add_operator(OperatorAddS,			TypeVoid,		TypeInt,		TypeInt);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeInt,		TypeInt);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeInt,		TypeInt);
	add_operator(OperatorDivideS,		TypeVoid,		TypeInt,		TypeInt);
	add_operator(OperatorModulo,		TypeInt,		TypeInt,		TypeInt);
	add_operator(OperatorEqual,			TypeBool,		TypeInt,		TypeInt);
	add_operator(OperatorNotEqual,		TypeBool,		TypeInt,		TypeInt);
	add_operator(OperatorGreater,		TypeBool,		TypeInt,		TypeInt);
	add_operator(OperatorGreaterEqual,	TypeBool,		TypeInt,		TypeInt);
	add_operator(OperatorSmaller,		TypeBool,		TypeInt,		TypeInt);
	add_operator(OperatorSmallerEqual,	TypeBool,		TypeInt,		TypeInt);
	add_operator(OperatorBitAnd,		TypeInt,		TypeInt,		TypeInt);
	add_operator(OperatorBitOr,			TypeInt,		TypeInt,		TypeInt);
	add_operator(OperatorShiftRight,	TypeInt,		TypeInt,		TypeInt);
	add_operator(OperatorShiftLeft,		TypeInt,		TypeInt,		TypeInt);
	add_operator(OperatorSubtract,		TypeInt,		TypeVoid,		TypeInt);
	add_operator(OperatorIncrease,		TypeVoid,		TypeInt,		TypeVoid);
	add_operator(OperatorDecrease,		TypeVoid,		TypeInt,		TypeVoid);
	add_operator(OperatorAssign,		TypeVoid,		TypeFloat,		TypeFloat);
	add_operator(OperatorAdd,			TypeFloat,		TypeFloat,		TypeFloat);
	add_operator(OperatorSubtract,		TypeFloat,		TypeFloat,		TypeFloat);
	add_operator(OperatorMultiply,		TypeFloat,		TypeFloat,		TypeFloat);
	add_operator(OperatorMultiply,		TypeFloat,		TypeFloat,		TypeInt);
	add_operator(OperatorMultiply,		TypeFloat,		TypeInt,		TypeFloat);
	add_operator(OperatorDivide,		TypeFloat,		TypeFloat,		TypeFloat);
	add_operator(OperatorAddS,			TypeVoid,		TypeFloat,		TypeFloat);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeFloat,		TypeFloat);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeFloat,		TypeFloat);
	add_operator(OperatorDivideS,		TypeVoid,		TypeFloat,		TypeFloat);
	add_operator(OperatorEqual,			TypeBool,		TypeFloat,		TypeFloat);
	add_operator(OperatorNotEqual,		TypeBool,		TypeFloat,		TypeFloat);
	add_operator(OperatorGreater,		TypeBool,		TypeFloat,		TypeFloat);
	add_operator(OperatorGreaterEqual,	TypeBool,		TypeFloat,		TypeFloat);
	add_operator(OperatorSmaller,		TypeBool,		TypeFloat,		TypeFloat);
	add_operator(OperatorSmallerEqual,	TypeBool,		TypeFloat,		TypeFloat);
	add_operator(OperatorSubtract,		TypeFloat,		TypeVoid,		TypeFloat);
	add_operator(OperatorAssign,		TypeVoid,		TypeString,		TypeString);
	add_operator(OperatorAssign,		TypeVoid,		TypeString,		TypeStringP);
	add_operator(OperatorAdd,			TypeString,		TypeString,		TypeString);
	add_operator(OperatorAdd,			TypeString,		TypeString,		TypeStringP);
	add_operator(OperatorAdd,			TypeString,		TypeStringP,	TypeString);
	add_operator(OperatorAdd,			TypeString,		TypeStringP,	TypeStringP);
	add_operator(OperatorAddS,			TypeVoid,		TypeString,		TypeString);
	add_operator(OperatorAddS,			TypeVoid,		TypeString,		TypeStringP);
	add_operator(OperatorEqual,			TypeBool,		TypeString,		TypeString);
	add_operator(OperatorEqual,			TypeBool,		TypeStringP,	TypeString);
	add_operator(OperatorEqual,			TypeBool,		TypeString,		TypeStringP);
	add_operator(OperatorEqual,			TypeBool,		TypeStringP,	TypeStringP);
	add_operator(OperatorNotEqual,		TypeBool,		TypeString,		TypeString);
	add_operator(OperatorNotEqual,		TypeBool,		TypeStringP,	TypeString);
	add_operator(OperatorNotEqual,		TypeBool,		TypeString,		TypeStringP);
	add_operator(OperatorNotEqual,		TypeBool,		TypeStringP,	TypeStringP);
	add_operator(OperatorAssign,		TypeVoid,		TypeStruct,		TypeStruct);
	add_operator(OperatorEqual,			TypeBool,		TypeStruct,		TypeStruct);
	add_operator(OperatorNotEqual,		TypeBool,		TypeStruct,		TypeStruct);
	add_operator(OperatorAdd,			TypeVector,		TypeVector,		TypeVector);
	add_operator(OperatorSubtract,		TypeVector,		TypeVector,		TypeVector);
	add_operator(OperatorMultiply,		TypeVector,		TypeVector,		TypeVector);
	add_operator(OperatorMultiply,		TypeVector,		TypeVector,		TypeFloat);
	add_operator(OperatorMultiply,		TypeVector,		TypeFloat,		TypeVector);
	add_operator(OperatorDivide,		TypeVector,		TypeVector,		TypeVector);
	add_operator(OperatorDivide,		TypeVector,		TypeVector,		TypeFloat);
	add_operator(OperatorAddS,			TypeVoid,		TypeVector,		TypeVector);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeVector,		TypeVector);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeVector,		TypeFloat);
	add_operator(OperatorDivideS,		TypeVoid,		TypeVector,		TypeFloat);
	add_operator(OperatorSubtract,		TypeVector,		TypeVoid,		TypeVector);












	msg_db_m("const", 1);

	add_const("nil", TypePointer, NULL);
	// bool
	add_const("false",TypeBool,(void*)false);
	add_const("true",TypeBool,(void*)true);
	// float
	add_const("pi",TypeFloat,*(void**)&pi);
	// vector
	add_const("v0",TypeVector,(void*)&v0);
	add_const("e_x",TypeVector,(void*)&e_x);
	add_const("e_y",TypeVector,(void*)&e_y);
	add_const("e_z",TypeVector,(void*)&e_z);
	// matrix
	add_const("m_id",TypeMatrix,(void*)&m_id);
	// quaternion
	add_const("q_id",TypeVector,(void*)&q_id);
	// color
	add_const("White",TypeColor,(void*)&White);
	add_const("Black",TypeColor,(void*)&Black);
	add_const("Gray",TypeColor,(void*)&Gray);
	add_const("Red",TypeColor,(void*)&Red);
	add_const("Green",TypeColor,(void*)&Green);
	add_const("Blue",TypeColor,(void*)&Blue);
	add_const("Yellow",TypeColor,(void*)&Yellow);
	add_const("Orange",TypeColor,(void*)&Orange);
	// rect
	add_const("r01",TypeRect,(void*)&r01);
	// key ids (int)
	add_const("KEY_LCONTROL",TypeInt,(void*)KEY_LCONTROL);
	add_const("KEY_RCONTROL",TypeInt,(void*)KEY_RCONTROL);
	add_const("KEY_LSHIFT",TypeInt,(void*)KEY_LSHIFT);
	add_const("KEY_RSHIFT",TypeInt,(void*)KEY_RSHIFT);
	add_const("KEY_LALT",TypeInt,(void*)KEY_LALT);
	add_const("KEY_RALT",TypeInt,(void*)KEY_RALT);
	add_const("KEY_ADD",TypeInt,(void*)KEY_ADD);
	add_const("KEY_SUBTRACT",TypeInt,(void*)KEY_SUBTRACT);
	add_const("KEY_FENCE",TypeInt,(void*)KEY_FENCE);
	add_const("KEY_END",TypeInt,(void*)KEY_END);
	add_const("KEY_NEXT",TypeInt,(void*)KEY_NEXT);
	add_const("KEY_PRIOR",TypeInt,(void*)KEY_PRIOR);
	add_const("KEY_UP",TypeInt,(void*)KEY_UP);
	add_const("KEY_DOWN",TypeInt,(void*)KEY_DOWN);
	add_const("KEY_LEFT",TypeInt,(void*)KEY_LEFT);
	add_const("KEY_RIGHT",TypeInt,(void*)KEY_RIGHT);
	add_const("KEY_RETURN",TypeInt,(void*)KEY_RETURN);
	add_const("KEY_ESCAPE",TypeInt,(void*)KEY_ESCAPE);
	add_const("KEY_INSERT",TypeInt,(void*)KEY_INSERT);
	add_const("KEY_DELETE",TypeInt,(void*)KEY_DELETE);
	add_const("KEY_SPACE",TypeInt,(void*)KEY_SPACE);
	add_const("KEY_F1",TypeInt,(void*)KEY_F1);
	add_const("KEY_F2",TypeInt,(void*)KEY_F2);
	add_const("KEY_F3",TypeInt,(void*)KEY_F3);
	add_const("KEY_F4",TypeInt,(void*)KEY_F4);
	add_const("KEY_F5",TypeInt,(void*)KEY_F5);
	add_const("KEY_F6",TypeInt,(void*)KEY_F6);
	add_const("KEY_F7",TypeInt,(void*)KEY_F7);
	add_const("KEY_F8",TypeInt,(void*)KEY_F8);
	add_const("KEY_F9",TypeInt,(void*)KEY_F9);
	add_const("KEY_F10",TypeInt,(void*)KEY_F10);
	add_const("KEY_F11",TypeInt,(void*)KEY_F11);
	add_const("KEY_F12",TypeInt,(void*)KEY_F12);
	add_const("KEY_0",TypeInt,(void*)KEY_0);
	add_const("KEY_1",TypeInt,(void*)KEY_1);
	add_const("KEY_2",TypeInt,(void*)KEY_2);
	add_const("KEY_3",TypeInt,(void*)KEY_3);
	add_const("KEY_4",TypeInt,(void*)KEY_4);
	add_const("KEY_5",TypeInt,(void*)KEY_5);
	add_const("KEY_6",TypeInt,(void*)KEY_6);
	add_const("KEY_7",TypeInt,(void*)KEY_7);
	add_const("KEY_8",TypeInt,(void*)KEY_8);
	add_const("KEY_9",TypeInt,(void*)KEY_9);
	add_const("KEY_A",TypeInt,(void*)KEY_A);
	add_const("KEY_B",TypeInt,(void*)KEY_B);
	add_const("KEY_C",TypeInt,(void*)KEY_C);
	add_const("KEY_D",TypeInt,(void*)KEY_D);
	add_const("KEY_E",TypeInt,(void*)KEY_E);
	add_const("KEY_F",TypeInt,(void*)KEY_F);
	add_const("KEY_G",TypeInt,(void*)KEY_G);
	add_const("KEY_H",TypeInt,(void*)KEY_H);
	add_const("KEY_I",TypeInt,(void*)KEY_I);
	add_const("KEY_J",TypeInt,(void*)KEY_J);
	add_const("KEY_K",TypeInt,(void*)KEY_K);
	add_const("KEY_L",TypeInt,(void*)KEY_L);
	add_const("KEY_M",TypeInt,(void*)KEY_M);
	add_const("KEY_N",TypeInt,(void*)KEY_N);
	add_const("KEY_O",TypeInt,(void*)KEY_O);
	add_const("KEY_P",TypeInt,(void*)KEY_P);
	add_const("KEY_Q",TypeInt,(void*)KEY_Q);
	add_const("KEY_R",TypeInt,(void*)KEY_R);
	add_const("KEY_S",TypeInt,(void*)KEY_S);
	add_const("KEY_T",TypeInt,(void*)KEY_T);
	add_const("KEY_U",TypeInt,(void*)KEY_U);
	add_const("KEY_V",TypeInt,(void*)KEY_V);
	add_const("KEY_W",TypeInt,(void*)KEY_W);
	add_const("KEY_X",TypeInt,(void*)KEY_X);
	add_const("KEY_Y",TypeInt,(void*)KEY_Y);
	add_const("KEY_Z",TypeInt,(void*)KEY_Z);
	add_const("KEY_BACKSPACE",TypeInt,(void*)KEY_BACKSPACE);
	add_const("KEY_TAB",TypeInt,(void*)KEY_TAB);
	add_const("KEY_HOME",TypeInt,(void*)KEY_HOME);
	add_const("KEY_NUM_0",TypeInt,(void*)KEY_NUM_0);
	add_const("KEY_NUM_1",TypeInt,(void*)KEY_NUM_1);
	add_const("KEY_NUM_2",TypeInt,(void*)KEY_NUM_2);
	add_const("KEY_NUM_3",TypeInt,(void*)KEY_NUM_3);
	add_const("KEY_NUM_4",TypeInt,(void*)KEY_NUM_4);
	add_const("KEY_NUM_5",TypeInt,(void*)KEY_NUM_5);
	add_const("KEY_NUM_6",TypeInt,(void*)KEY_NUM_6);
	add_const("KEY_NUM_7",TypeInt,(void*)KEY_NUM_7);
	add_const("KEY_NUM_8",TypeInt,(void*)KEY_NUM_8);
	add_const("KEY_NUM_9",TypeInt,(void*)KEY_NUM_9);
	add_const("KEY_NUM_ADD",TypeInt,(void*)KEY_NUM_ADD);
	add_const("KEY_NUM_SUBTRACT",TypeInt,(void*)KEY_NUM_SUBTRACT);
	add_const("KEY_NUM_MULTIPLY",TypeInt,(void*)KEY_NUM_MULTIPLY);
	add_const("KEY_NUM_DIVIDE",TypeInt,(void*)KEY_NUM_DIVIDE);
	add_const("KEY_NUM_COMMA",TypeInt,(void*)KEY_NUM_COMMA);
	add_const("KEY_NUM_ENTER",TypeInt,(void*)KEY_NUM_ENTER);
	add_const("KEY_COMMA",TypeInt,(void*)KEY_COMMA);
	add_const("KEY_DOT",TypeInt,(void*)KEY_DOT);
	add_const("KEY_SMALLER",TypeInt,(void*)KEY_SMALLER);
	add_const("KEY_SZ",TypeInt,(void*)KEY_SZ);
	add_const("KEY_AE",TypeInt,(void*)KEY_AE);
	add_const("KEY_OE",TypeInt,(void*)KEY_OE);
	add_const("KEY_UE",TypeInt,(void*)KEY_UE);
	add_const("NUM_KEYS",TypeInt,(void*)HUI_NUM_KEYS);
	add_const("KEY_ANY",TypeInt,(void*)KEY_ANY);
	// alpha operations
	add_const("AlphaNone",TypeInt,(void*)AlphaNone);
	add_const("AlphaZero",TypeInt,(void*)AlphaZero);
	add_const("AlphaOne",TypeInt,(void*)AlphaOne);
	add_const("AlphaColorKey",TypeInt,(void*)AlphaColorKey);
	add_const("AlphaColorKeyHard",TypeInt,(void*)AlphaColorKeyHard);
	add_const("AlphaAdd",TypeInt,(void*)AlphaAdd);
	add_const("AlphaMaterial",TypeInt,(void*)AlphaMaterial);
	add_const("AlphaSourceColor",TypeInt,(void*)AlphaSourceColor);
	add_const("AlphaSourceInvColor",TypeInt,(void*)AlphaSourceInvColor);
	add_const("AlphaSourceAlpha",TypeInt,(void*)AlphaSourceAlpha);
	add_const("AlphaSourceInvAlpha",TypeInt,(void*)AlphaSourceInvAlpha);
	add_const("AlphaDestColor",TypeInt,(void*)AlphaDestColor);
	add_const("AlphaDestInvColor",TypeInt,(void*)AlphaDestInvColor);
	add_const("AlphaDestAlpha",TypeInt,(void*)AlphaDestAlpha);
	add_const("AlphaDestInvAlpha",TypeInt,(void*)AlphaDestInvAlpha);
	// stencil operations
	add_const("StencilNone",TypeInt,(void*)StencilNone);
	add_const("StencilIncrease",TypeInt,(void*)StencilIncrease);
	add_const("StencilDecrease",TypeInt,(void*)StencilDecrease);
	add_const("StencilSet",TypeInt,(void*)StencilSet);
	add_const("StencilMaskEqual",TypeInt,(void*)StencilMaskEqual);
	add_const("StencilMaskNotEqual",TypeInt,(void*)StencilMaskNotEqual);
	add_const("StencilMaskLess",TypeInt,(void*)StencilMaskLess);
	add_const("StencilMaskLessEqual",TypeInt,(void*)StencilMaskLessEqual);
	add_const("StencilMaskGreater",TypeInt,(void*)StencilMaskGreater);
	add_const("StencilMaskGreaterEqual",TypeInt,(void*)StencilMaskGreaterEqual);
	add_const("StencilReset",TypeInt,(void*)StencilReset);
	// fog
	add_const("FogLinear",TypeInt,(void*)FogLinear);
	add_const("FogExp",TypeInt,(void*)FogExp);
	add_const("FogExp2",TypeInt,(void*)FogExp2);
	// model skins
	add_const("SkinHigh",TypeInt,god_p(SkinHigh));
	add_const("SkinViewMedium",TypeInt,god_p(SkinMedium));
	add_const("SkinLow",TypeInt,god_p(SkinLow));
	// trace
	add_const("TraceModeSimpleTest",TypeInt,god_p(TraceModeSimpleTest));
	add_const("TraceHitTerrain",TypeInt,god_p(TraceHitTerrain));
	add_const("TraceHitObject",TypeInt,god_p(TraceHitObject));
	// file date
	add_const("FileDateModification",TypeInt,(void*)FileDateModification);
	add_const("FileDateAccess",TypeInt,(void*)FileDateAccess);
	add_const("FileDateCreation",TypeInt,(void*)FileDateCreation);
	// hui window messages
	add_const("HUI_WIN_CLOSE",TypeInt,(void*)HUI_WIN_CLOSE);
	add_const("HUI_WIN_SIZE",TypeInt,(void*)HUI_WIN_SIZE);
	add_const("HUI_WIN_MOVE",TypeInt,(void*)HUI_WIN_MOVE);
	add_const("HUI_WIN_RENDER",TypeInt,(void*)HUI_WIN_RENDER);
	add_const("HUI_WIN_MOUSEMOVE",TypeInt,(void*)HUI_WIN_MOUSEMOVE);
	add_const("HUI_WIN_MOUSEWHEEL",TypeInt,(void*)HUI_WIN_MOUSEWHEEL);
	add_const("HUI_WIN_LBUTTONDOWN",TypeInt,(void*)HUI_WIN_LBUTTONDOWN);
	add_const("HUI_WIN_LBUTTONUP",TypeInt,(void*)HUI_WIN_LBUTTONUP);
	add_const("HUI_WIN_RBUTTONDOWN",TypeInt,(void*)HUI_WIN_RBUTTONDOWN);
	add_const("HUI_WIN_RBUTTONUP",TypeInt,(void*)HUI_WIN_RBUTTONUP);
	add_const("HUI_WIN_KEYDOWN",TypeInt,(void*)HUI_WIN_KEYDOWN);
	add_const("HUI_WIN_KEYUP",TypeInt,(void*)HUI_WIN_KEYUP);
	// hui answers
	add_const("HuiYes",TypeInt,(void*)HUI_YES);
	add_const("HuiNo",TypeInt,(void*)HUI_NO);
	add_const("HuiCancel",TypeInt,(void*)HUI_CANCEL);
	// engine presettings
	add_const("NIX_API_NONE",TypeInt,(void*)NIX_API_NONE);
#ifdef NIX_API_OPENGL
	add_const("NIX_API_OPENGL",TypeInt,(void*)NIX_API_OPENGL);
#else
	add_const("NIX_API_OPENGL",TypeInt,(void*)-1);
#endif
#ifdef NIX_API_DIRECTX9
	add_const("NIX_API_DIRECTX9",TypeInt,(void*)NIX_API_DIRECTX9);
#else
	add_const("NIX_API_DIRECTX9",TypeInt,(void*)-1);
#endif
#ifdef NIX_OS_WINDOWS
	add_const("NIX_OS_WINDOWS",TypeInt,(void*)1);
#else
	add_const("NIX_OS_WINDOWS",TypeInt,(void*)-1);
#endif
#ifdef NIX_OS_LINUX
	add_const("NIX_OS_LINUX",TypeInt,(void*)1);
#else
	add_const("NIX_OS_LINUX",TypeInt,(void*)-1);
#endif
	add_const("VBTemp",TypeInt,(void*)VBTemp);
	add_const("NixVersion",TypeString,(void*)&NixVersion);
	// animation operations
	add_const("MoveOpSet",TypeInt,god_p(MoveOpSet));
	add_const("MoveOpSetNewKeyed",TypeInt,god_p(MoveOpSetNewKeyed));
	add_const("MoveOpSetOldKeyed",TypeInt,god_p(MoveOpSetOldKeyed));
	add_const("MoveOpAdd1Factor",TypeInt,god_p(MoveOpAdd1Factor));
	add_const("MoveOpMix1Factor",TypeInt,god_p(MoveOpMix1Factor));
	add_const("MoveOpMix2Factor",TypeInt,god_p(MoveOpMix2Factor));


msg_db_m("ext_var", 1);


// Spiele-Variablen, die von Programmstart an existieren
	add_ext_var("AppName",			TypeString,		god_p(&AppName));
	add_ext_var("AppFilename",		TypeString,		&HuiAppFilename);
	add_ext_var("AppDirectory",		TypeString,		&HuiAppDirectory);
	add_ext_var("elapsed",			TypeFloat,		meta_p(&Elapsed));
	add_ext_var("elapsed_rt",		TypeFloat,		meta_p(&ElapsedRT));
	add_ext_var("TimeScale",		TypeFloat,		meta_p(&TimeScale));
	add_ext_var("TargetWidth",		TypeInt,		&NixTargetWidth);
	add_ext_var("TargetHeight",		TypeInt,		&NixTargetHeight);
	add_ext_var("ScreenWidth",		TypeInt,		&NixScreenWidth);
	add_ext_var("ScreenHeight",		TypeInt,		&NixScreenHeight);
	add_ext_var("ScreenDepth",		TypeInt,		&NixScreenDepth);
	add_ext_var("Api",				TypeInt,		&NixApi);
	add_ext_var("MinDepth",			TypeFloat,		&NixMinDepth);
	add_ext_var("MaxDepth",			TypeFloat,		&NixMaxDepth);
	add_ext_var("TextureLifeTime",	TypeInt,		&NixTextureMaxFramesToLive);
	add_ext_var("InitialWorldFile", TypeString,		god_p(&InitialWorldFile));
	add_ext_var("CurrentWorldFile", TypeString,		god_p(&CurrentWorldFile));
	add_ext_var("SecondWorldFile",	TypeString,		god_p(&SecondWorldFile));
	add_ext_var("NumObjects",		TypeInt,		god_p(&_NumObjects_));
	add_ext_var("Object",			TypeObjectPP,	god_p(&_ObjectList_));
	add_ext_var("ego",				TypeObjectP,	god_p(&ego));
	add_ext_var("NumTerrains",		TypeInt,		god_p(&NumTerrains));
	add_ext_var("Terrain",			TypeTerrainPList,god_p(&terrain));
	add_ext_var("Gravitation",		TypeVector,		god_p(&GlobalG));
	add_ext_var("Cam",				TypeViewP,		cam_p(&cam));
	add_ext_var("SkyBox",			TypeModelPP,	god_p(&pSkyBox));
	add_ext_var("SkyBoxAng",		TypeVectorP,	god_p(&pSkyBoxAng));
	add_ext_var("BackGroundColor",	TypeColor,		god_p(&BackGroundColor));
	add_ext_var("Fog",				TypeFog,		god_p(&GlobalFog));
	add_ext_var("ScriptVar",		TypeFloatP,		god_p(&ScriptVar));
	add_ext_var("Ambient",			TypeColor,		god_p(&GlobalAmbient));
	add_ext_var("SunLight",			TypeInt,		god_p(&SunLight));
	add_ext_var("TraceHitType",		TypeInt,		god_p(&TraceHitType));
	add_ext_var("TraceHitIndex",	TypeInt,		god_p(&TraceHitIndex));
	add_ext_var("TraceHitSubModel", TypeInt,		god_p(&TraceHitSubModel));
	add_ext_var("SuperGravitation", TypeBool,		god_p(&SuperGravitation));
	add_ext_var("CurrentGrouping",	TypeGroupingP,	gui_p(&CurrentGrouping));
	add_ext_var("MirrorLevelMax",	TypeInt,		god_p(&MirrorLevelMax));
	add_ext_var("SessionName",		TypeString,		meta_p(&SessionName));
	add_ext_var("HostNames",		TypeString,		meta_p(&HostNames));
	add_ext_var("NumAvailableHosts",TypeInt,		meta_p(&NumAvailableHosts));
	add_ext_var("HostName",			TypeStringList,	meta_p(&HostName));
	add_ext_var("HostSessionName",	TypeStringList,	meta_p(&HostSessionName));
	add_ext_var("NetIAmHost",		TypeBool,		meta_p(&NetIAmHost));
	add_ext_var("NetIAmClient",		 TypeBool,		meta_p(&NetIAmClient));
	add_ext_var("NetRead",			TypeBool,		meta_p(&NetRead));
	add_ext_var("NetWritten",		TypeBool,		meta_p(&NetWritten));
	add_ext_var("NumClients",		TypeInt,		meta_p(&NumClients));
	add_ext_var("SocketToServer",	TypeInt,		meta_p(&SocketToServer));
	add_ext_var("SocketToClient",	TypeIntList,	meta_p(&SocketToClient));
	add_ext_var("CurrentSocket",	TypeInt,		meta_p(&CurrentSocket));
	add_ext_var("Debug",			TypeBool,		meta_p(&Debug));
	add_ext_var("ShowTimings",		TypeBool,		meta_p(&ShowTimings));
	add_ext_var("WireMode",			TypeBool,		meta_p(&WireMode));
	add_ext_var("ConsoleEnabled",	TypeBool,		meta_p(&ConsoleEnabled));
	add_ext_var("Record",			TypeBool,		meta_p(&Record));
	add_ext_var("ShadowLevel",		TypeInt,		meta_p(&ShadowLevel));
	add_ext_var("ShadowLowerDetail",TypeBool,		meta_p(&ShadowLowerDetail));
	add_ext_var("ShadowLight",		TypeInt,		meta_p(&ShadowLight));
	add_ext_var("ShadowColor",		TypeColor,		meta_p(&ShadowColor));
	add_ext_var("FpsMax",			TypeFloat,		meta_p(&FpsMax));
	add_ext_var("FpsMin",			TypeFloat,		meta_p(&FpsMin));
	add_ext_var("DetailLevel",		TypeInt,		meta_p(&DetailLevel));
	add_ext_var("DetailFactorInv",	TypeFloat,		meta_p(&DetailFactorInv));
//	add_ext_var("VolumeMusic",		TypeFloat,		NULL);
//	add_ext_var("VolumeSounds",		TypeFloat,		NULL);
	add_ext_var("NetworkEnabled",	TypeBool,		meta_p(&NetworkEnabled));
	add_ext_var("XFontColor",		TypeColor,		meta_p(&XFontColor));
	add_ext_var("XFontIndex",		TypeInt,		meta_p(&XFontIndex));
	add_ext_var("DefaultFont",		TypeInt,		meta_p(&DefaultFont));
	add_ext_var("dir_search_num",	TypeInt,		&dir_search_num);
	add_ext_var("dir_search_name",	TypeStringPList,&dir_search_name_p);
	add_ext_var("dir_search_is_dir",TypeBoolList,	&dir_search_is_dir);
	add_ext_var("HuiFileDialogPath",TypeString,		&HuiFileDialogPath);
	add_ext_var("HuiFileDialogFile",TypeString,		&HuiFileDialogFile);
	add_ext_var("HuiFileDialogCompleteName",TypeString,&HuiFileDialogCompleteName);
	add_ext_var("HuiRunning",		TypeBool,		&HuiRunning);

	//NumTruePreExternalVars = PreExternalVar.size();

	msg_db_m("cmd", 1);

/*
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
*/


// "intern" functions
	add_func("return",		TypeVoid,	f_cp);
		func_add_param("return_value",	TypeVoid); // return: ParamType will be defined by the parser!
	add_func("-if-",		TypeVoid,	f_cp);
		func_add_param("b",	TypeBool);
	add_func("-if/else-",	TypeVoid,	f_cp);
		func_add_param("b",	TypeBool);
	add_func("-while-",		TypeVoid,	f_cp);
		func_add_param("b",	TypeBool);
	add_func("-for-",		TypeVoid,	f_cp);
	add_func("-break-",		TypeVoid,	f_cp);
	add_func("-continue-",	TypeVoid,	f_cp);
	add_func("sizeof",		TypeInt,	f_cp);
		func_add_param("type",	TypeVoid);
	
	add_func("wait",		TypeVoid,	f_cp);
		func_add_param("time",	TypeFloat);
	add_func("wait_rt",		TypeVoid,	f_cp);
		func_add_param("time",	TypeFloat);
	add_func("wait_of",		TypeVoid,	f_cp);
	add_func("f2i",			TypeInt,	(void*)&_Float2Int);
		func_add_param("f",		TypeFloat);
	add_func("i2f",			TypeFloat,	f_cp);
		func_add_param("i",		TypeInt);
	add_func("i2c",			TypeChar,	f_cp);
		func_add_param("i",		TypeInt);
	add_func("c2i",			TypeInt,	f_cp);
		func_add_param("c",		TypeChar);
	add_func("p2b",			TypeBool,	f_cp);
		func_add_param("p",		TypePointer);
	add_func("vector",		TypeVector,	f_cp);
		func_add_param("x",		TypeFloat);
		func_add_param("y",		TypeFloat);
		func_add_param("z",		TypeFloat);
	add_func("rect",		TypeRect,	f_cp);
		func_add_param("x1",	TypeFloat);
		func_add_param("x2",	TypeFloat);
		func_add_param("y1",	TypeFloat);
		func_add_param("y2",	TypeFloat);
	add_func("color",		TypeColor,	f_cp);
		func_add_param("a",		TypeFloat);
		func_add_param("r",		TypeFloat);
		func_add_param("g",		TypeFloat);
		func_add_param("b",		TypeFloat);
	add_func("-asm-",		TypeVoid,	f_cp);

	// mathematical
	add_func("sin",			TypeFloat,	(void*)&f_sin);
		func_add_param("x",		TypeFloat);
	add_func("cos",			TypeFloat,	(void*)&f_cos);
		func_add_param("x",		TypeFloat);
	add_func("tan",			TypeFloat,	(void*)&f_tan);
		func_add_param("x",		TypeFloat);
	add_func("asin",		TypeFloat,	(void*)&f_asin);
		func_add_param("x",		TypeFloat);
	add_func("acos",		TypeFloat,	(void*)&f_acos);
		func_add_param("x",		TypeFloat);
	add_func("atan",		TypeFloat,	(void*)&f_atan);
		func_add_param("x",		TypeFloat);
	add_func("atan2",		TypeFloat,	(void*)&f_atan2);
		func_add_param("x",		TypeFloat);
		func_add_param("y",		TypeFloat);
	add_func("sqrt",		TypeFloat,	(void*)&f_sqrt);
		func_add_param("x",		TypeFloat);
	add_func("sqr",			TypeFloat,		(void*)&f_sqr);
		func_add_param("x",		TypeFloat);
	add_func("exp",			TypeFloat,		(void*)&f_exp);
		func_add_param("x",		TypeFloat);
	add_func("pow",			TypeFloat,		(void*)&f_pow);
		func_add_param("x",		TypeFloat);
		func_add_param("exp",	TypeFloat);
	add_func("clampf",		TypeVoid,		(void*)&clampf);
		func_add_param("f",		TypeFloatPs);
		func_add_param("min",	TypeFloat);
		func_add_param("max",	TypeFloat);
	add_func("loopf",		TypeVoid,		(void*)&loopf);
		func_add_param("f",		TypeFloatPs);
		func_add_param("min",	TypeFloat);
		func_add_param("max",	TypeFloat);
	add_func("absf",		TypeFloat,		(void*)&f_abs);
		func_add_param("f",		TypeFloat);
	// int
	add_func("clampi",		TypeVoid,		(void*)&clampi);
		func_add_param("i",		TypeIntPs);
		func_add_param("min",	TypeInt);
		func_add_param("max",	TypeInt);
	add_func("loopi",		TypeVoid,		(void*)&loopi);
		func_add_param("i",		TypeIntPs);
		func_add_param("min",	TypeInt);
		func_add_param("max",	TypeInt);
	// type casting
	add_func("s2i",				TypeInt,		(void*)&s2i);
		func_add_param("s",		TypeStringP);
	add_func("s2f",				TypeFloat,		(void*)&s2f);
		func_add_param("s",		TypeStringP);
	add_func("i2s",				TypeStringP,	(void*)&i2s);
		func_add_param("i",		TypeInt);
	add_func("f2s",				TypeStringP,		(void*)&f2s);
		func_add_param("f",			TypeFloat);
		func_add_param("decimals",	TypeInt);
	// random numbers
	add_func("randi",			TypeInt,		(void*)&randi);
		func_add_param("max",	TypeInt);
	add_func("randf",			TypeFloat,		(void*)&randf);
		func_add_param("max",	TypeFloat);
	// debug output
	add_func("intout",			TypeVoid,		(void*)&intout);
		func_add_param("i",		TypeInt);
	add_func("boolout",			TypeVoid,		(void*)&boolout);
		func_add_param("b",		TypeBool);
	add_func("floatout",		TypeVoid,		(void*)&floatout);
		func_add_param("f",		TypeFloat);
	add_func("stringout",		TypeVoid,		(void*)&_stringout);
		func_add_param("str",	TypeStringP);
	add_func("pointerout",		TypeVoid,		(void*)&pointerout);
		func_add_param("p",		TypePointer);
	// vectors
	add_func("VecNormalize",		TypeVoid,	(void*)&VecNormalize);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVectorPs);
	add_func("VecDir2Ang",			TypeVector,	(void*)&VecDir2Ang);
		func_add_param("dir",		TypeVectorPs);
	add_func("VecDir2Ang2",			TypeVector,	(void*)&VecDir2Ang2);
		func_add_param("dir",		TypeVectorPs);
		func_add_param("up",		TypeVectorPs);
	add_func("VecAng2Dir",			TypeVector,	(void*)&VecAng2Dir);
		func_add_param("ang",		TypeVectorPs);
	add_func("VecAngAdd",			TypeVector,	(void*)&VecAngAdd);
		func_add_param("ang1",		TypeVectorPs);
		func_add_param("ang2",		TypeVectorPs);
	add_func("VecAngInterpolate",	TypeVector,	(void*)&VecAngInterpolate);
		func_add_param("ang1",		TypeVectorPs);
		func_add_param("ang2",		TypeVectorPs);
		func_add_param("t",			TypeFloat);
	add_func("VecLength",			TypeFloat,	(void*)&VecLength);
		func_add_param("v",			TypeVectorPs);
	add_func("VecLengthSqr",		TypeFloat,	(void*)&VecLengthSqr);
		func_add_param("v",			TypeVectorPs);
	add_func("VecTransform",		TypeVoid,	(void*)&VecTransform);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("m",			TypeMatrixPs);
		func_add_param("v_in",		TypeVectorPs);
	add_func("VecNormalTransform",	TypeVoid,	(void*)&VecNormalTransform);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("m",			TypeMatrixPs);
		func_add_param("v_in",		TypeVectorPs);
	add_func("VecDotProduct",		TypeFloat,	(void*)&VecDotProduct);
		func_add_param("v1",		TypeVectorPs);
		func_add_param("v2",		TypeVectorPs);
	add_func("VecCrossProduct",		TypeVector,	(void*)&VecCrossProduct);
		func_add_param("v1",		TypeVectorPs);
		func_add_param("v2",		TypeVectorPs);
	// matrices
	add_func("MatrixIdentity",		TypeVoid,	(void*)&MatrixIdentity);
		func_add_param("m_out",		TypeMatrixPs);
	add_func("MatrixTranslation",	TypeVoid,	(void*)&MatrixTranslation);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("trans",		TypeVectorPs);
	add_func("MatrixRotation",		TypeVoid,	(void*)&MatrixRotation);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("ang",		TypeVectorPs);
	add_func("MatrixRotationX",		TypeVoid,	(void*)&MatrixRotationX);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("ang",		TypeFloat);
	add_func("MatrixRotationY",		TypeVoid,	(void*)&MatrixRotationY);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("ang",		TypeFloat);
	add_func("MatrixRotationZ",		TypeVoid,	(void*)&MatrixRotationZ);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("ang",		TypeFloat);
	add_func("MatrixRotationQ",		TypeVoid,	(void*)&MatrixRotationQ);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("ang",		TypeQuaternionPs);
	add_func("MatrixRotationView",	TypeVoid,	(void*)&MatrixRotationView);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("ang",		TypeVectorPs);
	add_func("MatrixScale",			TypeVoid,	(void*)&MatrixScale);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("s_x",		TypeFloat);
		func_add_param("s_y",		TypeFloat);
		func_add_param("s_z",		TypeFloat);
	add_func("MatrixMultiply",		TypeVoid,	(void*)&MatrixMultiply);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("m2",		TypeMatrixPs);
		func_add_param("m1",		TypeMatrixPs);
	add_func("MatrixInverse",		TypeVoid,	(void*)&MatrixInverse);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("m_in",		TypeMatrixPs);
	// quaternions
	add_func("QuaternionRotationV",	TypeVoid,	(void*)&QuaternionRotationV);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("ang",		TypeVectorPs);
	add_func("QuaternionRotationA",	TypeVoid,	(void*)&QuaternionRotationA);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("axis",		TypeVectorPs);
		func_add_param("w",		TypeFloat);
	add_func("QuaternionMultiply",	TypeVoid,	(void*)&QuaternionMultiply);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("q2",		TypeQuaternionPs);
		func_add_param("q1",		TypeQuaternionPs);
	add_func("QuaternionInverse",	TypeVoid,	(void*)&QuaternionInverse);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("q_in",		TypeQuaternionPs);
	add_func("QuaternionScale",		TypeVoid,	(void*)&QuaternionScale);
		func_add_param("q",		TypeQuaternionPs);
		func_add_param("f",		TypeFloat);
	add_func("QuaternionNormalize",	TypeVoid,	(void*)&QuaternionNormalize);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("q_in",		TypeQuaternionPs);
	add_func("QuaternionToAngle",	TypeVector,	(void*)&QuaternionToAngle);
		func_add_param("q",		TypeQuaternionPs);
	// other types
	add_func("SetColorHSB",			TypeColor,		(void*)&SetColorHSB);
		func_add_param("a",		TypeFloat);
		func_add_param("h",		TypeFloat);
		func_add_param("s",		TypeFloat);
		func_add_param("p",		TypeFloat);
	add_func("ColorInterpolate",	TypeColor,		(void*)&ColorInterpolate);
		func_add_param("c1",		TypeColorPs);
		func_add_param("c2",		TypeColorPs);
		func_add_param("t",		TypeFloat);
	add_func("LoadTexture",			TypeInt,	meta_p(&MetaLoadTexture));
		func_add_param("filename",		TypeStringP);
	add_func("XFDrawStr",			TypeFloat,	meta_p(&XFDrawStr));
		func_add_param("x",		TypeFloat);
		func_add_param("y",		TypeFloat);
		func_add_param("size",		TypeFloat);
		func_add_param("s",		TypeStringP);
		func_add_param("centric",		TypeBool);
	add_func("XFDrawVertStr",		TypeFloat,	meta_p(&XFDrawVertStr));
		func_add_param("x",		TypeFloat);
		func_add_param("y",		TypeFloat);
		func_add_param("size",		TypeFloat);
		func_add_param("s",		TypeStringP);
	add_func("XFGetWidth",			TypeFloat,	meta_p(&XFGetWidth));
		func_add_param("size",		TypeFloat);
		func_add_param("s",		TypeStringP);
	add_func("LoadXFont",			TypeInt,	meta_p(&MetaLoadXFont));
		func_add_param("filename",		TypeStringP);
	add_func("CreatePicture",										TypePictureP,	gui_p(&GuiCreatePicture));
		func_add_param("pos",		TypeVector);
		func_add_param("width",		TypeFloat);
		func_add_param("height",		TypeFloat);
		func_add_param("texture",		TypeInt);
		func_add_param("source",		TypeRectPs);
		func_add_param("c",		TypeColorPs);
	add_func("CreatePicture3D",								TypePicture3DP,	gui_p(&GuiCreatePicture3D));
		func_add_param("m",		TypeModelP);
		func_add_param("mat",		TypeMatrixP);
		func_add_param("z",		TypeFloat);
	add_func("CreateText",												TypeTextP,	gui_p(&GuiCreateText));
		func_add_param("pos",		TypeVector);
		func_add_param("size",		TypeFloat);
		func_add_param("c",		TypeColorPs);
		func_add_param("s",		TypeStringP);
	add_func("CreateGrouping",										TypeGroupingP,	gui_p(&GuiCreateGrouping));
		func_add_param("pos",		TypeVector);
		func_add_param("c",		TypeColorPs);
		func_add_param("sel_cur",		TypeBool);
	add_func("GuiMouseOver",										TypeBool,	gui_p(&GuiMouseOver));
		func_add_param("p",		TypePointer);
	add_func("CreateParticle",										TypeParticleP,	fx_p(&FxParticleCreateDef));
		func_add_param("pos",		TypeVectorPs);
		func_add_param("texture",		TypeInt);
		func_add_param("func",		TypePointer);
		func_add_param("life",		TypeFloat);
		func_add_param("radius",		TypeFloat);
	add_func("CreateParticleRot",								TypeParticleP,	fx_p(&FxParticleCreateRot));
		func_add_param("pos",		TypeVectorPs);
		func_add_param("ang",		TypeVectorPs);
		func_add_param("texture",		TypeInt);
		func_add_param("func",		TypePointer);
		func_add_param("life",		TypeFloat);
		func_add_param("radius",		TypeFloat);
	add_func("CreateBeam",										TypeBeamP,	fx_p(&FxParticleCreateBeam));
		func_add_param("pos",		TypeVectorPs);
		func_add_param("length",		TypeVectorPs);
		func_add_param("texture",		TypeInt);
		func_add_param("func",		TypePointer);
		func_add_param("life",		TypeFloat);
		func_add_param("radius",		TypeFloat);
	add_func("CreateEffect",										TypeEffectP,	fx_p(&FxCreate));
		func_add_param("pos",		TypeVectorPs);
		func_add_param("func",		TypePointer);
		func_add_param("del_func",		TypePointer);
		func_add_param("life",		TypeFloat);
	add_func("LoadModel",												TypeModelP,	meta_p(&MetaLoadModel));
		func_add_param("filename",		TypeStringP);
	add_func("LoadItem",												TypeItemP,	meta_p(&MetaLoadItem));
		func_add_param("filename",		TypeStringP);
	add_func("GetItemOID",												TypeInt,	meta_p(&MetaGetItemOID));
		func_add_param("filename",		TypeStringP);
	add_func("CreateView",												TypeViewP,	cam_p(&CameraCreateView));
		func_add_param("pos",		TypeVector);
		func_add_param("ang",		TypeVector);
		func_add_param("dest",		TypeRectPs);
		func_add_param("show",		TypeBool);
	add_func("LoadShaderFile",										TypeInt,	meta_p(&MetaLoadShaderFile));
		func_add_param("filename",		TypeStringP);
	add_func("ListCreate",												TypeInt,	meta_p(&MetaListCreate));
		func_add_param("item_size",		TypeInt);
		func_add_param("max_items",		TypeInt);
	add_func("ListDelete",												TypeVoid,	meta_p(&MetaListDelete));
		func_add_param("list",		TypeInt);
	add_func("ListGetItemCount",								TypeInt,	meta_p(&MetaListGetItemCount));
		func_add_param("list",		TypeInt);
	add_func("ListIterate",												TypeBool,	meta_p(&MetaListIterate));
		func_add_param("list",		TypeInt);
		func_add_param("item",		TypePointerPS);
	add_func("ListIterateBack",										TypeBool,	meta_p(&MetaListIterateBack));
		func_add_param("list",		TypeInt);
		func_add_param("item",		TypePointerPS);
	add_func("ListDeleteItem",										TypeVoid,	meta_p(&MetaListDeleteItem));
		func_add_param("list",		TypeInt);
		func_add_param("item",		TypePointerPS);
	add_func("ListAddItem",												TypePointer,	meta_p(&MetaListAddItem));
		func_add_param("list",		TypeInt);
	add_func("ListAddItemIndexed",								TypePointer,	meta_p(&MetaListAddItemIndexed));
		func_add_param("list",		TypeInt);
		func_add_param("index",		TypeIntPs);
	add_func("ListGetItemByIndex",								TypePointer,	meta_p(&MetaListGetItemByIndex));
		func_add_param("list",		TypeInt);
		func_add_param("index",		TypeInt);
	add_func("ListItemGetIndex",								TypeInt,	meta_p(&MetaListItemGetIndex));
		func_add_param("list",		TypeInt);
		func_add_param("item",		TypePointer);

	// engine
		// user input
	add_func("NixUpdateInput",						TypeVoid,	(void*)&NixUpdateInput);
	add_func("GetKey",								TypeBool,	(void*)&NixGetKey);
		func_add_param("id",		TypeInt);
	add_func("GetKeyDown",							TypeBool,	(void*)&NixGetKeyDown);
		func_add_param("id",		TypeInt);
	add_func("GetKeyRhythmDown",					TypeInt,	(void*)&NixGetKeyRhythmDown);
	add_func("GetKeyChar",							TypeStringP,	(void*)&NixGetKeyChar);
		func_add_param("id",		TypeInt);
	add_func("GetKeyUp",									TypeBool,	(void*)&NixGetKeyUp);
		func_add_param("id",		TypeInt);
	add_func("GetKeyName",									TypeStringP,	(void*)&HuiGetKeyName);
		func_add_param("id",		TypeInt);
	add_func("GetMouseX",									TypeFloat,	(void*)&NixGetMx);
	add_func("GetMouseY",									TypeFloat,	(void*)&NixGetMy);
	add_func("GetMouseDx",									TypeFloat,	(void*)&NixGetDx);
	add_func("GetMouseDy",									TypeFloat,	(void*)&NixGetDy);
	add_func("GetWheelD",									TypeFloat,	(void*)&NixGetWheelD);
	add_func("GetMouseRel",									TypeVector,	(void*)&NixGetMouseRel);
	add_func("GetButL",											TypeBool,	(void*)&NixGetButL);
	add_func("GetButM",											TypeBool,	(void*)&NixGetButM);
	add_func("GetButR",											TypeBool,	(void*)&NixGetButR);
	add_func("GetButLDown",									TypeBool,	(void*)&NixGetButLDown);
	add_func("GetButMDown",									TypeBool,	(void*)&NixGetButMDown);
	add_func("GetButRDown",									TypeBool,	(void*)&NixGetButRDown);
	add_func("GetButLUp",									TypeBool,	(void*)&NixGetButLUp);
	add_func("GetButMUp",									TypeBool,	(void*)&NixGetButMUp);
	add_func("GetButRUp",									TypeBool,	(void*)&NixGetButRUp);
		// drawing
	add_func("NixStart",									TypeVoid,	(void*)&NixStart);
		func_add_param("texture",		TypeInt);
	add_func("NixEnd",											TypeVoid,	(void*)&NixEnd);
	add_func("NixKillWindows",							TypeVoid,	(void*)&NixKillWindows);
	add_func("NixDraw2D",								TypeVoid,	(void*)&NixDraw2D);
		func_add_param("texture",		TypeInt);
		func_add_param("c",		TypeColorP);
		func_add_param("source",		TypeRectP);
		func_add_param("dest",		TypeRectP);
		func_add_param("z",		TypeFloat);
	add_func("NixDraw3D",								TypeVoid,	(void*)&NixDraw3D);
		func_add_param("texture",		TypeInt);
		func_add_param("vb",		TypeInt);
		func_add_param("m",		TypeMatrixP);
	add_func("NixDrawStr",									TypeVoid,	(void*)&NixDrawStr);
		func_add_param("x",		TypeInt);
		func_add_param("y",		TypeInt);
		func_add_param("s",		TypeStringP);
	add_func("NixDrawLineH",							TypeVoid,	(void*)&NixDrawLineH);
		func_add_param("x",		TypeInt);
		func_add_param("y1",		TypeInt);
		func_add_param("y2",		TypeInt);
		func_add_param("c",		TypeColor);
		func_add_param("z",		TypeFloat);
	add_func("NixDrawLineV",							TypeVoid,	(void*)&NixDrawLineV);
		func_add_param("x1",		TypeInt);
		func_add_param("x2",		TypeInt);
		func_add_param("y",		TypeInt);
		func_add_param("c",		TypeColor);
		func_add_param("z",		TypeFloat);
	add_func("NixDrawLine",									TypeVoid,	(void*)&NixDrawLine);
		func_add_param("x1",		TypeFloat);
		func_add_param("y1",		TypeFloat);
		func_add_param("x2",		TypeFloat);
		func_add_param("y2",		TypeFloat);
		func_add_param("c",		TypeColor);
		func_add_param("z",		TypeFloat);
	add_func("NixDrawSprite",							TypeVoid,	(void*)&NixDrawSprite);
		func_add_param("texture",		TypeInt);
		func_add_param("c",		TypeColorP);
		func_add_param("source",		TypeRectP);
		func_add_param("pos",		TypeVectorP);
		func_add_param("radius",		TypeFloat);
	//add_func("NixDrawModel2D",						TypeVoid);
	//	func_add_param("???",		TypeFloat); // ???
	add_func("NixSetAlphaM",								TypeVoid,	(void*)&NixSetAlphaM);
		func_add_param("mode",		TypeInt);
	add_func("NixSetAlphaSD",						TypeVoid,	(void*)&NixSetAlphaSD);
		func_add_param("source",		TypeInt);
		func_add_param("dest",		TypeInt);
	add_func("NixSetStencil",							TypeVoid,	(void*)&NixSetStencil);
		func_add_param("mode",		TypeInt);
		func_add_param("param",		TypeInt);
	add_func("NixSetViewM",									TypeVoid,	(void*)&NixSetViewM);
		func_add_param("enable3d",		TypeBool);
		func_add_param("view_mat",		TypeMatrixPs);
	add_func("NixSetViewV",								TypeVoid,	(void*)&NixSetViewV);
		func_add_param("enable3d",		TypeBool);
		func_add_param("pos",		TypeVectorPs);
		func_add_param("ang",		TypeVectorPs);
	add_func("NixSetZ",											TypeVoid,	(void*)&NixSetZ);
		func_add_param("write",		TypeBool);
		func_add_param("test",		TypeBool);
	add_func("NixSetMaterial",							TypeVoid,	(void*)&NixSetMaterial);
		func_add_param("ambient",		TypeColorPs);
		func_add_param("diffuse",		TypeColorPs);
		func_add_param("specular",		TypeColorPs);
		func_add_param("shininess",		TypeFloat);
		func_add_param("emission",		TypeColorPs);
	add_func("NixSetFontColor",							TypeVoid,	(void*)&NixSetFontColor);
		func_add_param("c",		TypeColor);
	add_func("NixEnableLighting",					TypeVoid,	(void*)&NixEnableLighting);
		func_add_param("enable",		TypeBool);
	add_func("NixSetVideoMode",							TypeVoid,	(void*)&NixSetVideoMode);
		func_add_param("api",		TypeInt);
		func_add_param("width",		TypeInt);
		func_add_param("height",		TypeInt);
		func_add_param("depth",		TypeInt);
		func_add_param("fullscreen",		TypeBool);
	add_func("NixCreateDynamicTexture",	TypeInt,	(void*)&NixCreateDynamicTexture);
		func_add_param("width",		TypeInt);
		func_add_param("height",		TypeInt);
	add_func("NixLoadTextureData",					TypeVoid,	(void*)&NixLoadTextureData);
		func_add_param("filename",		TypeStringP);
		func_add_param("image",		TypePointerPS);
		func_add_param("width",		TypeIntPs);
		func_add_param("height",		TypeIntPs);
	add_func("NixSaveTGA",									TypeVoid,	(void*)&NixSaveTGA);
		func_add_param("filename",		TypeStringP);
		func_add_param("width",		TypeInt);
		func_add_param("height",		TypeInt);
		func_add_param("bits",		TypeInt);
		func_add_param("bits_alpha",		TypeInt);
		func_add_param("image",		TypePointer);
		func_add_param("palette",		TypePointer);
	add_func("VecProject",								TypeVoid,	(void*)&NixGetVecProject);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVectorPs);
	add_func("VecUnproject",							TypeVoid,	(void*)&NixGetVecUnproject);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVectorPs);
	add_func("VecProjectRel",						TypeVoid,	(void*)&NixGetVecProjectRel);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVectorPs);
	add_func("VecUnprojectRel",						TypeVoid,	(void*)&NixGetVecUnprojectRel);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVectorPs);
	add_func("NixVBEmpty",									TypeVoid,	(void*)&NixVBEmpty);
		func_add_param("vb",		TypeInt);
	add_func("NixVBAddTrias",							TypeVoid,	(void*)&NixVBAddTrias);
		func_add_param("vb",		TypeInt);
		func_add_param("num_trias",		TypeInt);
		func_add_param("p",		TypeVectorP);
		func_add_param("n",		TypeVectorP);
		func_add_param("t",		TypeFloatP);
	add_func("NixSetShaderData",					TypeVoid,	(void*)&NixSetShaderData);
		func_add_param("index",		TypeInt);
		func_add_param("name",		TypeStringP);
		func_add_param("data",		TypePointer);
		func_add_param("size",		TypeInt);
	add_func("NixGetShaderData",					TypeVoid,	(void*)&NixGetShaderData);
		func_add_param("index",		TypeInt);
		func_add_param("name",		TypeStringP);
		func_add_param("data",		TypePointer);
		func_add_param("size",		TypeInt);
	// sound
	add_func("SoundEmit",									TypeVoid,	meta_p(&MetaSoundEmit));
		func_add_param("filename",		TypeStringP);
		func_add_param("pos",		TypeVectorPs);
		func_add_param("radius",		TypeFloat);
		func_add_param("speed",		TypeFloat);
	add_func("SoundCreate",									TypeInt,	meta_p(&MetaSoundManagedNew));
		func_add_param("filename",		TypeStringP);
	add_func("SoundSetData",							TypeVoid,	meta_p(&MetaSoundManagedSetData));
		func_add_param("index",		TypeInt);
		func_add_param("pos",		TypeVectorPs);
		func_add_param("vel",		TypeVectorPs);
		func_add_param("radius",		TypeFloat);
		func_add_param("speed",		TypeFloat);
		func_add_param("volume",		TypeFloat);
	add_func("SoundDelete",									TypeVoid,	meta_p(&MetaSoundManagedDelete));
		func_add_param("index",		TypeInt);
	// music
	add_func("MusicLoad",									TypeInt,	meta_p(&MetaMusicLoad));
		func_add_param("filename",		TypeStringP);
	add_func("MusicPlay",									TypeInt,	(void*)&NixMusicPlay);
		func_add_param("index",		TypeInt);
		func_add_param("loop",		TypeBool);
	add_func("MusicStop",									TypeInt,	(void*)&NixMusicStop);
		func_add_param("index",		TypeInt);
	add_func("MusicPause",									TypeInt,	(void*)&NixMusicSetPause);
		func_add_param("index",		TypeInt);
		func_add_param("pause",		TypeBool);
	add_func("MusicSetRate",							TypeInt,	(void*)&NixMusicSetRate);
		func_add_param("index",		TypeInt);
		func_add_param("rate",		TypeFloat);
	// effects
	add_func("FXLightCreate",							TypeInt,	fx_p(&FxLightCreate));
	add_func("FXLightSetDirectional",			TypeVoid,	fx_p(&FxLightSetDirectional));
		func_add_param("index",		TypeInt);
		func_add_param("dir",		TypeVectorPs);
		func_add_param("ambient",		TypeColorPs);
		func_add_param("diffuse",		TypeColorPs);
		func_add_param("specular",		TypeColorPs);
	add_func("FXLightSetRadial",					TypeVoid,	fx_p(&FxLightSetRadial));
		func_add_param("index",		TypeInt);
		func_add_param("pos",		TypeVectorPs);
		func_add_param("radius",		TypeFloat);
		func_add_param("ambient",		TypeColorPs);
		func_add_param("diffuse",		TypeColorPs);
		func_add_param("specular",		TypeColorPs);
	add_func("FXLightDelete",							TypeVoid,	fx_p(&FxLightDelete));
		func_add_param("index",		TypeInt);
	add_func("FXLightEnable",							TypeVoid,	fx_p(&FxLightEnable));
		func_add_param("index",		TypeInt);
		func_add_param("enabled",		TypeBool);
	// game
	add_func("ExitProgram",									TypeVoid,	meta_p(MetaExitProgram));
	add_func("ScreenShot",									TypeVoid,	meta_p(MetaScreenShot));
	add_func("FindHosts",									TypeVoid,	meta_p(MetaFindHosts));
	add_func("XDelete",											TypeVoid,	meta_p(&MetaDelete));
		func_add_param("p",		TypePointer);
	add_func("LoadWorld",									TypeVoid,	meta_p(&MetaLoadWorld));
		func_add_param("filename",		TypeStringP);
	add_func("LoadGameFromHost",					TypeVoid,	meta_p(MetaLoadGameFromHost));
		func_add_param("host",		TypeInt);
	add_func("SaveGameState",							TypeVoid,	meta_p(MetaSaveGameState));
		func_add_param("filename",		TypeStringP);
	add_func("LoadGameState",							TypeVoid,	meta_p(MetaLoadGameState));
		func_add_param("filename",		TypeStringP);
	add_func("GetObjectByName",							TypeObjectP,	god_p(&GetObjectByName));
		func_add_param("name",		TypeStringP);
	add_func("NextObject",									TypeBool,	god_p(&NextObject));
		func_add_param("o",		TypeObjectPP);
	add_func("CreateObject",							TypeObjectP,	god_p(&_CreateObject));
		func_add_param("filename",		TypeStringP);
		func_add_param("pos",		TypeVector);
	add_func("DeleteObject",							TypeVoid,	god_p(&_DeleteObject));
		func_add_param("id",		TypeInt);
	add_func("CamStartScript",							TypeVoid,	cam_p(&CameraStartScript));
		func_add_param("filename",		TypeStringP);
		func_add_param("view",		TypeViewP);
		func_add_param("dpos",		TypeVectorPs);
	add_func("CamStopScript",							TypeVoid,	cam_p(&CameraStopScript));
		func_add_param("view",		TypeViewP);
	add_func("DrawSplashScreen",					TypeVoid,	meta_p(MetaDrawSplashScreen));
		func_add_param("status",		TypeStringP);
		func_add_param("progress",		TypeFloat);
/*	add_func("ModelCalcMove",							TypeVoid,	mf( (tmf) &CModel::CalcMove ) );
		func_add_param("m",		TypeModelP);
	add_func("ModelDraw",									TypeVoid,	mf((tmf)&CModel::Draw));
		func_add_param("m",		TypeModelP);
		func_add_param("skin",		TypeInt);
		func_add_param("mat",		TypeMatrixPs);
		func_add_param("fx",		TypeBool);*/
	add_func("ModelGetVertex",			TypeVector,		(void*)&ModelGetVertex);
		func_add_param("m",		TypeModelP);
		func_add_param("index",		TypeInt);
		func_add_param("skin",		TypeInt);
	add_func("ModelMoveReset",							TypeVoid,	meta_p(&MetaModelMoveReset));
		func_add_param("m",		TypeModelP);
	add_func("ModelMoveSet",							TypeBool,	meta_p(&MetaModelMoveSet));
		func_add_param("m",		TypeModelP);
		func_add_param("operation",		TypeInt);
		func_add_param("param1",		TypeFloat);
		func_add_param("param2",		TypeFloat);
		func_add_param("move",		TypeInt);
		func_add_param("time",		TypeFloatPs);
		func_add_param("dt",		TypeFloat);
		func_add_param("v",		TypeFloat);
		func_add_param("loop",		TypeBool);
	add_func("ModelMoveGetFrames",					TypeInt,	meta_p(&MetaModelMoveGetFrames));
		func_add_param("m",		TypeModelP);
		func_add_param("move",		TypeInt);
	add_func("ModelMoveEditBegin",					TypeVoid,	meta_p(&MetaModelMoveEditBegin));
		func_add_param("m",		TypeModelP);
	add_func("ModelMakeEditable",					TypeVoid,	meta_p(&MetaModelMakeEditable));
		func_add_param("m",		TypeModelP);
	add_func("ModelEditBegin",							TypeVoid,	meta_p(&MetaModelEditBegin));
		func_add_param("m",		TypeModelP);
		func_add_param("skin",		TypeInt);
	add_func("ModelEditEnd",							TypeVoid,	meta_p(&MetaModelEditEnd));
		func_add_param("m",		TypeModelP);
		func_add_param("skin",		TypeInt);
	add_func("ModelSetBoneModel",					TypeVoid,	god_p(&GodModelSetBoneModel));
		func_add_param("m",		TypeModelP);
		func_add_param("index",		TypeInt);
		func_add_param("bone",		TypeModelP);
	add_func("TerrainUpdate",							TypeVoid,	god_p(&TerrainUpdate));
		func_add_param("t",		TypeTerrainP);
		func_add_param("x1",		TypeInt);
		func_add_param("x2",		TypeInt);
		func_add_param("z1",		TypeInt);
		func_add_param("z2",		TypeInt);
		func_add_param("mode",		TypeInt);
	add_func("RenderScene",									TypeVoid, 	NULL);
	add_func("GetG",											TypeVector,	god_p(&GetG));
		func_add_param("pos",		TypeVectorPs);
	add_func("Trace",											TypeBool,	god_p(&GodTrace));
		func_add_param("p1",		TypeVectorPs);
		func_add_param("p2",		TypeVectorPs);
		func_add_param("tp",		TypeVectorPs);
		func_add_param("mode",		TypeInt);
		func_add_param("o_ignore",		TypeInt);
	add_func("AddLinkSpring",							TypeInt,	god_p(&AddLinkSpring));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("rho1",		TypeVectorPs);
		func_add_param("rho2",		TypeVectorPs);
		func_add_param("x0",		TypeFloat);
		func_add_param("k",		TypeFloat);
	add_func("AddLinkHinge",							TypeInt,	god_p(&AddLinkHinge));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("rho1",		TypeVectorPs);
		func_add_param("rho2",		TypeVectorPs);
		func_add_param("ax1",		TypeVectorPs);
		func_add_param("ax2",		TypeVectorPs);
	add_func("AddLinkHingeAbs",							TypeInt,	god_p(&AddLinkHingeAbs));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("p",		TypeVectorPs);
		func_add_param("ax",		TypeVectorPs);
	add_func("AddLinkBall",									TypeInt,	god_p(&AddLinkBall));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("rho1",		TypeVectorPs);
		func_add_param("rho2",		TypeVectorPs);
	add_func("AddLinkBallAbs",							TypeInt,	god_p(&AddLinkBallAbs));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("p",		TypeVectorPs);
	add_func("LinkHingeSetTorque",					TypeVoid,	god_p(&LinkHingeSetTorque));
		func_add_param("link",		TypeInt);
		func_add_param("torque",		TypeFloat);
	add_func("LinkHingeSetAxis",					TypeVoid,	god_p(&LinkHingeSetAxis));
		func_add_param("link",		TypeInt);
		func_add_param("ax1",		TypeVectorPs);
		func_add_param("ax2",		TypeVectorPs);
	add_func("LinkSetFriction",		TypeVoid,	god_p(&LinkSetFriction));
		func_add_param("link",		TypeInt);
		func_add_param("friction",		TypeFloat);
	add_func("ObjectAddForce",		TypeVoid,	god_p(&ObjectAddForce));
		func_add_param("o",		TypeObjectP);
		func_add_param("force",		TypeVectorPs);
		func_add_param("rho",		TypeVectorPs);
	add_func("ObjectAddTorque",		TypeVoid,	god_p(&ObjectAddTorque));
		func_add_param("o",		TypeObjectP);
		func_add_param("torque",		TypeVectorPs);
	// file access
	add_func("FileOpen",			TypeFileP,				(void*)&FileOpen);
		func_add_param("filename",		TypeStringP);
	add_func("FileCreate",			TypeFileP,				(void*)&FileCreate);
		func_add_param("filename",		TypeStringP);
	add_func("FileClose",			TypeBool,				(void*)&FileClose);
		func_add_param("f",		TypeFileP);
	add_func("FileTestExistence",	TypeBool,		(void*)&file_test_existence);
		func_add_param("filename",		TypeStringP);
	add_func("dir_search",			TypeInt,			(void*)&dir_search);
		func_add_param("dir",		TypeStringP);
		func_add_param("filter",		TypeStringP);
		func_add_param("show_dirs",		TypeBool);
	// network
	add_func("NetConnect",			TypeInt,			(void*)&NixNetConnect);
		func_add_param("addr",		TypeStringP);
		func_add_param("port",		TypeInt);
	add_func("NetAccept",			TypeInt,			(void*)&NixNetAccept);
		func_add_param("s",		TypeInt);
	add_func("NetCreate",			TypeInt,			(void*)&NixNetCreate);
		func_add_param("port",		TypeInt);
		func_add_param("blocking",		TypeBool);
	add_func("NetClose",			TypeVoid,			(void*)&NixNetClose);
		func_add_param("s",		TypeIntPs);
	add_func("NetResetBuffer",		TypeVoid,			(void*)&NixNetResetBuffer);
	add_func("NetReadBuffer",		TypeVoid,			(void*)&NixNetReadBuffer);
		func_add_param("s",		TypeInt);
	add_func("NetWriteBuffer",		TypeVoid,			(void*)&NixNetWriteBuffer);
		func_add_param("s",		TypeInt);
	add_func("NetDirectRead",		TypeInt,			(void*)&NixNetRead);
		func_add_param("s",		TypeInt);
		func_add_param("buffer",		TypePointer);
		func_add_param("max_size",		TypeInt);
	add_func("NetDirectWrite",		TypeInt,			(void*)&NixNetWrite);
		func_add_param("s",		TypeInt);
		func_add_param("buffer",		TypePointer);
		func_add_param("size",		TypeInt);
	add_func("NetReadyToWrite",		TypeBool,			(void*)&NixNetReadyToWrite);
		func_add_param("s",		TypeInt);
	add_func("NetReadyToRead",		TypeBool,			(void*)&NixNetReadyToRead);
		func_add_param("s",		TypeInt);
	add_func("NetReadInt",			TypeInt,			(void*)&NixNetReadInt);
	add_func("NetReadBool",			TypeBool,			(void*)&NixNetReadBool);
	add_func("NetReadFloat",		TypeFloat,			(void*)&NixNetReadFloat);
	add_func("NetReadVector",		TypeVector,			(void*)&NixNetReadVector);
	add_func("NetReadStr",			TypeStringP,		(void*)&NixNetReadStr);
	add_func("NetReadStrL",			TypeVoid,			(void*)&NixNetReadStrL);
		func_add_param("s",		TypeStringP);
		func_add_param("length",		TypeIntPs);
	add_func("NetWriteInt",			TypeInt,			(void*)&NixNetWriteInt);
		func_add_param("i",		TypeInt);
	add_func("NetWriteBool",		TypeVoid,			(void*)&NixNetWriteBool);
		func_add_param("b",		TypeBool);
	add_func("NetWriteFloat",		TypeVoid,			(void*)&NixNetWriteFloat);
		func_add_param("f",		TypeFloat);
	add_func("NetWriteVector",		TypeVoid,			(void*)&NixNetWriteVector);
		func_add_param("v",		TypeVector);
	add_func("NetWriteStr",			TypeVoid,			(void*)&NixNetWriteStr);
		func_add_param("s",		TypeStringP);
	add_func("NetWriteStrL",		TypeVoid,			(void*)&NixNetWriteStrL);
		func_add_param("s",		TypeStringP);
		func_add_param("length",		TypeInt);

// add_func("ExecuteScript",	TypeVoid);
//		func_add_param("filename",		TypeString);


	
	add_struct(TypeFile);
		struct_add_func("GetDate",		TypeDate,				(void*)&FileGetDate);
			func_add_param("type",		TypeInt);
		struct_add_func("SetBinaryMode",TypeVoid,		mf((tmf)&CFile::SetBinaryMode));
			func_add_param("binary",	TypeBool);
		struct_add_func("WriteBool",	TypeVoid,			mf((tmf)&CFile::WriteBool));
			func_add_param("b",			TypeBool);
		struct_add_func("WriteInt",		TypeVoid,			mf((tmf)&CFile::WriteInt));
			func_add_param("i",			TypeInt);
		struct_add_func("WriteFloat",	TypeVoid,			mf((tmf)&CFile::WriteFloat));
			func_add_param("x",			TypeFloat);
		struct_add_func("WriteStr",		TypeVoid,			(void*)&FileWriteStr);
			func_add_param("s",			TypeStringP);
//		struct_add_func("WriteStr",		TypeVoid,			mf((tmf)&CFile::WriteStr));
//			func_add_param("s",			TypeString);
		struct_add_func("ReadBool",		TypeBool,			mf((tmf)&CFile::ReadBool));
		struct_add_func("ReadInt",		TypeInt,				mf((tmf)&CFile::ReadInt));
		struct_add_func("ReadFloat",	TypeFloat,			mf((tmf)&CFile::ReadFloat));
		struct_add_func("ReadStr",		TypeStringP,			mf((tmf)&CFile::ReadStr));
		struct_add_func("ReadBoolC",	TypeBool,			mf((tmf)&CFile::ReadBoolC));
		struct_add_func("ReadIntC",		TypeInt,			mf((tmf)&CFile::ReadIntC));
		struct_add_func("ReadFloatC",	TypeFloat,			mf((tmf)&CFile::ReadFloatC));
		struct_add_func("ReadStrC",		TypeStringP,			mf((tmf)&CFile::ReadStrC));


	add_type_cast(10,	TypeInt,		TypeFloat,	CommandIntToFloat,		(void*)&CastInt2Float);
	add_type_cast(20,	TypeFloat,		TypeInt,	CommandFloatToInt,		(void*)&CastFloat2Int);
	add_type_cast(10,	TypeInt,		TypeChar,	CommandIntToChar,		(void*)&CastInt2Char);
	add_type_cast(20,	TypeChar,		TypeInt,	CommandCharToInt,		(void*)&CastChar2Int);
	add_type_cast(50,	TypePointer,	TypeBool,	CommandPointerToBool,	(void*)&CastPointer2Bool);





//-------------------------------------------------
// hui



	sType *TypeHuiMenu		= add_type("-CHuiMenu-",	0				,0	,false	,false	,NULL);
	sType *TypeHuiMenuP		= add_type("HuiMenu",		PointerSize		,0	,true	,false	,TypeHuiMenu);
	sType *TypeHuiWindow	= add_type("-CHuiWindow-",	0				,0	,false	,false	,NULL);
	sType *TypeHuiWindowP	= add_type("HuiWindow",		PointerSize		,0	,true	,false	,TypeHuiWindow);

	add_struct(TypeHuiMenu);
		struct_add_func("OpenPopup",	TypeVoid,		mf((tmf)&CHuiMenu::OpenPopup));
			func_add_param("w",			TypeHuiWindowP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
		struct_add_func("AddEntry",		TypeVoid,		mf((tmf)&CHuiMenu::AddEntry));
			func_add_param("name",		TypeStringP);
			func_add_param("id",		TypeInt);
		struct_add_func("AddSeparator",	TypeVoid,		mf((tmf)&CHuiMenu::AddSeparator));
		struct_add_func("AddSubMenu",	TypeVoid,		mf((tmf)&CHuiMenu::AddSubMenu));
			func_add_param("name",		TypeStringP);
			func_add_param("id",		TypeInt);
			func_add_param("sub_menu",	TypeHuiMenuP);
		struct_add_func("CheckItem",	TypeVoid,		mf((tmf)&CHuiMenu::CheckItem));
			func_add_param("id",		TypeInt);
			func_add_param("checked",	TypeBool);
		struct_add_func("IsItemChecked",TypeBool,		mf((tmf)&CHuiMenu::IsItemChecked));
			func_add_param("id",		TypeInt);
		struct_add_func("EnableItem",	TypeVoid,		mf((tmf)&CHuiMenu::EnableItem));
			func_add_param("id",		TypeInt);
			func_add_param("enabled",	TypeBool);
		struct_add_func("SetText",		TypeVoid,		mf((tmf)&CHuiMenu::SetText));
			func_add_param("id",		TypeInt);
			func_add_param("text",		TypeStringP);
	
	add_struct(TypeHuiWindow);
		struct_add_element("Menu",		TypeHuiMenuP,	GetDAWindow(Menu));
		struct_add_func("Update",		TypeVoid,		mf((tmf)&CHuiWindow::Update));
		struct_add_func("Hide",			TypeVoid,		mf((tmf)&CHuiWindow::Hide));
			func_add_param("hide",		TypeBool);
		struct_add_func("SetMaximized",							TypeVoid,		mf((tmf)&CHuiWindow::SetMaximized));
			func_add_param("max",		TypeBool);
		struct_add_func("IsMaximized",								TypeBool,		mf((tmf)&CHuiWindow::IsMaximized));
		struct_add_func("IsMinimized",								TypeBool,		mf((tmf)&CHuiWindow::IsMinimized));
		struct_add_func("SetID",												TypeVoid,		mf((tmf)&CHuiWindow::SetID));
			func_add_param("id",		TypeInt);
		struct_add_func("SetFullscreen",					TypeVoid,		mf((tmf)&CHuiWindow::SetFullscreen));
			func_add_param("fullscreen",TypeBool);
		struct_add_func("SetTitle",										TypeVoid,		mf((tmf)&CHuiWindow::SetTitle));
			func_add_param("title",		TypeStringP);
		struct_add_func("SetPosition",								TypeVoid,		mf((tmf)&CHuiWindow::SetPosition));
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
	//add_func("SetOuterior",								TypeVoid,		2,	TypePointer,"win",
	//																										TypeIRect,"r");
	//add_func("GetOuterior",								TypeIRect,		1,	TypePointer,"win");
	//add_func("SetInerior",								TypeVoid,		2,	TypePointer,"win",
	//																										TypeIRect,"r");
	//add_func("GetInterior",									TypeIRect,		1,	TypePointer,"win");
		struct_add_func("SetCursorPos",								TypeVoid,		mf((tmf)&CHuiWindow::SetCursorPos));
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
		struct_add_func("Activate",										TypeVoid,		mf((tmf)&CHuiWindow::Activate));
			func_add_param("id",		TypeInt);
		struct_add_func("IsActive",										TypeVoid,		mf((tmf)&CHuiWindow::IsActive));
			func_add_param("recursive",	TypeBool);
		struct_add_func("AddButton",										TypeVoid,		mf((tmf)&CHuiWindow::AddButton));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("AddCheckBox",								TypeVoid,		mf((tmf)&CHuiWindow::AddCheckBox));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("AddText",										TypeVoid,		mf((tmf)&CHuiWindow::AddText));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("AddEdit",										TypeVoid,		mf((tmf)&CHuiWindow::AddEdit));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("AddGroup",										TypeVoid,		mf((tmf)&CHuiWindow::AddGroup));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("AddComboBox",								TypeVoid,		mf((tmf)&CHuiWindow::AddComboBox));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("AddTabControl",								TypeVoid,		mf((tmf)&CHuiWindow::AddTabControl));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("SetTabCreationPage",				TypeVoid,		mf((tmf)&CHuiWindow::SetTabCreationPage));
			func_add_param("id",		TypeInt);
			func_add_param("page",		TypeInt);
		struct_add_func("AddListView",								TypeVoid,		mf((tmf)&CHuiWindow::AddListView));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("AddProgressBar",						TypeVoid,		mf((tmf)&CHuiWindow::AddProgressBar));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("AddSlider",										TypeVoid,		mf((tmf)&CHuiWindow::AddSlider));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("AddImage",										TypeVoid,		mf((tmf)&CHuiWindow::AddImage));
			func_add_param("title",		TypeStringP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		struct_add_func("SetControlText",						TypeVoid,		mf((tmf)&CHuiWindow::SetControlText));
			func_add_param("id",		TypeInt);
			func_add_param("s",			TypeStringP);
		struct_add_func("GetControlText",						TypeStringP,		mf((tmf)&CHuiWindow::GetControlText));
			func_add_param("id",		TypeInt);
		struct_add_func("SetControlFloat",						TypeVoid,		mf((tmf)&CHuiWindow::SetControlFloat));
			func_add_param("id",		TypeInt);
			func_add_param("f",			TypeFloat);
	func_add_param("decimals",	TypeInt);
		struct_add_func("GetControlFloat",						TypeFloat,		mf((tmf)&CHuiWindow::GetControlFloat));
			func_add_param("id",		TypeInt);
		struct_add_func("EnableControl",								TypeVoid,		mf((tmf)&CHuiWindow::EnableControl));
			func_add_param("id",		TypeInt);
			func_add_param("enabled",	TypeBool);
		struct_add_func("IsControlEnabled",					TypeBool,		mf((tmf)&CHuiWindow::IsControlEnabled));
			func_add_param("id",		TypeInt);
		struct_add_func("CheckControl",								TypeVoid,		mf((tmf)&CHuiWindow::CheckControl));
			func_add_param("id",		TypeInt);
			func_add_param("checked",	TypeBool);
		struct_add_func("IsControlChecked",					TypeBool,		mf((tmf)&CHuiWindow::IsControlChecked));
			func_add_param("id",		TypeInt);
		struct_add_func("GetControlSelection",			TypeInt,		mf((tmf)&CHuiWindow::GetControlSelection));
			func_add_param("id",		TypeInt);
		struct_add_func("GetControlSelectionM",			TypeInt,		mf((tmf)&CHuiWindow::GetControlSelectionM));
			func_add_param("id",		TypeInt);
			func_add_param("indices",	TypeIntP);
		struct_add_func("SetControlSelection",			TypeVoid,		mf((tmf)&CHuiWindow::SetControlSelection));
			func_add_param("id",		TypeInt);
			func_add_param("index",		TypeInt);
		struct_add_func("ResetControl",								TypeVoid,		mf((tmf)&CHuiWindow::ResetControl));
			func_add_param("id",		TypeInt);

	

	add_func("NixInit",				TypeVoid,					(void*)&NixInit);
		func_add_param("api",		TypeInt);
		func_add_param("xres",		TypeInt);
		func_add_param("yres",		TypeInt);
		func_add_param("depth",		TypeInt);
		func_add_param("fullscreen",TypeBool);
		func_add_param("w",			TypeHuiWindowP);
	// user interface
	add_func("HuiSetIdleFunction",	TypeVoid,		(void*)HuiSetIdleFunction);
		func_add_param("idle_func",	TypePointer);
	add_func("HuiRun",				TypeVoid,		(void*)&HuiRun);
	add_func("HuiEnd",				TypeVoid,		(void*)&HuiEnd);
	add_func("HuiWaitTillWindowClosed",		TypeVoid,	(void*)&HuiWaitTillWindowClosed);
		func_add_param("w",			TypeHuiWindowP);
	add_func("HuiDoSingleMainLoop",	TypeVoid,	(void*)&HuiDoSingleMainLoop);
	add_func("HuiSleep",			TypeVoid,	(void*)&HuiSleep);
		func_add_param("ms",		TypeInt);
	add_func("HuiFileDialogOpen",	TypeBool,	(void*)&HuiFileDialogOpen);
		func_add_param("root",		TypePointer);
		func_add_param("title",		TypeStringP);
		func_add_param("dir",		TypeStringP);
		func_add_param("show_filter",	TypeStringP);
		func_add_param("filter",	TypeStringP);
	add_func("HuiFileDialogSave",	TypeBool,	(void*)&HuiFileDialogSave);
		func_add_param("root",		TypePointer);
		func_add_param("title",		TypeStringP);
		func_add_param("dir",		TypeStringP);
		func_add_param("show_filter",	TypeStringP);
		func_add_param("filter",	TypeStringP);
	add_func("HuiFileDialogDir",	TypeBool,	(void*)&HuiFileDialogDir);
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("title",		TypeStringP);
		func_add_param("dir",		TypeStringP);
	add_func("HuiQuestionBox",		TypeInt,	(void*)&HuiQuestionBox);
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("title",		TypeStringP);
		func_add_param("text",		TypeStringP);
		func_add_param("allow_cancel",	TypeBool);
	add_func("HuiInfoBox",			TypeVoid,			(void*)&HuiInfoBox);
		func_add_param("win",		TypeHuiWindowP);
		func_add_param("title",		TypeStringP);
		func_add_param("text",		TypeStringP);
	add_func("HuiErrorBox",			TypeVoid,		(void*)&HuiErrorBox);
		func_add_param("win",		TypeHuiWindowP);
		func_add_param("title",		TypeStringP);
		func_add_param("text",		TypeStringP);
	add_func("HuiConfigWriteInt",								TypeVoid,	(void*)&HuiConfigWriteInt);
		func_add_param("name",		TypeStringP);
		func_add_param("value",		TypeInt);
	add_func("HuiConfigWriteStr",								TypeVoid,	(void*)&HuiConfigWriteStr);
		func_add_param("name",		TypeStringP);
		func_add_param("value",		TypeStringP);
	add_func("HuiConfigReadInt",								TypeVoid,	(void*)&HuiConfigReadInt);
		func_add_param("name",		TypeStringP);
		func_add_param("value",		TypeIntP);
		func_add_param("default",	TypeInt);
	add_func("HuiConfigReadStr",								TypeVoid,	(void*)&HuiConfigReadStr);
		func_add_param("name",		TypeStringP);
		func_add_param("value",		TypeStringP);
		func_add_param("default",	TypeStringP);

	// clipboard
	add_func("HuiCopyToClipboard",	TypeVoid,			(void*)&HuiCopyToClipBoard);
		func_add_param("buffer",	TypeStringP);
		func_add_param("length",	TypeInt);
	add_func("HuiPasteFromClipboard",	TypeVoid,		(void*)&HuiPasteFromClipBoard);
		func_add_param("buf",		TypeStringPP);
		func_add_param("length",	TypeIntPs);
	add_func("HuiOpenDocument",		TypeVoid,			(void*)&HuiOpenDocument);
		func_add_param("filename",	TypeStringP);
	add_func("CreateTimer",			TypeInt,			(void*)&HuiCreateTimer);
	add_func("GetTime",				TypeFloat,			(void*)&HuiGetTime);
		func_add_param("timer",		TypeInt);
	// menu
	add_func("HuiCreateMenu",		TypeHuiMenuP,		(void*)&HuiCreateMenu);
	// window
	add_func("HuiCreateWindow",		TypeHuiWindowP,			(void*)&HuiCreateWindow);
		func_add_param("title",		TypeStringP);
		func_add_param("x",			TypeInt);
		func_add_param("y",			TypeInt);
		func_add_param("width",		TypeInt);
		func_add_param("height",	TypeInt);
		func_add_param("message_func",	TypePointer);
	add_func("HuiCreateNixWindow",	TypeHuiWindowP,			(void*)&HuiCreateNixWindow);
		func_add_param("title",		TypeStringP);
		func_add_param("x",			TypeInt);
		func_add_param("y",			TypeInt);
		func_add_param("width",		TypeInt);
		func_add_param("height",	TypeInt);
		func_add_param("message_func",	TypePointer);
	add_func("HuiCreateDialog",		TypeHuiWindowP,			(void*)&HuiCreateDialog);
		func_add_param("title",		TypeStringP);
		func_add_param("width",		TypeInt);
		func_add_param("height",	TypeInt);
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("allow_root",TypeBool);
		func_add_param("message_func",	TypePointer);
	add_func("HuiWinClose",			TypeVoid,				(void*)&HuiCloseWindow);
		func_add_param("w",			TypeHuiWindowP);


	msg_db_l(1);
}

void ScriptResetSemiExternalVars()
{
	for (int i=NumTruePreExternalVars;i<PreExternalVar.size();i++)
		delete[](PreExternalVar[i].Name);
	PreExternalVar.erase(PreExternalVar.begin() + NumTruePreExternalVars, PreExternalVar.end());
}

// Programm-Variablen, die anwendungsspezifisch sind und deshalb nicht immer da sein duerfen...
void ScriptLinkSemiExternalVar(char *name, void *pointer)
{
	sPreExternalVar v;
	v.Name = new char[strlen(name) + 1];
	strcpy(v.Name, name);
	v.Pointer = pointer;
	v.Type = TypeUnknown; // unusable until defined via "extern" in the script!
	PreExternalVar.push_back(v);
}



sScriptLocation ScriptLocation[NumScriptLocations]={
	{"CalcMovePrae",		ScriptLocationCalcMovePrae},
	{"CalcMovePost",		ScriptLocationCalcMovePost},
	{"RenderPrae",			ScriptLocationRenderPrae},
	{"RenderPost1",			ScriptLocationRenderPost1},
	{"RenderPost2",			ScriptLocationRenderPost2},
	{"GetInputPrae",		ScriptLocationGetInputPrae},
	{"GetInputPost",		ScriptLocationGetInputPost},
	{"NetworkSend",			ScriptLocationNetworkSend},
	{"NetworkRecieve",		ScriptLocationNetworkRecieve},
	{"NetworkAddClient",	ScriptLocationNetworkAddClient},
	{"NetworkRemoveClient",	ScriptLocationNetworkRemoveClient},
	{"OnKillObject",		ScriptLocationOnKillObject},
	{"OnCollision",			ScriptLocationOnCollision}
};

