#include "mmn14.h"
#define DEBUGMODE 0

const char* commands[] =	{"mov","cmp", "add", "sub", "ror",
							 "shr", "lea","inc", "dec", "jmp",
							 "bne", "red", "prn", "jsr","rts", "hlt"};
							 

static int rowNum;		/*like IC*/
static int dataRowNum;		/*like DC*/

void setDataRowNum(int val)
{
	dataRowNum=val;
}

void setRowNum(int val)
{
	rowNum = val;
}

void renewDataRowNum(int icVal)
{
	dataRowNum+=icVal; /*updates dataRowNum to match the correct row number*/
}

void removeSpaces(char (*res)[], char* stringIn)
{
	char *start;
	while (isspace(*stringIn)) /*delete spaces from the start*/
		stringIn++;
	start = stringIn;
	while (*stringIn!='\0')
		stringIn++;
	stringIn--;
	while (isspace(*stringIn)) /*delete spaces from the end*/
		stringIn--;
	*(stringIn+1)='\0';
	strcpy(*res,start);
}

void getOpWord(char (*dest)[],char op[], int opAddressForm, FILE *ext)
{
	char word[MAXLINELENGTH];
	char buffer[MAXLINELENGTH];
	int num;
	char *helper;
	symbolRow *symbRow;
	if (opAddressForm == IMMEDIATE)
	{
		sscanf(op,"#%d",&num);
		num&=0xffff; /*make num correspond to a 16-bit machine. (if num is negative - truncate to 16 bit)*/
		sprintf(word,"%o\t\t%.6o\t\t%c\n",rowNum,num,'a'); /*a = absolute value*/
	}
	else if (opAddressForm == RELATIONAL)
	{
		helper = &(op[1]); /*op[0] = '*' */
		strcpy(buffer,helper);
		symbRow = getRow(buffer); /*get the row with the required tag*/
		
		if(DEBUGMODE && (symbRow!=NULL))
			printf("symbol: tag %s address %d\n",symbRow->tag,symbRow->address);
		
		if (symbRow == NULL) /*no such row*/
			sprintf(word,"%o\t\tError: Tag \"%s\" is not defined!\n",rowNum,buffer);
		else if (hasADuplicate(buffer))
			sprintf(word,"%o\t\tError: Tag \"%s\" is defined more than once in the source file!\n",rowNum,buffer);
		else if (symbRow->isExtern == YES)
			sprintf(word,"%o\t\tError: Tag \"%s\" cannot be external in relational addressing!\n",rowNum,buffer);
		else
			sprintf(word,"%o\t\t%.6o\t\t%c\n",rowNum,(symbRow->address)-rowNum,'a');
	}
	else /* opAddressForm == DIRECT*/
	{
		symbRow = getRow(op);
		
		if(DEBUGMODE && (symbRow != NULL))
			printf("symbol: tag %s address %d\n",symbRow->tag,symbRow->address);
		
		if (symbRow == NULL)
			sprintf(word,"%o\t\tError: Tag \"%s\" not defined!\n",rowNum,op);
		else if (hasADuplicate(op))
			sprintf(word,"%o\t\tError: Tag \"%s\" is defined more than once in the source file!\n",rowNum,op);
		else if (symbRow->isExtern == YES)
		{
			sprintf(word,"%o\t\t%.6o\t\t%c\n",rowNum,symbRow->address,'e'); /*e for external*/
			fprintf(ext,"%s\t\t%o\n",symbRow->tag,rowNum);
		}
		else
			sprintf(word,"%o\t\t%.6o\t\t%c\n",rowNum,symbRow->address,'r');
	}
	rowNum++;
	
	if(DEBUGMODE)
		printf("opWord: op = %s\n\tword = %s\n",op,word);
	
	strcpy(*dest,word);
}

void getCommand(char (*dest)[],char *op1, char *op2, int opcode, FILE *ext)
{
	char command[MAXLINELENGTH*3]; /*max 3 output rows (output row is smaller than an input line)*/
	char buffer[MAXLINELENGTH];
	int op1AddressForm = BADADDRESSFORM, op2AddressForm = BADADDRESSFORM;
	char op1word[MAXLINELENGTH], op2word[MAXLINELENGTH];
	
	/*Generate command machine code:*/
	
	sprintf(command,"%o\t\t",rowNum);
	sprintf(buffer,"%.2o",opcode);
	strcat(command,buffer);
	
	switch(opcode)
	{	/*op2 may come in op1*/
		case DEC:
		case INC:
		case JMP:
		case BNE:
		case PRN:
		case RED:
		case JSR:
					op2=op1;
					op1 = NULL;
					break;
	}
	
	if (op1!=NULL && op1[0]!='\0')
		op1AddressForm = getAddressForm(op1);
	if (op2!=NULL && op2[0]!='\0')
		op2AddressForm = getAddressForm(op2);

	if (op1!=NULL && op1[0]!='\0')
	{
		sprintf(buffer,"%o",op1AddressForm);
		strcat(command,buffer);
		if(op1AddressForm == DIRECTREG)
		{
			sprintf(buffer,"%o",op1[1]-'0'); /*op[1] is the register number (char), get it as int by substracting '0'*/
			strcat(command,buffer);
		}
		else
			strcat(command,"0"); /*register bits are not used in address form 0,1,3*/
	}
	else
		strcat(command,"00"); /*operand is not used*/
	
	/*do the same for op2*/

	if (op2!=NULL && op2[0]!='\0')
	{
		sprintf(buffer,"%o",op2AddressForm);
		strcat(command,buffer);
		if(op2AddressForm == DIRECTREG)
		{
			sprintf(buffer,"%o",op2[1]-'0'); 
			strcat(command,buffer);
		}
		else
			strcat(command,"0");
	}
	else
		strcat(command,"00");

	strcat(command,"\t\ta\n"); /*command is always absolute*/
	rowNum++;
	
	if(DEBUGMODE)
		printf("first command row: \n%s",command);
	
	strcpy(op1word,"");
	strcpy(op2word,"");

	if (op1AddressForm != BADADDRESSFORM && op1AddressForm != DIRECTREG)
		getOpWord(&op1word,op1,op1AddressForm,ext);

	if (op2AddressForm != BADADDRESSFORM && op2AddressForm != DIRECTREG)
		getOpWord(&op2word,op2,op2AddressForm,ext);

	strcat(command,op1word);
	strcat(command,op2word);
	
	if(DEBUGMODE)
		printf("command: \n%s\n",command);
		
	strcpy(*dest,command);
}

int commandAddressFormIsLegal(char *op1, char *op2, int opcode)
{	
	/*This method is based on checkCommandAddressForm in firstpass.c*/

	/*this checks if the command operands are legal according to the command.*/
	/*by table in page 31 (specs.); if everything is legal: returns SUCCESS =0), see error_e*/
	/*if there is an error - returns BADADDRESSFORM (=5), see enum addressForm_e.*/
	
	int op1address, op2address;
	switch (opcode)
	{
		case MOV:
		case ADD:
		case SUB:
		case ROR:
		case SHR:
					if (op1==NULL || op2==NULL || *op1=='\0' || *op2=='\0')
						return NULLOPERANDS;
					op2address = getAddressForm(op2);
					
					if (op2address == IMMEDIATE)
						return BADADDRESSFORM;
						
					break;

		case CMP:
					if (op1==NULL || op2==NULL || *op1=='\0' || *op2=='\0')
						return NULLOPERANDS;
					break;

		case LEA:
					if (op1==NULL || op2==NULL || *op1=='\0' || *op2=='\0')
						return NULLOPERANDS;
					op1address = getAddressForm(op1);
					op2address = getAddressForm(op2);
					if (op1address != DIRECT || op2address == IMMEDIATE)
						return BADADDRESSFORM;
					
					break;

		case INC:
		case DEC:
		case RED:
					/*op1 should be NULL (see table on specs. page 31)*/
					if (DEBUGMODE && (op2 != NULL || *op2 != '\0'))
					{
						printf("ERROR!!\n*********************\n");
						printf("checkCommandAddressForm (INC\\DEC\\RED) reports op1 is not NULL:\n");
						printf("op1 = %s\t op2 = %s\n",op1,op2);
					}
					if (op1==NULL || *op1=='\0')
						return NULLOPERANDS;
					op1address = getAddressForm(op1);
					
					if (op1address == IMMEDIATE)
						return BADADDRESSFORM;
						
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
						
					break;

		case RTS:
		case HLT:
					if (DEBUGMODE && (op1 != NULL || *op1 != '\0') && (op2 != NULL || *op2 != '\0'))
					{
						printf("ERROR!!\n*********************\n");
						printf("checkCommandAddressForm (RTS\\HLT) reports op1 or op2 is not NULL!\n");
						printf("op1 = %s\t op2 = %s\n",op1,op2);
					}
					
					break;
	}
	return SUCCESS;
}

int secondPass(char *tag, int instructionType, int opcode, char *op1, char *op2, FILE *fp, FILE *ext)
{
	/*fp = file pointer (to .obj or to .ent). ext = pointer to .ext file. The rest are the same-*/
	/*as in firspass*/
	
	symbolRow *row = NULL;
	int result = SUCCESS;
	char command[MAXLINELENGTH*3];
	char *temp = NULL;
	char op[MAXLINELENGTH], buffer[MAXLINELENGTH+100]; /*buffer large enough to hold error messages*/
	char tempBuf[MAXLINELENGTH];
	int num, i, length;
	
	switch (instructionType)
	{
		case DATA:			
					if (DEBUGMODE)
						printf("data instruction secondpass.\n");
					
					if(tag!=NULL)
					{
						row = getRow(tag);
						if (row == NULL)
						{	/*tag should be always found unless the symbol table is full*/
							fprintf(fp,"%o\t\tError: Tag \"%s\" not found!\n",rowNum, tag);
							rowNum++;
							return SUCCESS;
						}
						if (hasADuplicate(tag))
						{
							sprintf(buffer,"%o\t\tError: Tag \"%s\" is defined more than once in the source file!\n",dataRowNum,tag);
							addData(buffer);
							dataRowNum++;
							return SUCCESS;
						}
					}
					
					/*data is ok so far. break it into elements:*/
					strcpy(op,op1);
					temp = strtok(op,",");
					while(temp != NULL)
					{
						removeSpaces(&buffer,temp);
						if(buffer[0]=='\0')
						{
							sprintf(buffer,"%o\t\tError: Spaces alone are not allowed between commas!\n",dataRowNum);
							addData(buffer);
						}
						else
						{
							if(1==sscanf(buffer,"%d%s",&num,tempBuf)) /*buffer contains a number only*/
							{
								num &= 0xffff; /*truncate num to a 16 bit number*/
								sprintf(buffer,"%o\t\t%.6o\n",dataRowNum,num);
								addData(buffer);
							}
							else
							{
								sprintf(buffer,"%o\t\tError: Only numbers are allowed between commas!\n",dataRowNum);
								addData(buffer);
							}
						}
						dataRowNum++;
						temp = strtok(NULL,",");
					}
					
					return SUCCESS;					

					break;

		case COMMAND:
					if (DEBUGMODE)
						printf("command.\n");
					if (tag!=NULL)
					{
						if (hasADuplicate(tag))
						{
							fprintf(fp,"%o\t\tError: Tag \"%s\" is defined more than once in the source file!\n",rowNum,tag);
							rowNum++;
							return SUCCESS;
						}
					}
					if(DEBUGMODE)
						printf("tag is not duplicated\n");
					result = commandAddressFormIsLegal(op1,op2,opcode);
					if (result == BADADDRESSFORM)
					{
						fprintf(fp,"%o\t\tError: Illegal addressing form in command \"%s %s,%s\"\n",
								rowNum,commands[opcode],op1,op2);
						rowNum++;
						return SUCCESS;
					}
					else if (result == NULLOPERANDS)
					{
						fprintf(fp,"%o\t\tError: Wrong number of parameters in the command!\n",rowNum);
						rowNum++;
						return SUCCESS;
					}
					if(DEBUGMODE)
						printf("address is ok\n");
					
					/*else: result == SUCCESS (address form is legal)*/
					getCommand(&command,op1,op2,opcode,ext);
					fputs(command,fp);
					return SUCCESS;
						
					break;
		
		case STRING:
					if (DEBUGMODE)
						printf("string.\n");
					if (((0==strcmp(op1,"EmptyLine")) || (0==strcmp(op1,"RemarkLine"))) &&
						((0==strcmp(op2,"EmptyLine")) || (0==strcmp(op2,"RemarkLine"))))
						return SUCCESS; /*do nothing, no code generated*/
					if(tag!=NULL)
					{	/*tag should be always found unless the symbol table is full*/
						row = getRow(tag);
						if (row == NULL)
						{
							fprintf(fp,"%o\t\tError: Tag \"%s\" not found!\n",rowNum, tag);
							rowNum++;
							return SUCCESS;
						}
						if (hasADuplicate(tag))
						{
							fprintf(fp,"%o\t\tError: Tag \"%s\" is defined more than once in the source file!\n",rowNum,tag);
							rowNum++;
							return SUCCESS;
						}
					}

					length = stringLength(op1);
					if (length<1) /*bad string (doesn't start or end with a " )*/
					{
						fprintf(fp,"%o\t\tError: String %s doesn't start or end with a \" !\n",rowNum,op1);
						rowNum++;
						return SUCCESS;
					}
					else
					{
						if (DEBUGMODE)
							printf("string length: %d\n",length);

						op1++; /*skip the first " */
						for (i=0;i<length-1;i++)
						{
							sprintf(buffer,"%o\t\t%.6o\n",dataRowNum,*(op1+i));
							dataRowNum++;
							addData(buffer);
						}
						/*add the last \0 null value to the string:*/
						sprintf(buffer,"%o\t\t%.6o\n",dataRowNum,0);
						dataRowNum++;
						addData(buffer);
					}

					return SUCCESS;
					
					break;
		
		case EXTERN:
					/*do nothing. No need to generate anything...*/
					return SUCCESS;
					
					break;
					
		case ENTRY:
					row = getRow(op1);
					if (row == NULL) /*entry with no such tag*/
					{
						fprintf(fp,"Error: Tag \" %s \" is not defined in the source file!\n",op1);
						return SUCCESS;
					}
					
					/*assuming address is a non-negative number less than or equal to 2^16-1*/
					if (hasADuplicate(row->tag))
						fprintf(fp,"Error: %s is defined in more than once in the source file!\n",row->tag);
					else
						fprintf(fp,"%s\t\t%o\n",row->tag,row->address);
					return SUCCESS;
					
					break;
					
		case ERROR:
					fprintf(fp,"%o\t\tError: %s\n",rowNum,op1);
					rowNum++;
					return SUCCESS;
					
					break;
	}
	return SUCCESS;
}
