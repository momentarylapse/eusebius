/*----------------------------------------------------------------------------*\
| Script Data                                                                  |
| -> "standart library" for the scripting system                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2007.03.19 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "script_data.h"

#include <string.h>
#include "nix.h"
#include "00_config.h"
#ifdef _X_ALLOW_CAMERA_
	#include "camera.h"
#endif
#ifdef _X_ALLOW_TERRAIN_
	#include "terrain.h"
#endif
#ifdef _X_ALLOW_GOD_
	#include "god.h"
#endif
#ifdef _X_ALLOW_META_
	#include "meta.h"
#endif

#ifdef _X_ALLOW_GOD_
	extern void SetMessage(char *msg);
#endif

char ScriptDataVersion[128]="0.4.2.2";



// ################################################################################################
//                                        Typen
// ################################################################################################


sType TypeUnknown	={"-\?\?\?-"	,0					,0	,false	,NULL}; // should not appear anywhere....or else we're screwed up!
sType TypeVoid		={"void"		,0					,0	,false	,NULL};
sType TypePointer	={"void*"		,PointerSize		,0	,true	,NULL}; // stellvertretend fuer alle Pointer-Arten
sType TypeStruct	={"-struct-"	,0					,0	,false	,NULL}; // stellvertretend fuer alle Struktur-Arten
sType TypeBool		={"bool"		,1					,0	,false	,NULL};
sType TypeBoolList	={"bool[]"		,0					,64	,false	,&TypeBool};
sType TypeInt		={"int"			,4					,0	,false	,NULL};
sType TypeIntP		={"int*"		,PointerSize		,0	,true	,&TypeInt};
sType TypeIntList	={"int[]"		,0					,64	,false	,&TypeInt};
sType TypeIntList2	={"int[][]"		,0					,4	,false	,&TypeIntList};
sType TypeFloat		={"float"		,4					,0	,false	,NULL};
sType TypeFloatP	={"float*"		,PointerSize		,0	,true	,&TypeFloat};
sType TypeFloatList	={"float[]"		,0					,64	,false	,&TypeFloat};
sType TypeChar		={"char"		,1					,0	,false	,NULL};
sType TypeString	={"string"		,256*TypeChar.Size	,256,false	,&TypeChar};
sType TypeStringP	={"string*"		,PointerSize		,0	,true	,&TypeString};
sType TypeStringList={"string[]"	,0					,64	,false	,&TypeString};
sType TypeStringPList={"string*[]"	,0					,64	,false	,&TypeStringP};
sType TypeVector	={"vector"		,12					,0	,false	,NULL};
sType TypeVectorP	={"vector*"		,PointerSize		,0	,true	,&TypeVector};
sType TypeVectorList={"vector[]"	,64*TypeVector.Size	,64	,false	,&TypeVector};
sType TypeRect		={"rect"		,16					,0	,false	,NULL};
sType TypeRectP		={"rect*"		,PointerSize		,0	,true	,&TypeRect};
sType TypeMatrix	={"matrix"		,64					,0	,false	,NULL};
sType TypeMatrixP	={"matrix*"		,PointerSize		,0	,true	,&TypeMatrix};
sType TypeQuaternion={"quaternion"	,16					,0	,false	,NULL};
sType TypeQuaternionP={"quaternion*",PointerSize		,0	,true	,&TypeQuaternion};
sType TypePlane		={"plane"		,16					,0	,false	,NULL};
sType TypePlaneP	={"plane*"		,PointerSize		,0	,true	,&TypePlane};
sType TypeColor		={"color"		,16					,0	,false	,NULL};
sType TypeColorP	={"color*"		,PointerSize		,0	,true	,&TypeColor};
sType TypeObject	={"-CObject-"	,0					,0	,false	,NULL};
sType TypeObjectP	={"object"		,PointerSize		,0	,true	,&TypeObject};
sType TypeModel		={"-CModel-"	,0					,0	,false	,NULL};
sType TypeModelP	={"model"		,PointerSize		,0	,true	,&TypeModel};
sType TypeModelPP	={"model*"		,PointerSize		,0	,true	,&TypeModelP};
sType TypeItem		={"-sItem-"		,PointerSize		,0	,false	,NULL};
sType TypeItemP		={"item"		,PointerSize		,0	,true	,&TypeItem};
sType TypeItemPP	={"item*"		,0					,0	,true	,&TypeItemP};
sType TypeFile		={"file"		,PointerSize		,0	,true	,&TypeVoid};
sType TypeDate		={"date"		,sizeof(sDate)		,0	,false	,NULL};

#ifdef _X_ALLOW_GOD_
	sType TypeObjectPList	={"object[]"	,TypeObjectP.Size*GOD_MAX_OBJECTS,GOD_MAX_OBJECTS,false	,&TypeObjectP};
	sType TypeText			={"-sText-"		,sizeof(sText)					,0			,false	,NULL};
	sType TypeTextP			={"text"		,PointerSize					,0			,true	,&TypeText};
	sType TypePicture		={"-sPicture-"	,sizeof(sPicture)				,0			,false	,NULL};
	sType TypePictureP		={"picture"		,PointerSize					,0			,true	,&TypePicture};
	sType TypePicture3D		={"-sPicture3d-",sizeof(sPicture3D)				,0			,false	,NULL};
	sType TypePicture3DP	={"picture3d"	,PointerSize					,0			,true	,&TypePicture3D};
	sType TypeGrouping		={"-sGrouping-"	,sizeof(sGrouping)				,0			,false	,NULL};
	sType TypeGroupingP		={"grouping"	,PointerSize					,0			,true	,&TypeGrouping};
	sType TypeParticle		={"-sParticle-"	,sizeof(sParticle)				,0			,false	,NULL};
	sType TypeParticleP		={"particle"	,PointerSize					,0			,true	,&TypeParticle};
	sType TypeBeam			={"-sBeam-"		,sizeof(sParticle)				,0			,false	,NULL};
	sType TypeBeamP			={"beam"		,PointerSize					,0			,true	,&TypeBeam};
	sType TypeView			={"-sView-"		,sizeof(sView)					,0			,false	,NULL};
	sType TypeViewP			={"view"		,PointerSize					,0			,true	,&TypeView};
	sType TypeSkin			={"-sSkin-"		,sizeof(sSkin)					,0			,false	,NULL};
	sType TypeSkinP			={"skin"		,PointerSize					,0			,true	,&TypeSkin};
	sType TypeSkinPList		={"skin[]"		,PointerSize*5					,5			,true	,&TypeSkinP};
	sType TypeFog			={"fog"			,sizeof(sFog)					,0			,false	,NULL};
	sType TypeTerrain		={"terrain"		,sizeof(CTerrain)				,0			,false	,NULL};
	sType TypeTerrainP		={"terrain*"	,PointerSize					,0			,true	,&TypeTerrain};
	sType TypeTerrainPList	={"terrain*[]"	,64*TypeTerrainP.Size			,64			,false	,&TypeTerrainP};
#else
	sType TypeObjectPList	={"object[]"	,0								,64			,false	,&TypeObjectP};
	sType TypeText			={"-sText-"		,0								,0			,false	,NULL};
	sType TypeTextP			={"text"		,PointerSize					,0			,true	,&TypeText};
	sType TypePicture		={"-sPicture-"	,0								,0			,false	,NULL};
	sType TypePictureP		={"picture"		,PointerSize					,0			,true	,&TypePicture};
	sType TypePicture3D		={"-sPicture3d-",0								,0			,false	,NULL};
	sType TypePicture3DP	={"picture3d"	,PointerSize					,0			,true	,&TypePicture3D};
	sType TypeGrouping		={"-sGrouping-"	,0								,0			,false	,NULL};
	sType TypeGroupingP		={"grouping"	,PointerSize					,0			,true	,&TypeGrouping};
	sType TypeParticle		={"-sParticle-"	,0								,0			,false	,NULL};
	sType TypeParticleP		={"particle"	,PointerSize					,0			,true	,&TypeParticle};
	sType TypeBeam			={"-sBeam-"		,0								,0			,false	,NULL};
	sType TypeBeamP			={"beam"		,PointerSize					,0			,true	,&TypeBeam};
	sType TypeView			={"-sView-"		,0								,0			,false	,NULL};
	sType TypeViewP			={"view"		,PointerSize					,0			,true	,&TypeView};
	sType TypeSkin			={"-sSkin-"		,0								,0			,false	,NULL};
	sType TypeSkinP			={"skin"		,PointerSize					,0			,true	,&TypeSkin};
	sType TypeSkinPList		={"skin[]"		,PointerSize*5					,5			,true	,&TypeSkinP};
	sType TypeFog			={"fog"			,0								,0			,false	,NULL};
	sType TypeTerrain		={"terrain"		,0								,0			,false	,NULL};
	sType TypeTerrainP		={"terrain*"	,PointerSize					,0			,true	,&TypeTerrain};
	sType TypeTerrainPList	={"terrain*[]"	,64*TypeTerrainP.Size			,64			,false	,&TypeTerrainP};
#endif

int NumPreTypes=62;
sType *PreType[SCRIPT_MAX_TYPES]={
	&TypeUnknown,
	&TypeVoid,
	&TypePointer,
	&TypeStruct,
	&TypeBool,
	&TypeBoolList,
	&TypeInt,
	&TypeIntP,
	&TypeIntList,
	&TypeIntList2,
	&TypeFloat,
	&TypeFloatP,
	&TypeFloatList,
	&TypeChar,
	&TypeString,
	&TypeStringP,
	&TypeStringList,
	&TypeStringPList,
	&TypeVector,
	&TypeVectorP,
	&TypeRect,
	&TypeRectP,
	&TypeMatrix,
	&TypeMatrixP,
	&TypeQuaternion,
	&TypeQuaternionP,
	&TypePlane,
	&TypePlaneP,
	&TypeColor,
	&TypeColorP,
	&TypeObject,
	&TypeObjectP,
	&TypeObjectPList,
	&TypeModel,
	&TypeModelP,
	&TypeModelPP,
	&TypeItem,
	&TypeItemP,
	&TypeItemPP,
	&TypeTerrain,
	&TypeTerrainP,
	&TypeTerrainPList,
	&TypeFile,
	&TypeDate,
	&TypeText,
	&TypeTextP,
	&TypePicture,
	&TypePictureP,
	&TypePicture3D,
	&TypePicture3DP,
	&TypeGrouping,
	&TypeGroupingP,
	&TypeParticle,
	&TypeParticleP,
	&TypeBeam,
	&TypeBeamP,
	&TypeView,
	&TypeViewP,
	&TypeSkin,
	&TypeSkinP,
	&TypeSkinPList,
	&TypeFog
};



// ################################################################################################
//                                        Operatoren
// ################################################################################################
//   ohne Typen ("primitive")
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
	OperatorAnd,			// &&
	OperatorOr,				// ||
	OperatorModulo,			//  %
	OperatorBitAnd,			//  &
	OperatorBitOr,			//  |
	OperatorShiftLeft,		// <<
	OperatorShiftRight,		// >>
	OperatorIncrease,		// ++
	OperatorDecrease,		// --
	NUM_PRIMITIVE_OPERATORS
};
int NumPrimitiveOperators=NUM_PRIMITIVE_OPERATORS;

sPrimitiveOperator PrimitiveOperator[NUM_PRIMITIVE_OPERATORS]={
	{"=",	OperatorAssign,			1},
	{"+",	OperatorAdd,			11},
	{"-",	OperatorSubtract,		11},
	{"*",	OperatorMultiply,		12},
	{"/",	OperatorDivide,			12},
	{"+=",	OperatorAddS,			1},
	{"-=",	OperatorSubtractS,		1},
	{"*=",	OperatorMultiplyS,		1},
	{"/=",	OperatorDivideS,		1},
	{"==",	OperatorEqual,			8},
	{"!=",	OperatorNotEqual,		8},
	{"!",	OperatorNegate,			2},
	{"<",	OperatorSmaller,		9},
	{">",	OperatorGreater,		9},
	{"<=",	OperatorSmallerEqual,	9},
	{">=",	OperatorGreaterEqual,	9},
	{"&&",	OperatorAnd,			4},
	{"||",	OperatorOr,				3},
	{"%",	OperatorModulo,			12},
	{"&",	OperatorBitAnd,			7},
	{"|",	OperatorBitOr,			5},
	{"<<",	OperatorShiftLeft,		10},
	{">>",	OperatorShiftRight,		10},
	{"++",	OperatorIncrease,		2},
	{"--",	OperatorDecrease,		2}
// Level = 15 - (offizielle C-Operator Prioritaet)
// Prioritaet aus "C als erste Programmiersprache", Seite 552
};

//   mit Typen

int NumPreOperators=NUM_PRE_OPERATORS;

sPreOperator PreOperator[NUM_PRE_OPERATORS]={
	{OperatorPointerAssign,		OperatorAssign,			&TypeVoid,		&TypePointer,	&TypePointer},
	{OperatorPointerEqual,		OperatorEqual,			&TypeBool,		&TypePointer,	&TypePointer},
	{OperatorPointerNotEqual,	OperatorNotEqual,		&TypeBool,		&TypePointer,	&TypePointer},
	{OperatorCharAssign,		OperatorAssign,			&TypeVoid,		&TypeChar,		&TypeChar},
	{OperatorCharEqual,			OperatorEqual,			&TypeBool,		&TypeChar,		&TypeChar},
	{OperatorCharNotEqual,		OperatorNotEqual,		&TypeBool,		&TypeChar,		&TypeChar},
	{OperatorCharAdd,			OperatorAdd,			&TypeChar,		&TypeChar,		&TypeChar},
	{OperatorCharSubtractS,		OperatorSubtractS,		&TypeChar,		&TypeChar,		&TypeChar},
	{OperatorCharAddS,			OperatorAddS,			&TypeChar,		&TypeChar,		&TypeChar},
	{OperatorCharSubtract,		OperatorSubtract,		&TypeChar,		&TypeChar,		&TypeChar},
	{OperatorCharBitAnd,		OperatorBitAnd,			&TypeChar,		&TypeChar,		&TypeChar},
	{OperatorCharBitOr,			OperatorBitOr,			&TypeChar,		&TypeChar,		&TypeChar},
	{OperatorCharNegate,		OperatorSubtract,		&TypeChar,		&TypeVoid,		&TypeChar},
	{OperatorBoolAssign,		OperatorAssign,			&TypeVoid,		&TypeBool,		&TypeBool},
	{OperatorBoolEqual,			OperatorEqual,			&TypeBool,		&TypeBool,		&TypeBool},
	{OperatorBoolNotEqual,		OperatorNotEqual,		&TypeBool,		&TypeBool,		&TypeBool},
	{OperatorBoolGreater,		OperatorGreater,		&TypeBool,		&TypeBool,		&TypeBool},
	{OperatorBoolGreaterEqual,	OperatorGreaterEqual,	&TypeBool,		&TypeBool,		&TypeBool},
	{OperatorBoolSmaller,		OperatorSmaller,		&TypeBool,		&TypeBool,		&TypeBool},
	{OperatorBoolSmallerEqual,	OperatorSmallerEqual,	&TypeBool,		&TypeBool,		&TypeBool},
	{OperatorBoolAnd,			OperatorAnd,			&TypeBool,		&TypeBool,		&TypeBool},
	{OperatorBoolOr,			OperatorOr,				&TypeBool,		&TypeBool,		&TypeBool},
	{OperatorBoolNegate,		OperatorNegate,			&TypeBool,		&TypeVoid,		&TypeBool},
	{OperatorIntAssign,			OperatorAssign,			&TypeVoid,		&TypeInt,		&TypeInt},
	{OperatorIntAdd,			OperatorAdd,			&TypeInt,		&TypeInt,		&TypeInt},
	{OperatorIntSubtract,		OperatorSubtract,		&TypeInt,		&TypeInt,		&TypeInt},
	{OperatorIntMultiply,		OperatorMultiply,		&TypeInt,		&TypeInt,		&TypeInt},
	{OperatorIntDivide,			OperatorDivide,			&TypeInt,		&TypeInt,		&TypeInt},
	{OperatorIntAddS,			OperatorAddS,			&TypeVoid,		&TypeInt,		&TypeInt},
	{OperatorIntSubtractS,		OperatorSubtractS,		&TypeVoid,		&TypeInt,		&TypeInt},
	{OperatorIntMultiplyS,		OperatorMultiplyS,		&TypeVoid,		&TypeInt,		&TypeInt},
	{OperatorIntDivideS,		OperatorDivideS,		&TypeVoid,		&TypeInt,		&TypeInt},
	{OperatorIntModulo,			OperatorModulo,			&TypeInt,		&TypeInt,		&TypeInt},
	{OperatorIntEqual,			OperatorEqual,			&TypeBool,		&TypeInt,		&TypeInt},
	{OperatorIntNotEqual,		OperatorNotEqual,		&TypeBool,		&TypeInt,		&TypeInt},
	{OperatorIntGreater,		OperatorGreater,		&TypeBool,		&TypeInt,		&TypeInt},
	{OperatorIntGreaterEqual,	OperatorGreaterEqual,	&TypeBool,		&TypeInt,		&TypeInt},
	{OperatorIntSmaller,		OperatorSmaller,		&TypeBool,		&TypeInt,		&TypeInt},
	{OperatorIntSmallerEqual,	OperatorSmallerEqual,	&TypeBool,		&TypeInt,		&TypeInt},
	{OperatorIntBitAnd,			OperatorBitAnd,			&TypeInt,		&TypeInt,		&TypeInt},
	{OperatorIntBitOr,			OperatorBitOr,			&TypeInt,		&TypeInt,		&TypeInt},
	{OperatorIntShiftRight,		OperatorShiftRight,		&TypeInt,		&TypeInt,		&TypeInt},
	{OperatorIntShiftLeft,		OperatorShiftLeft,		&TypeInt,		&TypeInt,		&TypeInt},
	{OperatorIntNegate,			OperatorSubtract,		&TypeInt,		&TypeVoid,		&TypeInt},
	{OperatorIntIncrease,		OperatorIncrease,		&TypeInt,		&TypeInt,		&TypeVoid},
	{OperatorIntIncrease,		OperatorDecrease,		&TypeInt,		&TypeInt,		&TypeVoid},
	{OperatorFloatAssign,		OperatorAssign,			&TypeVoid,		&TypeFloat,		&TypeFloat},
	{OperatorFloatAdd,			OperatorAdd,			&TypeFloat,		&TypeFloat,		&TypeFloat},
	{OperatorFloatSubtract,		OperatorSubtract,		&TypeFloat,		&TypeFloat,		&TypeFloat},
	{OperatorFloatMultiply,		OperatorMultiply,		&TypeFloat,		&TypeFloat,		&TypeFloat},
	{OperatorFloatMultiplyFI,	OperatorMultiply,		&TypeFloat,		&TypeFloat,		&TypeInt},
	{OperatorFloatMultiplyIF,	OperatorMultiply,		&TypeFloat,		&TypeInt,		&TypeFloat},
	{OperatorFloatDivide,		OperatorDivide,			&TypeFloat,		&TypeFloat,		&TypeFloat},
	{OperatorFloatAddS,			OperatorAddS,			&TypeVoid,		&TypeFloat,		&TypeFloat},
	{OperatorFloatSubtractS,	OperatorSubtractS,		&TypeVoid,		&TypeFloat,		&TypeFloat},
	{OperatorFloatMultiplyS,	OperatorMultiplyS,		&TypeVoid,		&TypeFloat,		&TypeFloat},
	{OperatorFloatDivideS,		OperatorDivideS,		&TypeVoid,		&TypeFloat,		&TypeFloat},
	{OperatorFloatEqual,		OperatorEqual,			&TypeBool,		&TypeFloat,		&TypeFloat},
	{OperatorFloatNotEqual,		OperatorNotEqual,		&TypeBool,		&TypeFloat,		&TypeFloat},
	{OperatorFloatGreater,		OperatorGreater,		&TypeBool,		&TypeFloat,		&TypeFloat},
	{OperatorFloatGreaterEqual,	OperatorGreaterEqual,	&TypeBool,		&TypeFloat,		&TypeFloat},
	{OperatorFloatSmaller,		OperatorSmaller,		&TypeBool,		&TypeFloat,		&TypeFloat},
	{OperatorFloatSmallerEqual,	OperatorSmallerEqual,	&TypeBool,		&TypeFloat,		&TypeFloat},
	{OperatorFloatNegate,		OperatorSubtract,		&TypeFloat,		&TypeVoid,		&TypeFloat},
	{OperatorStringAssignAA,	OperatorAssign,			&TypeVoid,		&TypeString,	&TypeString},
	{OperatorStringAssignAP,	OperatorAssign,			&TypeVoid,		&TypeString,	&TypeStringP},
	{OperatorStringAddAA,		OperatorAdd,			&TypeString,	&TypeString,	&TypeString},
	{OperatorStringAddAP,		OperatorAdd,			&TypeString,	&TypeString,	&TypeStringP},
	{OperatorStringAddPA,		OperatorAdd,			&TypeString,	&TypeStringP,	&TypeString},
	{OperatorStringAddPP,		OperatorAdd,			&TypeString,	&TypeStringP,	&TypeStringP},
	{OperatorStringAddAAS,		OperatorAddS,			&TypeVoid,		&TypeString,	&TypeString},
	{OperatorStringAddAPS,		OperatorAddS,			&TypeVoid,		&TypeString,	&TypeStringP},
	{OperatorStringEqualAA,		OperatorEqual,			&TypeBool,		&TypeString,	&TypeString},
	{OperatorStringEqualPA,		OperatorEqual,			&TypeBool,		&TypeStringP,	&TypeString},
	{OperatorStringEqualAP,		OperatorEqual,			&TypeBool,		&TypeString,	&TypeStringP},
	{OperatorStringEqualPP,		OperatorEqual,			&TypeBool,		&TypeStringP,	&TypeStringP},
	{OperatorStringNotEqualAA,	OperatorNotEqual,		&TypeBool,		&TypeString,	&TypeString},
	{OperatorStringNotEqualPA,	OperatorNotEqual,		&TypeBool,		&TypeStringP,	&TypeString},
	{OperatorStringNotEqualAP,	OperatorNotEqual,		&TypeBool,		&TypeString,	&TypeStringP},
	{OperatorStringNotEqualPP,	OperatorNotEqual,		&TypeBool,		&TypeStringP,	&TypeStringP},
	{OperatorStructAssign,		OperatorAssign,			&TypeVoid,		&TypeStruct,	&TypeStruct},
	{OperatorStructEqual,		OperatorEqual,			&TypeStruct,	&TypeStruct,	&TypeStruct},
	{OperatorStructNotEqual,	OperatorNotEqual,		&TypeStruct,	&TypeStruct,	&TypeStruct},
	{OperatorVectorAdd,			OperatorAdd,			&TypeVector,	&TypeVector,	&TypeVector},
	{OperatorVectorSubtract,	OperatorSubtract,		&TypeVector,	&TypeVector,	&TypeVector},
	{OperatorVectorMultiplyVV,	OperatorMultiply,		&TypeVector,	&TypeVector,	&TypeVector},
	{OperatorVectorMultiplyVF,	OperatorMultiply,		&TypeVector,	&TypeVector,	&TypeFloat},
	{OperatorVectorMultiplyFV,	OperatorMultiply,		&TypeVector,	&TypeFloat,		&TypeVector},
	{OperatorVectorDivide,		OperatorDivide,			&TypeVector,	&TypeVector,	&TypeVector},
	{OperatorVectorDivideVF,	OperatorDivide,			&TypeVector,	&TypeVector,	&TypeFloat},
	{OperatorVectorAddS,		OperatorAddS,			&TypeVoid,		&TypeVector,	&TypeVector},
	{OperatorVectorSubtractS,	OperatorSubtractS,		&TypeVoid,		&TypeVector,	&TypeVector},
	{OperatorVectorMultiplyS,	OperatorMultiplyS,		&TypeVoid,		&TypeVector,	&TypeFloat},
	{OperatorVectorDivideS,		OperatorDivideS,		&TypeVoid,		&TypeVector,	&TypeFloat},
	{OperatorVectorNegate,		OperatorSubtract,		&TypeVector,	&TypeVoid,		&TypeVector}
};



// ################################################################################################
//                                        Strukturen & Elemente
// ################################################################################################

#ifdef _X_ALLOW_MODEL_
	static sText *_text;
	#define	GetDAText(x)		int(&_text->x)-int(_text)
	static sPicture *_picture;
	#define	GetDAPicture(x)		int(&_picture->x)-int(_picture)
	static sPicture3D *_picture3d;
	#define	GetDAPicture3D(x)	int(&_picture3d->x)-int(_picture3d)
	static sGrouping *_grouping;
	#define	GetDAGrouping(x)	int(&_grouping->x)-int(_grouping)
	static sParticle *_particle;
	#define	GetDAParticle(x)	int(&_particle->x)-int(_particle)
	static sItem *_item;
	#define	GetDAItem(x)		int(&_item->x)-int(_item)
	static CModel *_model;
	#define	GetDAModel(x)		int(&_model->x)-int(_model)
	static sSkin *_skin;
	#define	GetDASkin(x)		int(&_skin->x)-int(_skin)
#else
	#define	GetDAText(x)		0
	#define	GetDAPicture(x)		0
	#define	GetDAPicture3D(x)	0
	#define	GetDAGrouping(x)	0
	#define	GetDAParticle(x)	0
	#define	GetDABeam(x)		0
	#define	GetDAItem(x)		0
	#define	GetDAModel(x)		0
	#define	GetDASkin(x)		0
#endif
#ifdef _X_ALLOW_GOD_
	static sFog *_fog;
	#define	GetDAFog(x)			int(&_fog->x)-int(_fog)
#else
	#define	GetDAFog(x)			0
#endif
#ifdef _X_ALLOW_OBJECT_
	static CObject *_object;
	#define	GetDAObject(x)		int(&_object->x)-int(_object)
#else
	#define	GetDAObject(x)		0
#endif
#ifdef _X_ALLOW_CAMERA_
	static sView *_view;
	#define	GetDAView(x)		int(&_view->x)-int(_view)
#else
	#define	GetDAView(x)		0
#endif
#ifdef _X_ALLOW_TERRAIN_
	static CTerrain *_terrain;
	#define	GetDATerrain(x)		int(&_terrain->x)-int(_terrain)
#else
	#define	GetDATerrain(x)		0
#endif
static sDate *_date;
#define	GetDADate(x)		int(&_date->x)-int(_date)

#define NUM_PRE_STRUCTS		20
int NumPreStructs=NUM_PRE_STRUCTS;
sStruct PreStruct[SCRIPT_MAX_STRUCTS]={
	{&TypeVector,		3,	{	{"x",				&TypeFloat,		0},
								{"y",				&TypeFloat,		4},
								{"z",				&TypeFloat,		8}}},
	{&TypeQuaternion,	4,	{	{"x",				&TypeFloat,		0},
								{"y",				&TypeFloat,		4},
								{"z",				&TypeFloat,		8},
								{"w",				&TypeFloat,		12}}},
	{&TypeRect,			4,	{	{"x1",				&TypeFloat,		0},
								{"x2",				&TypeFloat,		4},
								{"y1",				&TypeFloat,		8},
								{"y2",				&TypeFloat,		12}}},
	{&TypeColor,		4,	{	{"a",				&TypeFloat,		0},
								{"r",				&TypeFloat,		4},
								{"g",				&TypeFloat,		8},
								{"b",				&TypeFloat,		12}}},
	{&TypePlane,		4,	{	{"a",				&TypeFloat,		0},
								{"b",				&TypeFloat,		4},
								{"c",				&TypeFloat,		8},
								{"d",				&TypeFloat,		12}}},
	{&TypeMatrix,		16,	{	{"_11",				&TypeFloat,		0},
								{"_12",				&TypeFloat,		4},
								{"_13",				&TypeFloat,		8},
								{"_14",				&TypeFloat,		12},
								{"_21",				&TypeFloat,		16},
								{"_22",				&TypeFloat,		20},
								{"_23",				&TypeFloat,		24},
								{"_24",				&TypeFloat,		28},
								{"_31",				&TypeFloat,		32},
								{"_32",				&TypeFloat,		36},
								{"_33",				&TypeFloat,		40},
								{"_34",				&TypeFloat,		44},
								{"_41",				&TypeFloat,		48},
								{"_42",				&TypeFloat,		52},
								{"_43",				&TypeFloat,		56},
								{"_44",				&TypeFloat,		60}}},
	{&TypePicture,		9,	{	{"Enabled",			&TypeBool,		GetDAPicture(Enabled)},
								{"TCInverted",		&TypeBool,		GetDAPicture(TCInverted)},
								{"Pos",				&TypeVector,	GetDAPicture(Pos)},
								{"Width",			&TypeFloat,		GetDAPicture(Width)},
								{"Height",			&TypeFloat,		GetDAPicture(Height)},
								{"Color",			&TypeColor,		GetDAPicture(Color)},
								{"Texture",			&TypeInt,		GetDAPicture(Texture)},
								{"Source",			&TypeRect,		GetDAPicture(Source)},
								{"ShaderFile",		&TypeInt,		GetDAPicture(ShaderFile)}}},
	{&TypePicture3D,	6,	{	{"Enabled",			&TypeBool,		GetDAPicture3D(Enabled)},
								{"Relative",		&TypeBool,		GetDAPicture3D(Relative)},
								{"Lighting",		&TypeBool,		GetDAPicture3D(Lighting)},
								{"z",				&TypeFloat,		GetDAPicture3D(z)},
								{"Matrix",			&TypeMatrix,	GetDAPicture3D(Matrix)},
								{"model",			&TypeModelP,	GetDAPicture3D(model)}}},
	{&TypeGrouping,		3,	{	{"Enabled",			&TypeBool,		GetDAGrouping(Enabled)},
								{"Pos",				&TypeVector,	GetDAGrouping(Pos)},
								{"Color",			&TypeColor,		GetDAGrouping(Color)}}},
	{&TypeText,			8,	{	{"Enabled",			&TypeBool,		GetDAText(Enabled)},
								{"Centric",			&TypeBool,		GetDAText(Centric)},
								{"Vertical",		&TypeBool,		GetDAText(Vertical)},
								{"Font",			&TypeInt,		GetDAText(Font)},
								{"Pos",				&TypeVector,	GetDAText(Pos)},
								{"Size",			&TypeFloat,		GetDAText(Size)},
								{"Color",			&TypeColor,		GetDAText(Color)},
								{"Str",				&TypeString,	GetDAText(Str)}}},
	{&TypeParticle,		11,	{	{"Enabled",			&TypeBool,		GetDAParticle(Enabled)},
								{"Suicidal",		&TypeBool,		GetDAParticle(Suicidal)},
								{"Pos",				&TypeVector,	GetDAParticle(Pos)},
								{"Vel",				&TypeVector,	GetDAParticle(Vel)},
								{"Ang",				&TypeVector,	GetDAParticle(Parameter)},
								{"TimeToLive",		&TypeFloat,		GetDAParticle(TimeToLive)},
								{"Radius",			&TypeFloat,		GetDAParticle(Radius)},
								{"Color",			&TypeColor,		GetDAParticle(Color)},
								{"Texture",			&TypeInt,		GetDAParticle(Texture)},
								{"Source",			&TypeRect,		GetDAParticle(Source)},
								{"Func",			&TypePointer,	GetDAParticle(Func)}}},
	{&TypeBeam,			11,	{	{"Enabled",			&TypeBool,		GetDAParticle(Enabled)},
								{"Suicidal",		&TypeBool,		GetDAParticle(Suicidal)},
								{"Pos",				&TypeVector,	GetDAParticle(Pos)},
								{"Vel",				&TypeVector,	GetDAParticle(Vel)},
								{"Length",			&TypeVector,	GetDAParticle(Parameter)},
								{"TimeToLive",		&TypeFloat,		GetDAParticle(TimeToLive)},
								{"Radius",			&TypeFloat,		GetDAParticle(Radius)},
								{"Color",			&TypeColor,		GetDAParticle(Color)},
								{"Texture",			&TypeInt,		GetDAParticle(Texture)},
								{"Source",			&TypeRect,		GetDAParticle(Source)},
								{"Func",			&TypePointer,	GetDAParticle(Func)}}},
	{&TypeObject,		24,	{	{"Pos",				&TypeVector,	GetDAObject(Pos)},
								{"Vel",				&TypeVector,	GetDAObject(Vel)},
								{"VelS",			&TypeVector,	GetDAObject(VelS)},
								{"Ang",				&TypeVector,	GetDAObject(Ang)},
								{"Rot",				&TypeVector,	GetDAObject(Rot)},
								{"Matrix",			&TypeMatrix,	GetDAObject(Matrix)},
								{"Name",			&TypeString,	GetDAObject(Name)},
								{"OnGround",		&TypeBool,		GetDAObject(OnGround)},
								{"GroundNormal",	&TypeVector,	GetDAObject(GroundNormal)},
								{"GFactor",			&TypeFloat,		GetDAObject(GFactor)},
								{"ID",				&TypeInt,		GetDAObject(ID)},
								{"Visible",			&TypeBool,		GetDAObject(Visible)},
								{"ActivePhysics",	&TypeBool,		GetDAObject(ActivePhysics)},
								{"PassivePhysics",	&TypeBool,		GetDAObject(PassivePhysics)},
								{"AllowShadow",		&TypeBool,		GetDAObject(AllowShadow)},
								{"model",			&TypeModelP,	GetDAObject(model)},
								{"Mass",			&TypeFloat,		GetDAObject(Mass)},
								{"Radius",			&TypeFloat,		GetDAObject(Radius)},
								{"Life",			&TypeFloat,		GetDAObject(Life)},
								{"MaxLife",			&TypeFloat,		GetDAObject(MaxLife)},
								{"ScriptVar",		&TypeFloatP,	GetDAObject(ScriptVar)},
								{"ScriptVarI",		&TypeIntP,		GetDAObject(ScriptVar)},
								{"Item",			&TypeItemPP,	GetDAObject(items)},
								{"ItemFilename",	&TypeStringP,	GetDAObject(ItemFilename)}}},
	{&TypeModel,		18,	{	{"Skin",			&TypeSkinPList,	GetDAModel(Skin)},
								{"Skin0",			&TypeSkinP,		GetDAModel(Skin[0])},
								{"Skin1",			&TypeSkinP,		GetDAModel(Skin[1])},
								{"Skin2",			&TypeSkinP,		GetDAModel(Skin[2])},
								{"Texture",			&TypeIntList,	GetDAModel(Texture)},
								{"ShaderFile",		&TypeInt,		GetDAModel(ShaderFile)},
								{"sub_model",		&TypeModelPP,	GetDAModel(sub_model)},
								{"PartMatrix",		&TypeMatrixP,	GetDAModel(PartMatrix)},
								{"DPartMatrix",		&TypeMatrixP,	GetDAModel(DPartMatrix)},
								{"FXAllEnabled",	&TypeBool,		GetDAModel(FXAllEnabled)},
								{"Min",				&TypeVector,	GetDAModel(Min)},
								{"Max",				&TypeVector,	GetDAModel(Max)},
								{"TestCollisions",	&TypeBool,		GetDAModel(TestCollisions)},
								{"Ambient",			&TypeColor,		GetDAModel(Ambient)},
								{"Diffuse",			&TypeColor,		GetDAModel(Diffuse)},
								{"Specular",		&TypeColor,		GetDAModel(Specular)},
								{"Emission",		&TypeColor,		GetDAModel(Emission)},
								{"Shininess",		&TypeFloat,		GetDAModel(Shininess)}}},
	{&TypeSkin,			5,	{	{"NumVertices",		&TypeInt,		GetDASkin(NumVertices)},
								{"NumSkinVertices",	&TypeInt,		GetDASkin(NumSkinVertices)},
								{"Vertex",			&TypeVectorP,	GetDASkin(Vertex)},
								{"SkinVertex",		&TypeFloatP,	GetDASkin(SkinVertex)},
								{"NumTriangles",	&TypeInt,		GetDASkin(NumTriangles)}}},
	{&TypeItem,			8,	{	{"Kind",			&TypeInt,		GetDAItem(Kind)},
								{"OID",				&TypeInt,		GetDAItem(OID)},
								{"Quantity",		&TypeInt,		GetDAItem(Quantity)},
								{"QuantityMax",		&TypeInt,		GetDAItem(QuantityMax)},
								{"ScriptVar",		&TypeFloatList,	GetDAItem(ScriptVar[0])},
								{"model",			&TypeModelP,	GetDAItem(model)},
								{"Name",			&TypeString,	GetDAItem(Name[0])},
								{"Description",		&TypeString,	GetDAItem(Description[0])}}},
	{&TypeTerrain,		8,	{	{"Pos",				&TypeVector,	GetDATerrain(Pos)},
								{"NumX",			&TypeInt,		GetDATerrain(NumX)},
								{"NumZ",			&TypeInt,		GetDATerrain(NumZ)},
								{"Height",			&TypeFloatP,	GetDATerrain(Height)},
								{"Pattern",			&TypeVector,	GetDATerrain(Pattern)},
								{"NumTextures",		&TypeInt,		GetDATerrain(NumTextures)},
								{"Texture",			&TypeIntList,	GetDATerrain(Texture)},
								{"TextureScale",	&TypeVectorList,GetDATerrain(TextureScale)}}},
	{&TypeView,			15,	{	{"Enabled",			&TypeBool,		GetDAView(Enabled)},
								{"RenderToTexture",	&TypeBool,		GetDAView(RenderToTexture)},
								{"Show",			&TypeBool,		GetDAView(Show)},
								{"Texture",			&TypeInt,		GetDAView(Texture)},
								{"Recursivity",		&TypeFloat,		GetDAView(Recursivity)},
								{"RecTranslation",	&TypeVector,	GetDAView(RecTranslation)},
								{"ShaderFile",		&TypeInt,		GetDAView(ShaderFile)},
								{"ShadedDisplays",	&TypeBool,		GetDAView(ShadedDisplays)},
								{"Pos",				&TypeVector,	GetDAView(Pos)},
								{"Ang",				&TypeVector,	GetDAView(Ang)},
								{"Vel",				&TypeVector,	GetDAView(Vel)},
								{"Rot",				&TypeVector,	GetDAView(Rot)},
								{"Zoom",			&TypeFloat,		GetDAView(Zoom)},
								{"Dest",			&TypeRect,		GetDAView(Dest)},
								{"z",				&TypeFloat,		GetDAView(z)}}},
	{&TypeFog,			6,	{	{"Enabled",			&TypeBool,		GetDAFog(Enabled)},
								{"Mode",			&TypeInt,		GetDAFog(Mode)},
								{"Start",			&TypeFloat,		GetDAFog(Start)},
								{"End",				&TypeFloat,		GetDAFog(End)},
								{"Density",			&TypeFloat,		GetDAFog(Density)},
								{"Color",			&TypeColor,		GetDAFog(Color)}}},
	{&TypeDate,			9,	{	{"time",			&TypeInt,		GetDADate(time)},
								{"year",			&TypeInt,		GetDADate(year)},
								{"month",			&TypeInt,		GetDADate(month)},
								{"day",				&TypeInt,		GetDADate(day)},
								{"hour",			&TypeInt,		GetDADate(hour)},
								{"minute",			&TypeInt,		GetDADate(minute)},
								{"second",			&TypeInt,		GetDADate(second)},
								{"milli_second",	&TypeInt,		GetDADate(milli_second)},
								{"day_of_week",		&TypeInt,		GetDADate(day_of_week)},
								{"day_of_year",		&TypeInt,		GetDADate(day_of_year)}}}
};


vector NullVector=vector(0,0,0);


// ################################################################################################
//                                        Konstanten
// ################################################################################################

#define NUM_PRE_CONSTANTS		175
int  NumPreConstants=NUM_PRE_CONSTANTS;
sPreConstant PreConstant[NUM_PRE_CONSTANTS]={
	// Pointer
	{"NULL",&TypePointer,NULL},
	// bool
	{"false",&TypeBool,(char*)false},{"true",&TypeBool,(char*)true},
	// float
	{"pi",&TypeFloat,*(char**)&pi},
	// vector
	{"v0",&TypeVector,(char*)&NullVector},
	// color
	{"White",&TypeColor,(char*)&White},
	{"Black",&TypeColor,(char*)&Black},
	{"Gray",&TypeColor,(char*)&Gray},
	{"Red",&TypeColor,(char*)&Red},
	{"Green",&TypeColor,(char*)&Green},
	{"Blue",&TypeColor,(char*)&Blue},
	{"Yellow",&TypeColor,(char*)&Yellow},
	{"Orange",&TypeColor,(char*)&Orange},
	// Tasten (int)
	{"KEY_LCONTROL",&TypeInt,(char*)KEY_LCONTROL},{"KEY_RCONTROL",&TypeInt,(char*)KEY_RCONTROL},
	{"KEY_LSHIFT",&TypeInt,(char*)KEY_LSHIFT},{"KEY_RSHIFT",&TypeInt,(char*)KEY_RSHIFT},{"KEY_LALT",&TypeInt,(char*)KEY_LALT},
	{"KEY_RALT",&TypeInt,(char*)KEY_RALT},{"KEY_ADD",&TypeInt,(char*)KEY_ADD},{"KEY_SUBTRACT",&TypeInt,(char*)KEY_SUBTRACT},
	{"KEY_FENCE",&TypeInt,(char*)KEY_FENCE},{"KEY_END",&TypeInt,(char*)KEY_END},{"KEY_NEXT",&TypeInt,(char*)KEY_NEXT},
	{"KEY_PRIOR",&TypeInt,(char*)KEY_PRIOR},{"KEY_UP",&TypeInt,(char*)KEY_UP},{"KEY_DOWN",&TypeInt,(char*)KEY_DOWN},
	{"KEY_LEFT",&TypeInt,(char*)KEY_LEFT},{"KEY_RIGHT",&TypeInt,(char*)KEY_RIGHT},{"KEY_RETURN",&TypeInt,(char*)KEY_RETURN},
	{"KEY_ESCAPE",&TypeInt,(char*)KEY_ESCAPE},{"KEY_INSERT",&TypeInt,(char*)KEY_INSERT},
	{"KEY_DELETE",&TypeInt,(char*)KEY_DELETE},{"KEY_SPACE",&TypeInt,(char*)KEY_SPACE},
	{"KEY_F1",&TypeInt,(char*)KEY_F1},{"KEY_F2",&TypeInt,(char*)KEY_F2},{"KEY_F3",&TypeInt,(char*)KEY_F3},
	{"KEY_F4",&TypeInt,(char*)KEY_F4},{"KEY_F5",&TypeInt,(char*)KEY_F5},{"KEY_F6",&TypeInt,(char*)KEY_F6},
	{"KEY_F7",&TypeInt,(char*)KEY_F7},{"KEY_F8",&TypeInt,(char*)KEY_F8},{"KEY_F9",&TypeInt,(char*)KEY_F9},
	{"KEY_F10",&TypeInt,(char*)KEY_F10},{"KEY_F11",&TypeInt,(char*)KEY_F11},{"KEY_F12",&TypeInt,(char*)KEY_F12},
	{"KEY_0",&TypeInt,(char*)KEY_0},{"KEY_1",&TypeInt,(char*)KEY_1},{"KEY_2",&TypeInt,(char*)KEY_2},
	{"KEY_3",&TypeInt,(char*)KEY_3},{"KEY_4",&TypeInt,(char*)KEY_4},{"KEY_5",&TypeInt,(char*)KEY_5},
	{"KEY_6",&TypeInt,(char*)KEY_6},{"KEY_7",&TypeInt,(char*)KEY_7},{"KEY_8",&TypeInt,(char*)KEY_8},{"KEY_9",&TypeInt,(char*)KEY_9},
	{"KEY_A",&TypeInt,(char*)KEY_A},{"KEY_B",&TypeInt,(char*)KEY_B},{"KEY_C",&TypeInt,(char*)KEY_C},{"KEY_D",&TypeInt,(char*)KEY_D},
	{"KEY_E",&TypeInt,(char*)KEY_E},{"KEY_F",&TypeInt,(char*)KEY_F},{"KEY_G",&TypeInt,(char*)KEY_G},{"KEY_H",&TypeInt,(char*)KEY_H},
	{"KEY_I",&TypeInt,(char*)KEY_I},{"KEY_J",&TypeInt,(char*)KEY_J},{"KEY_K",&TypeInt,(char*)KEY_K},{"KEY_L",&TypeInt,(char*)KEY_L},
	{"KEY_M",&TypeInt,(char*)KEY_M},{"KEY_N",&TypeInt,(char*)KEY_N},{"KEY_O",&TypeInt,(char*)KEY_O},{"KEY_P",&TypeInt,(char*)KEY_P},
	{"KEY_Q",&TypeInt,(char*)KEY_Q},{"KEY_R",&TypeInt,(char*)KEY_R},{"KEY_S",&TypeInt,(char*)KEY_S},{"KEY_T",&TypeInt,(char*)KEY_T},
	{"KEY_U",&TypeInt,(char*)KEY_U},{"KEY_V",&TypeInt,(char*)KEY_V},{"KEY_W",&TypeInt,(char*)KEY_W},{"KEY_X",&TypeInt,(char*)KEY_X},
	{"KEY_Y",&TypeInt,(char*)KEY_Y},{"KEY_Z",&TypeInt,(char*)KEY_Z},
	{"KEY_BACKSPACE",&TypeInt,(char*)KEY_BACKSPACE},{"KEY_TAB",&TypeInt,(char*)KEY_TAB},{"KEY_HOME",&TypeInt,(char*)KEY_HOME},
	{"KEY_NUM_0",&TypeInt,(char*)KEY_NUM_0},{"KEY_NUM_1",&TypeInt,(char*)KEY_NUM_1},{"KEY_NUM_2",&TypeInt,(char*)KEY_NUM_2},
	{"KEY_NUM_3",&TypeInt,(char*)KEY_NUM_3},{"KEY_NUM_4",&TypeInt,(char*)KEY_NUM_4},{"KEY_NUM_5",&TypeInt,(char*)KEY_NUM_5},
	{"KEY_NUM_6",&TypeInt,(char*)KEY_NUM_6},{"KEY_NUM_7",&TypeInt,(char*)KEY_NUM_7},
	{"KEY_NUM_8",&TypeInt,(char*)KEY_NUM_8},{"KEY_NUM_9",&TypeInt,(char*)KEY_NUM_9},
	{"KEY_NUM_ADD",&TypeInt,(char*)KEY_NUM_ADD},{"KEY_NUM_SUBTRACT",&TypeInt,(char*)KEY_NUM_SUBTRACT},
	{"KEY_NUM_MULTIPLY",&TypeInt,(char*)KEY_NUM_MULTIPLY},{"KEY_NUM_DIVIDE",&TypeInt,(char*)KEY_NUM_DIVIDE},
	{"KEY_NUM_COMMA",&TypeInt,(char*)KEY_NUM_COMMA},{"KEY_NUM_ENTER",&TypeInt,(char*)KEY_NUM_ENTER},
	{"KEY_COMMA",&TypeInt,(char*)KEY_COMMA},{"KEY_DOT",&TypeInt,(char*)KEY_DOT},{"KEY_SMALLER",&TypeInt,(char*)KEY_SMALLER},
	{"KEY_SZ",&TypeInt,(char*)KEY_SZ},{"KEY_AE",&TypeInt,(char*)KEY_AE},{"KEY_OE",&TypeInt,(char*)KEY_OE},{"KEY_UE",&TypeInt,(char*)KEY_UE},
	{"NUM_KEYS",&TypeInt,(char*)HUI_NUM_KEYS},
	// Alpha-Konstanten
	{"AlphaNone",&TypeInt,(char*)AlphaNone},{"AlphaZero",&TypeInt,(char*)AlphaZero},{"AlphaOne",&TypeInt,(char*)AlphaOne},
	{"AlphaColorKey",&TypeInt,(char*)AlphaColorKey},{"AlphaColorKeyHard",&TypeInt,(char*)AlphaColorKeyHard},
	{"AlphaAdd",&TypeInt,(char*)AlphaAdd},{"AlphaMaterial",&TypeInt,(char*)AlphaMaterial},
	{"AlphaSourceColor",&TypeInt,(char*)AlphaSourceColor},{"AlphaSourceInvColor",&TypeInt,(char*)AlphaSourceInvColor},
	{"AlphaSourceAlpha",&TypeInt,(char*)AlphaSourceAlpha},{"AlphaSourceInvAlpha",&TypeInt,(char*)AlphaSourceInvAlpha},
	{"AlphaDestColor",&TypeInt,(char*)AlphaDestColor},{"AlphaDestInvColor",&TypeInt,(char*)AlphaDestInvColor},
	{"AlphaDestAlpha",&TypeInt,(char*)AlphaDestAlpha},{"AlphaDestInvAlpha",&TypeInt,(char*)AlphaDestInvAlpha},
	// Stencil-Konstanten
	{"StencilNone",&TypeInt,(char*)StencilNone},{"StencilIncrease",&TypeInt,(char*)StencilIncrease},
	{"StencilDecrease",&TypeInt,(char*)StencilDecrease},{"StencilSet",&TypeInt,(char*)StencilSet},
	{"StencilMaskEqual",&TypeInt,(char*)StencilMaskEqual},{"StencilMaskNotEqual",&TypeInt,(char*)StencilMaskNotEqual},
	{"StencilMaskLess",&TypeInt,(char*)StencilMaskLess},{"StencilMaskLessEqual",&TypeInt,(char*)StencilMaskLessEqual},
	{"StencilMaskGreater",&TypeInt,(char*)StencilMaskGreater},{"StencilMaskGreaterEqual",&TypeInt,(char*)StencilMaskGreaterEqual},
	{"StencilReset",&TypeInt,(char*)StencilReset},
	// Nebel
	{"FogLinear",&TypeInt,(char*)FogLinear},{"FogExp",&TypeInt,(char*)FogExp},{"FogExp2",&TypeInt,(char*)FogExp2},
	// Model-Ansichten
#ifdef _X_ALLOW_MODEL_
	{"SkinView0",&TypeInt,(char*)SkinView0},{"SkinView1",&TypeInt,(char*)SkinView0},{"SkinView2",&TypeInt,(char*)SkinView2},
#else
	{"SkinView0",&TypeInt,(char*)0},{"SkinView1",&TypeInt,(char*)0},{"SkinView2",&TypeInt,(char*)0},
#endif
	// Trace
#ifdef _X_ALLOW_MODEL_
	{"TraceModeSimpleTest",&TypeInt,(char*)TraceModeSimpleTest},
	{"TraceHitTerrain",&TypeInt,(char*)TraceHitTerrain},{"TraceHitObject",&TypeInt,(char*)TraceHitObject},
#else
	{"TraceModeSimpleTest",&TypeInt,(char*)0},{"TraceHitTerrain",&TypeInt,(char*)0},{"TraceHitObject",&TypeInt,(char*)0},
#endif
	// File Date
	{"FileDateModification",&TypeInt,(char*)FileDateModification},{"FileDateAccess",&TypeInt,(char*)FileDateAccess},
	{"FileDateCreation",&TypeInt,(char*)FileDateCreation},
	// hui window messages
	{"HUI_WIN_CLOSE",&TypeInt,(char*)HUI_WIN_CLOSE},			{"HUI_WIN_SIZE",&TypeInt,(char*)HUI_WIN_SIZE},
	{"HUI_WIN_MOVE",&TypeInt,(char*)HUI_WIN_MOVE},				{"HUI_WIN_RENDER",&TypeInt,(char*)HUI_WIN_RENDER},
	{"HUI_WIN_ERASEBKGND",&TypeInt,(char*)HUI_WIN_ERASEBKGND},	{"HUI_WIN_LBUTTONDOWN",&TypeInt,(char*)HUI_WIN_LBUTTONDOWN},
	{"HUI_WIN_LBUTTONUP",&TypeInt,(char*)HUI_WIN_LBUTTONUP},	{"HUI_WIN_RBUTTONDOWN",&TypeInt,(char*)HUI_WIN_RBUTTONDOWN},
	{"HUI_WIN_RBUTTONUP",&TypeInt,(char*)HUI_WIN_RBUTTONUP},	{"HUI_WIN_KEYDOWN",&TypeInt,(char*)HUI_WIN_KEYDOWN},
	{"HUI_WIN_KEYUP",&TypeInt,(char*)HUI_WIN_KEYUP},
	// Vorgaben der Engine
	{"NIX_API_NONE",&TypeInt,(char*)NIX_API_NONE},
#ifdef NIX_API_OPENGL
	{"NIX_API_OPENGL",&TypeInt,(char*)NIX_API_OPENGL},
#else
	{"NIX_API_OPENGL",&TypeInt,(char*)-1},
#endif
#ifdef NIX_API_DIRECTX8
	{"NIX_API_DIRECTX8",&TypeInt,(char*)NIX_API_DIRECTX8},
#else
	{"NIX_API_DIRECTX8",&TypeInt,(char*)-1},
#endif
#ifdef NIX_API_DIRECTX9
	{"NIX_API_DIRECTX9",&TypeInt,(char*)NIX_API_DIRECTX9},
#else
	{"NIX_API_DIRECTX9",&TypeInt,(char*)-1},
#endif
#ifdef NIX_IDE_VCS
	{"NIX_IDE_VCS",&TypeInt,(char*)1},
#else
	{"NIX_IDE_VCS",&TypeInt,(char*)-1},
#endif
#ifdef NIX_IDE_DEVCPP
	{"NIX_IDE_DEVCPP",&TypeInt,(char*)1},
#else
	{"NIX_IDE_DEVCPP",&TypeInt,(char*)-1},
#endif
#ifdef NIX_IDE_KDEVELOP
	{"NIX_IDE_KDEVELOP",&TypeInt,(char*)1},
#else
	{"NIX_IDE_KDEVELOP",&TypeInt,(char*)-1},
#endif
#ifdef NIX_OS_WINDOWS
	{"NIX_OS_WINDOWS",&TypeInt,(char*)1},
#else
	{"NIX_OS_WINDOWS",&TypeInt,(char*)-1},
#endif
#ifdef NIX_OS_LINUX
	{"NIX_OS_LINUX",&TypeInt,(char*)1},
#else
	{"NIX_OS_LINUX",&TypeInt,(char*)-1},
#endif
	{"VBTemp",&TypeInt,(char*)VBTemp},
	{"NixVersion",&TypeString,(char*)&NixVersion},
	// animation operations
#ifdef _X_ALLOW_MODEL_
	{"MoveOpSet",&TypeInt,(char*)MoveOpSet},					{"MoveOpSetNewKeyed",&TypeInt,(char*)MoveOpSetNewKeyed},
	{"MoveOpSetOldKeyed",&TypeInt,(char*)MoveOpSetOldKeyed},	{"MoveOpAdd1Factor",&TypeInt,(char*)MoveOpAdd1Factor},
	{"MoveOpMix1Factor",&TypeInt,(char*)MoveOpMix1Factor},		{"MoveOpMix2Factor",&TypeInt,(char*)MoveOpMix2Factor},
#else
	{"MoveOpSet",&TypeInt,NULL},	{"MoveOpSetNewKeyed",&TypeInt,NULL},	{"MoveOpSetOldKeyed",&TypeInt,NULL},
	{"MoveOpAdd1Factor",&TypeInt,NULL},	{"MoveOpMix1Factor",&TypeInt,NULL},	{"MoveOpMix2Factor",&TypeInt,NULL},
#endif
};



// ################################################################################################
//                                        vordef. globale Variablen
// ################################################################################################

#define NUM_PRE_GLOBAL_VARS		1
#define MAX_PRE_GLOBAL_VARS		128

int NumPreGlobalVars=NUM_PRE_GLOBAL_VARS;
sPreGlobalVar PreGlobalVar[MAX_PRE_GLOBAL_VARS]={
	{"this",	&TypeObjectP}
};


// ################################################################################################
//                                        Umgebungs-Variablen
// ################################################################################################


enum{
	ExternalVarAppName,
	ExternalVarAppFilename,
	ExternalVarAppDirectory,
	ExternalVarElapsed,
	ExternalVarElapsedRT,
	ExternalVarTimeScale,
	ExternalVarInitialWorldFile,
	ExternalVarCurrentWorldFile,
	ExternalVarTargetWidth,
	ExternalVarTargetHeight,
	ExternalVarScreenWidth,
	ExternalVarScreenHeight,
	ExternalVarScreenDepth,
	ExternalVarApi,
	ExternalVarMinDepth,
	ExternalVarMaxDepth,
	ExternalVarTextureLifeTime,
	ExternalVarNumObjects,
	ExternalVarObject,
	ExternalVarEgo,
	ExternalVarNumTerrains,
	ExternalVarTerrain,
	ExternalVarGravitation,
	ExternalVarCam,
	ExternalVarSkybox,
	ExternalVarSkyboxAng,
	ExternalVarBackGroundColor,
	ExternalVarFog,
	ExternalVarScriptVar,
	ExternalVarAmbient,
	ExternalVarSunLight,
	ExternalVarTraceHitType,
	ExternalVarTraceHitIndex,
	ExternalVarTraceHitSubModel,
	ExternalVarSuperGravitation,
	ExternalVarCurrentGrouping,
	ExternalVarSessionName,
	ExternalVarHostNames,
	ExternalVarNumAvailableHosts,
	ExternalVarHostName,
	ExternalVarHostSessionName,
	ExternalVarNetIAmHost,
	ExternalVarNetIAmClient,
	ExternalVarDebug,
	ExternalVarShowTimings,
	ExternalVarWireMode,
	ExternalVarConsoleEnabled,
	ExternalVarRecord,
	ExternalVarShadowLevel,
	ExternalVarMirrorLevelMax,
	ExternalVarFpsConstant,
	ExternalVarFpsWanted,
	ExternalVarFpsMax,
	ExternalVarFpsMin,
	ExternalVarDetailLevel,
	ExternalVarDetailFactorInv,
	ExternalVarVolumeMusic,
	ExternalVarVolumeSounds,
	ExternalVarNetworkEnabled,
	ExternalVarXFontColor,
	ExternalVarXFontIndex,
	ExternalVarGodNumMessages,
	ExternalVarGodMessage,
	ExternalVarDirSearchNum,
	ExternalVarDirSearchName,
	ExternalVarDirSearchIsDir,
	ExternalVarHuiFileDialogPath,
	ExternalVarHuiFileDialogFile,
	ExternalVarHuiFileDialogCompleteName,
	ExternalVarHuiRunning,
	NUM_PRE_EXTERNAL_VARS
};

int NumPreExternalVars=NUM_PRE_EXTERNAL_VARS;

sPreExternalVar PreExternalVar[NUM_PRE_EXTERNAL_VARS]={ // same order as enum above...!!!
	{"AppName",			ExternalVarAppName,			&TypeString,	NULL},
	{"AppFilename",		ExternalVarAppFilename,		&TypeString,	NULL},
	{"AppDirectory",	ExternalVarAppDirectory,	&TypeString,	NULL},
	{"elapsed",			ExternalVarElapsed,			&TypeFloat,		NULL},
	{"elapsed_rt",		ExternalVarElapsedRT,		&TypeFloat,		NULL},
	{"TimeScale",		ExternalVarTimeScale,		&TypeFloat,		NULL},
	{"InitialWorldFile",ExternalVarInitialWorldFile,&TypeString,	NULL},
	{"CurrentWorldFile",ExternalVarCurrentWorldFile,&TypeString,	NULL},
	{"TargetWidth",		ExternalVarTargetWidth,		&TypeInt,		NULL},
	{"TargetHeight",	ExternalVarTargetHeight,	&TypeInt,		NULL},
	{"ScreenWidth",		ExternalVarScreenWidth,		&TypeInt,		NULL},
	{"ScreenHeight",	ExternalVarScreenHeight,	&TypeInt,		NULL},
	{"ScreenDepth",		ExternalVarScreenDepth,		&TypeInt,		NULL},
	{"Api",				ExternalVarApi,				&TypeInt,		NULL},
	{"MinDepth",		ExternalVarMinDepth,		&TypeFloat,		NULL},
	{"MaxDepth",		ExternalVarMaxDepth,		&TypeFloat,		NULL},
	{"TextureLifeTime",	ExternalVarTextureLifeTime,	&TypeInt,		NULL},
	{"NumObjects",		ExternalVarNumObjects,		&TypeInt,		NULL},
	{"Object",			ExternalVarObject,			&TypeObjectPList,NULL},
	{"ego",				ExternalVarEgo,				&TypeObjectP,	NULL},
	{"NumTerrains",		ExternalVarNumTerrains,		&TypeInt,		NULL},
	{"Terrain",			ExternalVarTerrain,			&TypeTerrainPList,NULL},
	{"Gravitation",		ExternalVarGravitation,		&TypeVector,	NULL},
	{"Cam",				ExternalVarCam,				&TypeViewP,		NULL},
	{"SkyBox",			ExternalVarSkybox,			&TypeModelPP,	NULL},
	{"SkyBoxAng",		ExternalVarSkyboxAng,		&TypeVectorP,	NULL},
	{"BackGroundColor",	ExternalVarBackGroundColor,	&TypeColor,		NULL},
	{"Fog",				ExternalVarFog,				&TypeFog,		NULL},
	{"ScriptVar",		ExternalVarScriptVar,		&TypeFloatP,	NULL},
	{"Ambient",			ExternalVarAmbient,			&TypeColor,		NULL},
	{"SunLight",		ExternalVarSunLight,		&TypeInt,		NULL},
	{"TraceHitType",	ExternalVarTraceHitType,	&TypeInt,		NULL},
	{"TraceHitIndex",	ExternalVarTraceHitIndex,	&TypeInt,		NULL},
	{"TraceHitSubModel",ExternalVarTraceHitSubModel,&TypeInt,		NULL},
	{"SuperGravitation",ExternalVarSuperGravitation,&TypeBool,		NULL},
	{"CurrentGrouping",	ExternalVarCurrentGrouping,	&TypeGroupingP,	NULL},
	{"SessionName",		ExternalVarSessionName,		&TypeString,	NULL},
	{"HostNames",		ExternalVarHostNames,		&TypeString,	NULL},
	{"NumAvailableHosts",ExternalVarNumAvailableHosts,&TypeInt,		NULL},
	{"HostName",		ExternalVarHostName,		&TypeStringList,NULL},
	{"HostSessionName",	ExternalVarHostSessionName,	&TypeStringList,NULL},
	{"NetIAmHost",		ExternalVarNetIAmHost,		&TypeBool,		NULL},
	{"NetIAmClient",	ExternalVarNetIAmClient,	&TypeBool,		NULL},
	{"Debug",			ExternalVarDebug,			&TypeBool,		NULL},
	{"ShowTimings",		ExternalVarShowTimings,		&TypeBool,		NULL},
	{"WireMode",		ExternalVarWireMode,		&TypeBool,		NULL},
	{"ConsoleEnabled",	ExternalVarConsoleEnabled,	&TypeBool,		NULL},
	{"Record",			ExternalVarRecord,			&TypeBool,		NULL},
	{"ShadowLevel",		ExternalVarShadowLevel,		&TypeInt,		NULL},
	{"MirrorLevelMax",	ExternalVarMirrorLevelMax,	&TypeInt,		NULL},
	{"FpsConstant",		ExternalVarFpsConstant,		&TypeBool,		NULL},
	{"FpsWanted",		ExternalVarFpsWanted,		&TypeInt,		NULL},
	{"FpsMax",			ExternalVarFpsMax,			&TypeFloat,		NULL},
	{"FpsMin",			ExternalVarFpsMin,			&TypeFloat,		NULL},
	{"DetailLevel",		ExternalVarDetailLevel,		&TypeInt,		NULL},
	{"DetailFactorInv",	ExternalVarDetailFactorInv,	&TypeFloat,		NULL},
	{"VolumeMusic",		ExternalVarVolumeMusic,		&TypeFloat,		NULL},
	{"VolumeSounds",	ExternalVarVolumeSounds,	&TypeFloat,		NULL},
	{"NetworkEnabled",	ExternalVarNetworkEnabled,	&TypeBool,		NULL},
	{"XFontColor",		ExternalVarXFontColor,		&TypeColor,		NULL},
	{"XFontIndex",		ExternalVarXFontIndex,		&TypeInt,		NULL},
	{"GodNumMessages",	ExternalVarGodNumMessages,	&TypeInt,		NULL},
	{"GodMessage",		ExternalVarGodMessage,		&TypeIntList2,	NULL},
	{"dir_search_num",	ExternalVarDirSearchNum,	&TypeInt,		NULL},
	{"dir_search_name",	ExternalVarDirSearchName,	&TypeStringPList,NULL},
	{"dir_seachr_is_dir",ExternalVarDirSearchIsDir,	&TypeBoolList,	NULL},
	{"HuiFileDialogPath",ExternalVarHuiFileDialogPath,	&TypeString,	NULL},
	{"HuiFileDialogFile",ExternalVarHuiFileDialogFile,	&TypeString,	NULL},
	{"HuiFileDialogCompleteName",ExternalVarHuiFileDialogCompleteName,	&TypeString,NULL},
	{"HuiRunning",		ExternalVarHuiRunning,		&TypeBool,		NULL}
};


// ################################################################################################
//                                        Compiler-Funktionen
// ################################################################################################

#define ALLOW_SCRIPT_HUI


enum{
	// mathematical (float)
	CommandFloatSinus=NUM_INTERN_PRE_COMMANDS,
	CommandFloatCosinus,
	CommandFloatTangens,
	CommandFloatArcSinus,
	CommandFloatArcCosinus,
	CommandFloatArcTangens,
	CommandFloatArcTangens2,
	CommandFloatSquareRoot,
	CommandFloatSquare,
	CommandFloatPower,
	// type casting
	CommandIntFromString,
	CommandFloatFromString,
	CommandStringFromInt,
	CommandStringFromFloat,
	// random
	CommandRandInt,
	CommandRandFloat,
	// debug
	CommandBoolOut,
	CommandIntOut,
	CommandFloatOut,
	CommandStringOut,
	//CommandVectorOut,
	CommandPointerOut,
	// vectors
	CommandVectorNormalize,
	CommandVectorDir2Ang,
	CommandVectorDir2Ang2,
	CommandVectorAng2Dir,
	CommandVectorAngAdd,
	CommandVectorLength,
	CommandVectorLengthSqr,
	CommandVectorTransform,
	CommandVectorNormalTransform,
	CommandVectorDotProduct,
	CommandVectorCrossProduct,
	// martices
	CommandMatrixIdentity,
	CommandMatrixTranslation,
	CommandMatrixRotation,
	CommandMatrixRotationX,
	CommandMatrixRotationY,
	CommandMatrixRotationZ,
	CommandMatrixRotationQ,
	CommandMatrixRotationView,
	CommandMatrixScale,
	CommandMatrixMultiply,
	CommandMatrixInverse,
	// quaternions
	CommandQuaternionRotationV,
	CommandQuaternionRotationA,
	CommandQuaternionMultiply,
	CommandQuaternionToAngle,
	// other types
	CommandSetColorHSB,
	CommandTextureLoad,
	CommandXFDrawStr,
	CommandXFDrawVertStr,
	CommandXFGetWidth,
	CommandLoadXFont,
	CommandPictureCreate,
	CommandPicture3DCreate,
	CommandTextCreate,
	CommandGroupingCreate,
	CommandParticleCreate,
	CommandParticleRotCreate,
	CommandParticleBeamCreate,
	CommandModelLoad,
	CommandItemLoad,
	CommandItemGetOID,
	CommandViewCreate,
	CommandShaderFileLoad,

#ifdef ALLOW_SCRIPT_HUI
// HUI-GUI
	// common
	//CommandHuiCreate,
	CommandHuiSetIdleFunction,
	CommandHuiRun,
	CommandHuiEnd,
	CommandHuiWaitTillWindowClosed,
	CommandHuiSleep,
	CommandHuiFileDialogOpen,
	CommandHuiFileDialogSave,
	CommandHuiFileDialogDir,
	CommandHuiQuestionBox,
	CommandHuiInfoBox,
	CommandHuiErrorBox,
	CommandHuiConfigWriteInt,
	CommandHuiConfigWriteStr,
	CommandHuiConfigReadInt,
	CommandHuiConfigReadStr,
	CommandHuiCopyToClipBoard,
//	CommandHuiPasteFromClipBoard,
	CommandHuiOpenDocument,
	CommandHuiCreateTimer,
	CommandHuiGetTime,
	// menus
	CommandHuiCreateMenu,
	CommandHuiMenuOpenPopup,
	CommandHuiMenuAddEntry,
	CommandHuiMenuAddSeparator,
	CommandHuiMenuAddSubMenu,
	CommandHuiMenuCheckItem,
	CommandHuiMenuIsItemChecked,
	CommandHuiMenuEnableItem,
	CommandHuiMenuSetText,
	// windows
	CommandHuiCreateWindow,
	CommandHuiCreateNixWindow,
	CommandHuiCreateDialog,
	CommandHuiWinUpdate,
	CommandHuiWinHide,
	CommandHuiWinSetMaximized,
	CommandHuiWinIsMaximized,
	CommandHuiWinIsMinimized,
	CommandHuiWinSetID,
	CommandHuiWinSetFullscreen,
	CommandHuiWinSetTitle,
	CommandHuiWinSetPosition,
//	CommandHuiWinSetOuterior,
//	CommandHuiWinGetOuterior,
//	CommandHuiWinSetInerior,
//	CommandHuiGetInterior,
	CommandHuiWinSetCursorPos,
	CommandHuiWinActivate,
	CommandHuiWinIsActive,
	CommandHuiWinAddButton,
	CommandHuiWinAddCheckBox,
	CommandHuiWinAddText,
	CommandHuiWinAddEdit,
	CommandHuiWinAddGroup,
	CommandHuiWinAddComboBox,
	CommandHuiWinAddTabControl,
	CommandHuiWinSetTabCreationPage,
	CommandHuiWinAddListView,
	CommandHuiWinSetControlText,
	CommandHuiWinGetControlText,
	CommandHuiWinEnableControl,
	CommandHuiWinIsControlEnabled,
	CommandHuiWinCheckControl,
	CommandHuiWinIsControlChecked,
	CommandHuiWinGetControlSelection,
	CommandHuiWinGetControlSelectionM,
	CommandHuiWinSetControlSelection,
	CommandHuiWinResetControl,
	CommandHuiWinSetMenu,
	CommandNixInit,
#endif
	// engine
		// input
	CommandNixUpdateInput,
	CommandGetKey,
	CommandGetKeyDown,
	CommandGetKeyRhythmDown,
	CommandGetKeyChar,
	CommandGetKeyUp,
	CommandGetKeyName,
	CommandGetMouseX,
	CommandGetMouseY,
	CommandGetMouseDx,
	CommandGetMouseDy,
	CommandGetWheelD,
	CommandGetButL,
	CommandGetButM,
	CommandGetButR,
	CommandGetButLDown,
	CommandGetButMDown,
	CommandGetButRDown,
	CommandGetButLUp,
	CommandGetButMUp,
	CommandGetButRUp,
		// drawing
	CommandNixStart,
	CommandNixEnd,
	CommandNixKillWindows,
	CommandNixDraw2D,
	CommandNixDraw3D,
	CommandNixDrawStr,
	CommandNixDrawLineH,
	CommandNixDrawLineV,
	CommandNixDrawLine,
	CommandNixDrawSprite,
//	CommandNixDrawModel2D,
	CommandNixSetAlpha,
	CommandNixSetAlpha2,
	CommandNixSetStencil,
	CommandNixSetView,
	CommandNixSetView2,
	CommandNixSetZ,
	CommandNixSetMaterial,
	CommandNixSetFontColor,
	CommandNixEnableLighting,
	CommandNixSetVideoMode,
	CommandNixCreateDynamicTexture,
	CommandGetVecProject,
	CommandGetVecUnproject,
	CommandGetVecProjectRel,
	CommandGetVecUnprojectRel,
	CommandNixVBEmpty,
	CommandNixVBAddTrias,
	// sound
	CommandSoundEmit,
	CommandSoundCreate,
	CommandSoundSetData,
	CommandSoundDelete,
	// music
	CommandMusicLoad,
	CommandMusicPlay,
	CommandMusicStop,
	CommandMusicPause,
	CommandMusicSetRate,
	// effects
	CommandFXLightCreate,
	CommandFXLightSetDirectional,
	CommandFXLightSetRadial,
	CommandFXLightDelete,
	CommandFXLightEnable,
	// game
	CommandExitProgram,
	CommandScreenShot,
	CommandLoadWorld,
	CommandLoadGameFromHost,
	CommandSaveGameState,
	CommandLoadGameState,
	CommandFindHosts,
	CommandXDelete,
	CommandGetObjectByName,
	CommandCreateObject,
	CommandDeleteObject,
	CommandCamStartScript,
	CommandCamStopScript,
	CommandDrawSplashScreen,
	CommandModelCalcMove,
	CommandModelDraw,
	CommandModelGetVertex,
	CommandModelMoveReset,
	CommandModelMoveSet,
	CommandModelMoveGetFrames,
	CommandTerrainUpdate,
	CommandRenderScene,
	CommandGetG,
	CommandTrace,
	// files
	CommandFileOpen,
	CommandFileCreate,
	CommandFileClose,
	CommandFileGetDate,
	CommandFileWriteBool,
	CommandFileWriteInt,
	CommandFileWriteString,
	CommandFileReadBool,
	CommandFileReadInt,
	CommandFileReadString,
	CommandFileReadBoolC,
	CommandFileReadIntC,
	CommandFileReadStringC,
	CommandFileTestExistence,
	Commanddir_search,
//	CommandExecuteScript,
	// network
	CommandNetConnect,
	CommandNetAccept,
	CommandNetCreateSocket,
	CommandNetClose,
	CommandNetResetBuffer,
	CommandNetReadBuffer,
	CommandNetWriteBuffer,
	CommandNetReadyToWrite,
	CommandNetReadyToRead,
	CommandNetReadInt,
	CommandNetReadBool,
	CommandNetReadFloat,
	CommandNetReadVector,
	CommandNetReadStr,
	CommandNetWriteInt,
	CommandNetWriteBool,
	CommandNetWriteFloat,
	CommandNetWriteVector,
	CommandNetWriteStr,

	NUM_PRE_COMMANDS
};

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
float _cdecl randf(float m){	return (float)rand()*m/(float)RAND_MAX;	}
int _cdecl randi(int m){	return int((float)rand()*m/(float)RAND_MAX);	}
void _cdecl intout(int i){	msg_write(string2("IntOut:    %d",i));	}
void _cdecl floatout(float f){	msg_write(string2("FloatOut:  %.3f",f));	}
void _cdecl boolout(bool b){	if (b)	msg_write("BoolOut:  true");	else	msg_write("BoolOut:  false");	}
#ifdef _X_ALLOW_GOD_
	void _cdecl _stringout(char *str){	msg_write(string("StringOut: ",SysStr(str)));	}
#else
	void _cdecl _stringout(char *str){	msg_write(string("StringOut: ",str));	}
#endif
void _cdecl pointerout(char *p){	msg_write(string("PointerOut:    ",d2h((char*)&p,4,false)));	}
CFile *_cdecl FileOpen(char *filename){	CFile *f=new CFile();	f->Open(filename);		return f;	}
CFile *_cdecl FileCreate(char *filename){	CFile *f=new CFile();	f->Create(filename);	return f;	}
sDate _cdecl FileGetDate(CFile *f,int type){	return f->GetDate(type);	}
void _cdecl FileClose(CFile *f){	f->Close();	delete(f);	}
void _cdecl FileWriteStr(CFile *f,char *str){	f->WriteStr(str);	}
int _cdecl _Float2Int(float f){	return int(f);	}
int _cdecl dir_search_2(char *dir,char *filter,bool show_dirs)
{
	char dir2[256];
	strcpy(dir2,dir);
	if ((dir[0]!='/')&&(dir[1]!=':')){
		strcpy(dir2,hui->AppDirectory);
		strcat(dir2,dir);
	}
	return dir_search(dir2,filter,show_dirs);
}

void _cdecl HuiSetIdleFunction(void_function *idle_func)
{	hui->IdleFunction=idle_func;	}

#ifdef _X_ALLOW_MODEL_
	vector _cdecl ModelGetVertex(CModel *m,int index,int skin,matrix *mat)
	{	return m->GetVertex(index,skin,mat);	}
#else
	vector _cdecl ModelGetVertex(void *m,int index,int skin,matrix *mat)
	{	return vector(0,0,0);	}
#endif

void _cdecl HuiWindowSetMenu(CHuiWindow *win,CHuiMenu *menu)
{	win->Menu=menu;	}


typedef void (CFile::*tmf)();
typedef char *tcpa[4];
char *mf(tmf vmf)
{
	tcpa *cpa=(tcpa*)&vmf;
	return (*cpa)[0];
}


char *f_cp=(char*)1; // fuer gefakte (Compiler-) Funktionen
char *i_hui=(char*)2;

int NumPreCommands=NUM_PRE_COMMANDS;
sPreCommand PreCommand[NUM_PRE_COMMANDS]={

// "interne" Funktionen

	{"return",				CommandReturn,				NULL,	f_cp,				&TypeVoid,		1,	&TypeVoid,"return_value"}, // return: ParamType erst durch Parser festgelegt!
	{"if",					CommandIf,					NULL,	f_cp,				&TypeVoid,		1,	&TypeBool,"condition"},
	{"-if/else-",			CommandIfElse,				NULL,	f_cp,				&TypeVoid,		1,	&TypeBool,"condition"},
	{"while",				CommandWhile,				NULL,	f_cp,				&TypeVoid,		1,	&TypeBool,"condition"},
 	{"for",					CommandFor,					NULL,	f_cp,				&TypeVoid,		1,	&TypeVoid,"expression"},
	{"wait",				CommandWait,				NULL,	f_cp,				&TypeVoid,		1,	&TypeFloat,"time"},
	{"wait_rt",				CommandWaitRT,				NULL,	f_cp,				&TypeVoid,		1,	&TypeFloat,"time"},
	{"wait_of",				CommandWaitOneFrame,		NULL,	f_cp,				&TypeVoid,		0},
	{"int",					CommandIntFromFloat,		NULL,	(char*)&_Float2Int,	&TypeInt,		1,	&TypeFloat,"f"},
	{"float",				CommandFloatFromInt,		NULL,	f_cp,				&TypeFloat,		1,	&TypeInt,"i"},
	{"c2i",					CommandIntFromChar,			NULL,	f_cp,				&TypeInt,		1,	&TypeChar,"c"},
	{"i2c",					CommandCharFromInt,			NULL,	f_cp,				&TypeChar,		1,	&TypeInt,"i"},
	{"vector",				CommandVectorSet,			NULL,	f_cp,				&TypeVector,	3,	&TypeFloat,"x",
																										&TypeFloat,"y",
																										&TypeFloat,"z"},
	{"rect",				CommandRectSet,				NULL,	f_cp,				&TypeRect,		4,	&TypeFloat,"x1",
																										&TypeFloat,"x2",
																										&TypeFloat,"y1",
																										&TypeFloat,"y2"},
	{"color",				CommandColorSet,			NULL,	f_cp,				&TypeColor,		4,	&TypeFloat,"a",
																										&TypeFloat,"r",
																										&TypeFloat,"g",
																										&TypeFloat,"b"},
	{"-asm-",				CommandAsm,					NULL,	f_cp,				&TypeVoid,		0},

	// mathematisch
	{"sin",					CommandFloatSinus,			NULL,	(char*)&f_sin,			&TypeFloat,		1,	&TypeFloat,"x"},
	{"cos",					CommandFloatCosinus,		NULL,	(char*)&f_cos,			&TypeFloat,		1,	&TypeFloat,"x"},
	{"tan",					CommandFloatTangens,		NULL,	(char*)&f_tan,			&TypeFloat,		1,	&TypeFloat,"x"},
	{"asin",				CommandFloatArcSinus,		NULL,	(char*)&f_asin,			&TypeFloat,		1,	&TypeFloat,"x"},
	{"acos",				CommandFloatArcCosinus,		NULL,	(char*)&f_acos,			&TypeFloat,		1,	&TypeFloat,"x"},
	{"atan",				CommandFloatArcTangens,		NULL,	(char*)&f_atan,			&TypeFloat,		1,	&TypeFloat,"x"},
	{"atan2",				CommandFloatArcTangens2,	NULL,	(char*)&f_atan2,		&TypeFloat,		2,	&TypeFloat,"x",
																											&TypeFloat,"y"},
	{"sqrt",				CommandFloatSquareRoot,		NULL,	(char*)&f_sqrt,			&TypeFloat,		1,	&TypeFloat,"x"},
	{"sqr",					CommandFloatSquare,			NULL,	(char*)&f_sqr,			&TypeFloat,		1,	&TypeFloat,"x"},
	{"pow",					CommandFloatPower,			NULL,	(char*)&f_pow,			&TypeFloat,		2,	&TypeFloat,"x",
																											&TypeFloat,"exp"},
	// Typ-Umwandlungen
	{"s2i",					CommandIntFromString,		NULL,	(char*)&s2i,			&TypeInt,		1,	&TypeStringP,"s"},
	{"s2f",					CommandFloatFromString,		NULL,	(char*)&s2f,			&TypeFloat,		1,	&TypeStringP,"s"},
	{"i2s",					CommandStringFromInt,		NULL,	(char*)&i2s,			&TypeStringP,	1,	&TypeInt,"i"},
	{"f2s",					CommandStringFromFloat,		NULL,	(char*)&f2s,			&TypeStringP,	2,	&TypeFloat,"f",
																										&TypeInt,"decimals"},
	// Zufalls-Zahlen
 	{"randi",				CommandRandInt,				NULL,	(char*)&randi,			&TypeInt,	1	,&TypeInt,"max"},
 	{"randf",				CommandRandFloat,			NULL,	(char*)&randf,			&TypeFloat,	1	,&TypeFloat,"max"},
	// Debug-Funktionen
	{"intout",				CommandIntOut,				NULL,	(char*)&intout,			&TypeVoid,		1,	&TypeInt,"i"},
	{"boolout",				CommandBoolOut,				NULL,	(char*)&boolout,		&TypeVoid,		1,	&TypeBool,"b"},
	{"floatout",			CommandFloatOut,			NULL,	(char*)&floatout,		&TypeVoid,		1,	&TypeFloat,"f"},
	{"stringout",			CommandStringOut,			NULL,	(char*)&_stringout,		&TypeVoid,		1,	&TypeStringP,"str"},
	{"pointerout",			CommandPointerOut,			NULL,	(char*)&pointerout,		&TypeVoid,		1,	&TypePointer,"p"},
	// Vektoren
	{"VecNormalize",		CommandVectorNormalize,		NULL,	(char*)&VecNormalize,&TypeVoid,		2,	&TypeVectorP,"v_out",
																										&TypeVectorP,"v_in"},
	{"VecDir2Ang",			CommandVectorDir2Ang,		NULL,	(char*)&VecDir2Ang,	&TypeVector,	1,	&TypeVectorP,"dir"},
	{"VecDir2Ang2",			CommandVectorDir2Ang2,		NULL,	(char*)&VecDir2Ang2,&TypeVector,	2,	&TypeVectorP,"dir",
																										&TypeVectorP,"up"},
	{"VecAng2Dir",			CommandVectorAng2Dir,		NULL,	(char*)&VecAng2Dir,	&TypeVector,	1,	&TypeVectorP,"ang"},
	{"VecAngAdd",			CommandVectorAngAdd,		NULL,	(char*)&VecAngAdd,	&TypeVector,	2,	&TypeVectorP,"ang1",
																										&TypeVectorP,"ang2"},
	{"VecLength",			CommandVectorLength,		NULL,	(char*)&VecLength,	&TypeFloat,		1,	&TypeVectorP,"v"},
	{"VecLengthSqr",		CommandVectorLengthSqr,		NULL,	(char*)&VecLengthSqr,&TypeFloat,	1,	&TypeVectorP,"v"},
	{"VecTransform",		CommandVectorTransform,		NULL,	(char*)&VecTransform,&TypeVoid,		3,	&TypeVectorP,"v_out",
																										&TypeMatrixP,"mat",
																										&TypeVectorP,"v_in"},
	{"VecNormalTransform",	CommandVectorNormalTransform,NULL,	(char*)&VecNormalTransform,	&TypeVoid,	3,	&TypeVectorP,"v_out",
																											&TypeMatrixP,"mat",
																											&TypeVectorP,"v_in"},
	{"VecDotProduct",		CommandVectorDotProduct,	NULL,	(char*)&VecDotProduct,		&TypeFloat,	2,	&TypeVectorP,"v1",
																											&TypeVectorP,"v2"},
	{"VecCrossProduct",		CommandVectorCrossProduct,	NULL,	(char*)&VecCrossProduct,	&TypeVector,2,	&TypeVectorP,"v1",
																											&TypeVectorP,"v2"},
	// Matrizen
	{"MatrixIdentity",		CommandMatrixIdentity,		NULL,	(char*)&MatrixIdentity,		&TypeVoid,	1,	&TypeMatrixP,"m"},
	{"MatrixTranslation",	CommandMatrixTranslation,	NULL,	(char*)&MatrixTranslation,	&TypeVoid,	2,	&TypeMatrixP,"m",
																											&TypeVectorP,"trans"},
	{"MatrixRotation",		CommandMatrixRotation,		NULL,	(char*)&MatrixRotation,		&TypeVoid,	2,	&TypeMatrixP,"m",
																											&TypeVectorP,"ang"},
	{"MatrixRotationX",		CommandMatrixRotationX,		NULL,	(char*)&MatrixRotationX,	&TypeVoid,	2,	&TypeMatrixP,"m",
																											&TypeFloat,"ang"},
	{"MatrixRotationY",		CommandMatrixRotationY,		NULL,	(char*)&MatrixRotationY,	&TypeVoid,	2,	&TypeMatrixP,"m",
																											&TypeFloat,"ang"},
	{"MatrixRotationZ",		CommandMatrixRotationZ,		NULL,	(char*)&MatrixRotationZ,	&TypeVoid,	2,	&TypeMatrixP,"m",
																											&TypeFloat,"ang"},
	{"MatrixRotationQ",		CommandMatrixRotationQ,		NULL,	(char*)&MatrixRotationQ,	&TypeVoid,	2,	&TypeMatrixP,"m",
																											&TypeQuaternionP,"ang"},
	{"MatrixRotationView",	CommandMatrixRotationView,	NULL,	(char*)&MatrixRotationView,	&TypeVoid,	2,	&TypeMatrixP,"m",
																											&TypeVectorP,"ang"},
	{"MatrixScale",			CommandMatrixScale,			NULL,	(char*)&MatrixScale,		&TypeVoid,	4,	&TypeMatrixP,"m",
																											&TypeFloat,"s_x",
																											&TypeFloat,"s_y",
																											&TypeFloat,"s_z"},
	{"MatrixMultiply",		CommandMatrixMultiply,		NULL,	(char*)&MatrixMultiply,		&TypeVoid,	3,	&TypeMatrixP,"m",
																											&TypeMatrixP,"mat2",
																											&TypeMatrixP,"mat1"},
	{"MatrixInverse",		CommandMatrixInverse,		NULL,	(char*)&MatrixInverse,		&TypeVoid,	2,	&TypeMatrixP,"m_out",
																											&TypeMatrixP,"m_in"},
	// Quaternione
	{"QuaternionRotationV",	CommandQuaternionRotationV,	NULL,	(char*)&QuaternionRotationV,&TypeVoid,	2,	&TypeQuaternionP,"q_out",
																											&TypeVectorP,"v"},
	{"QuaternionRotationA",	CommandQuaternionRotationA,	NULL,	(char*)&QuaternionRotationA,&TypeVoid,	3,	&TypeQuaternionP,"q_out",
																											&TypeVectorP,"axis",
																											&TypeFloat,"angle"},
	{"QuaternionMultiply",	CommandQuaternionMultiply,	NULL,	(char*)&QuaternionMultiply,	&TypeVoid,	3,	&TypeQuaternionP,"q_out",
																											&TypeQuaternionP,"q2",
																											&TypeQuaternionP,"q1"},
	{"QuaternionToAngle",	CommandQuaternionToAngle,	NULL,	(char*)&QuaternionToAngle,	&TypeVector,1,	&TypeQuaternionP,"q"},
	// andere Typen
	{"SetColorHSB",			CommandSetColorHSB,			NULL,	(char*)&SetColorHSB,	&TypeColor,		4,	&TypeFloat,"a",
																											&TypeFloat,"h",
																											&TypeFloat,"s",
																											&TypeFloat,"b"},
	{"LoadTexture",			CommandTextureLoad,			NULL,	NULL,					&TypeInt,		1,	&TypeStringP,"filename"},
	{"XFDrawStr",			CommandXFDrawStr,			NULL,	NULL,					&TypeVoid,		5,	&TypeFloat,"x",
																											&TypeFloat,"y",
																											&TypeFloat,"size",
																											&TypeStringP,"str",
																											&TypeBool,"centric"},
	{"XFDrawVertStr",		CommandXFDrawVertStr,		NULL,	NULL,					&TypeVoid,		4,	&TypeFloat,"x",
																											&TypeFloat,"y",
																											&TypeFloat,"size",
																											&TypeStringP,"str"},
	{"XFGetWidth",			CommandXFGetWidth,			NULL,	NULL,					&TypeFloat,		2,	&TypeFloat,"size",
																											&TypeStringP,"str"},
	{"LoadXFont",			CommandLoadXFont,			NULL,	NULL,					&TypeInt,		1,	&TypeStringP,"filename"},
	{"CreatePicture",		CommandPictureCreate,		NULL,	NULL,					&TypePictureP,	6,	&TypeVector,"pos",
																											&TypeFloat,"w",
																											&TypeFloat,"h",
																											&TypeInt,"tex",
																											&TypeRectP,"src",
																											&TypeColorP,"col"},
	{"CreatePicture3D",		CommandPicture3DCreate,		NULL,	NULL,					&TypePicture3DP,3,	&TypeModelP,"m",
																											&TypeMatrixP,"mat",
																											&TypeFloat,"z"},
	{"CreateText",			CommandTextCreate,			NULL,	NULL,					&TypeTextP,		4,	&TypeVector,"pos",
																											&TypeFloat,"size",
																											&TypeColorP,"col",
																											&TypeStringP,"str"},
	{"CreateGrouping",		CommandGroupingCreate,		NULL,	NULL,					&TypeGroupingP,3,	&TypeVector,"pos",
																											&TypeColorP,"col",
																											&TypeBool,"set_cur"},
	{"CreateParticle",		CommandParticleCreate,		NULL,	NULL,					&TypeParticleP,	5,	&TypeVectorP,"pos",
																											&TypeInt,"tex",
																											&TypePointer,"func",
																											&TypeFloat,"life",
																											&TypeFloat,"radius"},
	{"CreateParticleRot",	CommandParticleRotCreate,	NULL,	NULL,					&TypeParticleP,	6,	&TypeVectorP,"pos",
																											&TypeVectorP,"ang",
																											&TypeInt,"tex",
																											&TypePointer,"func",
																											&TypeFloat,"life",
																											&TypeFloat,"radius"},
	{"CreateBeam",			CommandParticleBeamCreate,	NULL,	NULL,					&TypeBeamP,		6,	&TypeVectorP,"pos",
																											&TypeVectorP,"length",
																											&TypeInt,"tex",
																											&TypePointer,"func",
																											&TypeFloat,"life",
																											&TypeFloat,"radius"},
	{"LoadModel",			CommandModelLoad,			NULL,	NULL,					&TypeModelP,	1,	&TypeStringP,"filename"},
	{"LoadItem",			CommandItemLoad,			NULL,	NULL,					&TypeItemP,		1,	&TypeStringP,"filename"},
	{"GetItemOID",			CommandItemGetOID,			NULL,	NULL,					&TypeInt,		1,	&TypeStringP,"filename"},
	{"CreateView",			CommandViewCreate,			NULL,	NULL,					&TypeViewP,		4,	&TypeVector,"pos",
																											&TypeVector,"ang",
																											&TypeRectP,"dst",
																											&TypeBool,"show"},
	{"LoadShaderFile",		CommandShaderFileLoad,		NULL,	NULL,					&TypeInt,		1,	&TypeStringP,"filename"},
#ifdef ALLOW_SCRIPT_HUI
	{"HuiSetIdleFunction",	CommandHuiSetIdleFunction,	NULL,	(char*)HuiSetIdleFunction,&TypeVoid,	1,	&TypePointer,"if"},
	{"HuiRun",				CommandHuiRun,				i_hui,	NULL,					&TypeVoid,		0},
	{"HuiEnd",				CommandHuiEnd,				i_hui,	NULL,					&TypeVoid,		0},
	{"HuiWaitTillWindowClosed",CommandHuiWaitTillWindowClosed,i_hui,NULL,				&TypeVoid,		1,	&TypePointer,"win"},
	{"HuiSleep",			CommandHuiSleep,			i_hui,	NULL,					&TypeVoid,		1,	&TypeInt,"ms"},
	{"HuiFileDialogOpen",	CommandHuiFileDialogOpen,	i_hui,	NULL,					&TypeBool,		5,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeStringP,"dir",
																											&TypeStringP,"s_filter",
																											&TypeStringP,"filter"},
	{"HuiFileDialogSave",	CommandHuiFileDialogSave,	i_hui,	NULL,					&TypeBool,		5,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeStringP,"dir",
																											&TypeStringP,"s_filter",
																											&TypeStringP,"filter"},
	{"HuiFileDialogDir",	CommandHuiFileDialogDir,	i_hui,	NULL,					&TypeBool,		3,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeStringP,"dir"},
	{"HuiQuestionBox",		CommandHuiQuestionBox,		i_hui,	NULL,					&TypeInt,		4,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeStringP,"text",
																											&TypeBool,"cancel"},
	{"HuiInfoBox",			CommandHuiInfoBox,			i_hui,	NULL,					&TypeVoid,		3,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeStringP,"text"},
	{"HuiErrorBox",			CommandHuiErrorBox,			i_hui,	NULL,					&TypeVoid,		3,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeStringP,"text"},
	{"HuiConfigWriteInt",	CommandHuiConfigWriteInt,	i_hui,	NULL,					&TypeVoid,		2,	&TypeStringP,"name",
																											&TypeInt,"val"},
	{"HuiConfigWriteStr",	CommandHuiConfigWriteStr,	i_hui,	NULL,					&TypeVoid,		2,	&TypeStringP,"name",
																											&TypeStringP,"val"},
	{"HuiConfigReadInt",	CommandHuiConfigReadInt,	i_hui,	NULL,					&TypeVoid,		3,	&TypeStringP,"name",
																											&TypeIntP,"val",
																											&TypeInt,"def"},
	{"HuiConfigReadStr",	CommandHuiConfigReadStr,	i_hui,	NULL,					&TypeVoid,		3,	&TypeStringP,"name",
																											&TypeStringP,"val",
																											&TypeStringP,"def"},

	// clipboard
	{"HuiCopyToClipBoard",	CommandHuiCopyToClipBoard,	i_hui,	NULL,					&TypeVoid,		2,	&TypeStringP,"buf",
																											&TypeInt,"len"},
//	{"HuiPasteFromClipBoard",CommandHuiPasteFromClipBoard,i_hui,NULL,					&TypeVoid,		2,	&TypeStringPP,"buf",
//																											&TypeIntP,"len"},
	{"HuiOpenDocument",		CommandHuiOpenDocument,		i_hui,	NULL,					&TypeVoid,		1,	&TypeStringP,"file"},
	{"CreateTimer",			CommandHuiCreateTimer,		i_hui,	NULL,					&TypeInt,		0},
	{"GetTime",				CommandHuiGetTime,			i_hui,	NULL,					&TypeFloat,		1,	&TypeInt,"index"},
	// Menue
	{"HuiCreateMenu",		CommandHuiCreateMenu,		NULL,	(char*)&HuiCreateMenu,	&TypePointer,	0},
	{"HuiMenuOpenPopup",	CommandHuiMenuOpenPopup,	NULL,	NULL,					&TypeVoid,		4,	&TypePointer,"m",
																											&TypePointer,"win",
																											&TypeInt,"x",
																											&TypeInt,"y"},
	{"HuiMenuAddEntry",		CommandHuiMenuAddEntry,		NULL,	NULL,					&TypeVoid,		3,	&TypePointer,"m",
																											&TypeStringP,"name",
																											&TypeInt,"id"},
	{"HuiMenuAddSeparator",	CommandHuiMenuAddSeparator,	NULL,	NULL,					&TypeVoid,		1,	&TypePointer,"m"},
	{"HuiMenuAddSubMenu",	CommandHuiMenuAddSubMenu,	NULL,	NULL,					&TypeVoid,		4,	&TypePointer,"m",
																											&TypeStringP,"name",
																											&TypeInt,"id",
																											&TypePointer,"sm"},
	{"HuiMenuCheckItem",	CommandHuiMenuCheckItem,	NULL,	NULL,					&TypeVoid,		3,	&TypePointer,"m",
																											&TypeInt,"id",
																											&TypeBool,"checked"},
	{"HuiMenuIsItemChecked",CommandHuiMenuIsItemChecked,NULL,	NULL,					&TypeBool,		2,	&TypePointer,"m",
																											&TypeInt,"id"},
	{"HuiMenuEnableItem",	CommandHuiMenuEnableItem,	NULL,	NULL,					&TypeVoid,		3,	&TypePointer,"m",
																											&TypeInt,"id",
																											&TypeBool,"enabled"},
	{"HuiMenuSetText",		CommandHuiMenuSetText,		NULL,	NULL,					&TypeVoid,		3,	&TypePointer,"m",
																											&TypeInt,"id",
																											&TypeStringP,"text"},
	// Fenster
	{"HuiCreateWindow",		CommandHuiCreateWindow,		NULL,	NULL,					&TypePointer,	6,	&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypePointer,"mf"},
	{"HuiCreateNixWindow",	CommandHuiCreateNixWindow,	NULL,	NULL,					&TypePointer,	6,	&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypePointer,"mf"},
	{"HuiCreateDialog",		CommandHuiCreateDialog,		NULL,	NULL,					&TypePointer,	6,	&TypeStringP,"title",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypePointer,"root",
																											&TypeBool,"allow_root",
																											&TypePointer,"mf"},
	{"HuiWinUpdate",	CommandHuiWinUpdate,			NULL,	NULL,					&TypeVoid,		1,	&TypePointer,"win"},
	{"HuiWinHide",		CommandHuiWinHide,				NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeBool,"hide"},
	{"HuiWinSetMaximized",CommandHuiWinSetMaximized,	NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeBool,"max"},
	{"HuiWinIsMaximized",CommandHuiWinIsMaximized,		NULL,	NULL,					&TypeBool,		1,	&TypePointer,"win"},
	{"HuiWinIsMinimized",CommandHuiWinIsMinimized,		NULL,	NULL,					&TypeBool,		1,	&TypePointer,"win"},
	{"HuiWinSetID",		CommandHuiWinSetID,				NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeInt,"id"},

	{"HuiWinSetFullscreen",	CommandHuiWinSetFullscreen,	NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeBool,"fs"},
	{"HuiWinSetTitle",		CommandHuiWinSetTitle,		NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeStringP,"title"},
	{"HuiWinSetPosition",	CommandHuiWinSetPosition,	NULL,	NULL,					&TypeVoid,		3,	&TypePointer,"win",
																											&TypeInt,"x",
																											&TypeInt,"y"},
	/*{"HuiWinSetOuterior",	CommandHuiWinSetOuterior,	NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeIRect,"r"},
	{"HuiWinGetOuterior",	CommandHuiWinGetOuterior,	NULL,	NULL,					&TypeIRect,		1,	&TypePointer,"win"},
	{"HuiWinSetInerior",	CommandHuiWinSetInerior,	NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeIRect,"r"},
	{"HuiWinGetInterior",	CommandHuiGetInterior,		NULL,	NULL,					&TypeIRect,		1,	&TypePointer,"win"},*/
	{"HuiWinSetCursorPos",	CommandHuiWinSetCursorPos,	NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeInt,"id"},

	{"HuiWinActivate",		CommandHuiWinActivate,		NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeInt,"id"},
	{"HuiWinIsActive",		CommandHuiWinIsActive,		NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeBool,"recursive"},
	{"HuiWinAddButton",		CommandHuiWinAddButton,		NULL,	NULL,					&TypeVoid,		7,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypeInt,"id"},
	{"HuiWinAddCheckBox",	CommandHuiWinAddCheckBox,	NULL,	NULL,					&TypeVoid,		7,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypeInt,"id"},
	{"HuiWinAddText",		CommandHuiWinAddText,		NULL,	NULL,					&TypeVoid,		7,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypeInt,"id"},
	{"HuiWinAddEdit",		CommandHuiWinAddEdit,		NULL,	NULL,					&TypeVoid,		7,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypeInt,"id"},
	{"HuiWinAddGroup",		CommandHuiWinAddGroup,		NULL,	NULL,					&TypeVoid,		7,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypeInt,"id"},
	{"HuiWinAddComboBox",	CommandHuiWinAddComboBox,	NULL,	NULL,					&TypeVoid,		7,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypeInt,"id"},
	{"HuiWinAddTabControl",	CommandHuiWinAddTabControl,	NULL,	NULL,					&TypeVoid,		7,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypeInt,"id"},
	{"HuiWinSetTabCreationPage",CommandHuiWinSetTabCreationPage,NULL,NULL,				&TypeVoid,		3,	&TypePointer,"win",
																											&TypeInt,"id",
																											&TypeInt,"page"},
	{"HuiWinAddListView",	CommandHuiWinAddListView,	NULL,	NULL,					&TypeVoid,		7,	&TypePointer,"win",
																											&TypeStringP,"title",
																											&TypeInt,"x",
																											&TypeInt,"y",
																											&TypeInt,"w",
																											&TypeInt,"h",
																											&TypeInt,"id"},
	{"HuiWinSetControlText",CommandHuiWinSetControlText,NULL,	NULL,					&TypeVoid,		3,	&TypePointer,"win",
																											&TypeInt,"id",
																											&TypeStringP,"str"},
	{"HuiWinGetControlText",CommandHuiWinGetControlText,NULL,	NULL,					&TypeStringP,	2,	&TypePointer,"win",
																											&TypeInt,"id"},
	{"HuiWinEnableControl",	CommandHuiWinEnableControl,	NULL,	NULL,					&TypeVoid,		3,	&TypePointer,"win",
																											&TypeInt,"id",
																											&TypeBool,"enabled"},
	{"HuiWinIsControlEnabled",CommandHuiWinIsControlEnabled,NULL,NULL,					&TypeBool,		2,	&TypePointer,"win",
																											&TypeInt,"id"},
	{"HuiWinCheckControl",	CommandHuiWinCheckControl,	NULL,	NULL,					&TypeVoid,		3,	&TypePointer,"win",
																											&TypeInt,"id",
																											&TypeBool,"checked"},
	{"HuiWinIsControlChecked",CommandHuiWinIsControlChecked,NULL,NULL,					&TypeBool,		2,	&TypePointer,"win",
																											&TypeInt,"id"},
	{"HuiWinGetControlSelection",CommandHuiWinGetControlSelection,NULL,NULL,			&TypeInt,		2,	&TypePointer,"win",
																											&TypeInt,"id"},
	{"HuiWinGetControlSelectionM",CommandHuiWinGetControlSelectionM,NULL,NULL,			&TypeInt,		3,	&TypePointer,"win",
																											&TypeInt,"id",
																											&TypeIntP,"indices"},
	{"HuiWinSetControlSelection",CommandHuiWinSetControlSelection,NULL,NULL,			&TypeVoid,		3,	&TypePointer,"win",
																											&TypeInt,"id",
																											&TypeInt,"index"},
	{"HuiWinResetControl",	CommandHuiWinResetControl,	NULL,	NULL,					&TypeVoid,		2,	&TypePointer,"win",
																											&TypeInt,"id"},
	{"HuiWinSetMenu",		CommandHuiWinSetMenu,		NULL,	(char*)&HuiWindowSetMenu,&TypeVoid,		2,	&TypePointer,"win",
																											&TypePointer,"menu"},
	{"NixInit",				CommandNixInit,				NULL,	(char*)&NixInit,		&TypeVoid,		6,	&TypeInt,"api",
																											&TypeInt,"xres",
																											&TypeInt,"yres",
																											&TypeInt,"depth",
																											&TypeBool,"fullscreen",
																											&TypePointer,"win"},
#endif
	// Engine
		// Input
	{"NixUpdateInput",		CommandNixUpdateInput,		NULL,	NULL,		&TypeVoid,	0},
	{"GetKey",				CommandGetKey,				NULL,	NULL,		&TypeBool,	1,	&TypeInt,"id"},
	{"GetKeyDown",			CommandGetKeyDown,			NULL,	NULL,		&TypeBool,	1,	&TypeInt,"id"},
	{"GetKeyRhythmDown",	CommandGetKeyRhythmDown,	NULL,	NULL,		&TypeInt,	0},
	{"GetKeyChar",			CommandGetKeyChar,			NULL,	NULL,		&TypeChar,	1,	&TypeInt,"id"},
	{"GetKeyUp",			CommandGetKeyUp,			NULL,	NULL,		&TypeBool,	1,	&TypeInt,"id"},
	{"GetKeyName",			CommandGetKeyName,			i_hui,	NULL,		&TypeStringP,1,	&TypeInt,"id"},
	{"GetMouseX",			CommandGetMouseX,			NULL,	NULL,		&TypeInt,	0},
	{"GetMouseY",			CommandGetMouseY,			NULL,	NULL,		&TypeInt,	0},
	{"GetMouseDx",			CommandGetMouseDx,			NULL,	NULL,		&TypeInt,	0},
	{"GetMouseDy",			CommandGetMouseDy,			NULL,	NULL,		&TypeInt,	0},
	{"GetWheelD",			CommandGetWheelD,			NULL,	NULL,		&TypeInt,	0},
	{"GetButL",				CommandGetButL,				NULL,	NULL,		&TypeBool,	0},
	{"GetButM",				CommandGetButM,				NULL,	NULL,		&TypeBool,	0},
	{"GetButR",				CommandGetButR,				NULL,	NULL,		&TypeBool,	0},
	{"GetButLDown",			CommandGetButLDown,			NULL,	NULL,		&TypeBool,	0},
	{"GetButMDown",			CommandGetButMDown,			NULL,	NULL,		&TypeBool,	0},
	{"GetButRDown",			CommandGetButRDown,			NULL,	NULL,		&TypeBool,	0},
	{"GetButLUp",			CommandGetButLUp,			NULL,	NULL,		&TypeBool,	0},
	{"GetButMUp",			CommandGetButMUp,			NULL,	NULL,		&TypeBool,	0},
	{"GetButRUp",			CommandGetButRUp,			NULL,	NULL,		&TypeBool,	0},
		// Darstellung
	{"NixStart",			CommandNixStart,			NULL,	NULL,		&TypeVoid,	1,	&TypeInt,"texture"},
	{"NixEnd",				CommandNixEnd,				NULL,	NULL,		&TypeVoid,	0},
	{"NixKillWindows",		CommandNixKillWindows,		NULL,	NULL,		&TypeVoid,	0},
	{"NixDraw2D",			CommandNixDraw2D,			NULL,	NULL,		&TypeVoid,	5,	&TypeInt,"texture",
																							&TypeColorP,"col",
																							&TypeRectP,"source",
																							&TypeRectP,"dest",
																							&TypeFloat,"depth"},
	{"NixDraw3D",			CommandNixDraw3D,			NULL,	NULL,		&TypeVoid,	3,	&TypeInt,"texture",
																							&TypeInt,"vb",
																							&TypeMatrixP,"mat"},
	{"NixDrawStr",			CommandNixDrawStr,			NULL,	NULL,		&TypeVoid,	3,	&TypeInt,"x",
																							&TypeInt,"y",
																							&TypeStringP,"str"},
	{"NixDrawLineH",		CommandNixDrawLineH,		NULL,	NULL,		&TypeVoid,	5,	&TypeInt,"x",
																							&TypeInt,"y1",
																							&TypeInt,"y2",
																							&TypeColor,"col",
																							&TypeFloat,"depth"},
	{"NixDrawLineV",		CommandNixDrawLineV,		NULL,	NULL,		&TypeVoid,	5,	&TypeInt,"x1",
																							&TypeInt,"x2",
																							&TypeInt,"y",
																							&TypeColor,"col",
																							&TypeFloat,"depth"},
	{"NixDrawLine",			CommandNixDrawLine,			NULL,	NULL,		&TypeVoid,	6,	&TypeFloat,"x1",
																							&TypeFloat,"y1",
																							&TypeFloat,"x2",
																							&TypeFloat,"y2",
																							&TypeColor,"col",
																							&TypeFloat,"depth"},
	{"NixDrawSprite",		CommandNixDrawSprite,		NULL,	NULL,		&TypeVoid,	5,	&TypeInt,"texture",
																							&TypeColorP,"col",
																							&TypeRectP,"src",
																							&TypeVectorP,"pos",
																							&TypeFloat,"radius"},
	//{"NixDrawModel2D",	CommandNixDrawModel2D,		NULL,	NULL,		&TypeVoid,	1,	&TypeFloat,"???"}, // ???
	{"NixSetAlphaM",		CommandNixSetAlpha,			NULL,	NULL,		&TypeVoid,	1,	&TypeInt,"type"},
	{"NixSetAlphaSD",		CommandNixSetAlpha2,		NULL,	NULL,		&TypeVoid,	2,	&TypeInt,"source",
																							&TypeInt,"dest"},
	{"NixSetStencil",		CommandNixSetStencil,		NULL,	NULL,		&TypeVoid,	2,	&TypeInt,"type",
																							&TypeInt,"param"},
	{"NixSetViewM",			CommandNixSetView,			NULL,	NULL,		&TypeVoid,	2,	&TypeBool,"enable3d",
																							&TypeMatrixP,"view_mat"},
	{"NixSetViewV",			CommandNixSetView2,			NULL,	NULL,		&TypeVoid,	3,	&TypeBool,"enable3d",
																							&TypeVectorP,"view_pos",
																							&TypeVectorP,"view_ang"},
	{"NixSetZ",				CommandNixSetZ,				NULL,	NULL,		&TypeVoid,	2,	&TypeBool,"write",
																							&TypeBool,"test"},
	{"NixSetMaterial",		CommandNixSetMaterial,		NULL,	NULL,		&TypeVoid,	5,	&TypeColorP,"ambient",
																							&TypeColorP,"diffuse",
																							&TypeColorP,"specular",
																							&TypeFloat,"shininess",
																							&TypeColorP,"emission"},
	{"NixSetFontColor",		CommandNixSetFontColor,		NULL,	NULL,		&TypeVoid,	1,	&TypeColor,"col"},
	{"NixEnableLighting",	CommandNixEnableLighting,	NULL,	NULL,		&TypeVoid,	1,	&TypeBool,"enable"},
	{"NixSetVideoMode",		CommandNixSetVideoMode,		NULL,	NULL,		&TypeVoid,	5,	&TypeInt,"api",
																							&TypeInt,"width",
																							&TypeInt,"height",
																							&TypeInt,"depth",
																							&TypeBool,"fullscreen"},
	{"NixCreateDynamicTexture",CommandNixCreateDynamicTexture,NULL,NULL,	&TypeInt,	2,	&TypeInt,"width",
																							&TypeInt,"height"},
	{"GetVecProject",		CommandGetVecProject,		NULL,	NULL,		&TypeVoid,	2,	&TypeVectorP,"v_out",
																							&TypeVectorP,"v_in"},
	{"GetVecUnproject",		CommandGetVecUnproject,		NULL,	NULL,		&TypeVoid,	2,	&TypeVectorP,"v_out",
																							&TypeVectorP,"v_in"},
	{"GetVecProjectRel",	CommandGetVecProjectRel,	NULL,	NULL,		&TypeVoid,	2,	&TypeVectorP,"v_out",
																							&TypeVectorP,"v_in"},
	{"GetVecUnprojectRel",	CommandGetVecUnprojectRel,	NULL,	NULL,		&TypeVoid,	2,	&TypeVectorP,"v_out",
																							&TypeVectorP,"v_in"},
	{"NixVBEmpty",			CommandNixVBEmpty,			NULL,	NULL,		&TypeVoid,	1,	&TypeInt,"vb"},
	{"NixVBAddTrias",		CommandNixVBAddTrias,		NULL,	NULL,		&TypeVoid,	5,	&TypeInt,"vb",
																							&TypeInt,"num_trias",
																							&TypeVectorP,"p",
																							&TypeVectorP,"n",
																							&TypeFloatP,"t"},
	// sound
	{"SoundEmit",			CommandSoundEmit,			NULL,	NULL,		&TypeVoid,	4,	&TypeStringP,"filename",
																							&TypeVectorP,"pos",
																							&TypeFloat,"radius",
																							&TypeFloat,"speed"},
	{"SoundCreate",			CommandSoundCreate,			NULL,	NULL,		&TypeInt,	1,	&TypeStringP,"filename"},
	{"SoundSetData",		CommandSoundSetData,		NULL,	NULL,		&TypeVoid,	6,	&TypeInt,"index",
																							&TypeVectorP,"pos",
																							&TypeVectorP,"vel",
																							&TypeFloat,"radius",
																							&TypeFloat,"speed",
																							&TypeFloat,"volume"},
	{"SoundDelete",			CommandSoundDelete,			NULL,	NULL,		&TypeVoid,	1,	&TypeInt,"index"},
	// music
	{"MusicLoad",			CommandMusicLoad,			NULL,	NULL,		&TypeInt,	1,	&TypeStringP,"filename"},
	{"MusicPlay",			CommandMusicPlay,			NULL,	NULL,		&TypeInt,	2,	&TypeInt,"index",
																							&TypeBool,"repeat"},
	{"MusicStop",			CommandMusicStop,			NULL,	NULL,		&TypeInt,	1,	&TypeInt,"index"},
	{"MusicPause",			CommandMusicPause,			NULL,	NULL,		&TypeInt,	2,	&TypeInt,"index",
																							&TypeBool,"pause"},
	{"MusicSetRate",		CommandMusicSetRate,		NULL,	NULL,		&TypeInt,	2,	&TypeInt,"index",
																							&TypeFloat,"rate"},
	// effects
	{"FXLightCreate",		CommandFXLightCreate,		NULL,	NULL,		&TypeInt,	0,	},
	{"FXLightSetDirectional",CommandFXLightSetDirectional,NULL,	NULL,		&TypeVoid,	5,	&TypeInt,"index",
																							&TypeVectorP,"dir",
																							&TypeColorP,"am",
																							&TypeColorP,"di",
																							&TypeColorP,"sp",},
	{"FXLightSetRadial",	CommandFXLightSetRadial,	NULL,	NULL,		&TypeVoid,	6,	&TypeInt,"index",
																							&TypeVectorP,"pos",
																							&TypeFloat,"radius",
																							&TypeColorP,"am",
																							&TypeColorP,"di",
																							&TypeColorP,"sp",},
	{"FXLightDelete",		CommandFXLightDelete,		NULL,	NULL,		&TypeVoid,	1,	&TypeInt,"index"},
	{"FXLightEnable",		CommandFXLightEnable,		NULL,	NULL,		&TypeVoid,	2,	&TypeInt,"index",
																							&TypeBool,"enabled"},
	// game
	{"ExitProgram",			CommandExitProgram,			NULL,	NULL,		&TypeVoid,	0},
	{"ScreenShot",			CommandScreenShot,			NULL,	NULL,		&TypeVoid,	0},
	{"FindHosts",			CommandFindHosts,			NULL,	NULL,		&TypeVoid,	0},
	{"XDelete",				CommandXDelete,				NULL,	NULL,		&TypeVoid,	1,	&TypePointer,"p"},
	{"LoadWorld",			CommandLoadWorld,			NULL,	NULL,		&TypeVoid,	1,	&TypeStringP,"filename"},
	{"LoadGameFromHost",	CommandLoadGameFromHost,	NULL,	NULL,		&TypeVoid,	1,	&TypeInt,"host"},
	{"SaveGameState",		CommandSaveGameState,		NULL,	NULL,		&TypeVoid,	1,	&TypeStringP,"filename"},
	{"LoadGameState",		CommandLoadGameState,		NULL,	NULL,		&TypeVoid,	1,	&TypeStringP,"filename"},
	{"GetObjectByName",		CommandGetObjectByName,		NULL,	NULL,		&TypeObjectP,1,	&TypeStringP,"name"},
	{"CreateObject",		CommandCreateObject,		NULL,	NULL,		&TypeObjectP,2,	&TypeStringP,"filename",
																							&TypeVector,"pos"},
	{"DeleteObject",		CommandDeleteObject,		NULL,	NULL,		&TypeVoid,	1,	&TypeInt,"id"},
	{"CamStartScript",		CommandCamStartScript,		NULL,	NULL,		&TypeVoid,	3,	&TypeStringP,"filename",
																							&TypeViewP,"view",
																							&TypeVectorP,"dpos"},
	{"CamStopScript",		CommandCamStopScript,		NULL,	NULL,		&TypeVoid,	1,	&TypeViewP,"view"},
	{"DrawSplashScreen",	CommandDrawSplashScreen,	NULL,	NULL,		&TypeVoid,	2,	&TypeStringP,"status",
																							&TypeFloat,"progress"},
	{"ModelCalcMove",		CommandModelCalcMove,		NULL,	NULL,		&TypeVoid,	1,	&TypeModelP,"mod"},
	{"ModelDraw",			CommandModelDraw,			NULL,	NULL,		&TypeVoid,	4,	&TypeModelP,"mod",
																							&TypeInt,"skin",
																							&TypeMatrixP,"mat",
																							&TypeBool,"fx"},
	{"ModelGetVertex",		CommandModelGetVertex,		NULL,	(char*)&ModelGetVertex,&TypeVector,4,&TypeModelP,"mod",
																							&TypeInt,"index",
																							&TypeInt,"skin",
																							&TypeMatrixP,"mat"},
	{"ModelMoveReset",		CommandModelMoveReset,		NULL,	NULL,		&TypeVoid,	1,	&TypeModelP,"m"},
	{"ModelMoveSet",		CommandModelMoveSet,		NULL,	NULL,		&TypeBool,	9,	&TypeModelP,"m",
																							&TypeInt,"operation",
																							&TypeFloat,"param1",
																							&TypeFloat,"param2",
																							&TypeInt,"move",
																							&TypeFloatP,"time",
																							&TypeFloat,"dt",
																							&TypeFloat,"v",
																							&TypeBool,"loop"},
	{"ModelMoveGetFrames",	CommandModelMoveGetFrames,	NULL,	NULL,		&TypeInt,	2,	&TypeModelP,"m",
																							&TypeInt,"move"},
	{"TerrainUpdate",		CommandTerrainUpdate,		NULL,	NULL,		&TypeVoid,	6,	&TypeTerrainP,"t",
																							&TypeInt,"x1",
																							&TypeInt,"x2",
																							&TypeInt,"z1",
																							&TypeInt,"z2",
																							&TypeInt,"mode"},
	{"RenderScene",			CommandRenderScene,			NULL,	NULL,		&TypeVoid,	0},
	{"GetG",				CommandGetG,				NULL,	NULL,		&TypeVector,1,	&TypeVectorP,"pos"},
	{"Trace",				CommandTrace,				NULL,	NULL,		&TypeBool,	5,	&TypeVectorP,"p1",
																							&TypeVectorP,"p2",
																							&TypeVectorP,"tp",
																							&TypeInt,"mode",
																							&TypeInt,"o_ignore"},
	// file access
	{"FileOpen",			CommandFileOpen,			NULL,	(char*)&FileOpen,			&TypeFile,	1,	&TypeStringP,"filename"},
	{"FileCreate",			CommandFileCreate,			NULL,	(char*)&FileCreate,			&TypeFile,	1,	&TypeStringP,"filename"},
	{"FileClose",			CommandFileClose,			NULL,	(char*)&FileClose,			&TypeBool,	1,	&TypeFile,"f"},
	{"FileGetDate",			CommandFileGetDate,			NULL,	(char*)&FileGetDate,		&TypeDate,	2,	&TypeFile,"f",
																											&TypeInt,"type"},
	{"FileWriteBool",		CommandFileWriteBool,		NULL,	mf((tmf)&CFile::WriteBool),	&TypeVoid,	2,	&TypeFile,"f",
																											&TypeBool,"b"},
	{"FileWriteInt",		CommandFileWriteInt,		NULL,	mf((tmf)&CFile::WriteInt),	&TypeVoid,	2,	&TypeFile,"f",
																											&TypeInt,"i"},
	{"FileWriteStr",		CommandFileWriteString,		NULL,	(char*)&FileWriteStr,		&TypeVoid,	2,	&TypeFile,"f",
																											&TypeStringP,"str"},
//	{"FileWriteStr",		CommandFileWriteString,		NULL,	mf((tmf)&CFile::WriteStr),	&TypeVoid,	2,	&TypeFile,"f",&TypeString,"str"},
	{"FileReadBool",		CommandFileReadBool,		NULL,	mf((tmf)&CFile::ReadBool),	&TypeBool,	1,	&TypeFile,"f"},
	{"FileReadInt",			CommandFileReadInt,			NULL,	mf((tmf)&CFile::ReadInt),	&TypeInt,	1,	&TypeFile,"f"},
	{"FileReadStr",			CommandFileReadString,		NULL,	mf((tmf)&CFile::ReadStr),	&TypeStringP,1,	&TypeFile,"f"},
	{"FileReadBoolC",		CommandFileReadBoolC,		NULL,	mf((tmf)&CFile::ReadBoolC),	&TypeBool,	1,	&TypeFile,"f"},
	{"FileReadIntC",		CommandFileReadIntC,		NULL,	mf((tmf)&CFile::ReadIntC),	&TypeInt,	1,	&TypeFile,"f"},
	{"FileReadStrC",		CommandFileReadStringC,		NULL,	mf((tmf)&CFile::ReadStrC),	&TypeStringP,1,	&TypeFile,"f"},
	{"FileTestExistence",	CommandFileTestExistence,	NULL,	(char*)&file_test_existence,&TypeBool,	1,	&TypeStringP,"filename"},
 	{"dir_search",			Commanddir_search,		NULL,	(char*)&dir_search_2,		&TypeInt,	3,	&TypeStringP,"dir",
																											&TypeStringP,"filter",
																											&TypeBool,"show_dirs"},
	{"NetConnect",			CommandNetConnect,			NULL,	(char*)&NixNetConnectSocket,&TypeInt,	2,	&TypeStringP,"addr",
																											&TypeInt,"port"},
	{"NetAccept",			CommandNetAccept,			NULL,	(char*)&NixNetAcceptSocket,	&TypeInt,	1,	&TypeInt,"s"},
	{"NetCreateSocket",		CommandNetCreateSocket,		NULL,	(char*)&NixNetCreateSocket,	&TypeInt,	2,	&TypeInt,"port",
																											&TypeBool,"blocking"},
	{"NetClose",			CommandNetClose,			NULL,	(char*)&NixNetCloseSocket,	&TypeVoid,	1,	&TypeIntP,"s"},
	{"NetResetBuffer",		CommandNetResetBuffer,		NULL,	(char*)&NixNetResetBuffer,	&TypeVoid,	0	},
	{"NetReadBuffer",		CommandNetReadBuffer,		NULL,	(char*)&NixNetReadBuffer,	&TypeVoid,	1,	&TypeInt,"s"},
	{"NetWriteBuffer",		CommandNetWriteBuffer,		NULL,	(char*)&NixNetWriteBuffer,	&TypeVoid,	1,	&TypeInt,"s"},
	{"NetReadyToWrite",		CommandNetReadyToWrite,		NULL,	(char*)&NixNetReadyToWrite,	&TypeBool,	1,	&TypeInt,"s"},
	{"NetReadyToRead",		CommandNetReadyToRead,		NULL,	(char*)&NixNetReadyToRead,	&TypeBool,	1,	&TypeInt,"s"},
	{"NetReadInt",			CommandNetReadInt,			NULL,	(char*)&NixNetReadInt,		&TypeInt,	0	},
	{"NetReadBool",			CommandNetReadBool,			NULL,	(char*)&NixNetReadBool,		&TypeBool,	0	},
	{"NetReadFloat",		CommandNetReadFloat,		NULL,	(char*)&NixNetReadFloat,	&TypeFloat,	0	},
	{"NetReadVector",		CommandNetReadVector,		NULL,	(char*)&NixNetReadVector,	&TypeVector,0	},
	{"NetReadStr",			CommandNetReadStr,			NULL,	(char*)&NixNetReadStr,		&TypeStringP,0	},
	{"NetWriteInt",			CommandNetWriteInt,			NULL,	(char*)&NixNetWriteInt,		&TypeVoid,	1,	&TypeInt,"i"},
	{"NetWriteBool",		CommandNetWriteBool,		NULL,	(char*)&NixNetWriteBool,	&TypeVoid,	1,	&TypeBool,"b"},
	{"NetWriteFloat",		CommandNetWriteFloat,		NULL,	(char*)&NixNetWriteFloat,	&TypeVoid,	1,	&TypeFloat,"f"},
	{"NetWriteVector",		CommandNetWriteVector,		NULL,	(char*)&NixNetWriteVector,	&TypeVoid,	1,	&TypeVector,"v"},
	{"NetWriteStr",			CommandNetWriteStr,			NULL,	(char*)&NixNetWriteStr,		&TypeVoid,	1,	&TypeStringP,"s"},

// 	{"ExecuteScript",		CommandExecuteScript,		NULL,	NULL,				&TypeVoid,		1,	&TypeString,"filename"},
};



// automatische Typ-Umwandlungen


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

#define NUM_TYPE_CASTS	3

int NumTypeCasts=NUM_TYPE_CASTS;

sTypeCast TypeCast[NUM_TYPE_CASTS]={
	{&TypeInt,		&TypeFloat,	CommandFloatFromInt,	(t_cast_func*)&CastInt2Float},
	{&TypeFloat,	&TypeInt,	CommandIntFromFloat,	(t_cast_func*)&CastFloat2Int},
	{&TypeInt,		&TypeChar,	CommandCharFromInt,		(t_cast_func*)&CastInt2Char}
};







// Spiele-Variablen, die von Programmstart an existieren
void ScriptLinkExternalData()
{
	int i;

#ifdef ALLOW_SCRIPT_HUI
	PreExternalVar[ExternalVarAppFilename].Pointer=		(char*)&hui->AppFilename,
	PreExternalVar[ExternalVarAppDirectory].Pointer=	(char*)&hui->AppDirectory,
#endif
#ifdef _X_ALLOW_GOD_
	PreExternalVar[ExternalVarAppName].Pointer=			(char*)&AppName,
	PreExternalVar[ExternalVarElapsed].Pointer=			(char*)&Elapsed,
	PreExternalVar[ExternalVarElapsedRT].Pointer=		(char*)&ElapsedRT,
	PreExternalVar[ExternalVarTimeScale].Pointer=		(char*)&TimeScale,
	PreExternalVar[ExternalVarInitialWorldFile].Pointer=(char*)&InitialWorldFile,
	PreExternalVar[ExternalVarCurrentWorldFile].Pointer=(char*)&CurrentWorldFile,
	PreExternalVar[ExternalVarSessionName].Pointer=		(char*)&SessionName;
	PreExternalVar[ExternalVarHostNames].Pointer=		(char*)&HostNames;
	PreExternalVar[ExternalVarNumAvailableHosts].Pointer=(char*)&NumAvailableHosts;
	PreExternalVar[ExternalVarHostSessionName].Pointer=	(char*)&HostSessionName;
	PreExternalVar[ExternalVarHostName].Pointer=		(char*)&HostName;
	PreExternalVar[ExternalVarNetIAmHost].Pointer=		(char*)&NetIAmHost;
	PreExternalVar[ExternalVarNetIAmClient].Pointer=	(char*)&NetIAmClient;
	PreExternalVar[ExternalVarDebug].Pointer=			(char*)&Debug;
	PreExternalVar[ExternalVarShowTimings].Pointer=		(char*)&ShowTimings;
	PreExternalVar[ExternalVarWireMode].Pointer=		(char*)&WireMode;
	PreExternalVar[ExternalVarConsoleEnabled].Pointer=	(char*)&ConsoleEnabled;
	PreExternalVar[ExternalVarRecord].Pointer=			(char*)&Record;
	PreExternalVar[ExternalVarShadowLevel].Pointer=		(char*)&ShadowLevel;
	PreExternalVar[ExternalVarMirrorLevelMax].Pointer=	(char*)&MirrorLevelMax;
	PreExternalVar[ExternalVarFpsConstant].Pointer=		(char*)&FpsConstant;
	PreExternalVar[ExternalVarFpsMax].Pointer=			(char*)&FpsMax;
	PreExternalVar[ExternalVarFpsMin].Pointer=			(char*)&FpsMin;
	PreExternalVar[ExternalVarFpsWanted].Pointer=		(char*)&FpsWanted;
	PreExternalVar[ExternalVarDetailLevel].Pointer=		(char*)&DetailLevel;
	PreExternalVar[ExternalVarDetailFactorInv].Pointer=	(char*)&DetailFactorInv;
	PreExternalVar[ExternalVarNetworkEnabled].Pointer=	(char*)&NetworkEnabled;
	PreExternalVar[ExternalVarSuperGravitation].Pointer=(char*)&SuperGravitation;
	PreExternalVar[ExternalVarBackGroundColor].Pointer=	(char*)&BackGroundColor;
	PreExternalVar[ExternalVarFog].Pointer=				(char*)&GlobalFog;
	PreExternalVar[ExternalVarSkybox].Pointer=			(char*)&pSkyBox;
	PreExternalVar[ExternalVarSkyboxAng].Pointer=		(char*)&pSkyBoxAng;
	PreExternalVar[ExternalVarXFontColor].Pointer=		(char*)&XFontColor;
	PreExternalVar[ExternalVarXFontIndex].Pointer=		(char*)&XFontIndex;
	PreExternalVar[ExternalVarNumObjects].Pointer=		(char*)&NumObjects;
	PreExternalVar[ExternalVarObject].Pointer=			(char*)&object;
	PreExternalVar[ExternalVarEgo].Pointer=				(char*)&ego;
	PreExternalVar[ExternalVarNumTerrains].Pointer=		(char*)&NumTerrains;
	PreExternalVar[ExternalVarTerrain].Pointer=			(char*)&terrain;
	PreExternalVar[ExternalVarGravitation].Pointer=		(char*)&GlobalG;
	PreExternalVar[ExternalVarAmbient].Pointer=			(char*)&GlobalAmbient;
	PreExternalVar[ExternalVarSunLight].Pointer=		(char*)&SunLight;
	PreExternalVar[ExternalVarScriptVar].Pointer=		(char*)&ScriptVar;
	PreExternalVar[ExternalVarTraceHitType].Pointer=	(char*)&TraceHitType;
	PreExternalVar[ExternalVarTraceHitIndex].Pointer=	(char*)&TraceHitIndex;
	PreExternalVar[ExternalVarTraceHitSubModel].Pointer=(char*)&TraceHitSubModel;
	PreExternalVar[ExternalVarGodNumMessages].Pointer=	(char*)&NumMessages;
	PreExternalVar[ExternalVarGodMessage].Pointer=		(char*)&Message;
	PreExternalVar[ExternalVarCurrentGrouping].Pointer=	(char*)&CurrentGrouping;
#endif
	PreExternalVar[ExternalVarDirSearchNum].Pointer=	(char*)&dir_search_num;
	PreExternalVar[ExternalVarDirSearchName].Pointer=	(char*)&dir_search_name_p;
	PreExternalVar[ExternalVarDirSearchIsDir].Pointer=	(char*)&dir_search_is_dir;
	PreExternalVar[ExternalVarTargetWidth].Pointer=		(char*)&NixTargetWidth;
	PreExternalVar[ExternalVarTargetHeight].Pointer=	(char*)&NixTargetHeight;
	PreExternalVar[ExternalVarScreenWidth].Pointer=		(char*)&NixScreenWidth;
	PreExternalVar[ExternalVarScreenHeight].Pointer=	(char*)&NixScreenHeight;
	PreExternalVar[ExternalVarScreenDepth].Pointer=		(char*)&NixScreenDepth;
	PreExternalVar[ExternalVarApi].Pointer=				(char*)&NixApi;
	PreExternalVar[ExternalVarMinDepth].Pointer=		(char*)&NixMinDepth;
	PreExternalVar[ExternalVarMaxDepth].Pointer=		(char*)&NixMaxDepth;
	PreExternalVar[ExternalVarTextureLifeTime].Pointer=	(char*)&NixTextureMaxFramesToLive;
#ifdef ALLOW_SCRIPT_HUI
	PreExternalVar[ExternalVarHuiFileDialogPath].Pointer=	(char*)&hui->FileDialogPath;
	PreExternalVar[ExternalVarHuiFileDialogFile].Pointer=	(char*)&hui->FileDialogFile;
	PreExternalVar[ExternalVarHuiFileDialogCompleteName].Pointer=(char*)&hui->FileDialogCompleteName;
	PreExternalVar[ExternalVarHuiRunning].Pointer=			(char*)&hui->Running;
#endif

	for (i=0;i<NumPreCommands;i++){
		// Klassen-Instanzen
		if (PreCommand[i].Instance==i_hui)		PreCommand[i].Instance=(char*)&hui;
		// Funktions-Pointer
#ifdef _X_ALLOW_META_
		if (PreCommand[i].Nr==CommandXFDrawStr)			PreCommand[i].Func=(char*)&XFDrawStr;
		if (PreCommand[i].Nr==CommandXFDrawVertStr)		PreCommand[i].Func=(char*)&XFDrawVertStr;
		if (PreCommand[i].Nr==CommandXFGetWidth)		PreCommand[i].Func=(char*)&XFGetWidth;
		if (PreCommand[i].Nr==CommandLoadXFont)			PreCommand[i].Func=(char*)&MetaLoadXFont;
		if (PreCommand[i].Nr==CommandExitProgram)		PreCommand[i].Func=(char*)MetaExitProgram;
		if (PreCommand[i].Nr==CommandScreenShot)		PreCommand[i].Func=(char*)MetaScreenShot;
		if (PreCommand[i].Nr==CommandLoadWorld)			PreCommand[i].Func=(char*)MetaLoadWorld;
		if (PreCommand[i].Nr==CommandLoadGameFromHost)	PreCommand[i].Func=(char*)MetaLoadGameFromHost;
		if (PreCommand[i].Nr==CommandSaveGameState)		PreCommand[i].Func=(char*)MetaSaveGameState;
		if (PreCommand[i].Nr==CommandLoadGameState)		PreCommand[i].Func=(char*)MetaLoadGameState;
		if (PreCommand[i].Nr==CommandFindHosts)			PreCommand[i].Func=(char*)MetaFindHosts;
		if (PreCommand[i].Nr==CommandDrawSplashScreen)	PreCommand[i].Func=(char*)MetaDrawSplashScreen;
#endif
		if (PreCommand[i].Nr==CommandNixUpdateInput)	PreCommand[i].Func=(char*)&NixUpdateInput;
		if (PreCommand[i].Nr==CommandGetKey)			PreCommand[i].Func=(char*)&NixGetKey;
		if (PreCommand[i].Nr==CommandGetKeyDown)		PreCommand[i].Func=(char*)&NixGetKeyDown;
		if (PreCommand[i].Nr==CommandGetKeyRhythmDown)	PreCommand[i].Func=(char*)&NixGetKeyRhythmDown;
		if (PreCommand[i].Nr==CommandGetKeyChar)		PreCommand[i].Func=(char*)&NixGetKeyChar;
		if (PreCommand[i].Nr==CommandGetKeyUp)			PreCommand[i].Func=(char*)&NixGetKeyUp;
		if (PreCommand[i].Nr==CommandGetKeyName)		PreCommand[i].Func=mf((tmf)&CHui::GetKeyName);
		if (PreCommand[i].Nr==CommandGetMouseX)			PreCommand[i].Func=(char*)&NixGetMx;
		if (PreCommand[i].Nr==CommandGetMouseY)			PreCommand[i].Func=(char*)&NixGetMy;
		if (PreCommand[i].Nr==CommandGetMouseDx)		PreCommand[i].Func=(char*)&NixGetDx;
		if (PreCommand[i].Nr==CommandGetMouseDy)		PreCommand[i].Func=(char*)&NixGetDy;
		if (PreCommand[i].Nr==CommandGetWheelD)			PreCommand[i].Func=(char*)&NixGetWheelD;
		if (PreCommand[i].Nr==CommandGetButL)			PreCommand[i].Func=(char*)&NixGetButL;
		if (PreCommand[i].Nr==CommandGetButM)			PreCommand[i].Func=(char*)&NixGetButM;
		if (PreCommand[i].Nr==CommandGetButR)			PreCommand[i].Func=(char*)&NixGetButR;
		if (PreCommand[i].Nr==CommandGetButLDown)		PreCommand[i].Func=(char*)&NixGetButLDown;
		if (PreCommand[i].Nr==CommandGetButMDown)		PreCommand[i].Func=(char*)&NixGetButMDown;
		if (PreCommand[i].Nr==CommandGetButRDown)		PreCommand[i].Func=(char*)&NixGetButRDown;
		if (PreCommand[i].Nr==CommandGetButLUp)			PreCommand[i].Func=(char*)&NixGetButLUp;
		if (PreCommand[i].Nr==CommandGetButMUp)			PreCommand[i].Func=(char*)&NixGetButMUp;
		if (PreCommand[i].Nr==CommandGetButRUp)			PreCommand[i].Func=(char*)&NixGetButRUp;
		if (PreCommand[i].Nr==CommandNixStart)			PreCommand[i].Func=(char*)&NixStart;
		if (PreCommand[i].Nr==CommandNixEnd)			PreCommand[i].Func=(char*)&NixEnd;
		if (PreCommand[i].Nr==CommandNixKillWindows)	PreCommand[i].Func=(char*)&NixKillWindows;
		if (PreCommand[i].Nr==CommandNixDraw2D)			PreCommand[i].Func=(char*)&NixDraw2D;
		if (PreCommand[i].Nr==CommandNixDraw3D)			PreCommand[i].Func=(char*)&NixDraw3D;
		if (PreCommand[i].Nr==CommandNixDrawStr)		PreCommand[i].Func=(char*)&NixDrawStr;
		if (PreCommand[i].Nr==CommandNixDrawLineH)		PreCommand[i].Func=(char*)&NixDrawLineH;
		if (PreCommand[i].Nr==CommandNixDrawLineV)		PreCommand[i].Func=(char*)&NixDrawLineV;
		if (PreCommand[i].Nr==CommandNixDrawLine)		PreCommand[i].Func=(char*)&NixDrawLine;
		if (PreCommand[i].Nr==CommandNixDrawSprite)		PreCommand[i].Func=(char*)&NixDrawSprite;
	//{"NixDrawModel2D",	CommandNixDrawModel2D,		NULL,	mf((tmf)&CNix::Close),				&TypeVoid,	1,	&TypeFloat,"???"}, // ???
		if (PreCommand[i].Nr==CommandNixSetAlpha)		PreCommand[i].Func=(char*)&NixSetAlphaM;
		if (PreCommand[i].Nr==CommandNixSetAlpha2)		PreCommand[i].Func=(char*)&NixSetAlphaSD;
		if (PreCommand[i].Nr==CommandNixSetStencil)		PreCommand[i].Func=(char*)&NixSetStencil;
		if (PreCommand[i].Nr==CommandNixSetView)		PreCommand[i].Func=(char*)&NixSetViewM;
		if (PreCommand[i].Nr==CommandNixSetView2)		PreCommand[i].Func=(char*)&NixSetViewV;
		if (PreCommand[i].Nr==CommandNixSetZ)			PreCommand[i].Func=(char*)&NixSetZ;
		if (PreCommand[i].Nr==CommandNixSetMaterial)	PreCommand[i].Func=(char*)&NixSetMaterial;
		if (PreCommand[i].Nr==CommandNixSetFontColor)	PreCommand[i].Func=(char*)&NixSetFontColor;
		if (PreCommand[i].Nr==CommandNixEnableLighting)	PreCommand[i].Func=(char*)&NixEnableLighting;
		if (PreCommand[i].Nr==CommandNixSetVideoMode)	PreCommand[i].Func=(char*)&NixSetVideoMode;
		if (PreCommand[i].Nr==CommandNixCreateDynamicTexture)	PreCommand[i].Func=(char*)&NixCreateDynamicTexture;
		if (PreCommand[i].Nr==CommandGetVecProject)		PreCommand[i].Func=(char*)&NixGetVecProject;
		if (PreCommand[i].Nr==CommandGetVecUnproject)	PreCommand[i].Func=(char*)&NixGetVecUnproject;
		if (PreCommand[i].Nr==CommandGetVecProjectRel)	PreCommand[i].Func=(char*)&NixGetVecProjectRel;
		if (PreCommand[i].Nr==CommandGetVecUnprojectRel)PreCommand[i].Func=(char*)&NixGetVecUnprojectRel;
		if (PreCommand[i].Nr==CommandNixVBEmpty)		PreCommand[i].Func=(char*)&NixVBEmpty;
		if (PreCommand[i].Nr==CommandNixVBAddTrias)		PreCommand[i].Func=(char*)&NixVBAddTrias;
#ifdef _X_ALLOW_GOD_
		if (PreCommand[i].Nr==CommandTextureLoad)		PreCommand[i].Func=(char*)&MetaLoadTexture;
		if (PreCommand[i].Nr==CommandPictureCreate)		PreCommand[i].Func=(char*)&MetaCreatePicture;
		if (PreCommand[i].Nr==CommandPicture3DCreate)	PreCommand[i].Func=(char*)&MetaCreatePicture3D;
		if (PreCommand[i].Nr==CommandTextCreate)		PreCommand[i].Func=(char*)&MetaCreateText;
		if (PreCommand[i].Nr==CommandGroupingCreate)	PreCommand[i].Func=(char*)&MetaCreateGrouping;
		if (PreCommand[i].Nr==CommandParticleCreate)	PreCommand[i].Func=(char*)&FxParticleCreateDef;
		if (PreCommand[i].Nr==CommandParticleRotCreate)	PreCommand[i].Func=(char*)&FxParticleCreateRot;
		if (PreCommand[i].Nr==CommandParticleBeamCreate)PreCommand[i].Func=(char*)&FxParticleCreateBeam;
		if (PreCommand[i].Nr==CommandModelLoad)			PreCommand[i].Func=(char*)&MetaLoadModel;
		if (PreCommand[i].Nr==CommandItemLoad)			PreCommand[i].Func=(char*)&MetaLoadItem;
		if (PreCommand[i].Nr==CommandItemGetOID)		PreCommand[i].Func=(char*)&MetaGetItemOID;
		if (PreCommand[i].Nr==CommandViewCreate)		PreCommand[i].Func=(char*)&CameraCreateView;
		if (PreCommand[i].Nr==CommandShaderFileLoad)	PreCommand[i].Func=(char*)&MetaLoadShaderFile;
		if (PreCommand[i].Nr==CommandModelCalcMove)		PreCommand[i].Func=mf((tmf)&CModel::CalcMove);
		if (PreCommand[i].Nr==CommandModelDraw)			PreCommand[i].Func=mf((tmf)&CModel::Draw);
		if (PreCommand[i].Nr==CommandXDelete)			PreCommand[i].Func=(char*)&MetaXDelete;
		if (PreCommand[i].Nr==CommandGetObjectByName)	PreCommand[i].Func=(char*)&GetObjectByName;
		if (PreCommand[i].Nr==CommandCreateObject)		PreCommand[i].Func=(char*)&_CreateObject;
		if (PreCommand[i].Nr==CommandDeleteObject)		PreCommand[i].Func=(char*)&_DeleteObject;
		if (PreCommand[i].Nr==CommandCamStartScript)	PreCommand[i].Func=(char*)&CameraStartScript;
		if (PreCommand[i].Nr==CommandCamStopScript)		PreCommand[i].Func=(char*)&CameraStopScript;
		if (PreCommand[i].Nr==CommandModelMoveReset)	PreCommand[i].Func=(char*)&MetaModelMoveReset;
		if (PreCommand[i].Nr==CommandModelMoveSet)		PreCommand[i].Func=(char*)&MetaModelMoveSet;
		if (PreCommand[i].Nr==CommandModelMoveGetFrames)PreCommand[i].Func=(char*)&MetaModelMoveGetFrames;
		if (PreCommand[i].Nr==CommandModelMoveGetFrames)PreCommand[i].Func=(char*)&MetaModelMoveGetFrames;
		if (PreCommand[i].Nr==CommandTerrainUpdate)		PreCommand[i].Func=(char*)&TerrainUpdate;
		if (PreCommand[i].Nr==CommandGetG)				PreCommand[i].Func=(char*)&GetG;
		if (PreCommand[i].Nr==CommandTrace)				PreCommand[i].Func=(char*)&GodTrace;
		if (PreCommand[i].Nr==CommandSoundEmit)			PreCommand[i].Func=(char*)&MetaSoundEmit;
		if (PreCommand[i].Nr==CommandSoundCreate)		PreCommand[i].Func=(char*)&MetaSoundManagedNew;
		if (PreCommand[i].Nr==CommandSoundSetData)		PreCommand[i].Func=(char*)&MetaSoundManagedSetData;
		if (PreCommand[i].Nr==CommandSoundDelete)		PreCommand[i].Func=(char*)&MetaSoundManagedDelete;
		if (PreCommand[i].Nr==CommandMusicLoad)			PreCommand[i].Func=(char*)&MetaMusicLoad;
		if (PreCommand[i].Nr==CommandMusicPlay)			PreCommand[i].Func=(char*)&NixMusicPlay;
		if (PreCommand[i].Nr==CommandMusicStop)			PreCommand[i].Func=(char*)&NixMusicStop;
		if (PreCommand[i].Nr==CommandMusicPause)		PreCommand[i].Func=(char*)&NixMusicSetPause;
		if (PreCommand[i].Nr==CommandMusicSetRate)		PreCommand[i].Func=(char*)&NixMusicSetRate;
		if (PreCommand[i].Nr==CommandFXLightCreate)		PreCommand[i].Func=(char*)&FxLightCreate;
		if (PreCommand[i].Nr==CommandFXLightSetDirectional)	PreCommand[i].Func=(char*)&FxLightSetDirectional;
		if (PreCommand[i].Nr==CommandFXLightSetRadial)	PreCommand[i].Func=(char*)&FxLightSetRadial;
		if (PreCommand[i].Nr==CommandFXLightDelete)		PreCommand[i].Func=(char*)&FxLightDelete;
		if (PreCommand[i].Nr==CommandFXLightEnable)		PreCommand[i].Func=(char*)&FxLightEnable;

#endif
#ifdef ALLOW_SCRIPT_HUI
		if (PreCommand[i].Nr==CommandHuiRun)			PreCommand[i].Func=mf((tmf)&CHui::Run);
		if (PreCommand[i].Nr==CommandHuiEnd)			PreCommand[i].Func=mf((tmf)&CHui::End);
		if (PreCommand[i].Nr==CommandHuiWaitTillWindowClosed)PreCommand[i].Func=mf((tmf)&CHui::WaitTillWindowClosed);
		if (PreCommand[i].Nr==CommandHuiSleep)			PreCommand[i].Func=mf((tmf)&CHui::Sleep);
		if (PreCommand[i].Nr==CommandHuiFileDialogOpen)	PreCommand[i].Func=mf((tmf)&CHui::FileDialogOpen);
		if (PreCommand[i].Nr==CommandHuiFileDialogSave)	PreCommand[i].Func=mf((tmf)&CHui::FileDialogSave);
		if (PreCommand[i].Nr==CommandHuiFileDialogDir)	PreCommand[i].Func=mf((tmf)&CHui::FileDialogDir);
		if (PreCommand[i].Nr==CommandHuiQuestionBox)	PreCommand[i].Func=mf((tmf)&CHui::QuestionBox);
		if (PreCommand[i].Nr==CommandHuiInfoBox)		PreCommand[i].Func=mf((tmf)&CHui::InfoBox);
		if (PreCommand[i].Nr==CommandHuiErrorBox)		PreCommand[i].Func=mf((tmf)&CHui::ErrorBox);
		if (PreCommand[i].Nr==CommandHuiConfigWriteInt)	PreCommand[i].Func=mf((tmf)&CHui::ConfigWriteInt);
		if (PreCommand[i].Nr==CommandHuiConfigWriteStr)	PreCommand[i].Func=mf((tmf)&CHui::ConfigWriteStr);
		if (PreCommand[i].Nr==CommandHuiConfigReadInt)	PreCommand[i].Func=mf((tmf)&CHui::ConfigReadInt);
		if (PreCommand[i].Nr==CommandHuiConfigReadStr)	PreCommand[i].Func=mf((tmf)&CHui::ConfigReadStr);
		if (PreCommand[i].Nr==CommandHuiCopyToClipBoard)PreCommand[i].Func=mf((tmf)&CHui::CopyToClipBoard);
		if (PreCommand[i].Nr==CommandHuiOpenDocument)	PreCommand[i].Func=mf((tmf)&CHui::OpenDocument);
		if (PreCommand[i].Nr==CommandHuiCreateTimer)	PreCommand[i].Func=mf((tmf)&CHui::CreateTimer);
		if (PreCommand[i].Nr==CommandHuiGetTime)		PreCommand[i].Func=mf((tmf)&CHui::GetTime);
		if (PreCommand[i].Nr==CommandHuiMenuOpenPopup)		PreCommand[i].Func=mf((tmf)&CHuiMenu::OpenPopup);
		if (PreCommand[i].Nr==CommandHuiMenuAddEntry)		PreCommand[i].Func=mf((tmf)&CHuiMenu::AddEntry);
		if (PreCommand[i].Nr==CommandHuiMenuAddSeparator)	PreCommand[i].Func=mf((tmf)&CHuiMenu::AddSeparator);
		if (PreCommand[i].Nr==CommandHuiMenuAddSubMenu)		PreCommand[i].Func=mf((tmf)&CHuiMenu::AddSubMenu);
		if (PreCommand[i].Nr==CommandHuiMenuCheckItem)		PreCommand[i].Func=mf((tmf)&CHuiMenu::CheckItem);
		if (PreCommand[i].Nr==CommandHuiMenuIsItemChecked)	PreCommand[i].Func=mf((tmf)&CHuiMenu::IsItemChecked);
		if (PreCommand[i].Nr==CommandHuiMenuEnableItem)		PreCommand[i].Func=mf((tmf)&CHuiMenu::EnableItem);
		if (PreCommand[i].Nr==CommandHuiMenuSetText)		PreCommand[i].Func=mf((tmf)&CHuiMenu::SetText);
		if (PreCommand[i].Nr==CommandHuiCreateWindow)		PreCommand[i].Func=(char*)&HuiCreateWindow;
		if (PreCommand[i].Nr==CommandHuiCreateNixWindow)	PreCommand[i].Func=(char*)&HuiCreateNixWindow;
		if (PreCommand[i].Nr==CommandHuiCreateDialog)		PreCommand[i].Func=(char*)&HuiCreateDialog;
		if (PreCommand[i].Nr==CommandHuiWinUpdate)			PreCommand[i].Func=mf((tmf)&CHuiWindow::Update);
		if (PreCommand[i].Nr==CommandHuiWinHide)			PreCommand[i].Func=mf((tmf)&CHuiWindow::Hide);
		if (PreCommand[i].Nr==CommandHuiWinSetMaximized)	PreCommand[i].Func=mf((tmf)&CHuiWindow::SetMaximized);
		if (PreCommand[i].Nr==CommandHuiWinIsMaximized)		PreCommand[i].Func=mf((tmf)&CHuiWindow::IsMaximized);
		if (PreCommand[i].Nr==CommandHuiWinIsMinimized)		PreCommand[i].Func=mf((tmf)&CHuiWindow::IsMinimized);
		if (PreCommand[i].Nr==CommandHuiWinSetID)			PreCommand[i].Func=mf((tmf)&CHuiWindow::SetID);
		if (PreCommand[i].Nr==CommandHuiWinSetFullscreen)	PreCommand[i].Func=mf((tmf)&CHuiWindow::SetFullscreen);
		if (PreCommand[i].Nr==CommandHuiWinSetTitle)		PreCommand[i].Func=mf((tmf)&CHuiWindow::SetTitle);
		if (PreCommand[i].Nr==CommandHuiWinSetPosition)		PreCommand[i].Func=mf((tmf)&CHuiWindow::SetPosition);
//		if (PreCommand[i].Nr==CommandHuiWinSetOuterior)		PreCommand[i].Func=mf((tmf)&CHuiWindow::SetOuterior);
//		if (PreCommand[i].Nr==CommandHuiWinGetOuterior)		PreCommand[i].Func=mf((tmf)&CHuiWindow::GetOuterior);
//		if (PreCommand[i].Nr==CommandHuiWinSetInerior)		PreCommand[i].Func=mf((tmf)&CHuiWindow::SetInerior);
//		if (PreCommand[i].Nr==CommandHuiGetInterior)		PreCommand[i].Func=mf((tmf)&CHuiWindow::GetInterior);
		if (PreCommand[i].Nr==CommandHuiWinSetCursorPos)	PreCommand[i].Func=mf((tmf)&CHuiWindow::SetCursorPos);
		if (PreCommand[i].Nr==CommandHuiWinActivate)		PreCommand[i].Func=mf((tmf)&CHuiWindow::Activate);
		if (PreCommand[i].Nr==CommandHuiWinIsActive)		PreCommand[i].Func=mf((tmf)&CHuiWindow::IsActive);
		if (PreCommand[i].Nr==CommandHuiWinAddButton)		PreCommand[i].Func=mf((tmf)&CHuiWindow::AddButton);
		if (PreCommand[i].Nr==CommandHuiWinAddCheckBox)		PreCommand[i].Func=mf((tmf)&CHuiWindow::AddCheckBox);
		if (PreCommand[i].Nr==CommandHuiWinAddText)			PreCommand[i].Func=mf((tmf)&CHuiWindow::AddText);
		if (PreCommand[i].Nr==CommandHuiWinAddEdit)			PreCommand[i].Func=mf((tmf)&CHuiWindow::AddEdit);
		if (PreCommand[i].Nr==CommandHuiWinAddGroup)		PreCommand[i].Func=mf((tmf)&CHuiWindow::AddGroup);
		if (PreCommand[i].Nr==CommandHuiWinAddComboBox)		PreCommand[i].Func=mf((tmf)&CHuiWindow::AddComboBox);
		if (PreCommand[i].Nr==CommandHuiWinAddTabControl)	PreCommand[i].Func=mf((tmf)&CHuiWindow::AddTabControl);
		if (PreCommand[i].Nr==CommandHuiWinSetTabCreationPage)	PreCommand[i].Func=mf((tmf)&CHuiWindow::SetTabCreationPage);
		if (PreCommand[i].Nr==CommandHuiWinAddListView)		PreCommand[i].Func=mf((tmf)&CHuiWindow::AddListView);
		if (PreCommand[i].Nr==CommandHuiWinSetControlText)	PreCommand[i].Func=mf((tmf)&CHuiWindow::SetControlText);
		if (PreCommand[i].Nr==CommandHuiWinGetControlText)	PreCommand[i].Func=mf((tmf)&CHuiWindow::GetControlText);
		if (PreCommand[i].Nr==CommandHuiWinEnableControl)	PreCommand[i].Func=mf((tmf)&CHuiWindow::EnableControl);
		if (PreCommand[i].Nr==CommandHuiWinIsControlEnabled)PreCommand[i].Func=mf((tmf)&CHuiWindow::IsControlEnabled);
		if (PreCommand[i].Nr==CommandHuiWinCheckControl)	PreCommand[i].Func=mf((tmf)&CHuiWindow::CheckControl);
		if (PreCommand[i].Nr==CommandHuiWinIsControlChecked)PreCommand[i].Func=mf((tmf)&CHuiWindow::IsControlChecked);
		if (PreCommand[i].Nr==CommandHuiWinGetControlSelection)	PreCommand[i].Func=mf((tmf)&CHuiWindow::GetControlSelection);
		if (PreCommand[i].Nr==CommandHuiWinGetControlSelectionM)PreCommand[i].Func=mf((tmf)&CHuiWindow::GetControlSelectionM);
		if (PreCommand[i].Nr==CommandHuiWinSetControlSelection)	PreCommand[i].Func=mf((tmf)&CHuiWindow::SetControlSelection);
		if (PreCommand[i].Nr==CommandHuiWinResetControl)	PreCommand[i].Func=mf((tmf)&CHuiWindow::ResetControl);
#endif
	}
}

/*class aaa{
public:
	aaa();
	vector _cdecl test(int a);
};

aaa::aaa()
{
}

vector bbb;
int ccc;
aaa *ddd;

vector aaa::test(int a)
{
	ccc=a;
	ddd=this;
	return bbb;
}

void _cdecl testtt(int a)
{
	ccc=a;
}

void _cdecl tttest()
{
	bbb=ddd->test(13);
}

vector _cdecl tttest2(int a)
{
	ccc=a;
	return bbb;
}

void _cdecl tttest3()
{
	tttest2(13);
}

#include "dasm.h"*/

// Spiele-Variablen aus Objekten, die erst erstellt werden muessen
void ScriptLinkDynamicExternalData()
{
#ifdef _X_ALLOW_GOD_
	PreExternalVar[ExternalVarCam].Pointer=				(char*)&cam;

	for (int i=0;i<NumPreCommands;i++){
		// Funktions-Pointer
		if (PreCommand[i].Nr==CommandRenderScene)		PreCommand[i].Func=(char*)FxRenderFunc;

	}
	/*msg_write("------------------");
	msg_write(GetAsm(mf((tmf)&aaa::test),-1,true));
	msg_write("------------------");
	msg_write(GetAsm((char*)&testtt,-1,true));
	msg_write("------------------");
	msg_write(GetAsm((char*)&tttest,-1,true));
	msg_write("------------------");
	msg_write("------------------");
	msg_write(GetAsm((char*)&tttest2,-1,true));
	msg_write("------------------");
	msg_write(GetAsm((char*)&tttest3,-1,true));
	msg_write("------------------");*/
#endif
}



sScriptLocation ScriptLocation[NumScriptLocations]={
	{"CalcMovePrae",	ScriptLocationCalcMovePrae},
	{"CalcMovePost",	ScriptLocationCalcMovePost},
	{"RenderPrae",		ScriptLocationRenderPrae},
	{"RenderPost1",		ScriptLocationRenderPost1},
	{"RenderPost2",		ScriptLocationRenderPost2},
	{"GetInputPrae",	ScriptLocationGetInputPrae},
	{"GetInputPost",	ScriptLocationGetInputPost}
};

