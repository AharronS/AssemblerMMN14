#include "mmn14.h"
#define DEBUGMODE 1
#define printWarning();\
{\
	printf("ATTENTION: DURING THE FIRST PASS, A FUNCTION REPORTED THAT -\n");\
	printf("THE SYMBOL TABLE IS FULL. THIS IS A RESULT OF THE INPUT FILE HAVING -\n");\
	printf("TOO MANY TAGS, AND THE OUTPUT MAY BE WRONG.\n");\
	printf("Please see \"symbolTable.h\" for more information.\n\n");\
}


static int fullTableWarningPrinted = NO;



/*
instruction type = data\command\string\extern\entry\error (invalid tag or illegal command or anything else?)
op1,op2 = opernads. in case of a command - operands 1,2. any oher case - 
op1 will have the error msg\the data\the string\anything else (is there anything else?)
and op2 should be NULL (not used anyway).
opcode is the command opcode (in case of a command).
should be -1 in any other case (but it's not used, anyway). 
*/

static int IC; /*INSTRUCTION COUNTER*/
static int DC; /*DATA COUNTER*/

void updateIC(int updateVal)	/*called in case of a legal (command) line turned out to be an illegal line,*/
								/*due to a duplicated tag*/
{
	IC-=updateVal;
}

void updateDC(int updateVal)	/*called in case of a legal (data) line turned out to be an illegal line,*/
								/*due to a duplicated tag*/
{
	DC-=updateVal;
}

int CheckRandomType(char *op)
{
	char tempRandomType[MAX_SIZE_OF_RANDOM_PREFIX_COMMAND];

	strncpy(tempRandomType, op, 3);
	if (strcmp("***", tempRandomType) == 0)
	{
		return RANDOM3;
	}

	else if (strcmp("**", tempRandomType) == 0)
	{
		return RANDOM2;
	}
	else if (strcmp("*", tempRandomType) == 0)
	{
		return RANDOM1;
	}
	return BADADDRESSFORM;
}

/*get counters: interface for other files*/
int getIC()
{
	return IC;
}

int getDC()
{
	return DC;
}

void setIC(int val)
{
	IC = val;
}

void setDC(int val)
{
	DC = val;
}

int stringLength(char *str)
{
	int i=0;
	if (*str != '"') /*string must start with a " */
		return -1; /*error, length must be positive*/
	while (str[++i]!='\0')
		;
	if (str[--i]!='"')/*string must end with a " */
		return -1;
	if (i>0) /*i=0 in case of a string: " (only " is also illegal)*/
		return i;
	return -1;
}

int getAddressForm(char *op) /*assuming clean input (no spaces and stuff)*/
{
	if (*op == '#')
	{
		op++;
		if ((*op == '+') || (*op == '-') || isdigit(*op))
		{
			while (isdigit(*++op))
				;
			if (*op == '\0') /*end of the operand*/
				return IMMEDIATE;
		}
	}
	else if (*op == '*')
	{	/*if the tag is illegal, then an error will be printed later*/
		/*anything after the '*' is the tag as much as we care here*/
		//TODO:(AS): need to change to random, acording to the definition
		return RELATIONAL;
	}
	
	/*
	//TODO:(AS): replace this statment
	else if (*op == '*')
	{	
		return CheckRandomType(op);
	} 
	*/

	else if (*op == 'r')
	{
		op++;
		if ((*op >= '0') && (*op<='7'))
			if (*++op == '\0')
				return DIRECTREG;
	}
	return DIRECT; /*op has a label.*/
}

int checkCommandAddressForm(char *op1, char *op2, int opcode)
{	
	/*checks if the command operands are legal according to the command.*/
	/*by table in page 31 (specs.); if everything is legal: returns the-*/
	/*total number of lines for this command (less than or equal to 3).*/
	/*if there is an error - returns BADADDRESSFORM (=5), see enum addressForm_e.*/
	
	int op1address, op2address, lines = 1;
	switch (opcode)
	{
		case MOV:
		case ADD:
		case SUB:
		case ROR: //TODO:(AS): remove this
		case SHR: //TODO:(AS): remove this
			//TODO:(AS): lines = 3;
					if (op1==NULL || op2==NULL || *op1=='\0' || *op2=='\0')
						return NULLOPERANDS;
					op2address = getAddressForm(op2);
					
					if (op2address == IMMEDIATE)
						return BADADDRESSFORM;
						
					op1address = getAddressForm(op1);
					
					if (op1address != DIRECTREG)  //TODO:(AS): need to chage this, because there is at least one additional word memory
						//TODO:(AS): we need to init this with 2, only stop and rts get one word memory
						lines++;
					if (op2address != DIRECTREG) 
						/*
						TODO:(AS): change this if statment.
						if(op1address == DIRECTREG && op2address == DIRECTREG)
							lines--;
						*/
						lines++;
					
					break;

		case CMP:
					//TODO:(AS): lines = 3;
					if (op1==NULL || op2==NULL || *op1=='\0' || *op2=='\0')
						return NULLOPERANDS;
					op1address = getAddressForm(op1);
					op2address = getAddressForm(op2);
					if (op1address != DIRECTREG)
						//TODO:(AS): need to chage this, because there is at least one additional word memory
						//TODO:(AS): we need to init this with 2, only stop and rts get one word memory
						lines++;
					if (op2address != DIRECTREG)
						/*
						TODO:(AS): change this if statment.
						if(op1address == DIRECTREG && op2address == DIRECTREG)
						lines--;
						*/
						lines++;
					
					break;

		case LEA:
					if (op1==NULL || op2==NULL || *op1=='\0' || *op2=='\0')
						return NULLOPERANDS;
					op1address = getAddressForm(op1);
					op2address = getAddressForm(op2);
					if (op1address != DIRECT || op2address == IMMEDIATE) 
						/*TODO:(AS): only direct, or random with ***
						if (!(op1address == DIRECT || op1 == RAND(***))
						*/
						return BADADDRESSFORM;
					
					/*op1 needs 1 word at least. (addressing\mapping form 1, direct)*/
					/*op2 may vary. Therefore, check it:*/
					//TODO:(AS): There is no chance that the command does not use 3 words, need to remove this statment
					if (op2address != DIRECTREG)
						lines+=2;
					else
						lines++;

					break;

		case INC:
		/*
		TODO:(AS): add this:
		case NOT:
		case CLR:
		*/
		case DEC:
		case RED:
					
					/*op1 should be NULL (see table on specs. page 31)*/
					if (DEBUGMODE && (op2 != NULL || *op2 != '\0'))
					{
						printf("ERROR!!\n*********************\n");
						printf("checkCommandAddressForm reports op1 is not NULL!\n");
						printf("op1 = %s\t op2 = %s\n",op1,op2);
					}
					if (op1==NULL || *op1=='\0')
						return NULLOPERANDS;
					op1address = getAddressForm(op1);
					
					if (op1address == IMMEDIATE)
						/*TODO:(AS): change this statment to 
						if (op1address == IMMEDIATE || op1address == RAND)
						*/
						return BADADDRESSFORM;
					if (op1address != DIRECTREG)//TODO:(AS): remove the statment, remain the line++;
						lines++;
						
					break;

		case JMP:
		case BNE:
		case JSR:
					if (DEBUGMODE && (op2 != NULL || *op2 != '\0'))
					{
						printf("ERROR!!\n*********************\n");
						printf("checkCommandAddressForm (JMP\\BNE\\JSR) reports op1 is not NULL!\n");
						printf("op1 = %s\t op2 = %s\n",op1,op2);
					}
					if (op1==NULL || *op1=='\0')
						return NULLOPERANDS;
					
					op1address = getAddressForm(op1);
					
					if (op1address == IMMEDIATE || op1address == DIRECTREG)
						return BADADDRESSFORM;
						
					/*no option for DIRECTREG addressing ==> in any case lines = 2*/
					lines++;
					
					break;

		case PRN:
					if (DEBUGMODE && (op2 != NULL || *op2 != '\0'))
					{
						printf("ERROR!!\n*********************\n");
						printf("checkCommandAddressForm (PRN) reports op1 is not NULL!\n");
						printf("op1 = %s\t op2 = %s\n",op1,op2);
					}
					if (op1==NULL || *op1=='\0')
						return NULLOPERANDS;
					
					op1address = getAddressForm(op1);
					/*
					TODO:(AS):
					if (op1address == RAND)
						return BADADDRESSFORM;
					*/
					if (op1address != DIRECTREG) 
						/*
						TODO:(AS): remove this statment
						*/
						lines++;
					
					break;

		case RTS:
		//TODO:(AS): Change to STOP
		case HLT:
					if (DEBUGMODE && (op1 != NULL || *op1 != '\0') && (op2 != NULL || *op2 != '\0'))
					{
						printf("ERROR!!\n*********************\n");
						printf("checkCommandAddressForm (RTS\\HLT) reports op1 or op2 is not NULL!\n");
						printf("op1 = %s\t op2 = %s\n",op1,op2);
					}
					
					break;
	}
	return lines;
}

int getCommandLineLength(char *op1, char *op2, int opcode)
{
	/*returns the number of code words that the command takes*/
	int result = checkCommandAddressForm(op1,op2,opcode);
	if (result == BADADDRESSFORM || result == NULLOPERANDS)
		return 1; /*make room for 1 line: error line*/
	return result;
}

int firstPass(char *tag, int instructionType, int opcode, char *op1, char *op2)
{
	int length=0;
	int result = SUCCESS;
	switch (instructionType)
	{
		case DATA:
					if (DEBUGMODE)
						printf("data instruction.\n");
					
					char *temp = NULL;
					char op[MAXLINELENGTH];
					result = addSymbol(tag,DATA,DC);					
					if (result == SUCCESS)
					{
						/*check data length:*/
						strcpy(op,op1);
						temp = strtok(op,",");
						while(temp != NULL)
						{
							length++;
							temp = strtok(NULL,",");
						}
						DC+=length;
						setLastCommandLength(length);
					}
					else if (result == SYMBOL_EXISTS)
						addDuplicatedSymbol(tag);
					else if (result == FULL_TABLE)
					{
						if (fullTableWarningPrinted == NO)
						{
							fullTableWarningPrinted = YES;
							printWarning(); /*MACRO*/
						}
					}
					return result;					

					break;

		case COMMAND:
					
					if (DEBUGMODE)
						printf("command.\n");
					if (tag!=NULL)
						result = addSymbol(tag,COMMAND,IC);
					if (result == SUCCESS)
					{
						//TODO:(AS): need to change this(add relevant bytes to the IC) function acroding our project
						length=getCommandLineLength(op1,op2,opcode);
						IC+=length;
						if (tag!=NULL)
							setLastCommandLength(length);
					}
					else if (result == SYMBOL_EXISTS)
					{
						addDuplicatedSymbol(tag);
						IC++; /*make room for 1 error line*/
					}
					else if (result == FULL_TABLE)
					{
						if (fullTableWarningPrinted == NO)
						{
							fullTableWarningPrinted = YES;
							printWarning(); /*MACRO*/
						}
					}
					return result;
						
					break;
		
		case STRING:
					if (DEBUGMODE)
						printf("string.\n");
					if((((0==strcmp(op1,"EmptyLine")) || (0==strcmp(op1,"RemarkLine"))) &&
						((0==strcmp(op2,"EmptyLine")) || (0==strcmp(op2,"RemarkLine")))))
						return SUCCESS; /*do nothing, no code generated*/
					result = addSymbol(tag,DATA,DC);
					if (result == SUCCESS)
					{
						length = stringLength(op1);
						if (length<1) /*bad string (doesn't start or end with " */
							IC++; /*make room for 1 error line*/
						else
						{
							DC+=length; /*make room for the string*/
							setLastCommandLength(length);
						}
					}
					else if (result == SYMBOL_EXISTS)
					{
						addDuplicatedSymbol(tag);
						DC++; /*make room for 1 error line*/
					}
					else if (result == FULL_TABLE)
					{
						if (fullTableWarningPrinted == NO)
						{
							fullTableWarningPrinted = YES;
							printWarning(); /*MACRO*/
						}
					}
					return SUCCESS;
					
					break;
		
		case EXTERN:
					if (DEBUGMODE)
						printf("external.\n");
					result = addSymbol(op1,EXTERN,IC);
					if (result == SYMBOL_EXISTS)  /*external command doesn't generate code*/
						addDuplicatedSymbol(op1); /*so no need to update IC\DC.*/
					else if (result == FULL_TABLE)
					{
						if (fullTableWarningPrinted == NO)
						{
							fullTableWarningPrinted = YES;
							printWarning(); /*MACRO*/
						}
					}
					/*else if (result == SUCCESS) then do nothing (no code generated)*/
					return result;
					
					break;
					
		case ENTRY:
					/*DO NOTHING. Entry type is needed for the second pass only*/
					if (DEBUGMODE)
							printf("entry.\n");
					return SUCCESS;
					
					break;
					
		case ERROR:
					if (DEBUGMODE)
						printf("error.\n");
					/*make room for one row: print the error*/
					IC++; 
					return SUCCESS;
					
					break;
	}
	return SUCCESS;
}
