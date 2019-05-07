/* BIN2C Converts binary data to be included in C source codes */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


static void usage(void)
{
	fprintf(stderr, "BIN2C Binary to C converter\n");
	fprintf(stderr, "usage: BIN2C <binfile> <cfile> <varname>\n");
	exit(0);
}


int main(int argc, char **argv)
{
	FILE* binfile;
	FILE* cfile;
	struct stat statbuf;
	long len;
	int cntr;
	unsigned char inbyte;
	char varname[80], cfilename[80];

	if (argc == 1)
		usage();

	if (! (binfile = fopen(argv[1], "rb"))) {
		perror("fopen");
		exit(1);
	}

	stat(argv[1], &statbuf);
	len = statbuf.st_size;

	if (argc > 2)
		strcpy(cfilename, argv[2]);
	else
		strcpy(cfilename,"bin2c.c");
	cfile = fopen(cfilename, "wt");

	cntr = 0;

	if (argc > 3)
		strcpy(varname, argv[3]);
	else
		strcpy(varname, "varname");

	fprintf(cfile, "unsigned char %s[%ld] = {\n", varname, len);

	fread(&inbyte, 1, 1, binfile);
	while (1) {
		fprintf(cfile, "0X%02X", inbyte);

		fread(&inbyte, 1, 1, binfile);
		if (feof(binfile)) 
			break;
		fprintf(cfile, ",");
		if (++cntr > 30) {
			fprintf(cfile, "\n");
			cntr = 0;
		}
	}

	fprintf(cfile, "};\n");
	fclose(binfile);
	fclose(cfile);
	return 0;
}
