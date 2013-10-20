/*
Converts a binary file to a C array.
From http://stackoverflow.com/a/5679968/115589 

I think I'll need this, because I may write a "standard library"
for Rengine in Lua, and this will allow me to bake the code of said 
standard library into Rengine's executable.

Proper FIXME: Write my own.

My changes:
- Uses stdin and stdout by default.
- Specify input and output file as command-line parameters -i and -o

TODO:
- Perhaps you should be able to add more than one input file 
   (how will the variable names be handled in this case?)
   - Perhaps it should have an append option so that you run it a couple
   of times with the same parameters to append new variables to the 
   generated C file.
- Specify name array variable as command line parameter -a
- The output file should also contain the length of the array:
   eg. int hex_array_size = 123;
- Maybe an option to output a header as well -h
   eg. hex_array.h:
   extern char hex_array[];
   extern int hex_array_size;
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 80

void usage(const char *name) {
	fprintf(stderr, "Usage: %s [options]\n", name);
	fprintf(stderr, "where options:\n");
	fprintf(stderr, " -i file     : Set the input file\n");
	fprintf(stderr, " -o file     : Set the output file\n");
	fprintf(stderr, " -a          : Append; Don't replace output file\n");
}

int main(int argc, char *argv[])
{
	int opt, append = 0;
	const char *in = NULL, *out = NULL;
	FILE *fin = stdin;
    FILE *fout = stdout;
	const char *varname = "hex_array";
	
	while((opt = getopt(argc, argv, "i:o:?")) != -1) {
		switch(opt) {
			case 'i' : {
				in = optarg;
			} break;
			case 'o' : {
				out = optarg;
			} break;
			case 'a' : {
				append = 1;
			} break;
			case '?' : {
				usage(argv[0]);
				return 1;
			}
		}
	}

	if(in) {	
		fin = fopen(in, "rb");
		if(ferror(fin))
		{
			fprintf(stderr, "Error opening input file\n");
			return 1;
		}
	}
	if(out) {
		if(append) {
			fout = fopen(out, "w");
		} else {
			fout = fopen(out, "a");
		}
		if(ferror(fout))
		{
			fprintf(stderr, "Error opening output file\n");
			return 1;
		}
	}
	
    char init_line[]  = {"char [] = { "};
    const int offset_length = strlen(init_line) + strlen(varname);

    char offset_spc[offset_length];

    unsigned char buff[1024];
    char curr_out[64];

    int count, i;
    int line_length = 0;

    memset((void*)offset_spc, (char)32, sizeof(char) * offset_length - 1);
    offset_spc[offset_length - 1] = '\0';

    fprintf(fout, "char %s[] = { ", varname);

    while(!feof(fin))
    {
        count = fread(buff, sizeof(char), sizeof(buff) / sizeof(char), fin);

        for(i = 0; i < count; i++)
        {
            line_length += sprintf(curr_out, "%#x, ", buff[i]);

            fprintf(fout, "%s", curr_out);
            if(line_length >= MAX_LENGTH - offset_length)
            {
                fprintf(fout, "\n%s", offset_spc);
                line_length = 0;
            }
        }
    }
    fseek(fout, -2, SEEK_CUR);
    fprintf(fout, " };");

	if(fin != stdin) fclose(fin);
    if(fout != stdout) fclose(fout);

    return EXIT_SUCCESS;
}