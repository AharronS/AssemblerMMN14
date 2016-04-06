#include "mmn14.h"


#define RETURNMMESSAGELENGTH 80 		/*size of returned strings*/
#define ILLEGAL -1						/*returned when illegal value is detected*/
#define TRUE 1							/*Together with FALSE, used for the makeshift boolean*/
#define FALSE 0							/*Together with TRUE, used for the makeshift boolean*/
#define DEBUGMODE FALSE 				/*debugging mode. enable for most verbose mode*/
#define DEBUMGMODETIER2 FALSE			/*second most verbose debug mode*/
#define FINALDEBUGMODE FALSE			/*minimal output for debugging*/
#define FILENAMELENGTH 50				/*file name length*/

#define NUMBEROFLEGALINSTR 16	/*number of legal instructions*/


/*
 * Definitions:
 * each file contains lines, grabbed by fileReader() and passed to the parser().
 * each line, is dissected by parser() into a command, and it's parts are sent for checking.
 * each command might contain a tag, checked by TagChecker().
 * each command contains an instruction and optional operands, checked by InstructionChecker() and OperandNumChecker(), respectively.
 * if all the checks are OK, the parser will return an opcode for the instruction.
 */


int main(int argc, char *argv[])
{
	if(DEBUGMODE) printf("%s\n",__FUNCTION__);

	char tag[MAXTAGLENGTH], op1[MAXLINELENGTH], op2[MAXLINELENGTH]; /*return values for the command */
	int instropcode[1];
	int interCompilerCommand; 		/*holds the type of command passed to firstpass()*/
	char str[MAXLINELENGTH]; 		/*buffer for line reading*/
	int ExceededLineLengthFlag;		/*flag indicating if last line exceeded the max length permitted*/
	char fileName[FILENAMELENGTH];
	char fileNameTemp[FILENAMELENGTH];
	FILE *src, *entries, *externals, *object;
	int entryFileNeededFlag, externFileNeededFlag;
	int IC,DC;
	int fileIndex;
	
	
	
	if(argc==1)
	{ /*no command line file variables passed, hence exit*/
		printf("Error: Missing Argument. No input file given.\nUsage: ./AssemblyCompilerXZ5000  [YourFileNames].\n");
		exit(1);
	}

	if(DEBUGMODE) printf("input file1 name: %s\n",argv[1]);

	for (fileIndex=1;fileIndex<argc;fileIndex++)	/*for each file in argv, do:*/
	{
		entries   = NULL;
		externals = NULL;
		object    = NULL;
		interCompilerCommand   = ERROR;
		ExceededLineLengthFlag = FALSE;
		entryFileNeededFlag    = FALSE;
		externFileNeededFlag   = FALSE;
		setIC(COUNTERSTARTLINE);	/*set instr. counter to be as defined in mmn14.h*/
		setDC(0);			/*set data counter to be 0*/
		setRowNum(COUNTERSTARTLINE);	/*just like above, only for second pass*/
		setDataRowNum(0);		

		strcpy(fileName,argv[fileIndex]);	/*this copy is for convenience only (to avoid argv[] and use fileName)*/
		strcpy(fileNameTemp,fileName);
		strcat(fileNameTemp,".as");
		if((src = fopen(fileNameTemp, "r"))==NULL) {
		   printf("Error: Cannot open file %s.\n",fileNameTemp);
		   exit(1);
		}
		

		strcpy(fileNameTemp,fileName);
		strcat(fileNameTemp,".ob");
		object = fopen(fileNameTemp,"w");
		if(!object){
			printf("Cannot open object file!\n");
			exit(1);
		}
		init(); /*initialize symbol table*/
		
		/*reads the file into a string, calls the parser for each line  */
		while(!feof(src)) {
			if(fgets(str, MAXLINELENGTH-3, src))	/*read strings*/
			{
				if(FINALDEBUGMODE)printf("file reading sends:%s\n",str);

				if(strlen(str)>=(MAXLINELENGTH-4) || ExceededLineLengthFlag==TRUE)
				{

					if (ExceededLineLengthFlag==FALSE) /*invert the flag. used for detecting the rest of the line that caused the overflow*/
					{
						ExceededLineLengthFlag=TRUE;
						interCompilerCommand=STRING;
						strcpy(op1,"Line too long.");
						firstPass(NULL,ERROR,-1,op1,NULL);
						if (FINALDEBUGMODE) printf("Main returned Error: line too long.\n");
					}
					else
					{
						if(str[strlen(str)-1]=='\n') /*discard lines until end of the long line is reached */
							ExceededLineLengthFlag=FALSE;
						if (FINALDEBUGMODE) printf("Main discarded rest of long line.\n");
						/*don't call firstpass; no code generated*/
					}
				}
				else
				{

					interCompilerCommand=parser(str,&tag,&instropcode,&op1,&op2);

					if (FINALDEBUGMODE){
						if(strlen(tag)>0)
							printf("tag:%s: ",tag);
						switch(interCompilerCommand)
						{
							case(DATA):printf("DATA %s",op1); break;
							case(STRING):printf("STRING %s",op1); break;
							case(EXTERN):printf("EXTERN %s",op1); break;
							case(ENTRY):printf("ENTRY %s",op1); break;
							case(COMMAND):printf("COMMAND %s",op1); break;
							case(ERROR):printf("ERROR: %s",op1); break;
						}
						printf("\n");
					}
					
					if (FINALDEBUGMODE)
						printf("Calling firstpass with: tag %s, op1 %s, op2 %s\n",tag,op1,op2);
						
					/*for interCompilerCommand values look in the switch() block 5 rows above*/
					firstPass(tag,interCompilerCommand,instropcode[0],op1,op2);
					if (interCompilerCommand == EXTERN)
						externFileNeededFlag = TRUE;
					if (interCompilerCommand == ENTRY)
						entryFileNeededFlag = TRUE;
					
				}

				instropcode[0]=ILLEGAL; /*clean up for next round */
				if(FINALDEBUGMODE)printf("-----------------------\n");


			}
		} /*end while, end of first pass*/
		
		rewind(src);
		
		if (entryFileNeededFlag == TRUE){
			strcpy(fileNameTemp,fileName);
			strcat(fileNameTemp,".ent");
			entries = fopen(fileNameTemp,"w");
			if(!entries){
				printf("Cannot open entries file!\n"); /*after all this hard work?? why can't you open??*/
				exit(1);
			}
		}
		if (externFileNeededFlag == TRUE){
			strcpy(fileNameTemp,fileName);
			strcat(fileNameTemp,".ext");
			externals = fopen(fileNameTemp,"w");
			if(!externals){
				printf("Cannot open externals file!\n"); /*after all this hard work?? why can't you open??*/
				exit(1);
			}
		}

		IC = getIC();
		DC = getDC();
		
		update(IC,DC,object);	/*update symbol table after first pass. also start printing (see update() in firstpass.c)*/
		renewDataRowNum(IC);	/*update dataRowNum to match the correct row number*/
		if (DC>0)
			newDataSegment();
		
		/*read again, now for the second pass:*/
		
		while(!feof(src)) {
			if(fgets(str, MAXLINELENGTH-3, src))	/*read strings*/
			{
				if(strlen(str)>=(MAXLINELENGTH-4) || ExceededLineLengthFlag==TRUE)
				{
					if (ExceededLineLengthFlag==FALSE) /*invert the flag. used for detecting the rest of the line that caused the overflow*/
					{
						ExceededLineLengthFlag=TRUE;
						interCompilerCommand=STRING;
						strcpy(op1,"Line too long.");
						secondPass(NULL,ERROR,-1,op1,NULL,object,externals);
						if (FINALDEBUGMODE) printf("Main returned Error: line too long.\n");
					}
					else
					{
						if(str[strlen(str)-1]=='\n') /*discard lines until end of the long line is reached */
							ExceededLineLengthFlag=FALSE;
						/*don't call secondpass: firstpass wasn't called either in this case*/
					}
				}
				else
				{

					interCompilerCommand=parser(str,&tag,&instropcode,&op1,&op2);
					
					if (interCompilerCommand == ENTRY)
						secondPass(tag,ENTRY,-1,op1,op2,entries,externals);
					else
						secondPass(tag,interCompilerCommand,instropcode[0],op1,op2,object,externals);
				}

				instropcode[0]=ILLEGAL; /*clean up for next round */
				if(FINALDEBUGMODE)printf("-----------------------\n");


			}
		} /*end while, end of second pass*/
		
		
		/*cleanup:*/
		if(DC>0)
		{
			printData(object);		/*printf data to file*/
			deleteDataSegment();
		}

		if(object!=NULL)
			fclose(object);
		if(entries!=NULL)
			fclose(entries);
		if(externals!=NULL)
			fclose(externals);
		
		deleteTable();	/*clear symbol table*/
		
		fclose(src);
	}	/*for loop ended; no more files*/
	return(TRUE);

}


/*breaks the command into it's components. checks if they are legal.
  legal commands are one of the following:
 * a. empty,b.  remark(prefixed with ';'discarded),c. legal instruction
 * or d. compiler instruction(.data,.string,.entry, .extern)
 * compiler or regular instructions might be prefixed with a tag
 * Algorithm:
 * break each line into it's components ([tag:] instruction [operand1][,operand2])
 * check if opcode and tag are legal using TagChecker() and InstructionChecker(), respectively
 * returns opcode if both checks are OK,
 *  otherwise,if illegalities were found, returns -1;
 */
int parser(char line[],char (*rettag)[],int (*retInstrcode)[],char (*retop1)[],char (*retop2)[])
{
	if(DEBUGMODE) printf("%s\n",__FUNCTION__);
	if(DEBUGMODE) printf("Args: %s\n",line);

	const int MAXPARTSOFCOMMAND=4; 		/*maximum parts a legal command can have is 4 (tag: instr op1,op2)*/
	char lineCopy[MAXLINELENGTH];			/* a copy of the line, used for manipulation of the line without loosing data*/
	char delims[] = "\n: ,";				/*chars used as delimiters for seperating the command to tag, instruction and operands (tag: instr op1,op2)*/
	char *tokeniser;						/*holds the seperated command parts after each iteration of tokenization*/
	int partsCounter=0; 					/*counts how many parts each command has. used for matching the right part to the*/
	char tag[MAXTAGLENGTH+1]=""; 			/*spec p.27		+1 for null delimiter*/
	char instr[MAXLINELENGTH+1]=""; 		/*spec p.26 	+1 for null delimiter*/
	char op1[MAXLINELENGTH+1]=""; 			/*spec p.26		+1 for null delimiter*/
	char op2[MAXLINELENGTH+1]=""; 			/*spec p.26		+1 for null delimiter*/
	char *compilerInstrvars;
	char strTagRetValue[RETURNMMESSAGELENGTH];
	char strInstrCheckRetValue[RETURNMMESSAGELENGTH];
	int legalTagFound=FALSE;				/*flags if the line includes a tag*/

	int InstrCheckedOK=ILLEGAL; /*default value - assuming illegal.*/
	int tagChecked = FALSE;
	
	strcpy(lineCopy,line);
	removeSpaces(&lineCopy, line);
	strcpy(line,lineCopy);

	if(FINALDEBUGMODE) printf("parser gets:%s\n",line);

	if (strlen(line)<=1) /*empty line is legal*/
	{
			if(DEBUGMODE) printf("%s reports: got an empty line.\n",__FUNCTION__);
	    	strcpy(*retop1,"EmptyLine");
			strcpy(*retop2,"EmptyLine");
			return(STRING);/*return(emptyline);*/
	}
	if (line[0]==';') /*remark line, ignore everything in this line*/
	{
		if(DEBUGMODE) printf("%s reports: got remark line",__FUNCTION__);
    	strcpy(*retop1,"RemarkLine");
		strcpy(*retop2,"RemarkLine");
		return(STRING); /*return(REMARKLINE);*/
	}


	strcpy(lineCopy,line); /*make a copy for tokenization. looking for empty tag.*/
	tokeniser=strtok(lineCopy," ");

	if(strlen(tokeniser)>0)
	{
		if (tokeniser[0]==':') /*tag can't be empty*/
		{
			if(DEBUGMODE)printf("%s reports: Error: empty tag\n",__FUNCTION__);
			strcpy(*retop1,"Empty Tag.");
			return(ERROR);
		}
	}
	if(strlen(tokeniser)<=1)
	{
		if(DEBUGMODE) printf("%s reports: got line with nothing but spaces\n",__FUNCTION__);
		strcpy(*retop1,"EmptyLine");
		strcpy(*retop2,"EmptyLine");
		return(STRING);
	}


	if (strlen(line)<3) /*not empty, yet too short for legal command. shortest commands are hlt or rts without tag.*/
	{
			if(DEBUGMODE) printf("%s reports: Error: too short for legal command\n",__FUNCTION__);
	    	strcpy(*retop1,"Illegal command.");
			return(ERROR);

	}

	strcpy(lineCopy,line);

	tokeniser = strtok(line, delims );
		    while( tokeniser != NULL && partsCounter<=MAXPARTSOFCOMMAND+1)/*MAXPARTSOFCOMMAND+1: check if not exceeding amount of legal parts*/
		    {
		        if(DEBUGMODE)printf( "result %d of tokenization is \"%s\" token length=%d\n",partsCounter, tokeniser,strlen(tokeniser));

		        switch(partsCounter)
		        {
					case(0):{
								if(strlen(tokeniser)<=MAXTAGLENGTH)
							{
								strcpy(tag,tokeniser);
							}
								else
								{
									if(DEBUMGMODETIER2) printf("%s reports: Error: tag too long.\n",__FUNCTION__);
									strcpy(*retop1,"Tag too long.");
									return(ERROR);
								}
							break;
					}


					case(1):{
						if(strlen(tokeniser)<=MAXLINELENGTH)
					{
							strcpy(instr,tokeniser);
					}
						else
						{
							strcpy(*retop1,"Instruction too long.");
							return(ERROR);
						}
					break;
					}
					case(2):{
								if(strlen(tokeniser)<=MAXLINELENGTH)
							{
									strcpy(op1,tokeniser);
							}
								else
								{
									strcpy(*retop1,"Error: Operand1 too long.");
									return(ERROR);
								}
							break;
					}
					case(3):
						{
							if(strlen(tokeniser)<=MAXLINELENGTH)
					{
							strcpy(op2,tokeniser);
					}
						else
						{
							strcpy(*retop1,"Operand2 too long.");
							return(ERROR);
						}
					break;
					}
		        }
		        tokeniser = strtok( NULL, delims );
		        partsCounter++;
		    }


		    if(lineCopy[strlen(tag)]==':')
		    {
				TagChecker(tag,&strTagRetValue);
				tagChecked = TRUE;
			}
		    else
		    {
				/*if the command contains no tag we might still have a legal hidden in the tag. lets check!*/
				/*the command might be skewed so it's fixed by shuffling all the parts one step to their 'left'*/
				if(DEBUGMODE)printf("doing the shuffle!\n");
				strcpy(op2,op1);
				strcpy(op1,instr);
				strcpy(instr,tag);
				strcpy(tag,"");
		    	legalTagFound=FALSE;/*and of course, no tag was found..*/
		    }

			
		    if (DEBUGMODE){
		    printf("tag: %s\n",tag);
		    printf("instr: %s\n",instr);
		    printf("op1: %s\n",op1);
		    printf("op2: %s\n",op2);
			printf("partsCounter: %d\n",partsCounter);
		    printf("line %s\n\n",lineCopy);}

		    if(DEBUGMODE)printf("tagchecker returned: %s\n",strTagRetValue);

			if (!strcmp(strTagRetValue,"legal"))
				legalTagFound=TRUE;

			InstrCheckedOK=InstructionChecker(instr,op1,op2,&strInstrCheckRetValue);

			if(tagChecked==FALSE&& (!legalTagFound) && (lineCopy[strlen(instr)]==':'))
			{/*Even the shuffle couldn't save this one! it's an illegal instruction.*/
				strcpy(*retop1,strTagRetValue);
				return(ERROR);
			}
			if(tagChecked==TRUE&& (!legalTagFound) && (lineCopy[strlen(tag)]==':'))
			{/*Even the shuffle couldn't save this one! it's an illegal instruction.*/
				strcpy(*retop1,strTagRetValue);
				return(ERROR);
			}

			if ((InstrCheckedOK<NUMBEROFLEGALINSTR)&&((partsCounter-legalTagFound)>=MAXPARTSOFCOMMAND))
			{
			    	strcpy(*retop1,"Maximum parts of command exceeded");
			    	return(ERROR);
			}
			if(FINALDEBUGMODE)printf("InstructionChecker returned: %d\n",InstrCheckedOK);

			if(InstrCheckedOK==ILLEGAL)/*invalid command*/
			{
				if(DEBUGMODE) printf("%s reports: Invalid command, returning error message %s\n",__FUNCTION__,strInstrCheckRetValue);
				strcpy(*retop1,strInstrCheckRetValue);
				return(ERROR);
			}
			else
			{

				if(DEBUGMODE) printf("%s reports: Line Legal, returning opcode %d.\n",__FUNCTION__,InstrCheckedOK);


				compilerInstrvars=strtok(lineCopy,":");

				if (InstrCheckedOK>=NUMBEROFLEGALINSTR)/*if it's and compiler instruction*/
				{
					if(!strcmp(strTagRetValue,"legal"))	/*got two scenarios: if we got a tag then it needs to be copied to 'tag'*/
					{									/*and we tokenise to get the rest*/
						compilerInstrvars=strtok(NULL," ");
						compilerInstrvars=strtok(NULL,"\n");
						strcpy(*rettag,tag);
					}
					else	/*otherwise tag should be empty, and the separation is of the parts differs:*/
					{
						compilerInstrvars=strtok(lineCopy," ");
						compilerInstrvars=strtok(NULL,"\n");
						strcpy(*rettag,"");

					}/*the data in compiler instructions goes into operand1*/
					if (compilerInstrvars != NULL)
						strcpy(*retop1,compilerInstrvars);
					else
						strcpy(*retop1,op1);
				}
				else/*if it's not a compiler instruction and it's legal then it's a regular instruction*/
				{	/*thus operand1 and tag get their value from the initial tokenisation*/
					strcpy(*retop1,op1);
					strcpy(*rettag,tag);
				}
				
				/*opcode and operand2 return values*/
				(*retInstrcode)[0]=InstrCheckedOK;

				strcpy(*retop2,op2);


				switch(InstrCheckedOK)
				{

					case 16:	return(DATA);
					case 17:	return(STRING);
					case 18:	return(EXTERN);
					case 19:	return(ENTRY);
					default:	return(COMMAND);
				}
			}


}
/*
 * CompilerInstrChecker checks if the instruction is a compiler instruction.
 * returns 0 (DATA),1 (STRING),2 (EXTERN),3 (ENTRY) for compiler instructions
 * return ILLEGAL if not a compiler instruction.
 */

int CompilerInstrChecker(char* instr)
{
	const char *legalCompilerInstr[] =	{".data",".string",".extern",".entry"}; /*legal compiler instructions*/
	const int NUMBERCOMPILERINSTR=4;/*the number of legal compiler instructions , represented in the array legalCompilerInstr[].*/
	int instrNumber;	/*iterates through instruction arrays in loops*/
	char* tokeniser="";
	if(DEBUGMODE)printf("%s got:%s\n",__FUNCTION__,instr);
	tokeniser=strtok(instr," ");

	for(instrNumber=0; instrNumber<(NUMBERCOMPILERINSTR);instrNumber++)/*loops through the array, comparing the instruction with legal list*/
	{
		if(!strcmp(tokeniser,legalCompilerInstr[instrNumber]))
		{
			if(DEBUGMODE)printf("%s returns:%d\n",__FUNCTION__,instrNumber);
			return (instrNumber);
		}
	}/*if no match was found then it's not a compiler instruction*/
	if(FINALDEBUGMODE)printf("%s reports: not compiler instruction\n",__FUNCTION__);
	return(ILLEGAL);
}

/*checks if given instruction is valid(legal):
 * 1. it should be one of the 15 authorized commands (see spec.pg19-20) or a compiler instruction
 * 1. right number of operands for instruction
 * no operand instructions: rts, hlt
 * one operand instructions: inc, dec, jmp, bne, red, prn, jsr
 * two operand instructions: mov,cmp, add, sub, ror, shr, lea,
 * compiler instructions: ".data",".entry",".extern",".string" (all without the ")
 * returns:
 * 1. opcode for commands
 * 2. 				16:	return(DATA);
					17:	return(STRING);
					18:	return(EXTERN);
					19:	return(ENTRY);
 * 3. ILLEGAL if illegal instruction
 *
 * legal opcodes: 0-15
  */
int InstructionChecker(char* instr,char* op1, char* op2,char (*retstr)[])
{
	if(DEBUGMODE) printf("%s\n",__FUNCTION__);
	if(DEBUGMODE) printf("Args: %s %s %s\n",instr,op1,op2);

	const char *legalInstr[] =	{"mov","cmp", "add", "sub", "ror", "shr", "lea","inc", "dec", "jmp", "bne", "red", "prn", "jsr","rts", "hlt"}; /*the legal instructions*/
	int retCompilerInstrChecker=FALSE;		/*return value for compilerinstructionChecker*/
	int	flagFoundLegalInstruction=0;		/*flag for the stoppingthe loop going through the array of commands if a legal instruction was found.*/
	int opcode=ILLEGAL;						/*legal opcodes are 0-15. init to ILLEGAL*/
	int instrNumber;						/*iterates through instruction arrays in loops*/
	char OperandnumRet_s[MAXLINELENGTH];	/*Error message return value for OperandNumChecker*/

	if(DEBUMGMODETIER2)printf("instruction checker getting arg:%s\n",instr);

	if(instr[0]=='.')/* check if it's a compiler instruction. they start with a distinctive dot */
	{
		retCompilerInstrChecker=CompilerInstrChecker(instr);
		if (retCompilerInstrChecker==ILLEGAL)
		{
			if(DEBUGMODE)printf("%s reports: Error: illegal instruction\n",__FUNCTION__);
			strcpy(*retstr,"Illegal compiler instruction");
			return(ILLEGAL);
		}
		else
		{
			if(DEBUGMODE)printf("%s reports: legal compiler instruction\n",__FUNCTION__);
			strcpy(*retstr,instr);
			return(retCompilerInstrChecker+NUMBEROFLEGALINSTR);
		}
	}
	/*loop through the array of instructions and try to find a match*/
	for(instrNumber=0; instrNumber<(NUMBEROFLEGALINSTR)&&flagFoundLegalInstruction==0;instrNumber++)
	{
		if(DEBUGMODE)printf("comparing %s,%s\n",instr,legalInstr[instrNumber]);
		if(!strcmp(instr,legalInstr[instrNumber]))
		{
			if(DEBUGMODE)printf("found legal instruction @#%d\n",instrNumber);
			flagFoundLegalInstruction=TRUE;
			break;
		}
	}
	
	/*if a match was found the flag would be raised*/
	if (flagFoundLegalInstruction==TRUE)
	{
		if(DEBUMGMODETIER2)printf("%s says: found legal instruction\n",__FUNCTION__);
		/*found legal instruction, now checking that it's operands match*/
		if(instrNumber>=14) /*no operand instr*/
		{
			OperandNumChecker(0,op1,op2,&OperandnumRet_s);
			if(!strcmp(OperandnumRet_s,"legal"))
				opcode=instrNumber;
			else
			{
				strcpy(*retstr,OperandnumRet_s);
				return (ILLEGAL);
			}

		}
		else if(instrNumber>=7&&instrNumber<=13) /*1 operand instr*/
		{
			OperandNumChecker(1,op1,op2,&OperandnumRet_s);
			if(!strcmp(OperandnumRet_s,"legal"))
			{
				opcode=instrNumber;
			}
			else
			{
				strcpy(*retstr,"Wrong number of operands for instruction");
				return (ILLEGAL);
			}
		}
		else if(instrNumber<=6) /*2 operand instr*/
		{
			OperandNumChecker(2,op1,op2,&OperandnumRet_s);
			if(!strcmp(OperandnumRet_s,"legal"))
			{
				opcode=instrNumber;
			}
			else
			{
				strcpy(*retstr,"Wrong number of operands for instruction");
				return (ILLEGAL);
			}
		}
		else
		{/*non-existing instr*/
			if(DEBUGMODE)printf("%s reports: error: non-existing instruction\n",__FUNCTION__);
			strcpy(*retstr,"Illegal non-existing Instruction.");
			return (ILLEGAL);
		}
	}
	else	/*couldn't find a legal instruction*/
	{
		if(FINALDEBUGMODE)printf("%s reports: error: no legal instruction found\n",__FUNCTION__);
		strcpy(*retstr,"Illegal non-existing instruction.");
		return (ILLEGAL);
	}

	if(DEBUGMODE)printf("%s reports: Instruction Legal.\n",__FUNCTION__);
/*If no illegality found - the command is legal, and opcode is returned.*/
	return(opcode);
}


/*Tagchecker returns:
 * error message for illegal tag
 * "legal" for legal tags
* ILLEGAL names for tags: (specs.pg 27)
* 1. tags not including characters other than ':'
* 2. tags whose name is a command.
* 3. a tag that has already been used. (not checked in TagChecker)
* 4. tags exceeding the MAXTAGLENGTH length: 30 chars.
*/
void TagChecker(char* partstr,char (*strret)[])
{
	if(DEBUGMODE){ printf(__FUNCTION__);printf("\n");}
	if(DEBUGMODE) printf("Args: %s\n",partstr);

	const char* commands[] =	{"mov","cmp", "add", "sub", "ror", "shr", "lea","inc", "dec", "jmp", "bne", "red", "prn", "jsr","rts", "hlt"};
	const int NUMBEROFCOMMANDS=16; /*the number of legal command, represented in the array commands[].*/
	const char* registers[] = {"r0","r1","r2","r3","r4","r5","r6","r7"};/*array of the register names*/
	const int NUMBEROFREGISTERS=8; /*number of legal registers, represented in the array registers[]*/
	int flagFoundIllegalTag=FALSE;
	int i=0; 						/*loop counter*/
	if (strlen(partstr)==0) /*Empty tag is legal*/
	{
		strcpy (*strret,"legal");
		return;
	}

		/*tag can't be a command or register. specs.pg 27*/
		/*check if tag is a command*/
		for(i=0;i<NUMBEROFCOMMANDS;i++)
		{
				if(!strcmp(partstr,commands[i]))
				{
					flagFoundIllegalTag=TRUE; /*illegal tag found*/
					break;
				}
		}

		if(flagFoundIllegalTag==TRUE)
		{
			if(DEBUGMODE) printf("%s reports: error: tag can't be a instruction\n",__FUNCTION__);
			strcpy(*strret,"Tag cannot be an instruction.");
			return;
		}
		/*check if tag is a register. this is illegal.*/
		for(i=0;i<NUMBEROFREGISTERS;i++)
		{
				if(!strcmp(partstr,registers[i]))
				{
					flagFoundIllegalTag=TRUE; /*illegal tag found*/
					break;
				}
		}
		if(flagFoundIllegalTag==TRUE)
		{
			if(DEBUGMODE) printf("%s reports: error: tag can't be a register\n",__FUNCTION__);
			strcpy(*strret,"Tag cannot be a register.");/*tag can't be a command or register. specs.pg 27*/
			return;
		}

		if ((partstr[0]>='a' && partstr[0]<='z')|| (partstr[0]>='A' && partstr[0]<='Z')) /*first char of tag must be a letter. specs.pg 27*/
		{

			strcpy(*strret,"legal");
			if(DEBUGMODE) printf("%s reports: Tag is Legal. \n",__FUNCTION__);
			return;
		}
		else
		{
			if(DEBUGMODE) printf("%s reports: error: first char of tag isn't a letter\n", __FUNCTION__);
			flagFoundIllegalTag = TRUE;
			strcpy(*strret,"First character of tag must be a letter.");/*first char isn't a letter, thus error. specs.pg 27*/
			return;
		}
}



/*
 * checks if command has a legal number of operands.
 * numExpectedOperand is the number of expected operands. then compares with the actual number of not-empty operands received.
 * returns an error string ("error: ... ") if the expected operands and the actual operands differ.
 * return a string("legal") if they match.
 */
void OperandNumChecker(int numExpectedOperand,char* op1, char* op2,char (*retstr)[])
{
	if(DEBUGMODE) printf("%s\n",__FUNCTION__);
	if(DEBUGMODE) printf("Args: %d %s %s\n",numExpectedOperand,op1,op2);

	int actualOperands; /*counts actual number of operands*/
	actualOperands=0; /* initialize */

	if(strcmp(op1,"")) actualOperands++;/*if the operand is not empty then count it*/
	else if(numExpectedOperand==1) /*if expecting one operand than this should be it. if it's empty then it's an illegal instruction.*/
	{
		if(DEBUGMODE)printf("%s reports: error: illegal operands: Operand1.\n",__FUNCTION__);
		strcpy(*retstr,"Illegal operands: Operand1.");
		return;
	}


	if(strcmp(op2,"")) actualOperands++;/*if the operand is not empty then count it*/

	if(actualOperands!=numExpectedOperand)
	{
		if(DEBUGMODE)printf("%s reports: error: illegal operands: expected %d operands and got %d\n",__FUNCTION__,numExpectedOperand,actualOperands);
		/*return(ILLEGAL);*/
		strcpy(*retstr,"Illegal number of operands.");
		return;
	}
	if(DEBUGMODE)printf("%s reports: Operands Legal\n",__FUNCTION__);
	strcpy(*retstr,"legal");
}










