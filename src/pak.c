#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

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
	FILE *f;
	int nf;
	struct pak_dir *dir;
	
	int dirty;
	int next_offset;
};

struct pak_file *pak_open(const char *name) {
	struct pak_file *p;
	struct pak_hdr hdr;
	int i;
	
	if(pak_verbose > 1) printf("[pak_open] opening file %s\n", name);
	
	p = malloc(sizeof *p);
	if(!p) return NULL;
	
	p->dirty = 0;
	
	p->f = fopen(name, "r+b");	
	if(!p->f) {
		free(p);
		return NULL;
	}
	
	if(pak_verbose > 1) printf("[pak_open] file %s opened\n", name);
	
	if(fread(&hdr, sizeof hdr, 1, p->f) != 1) {
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
	
	if(fseek(p->f, hdr.offset, SEEK_SET) != 0) {
		if(pak_verbose) perror("[pak_open] couldn't locate directory");
		goto error;
	}
	
	p->dir = calloc(p->nf, sizeof *p->dir);
	if(fread(p->dir, sizeof *p->dir, p->nf, p->f) != p->nf) {
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
			if(pak_verbose) perror("[pak_open] directory is not at the end of the file");
			fseek(p->f, 0, SEEK_END);
			p->next_offset = ftell(p->f);
			break;
		}
	}		
	
	return p;
		
error:
	fclose(p->f);
	free(p);
	return NULL;
}

static void write_header(struct pak_file *p, int dir_offset) {	
	struct pak_hdr hdr;
	
	strncpy(hdr.magic, "PACK", 4);
	hdr.offset = dir_offset;
	hdr.length = p->nf * (sizeof *p->dir);
	
	assert(p->f);
	rewind(p->f);	
	fwrite(&hdr, sizeof hdr, 1, p->f);
}

struct pak_file *pak_create(const char *name) {
	struct pak_file *p;
	
	if(pak_verbose > 1) printf("[pak_create] creating new file %s\n", name);
	
	p = malloc(sizeof *p);
	p->nf = 0;	
	p->dir = NULL;
	p->dirty = 0;
	
	p->f = fopen(name, "wb");
	if(!p->f) {
		if(pak_verbose) fprintf(stderr, "[pak_create] unable to open %s: %s\n", name, strerror(errno));
		free(p);
		return NULL;
	}
	
	write_header(p, 12);	
	assert(ftell(p->f) == 12);
	
	p->next_offset = ftell(p->f);
	
	return p;
}

int pak_append_blob(struct pak_file *p, const char *filename, const char *blob, int len) {
	if(pak_verbose > 1) printf("[pak_append_blob] Appending blob as %s\n", filename);
	
	if(!p->dir) {
		assert(p->nf == 0);
		p->dir = calloc(1, sizeof *p->dir);
	} else { 
		p->dir = realloc(p->dir, (p->nf + 1) * sizeof *p->dir);
	}
	
	if(fseek(p->f, p->next_offset, SEEK_SET) != 0) {
		if(pak_verbose) perror("[pak_append_blob] couldn't fseek next offset\n");
		return 0;
	}
	if(fwrite(blob, 1, len, p->f) != len) {
		if(pak_verbose) fprintf(stderr, "[pak_append_blob] unable to write %s: %s\n", filename, strerror(errno));
		return 0;
	}
	
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
	FILE *infile;
	
	if(pak_verbose > 1) printf("[pak_append_file] loading contents of %s\n", filename);
	
	infile = fopen(filename, "rb");		
	if(!infile) {
		if(pak_verbose) fprintf(stderr, "[pak_append_file] unable to open %s: %s\n", filename, strerror(errno));
		return 0;
	}	
	
	fseek(infile, 0, SEEK_END);
	len = ftell(infile);
	rewind(infile);
	
	bytes = malloc(len);
	if(!bytes) {
		if(pak_verbose) perror("[pak_append_file] unable to allocate memory");
		fclose(infile);
		return 0;
	}
	
	if(fread(bytes, 1, len, infile) != len) {
		if(pak_verbose) fprintf(stderr, "[pak_append_file] unable to read %s: %s\n", filename, strerror(errno));
		free(bytes);
		fclose(infile);
		return 0;
	}
	fclose(infile);
	
	rv = pak_append_blob(p, filename, bytes, len);
	
	free(bytes);
	
	return rv;
}

int pak_close(struct pak_file * p) {
	int rv = 1;
	if(pak_verbose > 1) printf("[pak_close] closing file\n");
	
	if(p->dirty) {
		int offset;
		if(pak_verbose > 1) printf("[pak_close] writing changes to file\n");
		/* Seek the end of the file and write the directory */
		fseek(p->f, 0, SEEK_END);
		offset = ftell(p->f);
		if(fwrite(p->dir, sizeof *p->dir, p->nf, p->f) != p->nf) {
			if(pak_verbose) perror("[pak_close] couldn't write directory");
			rv = 0;
		} else {		
			/* Seek the start of the file and write the header */
			write_header(p, offset);
		}
	} else {
		if(pak_verbose > 1) printf("[pak_close] no changes need to be written\n");
	}
	
	free(p->dir);
	if(p->f) 
		fclose(p->f);		
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

	if(pak_verbose > 1) printf("[pak_get_blob] retrieving file %s\n", filename);

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
	if(fseek(p->f, dir->offset, SEEK_SET) != 0) {
		if(pak_verbose) perror("[pak_get_blob] Couldn't locate file");
		free(blob);
		return NULL;
	}
	
	if(fread(blob, 1, dir->length, p->f) != dir->length) {
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
	if(fseek(p->f, dir->offset, SEEK_SET) != 0) {
		if(pak_verbose) perror("[pak_get_text] couldn't fseek file.");
		free(blob);
		return NULL;
	}
	
	if(fread(blob, 1, len, p->f) != dir->length) {
		if(pak_verbose) perror("[pak_get_text] couldn't read file.");
		free(blob);
		return NULL;
	}
	blob[len] = '\0';
	
	return blob;
}

FILE *pak_get_file(struct pak_file * p, const char *filename) {
	struct pak_dir *dir;

	if(pak_verbose > 1) printf("[pak_get_file] retrieving file %s from archieve\n", filename);
	
	dir = get_file(p, filename);
		
	if(!dir) {
		if(pak_verbose) fprintf(stderr, "[pak_get_file] file not found: %s\n", filename);
		return NULL;
	}	
	
	if(fseek(p->f, dir->offset, SEEK_SET) != 0) {
		if(pak_verbose) perror("[pak_get_file] couldn't fseek file.");
		return NULL;
	}
	return p->f;
}

int pak_extract_file(struct pak_file * p, const char *filename, const char *to) {
	size_t len;
	char *blob;
	FILE *out;
	int rv = 1;
	
	if(pak_verbose > 1) printf("[pak_extract_file] extracting %s to %s\n", filename, to);
	
	blob = pak_get_blob(p, filename, &len);
	if(!blob) {
		return 0;
	}
	
	out = fopen(to, "wb");
	if(!out) {
		if(pak_verbose) fprintf(stderr, "[pak_get_text] couldn't open %s for output: %s\n", to, strerror(errno));
		rv = 0;
	} else {
		if(fwrite(blob, 1, len, out) != len) {
			if(pak_verbose) perror("[pak_get_text] couldn't write file.");
			rv = 0;
		}
	}
	
	free(blob);
	
	return rv;
}
