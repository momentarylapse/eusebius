#include "dasm.h"
#include "file.h"
#include "msg.h"
#include <stdio.h>

char buffer[16777216];
int ParamConstantDouble;

enum{
	Eb,Ev,Ew,Ed,Gb,Gv,Gw,Gd,
	Ib,Iv,Iw,Id,Ob,Ov,Ow,Od,Sw,Cd,Ip,
	eAX,eCX,eDX,eBX,eSP,eSI,eDI,eBP,
	AX,CX,DX,BX,BP,SP,SI,DI,
	AL,CL,DL,BL,AH,CH,DH,BH,
	CS,DS,SS,ES,FS,GS,
	CR0,CR1,CR2,CR3,
	peAX,peCX,peDX,peBX,peSI,peDI,
	pBX_pSI,pBX_pDI,pBP_pSI,pBP_pDI,pSI,pDI,pBX,
	disp8,disp16,disp32,pp,
	d8_peAX,d8_peCX,d8_peDX,d8_peBX,
	d8_pp,d8_peBP,d8_peSI,d8_peDI,
	d8_pBX,d8_pBP,d8_pSI,d8_pDI,
	d16_pBX,d16_pBP,d16_pSI,d16_pDI,
	d32_peAX,d32_peCX,d32_peDX,d32_peBX,
	d32_pp,d32_peBP,d32_peSI,d32_peDI,
	d8_pBX_pSI,d8_pBX_pDI,d8_pBP_pSI,d8_pBP_pDI,
	d16_pBX_pSI,d16_pBX_pDI,d16_pBP_pSI,d16_pBP_pDI
};

struct sRegister{
	char *name;
	int reg,type;
};

#define NumRegisters		(4*4 + 4*2 + 6 + 4)
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
};


struct sInstruction{
	char *name;
	int code,code_alt,cap;
	int param1,param2;
};

#define NumInstructions		319
sInstruction Instruction[NumInstructions]={
	{"add.b"	,0x00	,-1	,-1	,Eb	,Gb	},
	{"add"		,0x01	,-1	,-1	,Ev	,Gv	},
	{"add.b"	,0x02	,-1	,-1	,Gb	,Eb	},
	{"add"		,0x03	,-1	,-1	,Gv	,Ev	},
	{"add.b"	,0x04	,-1	,-1	,AL	,Ib	},
	{"add"		,0x05	,-1	,-1	,eAX,Iv	},
	{"push"		,0x06	,-1	,-1	,ES	,-1	},
	{"pop"		,0x07	,-1	,-1	,ES	,-1	},
	{"or.b"		,0x08	,-1	,-1	,Eb	,Gb	},
	{"or"		,0x09	,-1	,-1	,Ev	,Gv	},
	{"or.b"		,0x0a	,-1	,-1	,Gb	,Eb	},
	{"or"		,0x0b	,-1	,-1	,Gv	,Ev	},
	{"or.b"		,0x0c	,-1	,-1	,AL	,Ib	},
	{"or"		,0x0d	,-1	,-1	,eAX,Iv	},
	{"push"		,0x0e	,-1	,-1	,CS	,-1	},
	{"sldt"		,0x0f	,0x00	,0	,Ew	,-1	},
	{"str"		,0x0f	,0x00	,1	,Ew	,-1	},
	{"lldt"		,0x0f	,0x00	,2	,Ew	,-1	},
	{"ltr"		,0x0f	,0x00	,3	,Ew	,-1	},
	{"verr"		,0x0f	,0x00	,4	,Ew	,-1	},
	{"verw"		,0x0f	,0x00	,5	,Ew	,-1	},
	{"sgdt"		,0x0f	,0x01	,0	,Ev	,-1	},
	{"sidt"		,0x0f	,0x01	,1	,Ev	,-1	},
	{"lgdt"		,0x0f	,0x01	,2	,Ev	,-1	},
	{"lidt"		,0x0f	,0x01	,3	,Ev	,-1	},
	{"smsw"		,0x0f	,0x01	,4	,Ew	,-1	},
	{"lmsw"		,0x0f	,0x01	,6	,Ew	,-1	},
	{"mov"		,0x0f	,0x22	,-1	,Cd	,Ed	}, // Fehler im Algorhytmus!!!!
	{"mov"		,0x0f	,0x20	,-1	,Ed	,Cd	},
	{"jo"		,0x0f	,0x80	,-1	,Iv	,-1	},
	{"jno"		,0x0f	,0x81	,-1	,Iv	,-1	},
	{"jb"		,0x0f	,0x82	,-1	,Iv	,-1	},
	{"jnb"		,0x0f	,0x83	,-1	,Iv	,-1	},
	{"jz"		,0x0f	,0x84	,-1	,Iv	,-1	},
	{"jnz"		,0x0f	,0x85	,-1	,Iv	,-1	},
	{"jbe"		,0x0f	,0x86	,-1	,Iv	,-1	},
	{"jnbe"		,0x0f	,0x87	,-1	,Iv	,-1	},
	{"js"		,0x0f	,0x88	,-1	,Iv	,-1	},
	{"jns"		,0x0f	,0x89	,-1	,Iv	,-1	},
	{"jp"		,0x0f	,0x8a	,-1	,Iv	,-1	},
	{"jnp"		,0x0f	,0x8b	,-1	,Iv	,-1	},
	{"jl"		,0x0f	,0x8c	,-1	,Iv	,-1	},
	{"jnl"		,0x0f	,0x8d	,-1	,Iv	,-1	},
	{"jle"		,0x0f	,0x8e	,-1	,Iv	,-1	},
	{"jnle"		,0x0f	,0x8f	,-1	,Iv	,-1	},
	{"seto.b"	,0x0f	,0x90	,-1	,Eb	,-1	},
	{"setno.b"	,0x0f	,0x91	,-1	,Eb	,-1	},
	{"setb.b"	,0x0f	,0x92	,-1	,Eb	,-1	},
	{"setnb.b"	,0x0f	,0x93	,-1	,Eb	,-1	},
	{"setz.b"	,0x0f	,0x94	,-1	,Eb	,-1	},
	{"setnz.b"	,0x0f	,0x95	,-1	,Eb	,-1	},
	{"setbe.b"	,0x0f	,0x96	,-1	,Eb	,-1	},
	{"setnbe.b"	,0x0f	,0x97	,-1	,Eb	,-1	},
	{"sets"		,0x0f	,0x98	,-1	,Ev	,-1	},
	{"setns"	,0x0f	,0x99	,-1	,Ev	,-1	},
	{"setp"		,0x0f	,0x9a	,-1	,Ev	,-1	},
	{"setnp"	,0x0f	,0x9b	,-1	,Ev	,-1	},
	{"setl"		,0x0f	,0x9c	,-1	,Ev	,-1	},
	{"setnl"	,0x0f	,0x9d	,-1	,Ev	,-1	},
	{"setle"	,0x0f	,0x9e	,-1	,Ev	,-1	},
	{"setnle"	,0x0f	,0x9f	,-1	,Ev	,-1	},
	{"imul"		,0x0f	,0xaf	,-1	,Gv	,Ev	},
	{"movzx"	,0x0f	,0xb6	,-1	,Gv	,Eb	},
	{"movzx"	,0x0f	,0xb7	,-1	,Gv	,Ew	},
	{"movsx"	,0x0f	,0xbe	,-1	,Gv	,Eb	},
	{"movsx"	,0x0f	,0xbf	,-1	,Gv	,Ew	},
	{"adc.b"	,0x10	,-1	,-1	,Eb	,Gb	},
	{"adc"		,0x11	,-1	,-1	,Ev	,Gv	},
	{"adc.b"	,0x12	,-1	,-1	,Gb	,Eb	},
	{"adc"		,0x13	,-1	,-1	,Gv	,Ev	},
	{"adc.b"	,0x14	,-1	,-1	,AL	,Ib	},
	{"adc"		,0x15	,-1	,-1	,eAX,Iv	},
	{"push"		,0x16	,-1	,-1	,SS	,-1	},
	{"pop"		,0x17	,-1	,-1	,SS	,-1	},
	{"sbb.b"	,0x18	,-1	,-1	,Eb	,Gb	},
	{"sbb"		,0x19	,-1	,-1	,Ev	,Gv	},
	{"sbb.b"	,0x1a	,-1	,-1	,Gb	,Eb	},
	{"sbb"		,0x1b	,-1	,-1	,Gv	,Ev	},
	{"sbb.b"	,0x1c	,-1	,-1	,AL	,Ib	},
	{"sbb"		,0x1d	,-1	,-1	,eAX	,Iv	},
	{"push"		,0x1e	,-1	,-1	,DS	,-1	},
	{"pop"		,0x1f	,-1	,-1	,DS	,-1	},
	{"and.b"	,0x20	,-1	,-1	,Eb	,Gb	},
	{"and"		,0x21	,-1	,-1	,Ev	,Gv	},
	{"and.b"	,0x22	,-1	,-1	,Gb	,Eb	},
	{"and"		,0x23	,-1	,-1	,Gv	,Ev	},
	{"and.b"	,0x24	,-1	,-1	,AL	,Ib	},
	{"and"		,0x25	,-1	,-1	,eAX	,Iv	},
	{"sub.b"	,0x28	,-1	,-1	,Eb	,Gb	},
	{"sub"		,0x29	,-1	,-1	,Ev	,Gv	},
	{"sub.b"	,0x2a	,-1	,-1	,Gb	,Eb	},
	{"sub"		,0x2b	,-1	,-1	,Gv	,Ev	},
	{"sub.b"	,0x2c	,-1	,-1	,AL	,Ib	},
	{"sub"		,0x2d	,-1	,-1	,eAX	,Iv	},
	{"xor.b"	,0x30	,-1	,-1	,Eb	,Gb	},
	{"xor"		,0x31	,-1	,-1	,Ev	,Gv	},
	{"xor.b"	,0x32	,-1	,-1	,Gb	,Eb	},
	{"xor"		,0x33	,-1	,-1	,Gv	,Ev	},
	{"xor.b"	,0x34	,-1	,-1	,AL	,Ib	},
	{"xor"		,0x35	,-1	,-1	,eAX	,Iv	},
	{"cmp.b"	,0x38	,-1	,-1	,Eb	,Gb	},
	{"cmp"		,0x39	,-1	,-1	,Ev	,Gv	},
	{"cmp.b"	,0x3a	,-1	,-1	,Gb	,Eb	},
	{"cmp"		,0x3b	,-1	,-1	,Gv	,Ev	},
	{"cmp.b"	,0x3c	,-1	,-1	,AL	,Ib	},
	{"cmp"		,0x3d	,-1	,-1	,eAX	,Iv	},
	{"inc"		,0x40	,-1	,-1	,eAX	,-1	},
	{"inc"		,0x41	,-1	,-1	,eCX	,-1	},
	{"inc"		,0x42	,-1	,-1	,eDX	,-1	},
	{"inc"		,0x43	,-1	,-1	,eBX	,-1	},
	{"inc"		,0x44	,-1	,-1	,eSP	,-1	},
	{"inc"		,0x45	,-1	,-1	,eBP	,-1	},
	{"inc"		,0x46	,-1	,-1	,eSI	,-1	},
	{"inc"		,0x47	,-1	,-1	,eDI	,-1	},
	{"dec"		,0x48	,-1	,-1	,eAX	,-1	},
	{"dec"		,0x49	,-1	,-1	,eCX	,-1	},
	{"dec"		,0x4a	,-1	,-1	,eDX	,-1	},
	{"dec"		,0x4b	,-1	,-1	,eBX	,-1	},
	{"dec"		,0x4c	,-1	,-1	,eSP	,-1	},
	{"dec"		,0x4d	,-1	,-1	,eBP	,-1	},
	{"dec"		,0x4e	,-1	,-1	,eSI	,-1	},
	{"dec"		,0x4f	,-1	,-1	,eDI	,-1	},
	{"push"		,0x50	,-1	,-1	,eAX	,-1	},
	{"push"		,0x51	,-1	,-1	,eCX	,-1	},
	{"push"		,0x52	,-1	,-1	,eDX	,-1	},
	{"push"		,0x53	,-1	,-1	,eBX	,-1	},
	{"push"		,0x54	,-1	,-1	,eSP	,-1	},
	{"push"		,0x55	,-1	,-1	,eBP	,-1	},
	{"push"		,0x56	,-1	,-1	,eSI	,-1	},
	{"push"		,0x57	,-1	,-1	,eDI	,-1	},
	{"pop"		,0x58	,-1	,-1	,eAX	,-1	},
	{"pop"		,0x59	,-1	,-1	,eCX	,-1	},
	{"pop"		,0x5a	,-1	,-1	,eDX	,-1	},
	{"pop"		,0x5b	,-1	,-1	,eBX	,-1	},
	{"pop"		,0x5c	,-1	,-1	,eSP	,-1	},
	{"pop"		,0x5d	,-1	,-1	,eBP	,-1	},
	{"pop"		,0x5e	,-1	,-1	,eSI	,-1	},
	{"pop"		,0x5f	,-1	,-1	,eDI	,-1	},
	{"pusha"	,0x60	,-1	,-1	,-1	,-1	},
	{"popa"		,0x61	,-1	,-1	,-1	,-1	},
	{"push"		,0x68	,-1	,-1	,Iv	,-1	},
	{"imul"		,0x69	,-1	,-1	,Ev	,Iv	},
	{"push"		,0x6a	,-1	,-1	,Ib	,-1	},
	{"jo.b"		,0x70	,-1	,-1	,Ib	,-1	},
	{"jno.b"	,0x71	,-1	,-1	,Ib	,-1	},
	{"jb.b"		,0x72	,-1	,-1	,Ib	,-1	},
	{"jnb.b"	,0x73	,-1	,-1	,Ib	,-1	},
	{"jz.b"		,0x74	,-1	,-1	,Ib	,-1	},
	{"jnz.b"	,0x75	,-1	,-1	,Ib	,-1	},
	{"jbe.b"	,0x76	,-1	,-1	,Ib	,-1	},
	{"jnbe.b"	,0x77	,-1	,-1	,Ib	,-1	},
	{"js.b"		,0x78	,-1	,-1	,Ib	,-1	},
	{"jns.b"	,0x79	,-1	,-1	,Ib	,-1	},
	{"jp.b"		,0x7a	,-1	,-1	,Ib	,-1	},
	{"jnp.b"	,0x7b	,-1	,-1	,Ib	,-1	},
	{"jl.b"		,0x7c	,-1	,-1	,Ib	,-1	},
	{"jnl.b"	,0x7d	,-1	,-1	,Ib	,-1	},
	{"jle.b"	,0x7e	,-1	,-1	,Ib	,-1	},
	{"jnle.b"	,0x7f	,-1	,-1	,Ib	,-1	},
	// Immediate Group 1
	{"add.b"	,0x80	,-1	,0	,Eb	,Ib	},
	{"or.b"		,0x80	,-1	,1	,Eb	,Ib	},
	{"adc.b"	,0x80	,-1	,2	,Eb	,Ib	},
	{"sbb.b"	,0x80	,-1	,3	,Eb	,Ib	},
	{"and.b"	,0x80	,-1	,4	,Eb	,Ib	},
	{"sub.b"	,0x80	,-1	,5	,Eb	,Ib	},
	{"xor.b"	,0x80	,-1	,6	,Eb	,Ib	},
	{"cmp.b"	,0x80	,-1	,7	,Eb	,Ib	},
	{"add"		,0x81	,-1	,0	,Ev	,Iv	},
	{"or"		,0x81	,-1	,1	,Ev	,Iv	},
	{"adc"		,0x81	,-1	,2	,Ev	,Iv	},
	{"sbb"		,0x81	,-1	,3	,Ev	,Iv	},
	{"and"		,0x81	,-1	,4	,Ev	,Iv	},
	{"sub"		,0x81	,-1	,5	,Ev	,Iv	},
	{"xor"		,0x81	,-1	,6	,Ev	,Iv	},
	{"cmp"		,0x81	,-1	,7	,Ev	,Iv	},
	{"add.b"	,0x83	,-1	,0	,Ev	,Ib	},
	{"or.b"		,0x83	,-1	,1	,Ev	,Ib	},
	{"adc.b"	,0x83	,-1	,2	,Ev	,Ib	},
	{"sbb.b"	,0x83	,-1	,3	,Ev	,Ib	},
	{"and.b"	,0x83	,-1	,4	,Ev	,Ib	},
	{"sub.b"	,0x83	,-1	,5	,Ev	,Ib	},
	{"xor.b"	,0x83	,-1	,6	,Ev	,Ib	},
	{"cmp.b"	,0x83	,-1	,7	,Ev	,Ib	},
	{"test.b"	,0x84	,-1	,-1	,Eb	,Gb	},
	{"test"		,0x85	,-1	,-1	,Ev	,Gv	},
	{"xchg.b"	,0x86	,-1	,-1	,Eb	,Gb	},
	{"xchg"		,0x87	,-1	,-1	,Ev	,Gv	},
	{"mov.b"	,0x88	,-1	,-1	,Eb	,Gb	},
	{"mov"		,0x89	,-1	,-1	,Ev	,Gv	},
	{"mov.b"	,0x8a	,-1	,-1	,Gb	,Eb	},
	{"mov"		,0x8b	,-1	,-1	,Gv	,Ev	},
	{"mov"		,0x8c	,-1	,-1	,Ew	,Sw	},
	{"lea"		,0x8d	,-1	,-1	,Gv	,Ev	},
	{"mov"		,0x8e	,-1	,-1	,Sw	,Ew	},
	{"pop"		,0x8f	,-1	,-1	,Ev	,-1	},
	{"nop"		,0x90	,-1	,-1	,-1	,-1	},
	{"xchg"		,0x91	,-1	,-1	,eAX	,eCX	},
	{"xchg"		,0x92	,-1	,-1	,eAX	,eDX	},
	{"xchg"		,0x93	,-1	,-1	,eAX	,eBX	},
	{"xchg"		,0x94	,-1	,-1	,eAX	,eSP	},
	{"xchg"		,0x95	,-1	,-1	,eAX	,eBP	},
	{"xchg"		,0x96	,-1	,-1	,eAX	,eSI	},
	{"xchg"		,0x97	,-1	,-1	,eAX	,eDI	},
	{"cbw/cwde"	,0x98	,-1	,-1	,-1 ,-1	},
	{"cgq/cwd"	,0x99	,-1	,-1	,-1 ,-1	},
	{"mov"		,0xa0	,-1	,-1	,AL	,Ov	},
	{"mov"		,0xa1	,-1	,-1	,eAX	,Ov	},
	{"mov.b"	,0xa2	,-1	,-1	,Ob	,AL	},
	{"mov"		,0xa3	,-1	,-1	,Ov	,eAX	},
	{"movse_ds:si,es:di"		,0xa4	,-1	,-1	,-1,-1	},
	{"movsw/d_ds:si,es:di"		,0xa4	,-1	,-1	,-1,-1	},
	{"cmpse_ds:si,es:di"		,0xa4	,-1	,-1	,-1,-1	},
	{"cmpsw/d_ds:si,es:di"		,0xa4	,-1	,-1	,-1,-1	},
	{"mov.b"	,0xb0	,-1	,-1	,AL	,Ib	},
	{"mov.b"	,0xb1	,-1	,-1	,CL	,Ib	},
	{"mov.b"	,0xb2	,-1	,-1	,DL	,Ib	},
	{"mov.b"	,0xb3	,-1	,-1	,BL	,Ib	},
	{"mov.b"	,0xb4	,-1	,-1	,AH	,Ib	},
	{"mov.b"	,0xb5	,-1	,-1	,CH	,Ib	},
	{"mov.b"	,0xb6	,-1	,-1	,DH	,Ib	},
	{"mov.b"	,0xb7	,-1	,-1	,BH	,Ib	},
	{"mov"		,0xb8	,-1	,-1	,eAX	,Iv	},
	{"mov"		,0xb9	,-1	,-1	,eCX	,Iv	},
	{"mov"		,0xba	,-1	,-1	,eDX	,Iv	},
	{"mov"		,0xbb	,-1	,-1	,eBX	,Iv	},
	{"mov"		,0xbc	,-1	,-1	,eSP	,Iv	},
	{"mov"		,0xbd	,-1	,-1	,eBP	,Iv	},
	{"mov"		,0xbe	,-1	,-1	,eSI	,Iv	},
	{"mov"		,0xbf	,-1	,-1	,eDI	,Iv	},
	// Shift Group 2
	{"rol.b"	,0xc0	,-1	,0	,Eb	,Ib	},
	{"ror.b"	,0xc0	,-1	,1	,Eb	,Ib	},
	{"rcl.b"	,0xc0	,-1	,2	,Eb	,Ib	},
	{"rcr.b"	,0xc0	,-1	,3	,Eb	,Ib	},
	{"shl.b"	,0xc0	,-1	,4	,Eb	,Ib	},
	{"shr.b"	,0xc0	,-1	,5	,Eb	,Ib	},
	{"sar.b"	,0xc0	,-1	,7	,Eb	,Ib	},
	{"rol"		,0xc1	,-1	,0	,Ev	,Ib	}, // Ib, auch wenn die Tabelle Iv sagt!!!!
	{"ror"		,0xc1	,-1	,1	,Ev	,Ib	},
	{"rcl"		,0xc1	,-1	,2	,Ev	,Ib	},
	{"rcr"		,0xc1	,-1	,3	,Ev	,Ib	},
	{"shl"		,0xc1	,-1	,4	,Ev	,Ib	},
	{"shr"		,0xc1	,-1	,5	,Ev	,Ib	},
	{"sar"		,0xc1	,-1	,7	,Ev	,Ib	},
	{"ret"		,0xc2	,-1	,-1	,Iw	,-1	},
	{"ret"		,0xc3	,-1	,-1	,-1	,-1	},
	{"mov.b"	,0xc6	,-1	,-1	,Eb	,Ib	},
	{"mov"		,0xc7	,-1	,-1	,Ev	,Iv	},
	{"leave"	,0xc9	,-1	,-1	,-1	,-1	},
	{"ret_far"	,0xca	,-1	,-1	,Iw	,-1	},
	{"ret_far"	,0xcb	,-1	,-1	,-1	,-1	},
	{"int"		,0xcd	,-1	,-1	,Ib	,-1	},
	{"iret"		,0xcf	,-1	,-1	,-1	,-1	},
	{"rol"		,0xd3	,-1	,0	,Ev	,CL	},
	{"ror"		,0xd3	,-1	,1	,Ev	,CL	},
	{"rcl"		,0xd3	,-1	,2	,Ev	,CL	},
	{"rcr"		,0xd3	,-1	,3	,Ev	,CL	},
	{"shl"		,0xd3	,-1	,4	,Ev	,CL	},
	{"shr"		,0xd3	,-1	,5	,Ev	,CL	},
	{"sar"		,0xd3	,-1	,7	,Ev	,CL	},
	{"add.f"	,0xd8	,-1	,0	,Ev	,-1	},
	{"mul.f"	,0xd8	,-1	,1	,Ev	,-1	},
	{"Ssub.f"	,0xd8	,-1	,4	,Ev	,-1	},
	{"div.f"	,0xd8	,-1	,6	,Ev	,-1	},
	{"push.f"	,0xd9	,-1	,0	,Ev	,-1	},
	{"mov.f"	,0xd9	,-1	,3	,Ev	,-1	},
	{"push.fi"	,0xdb	,-1	,0	,Ev	,-1	},
	{"+pop.f"	,0xde	,-1	,0	,Ev	,-1	},
	{"*pop.f"	,0xde	,-1	,1	,Ev	,-1	},
	{"-pop.f"	,0xde	,-1	,5	,Ev	,-1	},
	{"/pop.f"	,0xde	,-1	,7	,Ev	,-1	},
	{"loopne"	,0xe0	,-1	,-1	,Ib	,-1	},
	{"loope"	,0xe1	,-1	,-1	,Ib	,-1	},
	{"loop"		,0xe2	,-1	,-1	,Ib	,-1	},
	{"in.b"		,0xe4	,-1	,-1	,AL	,Ib	},
	{"in.b"		,0xe5	,-1	,-1	,eAX,Ib	},
	{"out.b"	,0xe6	,-1	,-1	,Ib	,AL	},
	{"out.b"	,0xe7	,-1	,-1	,Ib	,eAX},
	{"call"		,0xe8	,-1	,-1	,Iv	,-1	},
	{"jmp"		,0xe9	,-1	,-1	,Iv	,-1	},
	{"jmp"		,0xea	,-1	,-1	,Ip	,-1	},
	{"jmp.b"	,0xeb	,-1	,-1	,Ib	,-1	},
	{"in"		,0xec	,-1	,-1	,AL	,DX	},
	{"in"		,0xed	,-1	,-1	,eAX,DX	},
	{"out"		,0xee	,-1	,-1	,DX	,AL	},
	{"out"		,0xef	,-1	,-1	,DX	,eAX},
	{"lock"		,0xf0	,-1	,-1	,-1	,-1	},
	{"repne"	,0xf2	,-1	,-1	,-1	,-1	},
	{"rep"		,0xf3	,-1	,-1	,-1	,-1	},
	{"hlt"		,0xf4	,-1	,-1	,-1	,-1	},
	{"cmc"		,0xf5	,-1	,-1	,-1	,-1	},
	// Unary Group 3
	{"test.b"	,0xf6	,-1	,0	,Eb	,Ib	},
	{"not.b"	,0xf6	,-1	,2	,Eb	,-1	},
	{"neg.b"	,0xf6	,-1	,3	,Eb	,-1	},
	{"mul.b"	,0xf6	,-1	,4	,AL	,Eb	},
	{"imul.b"	,0xf6	,-1	,5	,AL	,Eb	},
	{"div.b"	,0xf6	,-1	,6	,AL	,Eb	},
	{"idiv.b"	,0xf6	,-1	,7	,Eb	,-1	},
	{"test"		,0xf7	,-1	,0	,Ev	,Iv	},
	{"not"		,0xf7	,-1	,2	,Ev	,-1	},
	{"neg"		,0xf7	,-1	,3	,Ev	,-1	},
	{"mul"		,0xf7	,-1	,4	,eAX	,Ev	},
	{"imul"		,0xf7	,-1	,5	,eAX	,Ev	},
	{"div"		,0xf7	,-1	,6	,eAX	,Ev	},
	{"idiv"		,0xf7	,-1	,7	,eAX	,Ev	},
	{"clc"		,0xf8	,-1	,-1	,-1	,-1	},
	{"stc"		,0xf9	,-1	,-1	,-1	,-1	},
	{"cli"		,0xfa	,-1	,-1	,-1	,-1	},
	{"sti"		,0xfb	,-1	,-1	,-1	,-1	},
	{"cld"		,0xfc	,-1	,-1	,-1	,-1	},
	{"std"		,0xfd	,-1	,-1	,-1	,-1	},
	{"inc.b"	,0xfe	,-1	,0	,Eb	,-1	},
	{"dec.b"	,0xfe	,-1	,1	,Eb	,-1	},
	{"push"		,0xff	,-1	,6	,Ev	,-1	},
	{"inc"		,0xff	,-1	,0	,Ev	,-1	},
	{"dec"		,0xff	,-1	,1	,Ev	,-1	},
	{"call"		,0xff	,-1	,2	,Ev	,-1	},
	{"call_far"	,0xff	,-1	,3	,Ev	,-1	}, // Ep statt Ev...
	{"jmp"		,0xff	,-1	,4	,Ev	,-1	},
	{"push"		,0xff	,-1	,5	,Ev	,-1	},
};

bool mode16;
bool small_param;
bool small_add;

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
	if (p==Ev)	p=small_param?Ew:Ed;
	if (p==Iv)	p=small_param?Iw:Id;
	if (p==Gv)	p=small_param?Gw:Gd;
	if (p==Ov)	p=small_param?Ow:Od;
	if (small_param){
		if (p==eAX)	p=AX;
		if (p==eCX)	p=CX;
		if (p==eDX)	p=DX;
		if (p==eBX)	p=BX;
		if (p==eSP)	p=SP;
		if (p==eBP)	p=BP;
		if (p==eSI)	p=SI;
		if (p==eDI)	p=DI;
	}
}

char *GetAsm(char *code,int length,bool allow_comments)
{
	char param[256],*opcode;
	strcpy(buffer,"");
	char *end=code+length;
	char *orig=code;
	if (length<0)	end=code+65536;
	int i;
	char *cur=code;
	mode16=false;
	if (CurrentAsmMetaInfo)
		mode16=CurrentAsmMetaInfo->Mode16;
	while((int)code<(int)end){
		opcode=cur;
		bool done=false;
		if (CurrentAsmMetaInfo){
			for (i=0;i<CurrentAsmMetaInfo->NumLabels;i++)
				if ((int)code-(int)orig==CurrentAsmMetaInfo->LabelPos[i]){
					strcat(buffer,"    ");
					strcat(buffer,CurrentAsmMetaInfo->LabelName[i]);
					strcat(buffer,":\n");
				}
			for (i=0;i<CurrentAsmMetaInfo->NumDatas;i++)
				if ((int)code-(int)orig==CurrentAsmMetaInfo->DataPos[i]){
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
			for (i=0;i<CurrentAsmMetaInfo->NumBitChanges;i++)
				if ((int)code-(int)orig==CurrentAsmMetaInfo->BitChangePos[i]){
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
		for (i=0;i<NumInstructions;i++)
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
				for (int i=0;i<48-l;i++)
					strcat(buffer," ");
				strcat(buffer,"// ");
				strcat(buffer,d2h(code,1+int(cur)-int(code),false));
			}
		}else
			strcat(buffer,string("????? -                          unbekannt       ",d2h(code,1+int(cur)-int(code),false)));
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

char *GetMne(int &pos)
{
	EndOfLine=false;
	bool CommentLine=false;
	int i;
	strcpy(mne,"");

	// Kommentare und "white space" ueberspringen
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
		// Kommentare
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
		
		// String-aehnliches Gebilde
		if ((mne[i]=='\'')||(mne[i]=='\"'))
			in_string=!in_string;
		// Code-Ende
		if (code_buffer[pos+i]==0){
			EndOfCode=true;
			EndOfLine=true;
			break;
		}
		// Zeilen-Ende
		if (code_buffer[pos+i]=='\n'){
			LineNo++;
			EndOfLine=true;
			break;
		}
		if (!in_string){
			// "white space"
			if ((code_buffer[pos+i]==' ')||(code_buffer[pos+i]=='\t')||(code_buffer[pos+i]==',')){
				// Zeilen-Ende?
				int j;
				for (j=0;j<12;j++){
					if ((code_buffer[pos+i+j]!=' ')&&(code_buffer[pos+i+j]!='\t')&&(code_buffer[pos+i+j]!=',')){
						if ((code_buffer[pos+i+j]==0)||(code_buffer[pos+i+j]=='\n'))
							EndOfLine=true;
						// Kommentar beendet auch die Zeile
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

enum{
	PKInvalid,
	PKNone,
	PKRegister,
	PKDerefRegister,
	PKConstant,
	PKConstantDouble, // 0x00:0x0000
	PKDerefConstant
};

bool ConstantIsLabel,UnknownLabel;

void GetParam(int &pk,int &p,char *param,int pn)
{
	int i;
	pk=PKInvalid;
	if (strlen(param)>0){
		if ((param[0]=='[')&&(param[strlen(param)-1]==']')){
			if (DebugAsm)
				printf("Deref:   ");
			char deref[128];
			strcpy(deref,&param[1]);
			deref[strlen(deref)-1]=0;
			int drpk,drp;
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
		for (i=0;i<NumRegisters;i++)
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
			p=(int)ps;
			pk=PKConstant;
		}
		if ((param[0]=='0')&&(param[1]=='x')){
			p=0;
			pk=PKConstant;
			for (i=2;(unsigned)i<strlen(param);i++){
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
					int pk2;
					GetParam(pk2,ParamConstantDouble,&param[i+1],2);
					if (pk2!=PKConstant){
						AsmError(string("Fehler in Hex-Parameter:  ",param));
						pk=PKInvalid;
						return;						
					}
					pk=PKConstantDouble;
					break;
				}else{
					AsmError(string("Unerlaubtes Zeichen in Hex-Parameter:  ",param));
					pk=PKInvalid;
					return;
				}
			}
			if (DebugAsm)
				if (pk==PKConstantDouble)
					printf("Hex-Const:  %s:%s\n",d2h((char*)&p,2),d2h((char*)&ParamConstantDouble,4));
				else
					printf("Hex-Const:  %s\n",d2h((char*)&p,4));
			return;
		}
		if ((param[0]=='\'')&&(param[strlen(param)-1]=='\'')){
			p=param[1];
			pk=PKConstant;
			if (DebugAsm)
				printf("Hex-Const:  %s\n",d2h((char*)&p,4));
		}
		if (strcmp(param,"$")==0){
			p=CurrentAsmMetaInfo->CurrentOpcodePos+CurrentAsmMetaInfo->CodeOrigin;
			pk=PKConstant;
			ConstantIsLabel=true;
			if (DebugAsm)
				printf("Label:  %s\n",param);
			return;
		}
		if (CurrentAsmMetaInfo){
			// schon vorhandenes Label
			for (i=0;i<CurrentAsmMetaInfo->NumLabels;i++)
				if (strcmp(CurrentAsmMetaInfo->LabelName[i],param)==0){
					p=CurrentAsmMetaInfo->LabelPos[i]+CurrentAsmMetaInfo->CodeOrigin;
					pk=PKConstant;
					ConstantIsLabel=true;
					if (DebugAsm)
						printf("Label:  %s\n",param);
					return;
				}
			// C-Script Variabel (global)
			for (i=0;i<*(CurrentAsmMetaInfo->NumGlobalVars);i++){
				char *name=&CurrentAsmMetaInfo->GlobalVarNames[i*CurrentAsmMetaInfo->GlobalVarNameSize];
				if (strcmp(name,param)==0){
					p=CurrentAsmMetaInfo->GlobalVarsOffset-CurrentAsmMetaInfo->GlobalVarOffset[i];
					pk=PKDerefConstant;
					if (DebugAsm)
						printf("Globale Variable:  %s\n",param);
					return;
				}
			}
			// noch nicht vorhandenes Label...
			if (param[0]=='_'){
				strcpy(CurrentAsmMetaInfo->WantedLabelName[CurrentAsmMetaInfo->NumWantedLabels],param);
				CurrentAsmMetaInfo->WantedLabelAdd[CurrentAsmMetaInfo->NumWantedLabels]=0;
				CurrentAsmMetaInfo->WantedLabelPN[CurrentAsmMetaInfo->NumWantedLabels]=pn;
				CurrentAsmMetaInfo->NumWantedLabels++;
				if (DebugAsm)
					printf("Wanted-Label:  %s\n",param);
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
		AsmError(string("Unbekannter Parameter:  ",param));
}

bool ParamCompatible(int pk,int p,int param)
{
	if ((pk==PKNone)&&(param==-1))
		return true;
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
			// die Konstante ist nur ein Platzhalter, da ihr Wert noch nicht bekannt ist!
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

char *SetAsm(char *code)
{
	AsmCodeLength=0; // Block-Anfang -> aktuelles Zeichen
	PreInsertionLength=0; // Position des Blockes im gesammten Opcode
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
	int p1,p2;
	int pk1,pk2;
	int i;
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
			buffer[AsmCodeLength]=p1;	AsmCodeLength++;
			done=true;
		}
		if (strcmp(cmd,"dw")==0){
			so("Daten:   2 byte");
			if (CurrentAsmMetaInfo){
				CurrentAsmMetaInfo->DataPos[CurrentAsmMetaInfo->NumDatas]=CurrentAsmMetaInfo->CurrentOpcodePos;
				CurrentAsmMetaInfo->DataSize[CurrentAsmMetaInfo->NumDatas]=2;
				CurrentAsmMetaInfo->NumDatas++;
			}
			*(short*)&buffer[AsmCodeLength]=p1;	AsmCodeLength+=2;
			done=true;
		}
		if (strcmp(cmd,"dd")==0){
			so("Daten:   4 byte");
			if (CurrentAsmMetaInfo){
				CurrentAsmMetaInfo->DataPos[CurrentAsmMetaInfo->NumDatas]=CurrentAsmMetaInfo->CurrentOpcodePos;
				CurrentAsmMetaInfo->DataSize[CurrentAsmMetaInfo->NumDatas]=4;
				CurrentAsmMetaInfo->NumDatas++;
			}
			*(int*)&buffer[AsmCodeLength]=p1;	AsmCodeLength+=4;
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
				for (i=0;i<CurrentAsmMetaInfo->NumWantedLabels;i++)
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
		for (i=0;i<NumInstructions;i++)
			if (strcmp(Instruction[i].name,cmd)==0){
				found_any=true;
				int _AsmCodeLength=AsmCodeLength;
				int ip1=Instruction[i].param1;
				int ip2=Instruction[i].param2;
				if (use_mode16){
					if (ip1==Iv)	ip1=Iw;
					if (ip1==Ev)	ip1=Ew;
					if (ip1==Gv)	ip1=Gw;
					if (ip2==Iv)	ip2=Iw;
					if (ip2==Ev)	ip2=Ew;
					if (ip2==Gv)	ip2=Gw;
					if (ip1==eAX)	ip1=AX;
					if (ip1==eCX)	ip1=CX;
					if (ip1==eDX)	ip1=DX;
					if (ip1==eBX)	ip1=BX;
					if (ip1==eSP)	ip1=SP;
					if (ip1==eBP)	ip1=BP;
					if (ip1==eSI)	ip1=SI;
					if (ip1==eDI)	ip1=DI;
					if (ip2==eAX)	ip2=AX;
					if (ip2==eCX)	ip2=CX;
					if (ip2==eDX)	ip2=DX;
					if (ip2==eBX)	ip2=BX;
					if (ip2==eSP)	ip2=SP;
					if (ip2==eBP)	ip2=BP;
					if (ip2==eSI)	ip2=SI;
					if (ip2==eDI)	ip2=DI;
				}else{
					if (ip1==Iv)	ip1=Id;
					if (ip1==Ev)	ip1=Ed;
					if (ip1==Gv)	ip1=Gd;
					if (ip2==Iv)	ip2=Id;
					if (ip2==Ev)	ip2=Ed;
					if (ip2==Gv)	ip2=Gd;
				}
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

					if (use_mode16!=mode16){
						buffer[AsmCodeLength]=0x66;
						AsmCodeLength++;
					}

					buffer[AsmCodeLength]=Instruction[i].code;	AsmCodeLength++;
					if (Instruction[i].code_alt>=0)
					{	buffer[AsmCodeLength]=Instruction[i].code_alt;	AsmCodeLength++;	}
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
						buffer[AsmCodeLength]=rm;	AsmCodeLength++;
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
			AsmError(string("Unbekannter Befehl:  ",cmd));
			AsmCodeLength=-1;
			return NULL;
		}
		if (!found_valid){
			AsmError(string("Befehl nicht mit Parametern kompatibel:  ",cmd," ",param1,", ",param2));
			AsmCodeLength=-1;
			return NULL;
		}

		if (EndOfCode)
			break;
	}
	return buffer;
}
