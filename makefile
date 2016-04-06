mmn14: firstPass.o secondPass.o symbolTable.o parser.o dataSegment.o
	gcc	-g	-ansi	-Wall	firstPass.o	secondPass.o	symbolTable.o	parser.o	dataSegment.o	-o	AssemblyCompilerXZ5000

firstPass.o: firstPass.c
	gcc 	-g	-c	-ansi	-Wall	firstPass.c	-o	firstPass.o

secondPass.o: secondPass.c
	gcc 	-g	-c	-ansi	-Wall	secondPass.c	-o	secondPass.o

symbolTable.o: symbolTable.c
	gcc 	-g	-c	-ansi	-Wall	symbolTable.c	-o	symbolTable.o

parser.o: parser.c
	gcc	-g	-c	-ansi	-Wall	parser.c	-o	parser.o

dataSegment.o: dataSegment.c
	gcc	-g	-c	-ansi	-Wall	dataSegment.c	-o	dataSegment.o


