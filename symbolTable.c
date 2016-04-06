#include "mmn14.h"
#include "Base32Operations.h"

#define DEBUGMODE 1
#define EMPTYTAG ""


static symbolTable *symbolPtr;

void printTable() /*FOR DEBUGMODE ONLY*/
{
	int i;
	for (i=0;i<symbolPtr->currentSymbol;i++)
	{
		printf("Symbol: %s\t",symbolPtr->row[i].tag);
		printf("Address: %d\t",symbolPtr->row[i].address); /*DECIMAL (not octal) NUMBER*/ 
		printf("Command length: %d\t",symbolPtr->row[i].commandLength);
		
		if (symbolPtr->row[i].isTemp == YES)
			printf("isTemp: yes\t");
		if (symbolPtr->row[i].isTemp == NO)
			printf("isTemp: no\t");
		
		if (symbolPtr->row[i].isExtern == YES)

			printf("isExternal: yes\t");
		if (symbolPtr->row[i].isExtern == NO)
			printf("isExternal: no\t");
		
		if (symbolPtr->row[i].associatedTo == DATA)
			printf("assoc.: data\t");
		if (symbolPtr->row[i].associatedTo == COMMAND)
			printf("assoc.: command\t");
		if (symbolPtr->row[i].associatedTo == EXTERN)
			printf("assoc.: external\t");
		
		printf("\n");
	}
	printf("Current free symbol: %d\n",symbolPtr->currentSymbol);
}



void deleteTable()
{
	free(symbolPtr);
}

void setLastCommandLength(int len)
{	/*last symbol added: set its legnth to len.*/
	symbolPtr->row[(symbolPtr->currentSymbol)-1].commandLength = len;
}

int hasADuplicate(char *tag)
{
	int i=0;
	while(strcmp(duplicatedTags[i],EMPTYTAG)!=0)
	{
		if (strcmp(duplicatedTags[i],tag)==0)
			return YES; /*tag found. has a duplicate.*/
		i++;
	}
	return NO; /*tag not found here, therefore, it's unique.*/
}

void addDuplicatedSymbol(char *tag)
{
	int i=0;
	while(strcmp(duplicatedTags[i],EMPTYTAG)!=0)
	{
		if (strcmp(duplicatedTags[i],tag)==0)
			return; /*this tag is already duplicated, no need to add it again*/
		i++;
	}
	strcpy(duplicatedTags[i],tag);
}

void init()
 {
	int i;
	symbolRow temp;
	temp.address = temp.isTemp = temp.isExtern = temp.associatedTo = temp.commandLength = 0;
	strcpy(temp.tag,"\0");
	symbolPtr = (symbolTable *)malloc(sizeof(symbolTable));
	if (!symbolPtr)
	{
		printf ("Memory allocation for the symbol table failed!");
		exit(1);
	}
	for (i=0;i<MAXSYMBOLS;i++)
		symbolPtr->row[i] = temp;
    symbolPtr->currentSymbol = 0;
	
	/*initialize duplicatedTags array:*/
	for (i=0;i<MAXSYMBOLS;i++)
		strcpy(duplicatedTags[i],EMPTYTAG);
	
	if (DEBUGMODE)
	{
		printf("Symbol table after calling init():\n");
		printTable();
	}
}

int addSymbol (char symbol[], int lineType, int counterVal) /*called to update symbol table*/
{
	
	int i, currentSymb = symbolPtr->currentSymbol;
	int updateVal;

	if (DEBUGMODE)
	{
		printf("Symbol table before addSymbol():\n");
			printTable();
	}

	if (symbol != NULL && symbol[0]!='\0')
	{
		for (i=0;i<currentSymb;i++)
		{
			if (strcmp (symbolPtr->row[i].tag, symbol) == 0)
			{
				/*update command lengths to match an erorr line*/
				updateVal = symbolPtr->row[i].commandLength - 1;
				symbolPtr->row[i].commandLength=1; /*error line*/
				
				if (symbolPtr->row[i].associatedTo == COMMAND)		/*if the tag is defined already*/
				{													/*in a command row, the address change*/
					for (;i<currentSymb;i++)						/*will affect everything that comes-*/
						if (symbolPtr->row[i].isExtern == NO)		/*after it. i.e data and commands*/
							symbolPtr->row[i].address-=updateVal;					

					updateIC(updateVal);
				}
				else if (symbolPtr->row[i].associatedTo == DATA)	/*if tag is defined in a data row*/
				{													/*the new error line will only*/
					for (;i<currentSymb;i++)						/*affect data*/
						if (symbolPtr->row[i].associatedTo == DATA)
							symbolPtr->row[i].address-=updateVal;
	
					updateDC(updateVal);
				}
				return SYMBOL_EXISTS;
			}
		}
	}
	if (currentSymb == MAXSYMBOLS)
		return FULL_TABLE; /*array index = last index*/
	
	/*add new symbol:*/
	symbolRow new;
	if (symbol!=NULL)
		strcpy(new.tag,symbol);
	else
		strcpy(new.tag,EMPTYTAG);
	
	if (lineType == EXTERN)
	{
		new.address = 0;
		new.isTemp = NO;
		new.isExtern = YES;
		new.associatedTo = EXTERN;
		new.commandLength = 1;
		symbolPtr->row[symbolPtr->currentSymbol] = new;
		(symbolPtr->currentSymbol)++;
	}
	if (lineType == DATA)
	{
		new.address = counterVal; /*DC VALUE*/
		new.isTemp = YES;
		new.isExtern = NO;
		new.associatedTo = DATA;
		new.commandLength = 1;
		symbolPtr->row[symbolPtr->currentSymbol] = new;
		(symbolPtr->currentSymbol)++;
	}
	if (lineType == COMMAND)
	{
		new.address = counterVal; /*IC VALUE*/
		new.isTemp = NO;
		new.isExtern = NO;
		new.associatedTo = COMMAND;
		new.commandLength = 1;
		symbolPtr->row[symbolPtr->currentSymbol] = new;
		(symbolPtr->currentSymbol)++;
	}
	
	if (DEBUGMODE)
	{
		printf("Symbol table after addSymbol():\n");
		printTable();
	}
	return SUCCESS;
}

symbolRow *getRow(char symbol[])	/*returns a pointer to the desired tag field in the symbol table*/
{									/*returns NULL if no such tag.*/
	int i;
	symbolRow *Row = NULL;
	int currentSymb = symbolPtr->currentSymbol;
	for (i=0;i<currentSymb;i++)
	{
		if (strcmp (symbolPtr->row[i].tag, symbol) == 0)
		{
			Row = &(symbolPtr->row[i]);
			break;
		}
	}
	return Row;
}

int getRandomLabelAddress()	
{
	int i, randomNumber, address, count = 0;
	int currentSymb = symbolPtr->currentSymbol;
	for (i = 0; i < currentSymb; i++)
	{
		if (symbolPtr->row[i].isExtern == NO && symbolPtr->row[i].associatedTo == DATA)
		{
			count++;
		}
	}
	
	randomNumber = rand() % count;
	count = 0;

	for (i = 0; i < currentSymb; i++)
	{
		if (symbolPtr->row[i].isExtern == NO && symbolPtr->row[i].associatedTo == DATA)
		{
			count++;
			address = (count == randomNumber) ? symbolPtr->row[i].address : 0;
		}
	}

	return address;
}

int update(int icVal, int dcVal,FILE *obj)	/*when first pass finished, we need to update temporary addresses;*/
{ 											/*temp addresses were the DC values of the labels.*/
	
	int i;
	int currentSymb = symbolPtr->currentSymbol;
	for (i=0;i<currentSymb;i++)
	{
		if ((symbolPtr->row[i].isTemp == YES) && (symbolPtr->row[i].associatedTo == DATA))
		{	/*possible to check only one of the conditions above, because after first pass*/
			/*only data has a temporary address. But this is a double check...*/
			
			symbolPtr->row[i].isTemp = NO;
			(symbolPtr->row[i].address)+=icVal;
		}
	}
	//TODO:(AS): here we need to change the header in file.
	fputs("Base 32 Address\t\tBase 32 machine code\n\n",obj);
	fprintf(obj,"\t\t%s   %s\n",DecimalNumberToBase32(icVal),DecimalNumberToBase32(dcVal));
	return SUCCESS;
}











