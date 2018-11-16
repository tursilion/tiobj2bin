// tiobj2bin.cpp : Defines the entry point for the console application.
//

// whatever banking it did before, now I need to create EA#5 files from data
// in all the banks

#include "stdafx.h"
#include <stdio.h>

unsigned char memory[64*1024];
char tifiles[128] = "\x7TIFILES\0\x20\x01\0\0\0\0\0";
bool bRaw = false;

int _tmain(int argc, _TCHAR* argv[])
{
	FILE *f1,*f2;
	bool bFoundData=false;
	bool bEOF=false;
	int nCount=0, nFiles=1;
	char outbuf[256];
	int lowestAdr = 0xffff;
	int highestAdr = 0x0000;

	if (argc < 3) {
		printf("tiobj2bin <object file> <binary file> -raw\n-Only Absolute data is handled.\n");
		printf("-raw will skip the padding and TIFILES and PROGRAM headers, and just write the data\n");
		return 0;
	}

	if (argc >= 4) {
		if (0 == strcmp(argv[3], "-raw")) {
			printf("File will be written raw\n");
			bRaw = true;
		}
	}

	f1=fopen(argv[1], "r");
	strcpy(outbuf, argv[2]);

	unsigned int address=0xa000;		// default relative address
	bool b0=false, b1=false, b2=false, b3=false;	// TIFILES 'banks'
	bool c0=false;									// cartridge 'banks'

	while ((!feof(f1)) && (!bEOF)) {
		char buf[128];
		if (NULL == fgets(buf, 128, f1)) break;
		int n=0;
		while (buf[n]) {
			int x;
			switch (buf[n]) {
				case '0':	// tag, skip 5+8
					n+=8+5;
					break;
				case 'B':	// absolute data - the stuff we want
					sscanf(&buf[n+1], "%4x", &x);
					if (address < lowestAdr) lowestAdr=address;
					if (address > highestAdr) highestAdr=address;

					if ((address>=0x2000)&&(address<0x4000)) {
						b0=true;
						memory[address]=(x>>8)&0xff;
						memory[address+1] = (x&0xff);
						address+=2;
					} else if ((address>=0xa000)&&(address<=0xffff)) {
						if (address<0xbffa) {
							b1=true;
						} else if (address < 0xdff4) {
							b2=true;
						} else {
							b3=true;
						}
						memory[address]=(x>>8)&0xff;
						memory[address+1] = (x&0xff);
						address+=2;
					} else if ((address>=0x6000)&&(address<0x8000)) {
						c0=true;
						memory[address]=(x>>8)&0xff;
						memory[address+1] = (x&0xff);
						address+=2;
					} else {
						printf("bad address 0x%04X\n", address);
						return 0;
					}
					n+=5;
					nCount+=2;
					bFoundData=true;
					break;
				case 'F':	// checksum, end of line
				case ' ':	// just in case
					buf[n]='\0';
					break;
				case ':':	// end of file
					bEOF=true;
					buf[n]='\0';
					break;
				case '9':	// absolute address
					sscanf(&buf[n+1], "%4x", &x);
					address = x;
					n+=5;
					break;

				default:	// anything else, ignore tag
					n+=5;
					break;
			}

#if 0
			// this is supposed to break up programs that have banks or something
			if (nCount == 0xa000) {
				nCount=0;
				fclose(f2);
				outbuf[strlen(outbuf)-1]++;
				f2=fopen(outbuf, "wb");
				nFiles++;
			}
#endif
		}
	}

	fclose(f1);

	if (!bRaw) {
		// A000 first :)
		if (b1) {
			f2=fopen(outbuf, "wb");
			fwrite(tifiles, 1, 128, f2);
			fwrite("\xff\xff\x1f\xfa\xA0\x00", 1, 6, f2);
			fwrite(&memory[0xa000], 1, 8192-6, f2);
			fclose(f2);
			printf("Wrote '%s'\n", outbuf);
			outbuf[strlen(outbuf)-1]++;
		}
		if (b0) {
			f2=fopen(outbuf, "wb");
			fwrite(tifiles, 1, 128, f2);
			fwrite("\xff\xff\x1f\xfa\x20\x00", 1, 6, f2);
			fwrite(&memory[0x2000], 1, 8192-6, f2);
			fclose(f2);
			printf("Wrote '%s'\n", outbuf);
			outbuf[strlen(outbuf)-1]++;
		}
		if (b2) {
			f2=fopen(outbuf, "wb");
			fwrite(tifiles, 1, 128, f2);
			fwrite("\xff\xff\x1f\xfa\xbf\xfa", 1, 6, f2);
			fwrite(&memory[0xc000], 1, 8192-6, f2);
			fclose(f2);
			printf("Wrote '%s'\n", outbuf);
			outbuf[strlen(outbuf)-1]++;
		}
		if (b3) {
			f2=fopen(outbuf, "wb");
			fwrite(tifiles, 1, 128, f2);
			fwrite("\0\0\x1f\xfa\xdf\xf4", 1, 6, f2);
			fwrite(&memory[0xe000], 1, 8192, f2);
			fclose(f2);
			printf("Wrote '%s'\n", outbuf);
			outbuf[strlen(outbuf)-1]++;
		}
		if (c0) {
			f2=fopen(outbuf, "wb");
			// carts don't have TIFILES headers or EA5 loader headers
			fwrite(&memory[0x6000], 1, 8192, f2);
			fclose(f2);
			printf("Wrote '%s'\n", outbuf);
		}
	} else {
		// just a raw binary dump
		if (highestAdr > lowestAdr) {
			// make an even count of output bytes
			if ((highestAdr-lowestAdr+1)&0x01) ++highestAdr;
			if (lowestAdr&0x01) {
				printf("Can't start a dump at an odd memory address = >%04X\n", lowestAdr);
				bFoundData = false;
			} else {
				f2=fopen(outbuf, "wb");
				// carts don't have TIFILES headers or EA5 loader headers
				fwrite(&memory[lowestAdr], 1, highestAdr-lowestAdr+1, f2);
				fclose(f2);
				printf("Wrote '%s' from >%04X to >%04X\n", outbuf, lowestAdr, highestAdr);
			}
		}
	}

	if (!bFoundData) printf("Didn't find any absolute data to write!\n");

	return 0;
}

