#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#ifdef USESDL
#  include <SDL2/SDL.h>
#endif

#include "pak.h"

int pak_verbose = 0;

struct pak_hdr {
	char magic[4];
	int offset, length;
};

struct pak_dir {	
	char name[56];
	int offset, length;
};

struct pak_file {
#ifdef USESDL
    SDL_RWops *f;
#else
	FILE *f;
#endif
	int nf;
	struct pak_dir *dir;
	
	int dirty;
	int next_offset;
};

#ifdef USESDL
#define OPENFUN(name, mode) SDL_RWFromFile(name, mode);
#define READFUN(ptr, size, n, rw) SDL_RWread(rw, ptr, size, n)
#define SEEKFUN(rw, offs, mode) SDL_RWseek(rw, offs, RW_ ## mode)
#define SEEKOK(exp) ((exp)>=0)
#define TELLFUN(rw) SDL_RWtell(rw)
#define WRITFUN(ptr, size, n, rw) SDL_RWwrite(rw, ptr, size, n)
#define CLOSFUN(rw) SDL_RWclose(rw)
#define REWIND(rw) SDL_RWseek(rw, 0, RW_SEEK_SET)
#else
#define OPENFUN(name, mode) fopen(name, mode);
#define READFUN(ptr, size, n, f) fread(ptr, size, n, f)
#define SEEKFUN(f, offs, mode) fseek(f, offs, mode)
#define SEEKOK(exp) ((exp)==0)
#define TELLFUN(f) ftell(f)
#define WRITFUN(ptr, size, n, f) fwrite(ptr, size, n, f)
#define CLOSFUN(f) fclose(f)
#define REWIND(f) rewind(f)
#endif

struct pak_file *pak_open(const char *name) {
	struct pak_file *p;
	struct pak_hdr hdr;
	int i;
	
	if(pak_verbose > 1) printf("[pak_open] opening file %s\n", name);
	
	p = malloc(sizeof *p);
	if(!p) return NULL;
	
	p->dirty = 0;

	p->f = OPENFUN(name, "r+b");	
	if(!p->f) {
		free(p);
		return NULL;
	}
	
	if(pak_verbose > 1) printf("[pak_open] file %s opened\n", name);
	
	if(READFUN(&hdr, sizeof hdr, 1, p->f) != 1) {
		if(pak_verbose) perror("[pak_open] couldn't read header");
		goto error;
	}
	
	if(strncmp(hdr.magic, "PACK", 4)) {
		if(pak_verbose) fprintf(stderr, "[pak_open] bad magic\n");
		goto error;
	}
	
	if(pak_verbose > 1) printf("[pak_open] directory: offset: %d; length: %d;\n", hdr.offset, hdr.length);
	
	assert(sizeof *p->dir == 64);
	
	p->nf = hdr.length / (sizeof *p->dir);
	
	if(pak_verbose > 1) printf("[pak_open] there are %d files in the archive\n", p->nf);
	
	if(!SEEKOK(SEEKFUN(p->f, hdr.offset, SEEK_SET))) {
		if(pak_verbose) perror("[pak_open] couldn't locate directory");
		goto error;
	}
	
	p->dir = calloc(p->nf, sizeof *p->dir);
	if(READFUN(p->dir, sizeof *p->dir, p->nf, p->f) != p->nf) {
		if(pak_verbose) perror("[pak_open] couldn't read directory");
		free(p->dir);
		goto error;
	}
	
	/* Set p->next_offset.
	If the directory is at the end of the file, we can just overwrite the directory 
	when appending files and write the directory later.
	This loop is so that we don't blindly assume that it is the case.
	If the directory is not at the end of the file, the old directory will still take
	up space in the file, but I don't consider it a big issue at the moment.
	*/
	p->next_offset = hdr.offset;
	for(i = 0; i < p->nf; i++) {
		if(p->dir[i].offset > hdr.offset) {
			if(pak_verbose) 
				fprintf(stderr, "[pak_open] directory is not at the end of the file\n");
			SEEKFUN(p->f, 0, SEEK_END);
			p->next_offset = TELLFUN(p->f);
			break;
		}
	}		
	
	return p;
		
error:
	CLOSFUN(p->f);
	free(p);
	return NULL;
}

static void write_header(struct pak_file *p, int dir_offset) {	
	struct pak_hdr hdr;
	
	strncpy(hdr.magic, "PACK", 4);
	hdr.offset = dir_offset;
	hdr.length = p->nf * (sizeof *p->dir);
	
	assert(p->f);
	REWIND(p->f);	
	WRITFUN(&hdr, sizeof hdr, 1, p->f);
}

struct pak_file *pak_create(const char *name) {
	struct pak_file *p;
	
	if(pak_verbose > 1) 
		printf("[pak_create] creating new file %s\n", name);
	
	p = malloc(sizeof *p);
	p->nf = 0;	
	p->dir = NULL;
	p->dirty = 0;
	
	p->f = OPENFUN(name, "wb");
	if(!p->f) {
		if(pak_verbose) 
			fprintf(stderr, "[pak_create] unable to open %s: %s\n", name, strerror(errno));
		free(p);
		return NULL;
	}
	
	write_header(p, 12);	
	assert(TELLFUN(p->f) == 12);
	
	p->next_offset = TELLFUN(p->f);
	
	return p;
}

int pak_append_blob(struct pak_file *p, const char *filename, const char *blob, int len) {

	/*if(len <= 0) {
		printf("[pak_append_blob] warning: length of %s is %d bytes\n", filename, len);
		return 0;
	}*/

	if(pak_verbose > 1) 
		printf("[pak_append_blob] Appending blob of %d bytes as %s\n", len, filename);
	
	if(!p->dir) {
		assert(p->nf == 0);
		p->dir = calloc(1, sizeof *p->dir);
	} else { 
		p->dir = realloc(p->dir, (p->nf + 1) * sizeof *p->dir);
	}
	
	if(!SEEKOK(SEEKFUN(p->f, p->next_offset, SEEK_SET))) {
		if(pak_verbose) perror("[pak_append_blob] couldn't fseek next offset\n");
		return 0;
	}
	
	if(WRITFUN(blob, 1, len, p->f) != len) {
		if(pak_verbose) fprintf(stderr, "[pak_append_blob] unable to write %s: %s\n", filename, strerror(errno));
		return 0;
	}
	
	if(pak_verbose > 2) 
		printf("[pak_append_blob] Appended blob; Position is now %d\n", (int)TELLFUN(p->f));		
	
	p->dir[p->nf].length = len;
	p->dir[p->nf].offset = p->next_offset;
	p->next_offset += len;	
	snprintf(p->dir[p->nf].name, 55, "%s", filename);
		
	p->nf++;
	p->dirty = 1;
	
	return 1;
}

int pak_append_file(struct pak_file *p, const char *filename) {		
	int rv, len;
	char *bytes;

#ifdef USESDL
    SDL_RWops *infile;
#else
	FILE *infile;
#endif

	if(pak_verbose > 1)
		printf("[pak_append_file] loading contents of %s\n", filename);

	infile = OPENFUN(filename, "rb");		
	if(!infile) {
		if(pak_verbose) 
			fprintf(stderr, "[pak_append_file] unable to open %s: %s\n", filename, strerror(errno));
		return 0;
	}	
	
	if(!SEEKOK(SEEKFUN(infile, 0, SEEK_END))) {
		if(pak_verbose) 
			fprintf(stderr, "[pak_append_file] determine length of %s: %s\n", filename, strerror(errno));
		return 0;
	}
	len = TELLFUN(infile);
	REWIND(infile);
	
	if(pak_verbose > 1) 
		printf("[pak_append_file] %s contains %d bytes\n", filename, len);
	
	bytes = malloc(len);
	if(!bytes) {
		if(pak_verbose) 
			perror("[pak_append_file] unable to allocate memory");
		CLOSFUN(infile);
		return 0;
	}
	
	if(READFUN(bytes, 1, len, infile) != len) {
		if(pak_verbose) 
			fprintf(stderr, "[pak_append_file] unable to read %s: %s\n", filename, strerror(errno));
		free(bytes);
		CLOSFUN(infile);
		return 0;
	}
	CLOSFUN(infile);
	
	rv = pak_append_blob(p, filename, bytes, len);
	
	free(bytes);
	
	return rv;
}

int pak_close(struct pak_file * p) {
	int rv = 1;
	if(pak_verbose > 1) printf("[pak_close] closing file\n");
	
	if(p->dirty) {
		int offset;
		if(pak_verbose > 1) 
			printf("[pak_close] writing changes to file\n");
		/* Seek the end of the file and write the directory */
		if(!SEEKOK(SEEKFUN(p->f, 0, SEEK_END))) {
			if(pak_verbose) 
				fprintf(stderr, "[pak_append_file] unable to seek end of PAK for writing directory: %s\n", strerror(errno));
			return 0;
		}
		offset = TELLFUN(p->f);
		if(WRITFUN(p->dir, sizeof *p->dir, p->nf, p->f) != p->nf) {
			if(pak_verbose) 
				perror("[pak_close] couldn't write directory");
			rv = 0;
		} else {		
			/* Seek the start of the file and write the header */
			write_header(p, offset);
		}
	} else {
		if(pak_verbose > 1) 
			printf("[pak_close] no changes need to be written\n");
	}
	
	free(p->dir);
	if(p->f) 
		CLOSFUN(p->f);		
	free(p);	
	
	return rv;
}

int pak_num_files(struct pak_file * p) {
	return p->nf;
}

const char *pak_nth_file(struct pak_file * p, int n) {
	assert(n < p->nf);
	return p->dir[n].name;
}

static struct pak_dir *get_file(struct pak_file * p, const char *filename) {
	int i;
	for(i = 0; i < p->nf; i++) {
		if(!strcmp(p->dir[i].name, filename)) {
			return &p->dir[i];
		}
	}
	return NULL;
}

char *pak_get_blob(struct pak_file * p, const char *filename, size_t *len) {
	char *blob;
	struct pak_dir *dir;

	if(pak_verbose > 1) 
		printf("[pak_get_blob] retrieving file %s\n", filename);

	dir = get_file(p, filename);
	if(!dir) {
		if(pak_verbose) fprintf(stderr, "[pak_get_blob] file not found: %s\n", filename);
		return NULL;
	}	
	blob = malloc(dir->length);
	if(!blob) {
		if(pak_verbose) perror("[pak_get_blob] Couldn't allocate memory for blob");
		return NULL;
	}
	if(!SEEKOK(SEEKFUN(p->f, dir->offset, SEEK_SET))) {
		if(pak_verbose) perror("[pak_get_blob] Couldn't locate file");
		free(blob);
		return NULL;
	}
	
	if(READFUN(blob, 1, dir->length, p->f) != dir->length) {
		if(pak_verbose) perror("[pak_get_blob] Couldn't read file");
		free(blob);
		return NULL;
	}
	
	if(len) {
		*len = dir->length;
	}
	return blob;
}

char *pak_get_text(struct pak_file * p, const char *filename) {
	char *blob;
	size_t len;	
	struct pak_dir *dir;

	if(pak_verbose > 1) printf("[pak_get_text] retrieving file %s from archieve\n", filename);
	
	dir = get_file(p, filename);
		
	if(!dir) {
		if(pak_verbose) fprintf(stderr, "[pak_get_text] file not found: %s\n", filename);
		return NULL;
	}	
	
	len = dir->length;
	blob = malloc(len + 1);
	if(!blob) {
		if(pak_verbose) perror("[pak_get_text] couldn't allocate memory for text");
		return NULL;
	}
	if(!SEEKOK(SEEKFUN(p->f, dir->offset, SEEK_SET))) {
		if(pak_verbose) perror("[pak_get_text] couldn't fseek file.");
		free(blob);
		return NULL;
	}
	
	if(READFUN(blob, 1, len, p->f) != dir->length) {
		if(pak_verbose) perror("[pak_get_text] couldn't read file.");
		free(blob);
		return NULL;
	}
	blob[len] = '\0';
	
	return blob;
}

#ifndef USESDL
FILE *pak_get_file(struct pak_file * p, const char *filename) {
	struct pak_dir *dir;

	if(pak_verbose > 1) 
		printf("[pak_get_file] retrieving file %s from archieve\n", filename);
	
	dir = get_file(p, filename);
		
	if(!dir) {
		if(pak_verbose) fprintf(stderr, "[pak_get_file] file not found: %s\n", filename);
		return NULL;
	}	
	
	if(!SEEKOK(SEEKFUN(p->f, dir->offset, SEEK_SET))) {
		if(pak_verbose) perror("[pak_get_file] couldn't fseek file.");
		return NULL;
	}
	return p->f;
}
#endif

#ifdef USESDL
SDL_RWops *pak_get_rwops(struct pak_file * p, const char *filename) {
	struct pak_dir *dir;

	if(pak_verbose > 1) 
		printf("[pak_get_file] retrieving file %s from archieve\n", filename);
	
	dir = get_file(p, filename);
		
	if(!dir) {
		if(pak_verbose) fprintf(stderr, "[pak_get_file] file not found: %s\n", filename);
		return NULL;
	}	
	
	if(!SEEKOK(SEEKFUN(p->f, dir->offset, SEEK_SET))) {
		if(pak_verbose) perror("[pak_get_file] couldn't fseek file.");
		return NULL;
	}
	return p->f;
}
#endif

int pak_extract_file(struct pak_file * p, const char *filename, const char *to) {
	size_t len;
	char *blob;

#ifdef USESDL
    SDL_RWops *out;
#else
	FILE *out;
#endif
    
	int rv = 1;
	
	if(pak_verbose > 1) 
		printf("[pak_extract_file] extracting %s to %s\n", filename, to);
	
	blob = pak_get_blob(p, filename, &len);
	if(!blob) {
		return 0;
	}
	
	out = OPENFUN(to, "wb");
	if(!out) {
		if(pak_verbose) 
			fprintf(stderr, "[pak_get_text] couldn't open %s for output: %s\n", to, strerror(errno));
		rv = 0;
	} else {
		if(WRITFUN(blob, 1, len, out) != len) {
			if(pak_verbose) perror("[pak_get_text] couldn't write file.");
			rv = 0;
		}
	}
	
	free(blob);
	
	return rv;
}
