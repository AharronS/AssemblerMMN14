
#include "Base32Operations.h"
#define INT_SIZE_IN_BYTES (sizeof(int) * 8)

void append(char* s, char c)
{
	int len = strlen(s);
	s[len] = c;
	s[len + 1] = '\0';
}

//(AS): this function take number in decimal and return the number in base 32
unsigned int FromSingleNumberToBase32(unsigned int num)
{
	if (num >= 0 && num <= 9)
	{
		return (num + 48);
	}
	else if (num <= 31)
	{
		return (num + 55); //base32 offset in ascii table
	}
}

void SubRoutineFromDecimalNumberToBase32(int num, char* outputBase32)
{
	if (num < 32)
	{
		append(outputBase32, FromSingleNumberToBase32(num));
		return;
	}
	else
	{
		SubRoutineFromDecimalNumberToBase32(num / 32, outputBase32);
		append(outputBase32, FromSingleNumberToBase32(num % 32));
		return;
	}
}

//TODO:(AS): remove all this instances by the MachineCodeRowToBase32 function
void FromDecimalNumberToBase32(int num, char* outputBase32)
{
	*outputBase32 = 0;
	SubRoutineFromDecimalNumberToBase32(num, outputBase32);
}

char *DecimalNumberToBase32(int num)
{
	int length;
	char tempBuff[4] = { '\0' }, outputBase32[MAX_SIZE_MEMORY_WORDS] = "";
	SubRoutineFromDecimalNumberToBase32(num, outputBase32);
	//(AS): if we want to add padding to the base 32 number, uncomment these lines
	//length = strlen(outputBase32);
	//if (length < 3)
	//{
	//	memset(tempBuff, '0', 3);
	//	strcpy(tempBuff + (3 - length), outputBase32);
	//	strcpy(outputBase32, tempBuff);
	//}
	return outputBase32;
}

void ConvertMachineCodeRowToBase32(char *MachineCommand, char *base32OutputCommand)
{
	char base32SingleNumber;
	int i;
	for (i = 0; i < (MACHINE_CODE_ROW_LENGTH / BASE32_SINGLE_NUMBER_LENGTH); i++)
	{
		int decNumber = FromBinaryToDecimal(MachineCommand + (i * BASE32_SINGLE_NUMBER_LENGTH));
		base32SingleNumber = FromSingleNumberToBase32(decNumber);
		base32OutputCommand[i] = base32SingleNumber;
	}

	base32OutputCommand[BASE_32_ROW_COMMAND - 1] = '\0';
}


void FromMachineCodeWordToBase32Word(MachineCodeWord* word, char *base32OutputCommand)
{
	ConvertMachineCodeRowToBase32(word, base32OutputCommand);
}

void SetMachineCodeWord(MachineCodeWord* word, WordComponent component, char *val)
{
	switch (component)
	{
	case C_RND:
		strncpy(word->line + 1, val, 2);
		break;
	case C_GROUP:
		strncpy(word->line + 3, val, 2);
		break;
	case C_OPCODE:
		strncpy(word->line + 5, val, 4);
		break;
	case C_ADDRESS_SRC:
		strncpy(word->line + 9, val, 2);
		break;
	case C_ADDRESS_DST:
		strncpy(word->line + 11, val, 2);
		break;
	case C_CODING:
		strncpy(word->line + 13, val, 2);
		break;
	case C_FILL_WITH_OP:
		strncpy(word->line, val, 13);
		break;
	case C_FILL_REG_SRC:
		strncpy(word->line + 1, val, 6);
		break;
	case C_FILL_REG_DEST:
		strncpy(word->line + 7, val, 6);
		break;
	}
}

void InitMachineCodeWord(commandMachineCodeWord* commandWords)
{
	commandWords->linesCount = 0;
	memset(commandWords->lines[0].line, '0', MACHINE_CODE_ROW_LENGTH);
}

void AddAnotherMachineCodeWord(commandMachineCodeWord* commandWords)
{
	commandWords->linesCount++;
	memset(commandWords->lines[commandWords->linesCount].line, '0', MACHINE_CODE_ROW_LENGTH);
}

void FillAnotherMachineCodeWord(commandMachineCodeWord* commandWords, char *op, int group,
	 FILE *ext, FILE* errorFile, int rowNum)
{
	symbolRow *symbRow;
	int addressForm = getAddressForm(op);
	char buffer[MAXLINELENGTH];
	int num, randomNumber, line = (*commandWords).linesCount;

	switch (addressForm)
	{
	case IMMEDIATE:
		sscanf(op, "#%d", &num);
		strcpy(buffer, FromDecimalToBinary(num, 13));
		SetMachineCodeWord(&((*commandWords).lines[line]), C_FILL_WITH_OP, buffer);
		break;
	case DIRECT:
		symbRow = getRow(op);
		if (symbRow == NULL)
		{
			fprintf(errorFile, "%d\t\tError: Tag \"%s\" not defined!\n", rowNum, op);
		}
		else if (hasADuplicate(op))
		{
			fprintf(errorFile, "%d\t\tError: Tag \"%s\" is defined more than once in the source file!\n", rowNum, op);
		}
		else if (symbRow->isExtern == YES)
		{
			//(AS): the address wil be zero, update only the external CODING
			strcpy(buffer, FromDecimalToBinary(1, 2));
			SetMachineCodeWord(&((*commandWords).lines[line]), C_CODING, buffer);
			fprintf(ext, "%s\t\t%s\n", symbRow->tag, DecimalNumberToBase32(rowNum));
		}
		else
		{
			//(AS): fill with the address calculated in the first pass, and realocatble coding
			strcpy(buffer, FromDecimalToBinary(2, 2));
			SetMachineCodeWord(&((*commandWords).lines[line]), C_CODING, buffer);
			strcpy(buffer, FromDecimalToBinary(symbRow->address, 13));
			SetMachineCodeWord(&((*commandWords).lines[line]), C_FILL_WITH_OP, buffer);
		}
		break;
	case RANDOM:
		if (CheckRandomType(op) == RANDOM1)
		{
			randomNumber = rand() % 7; //(AS): random number between 1-7
			strcpy(buffer, FromDecimalToBinary(randomNumber, 6));
			SetMachineCodeWord(&((*commandWords).lines[line]), C_FILL_REG_SRC, buffer);
			strcpy(buffer, FromDecimalToBinary(1, 2));
			SetMachineCodeWord(&((*commandWords).lines[0]), C_RND, buffer);
		}

		else if (CheckRandomType(op) == RANDOM2)
		{
			randomNumber = rand() % 8180 + (-4090); //(AS): signed number 2^13 (13 bit) size
			strcpy(buffer, FromDecimalToBinary(num, 13));
			SetMachineCodeWord(&((*commandWords).lines[line]), C_FILL_WITH_OP, buffer);
			strcpy(buffer, FromDecimalToBinary(2, 2));
			SetMachineCodeWord(&((*commandWords).lines[0]), C_RND, buffer);

		}

		else if (CheckRandomType(op) == RANDOM3)
		{
			strcpy(buffer, FromDecimalToBinary(getRandomLabelAddress(), 13));
			SetMachineCodeWord(&((*commandWords).lines[line]), C_FILL_WITH_OP, buffer);
			strcpy(buffer, FromDecimalToBinary(2, 2));
			SetMachineCodeWord(&((*commandWords).lines[line]), C_CODING, buffer);
			strcpy(buffer, FromDecimalToBinary(2, 2));
			SetMachineCodeWord(&((*commandWords).lines[0]), C_RND, buffer);
		}

	case DIRECTREG:
		strncpy(buffer, (*commandWords).lines[line].line + 1, 7);
		if (atoi(buffer) == 0) //(AS): if there isnt data in source reg, fill the src
		{
			strcpy(buffer, FromDecimalToBinary(GetNumberOfRegister(op), 6));
			SetMachineCodeWord(&((*commandWords).lines[line]), C_FILL_REG_SRC, buffer);
		}
		else
		{
			strcpy(buffer, FromDecimalToBinary(GetNumberOfRegister(op), 6));
			SetMachineCodeWord(&((*commandWords).lines[line]), C_FILL_REG_DEST, buffer);
		}
		break;
	}
}

unsigned int GetNumberOfRegister(char *op)
{
	int i;
	char tempOp[4] = "";
	
	strncpy(tempOp, op, 3);
	for (i = 0; i < 4; i++)
	{
		if (!isdigit(tempOp[i]))
		{
			tempOp[i] = ' ';
		}
	}
	
	return atoi(tempOp);
}
//(AS): this function take string(5 characters) and return decimal number
int FromBinaryToDecimal(char *stringBinaryNumber) /* Function to convert binary to decimal.*/
{
	char stringNumber[6];
	char* start = stringNumber;
	int total = 0;

	stringNumber[5] = '\0';
	strncpy(stringNumber, stringBinaryNumber, 5);

	while (*start)
	{
		total *= 2;
		if (*start++ == '1') total += 1;
	}
	return total;
}

char *FromDecimalToBinary(unsigned int num, unsigned int bitNum) /* Function to convert binary to decimal.*/
{
	char *binaryString = DecimalToBinaryString(num);
	char buffer[16] = "";
	char *numInBinaryString = DecimalToBinaryString(num);
	strcpy(buffer, binaryString + (INT_SIZE_IN_BYTES - bitNum));
	return buffer;
}

char *DecimalToBinaryString(unsigned int num)
{
	int c, d, count = 0;
	char *pointer = (char*)malloc(INT_SIZE_IN_BYTES + 1);
	
	for (c = 31; c >= 0; c--)
	{
		d = num >> c;

		if (d & 1)
			*(pointer + count) = 1 + '0';
		else
			*(pointer + count) = 0 + '0';

		count++;
	}
	
	*(pointer + count) = '\0';

	return  pointer;
}

