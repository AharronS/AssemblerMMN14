#include "mmn14.h"
#include "Base32Operations.h"
#define DEBUGMODE 1

static int rowNum;		/*like IC*/
static int dataRowNum;		/*like DC*/

const char* commands[] =	{"mov","cmp", "add", "sub", "lea",
							 "not", "clr", "inc", "dec", "jmp",
							 "bne", "red", "prn", "jsr", "rts", "stop"};

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

unsigned int GetGroupCode(int opcode)
{
	switch (opcode)
	{
	case MOV:
	case CMP:
	case ADD:
	case SUB:
	case LEA:
		return 2;
		break;
	case NOT:
	case CLR:
	case INC:
	case DEC:
	case JMP:
	case BNE:
	case RED:
	case PRN:
	case JSR:
		return 1;
		break;
	case RTS:
	case STOP:
		return 0;
		break;
	}
	return 0;
}

//(AS): This function removing space from the beginning of the line and the end of the line
void removeSpaces(char(*res)[], char* stringIn, int stringLength)
{
	char lineCopy[MAXLINELENGTH];
	char *end = &stringIn[stringLength], *start = stringIn;

	while (isspace(*start)) /*delete spaces from the start*/
		start++;

	while (isspace(*end)) /*delete spaces from the end*/
		end--;

	strncpy(lineCopy, start, end - start);
	lineCopy[end - start] = '\0';
	strcpy(*res, lineCopy);
}

void WriteToFileCommandMachineWord(commandMachineCodeWord *wordCommand, FILE *fp, int rowNum)
{
	int i, lines = wordCommand->linesCount;
	char buffer[MAXLINELENGTH] = "";

	for (i = 0; i <= lines; i++)
	{
		ConvertMachineCodeRowToBase32(&((*wordCommand).lines[i].line), buffer);
		fprintf(fp, "%s\t\t%s\n", DecimalNumberToBase32(rowNum + i), buffer);
	}
}

commandMachineCodeWord ConvertCommand(char *op1, char *op2, int opcode, FILE *ext, FILE *errorFile)
{
	commandMachineCodeWord newWordcommand;
	char buffer[MAXLINELENGTH];
	int tempNum;
	InitMachineCodeWord(&newWordcommand);

	//(AS): update group
	strcpy(buffer, FromDecimalToBinary(GetGroupCode(opcode), 2));
	SetMachineCodeWord(&(newWordcommand.lines[0]), C_GROUP, buffer);
	//(AS): update opcode, command is always absolute
	strcpy(buffer, FromDecimalToBinary(opcode, 4));
	SetMachineCodeWord(&(newWordcommand.lines[0]), C_OPCODE, buffer);
	switch (GetGroupCode(opcode))
	{
	case 0:
		break;
	case 1:
		strcpy(buffer, FromDecimalToBinary(getAddressForm(op1), 2));
		SetMachineCodeWord(&(newWordcommand.lines[0]), C_ADDRESS_DST, buffer);
		AddAnotherMachineCodeWord(&newWordcommand);
		//(AS): Because we added a line, we have to add to rowNum +1. Note we do not update the value of the rowNum
		FillAnotherMachineCodeWord(&newWordcommand, op1, GetGroupCode(opcode), ext, errorFile, rowNum + 1);
		rowNum++;
		break;
	case 2:
		strcpy(buffer, FromDecimalToBinary(getAddressForm(op1), 2));
		SetMachineCodeWord(&(newWordcommand.lines[0]), C_ADDRESS_SRC, buffer);
		strcpy(buffer, FromDecimalToBinary(getAddressForm(op2), 2));
		SetMachineCodeWord(&(newWordcommand.lines[0]), C_ADDRESS_DST, buffer);
		
		//(AS): Add two machine code words if and only if both operands are not direct registers
		if (!(getAddressForm(op1) == DIRECTREG && getAddressForm(op2) == DIRECTREG)) 
		{
			AddAnotherMachineCodeWord(&newWordcommand);
			//(AS): Because we added a line, we have to add to rowNum +1. Note we do not update the value of the rowNum
			FillAnotherMachineCodeWord(&newWordcommand, op1, GetGroupCode(opcode), ext, errorFile, rowNum + 1);
			AddAnotherMachineCodeWord(&newWordcommand);
			//(AS): Because we added a line, we have to add to rowNum +2. Note we do not update the value of the rowNum
			FillAnotherMachineCodeWord(&newWordcommand, op2, GetGroupCode(opcode), ext, errorFile, rowNum + 2);
			rowNum += 2;
		}
		//(AS): Add one machine code word if and only if both operands are direct registers 
		else
		{
			AddAnotherMachineCodeWord(&newWordcommand);
			//(AS): Because we added a line, we have to add to rowNum +1. Note we do not update the value of the rowNum
			FillAnotherMachineCodeWord(&newWordcommand, op1, GetGroupCode(opcode), ext, errorFile, rowNum + 1);
			FillAnotherMachineCodeWord(&newWordcommand, op2, GetGroupCode(opcode), ext, errorFile, rowNum + 1);
			rowNum ++;
		}
		break;

	}
	
	rowNum++;
	return newWordcommand;
	
}

int commandAddressFormIsLegal(char *op1, char *op2, int opcode)
{
	/*This method is based on checkCommandAddressForm in firstpass.c*/
	/*this checks if the command operands are legal according to the command.*/
	/*by table in page 31 (specs.); if everything is legal: returns SUCCESS =0), see error_e*/
	/*if there is an error - returns BADADDRESSFORM (=5), see enum addressForm_e.*/

	int op1address, op2address, lines = 1;
	switch (opcode)
	{
	case MOV:
	case ADD:
	case SUB:
		//(AS): removed ror, shr, update the default line variable to 3 
		lines = 3;
		if (op1 == NULL || op2 == NULL || *op1 == '\0' || *op2 == '\0')
			return NULLOPERANDS;
		op2address = getAddressForm(op2);

		if (op2address == IMMEDIATE)
			return BADADDRESSFORM;
		break;

	case CMP:
		lines = 3;
		if (op1 == NULL || op2 == NULL || *op1 == '\0' || *op2 == '\0')
			return NULLOPERANDS;
		break;

	case LEA:
		if (op1 == NULL || op2 == NULL || *op1 == '\0' || *op2 == '\0')
			return NULLOPERANDS;
		op1address = getAddressForm(op1);
		op2address = getAddressForm(op2);
		if (!(op1address == DIRECT || op1 == RANDOM3))
		{
			return BADADDRESSFORM;
		}
		break;

	case INC:
	case NOT:
	case CLR:
	case DEC:
	case RED:

		/*op1 should be NULL (see table on specs. page 31)*/
		if (DEBUGMODE && (op2 != NULL || *op2 != '\0'))
		{
			printf("ERROR!!\n*********************\n");
			printf("checkCommandAddressForm reports op1 is not NULL!\n");
			printf("op1 = %s\t op2 = %s\n", op1, op2);
		}
		if (op1 == NULL || *op1 == '\0')
			return NULLOPERANDS;
		op1address = getAddressForm(op1);

		if (op1address != DIRECT && op1address != DIRECT)
			return BADADDRESSFORM;
		break;

	case JMP:
	case BNE:
	case JSR:
		if (DEBUGMODE && (op2 != NULL || *op2 != '\0'))
		{
			printf("ERROR!!\n*********************\n");
			printf("checkCommandAddressForm (JMP\\BNE\\JSR) reports op1 is not NULL!\n");
			printf("op1 = %s\t op2 = %s\n", op1, op2);
		}
		if (op1 == NULL || *op1 == '\0')
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
			printf("op1 = %s\t op2 = %s\n", op1, op2);
		}
		if (op1 == NULL || *op1 == '\0')
			return NULLOPERANDS;

		op1address = getAddressForm(op1);

		if (op1address >= RANDOM1 && op1address <= RANDOM3)
			return BADADDRESSFORM;
		break;

	case RTS:
	case STOP:
		if (DEBUGMODE && (op1 != NULL || *op1 != '\0') && (op2 != NULL || *op2 != '\0'))
		{
			printf("ERROR!!\n*********************\n");
			printf("checkCommandAddressForm (RTS\\HLT) reports op1 or op2 is not NULL!\n");
			printf("op1 = %s\t op2 = %s\n", op1, op2);
		}

		break;
	}
	return SUCCESS;
}

int secondPass(char *tag, int instructionType, int opcode, char *op1, char *op2, FILE *fp, FILE *ext, FILE *errorFile)
{
	/*fp = file pointer (to .obj or to .ent). ext = pointer to .ext file. The rest are the same-*/
	/*as in firspass*/
	commandMachineCodeWord machineCommand;
	symbolRow *row = NULL;
	int result = SUCCESS;
	char command[MAXLINELENGTH * 3];
	char *temp = NULL;
	char op[MAXLINELENGTH], buffer[MAXLINELENGTH + 100]; /*buffer large enough to hold error messages*/
	char tempBuf[MAXLINELENGTH];
	int num, i, length, prevRowNum;

 	switch (instructionType)
	{
	case DATA:
		if (DEBUGMODE)
			printf("data instruction secondpass.\n");

		if (tag != NULL)
		{
			row = getRow(tag);
			if (row == NULL)
			{	/*tag should be always found unless the symbol table is full*/
				fprintf(errorFile, "%d\t\tError: Tag \"%s\" not found!\n", rowNum, tag);
				rowNum++;
				return SUCCESS;
			}
			if (hasADuplicate(tag))
			{
				//TODO:(AS): check this
				//sprintf(buffer, "%o\t\tError: Tag \"%s\" is defined more than once in the source file!\n", dataRowNum, tag);
				fprintf(errorFile, "%d\t\tError: Tag \"%s\" is defined more than once in the source file!\n", dataRowNum, tag);
				//addData(buffer);
				dataRowNum++;
				return SUCCESS;
			}
		}

		/*data is ok so far. break it into elements:*/
		strcpy(op, op1);
 		temp = strtok(op, ",");
		while (temp != NULL)
		{
			removeSpaces(&buffer, temp, strlen(temp));
			if (buffer[0] == '\0')
			{
				fprintf(errorFile, "%s\t\tError: Spaces alone are not allowed between commas!\n", DecimalNumberToBase32(dataRowNum));
			}
			else
			{
				if (1 == sscanf(buffer, "%d%s", &num, tempBuf)) /*buffer contains a number only*/
				{
					num = num & 0xffff; /*truncate num to a 16 bit number*/
					strcpy(buffer, DecimalNumberToBase32(dataRowNum));
					strcat(buffer, "\t\t");
					strcat(buffer, DecimalNumberToBase32(num));
					strcat(buffer, "\n");
					addData(buffer);
				}
				else
				{
					fprintf(errorFile, "%s\t\tError: Only numbers are allowed between commas!\n", DecimalNumberToBase32(dataRowNum));
				}
			}

			dataRowNum++;
			temp = strtok(NULL, ",");
		}

		return SUCCESS;
		break;

	case COMMAND:
		if (DEBUGMODE)
			printf("command.\n");
		if (tag != NULL)
		{
			if (hasADuplicate(tag))
			{
				fprintf(errorFile, "%s\t\tError: Tag \"%s\" is defined more than once in the source file!\n", DecimalNumberToBase32(rowNum), tag);
				rowNum++;
				return SUCCESS;
			}
		}
		
		result = commandAddressFormIsLegal(op1, op2, opcode);
		if (result == BADADDRESSFORM)
		{
			fprintf(errorFile, "%s\t\tError: Illegal addressing form in command \"%s %s,%s\"\n",
				DecimalNumberToBase32(rowNum), commands[opcode], op1, op2);
			rowNum++;
			return SUCCESS;
		}
		
		else if (result == NULLOPERANDS)
		{
			fprintf(errorFile, "%s\t\tError: Wrong number of parameters in the command!\n", DecimalNumberToBase32(rowNum));
			rowNum++;
			return SUCCESS;
		}
		
		/*else: result == SUCCESS (address form is legal)*/
		prevRowNum = rowNum;
		machineCommand = ConvertCommand(op1, op2, opcode, ext, errorFile);
		WriteToFileCommandMachineWord(&machineCommand, fp, prevRowNum);
		return SUCCESS;

		break;

	case STRING:
		if (((0 == strcmp(op1, "EmptyLine")) || (0 == strcmp(op1, "RemarkLine"))) &&
			((0 == strcmp(op2, "EmptyLine")) || (0 == strcmp(op2, "RemarkLine"))))
			return SUCCESS; /*do nothing, no code generated*/
		if (tag != NULL)
		{	/*tag should be always found unless the symbol table is full*/
			row = getRow(tag);
			if (row == NULL)
			{
				fprintf(errorFile, "%d\t\tError: Tag \"%s\" not found!\n", rowNum, tag);
				rowNum++;
				return SUCCESS;
			}
			if (hasADuplicate(tag))
			{
				fprintf(errorFile, "%o\t\tError: Tag \"%s\" is defined more than once in the source file!\n", rowNum, tag);
				rowNum++;
				return SUCCESS;
			}
		}

		length = stringLength(op1);
		if (length<1) /*bad string (doesn't start or end with a " )*/
		{
			//TODO:(AS): change to error file handler
			fprintf(errorFile, "%s\t\tError: String %s doesn't start or end with a \" !\n", DecimalNumberToBase32(rowNum), op1);
			rowNum++;
			return SUCCESS;
		}
		else
		{
			if (DEBUGMODE)
				printf("string length: %d\n", length);

			op1++; /*skip the first " */
			for (i = 0;i<length - 1;i++)
			{
				strcpy(buffer, DecimalNumberToBase32(dataRowNum));
				strcat(buffer, "\t\t");
				strcat(buffer, DecimalNumberToBase32(*(op1 + i)));
				strcat(buffer, "\n");
				dataRowNum++;
				addData(buffer);
			}
			/*add the last \0 null value to the string:*/
			sprintf(buffer, "%s\t\t%s\n", DecimalNumberToBase32(dataRowNum), "000");
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
			fprintf(errorFile, "Error: Tag \" %s \" is not defined in the source file!\n", op1);
			return SUCCESS;
		}

		/*assuming address is a non-negative number less than or equal to 2^16-1*/
		if (hasADuplicate(row->tag))
			fprintf(errorFile, "Error: %s is defined in more than once in the source file!\n", row->tag);
		else
			fprintf(fp, "%s\t\t%s\n", row->tag, DecimalNumberToBase32(row->address));
		return SUCCESS;

		break;

	case ERROR:
		fprintf(errorFile, "%s\t\tError: %s\n", DecimalNumberToBase32(rowNum), op1);
		rowNum++;
		return SUCCESS;

		break;
	}
	return SUCCESS;
}