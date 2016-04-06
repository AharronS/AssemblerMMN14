#ifndef MMN14_H
#define MMN14_H

#include <string.h> /*for strcmp(),strcpy()*/
#include <stdlib.h> /*for malloc()*/
#include <stdio.h>  /*for FILE */
#include <ctype.h>  /*for isDigit,isSpace*/


#define MAXSYMBOLS 1000 /*specs page 49, possible to use arrays*/
#define MAX_SIZE_MEMORY_WORDS 1000
#define MAXTAGLENGTH 30
#define MAXLINELENGTH 80
#define COUNTERSTARTLINE 100
#define MAX_SIZE_OF_RANDOM_PREFIX_COMMAND 4
#define BASE_32_ROW_COMMAND 4
#define MACHINE_CODE_ROW_LENGTH 15
#define BASE32_SINGLE_NUMBER_LENGTH 5

enum {NO,YES};
enum {DATA,STRING,EXTERN,ENTRY,COMMAND,ERROR} lineType_e;
enum {SUCCESS,SYMBOL_EXISTS,FULL_TABLE} error_e;

typedef struct  machineCodeWord {
	char  line[15];
} MachineCodeWord;

typedef struct  CommandMachineCode {
	int linesCount;
	MachineCodeWord lines[3];
} commandMachineCodeWord;


typedef enum { C_RND = 100, C_GROUP, C_OPCODE, C_ADDRESS_SRC, C_ADDRESS_DST,
				C_CODING, C_FILL_WITH_OP, C_FILL_REG_DEST, C_FILL_REG_SRC } WordComponent;

typedef struct  st{
        char tag[MAXTAGLENGTH];
        int address;
        int isTemp;
        int isExtern;
        int associatedTo;
		int commandLength;	/*is 1 by default and it's set to be the actual command length if*/
							/*the tag in the line is unique*/
} symbolRow;

typedef struct {
		int currentSymbol;	/*next free symbol*/
        symbolRow row[MAXSYMBOLS];
} symbolTable;

char duplicatedTags[MAXSYMBOLS][MAXTAGLENGTH];

enum	{IMMEDIATE = 0, DIRECT, RANDOM, DIRECTREG, BADADDRESSFORM, NULLOPERANDS} addressForm_e;

enum {RANDOM1 = 1000, RANDOM2, RANDOM3} randomTypes_e;

enum { MOV = 0, CMP, ADD, SUB, NOT, CLR, LEA, INC, DEC, JMP, BNE, RED, PRN, JSR, RTS, STOP } commands_e;

/*******read description of each function in the source file***********/

/*in firstPass:*/
void updateIC(int);
void updateDC(int);
int getIC();
int getDC();
void setIC(int);
void setDC(int);
int stringLength(char *str);
int getAddressForm(char *op);
int checkCommandAddressForm(char *op1, char *op2, int opcode);
int getCommandLineLength(char *op1, char *op2, int opcode);
int firstPass(char *tag, int instructionType, int opcode, char *op1, char *op2);
int CheckRandomType(char *op);

/*in secondPass:*/
void renewDataRowNum(int icVal);
void setRowNum(int);
void setDataRowNum(int);
void removeSpaces(char(*res)[], char* stringIn, int stringLength);
void getOpWord(char (*dest)[],char op[], int opAddressForm, FILE *ext);
void getCommand(char (*dest)[],char *op1, char *op2, int opcode, FILE *ext);
int commandAddressFormIsLegal(char *op1, char *op2, int opcode);
int secondPass(char *tag, int instructionType, int opcode, char *op1, char *op2, FILE *fp, FILE *ext, FILE *errorFile);
void WriteToFileCommandMachineWord(commandMachineCodeWord *wordCommand, FILE *fp, int rowNum);

/*in symbolTable:*/
void printTable();
void deleteTable();
void setLastCommandLength(int len);
int hasADuplicate(char *tag);
void addDuplicatedSymbol(char *tag);
void init();
int addSymbol (char symbol[], int lineType, int counterVal);
symbolRow *getRow(char symbol[]);
int update(int icVal, int dcVal,FILE *obj);


/*in parser:*/
int parser(char line[],char (*rettag)[],int (*retInstrOpcode)[],char (*retop1)[],char (*retop2)[]);
void TagChecker(char* partstr,char (*strret)[]);
int InstructionChecker(char* instr,char* op1, char* op2,char (*retstr)[]);
void OperandNumChecker(int NumExpectedOperand,char* op1, char* op2,char (*retstr)[]);
int CompilerInstrChecker(char* instr);

/*in dataSegment:*/
void newDataSegment();
void addData(char dataRow[]);
void deleteDataSegment();
void printData(FILE *fp);

#endif





