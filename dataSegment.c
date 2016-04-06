#include "mmn14.h"
#define MAXLINELENGTH 80

typedef struct dataSegment * dataPtr;

typedef struct dataSegment
{
	char data[MAXLINELENGTH];
	dataPtr next;
} dataSegment;

static dataPtr start = NULL;
static dataPtr last = NULL;

void newDataSegment()
{
	start = (dataPtr)malloc(sizeof(dataSegment));
	if (!start)
	{
		fprintf(stderr,"Malloc failed!\n");
		exit(1);
	}
	strcpy(start->data,"");
	start->next = NULL;
	last = start;
}

void addData(char dataRow[])
{
	dataPtr new = (dataPtr)malloc(sizeof(dataSegment));
	if (!new)
	{
		fprintf(stderr,"Malloc failed!\n");
		exit(1);
	}
	new->next=NULL;
	strcpy(new->data,dataRow);
	last->next = new;
	last = new;
}

void deleteDataSegment()
{
	dataPtr tmp = start;
	dataPtr tmp2 = start;
	while (tmp!=NULL)
	{
		tmp2=tmp2->next;
		free(tmp);
		tmp=tmp2;
	}
}

void printData(FILE *fp)
{
	dataPtr temp = start;
	while(temp!=NULL)
	{
		fputs(temp->data, fp);
		temp=temp->next;
	}
}
