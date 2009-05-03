#define ASM_MAX_DATAS			512
#define ASM_MAX_BIT_CHANGES		64
#define ASM_MAX_LABELS			256
#define ASM_MAX_WANTED_LABELS	256
#define ASM_MAX_LABEL_NAME		32


enum{
	InstructionSetDefault,
	InstructionSetX86,
	InstructionSetAMD64
};

enum{
	Eb,Ev,Ew,Ed,Gb,Gv,Gw,Gd,
	Ib,Iv,Iw,Id,Ob,Ov,Ow,Od,Sw,Cd,Ip,
	eAX,eCX,eDX,eBX,eSP,eSI,eDI,eBP,
	AX,CX,DX,BX,BP,SP,SI,DI,
	AL,CL,DL,BL,AH,CH,DH,BH,
	CS,DS,SS,ES,FS,GS,
	CR0,CR1,CR2,CR3,
	ST0,ST1,ST2,ST3,ST4,ST5,ST6,ST7,
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
	d16_pBX_pSI,d16_pBX_pDI,d16_pBP_pSI,d16_pBP_pDI,
		
	PKInvalid,
	PKNone,
	PKRegister,			// eAX
	PKDerefRegister,	// [eAX]
	PKConstant,			// 0x0000
	PKConstantDouble,   // 0x00:0x0000
	PKDerefConstant		// [0x0000]
};


enum{
	inst_add,
	inst_add_b,
	inst_adc,	   // add with carry
	inst_adc_b,
	inst_sub,
	inst_sub_b,
	inst_sbb,	   // subtract with borrow
	inst_sbb_b,
	inst_inc,
	inst_inc_b,
	inst_dec,
	inst_dec_b,
	inst_mul,
	inst_mul_b,
	inst_imul,
	inst_imul_b,
	inst_div,
	inst_div_b,
	inst_idiv,
	inst_idiv_b,
	inst_mov,
	inst_mov_b,
	inst_movzx,
	inst_movsx,
	inst_and,
	inst_and_b,
	inst_or,
	inst_or_b,
	inst_xor,
	inst_xor_b,
	inst_not,
	inst_not_b,
	inst_neg,
	inst_neg_b,
	inst_pop,
	inst_popa,
	inst_push,
	inst_pusha,
	
	inst_jo,
	inst_jo_b,
	inst_jno,
	inst_jno_b,
	inst_jb,
	inst_jb_b,
	inst_jnb,
	inst_jnb_b,
	inst_jz,
	inst_jz_b,
	inst_jnz,
	inst_jnz_b,
	inst_jbe,
	inst_jbe_b,
	inst_jnbe,
	inst_jnbe_b,
	inst_js,
	inst_js_b,
	inst_jns,
	inst_jns_b,
	inst_jp,
	inst_jp_b,
	inst_jnp,
	inst_jnp_b,
	inst_jl,
	inst_jl_b,
	inst_jnl,
	inst_jnl_b,
	inst_jle,
	inst_jle_b,
	inst_jnle,
	inst_jnle_b,
	
	inst_cmp,
	inst_cmp_b,
	
	inst_seto_b,
	inst_setno_b,
	inst_setb_b,
	inst_setnb_b,
	inst_setz_b,
	inst_setnz_b,
	inst_setbe_b,
	inst_setnbe_b,
	inst_sets,
	inst_setns,
	inst_setp,
	inst_setnp,
	inst_setl,
	inst_setnl,
	inst_setle,
	inst_setnle,
	
	inst_sldt,
	inst_str,
	inst_lldt,
	inst_ltr,
	inst_verr,
	inst_verw,
	inst_sgdt,
	inst_sidt,
	inst_lgdt,
	inst_lidt,
	inst_smsw,
	inst_lmsw,
	
	inst_test,
	inst_test_b,
	inst_xchg,
	inst_xchg_b,
	inst_lea,
	inst_nop,
	inst_cbw_cwde,
	inst_cgq_cwd,
	inst_movs_ds_esi_es_edi,	// mov string
	inst_movs_b_ds_esi_es_edi,
	inst_cmps_ds_esi_es_edi,	// cmp string
	inst_cmps_b_ds_esi_es_edi,
	inst_rol,
	inst_rol_b,
	inst_ror,
	inst_ror_b,
	inst_rcl,
	inst_rcl_b,
	inst_rcr,
	inst_rcr_b,
	inst_shl,
	inst_shl_b,
	inst_shr,
	inst_shr_b,
	inst_sar,
	inst_sar_b,
	inst_ret,
	inst_leave,
	inst_ret_far,
	inst_int,
	inst_iret,
	
	inst_fadd,
	inst_fmul,
	inst_fsub,
	inst_fdiv,
	inst_fld,
	inst_fstp,
	inst_fild,
	inst_faddp,
	inst_fmulp,
	inst_fsubp,
	inst_fdivp,
	
	inst_loop,
	inst_loope,
	inst_loopne,
	inst_in,
	inst_in_b,
	inst_out,
	inst_out_b,
	
	inst_call,
	inst_call_far,
	inst_jmp,
	inst_jmp_b,
	inst_lock,
	inst_rep,
	inst_repne,
	inst_hlt,
	inst_cmc,
	inst_clc,
	inst_stc,
	inst_cli,
	inst_sti,
	inst_cld,
	inst_std,
	
	NumInstructionNames
};

struct sAsmMetaInfo{
	long CurrentOpcodePos,CodeOrigin;
	char *Opcode;
	bool Mode16;
	int LineOffset;

	int NumLabels;
	int LabelPos[ASM_MAX_LABELS];
	char LabelName[ASM_MAX_LABELS][ASM_MAX_LABEL_NAME];

	int NumWantedLabels;
	int WantedLabelPos[ASM_MAX_WANTED_LABELS],WantedLabelSize[ASM_MAX_WANTED_LABELS];
	int WantedLabelAdd[ASM_MAX_WANTED_LABELS],WantedLabelPN[ASM_MAX_WANTED_LABELS];
	char WantedLabelName[ASM_MAX_WANTED_LABELS][ASM_MAX_LABEL_NAME];

	int NumDatas;
	int DataPos[ASM_MAX_DATAS],DataSize[ASM_MAX_DATAS];

	int NumBitChanges;
	int BitChangePos[ASM_MAX_BIT_CHANGES],BitChange[ASM_MAX_BIT_CHANGES];

	int *NumGlobalVars;
	int GlobalVarNameSize;
	char *GlobalVarNames;
	char **GlobalVarPos;
};

char *Opcode2Asm(char *code,int length,bool allow_comments=true);
char *Asm2Opcode(char *code);
void OpcodeAddInstruction(char *oc,int &ocs,int inst,int param1_type,int param2_type,void *param=0,int offset=0,int insert_at=-1);
void SetInstructionSet(int set);
extern int AsmCodeLength;
extern sAsmMetaInfo *CurrentAsmMetaInfo;
