// tibin2obj.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <string.h>

int _tmain(int argc, _TCHAR* argv[])
{
	FILE *f1,*f2;
	int nCount=0, nLines=0;
	int address;
	char outbuf[256], tmpbuf[80];

	if (argc < 4) {
		printf("tibin2obj <binary file> <object file> <address in hex>\n-Only Absolute data is output, no headers please.\n");
		return 0;
	}

	f1=fopen(argv[1], "rb");
	strcpy(outbuf, argv[2]);
	f2=fopen(outbuf, "w");
	sscanf(argv[3], "%x", &address);
	printf("Locating at >%04X\n", address);

	sprintf(outbuf, "9%4X", address);
	nCount=5;

	while (!feof(f1)) {
		int c1 = fgetc(f1);
		int c2 = fgetc(f1);

		sprintf(tmpbuf, "B%02X%02X", c1&0xff, c2&0xff);
		strcat(outbuf, tmpbuf);
		nCount+=5;

		if (nCount>70) {
			strcat(outbuf, "80000F");
			fprintf(f2, "%s\n", outbuf);
			nLines++;
			strcpy(outbuf, "");
			nCount=0;
		}
	}

	if (strlen(outbuf)) {
		strcat(outbuf, "80000F");
		fprintf(f2, "%s\n", outbuf);
		nLines++;
	}

	fprintf(f2, ":0000 tibin2obj\n");

	fclose(f1);
	fclose(f2);

	printf("Output %d lines\n", nLines);

	return 0;
}

