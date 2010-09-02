/*----------------------------------------------------------------------------*\
| Script Data                                                                  |
| -> "standart library" for the scripting system                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2010.07.07 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include <algorithm>
#include <string.h>

#include "script.h"


//#include "../00_config.h"
#ifdef _X_ALLOW_META_
	#include "../x/x.h"
#endif

#ifdef _X_ALLOW_GOD_
	extern void SetMessage(char *msg);
#endif

char ScriptDataVersion[]="0.7.0.-1";

int ScriptStackSize = SCRIPT_DEFAULT_STACK_SIZE;

#ifdef object
#undef object
#endif

enum{
	FunctionSpecial,
	FunctionClass,
};


//------------------------------------------------------------------------------------------------//
//                                             types                                              //
//------------------------------------------------------------------------------------------------//

sType *TypeUnknown;
sType *TypeVoid;
sType *TypePointer;
sType *TypeClass;
sType *TypeBool;
sType *TypeInt;
sType *TypeFloat;
sType *TypeChar;
sType *TypeString;
sType *TypeSuperArray;

sType *TypeVector;
sType *TypeRect;
sType *TypeColor;
sType *TypeQuaternion;
 // internal:
sType *TypeStringP;
sType *TypePointerS;
sType *TypePointerPs;
sType *TypePointerList;
sType *TypeBoolP;
sType *TypeBoolPs;
sType *TypeBoolList;
sType *TypeIntP;
sType *TypeIntPs;
sType *TypeIntList;
sType *TypeIntListPs;
sType *TypeIntArray;
sType *TypeIntArrayP;
sType *TypeFloatP;
sType *TypeFloatPs;
sType *TypeFloatList;
sType *TypeFloatListPs;
sType *TypeFloatArray;
sType *TypeFloatArrayP;
sType *TypeFloatArray3x3;
sType *TypeFloatArray4x4;
sType *TypeComplex;
sType *TypeComplexList;
sType *TypeComplexListPs;
sType *TypeCharP;
sType *TypeStringPP;
sType *TypeStringList;
sType *TypeVectorP;
sType *TypeVectorPs;
sType *TypeVectorArray;
sType *TypeVectorArrayP;
sType *TypeVectorList;
sType *TypeRectP;
sType *TypeRectPs;
sType *TypeMatrix;
sType *TypeMatrixP;
sType *TypeMatrixPs;
sType *TypeQuaternionP;
sType *TypeQuaternionPs;
sType *TypePlane;
sType *TypePlaneP;
sType *TypePlanePs;
sType *TypeColorP;
sType *TypeColorPs;
sType *TypeMatrix3;
sType *TypeObject;
sType *TypeObjectP;
sType *TypeObjectPPs;
sType *TypeObjectPList;
sType *TypeObjectPListPs;
sType *TypeBone;
sType *TypeBoneList;
sType *TypeModel;
sType *TypeModelP;
sType *TypeModelPP;
sType *TypeModelPList;
sType *TypeItem;
sType *TypeItemP;
sType *TypeItemPList;
sType *TypeFile;
sType *TypeFileP;
sType *TypeDate;
sType *TypeText;
sType *TypeTextP;
sType *TypePicture;
sType *TypePictureP;
sType *TypePicture3D;
sType *TypePicture3DP;
sType *TypeGrouping;
sType *TypeGroupingP;
sType *TypeParticle;
sType *TypeParticleP;
sType *TypeBeam;
sType *TypeBeamP;
sType *TypeEffect;
sType *TypeEffectP;
sType *TypeView;
sType *TypeViewP;
sType *TypeSkin;
sType *TypeSkinP;
sType *TypeSkinPArray;
sType *TypeMaterial;
sType *TypeMaterialArray;
sType *TypeMaterialArrayP;
sType *TypeFog;
sType *TypeTerrain;
sType *TypeTerrainP;
sType *TypeTerrainPP;
sType *TypeTerrainPList;


std::vector<sType*> PreType;
sType *add_type(const char *name, int size)
{
	msg_db_r("add_type", 2);
	sType *t = new sType;
	strcpy(t->Name, name);
	t->Owner = NULL;
	t->Size = size;
	t->IsArray = false;
	t->IsSuperArray = false;
	t->ArrayLength = 0;
	t->IsPointer = false;
	t->IsSilent = false;
	t->Class = NULL;
	t->SubType = NULL;
	PreType.push_back(t);
	msg_db_l(2);
	return t;
}
sType *add_type_p(const char *name, sType *sub_type, bool is_silent = false)
{
	msg_db_r("add_type_p", 2);
	sType *t = new sType;
	strcpy(t->Name, name);
	t->Owner = NULL;
	t->Size = PointerSize;
	t->IsArray = false;
	t->IsSuperArray = false;
	t->ArrayLength = 0;
	t->IsPointer = true;
	t->IsSilent = is_silent;
	t->SubType = sub_type;
	t->Class = NULL;
	PreType.push_back(t);
	msg_db_l(2);
	return t;
}
sType *add_type_a(const char *name, sType *sub_type, int array_length)
{
	msg_db_r("add_type_a", 2);
	sType *t = new sType;
	strcpy(t->Name, name);
	t->Owner = NULL;
	t->IsPointer = false;
	t->IsSilent = false;
	t->Class = NULL;
	t->SubType = sub_type;
	if (array_length < 0){
		// super array
		t->Size = SuperArraySize;
		t->IsArray = false;
		t->IsSuperArray = true;
		t->ArrayLength = 0;
		//script_make_super_array(t); // do it later !!!
	}else{
		// standard array
		t->Size = sub_type->Size * array_length;
		t->IsArray = true;
		t->IsSuperArray = false;
		t->ArrayLength = array_length;
	}
	PreType.push_back(t);
	msg_db_l(2);
	return t;
}

sType *ScriptGetPreType(const char *name)
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
	msg_db_r("add_op", 2);
	sPreOperator o;
	o.PrimitiveID = primitive_op;
	o.ReturnType = return_type;
	o.ParamType1 = param_type1;
	o.ParamType2 = param_type2;
	PreOperator.push_back(o);
	msg_db_l(2);
	return PreOperator.size() - 1;
}


//------------------------------------------------------------------------------------------------//
//                                     classes & elements                                         //
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
	static sModelBone *_bone;
	#define	GetDABone(x)		long(&_bone->x)-long(_bone)
	static CModel *_model;
	#define sizeof_sModelBone	sizeof(sModelBone)
	#define	GetDAModel(x)		long(&_model->x)-long(_model)
	static sSkin *_skin;
	#define	GetDASkin(x)		long(&_skin->x)-long(_skin)
	static sMaterial *_material;
	#define	GetDAMaterial(x)	long(&_material->x)-long(_material)
#else
	#define	GetDAItem(x)		0
	#define	GetDABone(x)		0
	#define sizeof_sModelBone	0
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
	static CView *_view;
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


std::vector<sClass*> PreClass;
sClass *cur_class;

void add_class(sType *root_type, CPreScript *ps = NULL)
{
	msg_db_r("add_class", 2);
	cur_class = new sClass;
	cur_class->RootType = root_type;
	cur_class->Owner = ps;
	root_type->Class = cur_class;
	if (ps)
		ps->Class.push_back(cur_class);
	else
		PreClass.push_back(cur_class);
	msg_db_l(2);
}

void class_add_element(const char *name, sType *type, int offset)
{
	msg_db_r("add_class_el", 2);
	sClassElement e;
	strcpy(e.Name, name);
	e.Type = type;
	e.Offset = offset;
	cur_class->Element.push_back(e);
	msg_db_l(2);
}

int add_func(const char *name, sType *return_type, void *func, int special = -1);

void class_add_func(const char *name, sType *return_type, void *func)
{
	msg_db_r("add_class_func", 2);
	char *tname = cur_class->RootType->Name;
	if (tname[0] == '-')
		for (int i=0;i<PreType.size();i++)
			if ((PreType[i]->IsPointer) && (PreType[i]->SubType == cur_class->RootType))
				tname = PreType[i]->Name;
	char *mname = new char[strlen(tname) + strlen(name) + 2];
	strcpy(mname, string(tname, ".", name));
	int cmd = add_func(mname, return_type, func, FunctionClass);
	sClassFunction f;
	strcpy(f.Name, name);
	f.Kind = KindCompilerFunction;
	f.Nr = cmd;
	cur_class->Function.push_back(f);
	msg_db_l(2);
}


//------------------------------------------------------------------------------------------------//
//                                           constants                                            //
//------------------------------------------------------------------------------------------------//

std::vector<sPreConstant> PreConstant;
void add_const(const char *name, sType *type, void *value)
{
	msg_db_r("add_const", 2);
	sPreConstant c;
	c.Name = name;
	c.Type = type;
	c.Value = value;
	PreConstant.push_back(c);
	msg_db_l(2);
}


//------------------------------------------------------------------------------------------------//
//                                  pre defined global variables                                  //
//------------------------------------------------------------------------------------------------//

std::vector<sPreGlobalVar> PreGlobalVar;

void ScriptAddPreGlobalVar(const char *name, sType *type)
{
	sPreGlobalVar v;
	v.Name = (char*)name;
	v.Type = type;
	PreGlobalVar.push_back(v);
}


//------------------------------------------------------------------------------------------------//
//                                    environmental variables                                     //
//------------------------------------------------------------------------------------------------//

std::vector<sPreExternalVar> PreExternalVar;

void add_ext_var(const char *name, sType *type, void *var)
{
	sPreExternalVar v;
	v.Name = name;
	v.Type = type;
	v.Pointer = var;
	v.IsSemiExternal = false;
	PreExternalVar.push_back(v);
};

//------------------------------------------------------------------------------------------------//
//                                      compiler functions                                        //
//------------------------------------------------------------------------------------------------//



#ifndef FILE_OS_WINDOWS
	//#define _cdecl
	#include <stdlib.h>
#endif

float _cdecl f_sqr(float f){	return f*f;	}
//void _cdecl _stringout(char *str){	msg_write(string("StringOut: ",str));	}
void _cdecl _stringout(char *str){	msg_write(str);	}
int _cdecl _Float2Int(float f){	return (int)f;	}

typedef void (CFile::*tmf)();
typedef char *tcpa[4];
void *mf(tmf vmf)
{
	tcpa *cpa=(tcpa*)&vmf;
	return (*cpa)[0];
}

void *f_cp = (void*)1; // for fake (compiler-) functions


std::vector<sPreCommand> PreCommand;

int cur_func;

int add_func(const char *name, sType *return_type, void *func, int special)
{
	sPreCommand c;
	c.Name = name;
	c.ReturnType = return_type;
	c.Func = func;
	c.IsSpecial = (special == FunctionSpecial);
	c.IsClassFunction = (special == FunctionClass);
	c.IsSemiExternal = false;
	PreCommand.push_back(c);
	cur_func = PreCommand.size() - 1;
	return cur_func;
}

void func_add_param(const char *name, sType *type)
{
	sPreCommandParam p;
	p.Name = name;
	p.Type = type;
	PreCommand[cur_func].Param.push_back(p);
}

void CSuperArray::init_by_type(sType *t)
{	init(t->Size);	}

void CSuperArray::int_sort()
{	std::sort((int*)data, (int*)data + num);	}

void CSuperArray::int_unique()
{
	int ndiff = 0;
	int i0 = 1;
	while(((int*)data)[i0] != ((int*)data)[i0-1])
		i0 ++;
	for (int i=i0;i<num;i++){
		if (((int*)data)[i] == ((int*)data)[i-1])
			ndiff ++;
		else
			((int*)data)[i - ndiff] = ((int*)data)[i];
	}
	resize(num - ndiff);
}

void CSuperArray::float_sort()
{	std::sort((float*)data, (float*)data + num);	}

//void init_sub_super_array(CPreScript *ps, sFunction *f, sType *t, char* g_var, int offset); // script.cpp

#if 0
void CSuperArray::resize(int size)
{
	reserve(size);
	if (size > num){
		memset(&((char*)data)[num * item_size], 0, (size - num) * item_size);
		/*for (int i=num;i<size;i++)
			init_sub_super_array(NULL, NULL, */
	}
	num = size;
}
#endif

void super_array_assign(CSuperArray *a, CSuperArray *b)
{
	a->item_size = b->item_size; // ...
	a->reserve(b->num);
	memcpy(a->data, b->data, b->item_size * b->num);
	a->num = b->num;
}

// a += b
void super_array_add_s_int(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	int *pa = (int*)a->data;	int *pb = (int*)b->data;	for (int i=0;i<n;i++)	*(pa ++) += *(pb ++);	}
void super_array_sub_s_int(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	int *pa = (int*)a->data;	int *pb = (int*)b->data;	for (int i=0;i<n;i++)	*(pa ++) -= *(pb ++);	}
void super_array_mul_s_int(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	int *pa = (int*)a->data;	int *pb = (int*)b->data;	for (int i=0;i<n;i++)	*(pa ++) *= *(pb ++);	}
void super_array_div_s_int(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	int *pa = (int*)a->data;	int *pb = (int*)b->data;	for (int i=0;i<n;i++)	*(pa ++) /= *(pb ++);	}

// a = b + c
void super_array_add_int(CSuperArray *a, CSuperArray *b, CSuperArray *c)
{	int n = min(b->num, c->num);	a->resize(n);	int *pa = (int*)a->data;	int *pb = (int*)b->data;	int *pc = (int*)c->data;	for (int i=0;i<n;i++)	*(pa ++) = *(pb ++) + *(pc ++);	}
void super_array_sub_int(CSuperArray *a, CSuperArray *b, CSuperArray *c)
{	int n = min(b->num, c->num);	a->resize(n);	int *pa = (int*)a->data;	int *pb = (int*)b->data;	int *pc = (int*)c->data;	for (int i=0;i<n;i++)	*(pa ++) = *(pb ++) - *(pc ++);	}
void super_array_mul_int(CSuperArray *a, CSuperArray *b, CSuperArray *c)
{	int n = min(b->num, c->num);	a->resize(n);	int *pa = (int*)a->data;	int *pb = (int*)b->data;	int *pc = (int*)c->data;	for (int i=0;i<n;i++)	*(pa ++) = *(pb ++) * *(pc ++);	}
void super_array_div_int(CSuperArray *a, CSuperArray *b, CSuperArray *c)
{	int n = min(b->num, c->num);	a->resize(n);	int *pa = (int*)a->data;	int *pb = (int*)b->data;	int *pc = (int*)c->data;	for (int i=0;i<n;i++)	*(pa ++) = *(pb ++) / *(pc ++);	}

// a += x
void super_array_add_s_int_int(CSuperArray *a, int x)
{	int *pa = (int*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) += x;	}
void super_array_sub_s_int_int(CSuperArray *a, int x)
{	int *pa = (int*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) -= x;	}
void super_array_mul_s_int_int(CSuperArray *a, int x)
{	int *pa = (int*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) *= x;	}
void super_array_div_s_int_int(CSuperArray *a, int x)
{	int *pa = (int*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) /= x;	}

void super_array_add_s_float(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	float *pa = (float*)a->data;	float *pb = (float*)b->data;	for (int i=0;i<n;i++)	*(pa ++) += *(pb ++);	}
void super_array_sub_s_float(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	float *pa = (float*)a->data;	float *pb = (float*)b->data;	for (int i=0;i<n;i++)	*(pa ++) -= *(pb ++);	}
void super_array_mul_s_float(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	float *pa = (float*)a->data;	float *pb = (float*)b->data;	for (int i=0;i<n;i++)	*(pa ++) *= *(pb ++);	}
void super_array_div_s_float(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	float *pa = (float*)a->data;	float *pb = (float*)b->data;	for (int i=0;i<n;i++)	*(pa ++) /= *(pb ++);	}

void super_array_add_s_float_float(CSuperArray *a, float x)
{	float *pa = (float*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) += x;	}
void super_array_sub_s_float_float(CSuperArray *a, float x)
{	float *pa = (float*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) -= x;	}
void super_array_mul_s_float_float(CSuperArray *a, float x)
{	float *pa = (float*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) *= x;	}
void super_array_div_s_float_float(CSuperArray *a, float x)
{	float *pa = (float*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) /= x;	}

void super_array_add_s_com(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	complex *pa = (complex*)a->data;	complex *pb = (complex*)b->data;	for (int i=0;i<n;i++)	*(pa ++) += *(pb ++);	}
void super_array_sub_s_com(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	complex *pa = (complex*)a->data;	complex *pb = (complex*)b->data;	for (int i=0;i<n;i++)	*(pa ++) -= *(pb ++);	}
void super_array_mul_s_com(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	complex *pa = (complex*)a->data;	complex *pb = (complex*)b->data;	for (int i=0;i<n;i++)	*(pa ++) *= *(pb ++);	}
void super_array_div_s_com(CSuperArray *a, CSuperArray *b)
{	int n = min(a->num, b->num);	complex *pa = (complex*)a->data;	complex *pb = (complex*)b->data;	for (int i=0;i<n;i++)	*(pa ++) /= *(pb ++);	}

void super_array_add_s_com_com(CSuperArray *a, complex x)
{	complex *pa = (complex*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) += x;	}
void super_array_sub_s_com_com(CSuperArray *a, complex x)
{	complex *pa = (complex*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) -= x;	}
void super_array_mul_s_com_com(CSuperArray *a, complex x)
{	complex *pa = (complex*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) *= x;	}
void super_array_div_s_com_com(CSuperArray *a, complex x)
{	complex *pa = (complex*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) /= x;	}
void super_array_mul_s_com_float(CSuperArray *a, float x)
{	complex *pa = (complex*)a->data;	for (int i=0;i<a->num;i++)	*(pa ++) *= x;	}

void super_array_assign_8_single(CSuperArray *a, complex x)
{
	complex *p = (complex*)a->data;
	for (int i=0;i<a->num;i++)
		*(p ++) = x;
}

void super_array_assign_4_single(CSuperArray *a, int x)
{
	int *p = (int*)a->data;
	for (int i=0;i<a->num;i++)
		*(p ++) = x;
}

void super_array_assign_1_single(CSuperArray *a, char x)
{
	char *p = (char*)a->data;
	for (int i=0;i<a->num;i++)
		*(p ++) = x;
}

float sum_float_list(CSuperArray *a)
{
	float f = 0;
	float *p = (float*)a->data;
	for (int i=0;i<a->num;i++)
		f += *(p ++);
	return f;
}

float sum2_float_list(CSuperArray *a)
{
	float f = 0;
	float *p = (float*)a->data;
	for (int i=0;i<a->num;i++){
		f += *p * *p;
		p ++;
	}
	return f;
}

complex sum_complex_list(CSuperArray *a)
{
	complex r = complex(0, 0);
	complex *p = (complex*)a->data;
	for (int i=0;i<a->num;i++)
		r += *(p ++);
	return r;
}

float sum2_complex_list(CSuperArray *a)
{
	float f = 0;
	complex *p = (complex*)a->data;
	for (int i=0;i<a->num;i++){
		f += p->x * p->x + p->y * p->y;
		p ++;
	}
	return f;
}

CSuperArray int_range(int start, int end)
{
	CSuperArray a;
	a.init_by_type(TypeInt);
	for (int i=start;i<end;i++)
		a.append_4_single(i);
	return a;
}

CSuperArray float_range(float start, float end, float step)
{
	CSuperArray a;
	a.init_by_type(TypeFloat);
	for (float f=start;f<end;f+=step)
		a.append_4_single(*(int*)&f);
	return a;
}

void script_make_super_array(sType *t, CPreScript *ps)
{
	msg_db_r("make_super_array", 1);
	add_class(t, ps);
		class_add_element("num", TypeInt, PointerSize);
		class_add_func("clear", TypeVoid, mf((tmf)&CSuperArray::clear));
		class_add_func("erase", TypeVoid, mf((tmf)&CSuperArray::erase_single));
			func_add_param("index",		TypeInt);
		class_add_func("erasep", TypeVoid, mf((tmf)&CSuperArray::erase_single_by_pointer));
			func_add_param("pointer",		TypePointer);
		class_add_func("iterate", TypeBool, mf((tmf)&CSuperArray::iterate));
			func_add_param("pointer",		TypePointerPs);
		class_add_func("iterate_back", TypeBool, mf((tmf)&CSuperArray::iterate_back));
			func_add_param("pointer",		TypePointerPs);
		class_add_func("index", TypeInt, mf((tmf)&CSuperArray::index));
			func_add_param("pointer",		TypePointer);
		class_add_func("resize", TypeVoid, mf((tmf)&CSuperArray::resize));
			func_add_param("num",		TypeInt);
		class_add_func("ensure_size", TypeVoid, mf((tmf)&CSuperArray::ensure_size));
			func_add_param("num",		TypeInt);
		if (t->SubType->Size == 4){
			class_add_func("push", TypeVoid, mf((tmf)&CSuperArray::append_4_single));
				func_add_param("x",		t->SubType);
		}else if (t->SubType->Size == 1){
			class_add_func("push", TypeVoid, mf((tmf)&CSuperArray::append_1_single));
				func_add_param("x",		t->SubType);
		}else if (t->SubType == TypeComplex){
			class_add_func("push", TypeVoid, mf((tmf)&CSuperArray::append_single));
				func_add_param("x",		t->SubType);
		}else if (t->SubType->IsArray){
			class_add_func("push", TypeVoid, mf((tmf)&CSuperArray::append_single));
				func_add_param("x",		t->SubType);
		}else{
			class_add_func("push", TypeVoid, mf((tmf)&CSuperArray::append_single));
				func_add_param("x",		TypePointer);
		}
		if (t->SubType == TypeInt){
			class_add_func("sort", TypeVoid, mf((tmf)&CSuperArray::int_sort));
			class_add_func("unique", TypeVoid, mf((tmf)&CSuperArray::int_unique));
		}else if (t->SubType == TypeFloat){
			class_add_func("sort", TypeVoid, mf((tmf)&CSuperArray::float_sort));
		}

	// until we have automatic initialization... (T_T)
		class_add_func("_manual_init_", TypeVoid, mf((tmf)&CSuperArray::init));
			func_add_param("item_size",		TypeInt);
	msg_db_l(1);
}


// automatic type casting

#define MAX_TYPE_CAST_BUFFER	32768
char type_cast_buffer[MAX_TYPE_CAST_BUFFER];
int type_cast_buffer_size = 0;

inline char *get_type_cast_buf(int size)
{
	char *str = &type_cast_buffer[type_cast_buffer_size];
	type_cast_buffer_size += size;
	if (type_cast_buffer_size >= MAX_TYPE_CAST_BUFFER){
		msg_error("Script: type_cast_buffer overflow");
		type_cast_buffer_size = 0;
		str = type_cast_buffer;
	}
	return str;
}


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
char *CastInt2StringP(int *i)
{
	char *s = i2s(*i);
	char *str = get_type_cast_buf(strlen(s) + 1);
	strcpy(str, s);
	*(char**)&CastTemp[0] = str; // save the return address in CastTemp
	return &CastTemp[0];
}
char *CastFloat2StringP(float *f)
{
	char *s = f2sf(*f);
	char *str = get_type_cast_buf(strlen(s) + 1);
	strcpy(str, s);
	*(char**)&CastTemp[0] = str; // save the return address in CastTemp
	return &CastTemp[0];
}
char *CastBool2StringP(bool *b)
{
	char *s = b2s(*b);
	char *str = get_type_cast_buf(strlen(s) + 1);
	strcpy(str, s);
	type_cast_buffer_size += strlen(str) + 1;
	*(char**)&CastTemp[0] = str; // save the return address in CastTemp
	return &CastTemp[0];
}
char *CastPointer2StringP(void *p)
{
	char *s = p2s(p);
	char *str = get_type_cast_buf(strlen(s) + 1);
	strcpy(str, s);
	*(char**)&CastTemp[0] = str; // save the return address in CastTemp
	return &CastTemp[0];
}
char *CastVector2StringP(vector *v)
{
	char *s = fff2s((float*)v);
	char *str = get_type_cast_buf(strlen(s) + 1);
	strcpy(str, s);
	*(char**)&CastTemp[0] = str; // save the return address in CastTemp
	return &CastTemp[0];
}
char *CastFFFF2StringP(quaternion *v)
{
	char *s = ffff2s((float*)v);
	char *str = get_type_cast_buf(strlen(s) + 1);
	strcpy(str, s);
	*(char**)&CastTemp[0] = str; // save the return address in CastTemp
	return &CastTemp[0];
}
char *CastComplex2StringP(complex *z)
{
	char *s = ff2s((float*)z);
	char *str = get_type_cast_buf(strlen(s) + 1);
	strcpy(str, s);
	*(char**)&CastTemp[0] = str; // save the return address in CastTemp
	return &CastTemp[0];
}

std::vector<sTypeCast> TypeCast;
void add_type_cast(int penalty, sType *source, sType *dest, const char *cmd, void *func)
{
	sTypeCast c;
	c.Penalty = penalty;
	c.Command = -1;
	for (int i=0;i<PreCommand.size();i++)
		if (strcmp(PreCommand[i].Name, cmd) == 0){
			c.Command = i;
			break;
		}
	if (c.Command < 0){
		HuiErrorBox(NULL, "", string("add_type_cast (ScriptInit): ", cmd, " not found"));
		HuiRaiseError(string("add_type_cast (ScriptInit): ", cmd, " not found"));
		//exit(1);
	}
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
#ifdef _X_ALLOW_MODEL_
	#define mod_p(p)	(void*)p
#else
	#define mod_p(p)	NULL
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

void SIAddTypes()
{
	msg_db_r("SIAddTypes", 1);
	
	TypeUnknown			= add_type  ("-\?\?\?-",	0); // should not appear anywhere....or else we're screwed up!
	TypeClass			= add_type  ("-class-",	0); // substitute for all class types
	TypeVoid			= add_type  ("void",		0);
	TypeSuperArray		= add_type_a("void[]",		TypeVoid, -1); // substitute for all super arrays
	TypePointer			= add_type_p("void*",		TypeVoid); // substitute for all pointer types
	TypePointerS		= add_type_p("void&",		TypeVoid, true);
	TypePointerPs		= add_type_p("void*&",		TypePointer, true);
	TypePointerList		= add_type_a("void*[]",		TypePointer, -1);
	TypeBool			= add_type  ("bool",		sizeof(bool));
	TypeBoolP			= add_type_p("bool*",		TypeBool);
	TypeBoolPs			= add_type_p("bool&",		TypeBool, true);
	TypeBoolList		= add_type_a("bool[]",		TypeBool, -11);
	TypeInt				= add_type  ("int",			sizeof(int));
	TypeIntP			= add_type_p("int*",		TypeInt);
	TypeIntPs			= add_type_p("int&",		TypeInt, true);
	TypeIntList			= add_type_a("int[]",		TypeInt, -1);
	TypeIntListPs		= add_type_p("int[]&",		TypeIntList);
	TypeIntArray		= add_type_a("int[?]",		TypeInt, 1);
	TypeIntArrayP		= add_type_p("int[?]*",		TypeIntArray);
	TypeFloat			= add_type  ("float",		sizeof(float));
	TypeFloatP			= add_type_p("float*",		TypeFloat);
	TypeFloatPs			= add_type_p("float&",		TypeFloat, true);
	TypeFloatArray		= add_type_a("float[?]",	TypeFloat, 1);
	TypeFloatArrayP		= add_type_p("float[?]*",	TypeFloatArray);
	TypeFloatList		= add_type_a("float[]",		TypeFloat, -1);
	TypeFloatListPs		= add_type_p("float[]&",	TypeFloatList, true);
sType *TypeFloatArray3	= add_type_a("float[3]",		TypeFloat, 3);
	TypeFloatArray3x3	= add_type_a("float[3][3]",	TypeFloatArray3, 3);
sType *TypeFloatArray4	= add_type_a("float[4]",		TypeFloat, 4);
	TypeFloatArray4x4	= add_type_a("float[4][4]",	TypeFloatArray4, 4);
	TypeComplex			= add_type  ("complex",		sizeof(float) * 2);
	TypeComplexList		= add_type_a("complex[]",	TypeComplex, -1);
	TypeComplexListPs	= add_type_p("complex[]&",	TypeComplexList, true);
	TypeChar			= add_type  ("char",		sizeof(char));
	TypeCharP			= add_type_p("char*",		TypeChar);
	TypeString			= add_type_a("string",		TypeChar, 256);	// string := char[256]
	TypeStringP			= add_type_p("string*",		TypeString);
	TypeStringPP		= add_type_p("string**",	TypeStringP);
	TypeStringList		= add_type_a("string[]",	TypeString, -1);
	TypeVector			= add_type  ("vector",		sizeof(vector));
	TypeVectorP			= add_type_p("vector*",		TypeVector);
	TypeVectorPs		= add_type_p("vector&",		TypeVector, true);
	TypeVectorArray		= add_type_a("vector[?]",	TypeVector, 1);
	TypeVectorArrayP	= add_type_p("vector[?]*",	TypeVectorArray);
	TypeVectorList		= add_type_a("vector[]",	TypeVector, -1);
	TypeRect			= add_type  ("rect",		sizeof(rect));
	TypeRectP			= add_type_p("rect*",		TypeRect);
	TypeRectPs			= add_type_p("rect&",		TypeRect, true);
	TypeMatrix			= add_type  ("matrix",		sizeof(matrix));
	TypeMatrixP			= add_type_p("matrix*",		TypeMatrix);
	TypeMatrixPs		= add_type_p("matrix&",		TypeMatrix, true);
	TypeQuaternion		= add_type  ("quaternion",	sizeof(quaternion));
	TypeQuaternionP		= add_type_p("quaternion*",	TypeQuaternion);
	TypeQuaternionPs	= add_type_p("quaternion&",	TypeQuaternion, true);
	TypePlane			= add_type  ("plane",		sizeof(plane));
	TypePlaneP			= add_type_p("plane*",		TypePlane);
	TypePlanePs			= add_type_p("plane&",		TypePlane, true);
	TypeColor			= add_type  ("color",		sizeof(color));
	TypeColorP			= add_type_p("color*",		TypeColor);
	TypeColorPs			= add_type_p("color&",		TypeColor, true);
	TypeMatrix3			= add_type  ("matrix3",		sizeof(matrix3));
	TypeObject			= add_type  ("-CObject-",	0);
	TypeObjectP			= add_type_p("object",		TypeObject);
	TypeObjectPPs		= add_type_p("object&",		TypeObjectP, true);
	TypeObjectPList		= add_type_a("object[]",	TypeObjectP, -1);
	TypeObjectPListPs	= add_type_p("object[]&",	TypeObjectPList, true);
	TypeModel			= add_type  ("-CModel-",	0);
	TypeModelP			= add_type_p("model",		TypeModel);
	TypeModelPP			= add_type_p("model*",		TypeModelP);
	TypeModelPList		= add_type_a("model[]",		TypeModelP, -1);
	TypeBone			= add_type  ("bone",		sizeof_sModelBone);
	TypeBoneList		= add_type_a("bone[]",		TypeBone, -1);
	TypeItem			= add_type  ("-sItem-",		0);
	TypeItemP			= add_type_p("item",		TypeItem);
	TypeItemPList		= add_type_a("item[]",		TypeItemP, -1);
	TypeFile			= add_type  ("-CFile-",		0);
	TypeFileP			= add_type_p("file",		TypeFile);
	TypeDate			= add_type  ("date",		sizeof(sDate));
	TypeText			= add_type  ("-sText-",		0);
	TypeTextP			= add_type_p("text",		TypeText);
	TypePicture			= add_type  ("-sPicture-",	0);
	TypePictureP		= add_type_p("picture",		TypePicture);
	TypePicture3D		= add_type  ("-sPicture3d-",0);
	TypePicture3DP		= add_type_p("picture3d",	TypePicture3D);
	TypeGrouping		= add_type  ("-sGrouping-",	0);
	TypeGroupingP		= add_type_p("grouping",	TypeGrouping);
	TypeParticle		= add_type  ("-sParticle-",	0);
	TypeParticleP		= add_type_p("particle",	TypeParticle);
	TypeBeam			= add_type  ("-sBeam-",		0);
	TypeBeamP			= add_type_p("beam",		TypeBeam);
	TypeEffect			= add_type  ("-sEffect-",	0);
	TypeEffectP			= add_type_p("effect",		TypeEffect);
	TypeView			= add_type  ("-CView-",		0);
	TypeViewP			= add_type_p("view",		TypeView);
	TypeSkin			= add_type  ("-sSkin-",		0);
	TypeSkinP			= add_type_p("skin",		TypeSkin);
	TypeSkinPArray		= add_type_a("skin[?]",		TypeSkinP, 1);
	TypeMaterial		= add_type  ("material",	0);
	TypeMaterialArray	= add_type_a("material[?]",	TypeMaterial, 1);
	TypeMaterialArrayP	= add_type_p("material[?]*",TypeMaterialArray);
	TypeFog				= add_type  ("fog",			0);
	TypeTerrain			= add_type  ("-CTerrain-",	0);
	TypeTerrainP		= add_type_p("terrain",		TypeTerrain);
	TypeTerrainPP		= add_type_p("terrain*",	TypeTerrainP);
	TypeTerrainPList	= add_type_a("terrain[]",	TypeTerrainP, -1);

	msg_db_l(1);
}

void SIAddBasicCommands()
{
	msg_db_r("SIAddBasicCommands", 1);

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
	CommandComplexSet,
	CommandVectorSet,
	CommandRectSet,
	CommandColorSet,
	CommandAsm,
*/


// "intern" functions
	add_func("return",		TypeVoid,	NULL, FunctionSpecial);
		func_add_param("return_value",	TypeVoid); // return: ParamType will be defined by the parser!
	add_func("-if-",		TypeVoid,	NULL, FunctionSpecial);
		func_add_param("b",	TypeBool);
	add_func("-if/else-",	TypeVoid,	NULL, FunctionSpecial);
		func_add_param("b",	TypeBool);
	add_func("-while-",		TypeVoid,	NULL, FunctionSpecial);
		func_add_param("b",	TypeBool);
	add_func("-for-",		TypeVoid,	NULL, FunctionSpecial);
	add_func("-break-",		TypeVoid,	NULL, FunctionSpecial);
	add_func("-continue-",	TypeVoid,	NULL, FunctionSpecial);
	add_func("sizeof",		TypeInt,	NULL, FunctionSpecial);
		func_add_param("type",	TypeVoid);
	
	add_func("wait",		TypeVoid,	NULL, FunctionSpecial);
		func_add_param("time",	TypeFloat);
	add_func("wait_rt",		TypeVoid,	NULL, FunctionSpecial);
		func_add_param("time",	TypeFloat);
	add_func("wait_of",		TypeVoid,	NULL, FunctionSpecial);
	add_func("f2i",			TypeInt,	(void*)&_Float2Int);
//	add_func("f2i",			TypeInt,	NULL, FunctionSpecial);    // sometimes causes floating point exceptions...
		func_add_param("f",		TypeFloat);
	add_func("i2f",			TypeFloat,	NULL, FunctionSpecial);
		func_add_param("i",		TypeInt);
	add_func("i2c",			TypeChar,	NULL, FunctionSpecial);
		func_add_param("i",		TypeInt);
	add_func("c2i",			TypeInt,	NULL, FunctionSpecial);
		func_add_param("c",		TypeChar);
	add_func("p2b",			TypeBool,	NULL, FunctionSpecial);
		func_add_param("p",		TypePointer);
	add_func("complex",		TypeComplex,	NULL, FunctionSpecial);
		func_add_param("x",		TypeFloat);
		func_add_param("y",		TypeFloat);
	add_func("vector",		TypeVector,	NULL, FunctionSpecial);
		func_add_param("x",		TypeFloat);
		func_add_param("y",		TypeFloat);
		func_add_param("z",		TypeFloat);
	add_func("rect",		TypeRect,	NULL, FunctionSpecial);
		func_add_param("x1",	TypeFloat);
		func_add_param("x2",	TypeFloat);
		func_add_param("y1",	TypeFloat);
		func_add_param("y2",	TypeFloat);
	add_func("color",		TypeColor,	NULL, FunctionSpecial);
		func_add_param("a",		TypeFloat);
		func_add_param("r",		TypeFloat);
		func_add_param("g",		TypeFloat);
		func_add_param("b",		TypeFloat);
	add_func("-asm-",		TypeVoid,	NULL, FunctionSpecial);
	
	msg_db_l(1);
}

void SIAddClasses()
{
	msg_db_r("SIAddClasses", 1);
	
	
	add_class(TypeComplex);
		class_add_element("x",		TypeFloat,	0);
		class_add_element("y",		TypeFloat,	4);
	
	add_class(TypeVector);
		class_add_element("x",		TypeFloat,	0);
		class_add_element("y",		TypeFloat,	4);
		class_add_element("z",		TypeFloat,	8);
	
	add_class(TypeQuaternion);
		class_add_element("x",		TypeFloat,	0);
		class_add_element("y",		TypeFloat,	4);
		class_add_element("z",		TypeFloat,	8);
		class_add_element("w",		TypeFloat,	12);
	
	add_class(TypeRect);
		class_add_element("x1",	TypeFloat,	0);
		class_add_element("x2",	TypeFloat,	4);
		class_add_element("y1",	TypeFloat,	8);
		class_add_element("y2",	TypeFloat,	12);
	
	add_class(TypeColor);
		class_add_element("a",		TypeFloat,	12);
		class_add_element("r",		TypeFloat,	0);
		class_add_element("g",		TypeFloat,	4);
		class_add_element("b",		TypeFloat,	8);
	
	add_class(TypePlane);
		class_add_element("a",		TypeFloat,	0);
		class_add_element("b",		TypeFloat,	4);
		class_add_element("c",		TypeFloat,	8);
		class_add_element("d",		TypeFloat,	12);
	
	add_class(TypeMatrix);
		class_add_element("_00",	TypeFloat,	0);
		class_add_element("_10",	TypeFloat,	4);
		class_add_element("_20",	TypeFloat,	8);
		class_add_element("_30",	TypeFloat,	12);
		class_add_element("_01",	TypeFloat,	16);
		class_add_element("_11",	TypeFloat,	20);
		class_add_element("_21",	TypeFloat,	24);
		class_add_element("_31",	TypeFloat,	28);
		class_add_element("_02",	TypeFloat,	32);
		class_add_element("_12",	TypeFloat,	36);
		class_add_element("_22",	TypeFloat,	40);
		class_add_element("_32",	TypeFloat,	44);
		class_add_element("_03",	TypeFloat,	48);
		class_add_element("_13",	TypeFloat,	52);
		class_add_element("_23",	TypeFloat,	56);
		class_add_element("_33",	TypeFloat,	60);
		class_add_element("e",		TypeFloatArray4x4,	0);
		class_add_element("_e",		TypeFloatArray,	0);
	
	add_class(TypeMatrix3);
		class_add_element("_11",	TypeFloat,	0);
		class_add_element("_21",	TypeFloat,	4);
		class_add_element("_31",	TypeFloat,	8);
		class_add_element("_12",	TypeFloat,	12);
		class_add_element("_22",	TypeFloat,	16);
		class_add_element("_32",	TypeFloat,	20);
		class_add_element("_13",	TypeFloat,	24);
		class_add_element("_23",	TypeFloat,	28);
		class_add_element("_33",	TypeFloat,	32);
		class_add_element("e",		TypeFloatArray3x3,	0);
		class_add_element("_e",		TypeFloatArray,	0);
	
	add_class(TypePicture);
		class_add_element("Enabled",		TypeBool,		GetDAPicture(Enabled));
		class_add_element("TCInverted",		TypeBool,		GetDAPicture(TCInverted));
		class_add_element("Pos",			TypeVector,		GetDAPicture(Pos));
		class_add_element("Width",			TypeFloat,		GetDAPicture(Width));
		class_add_element("Height",			TypeFloat,		GetDAPicture(Height));
		class_add_element("Color",			TypeColor,		GetDAPicture(Color));
		class_add_element("Texture",		TypeInt,		GetDAPicture(Texture));
		class_add_element("Source",			TypeRect,		GetDAPicture(Source));
		class_add_element("ShaderFile",		TypeInt,		GetDAPicture(ShaderFile));
	
	add_class(TypePicture3D);
		class_add_element("Enabled",		TypeBool,		GetDAPicture3D(Enabled));
		class_add_element("Relative",		TypeBool,		GetDAPicture3D(Relative));
		class_add_element("Lighting",		TypeBool,		GetDAPicture3D(Lighting));
		class_add_element("World3D",		TypeBool,		GetDAPicture3D(World3D));
		class_add_element("z",				TypeFloat,		GetDAPicture3D(z));
		class_add_element("Matrix",			TypeMatrix,		GetDAPicture3D(Matrix));
		class_add_element("Model",			TypeModelP,		GetDAPicture3D(model));
	
	add_class(TypeGrouping);
		class_add_element("Enabled",		TypeBool,		GetDAGrouping(Enabled));
		class_add_element("Pos",			TypeVector,		GetDAGrouping(Pos));
		class_add_element("Color",			TypeColor,		GetDAGrouping(Color));
	
	add_class(TypeText);
		class_add_element("Enabled",		TypeBool,		GetDAText(Enabled));
		class_add_element("Centric",		TypeBool,		GetDAText(Centric));
		class_add_element("Vertical",		TypeBool,		GetDAText(Vertical));
		class_add_element("Font",			TypeInt,		GetDAText(Font));
		class_add_element("Pos",			TypeVector,		GetDAText(Pos));
		class_add_element("Size",			TypeFloat,		GetDAText(Size));
		class_add_element("Color",			TypeColor,		GetDAText(Color));
		class_add_element("Str",			TypeString,		GetDAText(Str));
	
	add_class(TypeParticle);
		class_add_element("Enabled",		TypeBool,		GetDAParticle(Enabled));
		class_add_element("Suicidal",		TypeBool,		GetDAParticle(Suicidal));
		class_add_element("Pos",			TypeVector,		GetDAParticle(Pos));
		class_add_element("Vel",			TypeVector,		GetDAParticle(Vel));
		class_add_element("Ang",			TypeVector,		GetDAParticle(Parameter));
		class_add_element("TimeToLive",		TypeFloat,		GetDAParticle(TimeToLive));
		class_add_element("Radius",			TypeFloat,		GetDAParticle(Radius));
		class_add_element("Color",			TypeColor,		GetDAParticle(Color));
		class_add_element("Texture",		TypeInt,		GetDAParticle(Texture));
		class_add_element("Source",			TypeRect,		GetDAParticle(Source));
		class_add_element("FuncDeltaT",		TypeFloat,		GetDAParticle(FuncDeltaT));
		class_add_element("Elapsed",		TypeFloat,		GetDAParticle(elapsed));
		class_add_element("Func",			TypePointer,	GetDAParticle(func));

	add_class(TypeBeam);
		class_add_element("Enabled",		TypeBool,		GetDAParticle(Enabled));
		class_add_element("Suicidal",		TypeBool,		GetDAParticle(Suicidal));
		class_add_element("Pos",			TypeVector,		GetDAParticle(Pos));
		class_add_element("Vel",			TypeVector,		GetDAParticle(Vel));
		class_add_element("Length",			TypeVector,		GetDAParticle(Parameter));
		class_add_element("TimeToLive",		TypeFloat,		GetDAParticle(TimeToLive));
		class_add_element("Radius",			TypeFloat,		GetDAParticle(Radius));
		class_add_element("Color",			TypeColor,		GetDAParticle(Color));
		class_add_element("Texture",		TypeInt,		GetDAParticle(Texture));
		class_add_element("Source",			TypeRect,		GetDAParticle(Source));
		class_add_element("FuncDeltaT",		TypeFloat,		GetDAParticle(FuncDeltaT));
		class_add_element("Elapsed",		TypeFloat,		GetDAParticle(elapsed));
		class_add_element("Func",			TypePointer,	GetDAParticle(func));
	
	add_class(TypeEffect);
		class_add_element("Enabled",		TypeBool,		GetDAEffect(Enabled));
		class_add_element("Used",			TypeBool,		GetDAEffect(Used));
		class_add_element("Suicidal",		TypeBool,		GetDAEffect(Suicidal));
		class_add_element("Pos",			TypeVector,		GetDAEffect(Pos));
		class_add_element("Vel",			TypeVector,		GetDAEffect(Vel));
		class_add_element("TimeToLive",		TypeFloat,		GetDAEffect(TimeToLive));
		class_add_element("Var",			TypeFloatList,	GetDAEffect(ScriptVar));
		class_add_element("VarI",			TypeIntList,	GetDAEffect(ScriptVar));
		class_add_element("VarP",			TypePointerList,GetDAEffect(ScriptVar));
		class_add_element("FuncDeltaT",		TypeFloat,		GetDAEffect(FuncDeltaT));
		class_add_element("Elapsed",		TypeFloat,		GetDAEffect(elapsed));
		class_add_element("Func",			TypePointer,	GetDAEffect(func));
		class_add_element("DelFunc",		TypePointer,	GetDAEffect(del_func));
		class_add_element("Model",			TypeModelP,		GetDAEffect(model));
		class_add_element("Vertex",			TypeInt,		GetDAEffect(vertex));
	
	add_class(TypeObject);
		class_add_element("Pos",			TypeVector,		GetDAObject(Pos));
		class_add_element("Vel",			TypeVector,		GetDAObject(Vel));
		class_add_element("VelS",			TypeVector,		GetDAObject(VelS));
		class_add_element("Ang",			TypeVector,		GetDAObject(Ang));
		class_add_element("Rot",			TypeVector,		GetDAObject(Rot));
		class_add_element("Matrix",			TypeMatrixP,	GetDAObject(Matrix));
		class_add_element("Name",			TypeString,		GetDAObject(Name));
		class_add_element("OnGround",		TypeBool,		GetDAObject(OnGround));
		class_add_element("GroundID",		TypeInt,		GetDAObject(GroundID));
		class_add_element("GroundNormal",	TypeVector,		GetDAObject(GroundNormal));
		class_add_element("GFactor",		TypeFloat,		GetDAObject(GFactor));
		class_add_element("ID",				TypeInt,		GetDAObject(ID));
		class_add_element("Visible",		TypeBool,		GetDAObject(Visible));
		class_add_element("ActivePhysics",	TypeBool,		GetDAObject(ActivePhysics));
		class_add_element("PassivePhysics",	TypeBool,		GetDAObject(PassivePhysics));
		class_add_element("Model",			TypeModelP,		GetDAObject(model));
		class_add_element("Mass",			TypeFloat,		GetDAObject(Mass));
		class_add_element("Theta",			TypeMatrix3,	GetDAObject(Theta));
		class_add_element("Radius",			TypeFloat,		GetDAObject(Radius));
		class_add_element("Life",			TypeFloat,		GetDAObject(Life));
		class_add_element("MaxLife",		TypeFloat,		GetDAObject(MaxLife));
		class_add_element("Var",			TypeFloatList,	GetDAObject(ScriptVar));
		class_add_element("VarI",			TypeIntList,	GetDAObject(ScriptVar));
		class_add_element("VarP",			TypePointerList,GetDAObject(ScriptVar));
		class_add_element("Item",			TypeItemPList,	GetDAObject(items));
		class_add_element("ItemFilename",	TypeStringP,	GetDAObject(ItemFilename));
		class_add_func("AddForce",		TypeVoid,	god_p(mf((tmf)&CObject::AddForce)));
			func_add_param("force",		TypeVectorPs);
			func_add_param("rho",		TypeVectorPs);
		class_add_func("AddTorque",		TypeVoid,	god_p(mf((tmf)&CObject::AddTorque)));
			func_add_param("torque",		TypeVectorPs);

	add_class(TypeSkin);
		class_add_element("NumVertices",		TypeInt,			GetDASkin(NumVertices));
		class_add_element("Vertex",				TypeVectorArrayP,	GetDASkin(Vertex));
		class_add_element("NumSkinVertices",	TypeInt,			GetDASkin(NumSkinVertices));
		class_add_element("SkinVertex",			TypeFloatArrayP,	GetDASkin(SkinVertex));
		class_add_element("NumTriangles",		TypeInt,			GetDASkin(NumTriangles));
		class_add_element("TriangleIndex",		TypeIntArrayP,		GetDASkin(TriangleIndex));
		class_add_element("TriangleSkinIndex",	TypeIntArrayP,		GetDASkin(TriangleSkinIndex));
		class_add_element("Normal",				TypeVectorArrayP,	GetDASkin(Normal));

	add_class(TypeMaterial);
		class_add_element("NumTextures",	TypeInt,		GetDAMaterial(NumTextures));
		class_add_element("Texture",		TypeIntArray,	GetDAMaterial(Texture));
		class_add_element("ShaderFile",		TypeInt,		GetDAMaterial(ShaderFile));
		class_add_element("AlphaFactor",	TypeFloat,		GetDAMaterial(AlphaFactor));
		class_add_element("Ambient",		TypeColor,		GetDAMaterial(Ambient));
		class_add_element("Diffuse",		TypeColor,		GetDAMaterial(Diffuse));
		class_add_element("Specular",		TypeColor,		GetDAMaterial(Specular));
		class_add_element("Emission",		TypeColor,		GetDAMaterial(Emission));
		class_add_element("Shininess",		TypeFloat,		GetDAMaterial(Shininess));

	add_class(TypeItem);
		class_add_element("Kind",			TypeInt,		GetDAItem(Kind));
		class_add_element("OID",			TypeInt,		GetDAItem(OID));
		class_add_element("Quantity",		TypeInt,		GetDAItem(Quantity));
		class_add_element("QuantityMax",	TypeInt,		GetDAItem(QuantityMax));
		class_add_element("Var",			TypeFloatList,	GetDAItem(ScriptVar));
		class_add_element("Model",			TypeModelP,		GetDAItem(model));
		class_add_element("Name",			TypeString,		GetDAItem(Name[0]));
		class_add_element("Description",	TypeString,		GetDAItem(Description[0]));

	add_class(TypeFog);
		class_add_element("Enabled",		TypeBool,		GetDAFog(Enabled));
		class_add_element("Mode",			TypeInt,		GetDAFog(Mode));
		class_add_element("Start",			TypeFloat,		GetDAFog(Start));
		class_add_element("End",			TypeFloat,		GetDAFog(End));
		class_add_element("Density",		TypeFloat,		GetDAFog(Density));
		class_add_element("Color",			TypeColor,		GetDAFog(Color));

	add_class(TypeDate);
		class_add_element("Time",			TypeInt,		GetDADate(time));
		class_add_element("Year",			TypeInt,		GetDADate(year));
		class_add_element("Month",			TypeInt,		GetDADate(month));
		class_add_element("Day",			TypeInt,		GetDADate(day));
		class_add_element("Hour",			TypeInt,		GetDADate(hour));
		class_add_element("Minute",			TypeInt,		GetDADate(minute));
		class_add_element("Second",			TypeInt,		GetDADate(second));
		class_add_element("MilliSecond",	TypeInt,		GetDADate(milli_second));
		class_add_element("DayOfWeek",		TypeInt,		GetDADate(day_of_week));
		class_add_element("DayOfYear",		TypeInt,		GetDADate(day_of_year));
	
	add_class(TypeFile);
		class_add_func("GetDate",		TypeDate,		mf((tmf)&CFile::GetDate));
			func_add_param("type",		TypeInt);
		class_add_func("SetBinaryMode",TypeVoid,		mf((tmf)&CFile::SetBinaryMode));
			func_add_param("binary",	TypeBool);
		class_add_func("WriteBool",	TypeVoid,			mf((tmf)&CFile::WriteBool));
			func_add_param("b",			TypeBool);
		class_add_func("WriteInt",		TypeVoid,			mf((tmf)&CFile::WriteInt));
			func_add_param("i",			TypeInt);
		class_add_func("WriteFloat",	TypeVoid,			mf((tmf)&CFile::WriteFloat));
			func_add_param("x",			TypeFloat);
		class_add_func("WriteStr",		TypeVoid,			mf((tmf)&CFile::WriteStr));//(void*)&FileWriteStr);
			func_add_param("s",			TypeString);
//		class_add_func("WriteStr",		TypeVoid,			mf((tmf)&CFile::WriteStr));
//			func_add_param("s",			TypeString);
		class_add_func("ReadBool",		TypeBool,			mf((tmf)&CFile::ReadBool));
		class_add_func("ReadInt",		TypeInt,				mf((tmf)&CFile::ReadInt));
		class_add_func("ReadFloat",	TypeFloat,			mf((tmf)&CFile::ReadFloat));
		class_add_func("ReadStr",		TypeString,			mf((tmf)&CFile::ReadStr));
		class_add_func("ReadBoolC",	TypeBool,			mf((tmf)&CFile::ReadBoolC));
		class_add_func("ReadIntC",		TypeInt,			mf((tmf)&CFile::ReadIntC));
		class_add_func("ReadFloatC",	TypeFloat,			mf((tmf)&CFile::ReadFloatC));
		class_add_func("ReadStrC",		TypeString,			mf((tmf)&CFile::ReadStrC));

	add_class(TypeBone);
		class_add_element("Root",		TypeInt,		GetDABone(Root));
		class_add_element("Pos",		TypeVector,		GetDABone(Pos));
		class_add_element("Model",		TypeModelP,		GetDABone(Model));
		class_add_element("DMatrix",	TypeMatrix,		GetDABone(DMatrix));
		class_add_element("CurAng",		TypeQuaternion,	GetDABone(CurAng));
		class_add_element("CurPos",		TypeVector,		GetDABone(CurPos));

	add_class(TypeModel);
		class_add_element("Skin",			TypeSkinPArray,	GetDAModel(Skin));
		class_add_element("Skin0",			TypeSkinP,		GetDAModel(Skin[0]));
		class_add_element("Skin1",			TypeSkinP,		GetDAModel(Skin[1]));
		class_add_element("Skin2",			TypeSkinP,		GetDAModel(Skin[2]));
		class_add_element("NumMaterials",	TypeInt,		GetDAModel(NumMaterials));
		class_add_element("Material",		TypeMaterialArrayP,	GetDAModel(Material));
		class_add_element("Theta",			TypeMatrix3,	GetDAModel(Theta));
		class_add_element("Bone",			TypeBoneList,	GetDAModel(Bone_ma));
		class_add_element("Matrix",			TypeMatrix,		GetDAModel(Matrix));
		class_add_element("Min",			TypeVector,		GetDAModel(Min));
		class_add_element("Max",			TypeVector,		GetDAModel(Max));
		class_add_element("TestCollisions",	TypeBool,		GetDAModel(TestCollisions));
		class_add_element("AllowShadow",	TypeBool,		GetDAModel(AllowShadow));
		class_add_element("Object",			TypeObjectP,	GetDAModel(object));
/*	add_func("CalcMove",					TypeVoid,		mod_p(mf((tmf)&CModel::CalcMove)));
	add_func("Draw",						TypeVoid,		mod_p(mf((tmf)&CModel::Draw)));
		func_add_param("skin",				TypeInt);
		func_add_param("mat",				TypeMatrixPs);
		func_add_param("fx",				TypeBool);*/
		class_add_func("GetVertex",		TypeVector,		mod_p(mf((tmf)&CModel::GetVertex)));
			func_add_param("index",			TypeInt);
			func_add_param("skin",			TypeInt);
		class_add_func("ResetMove",		TypeVoid,		mod_p(mf((tmf)&CModel::ResetMove)));
		class_add_func("Move",				TypeBool,		mod_p(mf((tmf)&CModel::Move)));
			func_add_param("operation",		TypeInt);
			func_add_param("param1",		TypeFloat);
			func_add_param("param2",		TypeFloat);
			func_add_param("move",			TypeInt);
			func_add_param("time",			TypeFloatPs);
			func_add_param("dt",			TypeFloat);
			func_add_param("v",				TypeFloat);
			func_add_param("loop",			TypeBool);
		class_add_func("GetFrames",		TypeInt,		mod_p(mf((tmf)&CModel::GetFrames)));
			func_add_param("move",			TypeInt);
		class_add_func("BeginEditMove",	TypeVoid,		mod_p(mf((tmf)&CModel::BeginEditMove)));
		class_add_func("MakeEditable",		TypeVoid,		mod_p(mf((tmf)&CModel::MakeEditable)));
		class_add_func("BeginEdit",		TypeVoid,		mod_p(mf((tmf)&CModel::BeginEdit)));
			func_add_param("skin",			TypeInt);
		class_add_func("EndEdit",			TypeVoid,		mod_p(mf((tmf)&CModel::EndEdit)));
			func_add_param("skin",			TypeInt);
		class_add_func("SetBoneModel",		TypeVoid,		mod_p(mf((tmf)&CModel::SetBoneModel)));
			func_add_param("index",			TypeInt);
			func_add_param("bone",			TypeModelP);

	add_class(TypeTerrain);
		class_add_element("Pos",			TypeVector,		GetDATerrain(Pos));
		class_add_element("NumX",			TypeInt,		GetDATerrain(NumX));
		class_add_element("NumZ",			TypeInt,		GetDATerrain(NumZ));
		class_add_element("Height",			TypeFloatArrayP,GetDATerrain(Height));
		class_add_element("Pattern",		TypeVector,		GetDATerrain(Pattern));
		class_add_element("NumTextures",	TypeInt,		GetDATerrain(NumTextures));
		class_add_element("Texture",		TypeIntArray,	GetDATerrain(Texture));
		class_add_element("TextureScale",	TypeVectorArray,GetDATerrain(TextureScale));
		class_add_func("Update",			TypeVoid,		god_p(mf((tmf)&CTerrain::Update)));
			func_add_param("x1",		TypeInt);
			func_add_param("x2",		TypeInt);
			func_add_param("z1",		TypeInt);
			func_add_param("z2",		TypeInt);
			func_add_param("mode",		TypeInt);

	add_class(TypeView);
		class_add_element("Enabled",		TypeBool,		GetDAView(Enabled));
		class_add_element("Show",			TypeBool,		GetDAView(Show));
		class_add_element("OutputTexture",	TypeInt,		GetDAView(OutputTexture));
		class_add_element("InputTexture",	TypeInt,		GetDAView(InputTexture));
		class_add_element("ShaderFile",	TypeInt,		GetDAView(ShaderFile));
		class_add_element("ShadedDisplays",TypeBool,		GetDAView(ShadedDisplays));
		class_add_element("Pos",			TypeVector,		GetDAView(Pos));
		class_add_element("Ang",			TypeVector,		GetDAView(Ang));
		class_add_element("Vel",			TypeVector,		GetDAView(Vel));
		class_add_element("Rot",			TypeVector,		GetDAView(Rot));
		class_add_element("Zoom",			TypeFloat,		GetDAView(Zoom));
		class_add_element("Dest",			TypeRect,		GetDAView(Dest));
		class_add_element("z",				TypeFloat,		GetDAView(z));
		class_add_func("StartScript",		TypeVoid,	cam_p(mf((tmf)&CView::StartScript)));
			func_add_param("filename",		TypeString);
			func_add_param("dpos",			TypeVector);
		class_add_func("StopScript",		TypeVoid,	cam_p(mf((tmf)&CView::StopScript)));
	
	msg_db_l(1);
}

void SIAddOperators()
{
	msg_db_r("SIAddOperators", 1);
	

	// same order as in .h file...
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
//	add_operator(OperatorAssign,		TypeVoid,		TypeComplex,	TypeComplex);
	add_operator(OperatorAdd,			TypeComplex,	TypeComplex,	TypeComplex);
	add_operator(OperatorSubtract,		TypeComplex,	TypeComplex,	TypeComplex);
	add_operator(OperatorMultiply,		TypeComplex,	TypeComplex,	TypeComplex);
	add_operator(OperatorMultiply,		TypeComplex,	TypeFloat,		TypeComplex);
	add_operator(OperatorMultiply,		TypeComplex,	TypeComplex,	TypeFloat);
	add_operator(OperatorDivide,		TypeComplex,	TypeComplex,	TypeComplex);
	add_operator(OperatorAddS,			TypeVoid,		TypeComplex,	TypeComplex);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeComplex,	TypeComplex);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeComplex,	TypeComplex);
	add_operator(OperatorDivideS,		TypeVoid,		TypeComplex,	TypeComplex);
	add_operator(OperatorEqual,			TypeBool,		TypeComplex,	TypeComplex);
	add_operator(OperatorSubtract,		TypeComplex,	TypeVoid,		TypeComplex);
	add_operator(OperatorAssign,		TypeVoid,		TypeString,		TypeString);
//	add_operator(OperatorAssign,		TypeVoid,		TypeString,		TypeStringP);
	add_operator(OperatorAdd,			TypeString,		TypeString,		TypeString);
/*	add_operator(OperatorAdd,			TypeString,		TypeString,		TypeStringP);
	add_operator(OperatorAdd,			TypeString,		TypeStringP,	TypeString);
	add_operator(OperatorAdd,			TypeString,		TypeStringP,	TypeStringP);*/
	add_operator(OperatorAddS,			TypeVoid,		TypeString,		TypeString);
//	add_operator(OperatorAddS,			TypeVoid,		TypeString,		TypeStringP);
	add_operator(OperatorEqual,			TypeBool,		TypeString,		TypeString);
/*	add_operator(OperatorEqual,			TypeBool,		TypeStringP,	TypeString);
	add_operator(OperatorEqual,			TypeBool,		TypeString,		TypeStringP);
	add_operator(OperatorEqual,			TypeBool,		TypeStringP,	TypeStringP);*/
	add_operator(OperatorNotEqual,		TypeBool,		TypeString,		TypeString);
/*	add_operator(OperatorNotEqual,		TypeBool,		TypeStringP,	TypeString);
	add_operator(OperatorNotEqual,		TypeBool,		TypeString,		TypeStringP);
	add_operator(OperatorNotEqual,		TypeBool,		TypeStringP,	TypeStringP);*/
	add_operator(OperatorAssign,		TypeVoid,		TypeClass,		TypeClass);
	add_operator(OperatorEqual,			TypeBool,		TypeClass,		TypeClass);
	add_operator(OperatorNotEqual,		TypeBool,		TypeClass,		TypeClass);
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
	
	msg_db_l(1);
}

void SIAddConsts()
{
	msg_db_r("SIAddConsts", 1);

	add_const("nil", TypePointer, NULL);
	// bool
	add_const("false",TypeBool,(void*)false);
	add_const("true",TypeBool,(void*)true);
	// float
	add_const("pi",TypeFloat,*(void**)&pi);
	// complex
	add_const("ci",TypeComplex,(void**)&ci);
	// vector
	add_const("v0",TypeVector,(void*)&v0);
	add_const("e_x",TypeVector,(void*)&e_x);
	add_const("e_y",TypeVector,(void*)&e_y);
	add_const("e_z",TypeVector,(void*)&e_z);
	// matrix
	//add_const("MatrixID",TypeMatrix,(void*)&m_id);
	add_const("m_id",TypeMatrix,(void*)&m_id);
	// quaternion
	//add_const("QuaternionID",TypeVector,(void*)&q_id);
	add_const("q_id",TypeQuaternion,(void*)&q_id);
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
	//add_const("Rect01",TypeRect,(void*)&r01);
	add_const("r01",TypeRect,(void*)&r01);
	// key ids (int)
	add_const("KeyControl",TypeInt,(void*)KEY_CONTROL);
	add_const("KeyControlL",TypeInt,(void*)KEY_LCONTROL);
	add_const("KeyControlR",TypeInt,(void*)KEY_RCONTROL);
	add_const("KeyShift",TypeInt,(void*)KEY_SHIFT);
	add_const("KeyShiftL",TypeInt,(void*)KEY_LSHIFT);
	add_const("KeyShiftR",TypeInt,(void*)KEY_RSHIFT);
	add_const("KeyAlt",TypeInt,(void*)KEY_ALT);
	add_const("KeyAltL",TypeInt,(void*)KEY_LALT);
	add_const("KeyAltR",TypeInt,(void*)KEY_RALT);
	add_const("KeyPlus",TypeInt,(void*)KEY_ADD);
	add_const("KeyMinus",TypeInt,(void*)KEY_SUBTRACT);
	add_const("KeyFence",TypeInt,(void*)KEY_FENCE);
	add_const("KeyEnd",TypeInt,(void*)KEY_END);
	add_const("KeyNext",TypeInt,(void*)KEY_NEXT);
	add_const("KeyPrior",TypeInt,(void*)KEY_PRIOR);
	add_const("KeyUp",TypeInt,(void*)KEY_UP);
	add_const("KeyDown",TypeInt,(void*)KEY_DOWN);
	add_const("KeyLeft",TypeInt,(void*)KEY_LEFT);
	add_const("KeyRight",TypeInt,(void*)KEY_RIGHT);
	add_const("KeyReturn",TypeInt,(void*)KEY_RETURN);
	add_const("KeyEscape",TypeInt,(void*)KEY_ESCAPE);
	add_const("KeyInsert",TypeInt,(void*)KEY_INSERT);
	add_const("KeyDelete",TypeInt,(void*)KEY_DELETE);
	add_const("KeySpace",TypeInt,(void*)KEY_SPACE);
	add_const("KeyF1",TypeInt,(void*)KEY_F1);
	add_const("KeyF2",TypeInt,(void*)KEY_F2);
	add_const("KeyF3",TypeInt,(void*)KEY_F3);
	add_const("KeyF4",TypeInt,(void*)KEY_F4);
	add_const("KeyF5",TypeInt,(void*)KEY_F5);
	add_const("KeyF6",TypeInt,(void*)KEY_F6);
	add_const("KeyF7",TypeInt,(void*)KEY_F7);
	add_const("KeyF8",TypeInt,(void*)KEY_F8);
	add_const("KeyF9",TypeInt,(void*)KEY_F9);
	add_const("KeyF10",TypeInt,(void*)KEY_F10);
	add_const("KeyF11",TypeInt,(void*)KEY_F11);
	add_const("KeyF12",TypeInt,(void*)KEY_F12);
	add_const("Key0",TypeInt,(void*)KEY_0);
	add_const("Key1",TypeInt,(void*)KEY_1);
	add_const("Key2",TypeInt,(void*)KEY_2);
	add_const("Key3",TypeInt,(void*)KEY_3);
	add_const("Key4",TypeInt,(void*)KEY_4);
	add_const("Key5",TypeInt,(void*)KEY_5);
	add_const("Key6",TypeInt,(void*)KEY_6);
	add_const("Key7",TypeInt,(void*)KEY_7);
	add_const("Key8",TypeInt,(void*)KEY_8);
	add_const("Key9",TypeInt,(void*)KEY_9);
	add_const("KeyA",TypeInt,(void*)KEY_A);
	add_const("KeyB",TypeInt,(void*)KEY_B);
	add_const("KeyC",TypeInt,(void*)KEY_C);
	add_const("KeyD",TypeInt,(void*)KEY_D);
	add_const("KeyE",TypeInt,(void*)KEY_E);
	add_const("KeyF",TypeInt,(void*)KEY_F);
	add_const("KeyG",TypeInt,(void*)KEY_G);
	add_const("KeyH",TypeInt,(void*)KEY_H);
	add_const("KeyI",TypeInt,(void*)KEY_I);
	add_const("KeyJ",TypeInt,(void*)KEY_J);
	add_const("KeyK",TypeInt,(void*)KEY_K);
	add_const("KeyL",TypeInt,(void*)KEY_L);
	add_const("KeyM",TypeInt,(void*)KEY_M);
	add_const("KeyN",TypeInt,(void*)KEY_N);
	add_const("KeyO",TypeInt,(void*)KEY_O);
	add_const("KeyP",TypeInt,(void*)KEY_P);
	add_const("KeyQ",TypeInt,(void*)KEY_Q);
	add_const("KeyR",TypeInt,(void*)KEY_R);
	add_const("KeyS",TypeInt,(void*)KEY_S);
	add_const("KeyT",TypeInt,(void*)KEY_T);
	add_const("KeyU",TypeInt,(void*)KEY_U);
	add_const("KeyV",TypeInt,(void*)KEY_V);
	add_const("KeyW",TypeInt,(void*)KEY_W);
	add_const("KeyX",TypeInt,(void*)KEY_X);
	add_const("KeyY",TypeInt,(void*)KEY_Y);
	add_const("KeyZ",TypeInt,(void*)KEY_Z);
	add_const("KeyBackspace",TypeInt,(void*)KEY_BACKSPACE);
	add_const("KeyTab",TypeInt,(void*)KEY_TAB);
	add_const("KeyHome",TypeInt,(void*)KEY_HOME);
	add_const("KeyNum0",TypeInt,(void*)KEY_NUM_0);
	add_const("KeyNum1",TypeInt,(void*)KEY_NUM_1);
	add_const("KeyNum2",TypeInt,(void*)KEY_NUM_2);
	add_const("KeyNum3",TypeInt,(void*)KEY_NUM_3);
	add_const("KeyNum4",TypeInt,(void*)KEY_NUM_4);
	add_const("KeyNum5",TypeInt,(void*)KEY_NUM_5);
	add_const("KeyNum6",TypeInt,(void*)KEY_NUM_6);
	add_const("KeyNum7",TypeInt,(void*)KEY_NUM_7);
	add_const("KeyNum8",TypeInt,(void*)KEY_NUM_8);
	add_const("KeyNum9",TypeInt,(void*)KEY_NUM_9);
	add_const("KeyNumPlus",TypeInt,(void*)KEY_NUM_ADD);
	add_const("KeyNumMinus",TypeInt,(void*)KEY_NUM_SUBTRACT);
	add_const("KeyNumMultiply",TypeInt,(void*)KEY_NUM_MULTIPLY);
	add_const("KeyNumDivide",TypeInt,(void*)KEY_NUM_DIVIDE);
	add_const("KeyNumComma",TypeInt,(void*)KEY_NUM_COMMA);
	add_const("KeyNumEnter",TypeInt,(void*)KEY_NUM_ENTER);
	add_const("KeyComma",TypeInt,(void*)KEY_COMMA);
	add_const("KeyDot",TypeInt,(void*)KEY_DOT);
	add_const("KeySmaller",TypeInt,(void*)KEY_SMALLER);
	add_const("KeySz",TypeInt,(void*)KEY_SZ);
	add_const("KeyAe",TypeInt,(void*)KEY_AE);
	add_const("KeyOe",TypeInt,(void*)KEY_OE);
	add_const("KeyUe",TypeInt,(void*)KEY_UE);
	add_const("NumKeys",TypeInt,(void*)HUI_NUM_KEYS);
	add_const("KeyAny",TypeInt,(void*)KEY_ANY);
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
	add_const("TraceHitTerrain",TypeInt,god_p(TraceHitTerrain));
	add_const("TraceHitObject",TypeInt,god_p(TraceHitObject));
	// file date
	add_const("FileDateModification",TypeInt,(void*)FileDateModification);
	add_const("FileDateAccess",TypeInt,(void*)FileDateAccess);
	add_const("FileDateCreation",TypeInt,(void*)FileDateCreation);
	// hui window messages
	add_const("HuiMsgClose",TypeInt,(void*)HUI_WIN_CLOSE);
	add_const("HuiMsgSize",TypeInt,(void*)HUI_WIN_SIZE);
	add_const("HuiMsgMove",TypeInt,(void*)HUI_WIN_MOVE);
	add_const("HuiMsgRender",TypeInt,(void*)HUI_WIN_RENDER);
	add_const("HuiMsgMouseMove",TypeInt,(void*)HUI_WIN_MOUSEMOVE);
	add_const("HuiMsgMouseWheel",TypeInt,(void*)HUI_WIN_MOUSEWHEEL);
	add_const("HuiMsgLButtonDown",TypeInt,(void*)HUI_WIN_LBUTTONDOWN);
	add_const("HuiMsgLButtonUp",TypeInt,(void*)HUI_WIN_LBUTTONUP);
	add_const("HuiMsgRButtonDown",TypeInt,(void*)HUI_WIN_RBUTTONDOWN);
	add_const("HuiMsgRButtonUp",TypeInt,(void*)HUI_WIN_RBUTTONUP);
	add_const("HuiMsgKeyDown",TypeInt,(void*)HUI_WIN_KEYDOWN);
	add_const("HuiMsgKeyUp",TypeInt,(void*)HUI_WIN_KEYUP);
	// hui answers
	add_const("HuiYes",TypeInt,(void*)HUI_YES);
	add_const("HuiNo",TypeInt,(void*)HUI_NO);
	add_const("HuiCancel",TypeInt,(void*)HUI_CANCEL);
	// hui images
	add_const("HuiImageAbout",TypeInt,(void*)HuiImageAbout);
	add_const("HuiImageAdd",TypeInt,(void*)HuiImageAdd);
	add_const("HuiImageApply",TypeInt,(void*)HuiImageApply);
	add_const("HuiImageBack",TypeInt,(void*)HuiImageBack);
	add_const("HuiImageCancel",TypeInt,(void*)HuiImageCancel);
	add_const("HuiImageClear",TypeInt,(void*)HuiImageClear);
	add_const("HuiImageClose",TypeInt,(void*)HuiImageClose);
	add_const("HuiImageCopy",TypeInt,(void*)HuiImageCopy);
	add_const("HuiImageCut",TypeInt,(void*)HuiImageCut);
	add_const("HuiImageDelete",TypeInt,(void*)HuiImageDelete);
	add_const("HuiImageDown",TypeInt,(void*)HuiImageDown);
	add_const("HuiImageEdit",TypeInt,(void*)HuiImageEdit);
	add_const("HuiImageExecute",TypeInt,(void*)HuiImageExecute);
	add_const("HuiImageFind",TypeInt,(void*)HuiImageFind);
	add_const("HuiImageFont",TypeInt,(void*)HuiImageFont);
	add_const("HuiImageForward",TypeInt,(void*)HuiImageForward);
	add_const("HuiImageFullscreen",TypeInt,(void*)HuiImageFullscreen);
	add_const("HuiImageHelp",TypeInt,(void*)HuiImageHelp);
	add_const("HuiImageInfo",TypeInt,(void*)HuiImageInfo);
	add_const("HuiImageMediaPause",TypeInt,(void*)HuiImageMediaPause);
	add_const("HuiImageMediaPlay",TypeInt,(void*)HuiImageMediaPlay);
	add_const("HuiImageMediaRecord",TypeInt,(void*)HuiImageMediaRecord);
	add_const("HuiImageMediaStop",TypeInt,(void*)HuiImageMediaStop);
	add_const("HuiImageNew",TypeInt,(void*)HuiImageNew);
	add_const("HuiImageNo",TypeInt,(void*)HuiImageNo);
	add_const("HuiImageOk",TypeInt,(void*)HuiImageOk);
	add_const("HuiImageOpen",TypeInt,(void*)HuiImageOpen);
	add_const("HuiImagePaste",TypeInt,(void*)HuiImagePaste);
	add_const("HuiImagePreferences",TypeInt,(void*)HuiImagePreferences);
	add_const("HuiImagePrint",TypeInt,(void*)HuiImagePrint);
	add_const("HuiImageProperties",TypeInt,(void*)HuiImageProperties);
	add_const("HuiImageQuit",TypeInt,(void*)HuiImageQuit);
	add_const("HuiImageRedo",TypeInt,(void*)HuiImageRedo);
	add_const("HuiImageRefresh",TypeInt,(void*)HuiImageRefresh);
	add_const("HuiImageRemove",TypeInt,(void*)HuiImageRemove);
	add_const("HuiImageSave",TypeInt,(void*)HuiImageSave);
	add_const("HuiImageSaveAs",TypeInt,(void*)HuiImageSaveAs);
	add_const("HuiImageSelectAll",TypeInt,(void*)HuiImageSelectAll);
	add_const("HuiImageStop",TypeInt,(void*)HuiImageStop);
	add_const("HuiImageUndo",TypeInt,(void*)HuiImageUndo);
	add_const("HuiImageUp",TypeInt,(void*)HuiImageUp);
	add_const("HuiImageYes",TypeInt,(void*)HuiImageYes);
	add_const("HuiImageZoomFit",TypeInt,(void*)HuiImageZoomFit);
	add_const("HuiImageZoomIn",TypeInt,(void*)HuiImageZoomIn);
	add_const("HuiImageZoomOne",TypeInt,(void*)HuiImageZoomOne);
	add_const("HuiImageZoomOut",TypeInt,(void*)HuiImageZoomOut);
	// engine presettings
	add_const("NixApiNone",TypeInt,(void*)NIX_API_NONE);
#ifdef NIX_API_OPENGL
	add_const("NixApiOpenGL",TypeInt,(void*)NIX_API_OPENGL);
#else
	add_const("NixApiOpenGL",TypeInt,(void*)-1);
#endif
#ifdef NIX_API_DIRECTX9
	add_const("NixApiDirectX9",TypeInt,(void*)NIX_API_DIRECTX9);
#else
	add_const("NixApiDirectX9",TypeInt,(void*)-1);
#endif
#ifdef NIX_OS_WINDOWS
	add_const("OSWindows",TypeInt,(void*)1);
#else
	add_const("OSWindows",TypeInt,(void*)-1);
#endif
#ifdef NIX_OS_LINUX
	add_const("OSLinux",TypeInt,(void*)1);
#else
	add_const("OSLinux",TypeInt,(void*)-1);
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
	
	msg_db_l(1);
}

void SIAddExtVars()
{
	msg_db_r("SIAddExtVars", 1);

	// game variables
	add_ext_var("AppName",			TypeString,		god_p(&AppName));
	add_ext_var("AppFilename",		TypeString,		&HuiAppFilename);
	add_ext_var("AppDirectory",		TypeString,		&HuiAppDirectory);
	add_ext_var("elapsed",			TypeFloat,		meta_p(&Elapsed));
	add_ext_var("elapsed_rt",		TypeFloat,		meta_p(&ElapsedRT));
	add_ext_var("TimeScale",		TypeFloat,		meta_p(&TimeScale));
	add_ext_var("TargetWidth",		TypeInt,		&NixTargetWidth);
	add_ext_var("TargetHeight",		TypeInt,		&NixTargetHeight);
	add_ext_var("Target",			TypeRect,		&NixTargetRect);
	add_ext_var("ScreenWidth",		TypeInt,		&NixScreenWidth);
	add_ext_var("ScreenHeight",		TypeInt,		&NixScreenHeight);
	add_ext_var("ScreenDepth",		TypeInt,		&NixScreenDepth);
	add_ext_var("Api",				TypeInt,		&NixApi);
	add_ext_var("MinDepth",			TypeFloat,		&NixMinDepth);
	add_ext_var("MaxDepth",			TypeFloat,		&NixMaxDepth);
	add_ext_var("Mouse",			TypeVector,		&NixMouse);
	add_ext_var("MouseRel",			TypeVector,		&NixMouseRel);
	add_ext_var("MouseD",			TypeVector,		&NixMouseD);
	add_ext_var("MouseDRel",		TypeVector,		&NixMouseDRel);
	add_ext_var("TextureLifeTime",	TypeInt,		&NixTextureMaxFramesToLive);
	add_ext_var("LineWidth",		TypeFloat,		&NixLineWidth);
	add_ext_var("SmoothLines",		TypeBool,		&NixSmoothLines);
	add_ext_var("InitialWorldFile", TypeString,		god_p(&InitialWorldFile));
	add_ext_var("CurrentWorldFile", TypeString,		god_p(&CurrentWorldFile));
	add_ext_var("SecondWorldFile",	TypeString,		god_p(&SecondWorldFile));
	add_ext_var("Object",			TypeObjectPList,god_p(&Object));
	add_ext_var("ego",				TypeObjectP,	god_p(&ego));
	add_ext_var("Terrain",			TypeTerrainPList,god_p(&Terrain));
	add_ext_var("Gravitation",		TypeVector,		god_p(&GlobalG));
	add_ext_var("PhysicsEnabled",	TypeBool,		god_p(&PhysicsEnabled));
	add_ext_var("Cam",				TypeViewP,		cam_p(&cam));
	add_ext_var("SkyBox",			TypeModelPList,	god_p(&SkyBox));
	add_ext_var("SkyBoxAng",		TypeVectorList,	god_p(&SkyBoxAng));
	add_ext_var("BackGroundColor",	TypeColor,		god_p(&BackGroundColor));
	add_ext_var("Fog",				TypeFog,		god_p(&GlobalFog));
	add_ext_var("ScriptVar",		TypeFloatList,	god_p(&ScriptVar));
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
	add_ext_var("AvailableHostName",TypeStringList,meta_p(&AvailableHostName));
	add_ext_var("AvailableSessionName",	TypeStringList,meta_p(&AvailableSessionName));
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
	add_ext_var("DirSearchName",	TypeStringList,	&DirSearchName);
	add_ext_var("DirSearchIsDir",	TypeBoolList,	&DirSearchIsDir);
	add_ext_var("HuiFileDialogPath",TypeString,		&HuiFileDialogPath);
	add_ext_var("HuiFileDialogFile",TypeString,		&HuiFileDialogFile);
	add_ext_var("HuiFileDialogCompleteName",TypeString,&HuiFileDialogCompleteName);
	add_ext_var("HuiRunning",		TypeBool,		&HuiRunning);
	
	msg_db_l(1);
}

void SIAddSuperArrays()
{
	msg_db_r("SIAddSuperArrays", 1);

	for (int i=0;i<PreType.size();i++)
		if (PreType[i]->IsSuperArray){
			//msg_error(string("super array:  ", PreType[i]->Name));
			script_make_super_array(PreType[i]);
		}
	
	add_operator(OperatorAssign,		TypeVoid,		TypeSuperArray,	TypeSuperArray);
	add_operator(OperatorAddS,			TypeVoid,		TypeIntList,	TypeIntList);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeIntList,	TypeIntList);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeIntList,	TypeIntList);
	add_operator(OperatorDivideS,		TypeVoid,		TypeIntList,	TypeIntList);
	/*add_operator(OperatorAdd,			TypeIntList,	TypeIntList,	TypeIntList);
	add_operator(OperatorSubtract,		TypeIntList,	TypeIntList,	TypeIntList);
	add_operator(OperatorMultiply,		TypeIntList,	TypeIntList,	TypeIntList);
	add_operator(OperatorDivide,		TypeIntList,	TypeIntList,	TypeIntList);*/
	add_operator(OperatorAssign,		TypeVoid,		TypeIntList,	TypeInt);
	add_operator(OperatorAddS,			TypeVoid,		TypeIntList,	TypeInt);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeIntList,	TypeInt);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeIntList,	TypeInt);
	add_operator(OperatorDivideS,		TypeVoid,		TypeIntList,	TypeInt);
	/*add_operator(OperatorAdd,			TypeIntList,	TypeIntList,	TypeInt);
	add_operator(OperatorSubtract,		TypeIntList,	TypeIntList,	TypeInt);
	add_operator(OperatorMultiply,		TypeIntList,	TypeIntList,	TypeInt);
	add_operator(OperatorDivide,		TypeIntList,	TypeIntList,	TypeInt);*/
	add_operator(OperatorAddS,			TypeVoid,		TypeFloatList,	TypeFloatList);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeFloatList,	TypeFloatList);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeFloatList,	TypeFloatList);
	add_operator(OperatorDivideS,		TypeVoid,		TypeFloatList,	TypeFloatList);
	add_operator(OperatorAssign,		TypeVoid,		TypeFloatList,	TypeFloat);
	add_operator(OperatorAddS,			TypeVoid,		TypeFloatList,	TypeFloat);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeFloatList,	TypeFloat);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeFloatList,	TypeFloat);
	add_operator(OperatorDivideS,		TypeVoid,		TypeFloatList,	TypeFloat);
	add_operator(OperatorAddS,			TypeVoid,		TypeComplexList,TypeComplexList);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeComplexList,TypeComplexList);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeComplexList,TypeComplexList);
	add_operator(OperatorDivideS,		TypeVoid,		TypeComplexList,TypeComplexList);
	add_operator(OperatorAssign,		TypeVoid,		TypeComplexList,TypeComplex);
	add_operator(OperatorAddS,			TypeVoid,		TypeComplexList,TypeComplex);
	add_operator(OperatorSubtractS,		TypeVoid,		TypeComplexList,TypeComplex);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeComplexList,TypeComplex);
	add_operator(OperatorDivideS,		TypeVoid,		TypeComplexList,TypeComplex);
	add_operator(OperatorMultiplyS,		TypeVoid,		TypeComplexList,TypeFloat);
	
	add_func("sumf",		TypeFloat,	(void*)&sum_float_list);
		func_add_param("x",		TypeFloatList);
	add_func("sum2f",		TypeFloat,	(void*)&sum2_float_list);
		func_add_param("x",		TypeFloatList);
	add_func("sumc",		TypeComplex,	(void*)&sum_complex_list);
		func_add_param("x",		TypeComplexList);
	add_func("sum2c",		TypeFloat,	(void*)&sum2_complex_list);
		func_add_param("x",		TypeComplexList);
	add_func("rangei",		TypeIntList,	(void*)&int_range);
		func_add_param("start",		TypeInt);
		func_add_param("end",		TypeInt);
	add_func("rangef",		TypeFloatList,	(void*)&float_range);
		func_add_param("start",		TypeFloat);
		func_add_param("end",		TypeFloat);
		func_add_param("step",		TypeFloat);
	
	msg_db_l(1);
}

void SIAddCommands()
{
	msg_db_r("SIAddCommands", 1);
	
	// mathematical
	add_func("sin",			TypeFloat,	(void*)&sinf);
		func_add_param("x",		TypeFloat);
	add_func("cos",			TypeFloat,	(void*)&cosf);
		func_add_param("x",		TypeFloat);
	add_func("tan",			TypeFloat,	(void*)&tanf);
		func_add_param("x",		TypeFloat);
	add_func("asin",		TypeFloat,	(void*)&asinf);
		func_add_param("x",		TypeFloat);
	add_func("acos",		TypeFloat,	(void*)&acosf);
		func_add_param("x",		TypeFloat);
	add_func("atan",		TypeFloat,	(void*)&atanf);
		func_add_param("x",		TypeFloat);
	add_func("atan2",		TypeFloat,	(void*)&atan2f);
		func_add_param("x",		TypeFloat);
		func_add_param("y",		TypeFloat);
	add_func("sqrt",		TypeFloat,	(void*)&sqrtf);
		func_add_param("x",		TypeFloat);
	add_func("sqr",			TypeFloat,		(void*)&f_sqr);
		func_add_param("x",		TypeFloat);
	add_func("exp",			TypeFloat,		(void*)&expf);
		func_add_param("x",		TypeFloat);
	add_func("log",			TypeFloat,		(void*)&logf);
		func_add_param("x",		TypeFloat);
	add_func("pow",			TypeFloat,		(void*)&powf);
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
	add_func("absf",		TypeFloat,		(void*)&fabsf);
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
		func_add_param("s",		TypeString);
	add_func("s2f",				TypeFloat,		(void*)&s2f);
		func_add_param("s",		TypeString);
	add_func("i2s",				TypeString,	(void*)&i2s);
		func_add_param("i",		TypeInt);
	add_func("f2s",				TypeString,		(void*)&f2s);
		func_add_param("f",			TypeFloat);
		func_add_param("decimals",	TypeInt);
	add_func("f2sf",			TypeString,		(void*)&f2sf);
		func_add_param("f",			TypeFloat);
	add_func("b2s",				TypeString,	(void*)&b2s);
		func_add_param("b",		TypeBool);
	add_func("p2s",				TypeString,	(void*)&p2s);
		func_add_param("p",		TypePointer);
	add_func("v2s",				TypeString,	(void*)&fff2s);
		func_add_param("v",		TypeVector);
	add_func("complex2s",		TypeString,	(void*)&ff2s);
		func_add_param("z",		TypeComplex);
	add_func("quaternion2s",	TypeString,	(void*)&ffff2s);
		func_add_param("q",		TypeQuaternion);
	add_func("plane2s",			TypeString,	(void*)&ffff2s);
		func_add_param("p",		TypePlane);
	add_func("color2s",			TypeString,	(void*)&ffff2s);
		func_add_param("c",		TypeColor);
	add_func("rect2s",			TypeString,	(void*)&ffff2s);
		func_add_param("r",		TypeRect);
	// random numbers
	add_func("randi",			TypeInt,		(void*)&randi);
		func_add_param("max",	TypeInt);
	add_func("randf",			TypeFloat,		(void*)&randf);
		func_add_param("max",	TypeFloat);
	add_func("rand_seed",		TypeInt,		(void*)&srand);
		func_add_param("seed",	TypeInt);
	// debug output
	/*add_func("intout",			TypeVoid,		(void*)&intout);
		func_add_param("i",		TypeInt);
	add_func("boolout",			TypeVoid,		(void*)&boolout);
		func_add_param("b",		TypeBool);
	add_func("floatout",		TypeVoid,		(void*)&floatout);
		func_add_param("f",		TypeFloat);
	add_func("complexout",		TypeVoid,		(void*)&complexout);
		func_add_param("c",		TypeComplex);*/
	add_func("print",			TypeVoid,		(void*)&_stringout);
		func_add_param("str",	TypeString);
	/*add_func("pointerout",		TypeVoid,		(void*)&pointerout);
		func_add_param("p",		TypePointer);
	add_func("vectorout",			TypeVoid,		(void*)&vectorout);
		func_add_param("v",		TypeVectorPs);*/
	
	// vectors
	add_func("VecNormalize",		TypeVoid,	(void*)&VecNormalize);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVector);
	add_func("VecDir2Ang",			TypeVector,	(void*)&VecDir2Ang);
		func_add_param("dir",		TypeVector);
	add_func("VecDir2Ang2",			TypeVector,	(void*)&VecDir2Ang2);
		func_add_param("dir",		TypeVector);
		func_add_param("up",		TypeVector);
	add_func("VecAng2Dir",			TypeVector,	(void*)&VecAng2Dir);
		func_add_param("ang",		TypeVector);
	add_func("VecAngAdd",			TypeVector,	(void*)&VecAngAdd);
		func_add_param("ang1",		TypeVector);
		func_add_param("ang2",		TypeVector);
	add_func("VecAngInterpolate",	TypeVector,	(void*)&VecAngInterpolate);
		func_add_param("ang1",		TypeVector);
		func_add_param("ang2",		TypeVector);
		func_add_param("t",			TypeFloat);
	add_func("VecRotate",			TypeVector,	(void*)&VecRotate);
		func_add_param("v",			TypeVector);
		func_add_param("ang",		TypeVector);
	add_func("VecLength",			TypeFloat,	(void*)&VecLength);
		func_add_param("v",			TypeVector);
	add_func("VecLengthSqr",		TypeFloat,	(void*)&VecLengthSqr);
		func_add_param("v",			TypeVector);
	add_func("VecLengthFuzzy",		TypeFloat,	(void*)&VecLengthFuzzy);
		func_add_param("v",			TypeVector);
	add_func("VecTransform",		TypeVoid,	(void*)&VecTransform);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("m",			TypeMatrix);
		func_add_param("v_in",		TypeVector);
	add_func("VecNormalTransform",	TypeVoid,	(void*)&VecNormalTransform);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("m",			TypeMatrix);
		func_add_param("v_in",		TypeVector);
	add_func("VecDotProduct",		TypeFloat,	(void*)&VecDotProduct);
		func_add_param("v1",		TypeVector);
		func_add_param("v2",		TypeVector);
	add_func("VecCrossProduct",		TypeVector,	(void*)&VecCrossProduct);
		func_add_param("v1",		TypeVector);
		func_add_param("v2",		TypeVector);
	// matrices
	add_func("MatrixIdentity",		TypeVoid,	(void*)&MatrixIdentity);
		func_add_param("m_out",		TypeMatrixPs);
	add_func("MatrixTranslation",	TypeVoid,	(void*)&MatrixTranslation);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("trans",		TypeVector);
	add_func("MatrixRotation",		TypeVoid,	(void*)&MatrixRotation);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("ang",		TypeVector);
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
		func_add_param("ang",		TypeQuaternion);
	add_func("MatrixRotationView",	TypeVoid,	(void*)&MatrixRotationView);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("ang",		TypeVector);
	add_func("MatrixScale",			TypeVoid,	(void*)&MatrixScale);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("s_x",		TypeFloat);
		func_add_param("s_y",		TypeFloat);
		func_add_param("s_z",		TypeFloat);
	add_func("MatrixMultiply",		TypeVoid,	(void*)&MatrixMultiply);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("m2",		TypeMatrix);
		func_add_param("m1",		TypeMatrix);
	add_func("MatrixInverse",		TypeVoid,	(void*)&MatrixInverse);
		func_add_param("m_out",		TypeMatrixPs);
		func_add_param("m_in",		TypeMatrix);
	// quaternions
	add_func("QuaternionRotationV",	TypeVoid,	(void*)&QuaternionRotationV);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("ang",		TypeVector);
	add_func("QuaternionRotationA",	TypeVoid,	(void*)&QuaternionRotationA);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("axis",		TypeVector);
		func_add_param("angle",		TypeFloat);
	add_func("QuaternionMultiply",	TypeVoid,	(void*)&QuaternionMultiply);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("q2",		TypeQuaternion);
		func_add_param("q1",		TypeQuaternion);
	add_func("QuaternionInverse",	TypeVoid,	(void*)&QuaternionInverse);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("q_in",		TypeQuaternion);
	add_func("QuaternionScale",		TypeVoid,	(void*)&QuaternionScale);
		func_add_param("q",		TypeQuaternionPs);
		func_add_param("f",		TypeFloat);
	add_func("QuaternionNormalize",	TypeVoid,	(void*)&QuaternionNormalize);
		func_add_param("q_out",		TypeQuaternionPs);
		func_add_param("q_in",		TypeQuaternion);
	add_func("QuaternionToAngle",	TypeVector,	(void*)&QuaternionToAngle);
		func_add_param("q",		TypeQuaternion);
	// plane
	add_func("PlaneFromPoints",	TypeVoid,	(void*)&PlaneFromPoints);
		func_add_param("pl",		TypePlanePs);
		func_add_param("a",		TypeVector);
		func_add_param("b",		TypeVector);
		func_add_param("c",		TypeVector);
	add_func("PlaneFromPointNormal",	TypeVoid,	(void*)&PlaneFromPointNormal);
		func_add_param("pl",		TypePlanePs);
		func_add_param("p",		TypeVector);
		func_add_param("n",		TypeVector);
	add_func("PlaneTransform",	TypeVoid,	(void*)&PlaneTransform);
		func_add_param("pl_out",		TypePlanePs);
		func_add_param("m",		TypeMatrix);
		func_add_param("pl_in",		TypePlane);
	add_func("PlaneGetNormal",	TypeVector,	(void*)&GetNormal);
		func_add_param("pl",		TypePlane);
	add_func("PlaneIntersectLine",	TypeBool,	(void*)&PlaneIntersectLine);
		func_add_param("inter",		TypeVectorPs);
		func_add_param("pl",		TypePlane);
		func_add_param("l1",		TypeVector);
		func_add_param("l2",		TypeVector);
	add_func("PlaneInverse",	TypeVoid,	(void*)&PlaneInverse);
		func_add_param("pl",		TypePlane);
	add_func("PlaneDistance",	TypeFloat,	(void*)&PlaneDistance);
		func_add_param("pl",		TypePlane);
		func_add_param("p",		TypeVector);
	// other types
	add_func("GetBaryCentric",	TypeVoid,	(void*)&GetBaryCentric);
		func_add_param("p",		TypeVector);
		func_add_param("a",		TypeVector);
		func_add_param("b",		TypeVector);
		func_add_param("c",		TypeVector);
		func_add_param("f",		TypeFloatPs);
		func_add_param("g",		TypeFloatPs);
	add_func("SetColorHSB",			TypeColor,		(void*)&SetColorHSB);
		func_add_param("a",		TypeFloat);
		func_add_param("h",		TypeFloat);
		func_add_param("s",		TypeFloat);
		func_add_param("b",		TypeFloat);
	add_func("ColorInterpolate",	TypeColor,		(void*)&ColorInterpolate);
		func_add_param("c1",		TypeColor);
		func_add_param("c2",		TypeColor);
		func_add_param("t",		TypeFloat);
	add_func("LoadTexture",			TypeInt,	meta_p(&NixLoadTexture));
		func_add_param("filename",		TypeString);
	add_func("XFDrawStr",			TypeFloat,	meta_p(&XFDrawStr));
		func_add_param("x",			TypeFloat);
		func_add_param("y",			TypeFloat);
		func_add_param("size",		TypeFloat);
		func_add_param("str",		TypeString);
		func_add_param("centric",	TypeBool);
	add_func("XFDrawVertStr",		TypeFloat,	meta_p(&XFDrawVertStr));
		func_add_param("x",			TypeFloat);
		func_add_param("y",			TypeFloat);
		func_add_param("size",		TypeFloat);
		func_add_param("str",		TypeString);
	add_func("XFGetWidth",			TypeFloat,	meta_p(&XFGetWidth));
		func_add_param("size",		TypeFloat);
		func_add_param("s",		TypeString);
	add_func("LoadXFont",			TypeInt,	meta_p(&MetaLoadXFont));
		func_add_param("filename",		TypeString);
	add_func("CreatePicture",										TypePictureP,	gui_p(&GuiCreatePicture));
		func_add_param("pos",		TypeVector);
		func_add_param("width",		TypeFloat);
		func_add_param("height",	TypeFloat);
		func_add_param("texture",	TypeInt);
		func_add_param("source",	TypeRect);
		func_add_param("c",			TypeColor);
	add_func("CreatePicture3D",								TypePicture3DP,	gui_p(&GuiCreatePicture3D));
		func_add_param("m",			TypeModelP);
		func_add_param("mat",		TypeMatrix);
		func_add_param("z",			TypeFloat);
	add_func("CreateText",												TypeTextP,	gui_p(&GuiCreateText));
		func_add_param("pos",		TypeVector);
		func_add_param("size",		TypeFloat);
		func_add_param("c",			TypeColor);
		func_add_param("str",		TypeString);
	add_func("CreateGrouping",										TypeGroupingP,	gui_p(&GuiCreateGrouping));
		func_add_param("pos",		TypeVector);
		func_add_param("c",			TypeColor);
		func_add_param("set_cur",	TypeBool);
	add_func("GuiMouseOver",										TypeBool,	gui_p(&GuiMouseOver));
		func_add_param("p",		TypePointer);
	add_func("CreateParticle",										TypeParticleP,	fx_p(&FxParticleCreateDef));
		func_add_param("pos",		TypeVector);
		func_add_param("texture",		TypeInt);
		func_add_param("func",		TypePointer);
		func_add_param("life",		TypeFloat);
		func_add_param("radius",		TypeFloat);
	add_func("CreateParticleRot",								TypeParticleP,	fx_p(&FxParticleCreateRot));
		func_add_param("pos",		TypeVector);
		func_add_param("ang",		TypeVector);
		func_add_param("texture",		TypeInt);
		func_add_param("func",		TypePointer);
		func_add_param("life",		TypeFloat);
		func_add_param("radius",		TypeFloat);
	add_func("CreateBeam",										TypeBeamP,	fx_p(&FxParticleCreateBeam));
		func_add_param("pos",		TypeVector);
		func_add_param("length",		TypeVector);
		func_add_param("texture",		TypeInt);
		func_add_param("func",		TypePointer);
		func_add_param("life",		TypeFloat);
		func_add_param("radius",		TypeFloat);
	add_func("CreateEffect",										TypeEffectP,	fx_p(&FxCreate));
		func_add_param("pos",		TypeVector);
		func_add_param("func",		TypePointer);
		func_add_param("del_func",		TypePointer);
		func_add_param("life",		TypeFloat);
	add_func("LoadModel",												TypeModelP,	meta_p(&MetaLoadModel));
		func_add_param("filename",		TypeString);
	add_func("LoadItem",												TypeItemP,	meta_p(&MetaLoadItem));
		func_add_param("filename",		TypeString);
	add_func("GetItemOID",												TypeInt,	meta_p(&MetaGetItemOID));
		func_add_param("filename",		TypeString);
	add_func("CreateView",												TypeViewP,	cam_p(&CameraCreateView));
		func_add_param("pos",		TypeVector);
		func_add_param("ang",		TypeVector);
		func_add_param("dest",		TypeRect);
		func_add_param("show",		TypeBool);
	add_func("LoadShaderFile",										TypeInt,	meta_p(&MetaLoadShaderFile));
		func_add_param("filename",		TypeString);
	add_func("CreateList",												TypeInt,	meta_p(&MetaListCreate));
		func_add_param("item_size",		TypeInt);
	add_func("ListDelete",												TypeVoid,	meta_p(&MetaListDelete));
		func_add_param("list",		TypeInt);
	add_func("ListGetItemCount",								TypeInt,	meta_p(&MetaListGetItemCount));
		func_add_param("list",		TypeInt);
	add_func("ListIterate",												TypeBool,	meta_p(&MetaListIterate));
		func_add_param("list",		TypeInt);
		func_add_param("item",		TypePointerPs);
	add_func("ListIterateBack",										TypeBool,	meta_p(&MetaListIterateBack));
		func_add_param("list",		TypeInt);
		func_add_param("item",		TypePointerPs);
	add_func("ListDeleteItem",										TypeVoid,	meta_p(&MetaListDeleteItem));
		func_add_param("list",		TypeInt);
		func_add_param("item",		TypePointerPs);
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
	add_func("GetButton",										TypeBool,	(void*)&NixGetButton);
		func_add_param("button",	TypeInt);
	add_func("GetButtonDown",									TypeBool,	(void*)&NixGetButtonDown);
		func_add_param("button",	TypeInt);
	add_func("GetButtonUp",										TypeBool,	(void*)&NixGetButtonUp);
		func_add_param("button",	TypeInt);
		// drawing
	add_func("NixStart",									TypeVoid,	(void*)&NixStart);
		func_add_param("texture",		TypeInt);
	add_func("NixEnd",											TypeVoid,	(void*)&NixEnd);
	add_func("NixKillWindows",							TypeVoid,	(void*)&NixKillWindows);
	add_func("NixDraw2D",								TypeVoid,	(void*)&NixDraw2D);
		func_add_param("texture",		TypeInt);
		func_add_param("c",		TypeColor);
		func_add_param("source",		TypeRect);
		func_add_param("dest",		TypeRect);
		func_add_param("z",		TypeFloat);
	add_func("NixDraw3D",								TypeVoid,	(void*)&NixDraw3D);
		func_add_param("texture",		TypeInt);
		func_add_param("vb",		TypeInt);
		func_add_param("m",		TypeMatrix);
	add_func("NixDrawStr",									TypeVoid,	(void*)&NixDrawStr);
		func_add_param("x",		TypeInt);
		func_add_param("y",		TypeInt);
		func_add_param("str",	TypeString);
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
		func_add_param("c",			TypeColor);
		func_add_param("source",		TypeRect);
		func_add_param("pos",		TypeVector);
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
		func_add_param("view_mat",		TypeMatrix);
	add_func("NixSetViewV",								TypeVoid,	(void*)&NixSetViewV);
		func_add_param("enable3d",		TypeBool);
		func_add_param("pos",		TypeVector);
		func_add_param("ang",		TypeVector);
	add_func("NixSetZ",											TypeVoid,	(void*)&NixSetZ);
		func_add_param("write",		TypeBool);
		func_add_param("test",		TypeBool);
	add_func("NixSetMaterial",							TypeVoid,	(void*)&NixSetMaterial);
		func_add_param("ambient",		TypeColor);
		func_add_param("diffuse",		TypeColor);
		func_add_param("specular",		TypeColor);
		func_add_param("shininess",		TypeFloat);
		func_add_param("emission",		TypeColor);
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
		func_add_param("filename",		TypeString);
		func_add_param("image",		TypePointerPs);
		func_add_param("width",		TypeIntPs);
		func_add_param("height",		TypeIntPs);
	add_func("NixSaveTGA",									TypeVoid,	(void*)&NixSaveTGA);
		func_add_param("filename",		TypeString);
		func_add_param("width",		TypeInt);
		func_add_param("height",		TypeInt);
		func_add_param("bits",		TypeInt);
		func_add_param("bits_alpha",		TypeInt);
		func_add_param("image",		TypePointer);
	add_func("VecProject",								TypeVoid,	(void*)&NixGetVecProject);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVector);
	add_func("VecUnproject",							TypeVoid,	(void*)&NixGetVecUnproject);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVector);
	add_func("VecProjectRel",						TypeVoid,	(void*)&NixGetVecProjectRel);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVector);
	add_func("VecUnprojectRel",						TypeVoid,	(void*)&NixGetVecUnprojectRel);
		func_add_param("v_out",		TypeVectorPs);
		func_add_param("v_in",		TypeVector);
	add_func("NixVBClear",									TypeVoid,	(void*)&NixVBClear);
		func_add_param("vb",		TypeInt);
	add_func("NixVBAddTria",							TypeVoid,	(void*)&NixVBAddTria);
		func_add_param("vb",		TypeInt);
		func_add_param("p1",		TypeVector);
		func_add_param("n1",		TypeVector);
		func_add_param("u1",		TypeFloat);
		func_add_param("v1",		TypeFloat);
		func_add_param("p2",		TypeVector);
		func_add_param("n2",		TypeVector);
		func_add_param("u2",		TypeFloat);
		func_add_param("v2",		TypeFloat);
		func_add_param("p3",		TypeVector);
		func_add_param("n3",		TypeVector);
		func_add_param("u3",		TypeFloat);
		func_add_param("v3",		TypeFloat);
	add_func("NixVBAddTrias",							TypeVoid,	(void*)&NixVBAddTrias);
		func_add_param("vb",		TypeInt);
		func_add_param("num_trias",		TypeInt);
		func_add_param("p",		TypeVectorArrayP);
		func_add_param("n",		TypeVectorArrayP);
		func_add_param("t",		TypeFloatArrayP);
	add_func("NixSetShaderData",					TypeVoid,	(void*)&NixSetShaderData);
		func_add_param("index",		TypeInt);
		func_add_param("name",		TypeString);
		func_add_param("data",		TypePointer);
		func_add_param("size",		TypeInt);
	add_func("NixGetShaderData",					TypeVoid,	(void*)&NixGetShaderData);
		func_add_param("index",		TypeInt);
		func_add_param("name",		TypeString);
		func_add_param("data",		TypePointer);
		func_add_param("size",		TypeInt);
	// sound
	add_func("SoundEmit",									TypeVoid,	meta_p(&MetaSoundEmit));
		func_add_param("filename",		TypeString);
		func_add_param("pos",		TypeVector);
		func_add_param("radius",		TypeFloat);
		func_add_param("speed",		TypeFloat);
	add_func("SoundCreate",									TypeInt,	meta_p(&MetaSoundManagedNew));
		func_add_param("filename",		TypeString);
	add_func("SoundSetData",							TypeVoid,	meta_p(&MetaSoundManagedSetData));
		func_add_param("index",		TypeInt);
		func_add_param("pos",		TypeVector);
		func_add_param("vel",		TypeVector);
		func_add_param("radius",		TypeFloat);
		func_add_param("speed",		TypeFloat);
		func_add_param("volume",		TypeFloat);
	add_func("SoundDelete",									TypeVoid,	meta_p(&MetaSoundManagedDelete));
		func_add_param("index",		TypeInt);
	// music
	add_func("MusicLoad",									TypeInt,	meta_p(&MetaMusicLoad));
		func_add_param("filename",		TypeString);
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
		func_add_param("dir",		TypeVector);
		func_add_param("ambient",		TypeColor);
		func_add_param("diffuse",		TypeColor);
		func_add_param("specular",		TypeColor);
	add_func("FXLightSetRadial",					TypeVoid,	fx_p(&FxLightSetRadial));
		func_add_param("index",		TypeInt);
		func_add_param("pos",		TypeVector);
		func_add_param("radius",		TypeFloat);
		func_add_param("ambient",		TypeColor);
		func_add_param("diffuse",		TypeColor);
		func_add_param("specular",		TypeColor);
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
	add_func("LoadWorld",									TypeVoid,	meta_p(MetaLoadWorld));
		func_add_param("filename",		TypeString);
	add_func("LoadGameFromHost",					TypeVoid,	meta_p(MetaLoadGameFromHost));
		func_add_param("host",		TypeInt);
	add_func("SaveGameState",							TypeVoid,	meta_p(MetaSaveGameState));
		func_add_param("filename",		TypeString);
	add_func("LoadGameState",							TypeVoid,	meta_p(MetaLoadGameState));
		func_add_param("filename",		TypeString);
	add_func("GetObjectByName",							TypeObjectP,	god_p(&GetObjectByName));
		func_add_param("name",		TypeString);
	add_func("FindObjects",								TypeInt,	god_p(&GodFindObjects));
		func_add_param("pos",		TypeVector);
		func_add_param("radius",	TypeFloat);
		func_add_param("mode",		TypeInt);
		func_add_param("o",			TypeObjectPListPs);
	add_func("NextObject",									TypeBool,	god_p(&NextObject));
		func_add_param("o",		TypeObjectPPs);
	add_func("CreateObject",							TypeObjectP,	god_p(&_CreateObject));
		func_add_param("filename",		TypeString);
		func_add_param("pos",		TypeVector);
	add_func("DeleteObject",							TypeVoid,	god_p(&_DeleteObject));
		func_add_param("id",		TypeInt);
	add_func("DeleteObjectLater",						TypeVoid,	god_p(&GodDeleteObjectLater));
		func_add_param("id",		TypeInt);
	add_func("DeleteObjects",						TypeVoid,	god_p(&GodDeleteObjects));
	add_func("DrawSplashScreen",					TypeVoid,	meta_p(MetaDrawSplashScreen));
		func_add_param("status",		TypeString);
		func_add_param("progress",		TypeFloat);
	add_func("RenderScene",									TypeVoid, 	NULL);
	add_func("GetG",											TypeVector,	god_p(&GetG));
		func_add_param("pos",		TypeVector);
	add_func("Trace",											TypeBool,	god_p(&GodTrace));
		func_add_param("p1",		TypeVector);
		func_add_param("p2",		TypeVector);
		func_add_param("tp",		TypeVectorPs);
		func_add_param("simple_test",	TypeBool);
		func_add_param("o_ignore",		TypeInt);
	add_func("AddLinkSpring",							TypeInt,	god_p(&AddLinkSpring));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("p1",		TypeVector);
		func_add_param("p2",		TypeVector);
		func_add_param("dx0",		TypeFloat);
		func_add_param("k",			TypeFloat);
	add_func("AddLinkBall",									TypeInt,	god_p(&AddLinkBall));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("p",			TypeVector);
	add_func("AddLinkHinge",							TypeInt,	god_p(&AddLinkHinge));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("p"	,		TypeVector);
		func_add_param("ax",		TypeVector);
	add_func("AddLinkHinge2",							TypeInt,	god_p(&AddLinkHinge2));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("p"	,		TypeVector);
		func_add_param("ax1",		TypeVector);
		func_add_param("ax2",		TypeVector);
	add_func("AddLinkSlider",									TypeInt,	god_p(&AddLinkSlider));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("ax",		TypeVector);
	add_func("AddLinkUniversal",									TypeInt,	god_p(&AddLinkUniversal));
		func_add_param("o1",		TypeObjectP);
		func_add_param("o2",		TypeObjectP);
		func_add_param("p",			TypeVector);
		func_add_param("ax1",		TypeVector);
		func_add_param("ax2",		TypeVector);
	add_func("LinkSetTorque",					TypeVoid,	god_p(&LinkSetTorque));
		func_add_param("link",		TypeInt);
		func_add_param("torque",	TypeFloat);
	add_func("LinkSetTorqueAxis",					TypeVoid,	god_p(&LinkSetTorqueAxis));
		func_add_param("link",		TypeInt);
		func_add_param("axis",		TypeInt);
		func_add_param("torque",	TypeFloat);
	add_func("LinkSetRange",					TypeVoid,	god_p(&LinkSetRange));
		func_add_param("link",		TypeInt);
		func_add_param("min",		TypeFloat);
		func_add_param("max",		TypeFloat);
	add_func("LinkSetRangeAxis",					TypeVoid,	god_p(&LinkSetRangeAxis));
		func_add_param("link",		TypeInt);
		func_add_param("axis",		TypeInt);
		func_add_param("min",		TypeFloat);
		func_add_param("max",		TypeFloat);
	add_func("LinkSetFriction",		TypeVoid,	god_p(&LinkSetFriction));
		func_add_param("link",		TypeInt);
		func_add_param("friction",	TypeFloat);
	/*add_func("LinkSetFrictionAxis",		TypeVoid,	god_p(&LinkSetFrictionAxis));
		func_add_param("link",		TypeInt);
		func_add_param("axis",		TypeInt);
		func_add_param("friction",	TypeFloat);*/
	add_func("LinkGetPosition",					TypeFloat,	god_p(&LinkGetPosition));
		func_add_param("link",		TypeInt);
	add_func("LinkGetPositionAxis",					TypeFloat,	god_p(&LinkGetPositionAxis));
		func_add_param("link",		TypeInt);
		func_add_param("axis",		TypeInt);
	// file access
	add_func("FileOpen",			TypeFileP,				(void*)&FileOpen);
		func_add_param("filename",		TypeString);
	add_func("FileCreate",			TypeFileP,				(void*)&FileCreate);
		func_add_param("filename",		TypeString);
	add_func("FileClose",			TypeBool,				(void*)&FileClose);
		func_add_param("f",		TypeFileP);
	add_func("FileTestExistence",	TypeBool,		(void*)&file_test_existence);
		func_add_param("filename",		TypeString);
	add_func("DirSearch",			TypeInt,			(void*)&dir_search);
		func_add_param("dir",		TypeString);
		func_add_param("filter",		TypeString);
		func_add_param("show_dirs",		TypeBool);
	// network
	add_func("NetConnect",			TypeInt,			(void*)&NixNetConnect);
		func_add_param("addr",		TypeString);
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
		func_add_param("s",		TypeString);
	add_func("NetWriteStrL",		TypeVoid,			(void*)&NixNetWriteStrL);
		func_add_param("s",		TypeString);
		func_add_param("length",		TypeInt);

// add_func("ExecuteScript",	TypeVoid);
//		func_add_param("filename",		TypeString);
	
	msg_db_l(1);
}

void SIAddHuiStuff()
{
	msg_db_r("SIAddHuiStuff", 1);
	
	sType *TypeHuiMenu		= add_type  ("-CHuiMenu-",	0);
	sType *TypeHuiMenuP		= add_type_p("HuiMenu",		TypeHuiMenu);
	sType *TypeHuiWindow	= add_type  ("-CHuiWindow-",0);
	sType *TypeHuiWindowP	= add_type_p("HuiWindow",	TypeHuiWindow);

	add_class(TypeHuiMenu);
		class_add_func("OpenPopup",	TypeVoid,		mf((tmf)&CHuiMenu::OpenPopup));
			func_add_param("w",			TypeHuiWindowP);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
		class_add_func("AddItem",		TypeVoid,		mf((tmf)&CHuiMenu::AddItem));
			func_add_param("name",		TypeString);
			func_add_param("id",		TypeInt);
		class_add_func("AddItemImage",	TypeVoid,		mf((tmf)&CHuiMenu::AddItemImage));
			func_add_param("name",		TypeString);
			func_add_param("image",		TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddItemCheckable",	TypeVoid,		mf((tmf)&CHuiMenu::AddItemCheckable));
			func_add_param("name",		TypeString);
			func_add_param("id",		TypeInt);
		class_add_func("AddSeparator",	TypeVoid,		mf((tmf)&CHuiMenu::AddSeparator));
		class_add_func("AddSubMenu",	TypeVoid,		mf((tmf)&CHuiMenu::AddSubMenu));
			func_add_param("name",		TypeString);
			func_add_param("id",		TypeInt);
			func_add_param("sub_menu",	TypeHuiMenuP);
		class_add_func("CheckItem",	TypeVoid,		mf((tmf)&CHuiMenu::CheckItem));
			func_add_param("id",		TypeInt);
			func_add_param("checked",	TypeBool);
		class_add_func("IsItemChecked",TypeBool,		mf((tmf)&CHuiMenu::IsItemChecked));
			func_add_param("id",		TypeInt);
		class_add_func("EnableItem",	TypeVoid,		mf((tmf)&CHuiMenu::EnableItem));
			func_add_param("id",		TypeInt);
			func_add_param("enabled",	TypeBool);
		class_add_func("SetText",		TypeVoid,		mf((tmf)&CHuiMenu::SetText));
			func_add_param("id",		TypeInt);
			func_add_param("text",		TypeString);
	
	add_class(TypeHuiWindow);
		class_add_element("Menu",		TypeHuiMenuP,	GetDAWindow(Menu));
		class_add_element("NumFloatDecimals",TypeInt,	GetDAWindow(NumFloatDecimals));
		class_add_func("Update",		TypeVoid,		mf((tmf)&CHuiWindow::Update));
		class_add_func("Hide",			TypeVoid,		mf((tmf)&CHuiWindow::Hide));
			func_add_param("hide",		TypeBool);
		class_add_func("SetMaximized",		TypeVoid,		mf((tmf)&CHuiWindow::SetMaximized));
			func_add_param("max",		TypeBool);
		class_add_func("IsMaximized",		TypeBool,		mf((tmf)&CHuiWindow::IsMaximized));
		class_add_func("IsMinimized",		TypeBool,		mf((tmf)&CHuiWindow::IsMinimized));
		class_add_func("SetID",			TypeVoid,		mf((tmf)&CHuiWindow::SetID));
			func_add_param("id",		TypeInt);
		class_add_func("SetFullscreen",				TypeVoid,		mf((tmf)&CHuiWindow::SetFullscreen));
			func_add_param("fullscreen",TypeBool);
		class_add_func("SetTitle",										TypeVoid,		mf((tmf)&CHuiWindow::SetTitle));
			func_add_param("title",		TypeString);
		class_add_func("SetPosition",								TypeVoid,		mf((tmf)&CHuiWindow::SetPosition));
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
	//add_func("SetOuterior",								TypeVoid,		2,	TypePointer,"win",
	//																										TypeIRect,"r");
	//add_func("GetOuterior",								TypeIRect,		1,	TypePointer,"win");
	//add_func("SetInerior",								TypeVoid,		2,	TypePointer,"win",
	//																										TypeIRect,"r");
	//add_func("GetInterior",									TypeIRect,		1,	TypePointer,"win");
		class_add_func("SetCursorPos",								TypeVoid,		mf((tmf)&CHuiWindow::SetCursorPos));
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
		class_add_func("Activate",										TypeVoid,		mf((tmf)&CHuiWindow::Activate));
			func_add_param("id",		TypeInt);
		class_add_func("IsActive",										TypeVoid,		mf((tmf)&CHuiWindow::IsActive));
			func_add_param("recursive",	TypeBool);
		class_add_func("AddButton",										TypeVoid,		mf((tmf)&CHuiWindow::AddButton));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddDefButton",										TypeVoid,		mf((tmf)&CHuiWindow::AddDefButton));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddCheckBox",								TypeVoid,		mf((tmf)&CHuiWindow::AddCheckBox));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddText",										TypeVoid,		mf((tmf)&CHuiWindow::AddText));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddEdit",										TypeVoid,		mf((tmf)&CHuiWindow::AddEdit));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddGroup",										TypeVoid,		mf((tmf)&CHuiWindow::AddGroup));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddComboBox",								TypeVoid,		mf((tmf)&CHuiWindow::AddComboBox));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddTabControl",								TypeVoid,		mf((tmf)&CHuiWindow::AddTabControl));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("SetTabCreationPage",				TypeVoid,		mf((tmf)&CHuiWindow::SetTabCreationPage));
			func_add_param("id",		TypeInt);
			func_add_param("page",		TypeInt);
		class_add_func("AddListView",								TypeVoid,		mf((tmf)&CHuiWindow::AddListView));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddProgressBar",						TypeVoid,		mf((tmf)&CHuiWindow::AddProgressBar));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddSlider",										TypeVoid,		mf((tmf)&CHuiWindow::AddSlider));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("AddImage",										TypeVoid,		mf((tmf)&CHuiWindow::AddImage));
			func_add_param("title",		TypeString);
			func_add_param("x",			TypeInt);
			func_add_param("y",			TypeInt);
			func_add_param("width",		TypeInt);
			func_add_param("height",	TypeInt);
			func_add_param("id",		TypeInt);
		class_add_func("SetString",						TypeVoid,		mf((tmf)&CHuiWindow::SetString));
			func_add_param("id",		TypeInt);
			func_add_param("s",			TypeString);
		class_add_func("GetString",						TypeStringP,		mf((tmf)&CHuiWindow::GetString));
			func_add_param("id",		TypeInt);
		class_add_func("SetFloat",						TypeVoid,		mf((tmf)&CHuiWindow::SetFloat));
			func_add_param("id",		TypeInt);
			func_add_param("f",			TypeFloat);
		class_add_func("GetFloat",						TypeFloat,		mf((tmf)&CHuiWindow::GetFloat));
			func_add_param("id",		TypeInt);
		class_add_func("Enable",								TypeVoid,		mf((tmf)&CHuiWindow::Enable));
			func_add_param("id",		TypeInt);
			func_add_param("enabled",	TypeBool);
		class_add_func("IsEnabled",					TypeBool,		mf((tmf)&CHuiWindow::IsEnabled));
			func_add_param("id",		TypeInt);
		class_add_func("Check",								TypeVoid,		mf((tmf)&CHuiWindow::Check));
			func_add_param("id",		TypeInt);
			func_add_param("checked",	TypeBool);
		class_add_func("IsChecked",					TypeBool,		mf((tmf)&CHuiWindow::IsChecked));
			func_add_param("id",		TypeInt);
		class_add_func("GetInt",			TypeInt,		mf((tmf)&CHuiWindow::GetInt));
			func_add_param("id",		TypeInt);
		class_add_func("GetMultiSelection",			TypeInt,		mf((tmf)&CHuiWindow::GetMultiSelection));
			func_add_param("id",		TypeInt);
			func_add_param("indices",	TypeIntArrayP);
		class_add_func("SetInt",			TypeVoid,		mf((tmf)&CHuiWindow::SetInt));
			func_add_param("id",		TypeInt);
			func_add_param("index",		TypeInt);
		class_add_func("SetImage",			TypeVoid,		mf((tmf)&CHuiWindow::SetImage));
			func_add_param("id",		TypeInt);
			func_add_param("image",		TypeInt);
		class_add_func("Reset",								TypeVoid,		mf((tmf)&CHuiWindow::Reset));
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
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("title",		TypeString);
		func_add_param("dir",		TypeString);
		func_add_param("show_filter",	TypeString);
		func_add_param("filter",	TypeString);
	add_func("HuiFileDialogSave",	TypeBool,	(void*)&HuiFileDialogSave);
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("title",		TypeString);
		func_add_param("dir",		TypeString);
		func_add_param("show_filter",	TypeString);
		func_add_param("filter",	TypeString);
	add_func("HuiFileDialogDir",	TypeBool,	(void*)&HuiFileDialogDir);
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("title",		TypeString);
		func_add_param("dir",		TypeString);
	add_func("HuiQuestionBox",		TypeInt,	(void*)&HuiQuestionBox);
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("title",		TypeString);
		func_add_param("text",		TypeString);
		func_add_param("allow_cancel",	TypeBool);
	add_func("HuiInfoBox",			TypeVoid,			(void*)&HuiInfoBox);
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("title",		TypeString);
		func_add_param("text",		TypeString);
	add_func("HuiErrorBox",			TypeVoid,		(void*)&HuiErrorBox);
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("title",		TypeString);
		func_add_param("text",		TypeString);
	add_func("HuiConfigWriteInt",								TypeVoid,	(void*)&HuiConfigWriteInt);
		func_add_param("name",		TypeString);
		func_add_param("value",		TypeInt);
	add_func("HuiConfigWriteFloat",								TypeVoid,	(void*)&HuiConfigWriteFloat);
		func_add_param("name",		TypeString);
		func_add_param("value",		TypeFloat);
	add_func("HuiConfigWriteBool",								TypeVoid,	(void*)&HuiConfigWriteBool);
		func_add_param("name",		TypeString);
		func_add_param("value",		TypeBool);
	add_func("HuiConfigWriteStr",								TypeVoid,	(void*)&HuiConfigWriteStr);
		func_add_param("name",		TypeString);
		func_add_param("value",		TypeString);
	add_func("HuiConfigReadInt",								TypeVoid,	(void*)&HuiConfigReadInt);
		func_add_param("name",		TypeString);
		func_add_param("value",		TypeIntPs);
		func_add_param("default",	TypeInt);
	add_func("HuiConfigReadFloat",								TypeVoid,	(void*)&HuiConfigReadFloat);
		func_add_param("name",		TypeString);
		func_add_param("value",		TypeFloatPs);
		func_add_param("default",	TypeFloat);
	add_func("HuiConfigReadBool",								TypeVoid,	(void*)&HuiConfigReadBool);
		func_add_param("name",		TypeString);
		func_add_param("value",		TypeBoolPs);
		func_add_param("default",	TypeBool);
	add_func("HuiConfigReadStr",								TypeVoid,	(void*)&HuiConfigReadStr);
		func_add_param("name",		TypeString);
		func_add_param("value",		TypeStringP);
		func_add_param("default",	TypeString);

	// clipboard
	add_func("HuiCopyToClipboard",	TypeVoid,			(void*)&HuiCopyToClipBoard);
		func_add_param("buffer",	TypeString);
		func_add_param("length",	TypeInt);
	add_func("HuiPasteFromClipboard",	TypeVoid,		(void*)&HuiPasteFromClipBoard);
		func_add_param("buffer",		TypeStringPP);
		func_add_param("length",	TypeIntPs);
	add_func("HuiOpenDocument",		TypeVoid,			(void*)&HuiOpenDocument);
		func_add_param("filename",	TypeString);
	add_func("CreateTimer",			TypeInt,			(void*)&HuiCreateTimer);
	add_func("GetTime",				TypeFloat,			(void*)&HuiGetTime);
		func_add_param("timer",		TypeInt);
	// menu
	add_func("HuiCreateMenu",		TypeHuiMenuP,		(void*)&HuiCreateMenu);
	// window
	add_func("HuiCreateWindow",		TypeHuiWindowP,			(void*)&HuiCreateWindow);
		func_add_param("title",		TypeString);
		func_add_param("x",			TypeInt);
		func_add_param("y",			TypeInt);
		func_add_param("width",		TypeInt);
		func_add_param("height",	TypeInt);
		func_add_param("message_func",	TypePointer);
	add_func("HuiCreateNixWindow",	TypeHuiWindowP,			(void*)&HuiCreateNixWindow);
		func_add_param("title",		TypeString);
		func_add_param("x",			TypeInt);
		func_add_param("y",			TypeInt);
		func_add_param("width",		TypeInt);
		func_add_param("height",	TypeInt);
		func_add_param("message_func",	TypePointer);
	add_func("HuiCreateDialog",		TypeHuiWindowP,			(void*)&HuiCreateDialog);
		func_add_param("title",		TypeString);
		func_add_param("width",		TypeInt);
		func_add_param("height",	TypeInt);
		func_add_param("root",		TypeHuiWindowP);
		func_add_param("allow_root",TypeBool);
		func_add_param("message_func",	TypePointer);
	add_func("HuiWinClose",			TypeVoid,				(void*)&HuiCloseWindow);
		func_add_param("w",			TypeHuiWindowP);
	
	msg_db_l(1);
}


void ScriptInit()
{
	msg_db_r("ScriptInit", 1);

	AsmInit();

	SIAddTypes();
	SIAddBasicCommands();
	SIAddClasses();
	SIAddOperators();
	SIAddConsts();
	SIAddExtVars();
	SIAddSuperArrays();
	SIAddCommands();




	add_type_cast(10,	TypeInt,		TypeFloat,	"i2f",	(void*)&CastInt2Float);
	add_type_cast(20,	TypeFloat,		TypeInt,	"f2i",	(void*)&CastFloat2Int);
	add_type_cast(10,	TypeInt,		TypeChar,	"i2c",	(void*)&CastInt2Char);
	add_type_cast(20,	TypeChar,		TypeInt,	"c2i",	(void*)&CastChar2Int);
	add_type_cast(50,	TypePointer,	TypeBool,	"p2b",	(void*)&CastPointer2Bool);
	add_type_cast(50,	TypeInt,		TypeString,	"i2s",	(void*)&CastInt2StringP);
	add_type_cast(50,	TypeFloat,		TypeString,	"f2sf",	(void*)&CastFloat2StringP);
	add_type_cast(50,	TypeBool,		TypeString,	"b2s",	(void*)&CastBool2StringP);
	add_type_cast(50,	TypePointer,	TypeString,	"p2s",	(void*)&CastPointer2StringP);
	add_type_cast(50,	TypeVector,		TypeString,	"v2s",	(void*)&CastVector2StringP);
	add_type_cast(50,	TypeComplex,	TypeString,	"complex2s",	(void*)&CastComplex2StringP);
	add_type_cast(50,	TypeColor,		TypeString,	"color2s",	(void*)&CastFFFF2StringP);
	add_type_cast(50,	TypeQuaternion,	TypeString,	"quaternion2s",	(void*)&CastFFFF2StringP);
	add_type_cast(50,	TypePlane,		TypeString,	"plane2s",	(void*)&CastFFFF2StringP);
	add_type_cast(50,	TypeRect,		TypeString,	"rect2s",	(void*)&CastFFFF2StringP);
	//add_type_cast(50,	TypeClass,		TypeString,	"f2s",	(void*)&CastFloat2StringP);




	SIAddHuiStuff();


	msg_db_l(1);
}

void ScriptResetSemiExternalData()
{
	msg_db_r("ScriptResetSemiExternalData", 1);
	for (int i=PreExternalVar.size()-1;i>=0;i--)
		if (PreExternalVar[i].IsSemiExternal){
			delete[](PreExternalVar[i].Name);
			PreExternalVar.erase(PreExternalVar.begin() + i);
		}
	for (int i=PreCommand.size()-1;i>=0;i--)
		if (PreCommand[i].IsSemiExternal){
			delete[](PreCommand[i].Name);
			PreCommand.erase(PreCommand.begin() + i);
		}
	msg_db_l(1);
}

// program variables - specific to the surrounding program, can't always be there...
void ScriptLinkSemiExternalVar(const char *name, void *pointer)
{
	msg_db_r("ScriptLinkSemiExternalVar", 1);
	sPreExternalVar v;
	v.Name = new char[strlen(name) + 1];
	strcpy((char*)v.Name, (char*)name);
	v.Pointer = pointer;
	v.Type = TypeUnknown; // unusable until defined via "extern" in the script!
	v.IsSemiExternal = true; // ???
	PreExternalVar.push_back(v);
	msg_db_l(1);
}

// program functions - specific to the surrounding program, can't always be there...
void ScriptLinkSemiExternalFunc(const char *name, void *pointer)
{
	sPreCommand c;
	c.Name = new char[strlen(name) + 1];
	strcpy((char*)c.Name, (char*)name);
	c.IsClassFunction = false;
	c.Func = pointer;
	c.ReturnType = TypeUnknown; // unusable until defined via "extern" in the script!
	c.IsSemiExternal = true;
	PreCommand.push_back(c);
}	

std::vector<sScriptLocation> ScriptLocation;

void ScriptEnd()
{
	DeleteAllScripts(true, true);

	ScriptResetSemiExternalData();
	
	// locations
	ScriptLocation.clear();

	for (int i=0;i<PreType.size();i++)
		delete(PreType[i]);
	PreType.clear();

	PreOperator.clear();

	for (int i=0;i<PreClass.size();i++){
		PreClass[i]->Element.clear();
		for (int j=0;j<PreClass[i]->Function.size();j++)
			if (PreClass[i]->Function[j].Kind == KindCompilerFunction) // always true...
				delete[](PreCommand[PreClass[i]->Function[j].Nr].Name);
		PreClass[i]->Function.clear();
		delete(PreClass[i]);
	}
	PreClass.clear();

	PreConstant.clear();
	PreGlobalVar.clear();
	PreExternalVar.clear();
}

