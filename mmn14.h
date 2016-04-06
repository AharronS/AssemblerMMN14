#include <string.h> /*for strcmp(),strcpy()*/
#include <stdlib.h> /*for malloc()*/
#include <stdio.h>  /*for FILE */
#include <ctype.h>  /*for isDigit,isSpace*/

#define MAXSYMBOLS 1000 /*specs page 49, possible to use arrays*/
#define MAXTAGLENGTH 30
#define MAXLINELENGTH 80
#define COUNTERSTARTLINE 0

enum {NO,YES};
enum {DATA,STRING,EXTERN,ENTRY,COMMAND,ERROR} lineType_e;
enum {SUCCESS,SYMBOL_EXISTS,FULL_TABLE} error_e;

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

enum	{IMMEDIATE = 0, DIRECT = 1, RELATIONAL = 3,
		 DIRECTREG = 4, BADADDRESSFORM = 5, NULLOPERANDS = 6} addressForm_e;
enum {MOV,CMP,ADD,SUB,ROR,SHR,LEA,INC,DEC,JMP,BNE,RED,PRN,JSR,RTS,HLT} commands_e;

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

/*in secondPass:*/
void renewDataRowNum(int icVal);
void setRowNum(int);
void setDataRowNum(int);
void removeSpaces(char (*res)[], char* stringIn);
void getOpWord(char (*dest)[],char op[], int opAddressForm, FILE *ext);
void getCommand(char (*dest)[],char *op1, char *op2, int opcode, FILE *ext);
int commandAddressFormIsLegal(char *op1, char *op2, int opcode);
int secondPass(char *tag, int instructionType, int opcode, char *op1, char *op2, FILE *fp, FILE *ext);

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







