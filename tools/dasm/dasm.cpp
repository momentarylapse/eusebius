#include "dasm.h"
#include "file.h"
#include "msg.h"
#include <stdio.h>

char buffer[16777216];
long ParamConstantDouble;

struct sRegister{
	char *name;
	int reg,type;
};

#define NumRegisters		(4*4 + 4*2 + 6 + 4 + 8)
sRegister Register[NumRegisters]={
	{"eax"	,eAX	,Gd},
	{"ax"	,AX		,Gw},
	{"ah"	,AH		,Gb},
	{"al"	,AL		,Gb},
	{"ecx"	,eCX	,Gd},
	{"cx"	,CX		,Gw},
	{"ch"	,CH		,Gb},
	{"cl"	,CL		,Gb},
	{"edx"	,eDX	,Gd},
	{"dx"	,DX		,Gw},
	{"dh"	,DH		,Gb},
	{"dl"	,DL		,Gb},
	{"ebx"	,eBX	,Gd},
	{"bx"	,BX		,Gw},
	{"bh"	,BH		,Gb},
	{"bl"	,BL		,Gb},

	{"esp"	,eSP	,Gd},
	{"sp"	,SP		,Gb},
	{"esi"	,eSI	,Gd},
	{"si"	,SI		,Gw},
	{"edi"	,eDI	,Gd},
	{"di"	,DI		,Gw},
	{"ebp"	,eBP	,Gd},
	{"bp"	,BP		,Gw},

	{"cs"	,CS		,Sw},
	{"ss"	,SS		,Sw},
	{"ds"	,DS		,Sw},
	{"es"	,ES		,Sw},
	{"fs"	,FS		,Sw},
	{"gs"	,GS		,Sw},

	{"cr0"	,CR0	,Cd},
	{"cr1"	,CR1	,Cd},
	{"cr2"	,CR2	,Cd},
	{"cr3"	,CR3	,Cd},
	
	{"st0"	,ST0	,-1},
	{"st1"	,ST1	,-1},
	{"st2"	,ST2	,-1},
	{"st3"	,ST3	,-1},
	{"st4"	,ST4	,-1},
	{"st5"	,ST5	,-1},
	{"st6"	,ST6	,-1},
	{"st7"	,ST7	,-1},
};

struct sInstructionName{
	int inst;
	char *name;
};

sInstructionName InstructionName[NumInstructionNames+1]={
	{inst_add,		"add"},
	{inst_add_b,	"add.b"},
	{inst_adc,		"adc"},
	{inst_adc_b,	"adc.b"},
	{inst_sub,		"sub"},
	{inst_sub_b,	"sub.b"},
	{inst_sbb,		"sbb"},
	{inst_sbb_b,	"sbb.b"},
	{inst_inc,		"inc"},
	{inst_inc_b,	"inc.b"},
	{inst_dec,		"dec"},
	{inst_dec_b,	"dec.b"},
	{inst_mul,		"mul"},
	{inst_mul_b,	"mul.b"},
	{inst_imul,		"imul"},
	{inst_imul_b,	"imul.b"},
	{inst_div,		"div"},
	{inst_div_b,	"div.b"},
	{inst_idiv,		"idiv"},
	{inst_idiv_b,	"idiv.b"},
	{inst_mov,		"mov"},
	{inst_mov_b,	"mov.b"},
	{inst_movzx,	"movzx"},
	{inst_movsx,	"movsx"},
	{inst_and,		"and"},
	{inst_and_b,	"and.b"},
	{inst_or,		"or"},
	{inst_or_b,		"or.b"},
	{inst_xor,		"xor"},
	{inst_xor_b,	"xor.b"},
	{inst_not,		"not"},
	{inst_not_b,	"not.b"},
	{inst_neg,		"neg"},
	{inst_neg_b,	"neg.b"},
	{inst_pop,		"pop"},
	{inst_popa,		"popa"},
	{inst_push,		"push"},
	{inst_pusha,	"pusha"},
	
	{inst_jo,		"jo"},
	{inst_jo_b,		"jo.b"},
	{inst_jno,		"jno"},
	{inst_jno_b,	"jno.b"},
	{inst_jb,		"jb"},
	{inst_jb_b,		"jb.b"},
	{inst_jnb,		"jnb"},
	{inst_jnb_b,	"jnb.b"},
	{inst_jz,		"jz"},
	{inst_jz_b,		"jz.b"},
	{inst_jnz,		"jnz"},
	{inst_jnz_b,	"jnz.b"},
	{inst_jbe,		"jbe"},
	{inst_jbe_b,	"jbe.b"},
	{inst_jnbe,		"jnbe"},
	{inst_jnbe_b,	"jnbe.b"},
	{inst_js,		"js"},
	{inst_js_b,		"js.b"},
	{inst_jns,		"jns"},
	{inst_jns_b,	"jns.b"},
	{inst_jp,		"jp"},
	{inst_jp_b,		"jp.b"},
	{inst_jnp,		"jnp"},
	{inst_jnp_b,	"jnp.b"},
	{inst_jl,		"jl"},
	{inst_jl_b,		"jl.b"},
	{inst_jnl,		"jnl"},
	{inst_jnl_b,	"jnl.b"},
	{inst_jle,		"jle"},
	{inst_jle_b,	"jle.b"},
	{inst_jnle,		"jnle"},
	{inst_jnle_b,	"jnle.b"},
	
	{inst_cmp,		"cmp"},
	{inst_cmp_b,	"cmp.b"},
	
	{inst_seto_b,	"seto.b"},
	{inst_setno_b,	"setno.b"},
	{inst_setb_b,	"setb.b"},
	{inst_setnb_b,	"setnb.b"},
	{inst_setz_b,	"setz.b"},
	{inst_setnz_b,	"setnz.b"},
	{inst_setbe_b,	"setbe.b"},
	{inst_setnbe_b,	"setnbe.b"},
	{inst_sets,		"sets"},
	{inst_setns,	"setns"},
	{inst_setp,		"setp"},
	{inst_setnp,	"setnp"},
	{inst_setl,		"setl"},
	{inst_setnl,	"setnl"},
	{inst_setle,	"setle"},
	{inst_setnle,	"setnle"},
	
	{inst_sldt,		"sldt"},
	{inst_str,		"str"},
	{inst_lldt,		"lldt"},
	{inst_ltr,		"ltr"},
	{inst_verr,		"verr"},
	{inst_verw,		"verw"},
	{inst_sgdt,		"sgdt"},
	{inst_sidt,		"sidt"},
	{inst_lgdt,		"lgdt"},
	{inst_lidt,		"lidt"},
	{inst_smsw,		"smsw"},
	{inst_lmsw,		"lmsw"},
	
	{inst_test,		"test"},
	{inst_test_b,	"test.b"},
	{inst_xchg,		"xchg"},
	{inst_xchg_b,	"xchg.b"},
	{inst_lea,		"lea"},
	{inst_nop,		"nop"},
	{inst_cbw_cwde,	"cbw/cwde"},
	{inst_cgq_cwd,	"cgq/cwd"},
	{inst_movs_ds_esi_es_edi,	"movs_ds:esi,es:edi"},
	{inst_movs_b_ds_esi_es_edi,	"movs.b_ds:esi,es:edi"},
	{inst_cmps_ds_esi_es_edi,	"cmps_ds:esi,es:edi"},
	{inst_cmps_b_ds_esi_es_edi,	"cmps.b_ds:esi,es:edi"},
	{inst_rol,		"rol"},
	{inst_rol_b,	"rol.b"},
	{inst_ror,		"ror"},
	{inst_ror_b,	"ror.b"},
	{inst_rcl,		"rcl"},
	{inst_rcl_b,	"rcl.b"},
	{inst_rcr,		"rcr"},
	{inst_rcr_b,	"rcr.b"},
	{inst_shl,		"shl"},
	{inst_shl_b,	"shl.b"},
	{inst_shr,		"shr"},
	{inst_shr_b,	"shr.b"},
	{inst_sar,		"sar"},
	{inst_sar_b,	"sar.b"},
	{inst_ret,		"ret"},
	{inst_leave,	"leave"},
	{inst_ret_far,	"ret_far"},
	{inst_int,		"int"},
	{inst_iret,		"iret"},
	
	{inst_fadd,		"fadd"},
	{inst_fmul,		"fmul"},
	{inst_fsub,		"fsub"},
	{inst_fdiv,		"fdiv"},
	{inst_fld,		"fld"},
	{inst_fstp,		"fstp"},
	{inst_fild,		"fild"},
	{inst_faddp,	"faddp"},
	{inst_fmulp,	"fmulp"},
	{inst_fsubp,	"fsubp"},
	{inst_fdivp,	"fdivp"},
	
	{inst_loop,		"loop"},
	{inst_loope,	"loope"},
	{inst_loopne,	"loopne"},
	{inst_in,		"in"},
	{inst_in_b,		"in.b"},
	{inst_out,		"out"},
	{inst_out_b,	"out.b"},
	
	{inst_call,		"call"},
	{inst_call_far,	"call_far"},
	{inst_jmp,		"jmp"},
	{inst_jmp_b,	"jmp.b"},
	{inst_lock,		"lock"},
	{inst_rep,		"rep"},
	{inst_repne,	"repne"},
	{inst_hlt,		"hlt"},
	{inst_cmc,		"cmc"},
	{inst_clc,		"clc"},
	{inst_stc,		"stc"},
	{inst_cli,		"cli"},
	{inst_sti,		"sti"},
	{inst_cld,		"cld"},
	{inst_std,		"std"},
	
	{-1,			"???"}
};

struct sInstruction{
	int inst;
	int code,code_alt,cap;
	int param1,param2;
	char *name;
};

#define NumInstructionsX86		319
sInstruction InstructionX86[NumInstructionsX86]={
	{inst_add_b		,0x00	,-1	,-1	,Eb	,Gb	},
	{inst_add		,0x01	,-1	,-1	,Ev	,Gv	},
	{inst_add_b		,0x02	,-1	,-1	,Gb	,Eb	},
	{inst_add		,0x03	,-1	,-1	,Gv	,Ev	},
	{inst_add_b		,0x04	,-1	,-1	,AL	,Ib	},
	{inst_add		,0x05	,-1	,-1	,eAX,Iv	},
	{inst_push		,0x06	,-1	,-1	,ES	,-1	},
	{inst_pop		,0x07	,-1	,-1	,ES	,-1	},
	{inst_or_b		,0x08	,-1	,-1	,Eb	,Gb	},
	{inst_or		,0x09	,-1	,-1	,Ev	,Gv	},
	{inst_or_b		,0x0a	,-1	,-1	,Gb	,Eb	},
	{inst_or		,0x0b	,-1	,-1	,Gv	,Ev	},
	{inst_or_b		,0x0c	,-1	,-1	,AL	,Ib	},
	{inst_or		,0x0d	,-1	,-1	,eAX,Iv	},
	{inst_push		,0x0e	,-1	,-1	,CS	,-1	},
	{inst_sldt		,0x0f	,0x00	,0	,Ew	,-1	},
	{inst_str		,0x0f	,0x00	,1	,Ew	,-1	},
	{inst_lldt		,0x0f	,0x00	,2	,Ew	,-1	},
	{inst_ltr		,0x0f	,0x00	,3	,Ew	,-1	},
	{inst_verr		,0x0f	,0x00	,4	,Ew	,-1	},
	{inst_verw		,0x0f	,0x00	,5	,Ew	,-1	},
	{inst_sgdt		,0x0f	,0x01	,0	,Ev	,-1	},
	{inst_sidt		,0x0f	,0x01	,1	,Ev	,-1	},
	{inst_lgdt		,0x0f	,0x01	,2	,Ev	,-1	},
	{inst_lidt		,0x0f	,0x01	,3	,Ev	,-1	},
	{inst_smsw		,0x0f	,0x01	,4	,Ew	,-1	},
	{inst_lmsw		,0x0f	,0x01	,6	,Ew	,-1	},
	{inst_mov		,0x0f	,0x22	,-1	,Cd	,Ed	}, // Fehler im Algorhytmus!!!!
	{inst_mov		,0x0f	,0x20	,-1	,Ed	,Cd	},
	{inst_jo		,0x0f	,0x80	,-1	,Iv	,-1	},
	{inst_jno		,0x0f	,0x81	,-1	,Iv	,-1	},
	{inst_jb		,0x0f	,0x82	,-1	,Iv	,-1	},
	{inst_jnb		,0x0f	,0x83	,-1	,Iv	,-1	},
	{inst_jz		,0x0f	,0x84	,-1	,Iv	,-1	},
	{inst_jnz		,0x0f	,0x85	,-1	,Iv	,-1	},
	{inst_jbe		,0x0f	,0x86	,-1	,Iv	,-1	},
	{inst_jnbe		,0x0f	,0x87	,-1	,Iv	,-1	},
	{inst_js		,0x0f	,0x88	,-1	,Iv	,-1	},
	{inst_jns		,0x0f	,0x89	,-1	,Iv	,-1	},
	{inst_jp		,0x0f	,0x8a	,-1	,Iv	,-1	},
	{inst_jnp		,0x0f	,0x8b	,-1	,Iv	,-1	},
	{inst_jl		,0x0f	,0x8c	,-1	,Iv	,-1	},
	{inst_jnl		,0x0f	,0x8d	,-1	,Iv	,-1	},
	{inst_jle		,0x0f	,0x8e	,-1	,Iv	,-1	},
	{inst_jnle		,0x0f	,0x8f	,-1	,Iv	,-1	},
	{inst_seto_b	,0x0f	,0x90	,-1	,Eb	,-1	},
	{inst_setno_b	,0x0f	,0x91	,-1	,Eb	,-1	},
	{inst_setb_b	,0x0f	,0x92	,-1	,Eb	,-1	},
	{inst_setnb_b	,0x0f	,0x93	,-1	,Eb	,-1	},
	{inst_setz_b	,0x0f	,0x94	,-1	,Eb	,-1	},
	{inst_setnz_b	,0x0f	,0x95	,-1	,Eb	,-1	},
	{inst_setbe_b	,0x0f	,0x96	,-1	,Eb	,-1	},
	{inst_setnbe_b	,0x0f	,0x97	,-1	,Eb	,-1	},
	{inst_sets		,0x0f	,0x98	,-1	,Ev	,-1	},
	{inst_setns		,0x0f	,0x99	,-1	,Ev	,-1	},
	{inst_setp		,0x0f	,0x9a	,-1	,Ev	,-1	},
	{inst_setnp		,0x0f	,0x9b	,-1	,Ev	,-1	},
	{inst_setl		,0x0f	,0x9c	,-1	,Ev	,-1	},
	{inst_setnl		,0x0f	,0x9d	,-1	,Ev	,-1	},
	{inst_setle		,0x0f	,0x9e	,-1	,Ev	,-1	},
	{inst_setnle	,0x0f	,0x9f	,-1	,Ev	,-1	},
	{inst_imul		,0x0f	,0xaf	,-1	,Gv	,Ev	},
	{inst_movzx		,0x0f	,0xb6	,-1	,Gv	,Eb	},
	{inst_movzx		,0x0f	,0xb7	,-1	,Gv	,Ew	},
	{inst_movsx		,0x0f	,0xbe	,-1	,Gv	,Eb	},
	{inst_movsx		,0x0f	,0xbf	,-1	,Gv	,Ew	},
	{inst_adc_b		,0x10	,-1	,-1	,Eb	,Gb	},
	{inst_adc		,0x11	,-1	,-1	,Ev	,Gv	},
	{inst_adc_b		,0x12	,-1	,-1	,Gb	,Eb	},
	{inst_adc		,0x13	,-1	,-1	,Gv	,Ev	},
	{inst_adc_b		,0x14	,-1	,-1	,AL	,Ib	},
	{inst_adc		,0x15	,-1	,-1	,eAX,Iv	},
	{inst_push		,0x16	,-1	,-1	,SS	,-1	},
	{inst_pop		,0x17	,-1	,-1	,SS	,-1	},
	{inst_sbb_b		,0x18	,-1	,-1	,Eb	,Gb	},
	{inst_sbb		,0x19	,-1	,-1	,Ev	,Gv	},
	{inst_sbb_b		,0x1a	,-1	,-1	,Gb	,Eb	},
	{inst_sbb		,0x1b	,-1	,-1	,Gv	,Ev	},
	{inst_sbb_b		,0x1c	,-1	,-1	,AL	,Ib	},
	{inst_sbb		,0x1d	,-1	,-1	,eAX	,Iv	},
	{inst_push		,0x1e	,-1	,-1	,DS	,-1	},
	{inst_pop		,0x1f	,-1	,-1	,DS	,-1	},
	{inst_and_b		,0x20	,-1	,-1	,Eb	,Gb	},
	{inst_and		,0x21	,-1	,-1	,Ev	,Gv	},
	{inst_and_b		,0x22	,-1	,-1	,Gb	,Eb	},
	{inst_and		,0x23	,-1	,-1	,Gv	,Ev	},
	{inst_and_b		,0x24	,-1	,-1	,AL	,Ib	},
	{inst_and		,0x25	,-1	,-1	,eAX	,Iv	},
	{inst_sub_b		,0x28	,-1	,-1	,Eb	,Gb	},
	{inst_sub		,0x29	,-1	,-1	,Ev	,Gv	},
	{inst_sub_b		,0x2a	,-1	,-1	,Gb	,Eb	},
	{inst_sub		,0x2b	,-1	,-1	,Gv	,Ev	},
	{inst_sub_b		,0x2c	,-1	,-1	,AL	,Ib	},
	{inst_sub		,0x2d	,-1	,-1	,eAX	,Iv	},
	{inst_xor_b		,0x30	,-1	,-1	,Eb	,Gb	},
	{inst_xor		,0x31	,-1	,-1	,Ev	,Gv	},
	{inst_xor_b		,0x32	,-1	,-1	,Gb	,Eb	},
	{inst_xor		,0x33	,-1	,-1	,Gv	,Ev	},
	{inst_xor_b		,0x34	,-1	,-1	,AL	,Ib	},
	{inst_xor		,0x35	,-1	,-1	,eAX	,Iv	},
	{inst_cmp_b		,0x38	,-1	,-1	,Eb	,Gb	},
	{inst_cmp		,0x39	,-1	,-1	,Ev	,Gv	},
	{inst_cmp_b		,0x3a	,-1	,-1	,Gb	,Eb	},
	{inst_cmp		,0x3b	,-1	,-1	,Gv	,Ev	},
	{inst_cmp_b		,0x3c	,-1	,-1	,AL	,Ib	},
	{inst_cmp		,0x3d	,-1	,-1	,eAX	,Iv	},
	{inst_inc		,0x40	,-1	,-1	,eAX	,-1	},
	{inst_inc		,0x41	,-1	,-1	,eCX	,-1	},
	{inst_inc		,0x42	,-1	,-1	,eDX	,-1	},
	{inst_inc		,0x43	,-1	,-1	,eBX	,-1	},
	{inst_inc		,0x44	,-1	,-1	,eSP	,-1	},
	{inst_inc		,0x45	,-1	,-1	,eBP	,-1	},
	{inst_inc		,0x46	,-1	,-1	,eSI	,-1	},
	{inst_inc		,0x47	,-1	,-1	,eDI	,-1	},
	{inst_dec		,0x48	,-1	,-1	,eAX	,-1	},
	{inst_dec		,0x49	,-1	,-1	,eCX	,-1	},
	{inst_dec		,0x4a	,-1	,-1	,eDX	,-1	},
	{inst_dec		,0x4b	,-1	,-1	,eBX	,-1	},
	{inst_dec		,0x4c	,-1	,-1	,eSP	,-1	},
	{inst_dec		,0x4d	,-1	,-1	,eBP	,-1	},
	{inst_dec		,0x4e	,-1	,-1	,eSI	,-1	},
	{inst_dec		,0x4f	,-1	,-1	,eDI	,-1	},
	{inst_push		,0x50	,-1	,-1	,eAX	,-1	},
	{inst_push		,0x51	,-1	,-1	,eCX	,-1	},
	{inst_push		,0x52	,-1	,-1	,eDX	,-1	},
	{inst_push		,0x53	,-1	,-1	,eBX	,-1	},
	{inst_push		,0x54	,-1	,-1	,eSP	,-1	},
	{inst_push		,0x55	,-1	,-1	,eBP	,-1	},
	{inst_push		,0x56	,-1	,-1	,eSI	,-1	},
	{inst_push		,0x57	,-1	,-1	,eDI	,-1	},
	{inst_pop		,0x58	,-1	,-1	,eAX	,-1	},
	{inst_pop		,0x59	,-1	,-1	,eCX	,-1	},
	{inst_pop		,0x5a	,-1	,-1	,eDX	,-1	},
	{inst_pop		,0x5b	,-1	,-1	,eBX	,-1	},
	{inst_pop		,0x5c	,-1	,-1	,eSP	,-1	},
	{inst_pop		,0x5d	,-1	,-1	,eBP	,-1	},
	{inst_pop		,0x5e	,-1	,-1	,eSI	,-1	},
	{inst_pop		,0x5f	,-1	,-1	,eDI	,-1	},
	{inst_pusha		,0x60	,-1	,-1	,-1	,-1	},
	{inst_popa		,0x61	,-1	,-1	,-1	,-1	},
	{inst_push		,0x68	,-1	,-1	,Iv	,-1	},
	{inst_imul		,0x69	,-1	,-1	,Ev	,Iv	},
	{inst_push		,0x6a	,-1	,-1	,Ib	,-1	},
	{inst_jo_b		,0x70	,-1	,-1	,Ib	,-1	},
	{inst_jno_b		,0x71	,-1	,-1	,Ib	,-1	},
	{inst_jb_b		,0x72	,-1	,-1	,Ib	,-1	},
	{inst_jnb_b		,0x73	,-1	,-1	,Ib	,-1	},
	{inst_jz_b		,0x74	,-1	,-1	,Ib	,-1	},
	{inst_jnz_b		,0x75	,-1	,-1	,Ib	,-1	},
	{inst_jbe_b		,0x76	,-1	,-1	,Ib	,-1	},
	{inst_jnbe_b	,0x77	,-1	,-1	,Ib	,-1	},
	{inst_js_b		,0x78	,-1	,-1	,Ib	,-1	},
	{inst_jns_b		,0x79	,-1	,-1	,Ib	,-1	},
	{inst_jp_b		,0x7a	,-1	,-1	,Ib	,-1	},
	{inst_jnp_b		,0x7b	,-1	,-1	,Ib	,-1	},
	{inst_jl_b		,0x7c	,-1	,-1	,Ib	,-1	},
	{inst_jnl_b		,0x7d	,-1	,-1	,Ib	,-1	},
	{inst_jle_b		,0x7e	,-1	,-1	,Ib	,-1	},
	{inst_jnle_b	,0x7f	,-1	,-1	,Ib	,-1	},
	// Immediate Group 1
	{inst_add_b		,0x80	,-1	,0	,Eb	,Ib	},
	{inst_or_b		,0x80	,-1	,1	,Eb	,Ib	},
	{inst_adc_b		,0x80	,-1	,2	,Eb	,Ib	},
	{inst_sbb_b		,0x80	,-1	,3	,Eb	,Ib	},
	{inst_and_b		,0x80	,-1	,4	,Eb	,Ib	},
	{inst_sub_b		,0x80	,-1	,5	,Eb	,Ib	},
	{inst_xor_b		,0x80	,-1	,6	,Eb	,Ib	},
	{inst_cmp_b		,0x80	,-1	,7	,Eb	,Ib	},
	{inst_add		,0x81	,-1	,0	,Ev	,Iv	},
	{inst_or		,0x81	,-1	,1	,Ev	,Iv	},
	{inst_adc		,0x81	,-1	,2	,Ev	,Iv	},
	{inst_sbb		,0x81	,-1	,3	,Ev	,Iv	},
	{inst_and		,0x81	,-1	,4	,Ev	,Iv	},
	{inst_sub		,0x81	,-1	,5	,Ev	,Iv	},
	{inst_xor		,0x81	,-1	,6	,Ev	,Iv	},
	{inst_cmp		,0x81	,-1	,7	,Ev	,Iv	},
	{inst_add_b		,0x83	,-1	,0	,Ev	,Ib	},
	{inst_or_b		,0x83	,-1	,1	,Ev	,Ib	},
	{inst_adc_b		,0x83	,-1	,2	,Ev	,Ib	},
	{inst_sbb_b		,0x83	,-1	,3	,Ev	,Ib	},
	{inst_and_b		,0x83	,-1	,4	,Ev	,Ib	},
	{inst_sub_b		,0x83	,-1	,5	,Ev	,Ib	},
	{inst_xor_b		,0x83	,-1	,6	,Ev	,Ib	},
	{inst_cmp_b		,0x83	,-1	,7	,Ev	,Ib	},
	{inst_test_b	,0x84	,-1	,-1	,Eb	,Gb	},
	{inst_test		,0x85	,-1	,-1	,Ev	,Gv	},
	{inst_xchg_b	,0x86	,-1	,-1	,Eb	,Gb	},
	{inst_xchg		,0x87	,-1	,-1	,Ev	,Gv	},
	{inst_mov_b		,0x88	,-1	,-1	,Eb	,Gb	},
	{inst_mov		,0x89	,-1	,-1	,Ev	,Gv	},
	{inst_mov_b		,0x8a	,-1	,-1	,Gb	,Eb	},
	{inst_mov		,0x8b	,-1	,-1	,Gv	,Ev	},
	{inst_mov		,0x8c	,-1	,-1	,Ew	,Sw	},
	{inst_lea		,0x8d	,-1	,-1	,Gv	,Ev	},
	{inst_mov		,0x8e	,-1	,-1	,Sw	,Ew	},
	{inst_pop		,0x8f	,-1	,-1	,Ev	,-1	},
	{inst_nop		,0x90	,-1	,-1	,-1	,-1	},
	{inst_xchg		,0x91	,-1	,-1	,eAX	,eCX	},
	{inst_xchg		,0x92	,-1	,-1	,eAX	,eDX	},
	{inst_xchg		,0x93	,-1	,-1	,eAX	,eBX	},
	{inst_xchg		,0x94	,-1	,-1	,eAX	,eSP	},
	{inst_xchg		,0x95	,-1	,-1	,eAX	,eBP	},
	{inst_xchg		,0x96	,-1	,-1	,eAX	,eSI	},
	{inst_xchg		,0x97	,-1	,-1	,eAX	,eDI	},
	{inst_cbw_cwde	,0x98	,-1	,-1	,-1 ,-1	},
	{inst_cgq_cwd	,0x99	,-1	,-1	,-1 ,-1	},
	{inst_mov		,0xa0	,-1	,-1	,AL	,Ov	},
	{inst_mov		,0xa1	,-1	,-1	,eAX	,Ov	},
	{inst_mov_b		,0xa2	,-1	,-1	,Ob	,AL	},
	{inst_mov		,0xa3	,-1	,-1	,Ov	,eAX	},
	{inst_movs_b_ds_esi_es_edi	,0xa4	,-1	,-1	,-1,-1	},
	{inst_movs_ds_esi_es_edi	,0xa5	,-1	,-1	,-1,-1	},
	{inst_cmps_b_ds_esi_es_edi	,0xa6	,-1	,-1	,-1,-1	},
	{inst_cmps_ds_esi_es_edi	,0xa7	,-1	,-1	,-1,-1	},
	{inst_mov_b		,0xb0	,-1	,-1	,AL	,Ib	},
	{inst_mov_b		,0xb1	,-1	,-1	,CL	,Ib	},
	{inst_mov_b		,0xb2	,-1	,-1	,DL	,Ib	},
	{inst_mov_b		,0xb3	,-1	,-1	,BL	,Ib	},
	{inst_mov_b		,0xb4	,-1	,-1	,AH	,Ib	},
	{inst_mov_b		,0xb5	,-1	,-1	,CH	,Ib	},
	{inst_mov_b		,0xb6	,-1	,-1	,DH	,Ib	},
	{inst_mov_b		,0xb7	,-1	,-1	,BH	,Ib	},
	{inst_mov		,0xb8	,-1	,-1	,eAX	,Iv	},
	{inst_mov		,0xb9	,-1	,-1	,eCX	,Iv	},
	{inst_mov		,0xba	,-1	,-1	,eDX	,Iv	},
	{inst_mov		,0xbb	,-1	,-1	,eBX	,Iv	},
	{inst_mov		,0xbc	,-1	,-1	,eSP	,Iv	},
	{inst_mov		,0xbd	,-1	,-1	,eBP	,Iv	},
	{inst_mov		,0xbe	,-1	,-1	,eSI	,Iv	},
	{inst_mov		,0xbf	,-1	,-1	,eDI	,Iv	},
	// Shift Group 2
	{inst_rol_b		,0xc0	,-1	,0	,Eb	,Ib	},
	{inst_ror_b		,0xc0	,-1	,1	,Eb	,Ib	},
	{inst_rcl_b		,0xc0	,-1	,2	,Eb	,Ib	},
	{inst_rcr_b		,0xc0	,-1	,3	,Eb	,Ib	},
	{inst_shl_b		,0xc0	,-1	,4	,Eb	,Ib	},
	{inst_shr_b		,0xc0	,-1	,5	,Eb	,Ib	},
	{inst_sar_b		,0xc0	,-1	,7	,Eb	,Ib	},
	{inst_rol		,0xc1	,-1	,0	,Ev	,Ib	}, // Ib, auch wenn die Tabelle Iv sagt!!!!
	{inst_ror		,0xc1	,-1	,1	,Ev	,Ib	},
	{inst_rcl		,0xc1	,-1	,2	,Ev	,Ib	},
	{inst_rcr		,0xc1	,-1	,3	,Ev	,Ib	},
	{inst_shl		,0xc1	,-1	,4	,Ev	,Ib	},
	{inst_shr		,0xc1	,-1	,5	,Ev	,Ib	},
	{inst_sar		,0xc1	,-1	,7	,Ev	,Ib	},
	{inst_ret		,0xc2	,-1	,-1	,Iw	,-1	},
	{inst_ret		,0xc3	,-1	,-1	,-1	,-1	},
	{inst_mov_b		,0xc6	,-1	,-1	,Eb	,Ib	},
	{inst_mov		,0xc7	,-1	,-1	,Ev	,Iv	},
	{inst_leave		,0xc9	,-1	,-1	,-1	,-1	},
	{inst_ret_far	,0xca	,-1	,-1	,Iw	,-1	},
	{inst_ret_far	,0xcb	,-1	,-1	,-1	,-1	},
	{inst_int		,0xcd	,-1	,-1	,Ib	,-1	},
	{inst_iret		,0xcf	,-1	,-1	,-1	,-1	},
	{inst_rol		,0xd3	,-1	,0	,Ev	,CL	},
	{inst_ror		,0xd3	,-1	,1	,Ev	,CL	},
	{inst_rcl		,0xd3	,-1	,2	,Ev	,CL	},
	{inst_rcr		,0xd3	,-1	,3	,Ev	,CL	},
	{inst_shl		,0xd3	,-1	,4	,Ev	,CL	},
	{inst_shr		,0xd3	,-1	,5	,Ev	,CL	},
	{inst_sar		,0xd3	,-1	,7	,Ev	,CL	},
	{inst_fadd		,0xd8	,-1	,0	,Ev	,-1	},
	{inst_fmul		,0xd8	,-1	,1	,Ev	,-1	},
	{inst_fsub		,0xd8	,-1	,4	,Ev	,-1	},
	{inst_fdiv		,0xd8	,-1	,6	,Ev	,-1	},
	{inst_fld		,0xd9	,-1	,0	,Ev	,-1	},
	{inst_fstp		,0xd9	,-1	,3	,Ev	,-1	},
	{inst_fild		,0xdb	,-1	,0	,Ev	,-1	},
	{inst_faddp		,0xde	,-1	,0	,Ev	,-1	},
	{inst_fmulp		,0xde	,-1	,1	,Ev	,-1	},
	{inst_fsubp		,0xde	,-1	,5	,Ev	,-1	},
	{inst_fdivp		,0xde	,-1	,7	,Ev	,-1	}, // de.f9 ohne Parameter...?
	{inst_loopne	,0xe0	,-1	,-1	,Ib	,-1	},
	{inst_loope		,0xe1	,-1	,-1	,Ib	,-1	},
	{inst_loop		,0xe2	,-1	,-1	,Ib	,-1	},
	{inst_in_b		,0xe4	,-1	,-1	,AL	,Ib	},
	{inst_in_b		,0xe5	,-1	,-1	,eAX,Ib	},
	{inst_out_b		,0xe6	,-1	,-1	,Ib	,AL	},
	{inst_out_b		,0xe7	,-1	,-1	,Ib	,eAX},
	{inst_call		,0xe8	,-1	,-1	,Iv	,-1	},
	{inst_jmp		,0xe9	,-1	,-1	,Iv	,-1	},
	{inst_jmp		,0xea	,-1	,-1	,Ip	,-1	},
	{inst_jmp_b		,0xeb	,-1	,-1	,Ib	,-1	},
	{inst_in		,0xec	,-1	,-1	,AL	,DX	},
	{inst_in		,0xed	,-1	,-1	,eAX,DX	},
	{inst_out		,0xee	,-1	,-1	,DX	,AL	},
	{inst_out		,0xef	,-1	,-1	,DX	,eAX},
	{inst_lock		,0xf0	,-1	,-1	,-1	,-1	},
	{inst_repne		,0xf2	,-1	,-1	,-1	,-1	},
	{inst_rep		,0xf3	,-1	,-1	,-1	,-1	},
	{inst_hlt		,0xf4	,-1	,-1	,-1	,-1	},
	{inst_cmc		,0xf5	,-1	,-1	,-1	,-1	},
	// Unary Group 3
	{inst_test_b	,0xf6	,-1	,0	,Eb	,Ib	},
	{inst_not_b		,0xf6	,-1	,2	,Eb	,-1	},
	{inst_neg_b		,0xf6	,-1	,3	,Eb	,-1	},
	{inst_mul_b		,0xf6	,-1	,4	,AL	,Eb	},
	{inst_imul_b	,0xf6	,-1	,5	,AL	,Eb	},
	{inst_div_b		,0xf6	,-1	,6	,AL	,Eb	},
	{inst_idiv_b	,0xf6	,-1	,7	,Eb	,-1	},
	{inst_test		,0xf7	,-1	,0	,Ev	,Iv	},
	{inst_not		,0xf7	,-1	,2	,Ev	,-1	},
	{inst_neg		,0xf7	,-1	,3	,Ev	,-1	},
	{inst_mul		,0xf7	,-1	,4	,eAX	,Ev	},
	{inst_imul		,0xf7	,-1	,5	,eAX	,Ev	},
	{inst_div		,0xf7	,-1	,6	,eAX	,Ev	},
	{inst_idiv		,0xf7	,-1	,7	,eAX	,Ev	},
	{inst_clc		,0xf8	,-1	,-1	,-1	,-1	},
	{inst_stc		,0xf9	,-1	,-1	,-1	,-1	},
	{inst_cli		,0xfa	,-1	,-1	,-1	,-1	},
	{inst_sti		,0xfb	,-1	,-1	,-1	,-1	},
	{inst_cld		,0xfc	,-1	,-1	,-1	,-1	},
	{inst_std		,0xfd	,-1	,-1	,-1	,-1	},
	{inst_inc_b		,0xfe	,-1	,0	,Eb	,-1	},
	{inst_dec_b		,0xfe	,-1	,1	,Eb	,-1	},
	{inst_push		,0xff	,-1	,6	,Ev	,-1	},
	{inst_inc		,0xff	,-1	,0	,Ev	,-1	},
	{inst_dec		,0xff	,-1	,1	,Ev	,-1	},
	{inst_call		,0xff	,-1	,2	,Ev	,-1	},
	{inst_call_far	,0xff	,-1	,3	,Ev	,-1	}, // Ep statt Ev...
	{inst_jmp		,0xff	,-1	,4	,Ev	,-1	},
	{inst_push		,0xff	,-1	,5	,Ev	,-1	},
};

int NumInstructions=0;
sInstruction *Instruction=NULL;

void SetInstructionSet(int set)
{
	if (set==InstructionSetDefault){
		if (sizeof(long)==8)
			set=InstructionSetAMD64;
		else
			set=InstructionSetX86;
	}
	
	set=InstructionSetX86;
	
	if (set==InstructionSetX86){
		NumInstructions=NumInstructionsX86;
		Instruction=InstructionX86;
		msg_write("--------------------------------x86");
	}
	
	// build name table
	for (int i=0;i<NumInstructions;i++){
		Instruction[i].name==InstructionName[NumInstructionNames].name;
		for (int j=0;j<NumInstructionNames;j++)
			if (Instruction[i].inst==InstructionName[j].inst)
				Instruction[i].name=InstructionName[j].name;
	}
}

static bool mode16;
static bool small_param;
static bool small_add;

// convert an asm parameter into a human readable expression
void AddParam(char *str,int param,int disp)
{
	for (int i=0;i<NumRegisters;i++)
		if (param==Register[i].reg){
			strcat(str,Register[i].name);
			return;
		}
	switch(param){
		case peAX:	strcat(str,"[eax]");	break;
		case peCX:	strcat(str,"[ecx]");	break;
		case peDX:	strcat(str,"[edx]");	break;
		case peBX:	strcat(str,"[ebx]");	break;
		case pp:	strcat(str,"[--][--]");	break;
		case disp8:	strcat(str,string("[",d2h((char*)&disp,1),"]"));	break;
		case disp16:	strcat(str,string("[",d2h((char*)&disp,2),"]"));	break;
		case disp32:	strcat(str,string("[",d2h((char*)&disp,4),"]"));	break;
		case peSI:	strcat(str,"[esi]");	break;
		case peDI:	strcat(str,"[edi]");	break;
		case pBX_pSI:	strcat(str,"[bx + si]");		break;
		case pBX_pDI:	strcat(str,"[bx + di]");		break;
		case pBP_pSI:	strcat(str,"[bp + si]");		break;
		case pBP_pDI:	strcat(str,"[bp + di]");		break;
		case pBX:	strcat(str,"[bx]");		break;
		case pSI:	strcat(str,"[si]");		break;
		case pDI:	strcat(str,"[di]");		break;

		case d8_peAX:	strcat(str,string("[eax + ",d2h((char*)&disp,1),"]"));	break;
		case d8_peCX:	strcat(str,string("[ecx + ",d2h((char*)&disp,1),"]"));	break;
		case d8_peDX:	strcat(str,string("[edx + ",d2h((char*)&disp,1),"]"));	break;
		case d8_peBX:	strcat(str,string("[ebx + ",d2h((char*)&disp,1),"]"));	break;
		case d8_pp:	strcat(str,string("[--][-- + ",d2h((char*)&disp,1),"]"));	break;
		case d8_peBP:	strcat(str,string("[ebp + ",d2h((char*)&disp,1),"]"));	break;
		case d8_peSI:	strcat(str,string("[esi + ",d2h((char*)&disp,1),"]"));	break;
		case d8_peDI:	strcat(str,string("[edi + ",d2h((char*)&disp,1),"]"));	break;

		case d8_pBX:	strcat(str,string("[bx + ",d2h((char*)&disp,1),"]"));	break;
		case d8_pBP:	strcat(str,string("[bp + ",d2h((char*)&disp,1),"]"));	break;
		case d8_pSI:	strcat(str,string("[si + ",d2h((char*)&disp,1),"]"));	break;
		case d8_pDI:	strcat(str,string("[di + ",d2h((char*)&disp,1),"]"));	break;
	
		case d16_pBX_pSI:	strcat(str,string("[bx + si + ",d2h((char*)&disp,2),"]"));	break;
		case d16_pBX_pDI:	strcat(str,string("[bx + di + ",d2h((char*)&disp,2),"]"));	break;
		case d16_pBP_pSI:	strcat(str,string("[bp + si + ",d2h((char*)&disp,2),"]"));	break;
		case d16_pBP_pDI:	strcat(str,string("[bp + di + ",d2h((char*)&disp,2),"]"));	break;
		case d16_pBX:	strcat(str,string("[bx + ",d2h((char*)&disp,2),"]"));	break;
		case d16_pBP:	strcat(str,string("[bp + ",d2h((char*)&disp,2),"]"));	break;
		case d16_pSI:	strcat(str,string("[si + ",d2h((char*)&disp,2),"]"));	break;
		case d16_pDI:	strcat(str,string("[di + ",d2h((char*)&disp,2),"]"));	break;

		case d32_peAX:	strcat(str,string("[eax + ",d2h((char*)&disp,4),"]"));	break;
		case d32_peCX:	strcat(str,string("[ecx + ",d2h((char*)&disp,4),"]"));	break;
		case d32_peDX:	strcat(str,string("[edx + ",d2h((char*)&disp,4),"]"));	break;
		case d32_peBX:	strcat(str,string("[ebx + ",d2h((char*)&disp,4),"]"));	break;
		case d32_pp:	strcat(str,string("[--][-- + ",d2h((char*)&disp,4),"]"));	break;
		case d32_peBP:	strcat(str,string("[ebp + ",d2h((char*)&disp,4),"]"));	break;
		case d32_peSI:	strcat(str,string("[esi + ",d2h((char*)&disp,4),"]"));	break;
		case d32_peDI:	strcat(str,string("[edi + ",d2h((char*)&disp,4),"]"));	break;

		case Ib:	strcat(str,d2h((char*)&disp,1));	break;
		case Iw:	strcat(str,d2h((char*)&disp,2));	break;
		case Id:	strcat(str,d2h((char*)&disp,4));	break;
		case Ip:
			if ((small_param)&&(!mode16))	strcat(str,"word ");
			if ((!small_param)&&(mode16))	strcat(str,"dword ");
			strcat(str,d2h((char*)&ParamConstantDouble,2));
			strcat(str,":");
			if (small_param)	strcat(str,d2h((char*)&disp,2));
			else				strcat(str,d2h((char*)&disp,4));
			break;

		default:	strcat(str,string(i2s(param),": -\?\?- "));	break;
	};
}

void CorrectParam(int &p,bool small_param)
{
	if (small_param){
		if (p==Iv)	p=Iw;
		if (p==Ev)	p=Ew;
		if (p==Gv)	p=Gw;
		if (p==Ov)	p=Ow;
		if (p==eAX)	p=AX;
		if (p==eCX)	p=CX;
		if (p==eDX)	p=DX;
		if (p==eBX)	p=BX;
		if (p==eSP)	p=SP;
		if (p==eBP)	p=BP;
		if (p==eSI)	p=SI;
		if (p==eDI)	p=DI;
	}else{
		if (p==Iv)	p=Id;
		if (p==Ev)	p=Ed;
		if (p==Gv)	p=Gd;
		if (p==Ov)	p=Od;
	}
}

// convert some opcode into (human readable) assembler language
char *Opcode2Asm(char *code,int length,bool allow_comments)
{
	if (!Instruction)
		SetInstructionSet(InstructionSetDefault);
	
	char param[256],*opcode;
	strcpy(buffer,"");
	char *end=code+length;
	char *orig=code;
	if (length<0)	end=code+65536;
	char *cur=code;
	mode16=false;
	if (CurrentAsmMetaInfo)
		mode16=CurrentAsmMetaInfo->Mode16;
	while((long)code<(long)end){
		opcode=cur;
		bool done=false;
		if (CurrentAsmMetaInfo){
			for (int i=0;i<CurrentAsmMetaInfo->NumLabels;i++)
				if ((long)code-(long)orig==CurrentAsmMetaInfo->LabelPos[i]){
					strcat(buffer,"    ");
					strcat(buffer,CurrentAsmMetaInfo->LabelName[i]);
					strcat(buffer,":\n");
				}
			for (int i=0;i<CurrentAsmMetaInfo->NumDatas;i++)
				if ((long)code-(long)orig==CurrentAsmMetaInfo->DataPos[i]){
					if (CurrentAsmMetaInfo->DataSize[i]==1){
						strcat(buffer,"  db\t");
						strcat(buffer,d2h(cur,1));
					}else if (CurrentAsmMetaInfo->DataSize[i]==2){
						strcat(buffer,"  dw\t");
						strcat(buffer,d2h(cur,2));
					}else if (CurrentAsmMetaInfo->DataSize[i]==4){
						strcat(buffer,"  dd\t");
						strcat(buffer,d2h(cur,4));
					}else{
						strcat(buffer,"  ds \t...");
					}
					cur+=CurrentAsmMetaInfo->DataSize[i]-1;
					strcat(buffer,"\n");
					done=true;
				}
			for (int i=0;i<CurrentAsmMetaInfo->NumBitChanges;i++)
				if ((long)code-(long)orig==CurrentAsmMetaInfo->BitChangePos[i]){
					mode16=(CurrentAsmMetaInfo->BitChange[i]==16);
					if (mode16)
						strcat(buffer,"   bits_16\n");
					else
						strcat(buffer,"   bits_32\n");
				}
		}
		if (!done){
		small_param=mode16;
		small_add=mode16;
		int seg=-1;
		int ae=-1;
		if (cur[0]==0x67){	small_add=!mode16;		cur++;	}
		if (cur[0]==0x66){	small_param=!mode16;	cur++;	}
		if (cur[0]==0x2e){	seg=CS;	cur++;	}
		else if (cur[0]==0x36){	seg=SS;	cur++;	}
		else if (cur[0]==0x3e){	seg=DS;	cur++;	}
		else if (cur[0]==0x26){	seg=ES;	cur++;	}
		else if (cur[0]==0x64){	seg=FS;	cur++;	}
		else if (cur[0]==0x65){	seg=GS;	cur++;	}
		opcode=cur;
		for (int i=0;i<NumInstructions;i++)
			if ((unsigned char)Instruction[i].code==(unsigned char)cur[0]){
				if (Instruction[i].code_alt!=-1){
					if ((unsigned char)Instruction[i].code_alt==(unsigned char)cur[1]){
						ae=i;
						cur++;
						break;
					}
				}else if (Instruction[i].cap!=-1){
					if ((unsigned char)Instruction[i].cap==((unsigned)cur[1]/8)%8){
						ae=i;
						break;
					}
				}else{
					ae=i;
					break;
				}
			}
		int disp1,disp2;
		if (ae>=0){
			strcpy(param,"");
			int p1=Instruction[ae].param1,p2=Instruction[ae].param2;
			CorrectParam(p1,small_param);
			CorrectParam(p2,small_param);
			int pp1=p1,pp2=p2;
			bool e_first=false;
			char modRM;
			int mod,Reg,rm;
			if ((p1>=0)||(p2>=0)){
				if ((p1==Ed)||(p1==Ew)||(p1==Eb)||(p1==Sw)||(p2==Ed)||(p2==Ew)||(p2==Eb)||(p2==Sw)){
					e_first=((p1==Ed)||(p1==Ew)||(p1==Eb)) && (p1!=Sw);
					if (!e_first){	int t=p1;		p1=p2;			p2=t;	}
					if (!e_first){	int t=pp1;		pp1=pp2;		pp2=t;	}
					if (!e_first){	int t=disp1;	disp1=disp2;	disp2=t;	}
					cur++;
					modRM=cur[0];
					mod=(unsigned char)modRM/64;
					Reg=((unsigned char)modRM/8)%8;
					rm=(unsigned char)modRM%8;
					if (mod==0){
						if (mode16){
							if (rm==0)	pp1=pBX_pSI;
							if (rm==1)	pp1=pBX_pDI;
							if (rm==2)	pp1=pBP_pSI;
							if (rm==3)	pp1=pBP_pDI;
							if (rm==4)	pp1=pSI;
							if (rm==5)	pp1=pDI;
							if (rm==6){	pp1=disp16;	disp1=*(short*)&cur[1];	cur+=2;	}
							if (rm==7)	pp1=pBX;
						}else{
							if (rm==0)	pp1=peAX;
							if (rm==1)	pp1=peCX;
							if (rm==2)	pp1=peDX;
							if (rm==3)	pp1=peBX;
							if (rm==4){	pp1=pp;		disp1=cur[1];		cur++;	}
							if (rm==5){	pp1=disp32;	disp1=*(int*)&cur[1];	cur+=4; }
							if (rm==6)	pp1=peSI;
							if (rm==7)	pp1=peDI;
						}
					}else if (mod==1){
						if (mode16){
							if (rm==0)	pp1=d8_pBX_pSI;
							if (rm==1)	pp1=d8_pBX_pDI;
							if (rm==2)	pp1=d8_pBP_pSI;
							if (rm==3)	pp1=d8_pBP_pDI;
							if (rm==4)	pp1=d8_pSI;
							if (rm==5)	pp1=d8_pDI;
							if (rm==6)	pp1=d8_pBP;
							if (rm==7)	pp1=d8_pBX;
							disp1=cur[1];	cur++;
						}else{
							if (rm==0)	pp1=d8_peAX;
							if (rm==1)	pp1=d8_peCX;
							if (rm==2)	pp1=d8_peDX;
							if (rm==3)	pp1=d8_peBX;
							if (rm==4){	pp1=d8_pp;		disp1=cur[1];		cur++;	}
							if (rm==5)	pp1=d8_peBP;
							if (rm==6)	pp1=d8_peSI;
							if (rm==7)	pp1=d8_peDI;
							disp1=cur[1];	cur++;
						}
					}else if (mod==2){
						if (mode16){
							if (rm==0)	pp1=d16_pBX_pSI;
							if (rm==1)	pp1=d16_pBX_pDI;
							if (rm==2)	pp1=d16_pBP_pSI;
							if (rm==3)	pp1=d16_pBP_pDI;
							if (rm==4)	pp1=d16_pSI;
							if (rm==5)	pp1=d16_pDI;
							if (rm==6)	pp1=d16_pBP;
							if (rm==7)	pp1=d16_pBX;
							disp1=*(short*)&cur[1];	cur+=2;
						}else{
							if (rm==0)	pp1=d32_peAX;
							if (rm==1)	pp1=d32_peCX;
							if (rm==2)	pp1=d32_peDX;
							if (rm==3)	pp1=d32_peBX;
							if (rm==4){	pp1=d32_pp;		disp1=cur[1];		cur++;	}
							if (rm==5)	pp1=d32_peBP;
							if (rm==6)	pp1=d32_peSI;
							if (rm==7)	pp1=d32_peDI;
							disp1=*(int*)&cur[1];	cur+=4;
						}
					}else if (mod==3){
						if (p1==Eb){
							if (rm==0)	pp1=AL;
							if (rm==1)	pp1=CL;
							if (rm==2)	pp1=DL;
							if (rm==3)	pp1=BL;
							if (rm==4)	pp1=AH;
							if (rm==5)	pp1=CH;
							if (rm==6)	pp1=DH;
							if (rm==7)	pp1=BH;
						}else if (p1==Ew){
							if (rm==0)	pp1=AX;
							if (rm==1)	pp1=CX;
							if (rm==2)	pp1=DX;
							if (rm==3)	pp1=BX;
							if (rm==4)	pp1=SP;
							if (rm==5)	pp1=BP;
							if (rm==6)	pp1=SI;
							if (rm==7)	pp1=DI;
						}else if (p1==Ed){
							if (rm==0)	pp1=eAX;
							if (rm==1)	pp1=eCX;
							if (rm==2)	pp1=eDX;
							if (rm==3)	pp1=eBX;
							if (rm==4)	pp1=eSP;
							if (rm==5)	pp1=eBP;
							if (rm==6)	pp1=eSI;
							if (rm==7)	pp1=eDI;
						}
					}
					if (p2==Gb){
						if (Reg==0)	pp2=AL;
						if (Reg==1)	pp2=CL;
						if (Reg==2)	pp2=DL;
						if (Reg==3)	pp2=BL;
						if (Reg==4)	pp2=AH;
						if (Reg==5)	pp2=CH;
						if (Reg==6)	pp2=DH;
						if (Reg==7)	pp2=BH;
					}else if (p2==Gw){
						if (Reg==0)	pp2=AX;
						if (Reg==1)	pp2=CX;
						if (Reg==2)	pp2=DX;
						if (Reg==3)	pp2=BX;
						if (Reg==4)	pp2=SP;
						if (Reg==5)	pp2=BP;
						if (Reg==6)	pp2=SI;
						if (Reg==7)	pp2=DI;
					}else if (p2==Gd){
						if (Reg==0)	pp2=eAX;
						if (Reg==1)	pp2=eCX;
						if (Reg==2)	pp2=eDX;
						if (Reg==3)	pp2=eBX;
						if (Reg==4)	pp2=eSP;
						if (Reg==5)	pp2=eBP;
						if (Reg==6)	pp2=eSI;
						if (Reg==7)	pp2=eDI;
					}else if (p2==Sw){
						if (Reg==0)	pp2=ES;
						if (Reg==1)	pp2=CS;
						if (Reg==2)	pp2=SS;
						if (Reg==3)	pp2=DS;
						if (Reg==4)	pp2=FS;
						if (Reg==5)	pp2=GS;
					}else if (p2==Cd){
						if (Reg==0)	pp2=CR0;
						if (Reg==1)	pp2=CR1;
						if (Reg==2)	pp2=CR2;
						if (Reg==3)	pp2=CR3;
					}else if ((p2==Ob)||(p2==Ow)||(p2==Od)){
						if (small_param){
							pp2=disp16;
							disp2=*(short*)&cur[1];	cur+=2;
						}else{
							pp2=disp32;
							disp2=*(int*)&cur[1];	cur+=4;
						}
					}

					if (!e_first){	int t=pp1;	pp1=pp2;	pp2=t;	}
					if (!e_first){	int t=p1;	p1=p2;		p2=t;	}
					if (!e_first){	int t=disp1;	disp1=disp2;	disp2=t;	}
				}else if ((p1==Ob)||(p1==Ow)||(p1==Od)){
					if (small_param){
						pp1=disp16;
						disp1=*(short*)&cur[1];	cur+=2;
					}else{
						pp1=disp32;
						disp1=*(int*)&cur[1];	cur+=4;
					}
				}else if (p1==Ip){
					if (small_param){	disp1=*(short*)&cur[1];	cur+=2;	}
					else{				disp1=*(int*)&cur[1];	cur+=4;	}
					ParamConstantDouble=*(short*)&cur[1];	cur+=2;
				}else if (p1==Id){
					disp1=*(int*)&cur[1];	cur+=4;
				}else if (p1==Iw){
					disp1=*(short*)&cur[1];	cur+=2;
				}else if (p1==Ib){
					disp1=cur[1];	cur++;
				}
				// Param2
				if ((p2==Ob)||(p2==Ow)||(p2==Od)){
					if (small_param){
						pp2=disp16;
						disp2=*(short*)&cur[1];	cur+=2;
					}else{
						pp2=disp32;
						disp2=*(int*)&cur[1];	cur+=4;
					}
				}else if (p2==Id){
					disp2=*(int*)&cur[1];	cur+=4;
				}else if (p2==Iw){
					disp2=*(short*)&cur[1];	cur+=2;
				}else if (p2==Ib){
					disp2=cur[1];	cur++;
				}
				//for (int i=0;i<32-l;i++)
				//	strcat(str," ");
				strcat(param," ");
				AddParam(param,pp1,disp1);
				if (p2>=0){
					strcat(param,", ");
					AddParam(param,pp2,disp2);
				}
			}
			char str[128];		strcpy(str,"");
			if (seg==CS)	strcat(str,"CS: ");
			if (seg==SS)	strcat(str,"SS: ");
			if (seg==DS)	strcat(str,"DS: ");
			if (seg==ES)	strcat(str,"ES: ");
			if (seg==FS)	strcat(str,"FS: ");
			if (seg==GS)	strcat(str,"GS: ");
			strcat(str,string(Instruction[ae].name,param));
			strcat(buffer,str);
			if (allow_comments){
				int l=strlen(str);
				strcat(buffer," ");
				for (int ii=0;ii<48-l;ii++)
					strcat(buffer," ");
				strcat(buffer,"// ");
				strcat(buffer,d2h(code,1+long(cur)-long(code),false));
			}
		}else
			strcat(buffer,string("????? -                          unknown         ",d2h(code,1+long(cur)-long(code),false)));
		strcat(buffer,"\n");
		}
		cur++;
		code=cur;
		if ((length<0)&&((opcode[0]==char(0xc3))||(opcode[0]==char(0xc2))))
			break;
	}
	return buffer;
}

bool DebugAsm=false;

static void so(char *str)
{
	if (DebugAsm)
		printf("%s\n",str);
}

static void so(int i)
{
	if (DebugAsm)
		printf("%d\n",i);
}

char *code_buffer,mne[128];
bool EndOfLine;
bool EndOfCode;
sAsmMetaInfo *CurrentAsmMetaInfo=NULL;
bool use_mode16=false;
int LineNo;

void AsmError(char *str)
{
	msg_error(string2("%s\nline %d",str,LineNo+1));
}

// returns one "word" in the source code
char *GetMne(int &pos)
{
	EndOfLine=false;
	bool CommentLine=false;
	int i;
	strcpy(mne,"");

	// ignore comments and "white space"
	for (i=0;i<1024;i++){
		if (code_buffer[pos+i]==0){
			EndOfCode=true;
			EndOfLine=true;
			return mne;
		}
		if (code_buffer[pos+i]=='\n'){
			LineNo++;
			CommentLine=false;
		}
		// "white space"
		if ((code_buffer[pos+i]=='\n')||(code_buffer[pos+i]==' ')||(code_buffer[pos+i]=='\t'))
			continue;
		// comments
		if ((code_buffer[pos+i]==';') || ((code_buffer[pos+i]=='/')&&(code_buffer[pos+i+1]=='/'))){
			CommentLine=true;
			continue;
		}
		if (!CommentLine)
			break;
	}
	
	pos+=i;
	bool in_string=false;
	for (i=0;i<128;i++){
		mne[i]=code_buffer[pos+i];
		
		// string like stuff
		if ((mne[i]=='\'')||(mne[i]=='\"'))
			in_string=!in_string;
		// end of code
		if (code_buffer[pos+i]==0){
			EndOfCode=true;
			EndOfLine=true;
			break;
		}
		// end of line
		if (code_buffer[pos+i]=='\n'){
			LineNo++;
			EndOfLine=true;
			break;
		}
		if (!in_string){
			// "white space"
			if ((code_buffer[pos+i]==' ')||(code_buffer[pos+i]=='\t')||(code_buffer[pos+i]==',')){
				// end of line?
				for (int j=0;j<12;j++){
					if ((code_buffer[pos+i+j]!=' ')&&(code_buffer[pos+i+j]!='\t')&&(code_buffer[pos+i+j]!=',')){
						if ((code_buffer[pos+i+j]==0)||(code_buffer[pos+i+j]=='\n'))
							EndOfLine=true;
						// comment ending the line
						if ((code_buffer[pos+i+j]==';') || ((code_buffer[pos+i+j]=='/')&&(code_buffer[pos+i+j+1]=='/')))
							EndOfLine=true;
						break;
					}
				}
				break;
			}
		}
	}
	pos+=i+1;
	mne[i]=0;
	/*msg_write>Write(mne);
	if (EndOfLine)
		msg_write>Write("    eol");*/
	return mne;
}

static bool ConstantIsLabel,UnknownLabel;

// interpret an expression from source code as an assembler parameter
void GetParam(long &pk,long &p,char *param,int pn)
{
	pk=PKInvalid;
	if (strlen(param)>0){
		if ((param[0]=='[')&&(param[strlen(param)-1]==']')){
			if (DebugAsm)
				printf("Deref:   ");
			char deref[128];
			strcpy(deref,&param[1]);
			deref[strlen(deref)-1]=0;
			long drpk,drp;
			//bool u16=use_mode16;
			GetParam(drpk,drp,deref,pn);
			if (drpk==PKRegister){
				pk=PKDerefRegister;
				p=drp;
			}
			if (drpk==PKConstant){
				pk=PKDerefConstant;
				p=drp;
			}
			//use_mode16=u16;
		}
		for (int i=0;i<NumRegisters;i++)
			if (strcmp(Register[i].name,param)==0){
				pk=PKRegister;
				p=i;
				int r=Register[i].reg;
				//if ((r==AX)||(r==CX)||(r==DX)||(r==BX)||(r==SP)||(r==BP)||(r==SI)||(r==DI))
				//	use_mode16=true;
				if (DebugAsm)
					printf("Register:  %s\n",Register[i].name);
				return;
			}
		if ((param[0]=='\"')&&(param[strlen(param)-1]=='\"')){
			if (DebugAsm)
				printf("String:   ");
			char str[4096];
			strcpy(str,&param[1]);
			str[strlen(str)-1]=0;
			char *ps=new char[strlen(str)+1];
			strcpy(ps,str);
			p=(long)ps;
			pk=PKConstant;
		}
		if ((param[0]=='0')&&(param[1]=='x')){
			p=0;
			pk=PKConstant;
			for (int i=2;(unsigned)i<strlen(param);i++){
				if ((param[i]>='a')&&(param[i]<='f')){
					p*=16;
					p+=param[i]-'a'+10;
				}else if ((param[i]>='A')&&(param[i]<='F')){
					p*=16;
					p+=param[i]-'A'+10;
				}else if ((param[i]>='0')&&(param[i]<='9')){
					p*=16;
					p+=param[i]-'0';
				}else if (param[i]==':'){
					long pk2;
					GetParam(pk2,ParamConstantDouble,&param[i+1],2);
					if (pk2!=PKConstant){
						AsmError(string("error in hex parameter:  ",param));
						pk=PKInvalid;
						return;						
					}
					pk=PKConstantDouble;
					break;
				}else{
					AsmError(string("evil character in hex parameter:  ",param));
					pk=PKInvalid;
					return;
				}
			}
			if (DebugAsm)
				if (pk==PKConstantDouble)
					printf("hex const:  %s:%s\n",d2h((char*)&p,2),d2h((char*)&ParamConstantDouble,4));
				else
					printf("hex const:  %s\n",d2h((char*)&p,4));
			return;
		}
		if ((param[0]=='\'')&&(param[strlen(param)-1]=='\'')){
			p=param[1];
			pk=PKConstant;
			if (DebugAsm)
				printf("hex const:  %s\n",d2h((char*)&p,4));
		}
		if (strcmp(param,"$")==0){
			p=CurrentAsmMetaInfo->CurrentOpcodePos+CurrentAsmMetaInfo->CodeOrigin;
			pk=PKConstant;
			ConstantIsLabel=true;
			if (DebugAsm)
				printf("label:  %s\n",param);
			return;
		}
		if (CurrentAsmMetaInfo){
			// existing label
			for (int i=0;i<CurrentAsmMetaInfo->NumLabels;i++)
				if (strcmp(CurrentAsmMetaInfo->LabelName[i],param)==0){
					p=CurrentAsmMetaInfo->LabelPos[i]+CurrentAsmMetaInfo->CodeOrigin;
					pk=PKConstant;
					ConstantIsLabel=true;
					if (DebugAsm)
						printf("label:  %s\n",param);
					return;
				}
			// C-Script variable (global)
			for (int i=0;i<*(CurrentAsmMetaInfo->NumGlobalVars);i++){
				char *name=&CurrentAsmMetaInfo->GlobalVarNames[i*CurrentAsmMetaInfo->GlobalVarNameSize];
				if (strcmp(name,param)==0){
					p=(long)CurrentAsmMetaInfo->GlobalVarPos[i];
					pk=PKDerefConstant;
					if (DebugAsm)
						printf("global variable:  %s\n",param);
					return;
				}
			}
			// not yet existing label...
			if (param[0]=='_'){
				strcpy(CurrentAsmMetaInfo->WantedLabelName[CurrentAsmMetaInfo->NumWantedLabels],param);
				CurrentAsmMetaInfo->WantedLabelAdd[CurrentAsmMetaInfo->NumWantedLabels]=0;
				CurrentAsmMetaInfo->WantedLabelPN[CurrentAsmMetaInfo->NumWantedLabels]=pn;
				CurrentAsmMetaInfo->NumWantedLabels++;
				if (DebugAsm)
					printf("wanted label:  %s\n",param);
				p=0;
				pk=PKConstant;
				ConstantIsLabel=true;
				UnknownLabel=true;
				return;
			}
		}
	}else
		pk=PKNone;
	if (pk==PKInvalid)
		AsmError(string("unknown parameter:  ",param));
}

// is some parameter (pk,p) from source code compatible with (param) from the instruction definition?
bool ParamCompatible(int pk,int p,int &param)
{
	// translate parameter into 16/32 bit
	CorrectParam(param,use_mode16);
	
	// no parameter?
	if ((pk==PKNone)&&(param==-1))
		return true;
	
	// register
	if (pk==PKRegister){
		if (param!=Register[p].reg){
			if (Register[p].type==param)
				return true;
			if ((Register[p].type==Gd)&&(param==Ed))
				return true;
			if ((Register[p].type==Gw)&&(param==Ew))
				return true;
			if ((Register[p].type==Gb)&&(param==Eb))
				return true;
			return false;
		}
		
	// register as pointer
	}else if (pk==PKDerefRegister){
		if ((Register[p].reg!=eAX)&&(Register[p].reg!=eCX)&&(Register[p].reg!=eDX)&&(Register[p].reg!=eBX)&&(Register[p].reg!=eSI)&&(Register[p].reg!=eDI)&&(Register[p].reg!=BX)&&(Register[p].reg!=SI)&&(Register[p].reg!=DI))
			return false;
		if ((param!=Ed)&&(param!=Ew)&&(param!=Eb))
			return false;
		/*if ((Register[p].type==Gd)&&(param!=Ev)&&(param!=Ed))&&(param!=Ed))
			return false;
		if ((Register[p].type==Gw)&&(param!=Ew))
			return false;
		if ((Register[p].type==Gb)&&(param!=Eb))
			return false;*/
	}else if (pk==PKConstant){
		if ((param!=Id)&&(param!=Iw)&&(param!=Ib))
			return false;
	}else if (pk==PKConstantDouble){
		return (param==Ip);
	}else if (pk==PKDerefConstant){
		if ((param!=Ed)&&(param!=Ew)&&(param!=Eb))
			return false;
	}else
		return false;
	return true;
}

int PreInsertionLength;
int CommandSize;
void CalcCommandSize(int c)
{
	CommandSize=0;
	if (Instruction[c].code>0)		CommandSize++;
	if (Instruction[c].code_alt>0)	CommandSize++;
	if (Instruction[c].param1==Ib)	CommandSize++;
	if (Instruction[c].param1==Iw)	CommandSize+=2;
	if (Instruction[c].param1==Id)	CommandSize+=4;
	if (Instruction[c].param1==Iv)	CommandSize+=use_mode16?2:4;
	if (Instruction[c].param2==Ib)	CommandSize++;
	if (Instruction[c].param2==Iw)	CommandSize+=2;
	if (Instruction[c].param2==Id)	CommandSize+=4;
	if (Instruction[c].param2==Iv)	CommandSize+=use_mode16?2:4;
	bool use_rm=false;
	if (Instruction[c].cap>=0)	use_rm=true;
	if ((Instruction[c].param1==Eb)||(Instruction[c].param1==Ew)||(Instruction[c].param1==Ed)
		||(Instruction[c].param1==Ev))	use_rm=true;
	if ((Instruction[c].param2==Eb)||(Instruction[c].param2==Ew)||(Instruction[c].param2==Ed)
		||(Instruction[c].param2==Ev))	use_rm=true;
	if (use_rm)						CommandSize++;
}

void InsertConstant(int c,int size,int pn,char *a=NULL,char *b=NULL)
{
	if ((!a)&&(UnknownLabel))
		if (CurrentAsmMetaInfo->WantedLabelPN[CurrentAsmMetaInfo->NumWantedLabels-1]==pn){
			// the constant is just fake, since its value is still unknown!
			CurrentAsmMetaInfo->WantedLabelPos[CurrentAsmMetaInfo->NumWantedLabels-1]=PreInsertionLength+AsmCodeLength;
			CurrentAsmMetaInfo->WantedLabelSize[CurrentAsmMetaInfo->NumWantedLabels-1]=size;
		}
	if (!a){
		a=b=&buffer[AsmCodeLength];
		AsmCodeLength+=size;
	}
	int tc=c;
	bool lost=false;
	if (size==4)
		*(int*)a=*(int*)b=c;
	if (size==2){
		*(short*)a=*(short*)b=c;
		lost=(c>65535)||(c<-32768);
		c=(short)c;
	}
	if (size==1){
		*a=*b=c;
		lost=(c>255)||(c<-128);
		c=(char)c;
	}
	if (lost){
		msg_write("----------------------------- Warning ----------------------------");
		msg_write(string("The constant is larger than supported by its type!\nline: ",i2s(LineNo+1)));
		msg_write(string(d2h((char*)&tc,4)," -> ",d2h((char*)&c,size)));
		msg_write("------------------------------------------------------------------");
	}
}

int AsmCodeLength;

// convert human readable asm code into opcode
char *Asm2Opcode(char *code)
{
	if (!Instruction)
		SetInstructionSet(InstructionSetDefault);
	
	AsmCodeLength=0; // beginning of block -> current char
	PreInsertionLength=0; // position of the block withing (overall) opcode
	if (CurrentAsmMetaInfo)
		PreInsertionLength=CurrentAsmMetaInfo->CurrentOpcodePos;
	// CurrentAsmMetaInfo->CurrentOpcodePos // Anfang aktuelle Zeile im gesammten Opcode
	code_buffer=code; // Asm-Source-Puffer
	LineNo=0;
	if (CurrentAsmMetaInfo)
		LineNo=CurrentAsmMetaInfo->LineOffset;
	LineNo-=2; // ????
	int pos=0;
	char cmd[128],param1[128],param2[128];
	long p1,p2;
	long pk1,pk2;
	mode16=false;
	use_mode16=false;
	if (CurrentAsmMetaInfo)
		mode16=CurrentAsmMetaInfo->Mode16;
	EndOfCode=false;
	while(true){
		use_mode16=mode16;
		strcpy(param1,"");
		strcpy(param2,"");
		ConstantIsLabel=false;
		UnknownLabel=false;
		if (CurrentAsmMetaInfo)
			CurrentAsmMetaInfo->CurrentOpcodePos=PreInsertionLength+AsmCodeLength;

		strcpy(cmd,GetMne(pos));
		if (strlen(cmd)<1)
			break;
		if (!EndOfLine){
			strcpy(param1,GetMne(pos));
			if (strcmp(param1,"dword")==0){
				use_mode16=false;
				if (!EndOfLine)
					strcpy(param1,GetMne(pos));
			}
			if (strcmp(param1,"word")==0){
				use_mode16=true;
				if (!EndOfLine)
					strcpy(param1,GetMne(pos));
			}
		}
		if (!EndOfLine)
			strcpy(param2,GetMne(pos));
		//msg_write>Write(string("----: ",cmd," ",param1,", ",param2));
		if (EndOfCode)
			break;
		so("------------------------------");
		so(cmd);
		so(param1);
		so(param2);
		so("------");

		GetParam(pk1,p1,param1,0);
		GetParam(pk2,p2,param2,1);
		if ((pk1==PKInvalid)||(pk2==PKInvalid)){
			AsmCodeLength=-1;
			return NULL;
		}
		bool done=false;
		if (strcmp(cmd,"bits_16")==0){
			so("16 bit Modus!");
			use_mode16=mode16=true;
			if (CurrentAsmMetaInfo){
				CurrentAsmMetaInfo->Mode16=true;
				CurrentAsmMetaInfo->BitChangePos[CurrentAsmMetaInfo->NumBitChanges]=CurrentAsmMetaInfo->CurrentOpcodePos;
				CurrentAsmMetaInfo->BitChange[CurrentAsmMetaInfo->NumBitChanges]=16;
				CurrentAsmMetaInfo->NumBitChanges++;
			}
			done=true;
		}
		if (strcmp(cmd,"bits_32")==0){
			so("32 bit Modus!");
			use_mode16=mode16=false;
			if (CurrentAsmMetaInfo){
				CurrentAsmMetaInfo->Mode16=false;
				CurrentAsmMetaInfo->BitChangePos[CurrentAsmMetaInfo->NumBitChanges]=CurrentAsmMetaInfo->CurrentOpcodePos;
				CurrentAsmMetaInfo->BitChange[CurrentAsmMetaInfo->NumBitChanges]=32;
				CurrentAsmMetaInfo->NumBitChanges++;
			}
			done=true;
		}
		if (strcmp(cmd,"db")==0){
			so("Daten:   1 byte");
			if (CurrentAsmMetaInfo){
				CurrentAsmMetaInfo->DataPos[CurrentAsmMetaInfo->NumDatas]=CurrentAsmMetaInfo->CurrentOpcodePos;
				CurrentAsmMetaInfo->DataSize[CurrentAsmMetaInfo->NumDatas]=1;
				CurrentAsmMetaInfo->NumDatas++;
			}
			buffer[AsmCodeLength]=(char)p1;	AsmCodeLength++;
			done=true;
		}
		if (strcmp(cmd,"dw")==0){
			so("Daten:   2 byte");
			if (CurrentAsmMetaInfo){
				CurrentAsmMetaInfo->DataPos[CurrentAsmMetaInfo->NumDatas]=CurrentAsmMetaInfo->CurrentOpcodePos;
				CurrentAsmMetaInfo->DataSize[CurrentAsmMetaInfo->NumDatas]=2;
				CurrentAsmMetaInfo->NumDatas++;
			}
			*(short*)&buffer[AsmCodeLength]=(short)p1;	AsmCodeLength+=2;
			done=true;
		}
		if (strcmp(cmd,"dd")==0){
			so("Daten:   4 byte");
			if (CurrentAsmMetaInfo){
				CurrentAsmMetaInfo->DataPos[CurrentAsmMetaInfo->NumDatas]=CurrentAsmMetaInfo->CurrentOpcodePos;
				CurrentAsmMetaInfo->DataSize[CurrentAsmMetaInfo->NumDatas]=4;
				CurrentAsmMetaInfo->NumDatas++;
			}
			*(int*)&buffer[AsmCodeLength]=(int)p1;	AsmCodeLength+=4;
			done=true;
		}
		if ((strcmp(cmd,"ds")==0)||(strcmp(cmd,"dz")==0)){
			so("Daten:   String");
			char *s=(char*)p1;
			int l=strlen(s);
			if (strcmp(cmd,"dz")==0)	l++;
			if (CurrentAsmMetaInfo){
				CurrentAsmMetaInfo->DataPos[CurrentAsmMetaInfo->NumDatas]=CurrentAsmMetaInfo->CurrentOpcodePos;
				CurrentAsmMetaInfo->DataSize[CurrentAsmMetaInfo->NumDatas]=l;
				CurrentAsmMetaInfo->NumDatas++;
			}
			memcpy(&buffer[AsmCodeLength],s,l);
			AsmCodeLength+=l;
			done=true;
		}
		if (strcmp(cmd,"org")==0){
			if (CurrentAsmMetaInfo)
				CurrentAsmMetaInfo->CodeOrigin=p1;
			done=true;
		}
		if (cmd[strlen(cmd)-1]==':'){
			so("Label");
			if (CurrentAsmMetaInfo){
				cmd[strlen(cmd)-1]=0;
				so(cmd);
				strcpy(CurrentAsmMetaInfo->LabelName[CurrentAsmMetaInfo->NumLabels],cmd);
				CurrentAsmMetaInfo->LabelPos[CurrentAsmMetaInfo->NumLabels]=CurrentAsmMetaInfo->CurrentOpcodePos;
				CurrentAsmMetaInfo->NumLabels++;

				// die bisherigen Platzhalter ersetzen
				for (int i=0;i<CurrentAsmMetaInfo->NumWantedLabels;i++)
					if (strcmp(CurrentAsmMetaInfo->WantedLabelName[i],cmd)==0){
						int lv=CurrentAsmMetaInfo->CurrentOpcodePos+CurrentAsmMetaInfo->CodeOrigin+CurrentAsmMetaInfo->WantedLabelAdd[i];
						char *a=&CurrentAsmMetaInfo->Opcode[CurrentAsmMetaInfo->WantedLabelPos[i]];
						char *b=&buffer[CurrentAsmMetaInfo->WantedLabelPos[i]-PreInsertionLength];
						if (CurrentAsmMetaInfo->WantedLabelPos[i]<PreInsertionLength)
							b=a;
						InsertConstant(lv,CurrentAsmMetaInfo->WantedLabelSize[i],-1,a,b);
						for (int j=i;j<CurrentAsmMetaInfo->NumWantedLabels-1;j++){
							CurrentAsmMetaInfo->WantedLabelPos[j]=CurrentAsmMetaInfo->WantedLabelPos[j+1];
							CurrentAsmMetaInfo->WantedLabelSize[j]=CurrentAsmMetaInfo->WantedLabelSize[j+1];
							CurrentAsmMetaInfo->WantedLabelAdd[j]=CurrentAsmMetaInfo->WantedLabelAdd[j+1];
							strcpy(CurrentAsmMetaInfo->WantedLabelName[j],CurrentAsmMetaInfo->WantedLabelName[j+1]);
						}
						CurrentAsmMetaInfo->NumWantedLabels--;
						i--;
					}
			}
			done=true;
		}
		if (done){
			if ((unsigned)pos>=strlen(code)-2)
				break;
			else
				continue;
		}

		bool found_any=false;
		bool found_valid=false;
		for (int i=0;i<NumInstructions;i++)
			if (strcmp(Instruction[i].name,cmd)==0){
				found_any=true;
				int _AsmCodeLength=AsmCodeLength;
				int ip1=Instruction[i].param1;
				int ip2=Instruction[i].param2;
				//so(ip1);
				//so(ip2);
				bool ok=ParamCompatible(pk1,p1,ip1)&&ParamCompatible(pk2,p2,ip2);
				/*if (ParamCompatible(pk1,p1,ip1))
					printf(" 1: %s\n",(d2h((char*)&Instruction[i].code,1)));
				if (ParamCompatible(pk2,p2,ip2))
					printf(" 2: %s\n",(d2h((char*)&Instruction[i].code,1)));*/
				if (ok){
					so(i);
					found_valid=true;
					CalcCommandSize(i);
					bool lc_relative=((cmd[0]=='j')&&(Instruction[i].param1!=Ip)) || (strcmp(cmd,"call")==0) || (strstr(cmd,"loop"));
					if ((ConstantIsLabel)&&(!UnknownLabel)&&(lc_relative)&&(CurrentAsmMetaInfo)){
						int d=CurrentAsmMetaInfo->CurrentOpcodePos+CurrentAsmMetaInfo->CodeOrigin+CommandSize;
						if (pk1==PKConstant)	p1-=d;
						if (pk2==PKConstant)	p2-=d;
					}
					if (UnknownLabel){
						CurrentAsmMetaInfo->WantedLabelSize[CurrentAsmMetaInfo->NumWantedLabels-1]=1;
						CurrentAsmMetaInfo->WantedLabelPos[CurrentAsmMetaInfo->NumWantedLabels-1]=CurrentAsmMetaInfo->CurrentOpcodePos+1;
						if (lc_relative)
							CurrentAsmMetaInfo->WantedLabelAdd[CurrentAsmMetaInfo->NumWantedLabels-1]=-CurrentAsmMetaInfo->CurrentOpcodePos-CurrentAsmMetaInfo->CodeOrigin-CommandSize;
						else
							CurrentAsmMetaInfo->WantedLabelAdd[CurrentAsmMetaInfo->NumWantedLabels-1]=0;
					}

					if (use_mode16!=mode16)
						buffer[AsmCodeLength++]=0x66;

					buffer[AsmCodeLength++]=Instruction[i].code;
					if (Instruction[i].code_alt>=0)
						buffer[AsmCodeLength++]=Instruction[i].code_alt;
					int use_rm=-1;
					if ((ip1==Gb)||(ip1==Gw)||(ip1==Gd))	use_rm=1;
					if ((ip2==Gb)||(ip2==Gw)||(ip2==Gd))	use_rm=2;
					if ((ip1==Eb)||(ip1==Ew)||(ip1==Ed))	use_rm=1;
					if ((ip2==Eb)||(ip2==Ew)||(ip2==Ed))	use_rm=2;
					int r1=-1,r2=-1;
					if ((pk1==PKRegister)||(pk1==PKDerefRegister))	r1=Register[p1].reg;
					if ((pk2==PKRegister)||(pk2==PKDerefRegister))	r2=Register[p2].reg;
					if (use_rm>=0){
						int rm=0;
						int ppk1=(use_rm==1)?pk1:pk2;
						int ppk2=(use_rm==1)?pk2:pk1;
						if (use_rm==2){
							int t=r1;	r1=r2;	r2=t;
						}
						if (ppk1==PKDerefRegister){
							if (use_mode16){
								if (r1==SI)	rm+=0x104;
								if (r1==DI)	rm+=0x105;
								if (r1==BX)	rm+=0x107;
							}else{
								if (r1==eAX)	rm+=0x100;
								if (r1==eCX)	rm+=0x101;
								if (r1==eDX)	rm+=0x102;
								if (r1==eBX)	rm+=0x103;
								if (r1==eSI)	rm+=0x106;
								if (r1==eDI)	rm+=0x107;
							}
						}else if (ppk1==PKDerefConstant){
							if (use_mode16)
								rm+=0x106;
							else
								rm+=0x105;
						}else{
							if ((r1==eAX)||(r1==AX)||(r1==AL))	rm+=0x1c0;
							if ((r1==eCX)||(r1==CX)||(r1==CL))	rm+=0x1c1;
							if ((r1==eDX)||(r1==DX)||(r1==DL))	rm+=0x1c2;
							if ((r1==eBX)||(r1==BX)||(r1==BL))	rm+=0x1c3;
							if ((r1==eSP)||(r1==SP)||(r1==AH))	rm+=0x1c4;
							if ((r1==eBP)||(r1==BP)||(r1==CH))	rm+=0x1c5;
							if ((r1==eSI)||(r1==SI)||(r1==DH))	rm+=0x1c6;
							if ((r1==eDI)||(r1==DI)||(r1==BH))	rm+=0x1c7;
						}
						if ((r2==eAX)||(r2==AX)||(r2==AL)||(r2==ES))	rm+=0x200;
						if ((r2==eCX)||(r2==CX)||(r2==CL)||(r2==CS))	rm+=0x208;
						if ((r2==eDX)||(r2==DX)||(r2==DL)||(r2==SS))	rm+=0x210;
						if ((r2==eBX)||(r2==BX)||(r2==BL)||(r2==DS))	rm+=0x218;
						if ((r2==eSP)||(r2==SP)||(r2==AH)||(r2==FS))	rm+=0x220;
						if ((r2==eBP)||(r2==BP)||(r2==CH)||(r2==GS))	rm+=0x228;
						if ((r2==eSI)||(r2==SI)||(r2==DH))				rm+=0x230;
						if ((r2==eDI)||(r2==DI)||(r2==BH))				rm+=0x238;
						if (r2==CR0)	rm+=0x200;
						if (r2==CR1)	rm+=0x208;
						if (r2==CR2)	rm+=0x210;
						if (r2==CR3)	rm+=0x218;
						if (Instruction[i].cap>0)
							rm+=Instruction[i].cap*8;
						buffer[AsmCodeLength++]=rm;
						if ((rm&0x100)==0){
							found_valid=false;
							AsmCodeLength=_AsmCodeLength;
							so("nachtraeglich falsch!");
							continue;
						}
					}
					if ((ip1==Id)||( ((!use_mode16)) &&(pk1==PKDerefConstant)))
						InsertConstant(p1,4,0);
					else if ((ip1==Iw)|| ((use_mode16)&&(pk1==PKDerefConstant)))
						InsertConstant(p1,2,0);
					else if (ip1==Ib)
						InsertConstant(p1,1,0);
					else if (ip1==Ip){
						InsertConstant(ParamConstantDouble,use_mode16?2:4,2);
						InsertConstant(p1,2,0);
					}
					if (pk2==PKDerefConstant) ip2=use_mode16?Iw:Id;
					if (ip2==Id)
						InsertConstant(p2,4,1);
					else if (ip2==Iw)
						InsertConstant(p2,2,1);
					else if (ip2==Ib)
						InsertConstant(p2,1,1);


					break;
				}
			}
		if (!found_any){
			AsmError(string("unknown instruction:  ",cmd));
			AsmCodeLength=-1;
			return NULL;
		}
		if (!found_valid){
			AsmError(string("instruction is not compatible with its parameters:  ",cmd," ",param1,", ",param2));
			AsmCodeLength=-1;
			return NULL;
		}

		if (EndOfCode)
			break;
	}
	return buffer;
}

bool _test_param_(int ip,int wp)
{
	if (ip==wp)
		return true;
	if ((ip==Gd)||(ip==Gb)){
		for (int i=0;i<NumRegisters;i++)
			if ((Register[i].reg==wp)&&(Register[i].type==ip))
				return true;
	}
	if ((ip==Ed)||(ip==Eb)){
		if ((wp==disp8)||(wp==disp16)||(wp==disp32))
			return true;
	}
	if ((ip==Id)||(ip==Ib)){
		if ((wp==PKConstant))
			return true;
	}
	return false;
}

void OpcodeAddInstruction(char *oc,int &ocs,int inst,int param1_type,int param2_type,void *param,int offset,int insert_at)
//void OpcodeAddInstruction(char *oc,int &ocs,int inst,int pt1,int p1,int pt2,int p2,void *param,int offset,int insert_at)
{
	if (!Instruction)
		SetInstructionSet(InstructionSetDefault);
	
	msg_write("add inst");
	int ninst=-1;
	for (int i=0;i<NumInstructions;i++)
		if (Instruction[i].inst==inst){
			int ip1=Instruction[i].param1;
			int ip2=Instruction[i].param2;
			CorrectParam(ip1,false);
			CorrectParam(ip2,false);
			if ((_test_param_(ip1,param1_type))&&(_test_param_(ip2,param2_type))){
				
//	int code,code_alt,cap;
				// add opcode
				oc[ocs++]=Instruction[i].code;
				if (Instruction[i].code_alt!=-1)
					oc[ocs++]=Instruction[i].code_alt;
				ninst=i;
				break;
			}
		}
	if (ninst<0)
		msg_error("asm nicht gut...");
	
#if 0	
	
	if (insert_at<0)
		insert_at=ocs;
	int insert_length=0;
	//char insert_oc[128];
	if ((kind!=KindRefToLocal)&&(kind!=KindRefToGlobal))
		param+=offset;
	int l=8;
	if (((long)param>125)||((long)param<-125))	l=32; // l=16;
	//if (((int)param>65530)||((int)param<-65530))	l=32;
	short *p_oc=&AsmInstruction[inst].oc_g[0];
	if ((kind==KindVarLocal)&&(l==8))
		p_oc=&AsmInstruction[inst].oc_l8[0];
	if ((kind==KindVarLocal)&&(l==32))
		p_oc=&AsmInstruction[inst].oc_l32[0];
	if ((kind==KindConstant)||(kind==KindRefToConst)){
		if (AsmInstruction[inst].oc_c[0]>=0)
			p_oc=&AsmInstruction[inst].oc_c[0];
		else
			kind=KindVarGlobal;
	}
	if (kind<0)
		p_oc=&AsmInstruction[inst].oc_l8[0];
	if ((kind==KindRefToLocal)||(kind==KindRefToGlobal)){
		if (kind==KindRefToLocal)
			OCAddInstruction(oc,ocs,inMovEdxM,KindVarLocal,param);
		if (kind==KindRefToGlobal)
			OCAddInstruction(oc,ocs,inMovEdxM,KindVarGlobal,param);
		if (offset!=0)
			OCAddInstruction(oc,ocs,inAddEdxM,KindConstant,(char*)offset);
		p_oc=&AsmInstruction[inst].oc_dr[0];
	}

	if (p_oc[0]>=0)	OCAddChar(oc,ocs,(char)p_oc[0]);
	if (p_oc[1]>=0)	OCAddChar(oc,ocs,(char)p_oc[1]);
	if (p_oc[2]>=0)	OCAddChar(oc,ocs,(char)p_oc[2]);
	OCOParam=ocs;
	if (kind==KindVarLocal){ // local
		if (l==8)
			OCAddChar(oc,ocs,(long)param);
//		else if (l==16)
//			OCAddWord(oc,ocs,(long)param);
		else if (l==32)
			OCAddInt(oc,ocs,(long)param);
	}else if (kind==KindVarGlobal)
		OCAddInt(oc,ocs,(long)param); // global
	else if (kind==KindConstant){
		if (AsmInstruction[inst].const_size==8)
			OCAddChar(oc,ocs,(long)param);
		else if (AsmInstruction[inst].const_size==16)
			OCAddWord(oc,ocs,(long)param);
		else if (AsmInstruction[inst].const_size==32)
			OCAddInt(oc,ocs,(long)param);
	}else if (kind==KindRefToConst){
		if (AsmInstruction[inst].const_size==8)
			OCAddChar(oc,ocs,*(int*)param);
		else if (AsmInstruction[inst].const_size==16)
			OCAddWord(oc,ocs,*(int*)param);
		else if (AsmInstruction[inst].const_size==32)
			OCAddInt(oc,ocs,*(int*)param);
	}

	p_oc=&AsmInstruction[inst].oc_p;
	if (p_oc[0]>=0)	OCAddChar(oc,ocs,(char)p_oc[0]);
	/*if (p_oc[0]>=0){	insert_oc[insert_length]=(char)p_oc[0];	insert_length++;	}
	if (p_oc[1]>=0){	insert_oc[insert_length]=(char)p_oc[1];	insert_length++;	}
	if (p_oc[2]>=0){	insert_oc[insert_length]=(char)p_oc[2];	insert_length++;	}
	OCOParam=insert_at+insert_length;
	if (kind==KindVarLocal){ // local
		if (l==8)
			insert_oc[insert_length]=(char)(int)param;
//		else if (l==16)
//			*(short*)&insert_oc[insert_length]=(short)param);
		else if (l==32)
			*(int*)&insert_oc[insert_length]=(int)param;
		insert_length+=l/8;
	}else if (kind==KindVarGlobal){ // global
		*(int*)&insert_oc[insert_length]=(int)param;
		insert_length+=4;
	}else if (kind==KindConstant){
		if (AsmInstruction[inst].const_size==8)
			insert_oc[insert_length]=(char)(int)param;
		else if (AsmInstruction[inst].const_size==16)
			*(short*)&insert_oc[insert_length]=(short)(int)param;
		else if (AsmInstruction[inst].const_size==32)
			*(int*)&insert_oc[insert_length]=(int)param;
		insert_length+=AsmInstruction[inst].const_size/8;
	}else if (kind==KindRefToConst){
		if (AsmInstruction[inst].const_size==8)
			insert_oc[insert_length]=*(char*)param;
		else if (AsmInstruction[inst].const_size==16)
			*(short*)&insert_oc[insert_length]=*(short*)param;
		else if (AsmInstruction[inst].const_size==32)
			*(int*)&insert_oc[insert_length]=*(int*)param;
		insert_length+=AsmInstruction[inst].const_size/8;
	}

	p_oc=&AsmInstruction[inst].oc_p;
	if (p_oc[0]>=0){	insert_oc[insert_length]=(char)p_oc[0];	insert_length++;	}

	int i;
	for (i=s->OpcodeSize-1;i>=insert_at;i--)
		oc[i+insert_length]=oc[i];
	for (i=0;i<insert_length;i++)
		oc[insert_at+i]=insert_oc[i];
	ocs+=insert_length;*/
	
#endif
}
