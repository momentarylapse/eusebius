#include "script.h"
#include "../file/msg.h"
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


//#define ScriptDebug

static int PreConstantNr,EnumNr;

void reset_pre_script(CPreScript *ps)
{
	//memset(ps, 0, sizeof(CPreScript));
	ps->Error = false;
	strcpy(ps->ErrorMsg, "");
	strcpy(ps->ErrorMsgExt[0], "");
	strcpy(ps->ErrorMsgExt[1], "");
	ps->ErrorLine = 0;
	ps->ErrorColumn = 0;
	ps->IncludeLinkerError = false;
	ps->Buffer = NULL;
	ps->Exp.buffer = NULL;
	ps->FlagShow = false;
	ps->FlagDisassemble = false;
	ps->FlagCompileOS = false;
	ps->FlagCompileInitialRealMode = false;
	ps->FlagOverwriteVariablesOffset = false;
	ps->AsmMetaInfo = NULL;
	ps->Command.reserve(10000);
	ps->Block.reserve(1000);
	ps->Type.reserve(300);
	ps->Function.reserve(100);
	ps->Constant.reserve(1000);
	ps->Enum.reserve(100);
	ps->Define.reserve(100);
	ps->Struct.reserve(100);
	ps->LinkData.reserve(1000);
	strcpy(ps->RootOfAllEvil.Name, "RootOfAllEvil");
	ps->RootOfAllEvil.VarSize = -1;
	ps->RootOfAllEvil.ParamSize = 0;
	ps->RootOfAllEvil.NumParams = 0;
	ps->RootOfAllEvil.Type = TypeVoid;
	/*ps->Command.clear();
	ps->Block.clear();
	ps->Type.clear();
	ps->Function.clear();
	ps->Constant.clear();
	ps->Enum.clear();
	ps->Define.clear();
	ps->Struct.clear();
	ps->LinkData.clear();*/

	for (int i=0;i<PreType.size();i++)
		ps->Type.push_back(PreType[i]);
	
	for (int i=0;i<PreStruct.size();i++)
		ps->Struct.push_back(PreStruct[i]);
}

CPreScript::CPreScript()
{
	reset_pre_script(this);
}


CPreScript::CPreScript(char *filename,bool just_analyse)
{
	msg_db_r("CPreScript",4);
	reset_pre_script(this);
	
	strcpy(Filename,filename);

	Error = !LoadToBuffer(string(ScriptDirectory, Filename), just_analyse);

	
	if (!Error)
		PreCompiler(just_analyse);

	if (!Error)
		Parser();

	clear_exp_buffer(&Exp);
	msg_db_l(4);
}



// ################################################################################################
//                                        Syntax-Analyse
// ################################################################################################

int indent_0;
bool indented, unindented;
void test_indent(int i)
{
	indented = (i > indent_0);
	unindented = (i < indent_0);
	indent_0 = i;
		
}

void reset_indent()
{
	indented = unindented = false;
	indent_0 = 0;
}


#define cur_name		Exp.cur_line->exp[Exp.cur_exp].name
#define get_name(n)		Exp.cur_line->exp[n].name
#define next_exp()		Exp.cur_exp ++//;ExpectNoNewline()
#define end_of_line()	(Exp.cur_exp >= Exp.cur_line->exp.size() - 1) // the last entry is "-eol-"
#define past_end_of_line()	(Exp.cur_exp >= Exp.cur_line->exp.size())
#define next_line()		{Exp.cur_line ++;Exp.cur_exp=0;test_indent(Exp.cur_line->indent);}
#define end_of_file()	((long)Exp.cur_line >= (long)&Exp.line[Exp.line.size() - 1]) // last line = "-eol-"

void line_out(CPreScript *ps)
{
	char str[1024];
	strcpy(str, ps->Exp.cur_line->exp[0].name);
	for (int i=1;ps->Exp.cur_line->exp.size();i++){
		strcat(str, "  ");
		strcat(str, ps->Exp.cur_line->exp[i].name);
	}
}


char Temp[1024];
int ExpKind;

static int shift_right=0;

static void stringout(char *str)
{
	msg_write(str);
}

static void so(char *str)
{
#ifdef ScriptDebug
	if (strlen(str)>256)
		str[256]=0;
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

int s2i2(char *str)
{
	if ((str[0]=='0')&&(str[1]=='x')){
		int r=0;
		str+=2;
		while(str[0]!=0){ 
			r*=16;
			if ((str[0]>='0')&&(str[0]<='9'))
				r+=str[0]-48;
			if ((str[0]>='a')&&(str[0]<='f'))
				r+=str[0]-'a'+10;
			if ((str[0]>='A')&&(str[0]<='F'))
				r+=str[0]-'A'+10;
			str++;
		}
		return r;
	}else
		return	s2i(str);
}

static int SCurrentStack=-1;
#define SCRIPT_STR_STACK_DEPTH		64
static char SStackStr[SCRIPT_STR_STACK_DEPTH][4096];
inline char *get_a_str()
{
	SCurrentStack = (SCurrentStack + 1) % SCRIPT_STR_STACK_DEPTH;
	return SStackStr[SCurrentStack];
}

char *Type2Str(CPreScript *s, sType *type)
{
	char *str = get_a_str();
	if (type)
		strcpy(str,string2("UNBEKANNTER TYP (%s)", type->Name));
	else
		strcpy(str,string2("UNBEKANNTER TYP (%d)", (long)type));
	for (int i=0;i<s->Type.size();i++)
		if (type == s->Type[i])
			strcpy(str,s->Type[i]->Name);
	if (type == TypeUnknown)
		strcpy(str,"[absichtlich unbekannter Typ]");
	return str;
}

char *Kind2Str(int kind)
{
	char *str = get_a_str();
	strcpy(str,string("UNBEKANNTE ART: ", i2s(kind)));
	if (kind == KindVarLocal)			strcpy(str, "lokale Variable");
	if (kind == KindVarGlobal)			strcpy(str, "globale Variable");
	if (kind == KindVarFunction)		strcpy(str, "Funktion als Variable");
	if (kind == KindVarExternal)		strcpy(str, "externe Programm-Variable");
	if (kind == KindConstant)			strcpy(str, "Konstante");
	if (kind == KindRefToConst)			strcpy(str, "Ref. auf Konstante");
	if (kind == KindFunction)			strcpy(str, "Funktion");
	if (kind == KindCompilerFunction)	strcpy(str, "Compiler-Funktion");
	if (kind == KindOperator)			strcpy(str, "Operator");
	if (kind == KindPrimitiveOperator)	strcpy(str, "PRIMITIVER Operator");
	if (kind == KindCommand)			strcpy(str, "Befehl");
	if (kind == KindBlock)				strcpy(str, "Befehls-Block");
	if (kind == KindPointerShift)		strcpy(str, "Pointer-Verschiebung");
	if (kind == KindArray)				strcpy(str, "Array-Element");
	if (kind == KindPointerAsArray)		strcpy(str, "Pointer-Array-Element");
	if (kind == KindReference)			strcpy(str, "Adress-Operator");
	if (kind == KindDereference)		strcpy(str, "Dereferenzierung");
	if (kind == KindDerefPointerShift)	strcpy(str, "deref. Pointer-Verschiebung");
	if (kind == KindType)				strcpy(str, "Typ");
	return str;
}

char *Operator2Str(CPreScript *s,int cmd)
{
	char *str=get_a_str();
	strcpy(str,string("UNBEKANNTER OPERATOR: ",i2s(cmd)));
	strcpy(str,string(	string("(",Type2Str(s,PreOperator[cmd].ParamType1),") "),
						PrimitiveOperator[PreOperator[cmd].PrimitiveID].Name,
						string(" (",Type2Str(s,PreOperator[cmd].ParamType2),")")));
	return str;
}

char *PrimitiveOperator2Str(int cmd)
{
	char *str=get_a_str();
	strcpy(str,string("UNBEKANNTER PRIMITIVER OPERATOR: ",i2s(cmd)));
	strcpy(str,PrimitiveOperator[cmd].Name);
	return str;
}

char *LinkNr2Str(CPreScript *s,int kind,int nr)
{
	char *str=get_a_str();
	strcpy(str,"UNBRAUCHBARE ART");
	if (kind==KindVarLocal)			strcpy(str,i2s(nr));
	if (kind==KindVarGlobal)		strcpy(str,i2s(nr));
	if (kind==KindVarFunction)		strcpy(str,i2s(nr));
	if (kind==KindVarExternal)		strcpy(str,PreExternalVar[nr].Name);
	if (kind==KindConstant)			strcpy(str,i2s(nr));
	if (kind==KindFunction)			strcpy(str,i2s(nr));
	if (kind==KindCompilerFunction)	strcpy(str,PreCommand[nr].Name);
	if (kind==KindOperator)			strcpy(str,Operator2Str(s,nr));
	if (kind==KindPrimitiveOperator)strcpy(str,PrimitiveOperator2Str(nr));
	if (kind==KindCommand)			strcpy(str,i2s(nr));
	if (kind==KindBlock)			strcpy(str,i2s(nr));
	if (kind==KindPointerShift)		strcpy(str,i2s(nr));
	if (kind==KindArray)			strcpy(str,i2s(nr));
	if (kind==KindPointerAsArray)	strcpy(str,i2s(nr));
	if (kind==KindReference)		strcpy(str,"(keine Parameter)");
	if (kind==KindDereference)		strcpy(str,"(keine Parameter)");
	if (kind==KindDerefPointerShift)strcpy(str,i2s(nr));
	if (kind==KindType)				strcpy(str,s->Type[nr]->Name);
	return str;
}

void CPreScript::DoError(char *str)
{
	if (Error)
		return;
	stringout("\n\n\n");
	stringout("------------------------       Error       -----------------------");	
	Error = true;

	// what data do we have?
	int line = -1;
	int pos = 0;
	bool exp_known = false;
	if (Exp.cur_line){
		line = Exp.cur_line->physical_line;
		if (Exp.cur_exp >= 0){
			exp_known = true;
			pos = Exp.cur_line->exp[Exp.cur_exp].pos;
		}
	}

	if (exp_known){
		// full data
		strcpy(ErrorMsg, string2("\"%s\" %s", cur_name, str));
		strcpy(ErrorMsgExt[0], str);
		strcpy(ErrorMsgExt[1], string2("\"%s\" , line %d:%d", cur_name, line + 1, pos + 1));
		ErrorLine = line;
		ErrorColumn = pos;
	}else{
		strcpy(ErrorMsg, str);
		strcpy(ErrorMsgExt[0], str);
		if (line >= 0)
			strcpy(ErrorMsgExt[1], string2("line %d", line + 1));
		else
			strcpy(ErrorMsgExt[1], "");
		ErrorLine = (line >= 0) ? line : 0;
		ErrorColumn = 0;
	}
	stringout(str);
	stringout(ErrorMsgExt[1]);
	stringout("------------------------------------------------------------------");
	stringout(Filename);
	stringout("\n\n\n");
}

void CScript::DoError(char *str)
{
	pre_script->DoError(str);
	Error = true;
	ErrorLine = pre_script->ErrorLine;
	ErrorColumn = pre_script->ErrorColumn;
	strcpy(ErrorMsgExt[0], pre_script->ErrorMsgExt[0]);
	strcpy(ErrorMsgExt[1], pre_script->ErrorMsgExt[1]);
	strcpy(ErrorMsg, pre_script->ErrorMsg);
}

void CScript::DoErrorLink(char *str)
{
	DoError(str);
	LinkerError = true;
}

inline bool isNumber(char c)
{
	if ((c>=48)&&(c<=57))
		return true;
	return false;
}

inline bool isLetter(char c)
{
	if ((c>='a')&&(c<='z'))
		return true;
	if ((c>='A')&&(c<='Z'))
		return true;
	if ((c=='_'))
		return true;
	// Umlaute
#ifdef HUI_OS_WINDOWS
	// Windows-Zeichensatz
	if ((c=='_')||(c==-28)||(c==-10)||(c==-4)||(c==-33)||(c==-60)||(c==-42)||(c==-36))
		return true;
#endif
#ifdef HUI_OS_LINUX
	// Linux-Zeichensatz??? testen!!!!
#endif
	return false;
}

inline bool isSpacing(char c)
{
	if ((c==' ')||(c=='\t')||(c=='\n'))
		return true;
	return false;
}

inline bool isSign(char c)
{
	if ((c=='.')||(c==':')||(c==',')||(c==';')||(c=='+')||(c=='-')||(c=='*')||(c=='%')||(c=='/')||(c=='=')||(c=='<')||(c=='>')||(c=='\''))
		return true;
	if ((c=='(')||(c==')')||(c=='{')||(c=='}')||(c=='&')||(c=='|')||(c=='!')||(c=='[')||(c==']')||(c=='\"')||(c=='\\')||(c=='#')||(c=='?')||(c=='$'))
		return true;
	return false;
}

int CPreScript::GetKind(char c)
{
	if (isNumber(c))
		return ExpKindNumber;
	else if (isLetter(c))
		return ExpKindLetter;
	else if (isSpacing(c))
		return ExpKindSpacing;
	else if (isSign(c))
		return ExpKindSign;
	else if (c==0)
		return -1;

	char str[8];
	str[0]=c;
	str[1]=0;
	Exp.cur_exp = Exp.cur_line->exp.size();
	strcpy(Exp.cur_line->exp[Exp.cur_exp].name,string2("   '%s' (%s)   ",str,d2h(str,1)));
	DoError("evil character found!");
	return -1;
}

char str_eol[] = "-eol-";

void clear_exp_buffer(ps_exp_buffer_t *e)
{
	e->cur_line = NULL;
	for (int i=0;i<e->line.size();i++)
		e->line[i].exp.clear();
	e->line.clear();
	if (e->buffer){
		dm("Exp.buffer", e->buffer);
		delete[]e->buffer;
		e->buffer = NULL;
	}
	e->cur_line = &e->temp_line;
	e->cur_exp = -1;
	e->comment_level = 0;
}

inline void exp_add_line(ps_exp_buffer_t *e)
{
	ps_line_t l;
	e->line.push_back(l);
	e->cur_line = &e->line[e->line.size()-1];
}

inline void insert_into_buffer(CPreScript *ps, char *name, int pos, int index = -1)
{
	ps_exp_t e;
	e.name = ps->Exp.buf_cur;
	ps->Exp.buf_cur += strlen(name) + 1;
	strcpy(e.name, name);
	e.pos = pos;
	if (index < 0)
		// at the end...
		ps->Exp.cur_line->exp.push_back(e);
	else
		ps->Exp.cur_line->exp.insert(ps->Exp.cur_line->exp.begin() + index, e);
}

inline void remove_from_buffer(CPreScript *ps, int index)
{
	ps->Exp.cur_line->exp.erase(ps->Exp.cur_line->exp.begin() + index);
}		     

void CPreScript::Analyse(char *buffer, bool just_analyse)
{
	msg_db_r("Analyse", 4);
	clear_exp_buffer(&Exp);
	Exp.buffer = new char[strlen(buffer)*2];
	am("Exp.buffer",asm_end - asm_start + 1, Exp.buffer);
	Exp.buf_cur = Exp.buffer;

	reset_indent();

	// scan all lines
	char *buf = buffer;
	for (int i=0;i<SCRIPT_MAX_LINES;i++){
		//exp_add_line(&Exp);
		Exp.cur_line->physical_line = i;
		if (AnalyseLine(buf, Exp.cur_line, i, just_analyse))
			break;
		if (Error){
			msg_db_l(4);
			return;
		}
		buf += Exp.cur_line->length + 1;
	}

	// glue together lines ending with a "\" or ","
	for (int i=0;i<(int)Exp.line.size()-1;i++){
		if ((strcmp(Exp.line[i].exp[Exp.line[i].exp.size() - 1].name, "\\") == 0) || (strcmp(Exp.line[i].exp[Exp.line[i].exp.size() - 1].name, ",") == 0)){
			int d = (strcmp(Exp.line[i].exp[Exp.line[i].exp.size() - 1].name, "\\") == 0) ? 1 : 0;
			// glue... (without \\ but with ,)
			for (int j=d;j<Exp.line[i + 1].exp.size();j++){
				ps_exp_t e;
				e.name = Exp.line[i + 1].exp[j].name;
				e.pos = 0; // Exp.line[i + 1].exp[j].name;
				Exp.line[i].exp.push_back(e);
			}
			// remove line
			Exp.line.erase(Exp.line.begin() + i + 1);
			i --;
			
		}
	}

	/*for (int i=0;i<Exp.line.size();i++){
		msg_write("--------------------");
		msg_write(Exp.line[i].indent);
		for (int j=0;j<Exp.line[i].exp.size();j++)
			msg_write(Exp.line[i].exp[j].name);
	}*/

	
	// safety
	Exp.temp_line.exp.clear();
	Exp.line.push_back(Exp.temp_line);
	for (int i=0;i<Exp.line.size();i++){
		ps_exp_t e;
		e.name = str_eol;
		e.pos = Exp.line[i].length;
		Exp.line[i].exp.push_back(e);
	}
	
	msg_db_l(4);
}

// scan one line
//   true -> end of file
bool CPreScript::AnalyseLine(char *buffer, ps_line_t *l, int &line_no, bool just_analyse)
{
	msg_db_r("AnalyseLine", 4);
	int pos = 0;
	l->indent = 0;
	l->length = 0;
	l->exp.clear();

	for (int i=0;i<SCRIPT_MAX_EXPRESSIONS_PER_LINE;i++){
		if (AnalyseExpression(buffer, pos, l, line_no, just_analyse))
			break;
		if (Error){
			msg_db_l(4);
			return false;
		}
	}
	l->length = pos;
	if (l->exp.size() > 0)
		Exp.line.push_back(*l);
	msg_db_l(4);
	return (buffer[pos] == 0);
}

bool DoMultiLineComment(CPreScript *ps, char *buffer, int &pos)
{
	while(true){
		if (buffer[pos] == '\n')
			return true;
		if ((buffer[pos] == '/') && (buffer[pos + 1] == '*'))
			ps->Exp.comment_level ++;
		if ((buffer[pos] == '*') && (buffer[pos + 1] == '/')){
			ps->Exp.comment_level --;
			if (ps->Exp.comment_level == 0){
				pos += 2;
				return false;
			}
		}
		if ((buffer[pos] == 0)){// || (BufferPos>=BufferLength)){
			ps->DoError("comment exceeds end of file");
			return true;
		}
		pos ++;
	}
	//ExpKind = ExpKindSpacing;
}

void DoAsmBlock(CPreScript *ps, char *buffer, int &pos, int &line_no)
{
	int line_breaks = 0;
	// find beginning
	for (int i=0;i<1024;i++){
		if (buffer[pos] == '{')
			break;
		if ((buffer[pos] != ' ') && (buffer[pos] != '\t') && (buffer[pos] != '\n')){
			ps->DoError("'{' expected after \"asm\"");
			return;
		}
		if (buffer[pos] == '\n')
			line_breaks ++;
		pos ++;
	}
	pos ++;
	int asm_start = pos;
	
	// find end
	for (int i=0;i<65536;i++){
		if (buffer[pos] == '}')
			break;
		if (buffer[pos] == 0){
			ps->DoError("'}' expected to end \"asm\"");
			return;
		}
		if (buffer[pos] == '\n')
			line_breaks ++;
		pos ++;
	}
	int asm_end = pos - 1;
	pos ++;

	sAsmBlock a;
	a.Line = ps->Exp.cur_line->physical_line;
	a.block = new char[asm_end - asm_start + 1];
	am("AsmBlock",asm_end - asm_start + 1, a.block);
	memcpy(a.block, &buffer[asm_start], asm_end - asm_start);
	a.block[asm_end - asm_start] = 0;
	ps->AsmBlock.push_back(a);

	line_no += line_breaks;
}

// scan one line
// starts with <pos> and sets <pos> to first character after the expression
//   true -> end of line (<pos> is on newline)
bool CPreScript::AnalyseExpression(char *buffer, int &pos, ps_line_t *l, int &line_no, bool just_analyse)
{
	msg_db_r("AnalyseExpression", 4);
	// skip whitespace and other "invisible" stuff to find the first interesting character
	if (Exp.comment_level > 0)
		if (DoMultiLineComment(this, buffer, pos))
			_return_(4, true);

	for (int i=0;i<SCRIPT_MAX_LINE_SIZE;i++){
		// end of file
		if (buffer[pos] == 0){
			strcpy(Temp, "");
			_return_(4, true);
		}
		else if (buffer[pos]=='\n'){ // line break
			_return_(4, true);
		}else if (buffer[pos]=='\t'){ // tab
			if (l->exp.size() == 0)
				l->indent ++;
		}else if ((buffer[pos] == '/') && (buffer[pos + 1]=='/')){ // single-line comment
			// skip to end of line
			for (int c=0;c<SCRIPT_MAX_LINE_SIZE;c++){
				pos ++;
				if (buffer[pos] == '\n')
					_return_(4, true);
			}
		}else if ((buffer[pos] == '/') && (buffer[pos + 1] == '*')){ // multi-line comment
			if (DoMultiLineComment(this, buffer, pos))
				_return_(4, true);
			ExpKind = ExpKindSpacing;
		}else if ((buffer[pos] == 'a') && (buffer[pos + 1] == 's') && (buffer[pos + 2] == 'm')){ // asm block
			int pos0 = pos;
			pos += 3;
			DoAsmBlock(this, buffer, pos, line_no);
			if (Error)	_return_(4, true);
			insert_into_buffer(this, "-asm-", pos0);
			_return_(4, true);
		}
		ExpKind = GetKind(buffer[pos]);
		if (Error)
			_return_(4, false);
		if (ExpKind != ExpKindSpacing)
			break;
		pos ++;
	}
	int TempLength = 0;
	//int ExpStart=BufferPos;

	// string
	if (buffer[pos] == '\"'){
		msg_db_m("string", 1);
		for (int i=0;i<SCRIPT_MAX_LINE_SIZE;i++){
			char c = Temp[TempLength ++] = buffer[pos ++];
			// end of string?
			if ((c == '\"') && (i > 0))
				break;
			else if (c == '\n'){
				_do_error_("string exceeds line", 4, false);
			}else{
				// escape sequence
				if (c == '\\'){
					if (buffer[pos] == '\\')
						Temp[TempLength - 1] = '\\';
					else if (buffer[pos] == '\"')
						Temp[TempLength - 1] = '\"';
					else if (buffer[pos] == 'n')
						Temp[TempLength - 1] = '\n';
					else if (buffer[pos] == 'r')
						Temp[TempLength - 1] = '\r';
					else if (buffer[pos] == 't')
						Temp[TempLength - 1] = '\t';
					else if (buffer[pos] == '0')
						Temp[TempLength - 1] = '\0';
					else
						_do_error_("unknown escape in string", 4, false);
					pos ++;
				}
				continue;
			}
		}

	// macro
	}else if ((buffer[pos] == '#') && (GetKind(buffer[pos + 1]) == ExpKindLetter)){
		msg_db_m("macro", 4);
		for (int i=0;i<SCRIPT_MAX_NAME;i++){
			int kind = GetKind(buffer[pos]);
			// may contain letters and numbers
			if ((i > 0) && (kind != ExpKindLetter) && (kind != ExpKindNumber))
				break;
			Temp[TempLength ++] = buffer[pos ++];
		}

	// character
	}else if (buffer[pos] == '\''){
		msg_db_m("char", 4);
		Temp[TempLength ++] = buffer[pos ++];
		Temp[TempLength ++] = buffer[pos ++];
		Temp[TempLength ++] = buffer[pos ++];
		if (Temp[TempLength - 1] != '\'')
			_do_error_("character constant should end with '''", 4, false);

	// word
	}else if (ExpKind == ExpKindLetter){
		msg_db_m("word", 4);
		for (int i=0;i<SCRIPT_MAX_NAME;i++){
			int kind = GetKind(buffer[pos]);
			// may contain letters and numbers
			if ((kind != ExpKindLetter) && (kind != ExpKindNumber))
				break;
			Temp[TempLength ++] = buffer[pos ++];
		}

	// number
	}else if (ExpKind == ExpKindNumber){
		msg_db_m("num", 4);
		bool hex = false;
		for (int i=0;i<SCRIPT_MAX_LINE_SIZE;i++){
			char c = Temp[TempLength] = buffer[pos];
			// "0x..." -> hexadecimal
			if ((i == 1) && (Temp[0] == '0') && (Temp[1] == 'x'))
				hex = true;
			int kind = GetKind(c);
			if (hex){
				if ((i > 1) && (kind != ExpKindNumber) && ((c < 'a') || (c > 'f')))
					break;
			}else{
				// may contain numbers and '.' or 'f'
				if ((kind != ExpKindNumber) && (c != '.'))// && (c != 'f'))
					break;
			}
			TempLength ++;
			pos ++;
		}

	// symbol
	}else if (ExpKind == ExpKindSign){
		msg_db_m("sym", 4);
		// mostly single-character symbols
		char c = Temp[TempLength ++] = buffer[pos ++];
		// double-character symbol
		if (((c == '=') && (buffer[pos] == '=')) || // ==
			((c == '!') && (buffer[pos] == '=')) || // !=
			((c == '<') && (buffer[pos] == '=')) || // <=
			((c == '>') && (buffer[pos] == '=')) || // >=
			((c == '+') && (buffer[pos] == '=')) || // +=
			((c == '-') && (buffer[pos] == '=')) || // -=
			((c == '*') && (buffer[pos] == '=')) || // *=
			((c == '/') && (buffer[pos] == '=')) || // /=
			((c == '+') && (buffer[pos] == '+')) || // ++
			((c == '-') && (buffer[pos] == '-')) || // --
			((c == '&') && (buffer[pos] == '&')) || // &&
			((c == '|') && (buffer[pos] == '|')) || // ||
			((c == '<') && (buffer[pos] == '<')) || // <<
			((c == '>') && (buffer[pos] == '>')) || // >>
			((c == '+') && (buffer[pos] == '+')) || // ++
			((c == '-') && (buffer[pos] == '-')) || // --
			((c == '-') && (buffer[pos] == '>'))) // ->
				Temp[TempLength ++] = buffer[pos ++];
	}

	Temp[TempLength] = 0;
	insert_into_buffer(this, Temp, pos - TempLength);

	msg_db_l(4);
	return (buffer[pos] == '\n');
}

bool IsIfDefed(int &num_ifdefs,bool *defed)
{
	for (int i=0;i<num_ifdefs;i++)
		if (!defed[i])
			return false;
	return true;
}

CScript *cur_script;
void CreateAsmMetaInfo(CPreScript* ps)
{
	msg_db_r("CreateAsmMetaInfo",5);
	//msg_error("zu coden: CreateAsmMetaInfo");
	sAsmMetaInfo *m=(sAsmMetaInfo*)ps->AsmMetaInfo;
	if (!m){
		m=new sAsmMetaInfo;
		ps->AsmMetaInfo = (char*)m;
		am("AsmMetaInfo",sizeof(sAsmMetaInfo),ps->AsmMetaInfo);
		m->Mode16 = ps->FlagCompileInitialRealMode;
		m->CodeOrigin=0; // FIXME:  &Opcode[0] ????
	}
	m->Opcode=cur_script->Opcode;
	m->GlobalVar.clear();
	for (int i=0;i<ps->RootOfAllEvil.Var.size();i++){
		sAsmGlobalVar v;
		v.Name = ps->RootOfAllEvil.Var[i].Name;
		v.Pos = cur_script->g_var[i];
		m->GlobalVar.push_back(v);
	}
	msg_db_l(5);
}

// import data from an included script file
void CPreScript::AddIncludeData(CScript *s)
{
	msg_db_r("AddIncludeData",5);
	Include.push_back(s);
	CPreScript *ps=s->pre_script;

	// defines
	for (int i=0;i<ps->Define.size();i++)
		Define.push_back(ps->Define[i]);

	// structs
	for (int i=0;i<ps->Struct.size() - PreStruct.size();i++)
		Struct.push_back(ps->Struct[i + PreStruct.size()]);

	// types
	for (int i=0;i<ps->Type.size() - PreType.size();i++)
		Type.push_back(ps->Type[i + PreType.size()]);

	// enums
	for (int i=0;i<ps->Enum.size();i++)
		Enum.push_back(ps->Enum[i]);
	msg_db_l(5);
}

enum{
	MacroInclude,
	MacroDefine,
	MacroIfdef,
	MacroIfndef,
	MacroEndif,
	MacroElse,
	MacroDisasm,
	MacroShow,
	MacroImmortal,
	MacroRule,
	MacroOs,
	MacroInitialRealmode,
	MacroVariablesOffset,
	MacroCodeOrigin,
	NumMacroNames
};

char MacroName[NumMacroNames][SCRIPT_MAX_NAME]=
{
	"#include",
	"#define",
	"#ifdef",
	"#ifndef",
	"#endif",
	"#else",
	"#disasm",
	"#show",
	"#immortal",
	"#rule",
	"#os",
	"#initial_realmode",
	"#variables_offset",
	"#code_origin"
};		

#if 0
void CPreScript::MakeExps(char *Buffer,bool just_ana				)
{
	msg_db_r("MakeExps",4);
	int i,NumIfDefs=0,ln;
	bool IfDefed[1024];
	Exp=new exp_buffer;
	am("exp_buffer",sizeof(exp_buffer),Exp);
	Exp->BufferUsed=0;
	BufferPos=0;
	Exp->TempLine=1;
	Exp->TempColumn=0;
	Exp->NumExps=0;
	char filename[256];
	CScript *include;
	
	while(true){
		int l=(Exp->NumExps==0)?0:Exp->TempLine;
		NextExp(Buffer);
		if (Error)	return;
		if (Temp[0]==0)
			break;

		if ((Exp->TempLine>l)&&(strcmp(Temp,"#")==0)){
			msg_db_m("makro",4);
			l=Exp->TempLine;
			so("# -Makro");
			NextExp(Buffer);

			int macro_no=-1;
			for (i=0;i<NumMacroNames;i++)
				if (strcmp(Temp,MacroName[i])==0)
					macro_no=i;

			switch(macro_no){
				case MacroInclude:
					NextExp(Buffer);
					if (!IsIfDefed(NumIfDefs,IfDefed))
						continue;
					strcpy(filename,dir_from_filename(Filename));
					strcat(filename,&Temp[1]);
					filename[strlen(filename)-1]=0; // remove "
					strcpy(filename,filename_no_recursion(filename));

					so("lade Include-Datei");
					right();

					include=LoadScriptAsInclude(filename,just_analyse);

					left();
					if ((!include)||(include->Error)){
						IncludeLinkerError|=include->LinkerError;
						DoError(string2("error in inluded file \"%s\":\n[ %s (line %d:) ]",filename,include->ErrorMsg,include->ErrorLine,include->ErrorColumn),Exp->ExpNr);
						return;
					}
					AddIncludeData(include);
					Exp->ExpNr++;
					break;
				case MacroDefine:
					Define[NumDefines]=new sDefine;
					am("Define",sizeof(sDefine),Define[NumDefines]);
					Define[NumDefines]->Owner=this;
					// Source
					NextExp(Buffer);
					strcpy(Define[NumDefines]->Source,Temp);
					Define[NumDefines]->NumDests=0;
					// Dests
					int t;
					for (i=0;i<SCRIPT_MAX_DEFINE_DESTS;i++){
						t=BufferPos;
						NextExp(Buffer);
						if (Exp->TempLine>l){
							BufferPos=t;
							break;
						}
						strcpy(Define[NumDefines]->Dest[Define[NumDefines]->NumDests],Temp);
						Define[NumDefines]->NumDests++;
					}
					Exp->TempLine=l;
					NumDefines++;
					break;
				case MacroIfdef:
					NextExp(Buffer);
					IfDefed[NumIfDefs]=false;
					for (i=0;i<NumDefines;i++)
						if (strcmp(Temp,Define[i]->Source)==0){
							IfDefed[NumIfDefs]=true;
							break;
						}
					NumIfDefs++;
					break;
				case MacroIfndef:
					NextExp(Buffer);
					IfDefed[NumIfDefs]=true;
					for (i=0;i<NumDefines;i++)
						if (strcmp(Temp,Define[i]->Source)==0){
							IfDefed[NumIfDefs]=false;
							break;
						}
					NumIfDefs++;
					break;
				case MacroElse:
					if (NumIfDefs<1){
						strcpy(Exp->Name[Exp->NumExps],Temp);
						Exp->Line[Exp->NumExps]=Exp->TempLine;
						Exp->Column[Exp->NumExps]=Exp->TempColumn;
						DoError("\"#else\" found but no matching \"#ifdef\"",Exp->NumExps);
						return;
					}
					IfDefed[NumIfDefs-1]=!IfDefed[NumIfDefs-1];
					break;
				case MacroEndif:
					if (NumIfDefs<1){
						strcpy(Exp->Name[Exp->NumExps],Temp);
						Exp->Line[Exp->NumExps]=Exp->TempLine;
						Exp->Column[Exp->NumExps]=Exp->TempColumn;
						DoError("\"#endif\" found but no matching \"#ifdef\"",Exp->NumExps);
						return;
					}
					NumIfDefs--;
					break;
				case MacroRule:
					NextExp(Buffer);
					ln=-1;
					for (i=0;i<NumScriptLocations;i++)
						if (strcmp(ScriptLocation[i].Name,Temp)==0)
							ln=i;
					if (ln<0){
						strcpy(Exp->Name[Exp->NumExps],Temp);
						Exp->Line[Exp->NumExps]=Exp->TempLine;
						Exp->Column[Exp->NumExps]=Exp->TempColumn;
						DoError("unknown location in script rule",Exp->NumExps);
						return;
					}
					PreScriptRule[NumPreScriptRules]=new sPreScriptRule;
					am("PreScriptRule",sizeof(sPreScriptRule),PreScriptRule[NumPreScriptRules]);
					PreScriptRule[NumPreScriptRules]->Location=ScriptLocation[ln].Location;
					NextExp(Buffer);
					PreScriptRule[NumPreScriptRules]->Level=s2i(Temp);
					NextExp(Buffer);
					Temp[strlen(Temp)-1]=0;
					strcpy(PreScriptRule[NumPreScriptRules]->Name,&Temp[1]);
					NumPreScriptRules++;
					break;
				case MacroDisasm:
					FlagDisassemble=true;
					break;
				case MacroShow:
					FlagShow=true;
					break;
				case MacroImmortal:
					FlagImmortal=true;
					break;
				case MacroOs:
					FlagCompileOS=true;
					break;
				case MacroInitialRealmode:
					FlagCompileInitialRealMode=true;
					break;
				case MacroVariablesOffset:
					FlagOverwriteVariablesOffset=true;
					NextExp(Buffer);
					VariablesOffset=s2i2(Temp);
					break;
				case MacroCodeOrigin:
					NextExp(Buffer);
					CreateAsmMetaInfo(this);
					((sAsmMetaInfo*)AsmMetaInfo)->CodeOrigin=s2i2(Temp);
					break;
				default:
					strcpy(Exp->Name[Exp->NumExps],Temp);
					Exp->Line[Exp->NumExps]=Exp->TempLine;
					Exp->Column[Exp->NumExps]=Exp->TempColumn;
					DoError("unknown makro atfer \"#\"",Exp->NumExps);
					return;
			}
			continue;
		}
	//msg_db_m("def",4);

		bool defed=false;
		for (i=0;i<NumDefines;i++)
			if (strcmp(Temp,Define[i]->Source)==0){
				defed=true;
				for (int j=0;j<Define[i]->NumDests;j++){
					strcpy(Exp->Name[Exp->NumExps],Define[i]->Dest[j]);
					Exp->BufferUsed+=strlen(Define[i]->Dest[j])+1;
					Exp->Name[Exp->NumExps+1]=&Exp->Buffer[Exp->BufferUsed];
					Exp->Line[Exp->NumExps]=Exp->TempLine;
					Exp->Column[Exp->NumExps]=Exp->TempColumn;
					Exp->NumExps++;
				}
				break;
			}
			
	//msg_db_m("postdef",4);
		if (defed)
			continue;

		if (!IsIfDefed(NumIfDefs,IfDefed))
			continue;
	//msg_db_m("zzz",4);

		strcpy(Exp->Name[Exp->NumExps],Temp);
		Exp->Line[Exp->NumExps]=Exp->TempLine;
		Exp->Column[Exp->NumExps]=Exp->TempColumn;
		Exp->NumExps++;
	}
	if (NumIfDefs>0){
		DoError("\"#ifdef\" found but no matching \"#endif\"",Exp->NumExps);
		return;
	}
	Exp->ExpNr=0;
	msg_db_l(4);
}
#endif

bool next_extern = false;

int CPreScript::AddVar(char *name, sType *type, sFunction *f)
{
/*	// "extern" variable -> link to main program
	if ((next_extern) && (f == &RootOfAllEvil)){
		for (int i=NumTruePreExternalVars;i<PreGlobalVar.size();i++)
			if (strcmp(name, PreGlobalVar[i].Name) == 0){
				f->Var[i].Type = type;
				return i;
			}
	}
should be done somwhere else (ParseVariableDefSingle) */
	so("                                AddVar");
	sLocalVariable v;
	strcpy(v.Name,name);
	int s=mem_align(type->Size);
	if (f->VarSize >= 0){ // "real" local variable
		v.Offset = -f->VarSize - s;
		f->VarSize += s;
	}else{ // as a parameter
		v.Offset = f->ParamSize;
		f->ParamSize += s;
	}
	v.Type = type;
	f->Var.push_back(v);
	return f->Var.size() - 1;
}

// constants

int CPreScript::AddConstant(sType *type)
{
	so("                                AddConstant");
	//	am("Constant",sizeof(sConstant),c);
	Constant.resize(Constant.size() + 1);
	sConstant *c = &Constant[Constant.size() - 1];
	c->type = type;
	int s = (type->Size>PointerSize) ? type->Size : PointerSize;
	c->data = new char[s];
	am("Constant->data",s,c->data);
	return Constant.size() - 1;
}

int CPreScript::AddBlock()
{
	so("AddBlock");
	//am("Block",sizeof(sBlock),Block[NumBlocks]);
	Block.resize(Block.size() + 1);
	sBlock *b = &Block[Block.size() - 1];
	b->Nr = Block.size() - 1;
	b->RootNr = -1;
	return Block.size() - 1;
}

// functions

int CPreScript::AddFunction(char *name)
{
	so("AddFunction");
	//am("Function",sizeof(sFunction),Function[NumFunctions]);
	Function.resize(Function.size() + 1);
	sFunction *f = &Function[Function.size() - 1];
	strcpy(f->Name, name);
	f->Block = &Block[AddBlock()];
	if (Error)
		return -1;
	f->NumParams = 0;
	f->Var.clear();
	f->VarSize = -1;
	f->ParamSize = 8; // space for return value and eBP
	f->Type = TypeVoid;
	return Function.size() - 1;
}

int CPreScript::AddCommand()
{
	//am("Command",sizeof(sCommand),Command[NumCommands]);
	so("AddCommand");
	Command.resize(Command.size() + 1);
	sCommand *c = &Command[Command.size() - 1];
	c->ReturnType = TypeVoid;
	c->Kind = KindUnknown;
	c->NumParams = 0;
	return Command.size() - 1;
}

inline int command_from_link(CPreScript *ps, sLinkData *link)
{
	int ncmd = ps->AddCommand();
	sCommand *cmd = &ps->Command[ncmd];
	cmd->Kind = link->Kind;
	cmd->LinkNr = link->Nr;
	cmd->script = link->script;
	cmd->ReturnType = link->type;
	if (link->Kind == KindFunction){
		if (cmd->script)
			cmd->NumParams = cmd->script->pre_script->Function[link->Nr].NumParams;
		else
			cmd->NumParams = ps->Function[link->Nr].NumParams;
	}else if (link->Kind == KindCompilerFunction){
		ps->SetCompilerFunction(cmd->LinkNr, cmd);
	}
	return ncmd;
}

inline int add_command_compilerfunc(CPreScript *ps, int cf)
{
	int ncmd = ps->AddCommand();
	ps->SetCompilerFunction(cf, &ps->Command[ncmd]);
	return ncmd;
}

int CPreScript::WhichPrimitiveOperator(char *name)
{
	for (int i=0;i<NumPrimitiveOperators;i++)
		if (strcmp(name, PrimitiveOperator[i].Name) == 0)
			return i;
	return -1;
}

int CPreScript::WhichExternalVariable(char *name)
{
	// wrong order -> "extern" varbiables are dominant...
	for (int i=PreExternalVar.size()-1;i>=0;i--)
		if (strcmp(name, PreExternalVar[i].Name) == 0)
			return i;

	return -1;
}

int CPreScript::WhichType(char *name)
{
	for (int i=0;i<Type.size();i++)
		if (strcmp(name, Type[i]->Name) == 0)
			return i;

	return -1;
}

int CPreScript::WhichCompilerFunction(char *name)
{
	for (int i=0;i<PreCommand.size();i++)
		if (strcmp(name, PreCommand[i].Name) == 0)
			return i;
	return -1;
}

bool CPreScript::GetExistence(char *name,sFunction *f)
{
	msg_db_r("GetExistence", 2);
	sFunction *lf=f;
	GetExistenceLink.type = TypeUnknown;
	GetExistenceLink.Meta = NULL;
	GetExistenceLink.script = NULL;

	// first test local variables
	if (lf){
		sLocalVariable *v;
		foreach(f->Var, v, i){
			if (strcmp(v->Name, name) == 0){
				GetExistenceLink.type = v->Type;
				GetExistenceLink.Nr = i;
				GetExistenceLink.Kind = KindVarLocal;
				msg_db_l(2);
				return true;
			}
		}
	}

	// then global variables (=local variables in "RootOfAllEvil")
	lf=&RootOfAllEvil;
	for (int i=0;i<lf->Var.size();i++)
		if (strcmp(lf->Var[i].Name,name)==0){
			GetExistenceLink.type=lf->Var[i].Type;
			GetExistenceLink.Nr=i;
			GetExistenceLink.Kind=KindVarGlobal;
			msg_db_l(2);
			return true;
		}

	// schliesslich die externen-Variablen
	int w=WhichExternalVariable(name);
	if (w>=0){
		SetExternalVariable(w,GetExistenceLink);
		msg_db_l(2);
		return true;
	}

	// in den Include-Dateien (nur global)...
	for (int i=0;i<Include.size();i++){
		if (Include[i]->pre_script->GetExistence(name,NULL)){
			if (Include[i]->pre_script->GetExistenceLink.script) // nicht rekursiv!!!
				continue;
			//msg_error(string("\"",name,"\" in Include gefunden!"));
			memcpy(&GetExistenceLink,&(Include[i]->pre_script->GetExistenceLink),sizeof(sLinkData));
			GetExistenceLink.script=Include[i];
			msg_db_l(2);
			return true;
		}
	}

	// dann die Funktionen
	for (int i=0;i<Function.size();i++)
		if (strcmp(Function[i].Name,name)==0){
			GetExistenceLink.type=Function[i].Type;
			GetExistenceLink.Nr=i;
			GetExistenceLink.Kind=KindFunction;
			msg_db_l(2);
			return true;
		}

	// dann die Compiler-Funktionen
	w = WhichCompilerFunction(name);
	if (w >= 0){
		GetExistenceLink.type = PreCommand[w].ReturnType;
		GetExistenceLink.Nr = w;
		GetExistenceLink.Kind = KindCompilerFunction;
		msg_db_l(2);
		return true;
	}

	// die Operatoren
	w=WhichPrimitiveOperator(name);
	if (w>=0){
		GetExistenceLink.Nr=w;
		GetExistenceLink.Kind=KindPrimitiveOperator;
		msg_db_l(2);
		return true;
	}

	// Typen
	w=WhichType(name);
	if (w>=0){
		GetExistenceLink.Nr=w;
		GetExistenceLink.Kind=KindType;
		msg_db_l(2);
		return true;
	}

	// Name nicht bekannt
	GetExistenceLink.type = TypeUnknown;
	GetExistenceLink.Nr = 0;
	GetExistenceLink.Kind = 0;
	msg_db_l(2);
	return false;
}

void CPreScript::SetCompilerFunction(int CF, sCommand *Com)
{
	msg_db_r("SetCompilerFunction", 4);
// a function the compiler knows
	Com->Kind = KindCompilerFunction;
	Com->LinkNr = CF;
	Com->NumParams = 0;
	Com->ReturnType = TypeVoid;

	Com->NumParams = PreCommand[CF].Param.size();
	for (int p=0;p<Com->NumParams;p++)
		Com->ParamLink[p].type = PreCommand[CF].Param[p].Type;
	Com->ReturnType = PreCommand[CF].ReturnType;
			
	msg_db_l(4);
}

#define is_variable(kind)	((kind == KindVarLocal) || (kind == KindVarGlobal) || (kind == KindVarExternal))

void CPreScript::SetExternalVariable(int gv, sLinkData &link)
{
	link.Kind = KindVarExternal;
	link.Nr = gv;
	link.type = PreExternalVar[gv].Type;
}

// find the type of a (potential) constant
//  "1.2" -> float
sType *CPreScript::GetConstantType(char *name)
{
	msg_db_r("GetConstantType", 4);
	PreConstantNr=-1;
	EnumNr=-1;

	// predefined constants
	for (PreConstantNr=0;PreConstantNr<PreConstant.size();PreConstantNr++)
		if (strcmp(name, PreConstant[PreConstantNr].Name) == 0)
			_return_(4, PreConstant[PreConstantNr].Type);
	PreConstantNr = -1;

	// enum
	for (EnumNr=0;EnumNr<Enum.size();EnumNr++)
		if (strcmp(name,Enum[EnumNr].Name)==0)
			_return_(4, TypeInt);
	EnumNr=-1;

	// character "..."
	if ((name[0]=='\'')&&(name[strlen(name)-1]=='\''))
		_return_(4, TypeChar);

	// string "..."
	if ((name[0]=='"')&&(name[strlen(name)-1]=='"'))
		_return_(4, TypeString);

	// numerical (int/float)
	sType *type = TypeInt;
	bool hex = (name[0] == '0') && (name[1] == 'x');
	for (unsigned int c=0;c<strlen(name);c++)
		if ((name[c] < '0') || (name[c] > '9'))
			if (hex){
				if ((c >= 2) && (name[c] < 'a') && (name[c] > 'f'))
					_return_(4, TypeUnknown);
			}else if (name[c] == '.')
				type = TypeFloat;
			else{
				if ((type != TypeFloat) || (name[c] != 'f')) // f in floats erlauben
					if ((c != 0) || (name[c] != '-')) // Vorzeichen erlauben
						_return_(4, TypeUnknown);
			}
	_return_(4, type);
}

static int _some_int_;
static float _some_float_;
static char _some_string_[2048];

void *CPreScript::GetConstantValue(char *name)
{
	sType *type = GetConstantType(name);
// named constants
	if (PreConstantNr >= 0){
		if ((type->Size > 4) && (!type->IsPointer))
			return PreConstant[PreConstantNr].Value;
		else
			return &PreConstant[PreConstantNr].Value;
	}
	if (EnumNr >= 0)
		return &Enum[EnumNr].Value;
// literal
	if (type == TypeChar){
		_some_int_ = name[1];
		return &_some_int_;
	}
	if (type == TypeString){
		for (unsigned int ui=0;ui<strlen(name) - 2;ui++)
				_some_string_[ui] = name[ui+1];
		_some_string_[strlen(name) - 2] = 0;
		return _some_string_;
	}
	if (type == TypeInt){
		_some_int_ = s2i2(name);
		return &_some_int_;
	}
	if (type == TypeFloat){
		_some_float_ = s2f(name);
		return &_some_float_;
	}
	return NULL;
}

// expression naming a type
sType *CPreScript::GetType(int &ie,bool force)
{
	sType *type=NULL;
	for (int i=0;i<Type.size();i++)
		if (strcmp(get_name(ie), Type[i]->Name)==0)
			type=Type[i];
	if (force){
		if (!type){
			Exp.cur_exp = ie;
			DoError("unknown type");
		}
	}
	if (type)
		ie++;
	return type;
}

// create a new type?
void CPreScript::AddType(sType **type)
{
	for (int i=0;i<Type.size();i++)
		if (strcmp((*type)->Name,Type[i]->Name)==0){
			(*type)=Type[i];
			return;
		}
	sType *t = new sType;
	am("Type",sizeof(sType),t);
	(*t)=(**type);
	t->Owner=this;
	strcpy(t->Name,(*type)->Name);
	so(string("AddType: ",t->Name));
	(*type)=t;
	Type.push_back(t);
}

sType *CPreScript::CreateNewType(char *name, int size, bool is_pointer, bool is_silent, int array_size, sType *sub)
{
	sType nt, *pt = &nt;
	nt.ArrayLength = array_size;
	nt.IsPointer = is_pointer;
	nt.IsSilent = is_silent;
	strcpy(nt.Name, name);
	nt.Size = size;
	nt.SubType = sub;
	AddType(&pt);
	return pt;
}

#define CreatePointerType(sub)	CreateNewType(string(sub->Name, "*"), PointerSize, true, false, 0, sub)

inline sLinkData *add_link_data(CPreScript *ps)
{
	ps->LinkData.resize(ps->LinkData.size() + 1);
	return &ps->LinkData[ps->LinkData.size()-1];
}

void DoClassFunction(CPreScript *ps, sLinkData *Operand, sStruct *s, int f_no, sFunction *f)
{
	msg_db_r("DoClassFunc", 1);
	switch(Operand->Kind){
		case KindVarLocal:
		case KindVarGlobal:
		case KindVarExternal:
		case KindVarTemp:
		case KindConstant:
		/*case KindRefToLocal:
		case KindRefToGlobal:
		case KindRefToConst:
		case KindPointerShift:
		case KindDerefPointerShift:*/
			break;
		default:
			ps->DoError("class functions only allowed for object variables");
			_return_(1,);
	}

	// create a command for the object
	int cmd = command_from_link(ps, Operand);
	//ps->ShowCommand(cmd);

	//msg_write(LinkNr2Str(ps, Operand->Kind, Operand->Nr));

	// the function
	sLinkData link;
    link.Kind = KindCompilerFunction;
	link.Nr = s->Function[f_no].cmd;
	link.type = PreCommand[s->Function[f_no].cmd].ReturnType;
	ps->GetFunctionCall(PreCommand[s->Function[f_no].cmd].Name, Operand, &link, f);
	ps->Command[Operand->Nr].SubLink1 = cmd;

	
//	ps->DoError("class functions not implemented yet  ...");
	msg_db_l(1);
}

// find any ".", "->", or "[...]"'s    or operators?
void CPreScript::GetOperandExtension(sLinkData *Operand, sFunction *f)
{
	msg_db_r("GetOperandExtension", 4);

	// nothing?
	int op = WhichPrimitiveOperator(cur_name);
	if ((strcmp(cur_name, ".") != 0) && (strcmp(cur_name, "[") !=0 ) && (strcmp(cur_name, "->") != 0) && (op < 0)){
		msg_db_l(4);
		return;
	}
	sLinkData link, temp;

	// struct element?
	if ((strcmp(cur_name, ".") == 0) || (strcmp(cur_name, "->") == 0)){
		so("->Struktur");
		next_exp();
		link.Kind = KindPointerShift;
		sType *type = Operand->type;

		// pointer -> dereference
		if (type->IsPointer){
			link.Kind = KindDerefPointerShift;
			type = type->SubType;
		}

		if (strcmp(get_name(Exp.cur_exp-1), "->") == 0){
			DoError("\"->\" deprecated,  use \".\" instead");
			msg_db_l(4);
			return;
		}

		// find element
		bool ok = false;
		for (int i=0;i<Struct.size();i++)
			if (type == Struct[i].RootType){
				for (int e=0;e<Struct[i].Element.size();e++)
					if (strcmp(cur_name, Struct[i].Element[e].Name) == 0){
						link.Nr = Struct[i].Element[e].Offset;
						link.type = Struct[i].Element[e].Type;
						ok = true;
						break;
					}
				break;
			}
		
		if (!ok){

			// class function?
			for (int i=0;i<Struct.size();i++)
				if (type == Struct[i].RootType){
					for (int e=0;e<Struct[i].Function.size();e++)
						if (strcmp(cur_name, Struct[i].Function[e].Name) == 0){
							next_exp();
							DoClassFunction(this, Operand, &Struct[i], e, f);
							//DoError(string("class functions not implemented yet  ...",Type2Str(this,type)));
							msg_db_l(4);
							return;
						}
					break;
				}
			
			DoError(string("unknown element of ",Type2Str(this,type)));
			msg_db_l(4);
			return;
		}

		// linking
		next_exp();
		temp = (*Operand);
		(*Operand) = link;
		Operand->Meta = add_link_data(this);
		(*Operand->Meta) = temp;

	// array?
	}else if (strcmp(cur_name, "[") == 0){
		so("->Array");

		// allowed?
		if ((Operand->type->ArrayLength < 1) && (!Operand->type->IsPointer)){
			DoError(string2("type \"%s\" is neither an array nor a pointer", Operand->type->Name));
			msg_db_l(4);
			return;
		}
		next_exp();

		// pointer?
		so(Operand->type->Name);
		if (Operand->type->IsPointer){
			link.Kind = KindPointerAsArray;
			so("  ->Pointer-Array");
		}else
			link.Kind = KindArray;

		// array index...
		sLinkData index = GetCommand(f);
		if (Error){
			msg_db_l(4);
			return;
		}
		if (index.type != TypeInt){
			Exp.cur_exp --;
			DoError(string2("type of index for an array needs to be (int), not (%s)", index.type->Name));
			msg_db_l(4);
			return;
		}
		if (strcmp(cur_name, "]") != 0){
			DoError("\"]\" expected after array index");
			msg_db_l(4);
			return;
		}
		next_exp();

		// linking
		temp = (*Operand);
		so(temp.type->Name);
		(*Operand) = link;
		Operand->Meta = add_link_data(this);
		Operand->type = temp.type->SubType;
		(*Operand->Meta) = temp;
		Operand->ParamLink = add_link_data(this);
		(*Operand->ParamLink) = index;

	// unary operator?
	}else if (op >= 0){
		for (int i=0;i<PreOperator.size();i++)
			if (PreOperator[i].PrimitiveID == op)
				if ((PreOperator[i].ParamType1 == Operand->type) && (PreOperator[i].ParamType2 == TypeVoid)){
					//DoError("Unaerer Operator",ie);
					so("  => unaerer Operator");
					so(LinkNr2Str(this,KindOperator,i));
					int nc = AddCommand();
					//CommandMakeOperator();
					Command[nc].Kind=KindOperator;
					Command[nc].LinkNr=i;
					Command[nc].NumParams=1;
					Command[nc].ParamLink[0] = *Operand;
					Command[nc].ReturnType=PreOperator[i].ReturnType;
					Operand->Kind=KindCommand;
					Operand->Nr=nc;
					Operand->type=PreOperator[i].ReturnType;
					next_exp();
					msg_db_l(4);
					return;
				}
		msg_db_l(4);
		return;
	}

	// recursion
	GetOperandExtension(Operand, f);
	msg_db_l(4);
}

inline bool direct_type_match(sType *a, sType *b)
{
	return ( (a==b) || ( (a->IsPointer) && (b->IsPointer) ) );
}

bool CPreScript::GetSpecialFunctionCall(char *f_name, sLinkData *Operand, sLinkData *link, sFunction *f)
{
	msg_db_r("GetSpecialFuncCall", 4);

	// sizeof
	if ((link->Kind == KindCompilerFunction) && (link->Nr == CommandSizeof)){
		so("sizeof");
		next_exp();
		int nc = AddConstant(TypeInt);
		Operand->Kind = KindConstant;
		Operand->Nr = nc;
		Operand->type = TypeInt;
		
		int nt = WhichType(cur_name);
		sType *type;
		if (nt >= 0)
			(*(int*)(Constant[nc].data)) = Type[nt]->Size;
		else if ((GetExistence(cur_name, f)) && ((GetExistenceLink.Kind == KindVarGlobal) || (GetExistenceLink.Kind == KindVarLocal) || (GetExistenceLink.Kind == KindVarExternal)))
			(*(int*)(Constant[nc].data)) = GetExistenceLink.type->Size;
		else if (type = GetConstantType(cur_name))
			(*(int*)(Constant[nc].data)) = type->Size;
		else{
			DoError("type-name or variable name expected in sizeof(...)");
			msg_db_l(4);
			return false;
		}
		next_exp();
		if (strcmp(cur_name, ")") != 0){
			DoError("\")\" expected after parameter list");
			msg_db_l(4);
			return false;
		}
		next_exp();
		
		so(*(int*)(Constant[nc].data));
		msg_db_l(4);
		return true;
	}

	// sizeof
	if ((link->Kind == KindCompilerFunction) && (link->Nr == CommandReturn)){
		DoError("return");
	}
	
	msg_db_l(4);
	return false;
}
void CPreScript::FindFunctionSingleParameter(int p, sType **WantedType, sFunction *f, int cmd, int fnc)
{
	msg_db_r("FindFuncSingleParam", 4);
	sLinkData Param = GetCommand(f);
	if (Error)
		_return_(4,);

	if (Param.type->ArrayLength > 0){
		sLinkData Meta;
		Meta = Param;
		//memcpy(&Meta,&Param,sizeof(sLinkData));
		Param.type = CreateNewType(string(Param.type->Name, "*"), PointerSize, true, false, 0, Param.type);
		Param.Kind = KindReference;
		Param.Meta = add_link_data(this);
		(*Param.Meta) = Meta;
		so("C-Standart:  Arrays als Parameter werden referenziert!");
	}

	WantedType[p] = Command[cmd].ParamLink[p].type;
	if (fnc >= 0){
		if (Command[cmd].script)
			WantedType[p] = Command[cmd].script->pre_script->Function[fnc].Var[p].Type;
		else
			WantedType[p] = Function[fnc].Var[p].Type;
	}
	// link parameters
	Command[cmd].ParamLink[p] = Param;
	msg_db_l(4);
}

void CPreScript::FindFunctionParameters(int &np, sType **WantedType, sFunction *f, int cmd, int fnc)
{
	if (strcmp(cur_name, "(") != 0){
		DoError("\"(\" expected in front of function parameter list");
		return;
	}
	msg_db_r("FindFunctionParameters", 4);
	next_exp();
		    
	// list of parameters
	np = 0;
	for (int p=0;p<SCRIPT_MAX_PARAMS;p++){
		if (strcmp(cur_name, ")") == 0)
			break;
		np ++;
		// find parameter

		FindFunctionSingleParameter(p, WantedType, f, cmd, fnc);
		if (Error)
			_return_(4,);

		if (strcmp(cur_name, ",") != 0){
			if (strcmp(cur_name, ")") == 0)
				break;
			DoError("\",\" or \")\" expected after parameter for function");
			_return_(4,);
		}
		next_exp();
	}
	next_exp(); // ')'
	msg_db_l(4);
}

void apply_type_cast(CPreScript *ps, int tc, sLinkData *param);

// check, if the command <link> links to really has type <type>
//   ...and try to cast, if not
void CPreScript::CheckParamLink(sLinkData *link, sType *type, char *f_name, int param_no)
{
	msg_db_r("CheckParamLink", 4);
	// type cast needed and possible?
	sType *pt = link->type;
	sType *wt = type;

	// "silent" pointer (&)?
	if ((wt->IsPointer) && (wt->IsSilent)){
		if (direct_type_match(pt, wt->SubType)){
			so("<silent Ref &>");

			sLinkData Meta = *link;
			link->type = CreatePointerType(pt);
			link->Kind = KindReference;
			link->Meta = add_link_data(this);
			(*link->Meta) = Meta;
		}else{
			Exp.cur_exp --;
			DoError(string2("(c) parameter %d for function \"%s\" has type (%s), (%s) expected", param_no + 1, f_name, pt->Name, wt->Name));
			_return_(4,);
		}

	// normal type cast
	}else if (!direct_type_match(pt, wt)){
		int tc = -1;
		for (int i=0;i<TypeCast.size();i++)
			if ((direct_type_match(TypeCast[i].Source, pt)) && (direct_type_match(TypeCast[i].Dest, wt)))
				tc = i;

		if (tc >= 0){
			so("TypeCast");
			apply_type_cast(this, tc, link);
		}else{
			DoError(string2("(a) parameter %d for function \"%s\" has type (%s), (%s) expected", param_no + 1, f_name, pt->Name, wt->Name));
			_return_(4,);
		}
	}
	msg_db_l(4);
}

// creates <Operand> to be the function call described by <link>
void CPreScript::GetFunctionCall(char *f_name, sLinkData *Operand, sLinkData *link, sFunction *f)
{
	msg_db_r("GetFunctionCall", 4);
	
	// function as a variable?
	if (Exp.cur_exp >= 2)
	if ((strcmp(get_name(Exp.cur_exp - 2), "&") == 0) && (strcmp(cur_name, "(") != 0)){
		if (link->Kind == KindFunction){
			so("Funktion als Variable!");
			Operand->Kind = KindVarFunction;
			Operand->type = TypeVoid;
			Operand->Nr = link->Nr;
		}else{
			Exp.cur_exp --;
			//DoError("\"(\" expected in front of parameter list");
			DoError("only script functions can be referenced");
		}
		_return_(4,);
	}

	
	// "special" functions
    if (link->Kind == KindCompilerFunction)
	    if (link->Nr == CommandSizeof){
			GetSpecialFunctionCall(f_name, Operand, link, f);
			_return_(4,);
		}

	// create command
	int ncmd = command_from_link(this, link);
	int fnc = -1;
	if (link->Kind == KindFunction)
		fnc = link->Nr;

	so(Type2Str(this, Command[ncmd].ReturnType));
	// link operand onto this command
	Operand->Kind = KindCommand;
	Operand->Nr = ncmd;
	Operand->type = link->type;
	so(Command[ncmd].NumParams);


	
	// find (and provisorically link) the parameters in the source
	int np;
	sType *WantedType[SCRIPT_MAX_PARAMS];
	
	bool needs_brackets = ((Command[ncmd].ReturnType != TypeVoid) || (Command[ncmd].NumParams != 1));
	if (needs_brackets){
		FindFunctionParameters(np, WantedType, f, ncmd, fnc);
		
	}else{
		np = 1;
		FindFunctionSingleParameter(0, WantedType, f, ncmd, fnc);
	}
	if (Error){
		_return_(4,);
	}

	// test compatibility
	if (np != Command[ncmd].NumParams){
		Exp.cur_exp --;
		DoError(string2("function \"%s\" expects %d parameters, %d were found",f_name, Command[ncmd].NumParams, np));
		_return_(4,);
	}
	for (int p=0;p<np;p++){

		// return-Typ der aktuellen Funktion anpassen
		if ((Command[ncmd].Kind == KindCompilerFunction) && (Command[ncmd].LinkNr == CommandReturn))
			WantedType[p] = f->Type;

		CheckParamLink(&Command[ncmd].ParamLink[p], WantedType[p], f_name, p);
		if (Error){
			_return_(4,);
		}
	}
	msg_db_l(4);
}

inline bool type_match(sType *type, bool is_struct, sType *wanted);
inline bool type_match_with_cast(sType *type, bool is_struct, bool is_modifiable, sType *wanted, int &penalty, int &cast);

sLinkData CPreScript::GetOperand(sFunction *f)
{
	msg_db_r("GetOperand", 4);
	sLinkData Operand;
	Operand.Meta = NULL;
	so(cur_name);

	// ( -> one level down and combine commands
	if (strcmp(cur_name, "(") == 0){
		next_exp();
		Operand = GetCommand(f);
		if (strcmp(cur_name, ")") != 0){
			DoError("\")\" expected");
			msg_db_l(4);
			return Operand;
		}
		next_exp();
	}else if (strcmp(cur_name, "&") == 0){ // & -> address operator
		so("<Adress-Operator &>");
		next_exp();
		Operand.Meta = add_link_data(this);
		sLinkData ttt = GetOperand(f);
		(*Operand.Meta) = ttt;
		if (Error){
			msg_db_l(4);
			return Operand;
		}
		Operand.Kind = KindReference;
		// create a new type as a pointer onto the meta type
		Operand.type = CreatePointerType(Operand.Meta->type);
	}else if (strcmp(cur_name, "*") == 0){ // * -> dereference
		so("<Dereferenzierung *>");
		next_exp();
		Operand.Meta = add_link_data(this);
		(*Operand.Meta) = GetOperand(f);
		if (Error){
			msg_db_l(4);
			return Operand;
		}
		if (!Operand.Meta->type->IsPointer){
			Exp.cur_exp --;
			DoError("only pointers can be dereferenced using \"*\"");
			msg_db_l(4);
			return Operand;
		}
		Operand.Kind=KindDereference;
		Operand.type=Operand.Meta->type->SubType;
	}else{
		// direct operand
		if (GetExistence(cur_name, f)){
			sLinkData link = GetExistenceLink;
			char f_name[SCRIPT_MAX_NAME * 2];
			strcpy(f_name, cur_name);
			so(string("=> ", Kind2Str(link. Kind)));
			Operand = link;
			next_exp();
			// variables get linked directly...

			// operand is executable
			if ((link.Kind == KindFunction) || (link.Kind == KindCompilerFunction)){
				GetFunctionCall(f_name, &Operand, &link, f);
				
			}else if (link.Kind == KindPrimitiveOperator){
				// unary operator
				int _ie=Exp.cur_exp-1;
				so("  => unaerer Operator");
				int po = link.Nr, o=-1;
				sLinkData sub_command = GetOperand(f);
				if (Error){
					msg_db_l(4);
					return Operand;
				}
				sType *r = TypeVoid;
				sType *p2 = sub_command.type;

				// exact match?
				bool ok=false;
				for (int i=0;i<PreOperator.size();i++)
					if ((unsigned)po == PreOperator[i].PrimitiveID)
						if ((PreOperator[i].ParamType1 == TypeVoid) && (type_match(p2, false, PreOperator[i].ParamType2))){
							o = i;
							r = PreOperator[i].ReturnType;
							ok = true;
							break;
						}


				// needs type casting?
				if (!ok){
					int pen2;
					int c2, c2_best;
					int pen_min = 100;
					for (int i=0;i<PreOperator.size();i++)
						if (po == PreOperator[i].PrimitiveID)
							if ((PreOperator[i].ParamType1 == TypeVoid) && (type_match_with_cast(p2, false, false, PreOperator[i].ParamType2, pen2, c2))){
								ok = true;
								if (pen2 < pen_min){
									r = PreOperator[i].ReturnType;
									o = i;
									pen_min = pen2;
									c2_best = c2;
								}
						}
					// cast
					if (ok){
						apply_type_cast(this, c2_best, &sub_command);
						if (Error)
							_return_(4, Operand);
					}
				}


				if (!ok){
					Exp.cur_exp = _ie;
					DoError(string("unknown unitary operator  ", p2->Name));
					msg_db_l(4);
					return Operand;
				}
				int cmd = AddCommand();
				sCommand *c = &Command[cmd];
				c->Kind=KindOperator;
				c->LinkNr=o;
				c->NumParams=1;
				c->ParamLink[0]=sub_command;
				c->ReturnType=PreOperator[o].ReturnType;
				Operand.Kind=KindCommand;
				Operand.Nr=cmd;
				Operand.type=PreOperator[o].ReturnType;
				so(Operator2Str(this,o));
				msg_db_l(4);
				return Operand;
			}
		}else{
			sType *t = GetConstantType(cur_name);
			if (t != TypeUnknown){
				so("=> Konstante");
				Operand.Kind = KindConstant;
				// constant for parameter (via variable)
				Operand.type = t;
				Operand.Nr = AddConstant(t);
				memcpy(Constant[Operand.Nr].data, GetConstantValue(cur_name), t->Size);
				next_exp();
			}else{
				DoError("unknown operand");
				//Operand.Kind=0;
				msg_db_l(4);
				return Operand;
			}
		}

	}
	if (Error){
		msg_db_l(4);
		return Operand;
	}

	// Arrays, Strukturen aufloessen...
	GetOperandExtension(&Operand,f);

	so(string("Operand endet mit ", get_name(Exp.cur_exp - 1)));
	msg_db_l(4);
	return Operand;
}

static sLinkData LastOperator;

// nur "primitiver" Operator -> keine Typen-Angaben
bool CPreScript::GetOperator(sFunction *f)
{
	msg_db_r("GetOperator",4);
	so(cur_name);
	int op = WhichPrimitiveOperator(cur_name);
	if (op >= 0){

		// Befehl aus Operator
		int cmd = AddCommand();
		Command[cmd].Kind = KindPrimitiveOperator;
		Command[cmd].LinkNr = op;
		// nur provisorisch (nur die Art des Zeichens, Parameter und deren Art erst am Ende von GetCommand!!!)

		// Befehl
		LastOperator.Kind = KindCommand;
		LastOperator.Nr = cmd;
		LastOperator.type = Command[cmd].ReturnType; // ???

		next_exp();
		msg_db_l(4);
		return true;
	}
	msg_db_l(4);
	return false;
}

inline void CommandMakeOperator(sCommand *cmd, sLinkData &p1, sLinkData &p2, int op)
{
	cmd->Kind = KindOperator;
	cmd->LinkNr = op;
	cmd->NumParams = (PreOperator[op].ParamType2 == TypeVoid) ? 1 : 2; // unary / binary
	cmd->ParamLink[0] = p1;
	cmd->ParamLink[1] = p2;
	cmd->ReturnType = PreOperator[op].ReturnType;
}

/*inline int find_operator(int primitive_id, sType *param_type1, sType *param_type2)
{
	for (int i=0;i<PreOperator.size();i++)
		if (PreOperator[i].PrimitiveID == primitive_id)
			if ((PreOperator[i].ParamType1 == param_type1) && (PreOperator[i].ParamType2 == param_type2))
				return i;
	//_do_error_("");
	return 0;
}*/

// both operand types have to match the operator's types
//   (operater wants a pointer -> all pointers are allowed!!!)
//   (same for structs of same type...)
inline bool type_match(sType *type, bool is_struct, sType *wanted)
{
	if (type == wanted)
		return true;
	if ((type->IsPointer) && (wanted == TypePointer))
		return true;
	if ((is_struct) && (wanted == TypeStruct))
		return true;
	return false;
}

inline bool type_match_with_cast(sType *type, bool is_struct, bool is_modifiable, sType *wanted, int &penalty, int &cast)
{
	penalty = 0;
	cast = -1;
	if (type_match(type, is_struct, wanted))
	    return true;
	if (is_modifiable) // is a variable getting assigned.... better not cast
		return false;
	for (int i=0;i<TypeCast.size();i++)
		if ((direct_type_match(TypeCast[i].Source, type)) && (direct_type_match(TypeCast[i].Dest, wanted))){ // type_match()?
			penalty = TypeCast[i].Penalty;
			cast = i;
			return true;
		}
	return false;
}

void apply_type_cast(CPreScript *ps, int tc, sLinkData *param)
{
	if (tc < 0)
		return;
	so(string2("Benoetige automatischen TypeCast: %s -> %s", TypeCast[tc].Source->Name, TypeCast[tc].Dest->Name));
	if (param->Kind == KindConstant){
		void *d = TypeCast[tc].Func(ps->Constant[param->Nr].data);
		memcpy(ps->Constant[param->Nr].data, d, TypeCast[tc].Dest->Size);
		ps->Constant[param->Nr].type = TypeCast[tc].Dest;
		param->type = TypeCast[tc].Dest;
		so("  ...Konstante wurde direkt gewandelt!");
	}else{
		int cmd = ps->AddCommand();
		//ps->SetCompilerFunction(TypeCast[tc].Command, ps->Command[cmd]);
		ps->Command[cmd].Kind = KindCompilerFunction;
		ps->Command[cmd].LinkNr = TypeCast[tc].Command;
		ps->Command[cmd].NumParams = 1;
		ps->Command[cmd].ParamLink[0] = *param;
		ps->Command[cmd].ReturnType = TypeCast[tc].Dest;

		// relink param
		param->Kind = KindCommand;
		param->Nr = cmd;
		param->type = TypeCast[tc].Dest;
		so("  ...keine Konstante: Wandel-Befehl wurde hinzugefuegt!");
	}
}

void CPreScript::LinkMostImportantOperator(int &NumOperators, sLinkData *Operand, sLinkData *Operator, int *op_exp)
{
	msg_db_r("LinkMostImpOp",4);
// find the most important operator (mio)
	int mio=0;
	for (int i=0;i<NumOperators;i++){
		so(string(i2s(PrimitiveOperator[Command[Operator[i].Nr].LinkNr].Level),", ",i2s(Command[Operator[i].Nr].LinkNr)));
		if (PrimitiveOperator[Command[Operator[i].Nr].LinkNr].Level>PrimitiveOperator[Command[Operator[mio].Nr].LinkNr].Level)
			mio=i;
	}
	so(mio);

// link it
	sLinkData param1 = Operand[mio];
	sLinkData param2 = Operand[mio + 1];
	bool left_modifiable = PrimitiveOperator[Command[Operator[mio].Nr].LinkNr].LeftModifiable;
	
	int po = Command[Operator[mio].Nr].LinkNr, o = -1;
	sType *r = TypeVoid;
	sType *p1 = Operand[mio].type;
	sType *p2 = Operand[mio+1].type;
	bool equal_structs = false;
	if (p1 == p2){
		for (int i=0;i<Struct.size();i++)
			if (Struct[i].RootType == p1)
				equal_structs = true;
	}


	// exact match?
	bool ok = false;
	for (int i=0;i<PreOperator.size();i++)
		if (po == PreOperator[i].PrimitiveID)
			if (type_match(p1, equal_structs, PreOperator[i].ParamType1) && type_match(p2, equal_structs, PreOperator[i].ParamType2)){
				o = i;
				r = PreOperator[i].ReturnType;
				ok = true;
				break;
			}


	// needs type casting?
	if (!ok){
		int pen1, pen2;
		int c1, c2, c1_best, c2_best;
		int pen_min = 200;
		for (int i=0;i<PreOperator.size();i++)
			if ((unsigned)po == PreOperator[i].PrimitiveID)
				if (type_match_with_cast(p1, equal_structs, left_modifiable, PreOperator[i].ParamType1, pen1, c1) && type_match_with_cast(p2, equal_structs, false, PreOperator[i].ParamType2, pen2, c2)){
					ok = true;
					if (pen1 + pen2 < pen_min){
						r = PreOperator[i].ReturnType;
						o = i;
						pen_min = pen1 + pen2;
						c1_best = c1;
						c2_best = c2;
					}
			}
		// cast
		if (ok){
			apply_type_cast(this, c1_best, &param1);
			apply_type_cast(this, c2_best, &param2);
			if (Error)
				_return_(4,);
		}
	}

	if (ok){
		CommandMakeOperator(&Command[Operator[mio].Nr], param1, param2, o);
		Operator[mio].type=r;
	}else{
		Exp.cur_exp = op_exp[mio];
		_do_error_(string2("no operator found: (%s) %s (%s)",Type2Str(this,p1),PrimitiveOperator2Str(po),Type2Str(this,p2)), 4,);
	}

// ihn aus der Liste herauskuerzen
	Operand[mio]=Operator[mio];
	for (int i=mio;i<NumOperators-1;i++){
		Operator[i]=Operator[i+1];
		Operand[i+1]=Operand[i+2];
		op_exp[i] = op_exp[i+1];
	}
	NumOperators--;
	msg_db_l(4);
}

sLinkData CPreScript::GetCommand(sFunction *f)
{
	msg_db_r("GetCommand", 4);
	int NumOperands = 0;
	std::vector<sLinkData> Operand;
	std::vector<sLinkData> Operator;
	std::vector<int> op_exp;

	// find the first operand
	Operand.push_back(GetOperand(f));
	if (Error){
		msg_db_l(4);
		return Operand[0];
	}
	NumOperands ++;

	// je einen Operator und einen Operanden finden
	for (int i=0;true;i++){
		op_exp.push_back(Exp.cur_exp);
		if (GetOperator(f)){
			Operator.push_back(LastOperator);
			if (end_of_line()){
				//Exp.cur_exp --;
				_do_error_("unexpected end of line after operator", 4, Operand[0]);
			}
			Operand.push_back(GetOperand(f));
			if (Error){
				msg_db_l(4);
				return Operand[0];
			}
			NumOperands++;
		}else{
			if (Error){
				msg_db_l(4);
				return Operand[0];
			}
			so("(kein weiterer Operator)");
			so(cur_name);
			break;
		}
	}


	// in jedem Schritt den wichtigsten Operator finden und herauskuerzen
	int NumOperators=NumOperands-1;
	for (int i=0;i<NumOperands-1;i++){
		LinkMostImportantOperator(NumOperators, &Operand[0], &Operator[0], &op_exp[0]);
		if (Error){
			msg_db_l(4);
			return Operand[0];
		}
	}

	sLinkData ret = Operand[0];
	Operand.clear();
	Operator.clear();
	op_exp.clear();

	// der gesammte Befehl hat sich dann im Operand[0] gesammelt
	// (ohne Operator war Operand[0] schon das einzig wichtige)

	so("-fertig");
	so(string("Command endet mit ",get_name(Exp.cur_exp - 1)));
	msg_db_l(4);
	return ret;
}

inline sLinkData GetLinkForCommand(CPreScript *ps, int cmd, sType *type)
{
	sLinkData link;
	link.Kind = KindCommand;
	link.Nr = cmd;
	link.type = type;
	link.Meta = link.ParamLink = NULL;
	link.script = NULL;
	return link;
}


struct loop_data_t
{
	int NumBreaks;
	int BreakCmd[256];
	int NumContinues;
	int ContinueCmd[256];
};
loop_data_t *CurLoop = NULL;
loop_data_t *loop_start()
{
	loop_data_t *last = CurLoop;
	CurLoop = new loop_data_t;
	am("CurLoop", sizeof(loop_data_t), CurLoop);
	CurLoop->NumBreaks = CurLoop->NumContinues = 0;
	return last;
}
void loop_end(CPreScript *ps, loop_data_t *last_loop, int cmd_cont, int cmd_end)
{
	// correct breaks...
	for (int i=0;i<CurLoop->NumBreaks;i++)
		ps->Command[CurLoop->BreakCmd[i]].SubLink1 = cmd_end;
	// correct continues...
	for (int i=0;i<CurLoop->NumContinues;i++)
		ps->Command[CurLoop->ContinueCmd[i]].SubLink1 = cmd_cont;
	//...
	delete(CurLoop);
	dm("CurLoop", CurLoop);
	CurLoop = last_loop;
}


void CPreScript::GetSpecialCommand(sBlock *block, sFunction *f)
{
	msg_db_r("GetSpecialCommand", 4);

	// special commands...
	if (strcmp(cur_name, "for") == 0){
		// variable
		next_exp();
		GetExistence(cur_name, f);
		sLinkData for_var = GetExistenceLink;
		if (Error)	_return_(4,);
		if ((!is_variable(for_var.Kind)) || ((for_var.type != TypeInt) && (for_var.type != TypeFloat)))
			_do_error_("int or float variable expected after \"for\"", 4,);
		next_exp();

		// first value
		if (strcmp(cur_name, ",") != 0)
			_do_error_("\",\" expected after variable in for", 4,);
		next_exp();
		sLinkData val0 = GetCommand(f);
		if (Error)	_return_(4,);
		if (val0.type != for_var.type){
			Exp.cur_exp --;
			_do_error_(string2("%s expected as first value of for", for_var.type->Name), 4,);
		}

		// last value
		if (strcmp(cur_name, ",") != 0)
			_do_error_("\",\" expected after variable in for", 4,);
		next_exp();
		sLinkData val1 = GetCommand(f);
		if (Error)	_return_(4,);
		if (val1.type != for_var.type){
			Exp.cur_exp --;
			_do_error_(string2("%s expected as last value of for", for_var.type->Name), 4,);
		}

		// implement
		// for_var = val0
		int cmd = AddCommand();
		CommandMakeOperator(&Command[cmd], for_var, val0, OperatorIntAssign);
		block->Command.push_back(cmd);
			
		// while(for_var < val1)
		int cmd_cmp = AddCommand();
		CommandMakeOperator(&Command[cmd_cmp], for_var, val1, OperatorIntSmaller);
			
		cmd = add_command_compilerfunc(this, CommandWhile);
		Command[cmd].ParamLink[0] = GetLinkForCommand(this, cmd_cmp, for_var.type);
		block->Command.push_back(cmd);
		Command[cmd].SubLink1 = block->Command.size()-1;
		if (ExpectNewline())
			_return_(4,);
		// ...block
		next_line();
		if (ExpectIndent())
			_return_(4,);
		int loop_block_no = Block.size(); // should get created...soon
		loop_data_t *last_loop = loop_start();
		GetCompleteCommand(block, f);
		Command[cmd].SubLinkEnd = block->Command.size(); // -> next command
			
		// ...for_var += 1
		int cmd_inc = AddCommand();
		if (for_var.type == TypeInt){
			CommandMakeOperator(&Command[cmd_inc], for_var, val1 /*dummy*/, OperatorIntIncrease);
		}else{
			int nc = AddConstant(TypeFloat);
			*(float*)Constant[nc].data = 1.0;
			sLinkData val_add;
			val_add.Kind = KindConstant;
			val_add.Nr = nc;
			val_add.type = TypeFloat;
			CommandMakeOperator(&Command[cmd_inc], for_var, val_add, OperatorFloatAddS);
		}
		sBlock *loop_block = &Block[loop_block_no];
		loop_block->Command.push_back(cmd_inc); // add to loop-block
		loop_end(this, last_loop, cmd_inc, block->Command.size());
		
	}else if (strcmp(cur_name, "while") == 0){
		next_exp();
		sLinkData cmp = GetCommand(f);
		CheckParamLink(&cmp, TypeBool, "while", 0);
		if (Error)	_return_(4,);
		if (ExpectNewline())
			_return_(4,);
			
		int cmd = add_command_compilerfunc(this, CommandWhile);
		Command[cmd].ParamLink[0] = cmp;
		block->Command.push_back(cmd);
		Command[cmd].SubLink1 = block->Command.size()-1;
		// ...block
		next_line();
		if (ExpectIndent())
			_return_(4,);
		loop_data_t *last_loop = loop_start();
		GetCompleteCommand(block, f);
		Command[cmd].SubLinkEnd = block->Command.size(); // -> next command
		loop_end(this, last_loop, -1, block->Command.size());
		
 	}else if (strcmp(cur_name, "break") == 0){
		next_exp();
		int cmd = add_command_compilerfunc(this, CommandBreak);
		block->Command.push_back(cmd);
		if (!CurLoop)
			_do_error_("break has to be within a loop", 4,);
		CurLoop->BreakCmd[CurLoop->NumBreaks ++] = cmd;
		
	}else if (strcmp(cur_name, "continue") == 0){
		next_exp();
		int cmd = add_command_compilerfunc(this, CommandContinue);
		block->Command.push_back(cmd);
		if (!CurLoop)
			_do_error_("continue has to be within a loop", 4,);
		CurLoop->ContinueCmd[CurLoop->NumContinues ++] = cmd;
		
	}else if (strcmp(cur_name, "if") == 0){
		int ind = Exp.cur_line->indent;
		next_exp();
		sLinkData cmp = GetCommand(f);
		CheckParamLink(&cmp, TypeBool, "if", 0);
		if (Error)	_return_(4,);
		if (ExpectNewline())
			_return_(4,);
			
		int cmd = add_command_compilerfunc(this, CommandIf);
		Command[cmd].ParamLink[0] = cmp;
		block->Command.push_back(cmd);
		Command[cmd].SubLink1 = block->Command.size()-1;
		// ...block
		next_line();
		if (ExpectIndent())
			_return_(4,);
		GetCompleteCommand(block, f);
		Command[cmd].SubLinkEnd = block->Command.size(); // -> next command
		next_line();

		// else?
		if ((!end_of_file()) && (strcmp(cur_name, "else") == 0) && (Exp.cur_line->indent >= ind)){
			Command[cmd].LinkNr = CommandIfElse;
			next_exp();
			// iterative if
			if (strcmp(cur_name, "if") == 0){
				// sub-if's in a new block
				int NewBlock = AddBlock();
				if (Error)
					_return_(4,);
				// parse the next if
				GetCompleteCommand(&Block[NewBlock], f);
				// command for the found block
				int cmd = AddCommand();
				Command[cmd].Kind=KindBlock;
				Command[cmd].LinkNr=NewBlock;
				// ...
				block->Command.push_back(cmd);
				Command[cmd].SubLinkEnd = block->Command.size(); // -> next command
				//_do_error_("elsif", 4,);
				_return_(4,);
			}
			if (ExpectNewline())
				_return_(4,);
			// ...block
			next_line();
			if (ExpectIndent())
				_return_(4,);
			GetCompleteCommand(block, f);
			Command[cmd].SubLinkEnd = block->Command.size(); // -> next command
			//next_line();
		}else{
			Exp.cur_line --;
			Exp.cur_exp = Exp.cur_line->exp.size() - 1;
		}
	}
	
	msg_db_l(4);
}

// we already are in the line to analyse ...indentation for a new block should compare to the last line
void CPreScript::GetCompleteCommand(sBlock *block, sFunction *f)
{
	msg_db_r("GetCompleteCommand", 4);
	// cur_exp = 0!

	sType *tType = GetType(Exp.cur_exp, false);
	int last_indent = indent_0;

	// block?  <- indent
	if (indented){
		indented = false;
		Exp.cur_exp = 0; // bad hack...
		msg_db_r("Block", 4);
		int NewBlock=AddBlock();
		if (Error){
			msg_db_l(4);
			_return_(4,);
		}
		Block[NewBlock].RootNr = block->Nr;

		int cmd = AddCommand();
		Command[cmd].Kind=KindBlock;
		Command[cmd].LinkNr=NewBlock;
		block->Command.push_back(cmd);

		for (int i=0;true;i++){
			if (((i > 0) && (Exp.cur_line->indent < last_indent)) || (end_of_file()))
				break;

			GetCompleteCommand(&Block[NewBlock], f);
			if (Error){
				msg_db_l(4);
				_return_(4,);
			}
			next_line();
		}
		Exp.cur_line --;
		indent_0 = Exp.cur_line->indent;
		indented = false;
		Exp.cur_exp = Exp.cur_line->exp.size() - 1;
		msg_db_l(4);

	// assembler block
	}else if (strcmp(cur_name, "-asm-") == 0){
		next_exp();
		so("<Asm-Block>");
		int cmd = add_command_compilerfunc(this, CommandAsm);
		block->Command.push_back(cmd);

	// local (variable) definitions...
	// type of variable
	}else if (tType){
		for (int l=0;!end_of_line();l++){
			ParseVariableDefSingle(tType, f);

			// assignment?
			if (strcmp(cur_name, "=") == 0){
				Exp.cur_exp --;
				sLinkData link = GetCommand(f);
				if (Error)
					_return_(4,);
				if (link.Kind == KindCommand)
					block->Command.push_back(link.Nr);
			}
			if (end_of_line())
				break;
			if ((strcmp(cur_name, ",") != 0) && (!end_of_line()))
				_do_error_("\",\", \"=\" or newline expected after definition of local variable", 4,);
			next_exp();
		}
		_return_(4,);
	}else{

		
	// commands (the actual code!)
		if ((strcmp(cur_name, "for") == 0) || (strcmp(cur_name, "while") == 0) || (strcmp(cur_name, "break") == 0) || (strcmp(cur_name, "continue") == 0) || (strcmp(cur_name, "if") == 0)){
			GetSpecialCommand(block, f);

		}else{

			// normal commands
			sLinkData link = GetCommand(f);
			if (Error)
				_return_(4,);

			if (link.Kind == KindCommand){
				// link
				block->Command.push_back(link.Nr);
			}
		}
	}

	if (ExpectNewline())
		_return_(4,);
	msg_db_l(4);
}

// look for array definitions and correct pointers
void CPreScript::TestArrayDefinition(sType **type, bool is_pointer)
{
	msg_db_r("TestArrayDef", 4);
	if (is_pointer){
		(*type) = CreatePointerType((*type));
	}
	if (strcmp(cur_name, "[") == 0){
		sType nt,*pt = &nt;
		int array_size;
		char or_name[SCRIPT_MAX_NAME];
		strcpy(or_name, (*type)->Name);
		int or_name_length = strlen(or_name);
		so("-Array-");
		next_exp();
		if (GetConstantType(cur_name) == TypeInt){
			array_size = *(int*)GetConstantValue(cur_name);
		}else{
			DoError("only constants of type \"int\" allowed for size of arrays");
			msg_db_l(4);
			return;
		}
		next_exp();
		if (strcmp(cur_name, "]") != 0){
			DoError("\"]\" expected after array size");
			msg_db_l(4);
			return;
		}
		next_exp();
		// recursion
		TestArrayDefinition(type, false); // is_pointer=false, since pointers have been handled
		
		// create array       (complicated name necessary to get correct ordering   int a[2][4] = (int[4])[2])
		(*type) = CreateNewType(	string2("%s[%d]%s", or_name, array_size, &(*type)->Name[or_name_length]),
		                        	(*type)->Size * array_size, false, false, array_size, (*type));
	}
	msg_db_l(4);
}



// Datei auslesen (und Kommentare auslesen)
bool CPreScript::LoadToBuffer(char *filename,bool just_analyse)
{
	msg_db_r("LoadToBuffer",4);

// read file
	CFile *f = FileOpen(filename);
	if (!f){
		DoError("script file not loadable");
		msg_db_l(4);
		return false;
	}
	am("file",sizeof(CFile),f);
	Buffer=new char[f->GetSize()+10];
	am("Buffer",f->GetSize()+10,Buffer);
	f->ReadComplete(Buffer, BufferLength);
	Buffer[BufferLength] = 0;
	FileClose(f);
	dm("file",f);

	Analyse(Buffer, just_analyse);


	delete[](Buffer);
	dm("Buffer",Buffer);

	msg_db_l(4);
	return !Error;
}

void CPreScript::HandleMacro(ps_line_t *l, int &line_no, int &NumIfDefs, bool *IfDefed, bool just_analyse)
{
	msg_db_r("HandleMacro", 4);
	Exp.cur_line = l;
	Exp.cur_exp = 0;
	int ln;
	char filename[256];
	CScript *include;
	
	int macro_no=-1;
	for (int i=0;i<NumMacroNames;i++)
		if (strcmp(cur_name, MacroName[i]) == 0)
			macro_no = i;
	
	switch(macro_no){
		case MacroInclude:
			next_exp();
			/*if (!IsIfDefed(NumIfDefs, IfDefed))
				continue;*/
			strcpy(filename, dir_from_filename(Filename));
			strcat(filename, &cur_name[1]);
			filename[strlen(filename) - 1] = 0; // remove "
			strcpy(filename, filename_no_recursion(filename));

			so("lade Include-Datei");
			right();

			include = LoadScriptAsInclude(filename, just_analyse);

			left();
			if ((!include) || (include->Error)){
				IncludeLinkerError |= include->LinkerError;
				DoError(string2("error in inluded file \"%s\":\n[ %s (line %d:) ]", filename, include->ErrorMsg, include->ErrorLine, include->ErrorColumn));
				return;
			}
			AddIncludeData(include);
			//DoError("include noch nicht implementiert");
			break;
		case MacroDefine:
			sDefine d;
			// source
			next_exp();
			strcpy(d.Source, cur_name);
			d.NumDests = 0;
			// dests
			for (int i=0;i<SCRIPT_MAX_DEFINE_DESTS;i++){
				next_exp();
				if (end_of_line())
					break;
				strcpy(d.Dest[d.NumDests++], cur_name);
			}
			Define.push_back(d);
			break;
		/*case MacroIfdef:
			next_exp();
			//IfDefed[NumIfDefs] = false;
			bool defed = false;
			for (int i=0;i<NumDefines;i++)
				if (strcmp(Temp, Define[i]->Source) == 0){
					//IfDefed[NumIfDefs] = true;
					defed = true;
					break;
				}
			//NumIfDefs ++;
			if (!defed){
			}
			break;*/
	/*	case MacroIfndef:
			NextExp(Buffer);
			IfDefed[NumIfDefs]=true;
			for (i=0;i<NumDefines;i++)
				if (strcmp(Temp,Define[i]->Source)==0){
					IfDefed[NumIfDefs]=false;
					break;
				}
			NumIfDefs++;
			break;
		case MacroElse:
			if (NumIfDefs<1){
				strcpy(Exp->Name[Exp->NumExps],Temp);
				Exp->Line[Exp->NumExps]=Exp->TempLine;
				Exp->Column[Exp->NumExps]=Exp->TempColumn;
				DoError("\"#else\" found but no matching \"#ifdef\"",Exp->NumExps);
				return;
			}
			IfDefed[NumIfDefs-1]=!IfDefed[NumIfDefs-1];
			break;
		case MacroEndif:
			if (NumIfDefs<1){
				strcpy(Exp->Name[Exp->NumExps],Temp);
				Exp->Line[Exp->NumExps]=Exp->TempLine;
				Exp->Column[Exp->NumExps]=Exp->TempColumn;
				DoError("\"#endif\" found but no matching \"#ifdef\"",Exp->NumExps);
				return;
			}
			NumIfDefs--;
			break;*/
		case MacroRule:
			next_exp();
			ln = -1;
			for (int i=0;i<NumScriptLocations;i++)
				if (strcmp(ScriptLocation[i].Name, cur_name)==0)
					ln=i;
			if (ln < 0){
				DoError("unknown location in script rule");
				msg_db_l(4);
				return;
			}
			sPreScriptRule pr;
			PreScriptRule.push_back(pr);
			pr.Location = ScriptLocation[ln].Location;
			next_exp();
			pr.Level = s2i(Temp);
			next_exp();
			strcpy(pr.Name, &cur_name[1]);
			pr.Name[strlen(pr.Name) - 1] = 0;
			break;
		case MacroDisasm:
			FlagDisassemble=true;
			break;
		case MacroShow:
			FlagShow=true;
			break;
		case MacroImmortal:
			FlagImmortal=true;
			break;
		case MacroOs:
			FlagCompileOS=true;
			break;
		case MacroInitialRealmode:
			FlagCompileInitialRealMode=true;
			break;
		case MacroVariablesOffset:
			FlagOverwriteVariablesOffset=true;
			next_exp();
			VariablesOffset=s2i2(cur_name);
			break;
		case MacroCodeOrigin:
			next_exp();
			CreateAsmMetaInfo(this);
			((sAsmMetaInfo*)AsmMetaInfo)->CodeOrigin = s2i2(cur_name);
			break;
		default:
			DoError("unknown makro atfer \"#\"");
			msg_db_l(4);
			return;
	}

	// remove macro line
	Exp.line[line_no].exp.clear();
	Exp.line.erase(Exp.line.begin() + line_no);
	line_no --;
	msg_db_l(4);
}

// ... maybe some time later
void CPreScript::PreCompiler(bool just_analyse)
{
	if (Error)	return;
	msg_db_r("PreCompiler", 4);

	int NumIfDefs = 0;
	bool IfDefed[1024];
	
	for (int i=0;i<Exp.line.size()-1;i++){
		Exp.cur_exp = 0;
		Exp.cur_line = &Exp.line[i];
		if (Exp.line[i].exp[0].name[0] == '#')
			HandleMacro(Exp.cur_line, i, NumIfDefs, IfDefed, just_analyse);
		else{

			// replace by definition?
			int num_defs_inserted = 0;
			while(!end_of_line()){
				sDefine *d;
				foreach(Define, d, j){
					if (strcmp(cur_name, d->Source) == 0){
						int pos = Exp.cur_line->exp[Exp.cur_exp].pos;
						remove_from_buffer(this, Exp.cur_exp);
						for (int k=0;k<d->NumDests;k++){
							insert_into_buffer(this, d->Dest[k], pos, Exp.cur_exp);
							next_exp();
						}
						Exp.cur_exp -= d->NumDests;
						num_defs_inserted ++;
						if (num_defs_inserted > SCRIPT_MAX_DEFINE_RECURSIONS){
							DoError("recursion in #define macros");
							msg_db_l(4);
							return;
						}
						break;
					}
				}
				next_exp();
			}

			// "-" in front of numbers (after ( , : [ = < >)
			Exp.cur_exp = 1;
			while(!end_of_line()){
				if (strcmp(cur_name, "-") == 0){
					if ((strcmp(get_name(Exp.cur_exp - 1), "(") == 0) ||
						(strcmp(get_name(Exp.cur_exp - 1), ",") == 0) ||
						(strcmp(get_name(Exp.cur_exp - 1), ":") == 0) ||
						(strcmp(get_name(Exp.cur_exp - 1), "[") == 0) ||
						(strcmp(get_name(Exp.cur_exp - 1), "=") == 0) ||
						(strcmp(get_name(Exp.cur_exp - 1), "<") == 0) ||
						(strcmp(get_name(Exp.cur_exp - 1), ">") == 0)){
						if (isNumber(get_name(Exp.cur_exp + 1)[0])){
							char name[SCRIPT_MAX_NAME * 2];
							strcpy(name, string("-", get_name(Exp.cur_exp + 1)));
							int pos = Exp.cur_line->exp[Exp.cur_exp].pos;
							remove_from_buffer(this, Exp.cur_exp);
							remove_from_buffer(this, Exp.cur_exp);
							insert_into_buffer(this, name, pos, Exp.cur_exp);
						}
					}
				}
				next_exp();
			}
		}
	}

	

	/*msg_db_r("MakeExps",4);
	int i,NumIfDefs=0,ln;
	bool IfDefed[1024];
	Exp=new exp_buffer;
	am("exp_buffer",sizeof(exp_buffer),Exp);
	Exp->BufferUsed=0;
	BufferPos=0;
	Exp->TempLine=1;
	Exp->TempColumn=0;
	Exp->NumExps=0;
	char filename[256];
	CScript *include;
	
	while(true){
		int l=(Exp->NumExps==0)?0:Exp->TempLine;
		NextExp(Buffer);
		if (Error)	return;
		if (Temp[0]==0)
			break;

		if ((Exp->TempLine>l)&&(strcmp(Temp,"#")==0)){
			msg_db_m("makro",4);
			l=Exp->TempLine;
			so("# -Makro");
			NextExp(Buffer);

			int macro_no=-1;
			for (i=0;i<NumMacroNames;i++)
				if (strcmp(Temp,MacroName[i])==0)
					macro_no=i;

			switch(macro_no){
				case MacroInclude:
					NextExp(Buffer);
					if (!IsIfDefed(NumIfDefs,IfDefed))
						continue;
					strcpy(filename,dir_from_filename(Filename));
					strcat(filename,&Temp[1]);
					filename[strlen(filename)-1]=0; // remove "
					strcpy(filename,filename_no_recursion(filename));

					so("lade Include-Datei");
					right();

					include=LoadScriptAsInclude(filename,just_analyse);

					left();
					if ((!include)||(include->Error)){
						IncludeLinkerError|=include->LinkerError;
						DoError(string2("error in inluded file \"%s\":\n[ %s (line %d:) ]",filename,include->ErrorMsg,include->ErrorLine,include->ErrorColumn),Exp->ExpNr);
						return;
					}
					AddIncludeData(include);
					Exp->ExpNr++;
					break;
				case MacroDefine:
					Define[NumDefines]=new sDefine;
					am("Define",sizeof(sDefine),Define[NumDefines]);
					Define[NumDefines]->Owner=this;
					// Source
					NextExp(Buffer);
					strcpy(Define[NumDefines]->Source,Temp);
					Define[NumDefines]->NumDests=0;
					// Dests
					int t;
					for (i=0;i<SCRIPT_MAX_DEFINE_DESTS;i++){
						t=BufferPos;
						NextExp(Buffer);
						if (Exp->TempLine>l){
							BufferPos=t;
							break;
						}
						strcpy(Define[NumDefines]->Dest[Define[NumDefines]->NumDests],Temp);
						Define[NumDefines]->NumDests++;
					}
					Exp->TempLine=l;
					NumDefines++;
					break;
				case MacroIfdef:
					NextExp(Buffer);
					IfDefed[NumIfDefs]=false;
					for (i=0;i<NumDefines;i++)
						if (strcmp(Temp,Define[i]->Source)==0){
							IfDefed[NumIfDefs]=true;
							break;
						}
					NumIfDefs++;
					break;
				case MacroIfndef:
					NextExp(Buffer);
					IfDefed[NumIfDefs]=true;
					for (i=0;i<NumDefines;i++)
						if (strcmp(Temp,Define[i]->Source)==0){
							IfDefed[NumIfDefs]=false;
							break;
						}
					NumIfDefs++;
					break;
				case MacroElse:
					if (NumIfDefs<1){
						strcpy(Exp->Name[Exp->NumExps],Temp);
						Exp->Line[Exp->NumExps]=Exp->TempLine;
						Exp->Column[Exp->NumExps]=Exp->TempColumn;
						DoError("\"#else\" found but no matching \"#ifdef\"",Exp->NumExps);
						return;
					}
					IfDefed[NumIfDefs-1]=!IfDefed[NumIfDefs-1];
					break;
				case MacroEndif:
					if (NumIfDefs<1){
						strcpy(Exp->Name[Exp->NumExps],Temp);
						Exp->Line[Exp->NumExps]=Exp->TempLine;
						Exp->Column[Exp->NumExps]=Exp->TempColumn;
						DoError("\"#endif\" found but no matching \"#ifdef\"",Exp->NumExps);
						return;
					}
					NumIfDefs--;
					break;
				case MacroRule:
					NextExp(Buffer);
					ln=-1;
					for (i=0;i<NumScriptLocations;i++)
						if (strcmp(ScriptLocation[i].Name,Temp)==0)
							ln=i;
					if (ln<0){
						strcpy(Exp->Name[Exp->NumExps],Temp);
						Exp->Line[Exp->NumExps]=Exp->TempLine;
						Exp->Column[Exp->NumExps]=Exp->TempColumn;
						DoError("unknown location in script rule",Exp->NumExps);
						return;
					}
					PreScriptRule[NumPreScriptRules]=new sPreScriptRule;
					am("PreScriptRule",sizeof(sPreScriptRule),PreScriptRule[NumPreScriptRules]);
					PreScriptRule[NumPreScriptRules]->Location=ScriptLocation[ln].Location;
					NextExp(Buffer);
					PreScriptRule[NumPreScriptRules]->Level=s2i(Temp);
					NextExp(Buffer);
					Temp[strlen(Temp)-1]=0;
					strcpy(PreScriptRule[NumPreScriptRules]->Name,&Temp[1]);
					NumPreScriptRules++;
					break;
				case MacroDisasm:
					FlagDisassemble=true;
					break;
				case MacroShow:
					FlagShow=true;
					break;
				case MacroImmortal:
					FlagImmortal=true;
					break;
				case MacroOs:
					FlagCompileOS=true;
					break;
				case MacroInitialRealmode:
					FlagCompileInitialRealMode=true;
					break;
				case MacroVariablesOffset:
					FlagOverwriteVariablesOffset=true;
					NextExp(Buffer);
					VariablesOffset=s2i2(Temp);
					break;
				case MacroCodeOrigin:
					NextExp(Buffer);
					CreateAsmMetaInfo(this);
					((sAsmMetaInfo*)AsmMetaInfo)->CodeOrigin=s2i2(Temp);
					break;
				default:
					strcpy(Exp->Name[Exp->NumExps],Temp);
					Exp->Line[Exp->NumExps]=Exp->TempLine;
					Exp->Column[Exp->NumExps]=Exp->TempColumn;
					DoError("unknown makro atfer \"#\"",Exp->NumExps);
					return;
			}
			continue;
		}
	//msg_db_m("def",4);

		bool defed=false;
		for (i=0;i<NumDefines;i++)
			if (strcmp(Temp,Define[i]->Source)==0){
				defed=true;
				for (int j=0;j<Define[i]->NumDests;j++){
					strcpy(Exp->Name[Exp->NumExps],Define[i]->Dest[j]);
					Exp->BufferUsed+=strlen(Define[i]->Dest[j])+1;
					Exp->Name[Exp->NumExps+1]=&Exp->Buffer[Exp->BufferUsed];
					Exp->Line[Exp->NumExps]=Exp->TempLine;
					Exp->Column[Exp->NumExps]=Exp->TempColumn;
					Exp->NumExps++;
				}
				break;
			}
			
	//msg_db_m("postdef",4);
		if (defed)
			continue;

		if (!IsIfDefed(NumIfDefs,IfDefed))
			continue;
	//msg_db_m("zzz",4);

		strcpy(Exp->Name[Exp->NumExps],Temp);
		Exp->Line[Exp->NumExps]=Exp->TempLine;
		Exp->Column[Exp->NumExps]=Exp->TempColumn;
		Exp->NumExps++;
	}
	if (NumIfDefs>0){
		DoError("\"#ifdef\" found but no matching \"#endif\"",Exp->NumExps);
		return;
	}
	Exp->ExpNr=0;
	msg_db_l(4);*/

	
	msg_db_l(4);
}

void CPreScript::ParseEnum()
{
	msg_db_r("ParseEnum", 4);
	next_exp(); // 'enum'
	if (ExpectNewline())
		_return_(4,);
	int value = 0;
	next_line();
	if (ExpectIndent())
		_return_(4,);
	for (int i=0;!end_of_file();i++){
		for (int j=0;!end_of_line();j++){
			sEnum e;
			strcpy(e.Name, cur_name);
			next_exp();

			// explicit value
			if (strcmp(cur_name, ":") == 0){
				next_exp();
				if (ExpectNoNewline())
					_return_(4,);
				sType *type = GetConstantType(cur_name);
				if (type == TypeInt)
					value = *(int*)GetConstantValue(cur_name);
				else
					_do_error_("integer constant expected after \":\" for explicit value of enum", 4,);
				next_exp();
			}
			e.Value = value ++;
			Enum.push_back(e);
			if (end_of_line())
				break;
			if ((strcmp(cur_name, ",") != 0))
				_do_error_("\",\" or newline expected after enum definition", 4,);
			next_exp();
			if (ExpectNoNewline())
				_return_(4,);
		}
		next_line();
		if (unindented)
			break;
	}
	Exp.cur_line --;
	msg_db_l(4);
}

inline bool type_needs_alignment(sType *t)
{
	if (t->ArrayLength > 0)
		return type_needs_alignment(t->SubType);
	return (t->Size >= 4);
}

void CPreScript::ParseStruct()
{
	msg_db_r("ParseStruct", 4);
	sStruct s;
	s.Owner = this;
	int _offset = 0;
	next_exp(); // 'enum'
	char name[SCRIPT_MAX_NAME * 2];
	strcpy(name, cur_name);
	next_exp();

	// parent structure
	if (strcmp(cur_name, ":") == 0){
		so("vererbung der struktur");
		next_exp();
		sType *ancestor = GetType(Exp.cur_exp, true);
		if (Error){
			msg_db_l(4);
			return;
		}
		bool found = false;
		sStruct *ss;
		foreach(Struct, ss, i)
			if (ss->RootType == ancestor){
				// inheritance of elements
				s.Element.assign(ss->Element.begin(), ss->Element.end());
				_offset = ss->RootType->Size;
				found = true;
				break;
			}
		if (!found){
			DoError(string2("parental type in structure definition after \":\" has to be a structure, but (%s) is not", ancestor->Name));
			msg_db_l(4);
			return;
		}
	}
	if (ExpectNewline()){
		msg_db_l(4);
		return;
	}

	// elements
	for (int num=0;true;num++){
		next_line();
		if (unindented)
			break;
		sType *tType = GetType(Exp.cur_exp, true);
		if (Error){
			msg_db_l(4);
			return;
		}
		for (int j=0;!end_of_line();j++){
			sStructElement el;
			bool is_pointer = false;
			sType *type = tType;
			if (strcmp(cur_name, "*") == 0){
				next_exp();
				is_pointer = true;
			}
			strcpy(el.Name, cur_name);
			next_exp();
			so(string2("Struct-Element: %s %s  Offset: %d", type->Name, el.Name, _offset));
			TestArrayDefinition(&type, is_pointer);
			el.Type = type;
			if ((strcmp(cur_name, ",") != 0) && (!end_of_line())){
				DoError("\",\" or newline expected after struct element");
				msg_db_l(4);
				return;
			}
			el.Offset = _offset;
			_offset += type->Size;
			if (type_needs_alignment(type))
				_offset = mem_align(_offset);
			s.Element.push_back(el);
			if (end_of_line())
				break;
			next_exp();
		}
	}
	for (int i=0;i<s.Element.size();i++)
		if (type_needs_alignment(s.Element[i].Type))
			_offset = mem_align(_offset);
	s.RootType = CreateNewType(name, _offset, false, false, 0, NULL);
	Struct.push_back(s);

	Exp.cur_line --;
	msg_db_l(4);
}

bool CPreScript::ExpectNoNewline()
{
	if (end_of_line()){
		DoError("unexpected newline");
		return true;
	}
	return false;
}

bool CPreScript::ExpectNewline()
{
	if (!end_of_line()){
		DoError("newline expected");
		return true;
	}
	return false;
}

bool CPreScript::ExpectIndent()
{
	if (!indented){
		DoError("additional indent expected");
		return true;
	}
	return false;
}

sType *CPreScript::ParseVariableDefSingle(sType *type, sFunction *f, bool as_param)
{
	msg_db_r("ParseVariableDefSingle", 6);
	
	bool is_pointer = false;
	char name[SCRIPT_MAX_NAME * 2];

	// pointer?
	if (strcmp(cur_name, "*") == 0){
		next_exp();
		is_pointer = true;
	}

	// name
	strcpy(name, cur_name);
	next_exp();
	so(string("Variable: ", name));

	// array?
	TestArrayDefinition(&type, is_pointer);
	if ((as_param) && (type->ArrayLength > 0)){
		// function parameter:  array -> pointer
		type = type->SubType;
		TestArrayDefinition(&type, true);
		so("C-Standart:   Array wurde in Pointer umgewandelt!!!!");
	}

	// add
	if (next_extern){
		so("extern");
		// already existing?
		bool found = false;
		for (int i=NumTruePreExternalVars;i<PreExternalVar.size();i++)
			if (strcmp(PreExternalVar[i].Name, name) == 0){
				PreExternalVar[i].Type = type;
				found = true;
				break;
			}

		// not found -> create provisorium (not linkable.... but parsable)
		if (!found){
			sPreExternalVar v;
			v.Name = new char[strlen(name) + 1];
			strcpy(v.Name, name);
			v.Pointer = NULL;
			v.Type = type;
			PreExternalVar.push_back(v);
		}
	}else{
		AddVar(name, type, f);
	}
	msg_db_l(6);
	return type;
}

void CPreScript::ParseVariableDef(bool single, sFunction *f)
{
	msg_db_r("ParseVariableDef", 4);
	sType *type = GetType(Exp.cur_exp, true);
	if (Error){
		msg_db_l(4);
		return;
	}
	
	for (int j=0;true;j++){
		if (ExpectNoNewline())
			break;

		ParseVariableDefSingle(type, f);
		if (Error)
			break;
		
		if ((strcmp(cur_name, ",") != 0) && (!end_of_line())){
			DoError("\",\" or newline expected after definition of a global variable");
			break;
		}

		// last one?
		if (end_of_line())
			break;
		
		next_exp(); // ','
	}
	msg_db_l(4);
}

void CPreScript::ParseFunction()
{
	msg_db_r("ParseFunction", 4);
	sType *type = GetType(Exp.cur_exp, true);
	if (Error){
		msg_db_l(4);
		return;
	}

	so(cur_name);
	int function = AddFunction(cur_name);
	if (Error){
		msg_db_l(4);
		return;
	}
	sFunction *f = &Function[function];
	f->Type = type;
	if (type->Size > 4)
		Function[function].ParamSize += 4;
	//Function[function]->VarSize += type->Size;
	next_exp();
	next_exp(); // '('

	// parameter list
	if (strcmp(cur_name, ")") != 0)
		for (int k=0;k<SCRIPT_MAX_PARAMS;k++){
			// like variable definitions

			f->NumParams ++;

			// type of parameter variable
			sType *param_type = GetType(Exp.cur_exp,true);
			if (Error){
				msg_db_l(4);
				return;
			}
			sType *pt = ParseVariableDefSingle(param_type, f, true);
			f->Var[f->Var.size() - 1].Type = pt;

			if (strcmp(cur_name, ")") == 0)
				break;

			if (strcmp(cur_name, ",") != 0){
				DoError("\",\" or \")\" expected after parameter");
				msg_db_l(4);
				return;
			}
			next_exp(); // ','
		}
	next_exp(); // ')'
	f->VarSize = 0;

	if (!end_of_line()){
		DoError("newline expected after parameter list");
		msg_db_l(4);
		return;
	}

	ps_line_t *this_line = Exp.cur_line;
	

	// instructions
	for (int k=0;k<SCRIPT_MAX_LINES;k++){
		next_line();
		indented = false;

		// end of file
		if (end_of_file())
			break;

		// end of function
		if (Exp.cur_line->indent <= this_line->indent)
			break;

		// command or local definition
		GetCompleteCommand(f->Block, f);
		if (Error){
			msg_db_l(4);
			return;
		}
	}

	Exp.cur_line --;
	msg_db_l(4);
}

// convert text into script data
void CPreScript::Parser()
{
	if (Error)	return;
	msg_db_r("Parser", 4);

	// create initial variables
	RootOfAllEvil.Var.clear();
	for (int i=0;i<PreGlobalVar.size();i++){
		sLocalVariable v;
		strcpy(v.Name, PreGlobalVar[i].Name);
		v.Type = PreGlobalVar[i].Type;
		RootOfAllEvil.Var.push_back(v);
	}
	strcpy(RootOfAllEvil.Name, "RootOfAllEvil");

	// syntax analysis
	Error = false;
	shift_right = 0;

	Exp.cur_line = &Exp.line[0];
	Exp.cur_exp = 0;
	reset_indent();

	// global definitions (enum, struct, variables and functions)
	while (!end_of_file()){
		if (Error)
			return;
		next_extern = false;

		// extern?
		if (strcmp(cur_name, "extern") == 0){
			next_extern = true;
			next_exp();
		}

		// enum
		if (strcmp(cur_name, "enum") == 0){
			ParseEnum();

		// struct
		}else if (strcmp(cur_name, "struct") == 0){
			ParseStruct();
			
		}else{

			// type of definition
			sType *tType = GetType(Exp.cur_exp, true);
			if (Error){
				msg_db_l(4);
				return;
			}
			Exp.cur_exp --;
			bool is_function = false;
			for (int j=1;j<Exp.cur_line->exp.size()-1;j++)
				if (strcmp(Exp.cur_line->exp[j].name, "(") == 0)
				    is_function = true;

			// own function?
			if (is_function){
				ParseFunction();
				
			// global variables
			}else{
				ParseVariableDef(false, &RootOfAllEvil);
			}
		}
		next_line();
	}

	msg_db_l(4);
}


void delete_cnst(sConstant *v)
{
	delete[](v->data);
	dm("var->data",v->data);
}

void delete_link(sLinkData *l)
{
	//msg_write(Kind2Str(l->Kind));
	if ((l->Kind==KindArray)||(l->Kind==KindPointerShift)||(l->Kind==KindDerefPointerShift)||(l->Kind==KindReference)||(l->Kind==KindDereference)){
		delete_link(l->Meta);
		delete(l->Meta);
		dm("link (meta)",l->Meta);
	}
	if (l->Kind==KindArray){
		delete_link(l->ParamLink);
		delete(l->ParamLink);
		dm("link (para)",l->ParamLink);
	}
}

#if 0
void delete_cmd(sCommand *c)
{
	/*delete_link(&c->ReturnLink);
	for (int i=0;i<c->NumParams;i++)
		delete_link(&c->ParamLink[i]);*/
	delete(c);
	dm("Command",c);
}
#endif

CPreScript::~CPreScript()
{
	msg_db_r("~CPreScript", 4);

	// delete all types created by this script
	for (int i=PreType.size();i<Type.size();i++)
		if (Type[i]->Owner==this){
			delete(Type[i]);
			dm("Type",Type[i]);
		}
	Type.clear();

	Struct.clear();
	Enum.clear();
	Define.clear();

	
	/*msg_db_m("def", 8);
	for (int i=0;i<NumDefines;i++)
		if (Define[i]->Owner==this){
			delete(Define[i]);
			dm("Define",Define[i]);
		}*/


	PreScriptRule.clear();

	
	msg_db_m("asm", 8);
	if (AsmMetaInfo){
		delete(AsmMetaInfo);
		dm("AsmMetaInfo",AsmMetaInfo);
	}
	for (int i=0;i<AsmBlock.size();i++){
		delete[](AsmBlock[i].block);
		dm("AsmBlock",AsmBlock[i].block);
	}
	AsmBlock.clear();
	
	msg_db_m("const", 8);
	for (int i=0;i<Constant.size();i++)
		delete_cnst(&Constant[i]);
	Constant.clear();

	
	msg_db_m("cmd", 8);
	//for (int i=0;i<Command.size();i++)
	//	delete_cmd(&Command[i]);
	Command.clear();

	msg_db_m("rest", 8);

	Block.clear();
	LinkData.clear();
	Function.clear();
	
	msg_db_l(4);
}


void CPreScript::ShowLink(sLinkData *link)
{
	msg_right();
	msg_write(string(Kind2Str(link->Kind),", ",LinkNr2Str(this,link->Kind,link->Nr)));
	msg_write(Type2Str(this,link->type));
	if ((link->Kind==KindPointerShift)||(link->Kind==KindArray)||(link->Kind==KindPointerAsArray)||(link->Kind==KindReference)||(link->Kind==KindDereference)){
		msg_write("Meta-Link");
		ShowLink(link->Meta);
	}
	if ((link->Kind==KindArray)||(link->Kind==KindPointerAsArray)){
		msg_write("Param-Link");
		ShowLink(link->ParamLink);
	}
	if (link->Kind==KindCommand){
		ShowCommand(link->Nr);
	}
	msg_left();
}

void CPreScript::ShowCommand(int c)
{
	msg_write(string(i2s(c),": ",Kind2Str(Command[c].Kind),", ",LinkNr2Str(this,Command[c].Kind,Command[c].LinkNr)));
	msg_right();
	msg_write(string("Return: ",Type2Str(this,Command[c].ReturnType)));
	for (int p=0;p<Command[c].NumParams;p++){
		msg_write("Parameter");
		ShowLink(&Command[c].ParamLink[p]);
	}
	if (Command[c].Kind == KindCompilerFunction)
		if (PreCommand[Command[c].LinkNr].Instance == f_class){
			msg_write("Objekt:");
			ShowCommand(Command[c].SubLink1);
		}
	msg_left();
	msg_write("");
}

void CPreScript::ShowBlock(int b)
{
	msg_write("b");
	msg_right();
	for (int c=0;c<Block[b].Command.size();c++){
		//msg_write(Block[b].Command[c]);
		if (Command[Block[b].Command[c]].Kind==KindBlock)
			ShowBlock(Command[Block[b].Command[c]].LinkNr);
		else
			ShowCommand(Block[b].Command[c]);
	}
	msg_left();
	msg_write("/b");
}

void CPreScript::ShowFunction(int f)
{
	msg_write(string(i2s(f),": ",Function[f].Name,"  --------------------------------------------"));
	ShowBlock(Function[f].Block->Nr);
}

void CPreScript::Show()
{
	//if (Error)	return;
	msg_write("\n\n\n################### Darstellung ######################\n\n\n");
	msg_write(Filename);
	/*msg_write("Befehle:\n");
	msg_right();
	for (int c=0;c<NumCommands;c++)
		ShowCommand(c);
	msg_left();*/
	msg_write("\nFunktionen:\n");
	msg_right();
	for (int f=0;f<Function.size();f++)
		ShowFunction(f);
	msg_left();
	msg_write("\n\n");
}
