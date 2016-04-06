#ifndef BASE32OPERATIONS_H
#define BASE32OPERATIONS_H
#include "mmn14.h"
void append(char* s, char c);

//(AS): this function take number in decimal and return the number in base 32
unsigned int FromSingleNumberToBase32(unsigned int num);
void SubRoutineFromDecimalNumberToBase32(int num, char* outputBase32);
void FromDecimalNumberToBase32(int num, char* outputBase32);
char *FromDecimalToBinary(unsigned int num, unsigned int bitNum);
char *DecimalToBinaryString(unsigned int num);
char *DecimalNumberToBase32(int num);
void ConvertMachineCodeRowToBase32(char *MachineCommand, char *base32OutputCommand);
void InitMachineCodeWord(MachineCodeWord* word);
void FromMachineCodeWordToBase32Word(MachineCodeWord* word, char *base32OutputCommand);
unsigned int GetNumberOfRegister(char *op);
void SetMachineCodeWord(MachineCodeWord* word, WordComponent component, char *val);
void FillAnotherMachineCodeWord(commandMachineCodeWord* commandWords, char *op, int group,
	FILE *ext, FILE* errorFile, int rowNum);
void AddAnotherMachineCodeWord(commandMachineCodeWord* commandWords);
//(AS): this function take string(5 characters) and return decimal number
int FromBinaryToDecimal(char *stringBinaryNumber);

#endif