// tibin2robj.cpp : Converts a binary file to a TI Relocatable Uncompressed Object file
// Writes it as a DF80 with TIFILES header.

#include <iostream>
#include <list>
#include <string>
#include <stdio.h>

using namespace std;
char output[1000][80];        // TODO: something is really messed up. Can't use vector or list here without crashes on push_back OR emplace_back
unsigned char buf[8192];

// every line will be padded to 80 chars
// per E/A standard, we stop emitting when we pass character 60
const int stopchar = 60;
char str[80];

// function stolen from Classic99 - hacked for DF80 TIFILES only
void WriteFileHeader(FILE *fp, int lines) {
	unsigned char h[128];							// header

	fseek(fp, 0, SEEK_SET);
	memset(h, 0, 128);							// initialize with zeros

	int sectors = (lines+2)/3;	// round up sector count

	/* create TIFILES header - more up to date version of the header? */
	h[0] = 7;
	h[1] = 'T';
	h[2] = 'I';
	h[3] = 'F';
	h[4] = 'I';
	h[5] = 'L';
	h[6] = 'E';
	h[7] = 'S';
	h[8] = (sectors>>8)&0xff;	// length in sectors HB
	h[9] = sectors&0xff;		// LB 
	h[10] = 0;					// File type  (0=DF)
	h[11] = 3;					// records/sector (256/80)
	h[12] = 0;					// # of bytes in last sector (Fred and Theirry say it's 0 on Fixed)
	h[13] = 80;					// record length 
	h[14] = lines&0xff;			// # of records(FIX)/sectors(VAR) LB 
	h[15] = (lines>>8)&0xff;	// HB

	fwrite(h, 1, 128, fp);
}

// function stolen from Classic99 - hacked for DF80 only
bool FlushFiad(const char *fn, int lines) {
	int nSector;

	FILE *fp = fopen(fn, "wb");
	if (NULL == fp) {
		printf("Unable to write file %s", fn);
		return false;
	}

	// write file info
	WriteFileHeader(fp, lines);
	fseek(fp, 128, SEEK_SET);

	nSector=256;
	for (int idx=0; idx<lines; ++idx) {
		int nLen = 80;

		// write a fixed length record
		// first, check if it will fit
		if (nSector < nLen) {
			// pad the sector out
			while (nSector > 0) {
				fputc(0, fp);
				nSector--;
			}
			nSector=256;
		}

		// write the data
		fwrite(output[idx], 1, nLen, fp);
		nSector-=nLen;
	}

	// don't forget to pad the file to a full sector!
	while (nSector > 0) {
		fputc(0, fp);
		nSector--;
	}

	fclose(fp);
	printf("Flushed %s (%d records) as TIFILES FIAD", fn, lines);

	return true;
}


int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("tibin2robj <input binary> <output TIFILES obj>\n");
        printf("Writes uncompressed, relocatable file suitable for XB\n");
        return 1;
    }

    int off = 0;
    int line = 1;

    FILE *fp = fopen(argv[1], "rb");
    if (NULL == fp) {
        printf("Failed to open input file\n");
        return 1;
    }

    int sz = fread(buf, 1, sizeof(buf), fp);
    if (!feof(fp)) {
        printf("Input file too large - max size %d\n", sizeof(buf));
        fclose(fp);
        return 1;
    }
    fclose(fp);

    // the first line contains the module ID tag 0, which is 0<length:4><name:8>
    sprintf(str, "0%04X        ", sz);
    bool newln = true;
    while (off < sz) {
        // check for start of new line
        if (newln) {
            // add the address tag A<load offset:4>
            sprintf(str, "%sA%04X", str, off);  // yes, get used to this evil pattern
            newln = false;
        }
        // check for end of current line
        if (strlen(str) >= stopchar) {
            // add the checksum, F, pad out to 80 characters including decimal line number
            int chk=0;
            // 16-bit checksum by byte in the line
            for (unsigned int i=0; i<strlen(str); ++i) {
                chk+=str[i];
            }
            chk+='7';   // the checksum includes the '7' tag for the checksum, which we haven't appended yet
            sprintf(str, "%s7%04XF", str, ((~chk)+1)&0xffff);    // two's complement
            sprintf(str, "%-76s%04d", str, line++);
            strcpy(output[line-2], str);    // line-2 is unfortunate - it's already 1-based, AND we already incremented it
            strcpy(str, "");
            newln = true;
            continue;
        }
        // now we just spit out bytes, 2 at a time, using tag B<data:4>
        int val = buf[off]*256+buf[off+1];  // we know this is always safe though if the file is odd bytes the last byte may be random
        sprintf(str, "%sB%04X", str, val);
        off+=2;
    }

    // emit the final data line
    if (strlen(str)) {
        // add the checksum, F, pad out to 80 characters including decimal line number
        int chk=0;
        // 16-bit checksum by byte in the line
        for (unsigned int i=0; i<strlen(str); ++i) {
            chk+=str[i];
        }
        chk+='7';   // the checksum includes the '7' tag for the checksum, which we haven't appended yet
        sprintf(str, "%s7%04XF", str, ((~chk)+1)&0xffff);    // two's complement
        sprintf(str, "%-76s%04d", str, line++);
        strcpy(output[line-2], str);    // line-2 is unfortunate - it's already 1-based, AND we already incremented it
    }

    // Now emit the DEF line
    sprintf(str, "50000MUSIC 7");
    int chk=0;
    // 16-bit checksum by byte in the line
    for (unsigned int i=0; i<strlen(str); ++i) {
        chk+=str[i];
    }
    sprintf(str, "%s%04XF", str, ((~chk)+1)&0xffff);    // two's complement
    sprintf(str, "%-76s%04d", str, line++);
    strcpy(output[line-2], str);    // line-2 is unfortunate - it's already 1-based, AND we already incremented it

    // and the file termination line
    sprintf(str, ":tibin2robj by Tursi");
    sprintf(str, "%-76s%04d", str, line++);
    strcpy(output[line-2], str);    // line-2 is unfortunate - it's already 1-based, AND we already incremented it

    // okay, now we need to format it into a TIFILES sector-based representation
    if (!FlushFiad(argv[2], line-1)) {
        return 1;
    }

    return 0;
}
