#define ASM_MAX_DATAS			512
#define ASM_MAX_BIT_CHANGES		64
#define ASM_MAX_LABELS			256
#define ASM_MAX_WANTED_LABELS	256
#define ASM_MAX_LABEL_NAME		32

struct sAsmMetaInfo{
	int CurrentOpcodePos,CodeOrigin;
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
	int GlobalVarsOffset,GlobalVarNameSize;
	char *GlobalVarNames;
	int *GlobalVarOffset;
};

char *GetAsm(char *code,int length,bool allow_comments=true);
char *SetAsm(char *code);
extern int AsmCodeLength;
extern sAsmMetaInfo *CurrentAsmMetaInfo;
