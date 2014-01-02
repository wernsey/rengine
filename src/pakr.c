/*
Utility program to create .PAK files.

NOTE: On Windows, I use MinGW which provides
	some POSIX functionality, such as unistd.h and
	so on. It also allows one to use '/' as the path
	separator, which is the way I prefer it.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h> 
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pak.h"

int inc_hidden = 0; /* Include hidden files in PAK. Default no */

void usage(const char *name) {
	fprintf(stderr, "Usage: %s [options] pakfile [files...]\n", name);
	fprintf(stderr, "where options:\n");
	fprintf(stderr, " -d dir      : Create/Overwrite pakfile from directory dir.\n");
	fprintf(stderr, " -c          : Create/Overwrite pakfile from files.\n");
	fprintf(stderr, " -x          : Extract pakfile into current directory.\n");
	fprintf(stderr, " -a          : Append files to pakfile.\n");
	fprintf(stderr, " -u          : dUmps the contents of a file.\n");
	fprintf(stderr, " -t          : Dumps the contents of a text file.\n");
	fprintf(stderr, " -o file     : Set the output file for -u and -t.\n");
	fprintf(stderr, " -h          : Include hidden files when using the -d option.");
	fprintf(stderr, " -v          : Verbose mode. Each -v increase verbosity.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "If no options are specified, the file is just listed.\n");
	fprintf(stderr, "If the -o option is not used, files are written to stdout.\n");	
	fprintf(stderr, "The extract option doesn't attempt to preserve the directory structure.\n");
}

void pak_dir(DIR *dir, const char *base, struct pak_file *pak) {		
	struct dirent *dp = NULL;
	while((dp = readdir(dir)) != NULL) {
		struct stat statbuf;
		if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		
		if(!inc_hidden && dp->d_name[0] == '.')
			continue;
		
		char path[512];
		if(base)
			snprintf(path, sizeof path, "%s/%s", base, dp->d_name);
		else
			strncpy(path, dp->d_name, sizeof path);
		
		if(stat(path, &statbuf)) {
			fprintf(stderr, "Unable to stat %s: %s\n", path, strerror(errno));
			continue;
		}
		
		if(S_ISDIR(statbuf.st_mode)) {			
			DIR *newdir;
			newdir = opendir(path);
			if(newdir) {
				pak_dir(newdir, path, pak);
			} else {
				fprintf(stderr, "error: Unable to read directory %s: %s\n", path, strerror(errno));
			}
		} else {					
			printf(" - %s\n", path);
			if(!pak_append_file(pak, path)) {
				fprintf(stderr, "error: Unable to write %s to PAK file\n", path);
				break;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	
	int opt;
	const char *pakfile;
	const char *dir_name;
	FILE *outfile = stdout;
	
	enum {
		CREATE,
		DIR_CREATE,
		APPEND,
		XTRACT,
		DUMP,
		DUMPT,
		LIST
	} mode = LIST;
	
	while((opt = getopt(argc, argv, "d:caxuto:v?")) != -1) {
		switch(opt) {
			case 'c' : {
				mode = CREATE;
			} break;
			case 'd' : {
				mode = DIR_CREATE;
				dir_name = optarg;
			} break;
			case 'a' : {
				mode = APPEND;
			} break;
			case 'x' : {
				mode = XTRACT;				
			} break;
			case 'u' : {
				mode = DUMP;
			} break;
			case 't' : {
				mode = DUMPT;
			} break;
			case 'o': {
				outfile = fopen(optarg, "w");
				if(!outfile) {
					fprintf(stderr, "error: unable to open %s for output\n", optarg);
					return 1;
				}
			} break;
			case 'h': {
				inc_hidden = 1;
			} break;
			case 'v' : {
				pak_verbose++;
			} break;
			case '?' : {
				usage(argv[0]);
				return 1;
			}
		}
	}
	
	if(optind >= argc) {
		fprintf(stderr, "error: expected pakfile\n");
		usage(argv[0]);
		return 1;
	}
	
	pakfile = argv[optind++];
		
	switch(mode) {
		case CREATE : {
			struct pak_file *p;
			if(optind >= argc) {
				fprintf(stderr, "error: cowardly refusing to create empty pakfile\n");
				return 1;
			}
			
			printf("Creating %s:\n", pakfile);
			p = pak_create(pakfile);
			
			while(optind < argc) {
				const char *filename = argv[optind++];
				printf(" - %s\n", filename);
				if(!pak_append_file(p, filename)) {
					fprintf(stderr, "error: Unable to write %s to PAK file\n", filename);
					break;
				}
			}
			pak_close(p);
		}break;
		case DIR_CREATE : {
			struct pak_file *p;
			DIR *dir;
			
			printf("Creating %s:\n", pakfile);
			p = pak_create(pakfile);
			
			if(!chdir(dir_name)) {
				dir = opendir(".");
				pak_dir(dir, NULL, p);
			} else {				
				fprintf(stderr, "error: Unable to read directory %s: %s\n", dir_name, strerror(errno));
			}			
			pak_close(p);			
		} break;
		case APPEND : {
			if(optind >= argc) {
				fprintf(stderr, "error: no files to append.\n");
				return 1;
			}
			struct pak_file *p = pak_open(pakfile);
			
			printf("Appending to %s:\n", pakfile);
			while(optind < argc) {
				const char *filename = argv[optind++];
				printf(" - %s\n", filename);
				if(!pak_append_file(p, filename)) {
					fprintf(stderr, "error: unable to append %s to PAK file\n", filename);
					break;
				}
			}			
			pak_close(p);
			
		} break;
		case XTRACT : {
			/* The extract option doesn't attempt to preserve the directory structure.
			It can't without going into platform specifics for managing the directories */
			struct pak_file *p = pak_open(pakfile);
			int i;

			if(!p) {
				fprintf(stderr, "error: unable to read %s\n", pakfile);
				return 1;
			}
			for(i = 0; i < pak_num_files(p); i++) {
				const char *filepath = pak_nth_file(p, i);
				const char *file = filepath;
				char *delim; 
				if((delim = strrchr(filepath, '/')) != NULL || (delim = strrchr(filepath, '\\')) != NULL) {
					file = delim + 1;
				}
				printf("%3d: %s -> %s\n", i, filepath, file);
				if(!pak_extract_file(p, filepath, file)) {
					fprintf(stderr, "error: unable to extract %s\n", file);
				}
			}		
			pak_close(p);			
		} break;
		case DUMP : {
			struct pak_file *p = pak_open(pakfile);
			char *blob;
			size_t len;
			if(!p) {
				fprintf(stderr, "error: unable to read %s\n", pakfile);
				return 1;
			}			
			while(optind < argc) {
				blob = pak_get_blob(p, argv[optind++], &len);
				if(blob) {
					fwrite(blob, len, 1, outfile);
					free(blob);
				} else {
					fprintf(stderr, "error: unable to load %s from %s\n", argv[optind - 1], pakfile);
				}
			}
			pak_close(p);	
		} break;
		case DUMPT : {
			struct pak_file *p = pak_open(pakfile);
			char *text;
			if(!p) {
				fprintf(stderr, "error: unable to read %s\n", pakfile);
				return 1;
			}			
			while(optind < argc) {
				text = pak_get_text(p, argv[optind++]);
				if(text) {
					fputs(text, outfile);
					free(text);
				} else {
					fprintf(stderr, "error: unable to load %s from %s\n", argv[optind - 1], pakfile);
				}
			}
			pak_close(p);		
		} break;
		default: {
			struct pak_file *p = pak_open(pakfile);
			int i;
			printf("Contents of %s\n", pakfile);
			if(!p) {
				fprintf(stderr, "error: unable to read %s\n", pakfile);
				return 1;
			}
			for(i = 0; i < pak_num_files(p); i++) {
				printf("%3d: %s\n", i, pak_nth_file(p, i));
			}		
			pak_close(p);		
		} break;
	}
	
	if(outfile != stdout) {
		fclose(outfile);
	}
	
	return 0;
}
