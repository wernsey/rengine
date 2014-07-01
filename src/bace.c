/*
Bace: Binary As C Encoder
Utility that takes a binary file as inputs and outputs it 
as C code in the form of a char[].
Pronounced 'Bake', because I need it to bake some binary 
files into my executable.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#define BUFFER_SIZE	1024

int verbose = 0;

void usage(const char *name) {
	fprintf(stderr, "Usage: %s [options] infile\n", name);
	fprintf(stderr, "where options:\n");
	fprintf(stderr, " -o file     : Set the output file (default: stdout)\n");
	fprintf(stderr, " -n name     : Name of the created variable.\n");
	fprintf(stderr, " -z          : Append a '\\0' to to the end of the generated array.\n");
	fprintf(stderr, " -v          : Verbose mode. Each -v increase verbosity.\n");
}

int main(int argc, char *argv[]) {
	FILE *infile = NULL;
	size_t len = 0, sublen, i;
	FILE *outfile = stdout;
	char *infile_name = NULL, *var_name = NULL, *c;
	char buffer[BUFFER_SIZE];
	int opt, append_z = 0;
	while((opt = getopt(argc, argv, "o:n:zv?")) != -1) {
		switch(opt) {	
			case 'o': {
				outfile = fopen(optarg, "w");
				if(!outfile) {
					fprintf(stderr, "error: unable to open %s for output\n", optarg);
					return 1;
				}
			} break;
			case 'n': {
				var_name = strdup(optarg);
			} break;
			case 'z' : {
				append_z = 1;
			} break;
			case 'v' : {
				verbose++;
			} break;
			case '?' : {
				usage(argv[0]);
				return 1;
			}
		}
	}
	
	if(optind >= argc) {
		fprintf(stderr, "error: expected infile\n");
		usage(argv[0]);
		return 1;
	}
	
	infile_name = argv[optind];
	infile = fopen(infile_name, "rb");
	if(!infile) {
		fprintf(stderr, "error: unable to open %s for input\n", infile_name);
		return 1;
	}
	
	if(!var_name) {
		var_name = strdup(argv[optind]);
	}
	for(c = var_name; *c; c++) {
		if(!isalnum(*c))
			*c = '_';
	}
	fprintf(outfile, "/*\nAuto-generated from %s using the Bace utility.\n", infile_name);
	fprintf(outfile, "Do not modify; Your modifications will be overwritten by the build process.\n\n");	
	fprintf(outfile, "extern const char %s[];\nextern size_t %s_len;\n*/\n\n", var_name, var_name);	
	fprintf(outfile, "#include <stdio.h>\n");
	fprintf(outfile, "const char %s[] = {", var_name);
	while(!feof(infile)) {
		sublen = fread(buffer, 1, BUFFER_SIZE, infile);
		for(i = 0; i < sublen; i++) {			
			char byte = buffer[i];
			if(len + i > 0) {
				if((len + i) % 8 == 0) {
					fprintf(outfile, ",\n  0x%02X", byte & 0xFF);
				} else
					fprintf(outfile, ", 0x%02X", byte & 0xFF);
			}else
				fprintf(outfile, "\n  0x%02X", byte & 0xFF);
		}
		len += sublen;
	}
	if(append_z) {
		/* Appending a \0 is useful if the input file is actually a text file.
			Having the null terminator allows you to treat the generated variable
			as a string. */
		fprintf(outfile, ",\n  0x00};\n");
	} else
		fprintf(outfile, "};\n");
	/* http://stackoverflow.com/a/2125854/115589 */
	fprintf(outfile, "size_t %s_len = %zd;\n", var_name, len);
	
	free(var_name);
	fclose(infile);
	if(outfile != stdout) {
		fclose(outfile);
	}		
	return 0;
}
