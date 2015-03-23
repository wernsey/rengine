#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <float.h>
#include <assert.h>

#ifdef USESDL
#  ifdef ANDROID
#    include <SDL.h>
#  else
#    include <SDL2/SDL.h>
#  endif
#endif

/*
Use the -DUSEPNG compiler option to enable PNG support via libpng.
If you use it, you need to link against the libpng (-lpng)
and zlib (-lz) libraries.
Use the -DUSEJPG compiler option to enable JPG support via libjpg.
I've decided to keep both optional, for situations where
you may not want to import a bunch of third party libraries.
*/
#ifdef USEPNG
#	include <png.h>
#endif
#ifdef USEJPG
#	include <jpeglib.h>
#	include <setjmp.h>
#endif

#include "bmp.h"

/*
TODO: 
	The alpha support is a recent addition, so it is still a bit 
	sporadic and not well tested.
	I may also decide to change the API around it (especially wrt. 
	blitting) in the future.
	
	Also, functions like bm_color_atoi() and bm_set_color_i() does 
	not take the alpha value into account. The integers they return and
	accept is still 0xRRGGBB instead of 0xRRGGBBAA - It probably implies
	that I should change the type to unsigned int where it currently is
	just int.
	
	PNG support is also fairly new, so it could do with some testing.
	
	I may also want to add a function to detect the file types. Both
	BMP and PNG files have magic numbers in their headers to check them.
	I should change bm_load() to use this, and add a separate bm_load_bmp
	function.
*/

#ifndef NO_FONTS
/* I basically drew font.xbm from the fonts at.
 * http://damieng.com/blog/2011/02/20/typography-in-8-bits-system-fonts
 * The Apple ][ font turned out to be the nicest normal font.
 * The bold font was inspired by Commodore 64.
 * I later added some others for a bit of variety.
 */
#include "fonts/bold.xbm"
#include "fonts/circuit.xbm"
#include "fonts/hand.xbm"
#include "fonts/normal.xbm"
#include "fonts/small.xbm"
#include "fonts/smallinv.xbm"
#include "fonts/thick.xbm"
#endif

#define FONT_WIDTH 96

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/* Data structures for the header of BMP files. */
struct bmpfile_magic {
  unsigned char magic[2];
};
 
struct bmpfile_header {
  uint32_t filesz;
  uint16_t creator1;
  uint16_t creator2;
  uint32_t bmp_offset;
};

struct bmpfile_dibinfo {
  uint32_t header_sz;
  int32_t width;
  int32_t height;
  uint16_t nplanes;
  uint16_t bitspp;
  uint32_t compress_type;
  uint32_t bmp_bytesz;
  int32_t hres;
  int32_t vres;
  uint32_t ncolors;
  uint32_t nimpcolors;
};

struct bmpfile_colinfo {
	uint8_t b, g, r, a;
};

#define BM_BPP			4 /* Bytes per Pixel */
#define BM_BLOB_SIZE(B)	(B->w * B->h * BM_BPP)
#define BM_ROW_SIZE(B)	(B->w * BM_BPP)

#define BM_SET(BMP, X, Y, R, G, B, A) do { \
		int _p = ((Y) * BM_ROW_SIZE(BMP) + (X)*BM_BPP);	\
		BMP->data[_p++] = B;\
		BMP->data[_p++] = G;\
		BMP->data[_p++] = R;\
		BMP->data[_p++] = A;\
	} while(0)

#define BM_GETB(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 0])
#define BM_GETG(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 1])
#define BM_GETR(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 2])
#define BM_GETA(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 3])

/* N=0 -> B, N=1 -> G, N=2 -> R, N=3 -> A */
#define BM_GETN(B,N,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + (N)])

Bitmap *bm_create(int w, int h) {	
	Bitmap *b = malloc(sizeof *b);
	
	b->w = w;
	b->h = h;
	
	b->clip.x0 = 0;
	b->clip.y0 = 0;
	b->clip.x1 = w;
	b->clip.y1 = h;
		
	b->data = malloc(BM_BLOB_SIZE(b));
	memset(b->data, 0x00, BM_BLOB_SIZE(b));
	
	b->font = NULL;
#ifndef NO_FONTS
	bm_std_font(b, BM_FONT_NORMAL);
#endif

	bm_set_color(b, 255, 255, 255);
	bm_set_alpha(b, 255);
	
	return b;
}

Bitmap *bm_load(const char *filename) {	
	Bitmap *bmp;
	FILE *f = fopen(filename, "rb");
	if(!f)
		return NULL;
	bmp = bm_load_fp(f);	
	fclose(f);	
	return bmp;
}

static Bitmap *bm_load_bmp_fp(FILE *f);
static Bitmap *bm_load_pcx_fp(FILE *f);
#ifdef USEPNG
static Bitmap *bm_load_png_fp(FILE *f);
#endif
#ifdef USEJPG
static Bitmap *bm_load_jpg_fp(FILE *f);
#endif

Bitmap *bm_load_fp(FILE *f) {
	unsigned char magic[2];	

	long start = ftell(f), 
		isbmp = 0, ispng = 0, isjpg = 0, ispcx = 0;	
	/* Tries to detect the type of file by looking at the first bytes. */
	if(fread(magic, sizeof magic, 1, f) == 1) {
		if(magic[0] == 'B' && magic[1] == 'M') 
			isbmp = 1;
		else if(magic[0] == 0xFF && magic[1] == 0xD8)
			isjpg = 1;
		else if(magic[0] == 0x0A)
			ispcx = 1;
		else
			ispng = 1; /* Assume PNG by default. 
					JPG and BMP magic numbers are simpler */
	}
	fseek(f, start, SEEK_SET);
	
#ifdef USEJPG
	if(isjpg)
		return bm_load_jpg_fp(f);
#else
	(void)isjpg;
#endif
#ifdef USEPNG
	if(ispng)
		return bm_load_png_fp(f);
#else
	(void)ispng;
#endif
	if(ispcx)
		return bm_load_pcx_fp(f);
	if(!isbmp) 
		return NULL;
	return bm_load_bmp_fp(f);
}

static Bitmap *bm_load_bmp_fp(FILE *f) {	
	struct bmpfile_magic magic; 
	struct bmpfile_header hdr;
	struct bmpfile_dibinfo dib;
	struct bmpfile_colinfo *palette = NULL;
	
	Bitmap *b = NULL;
	
	int rs, i, j;
	char *data = NULL;
	
	long start_offset = ftell(f);
		
	if(fread(&magic, sizeof magic, 1, f) != 1) {
		return NULL;
	}
	if(magic.magic[0] != 'B' || magic.magic[1] != 'M') {
		return NULL;
	}
	
	if(fread(&hdr, sizeof hdr, 1, f) != 1) {
		return NULL;
	}
	
	if(fread(&dib, sizeof dib, 1, f) != 1) {
		return NULL;
	}
	
	if((dib.bitspp != 8 && dib.bitspp != 24) || dib.compress_type != 0) {
		/* Unsupported BMP type. TODO (maybe): support more types? */
		return NULL;
	}
	
	b = bm_create(dib.width, dib.height);
	if(!b) {
		return NULL;
	}
	
	if(dib.bitspp <= 8) {
		if(!dib.ncolors) {
			dib.ncolors = 1 << dib.bitspp;
		}		
		palette = calloc(dib.ncolors, sizeof *palette);		
		if(!palette) {
			goto error;
		}
		if(fread(palette, sizeof *palette, dib.ncolors, f) != dib.ncolors) {
			goto error;
		}
	}
		
	if(fseek(f, hdr.bmp_offset + start_offset, SEEK_SET) != 0) {
		goto error;
	}

	rs = ((dib.width * dib.bitspp / 8) + 3) & ~3;
	assert(rs % 4 == 0);
	
	data = malloc(rs * b->h);
	if(!data) {
		goto error;
	}
	
	if(dib.bmp_bytesz == 0) {		
		if(fread(data, 1, rs * b->h, f) != rs * b->h) {
			goto error;
		}
	} else {		
		if(fread(data, 1, dib.bmp_bytesz, f) != dib.bmp_bytesz) {
			goto error;
		}
	}
		
	if(dib.bitspp == 8) {
		for(j = 0; j < b->h; j++) {
			for(i = 0; i < b->w; i++) {
				uint8_t p = data[(b->h - (j) - 1) * rs + i];
				assert(p < dib.ncolors);				
				BM_SET(b, i, j, palette[p].r, palette[p].g, palette[p].b, palette[p].a);
			}
		}			
	} else {	
		for(j = 0; j < b->h; j++) {
			for(i = 0; i < b->w; i++) {
				int p = ((b->h - (j) - 1) * rs + (i)*3);			
				BM_SET(b, i, j, data[p+2], data[p+1], data[p+0], 0xFF);
			}
		}
	}
	
end:
	if(data) free(data);	
	if(palette != NULL) free(palette);
	
	return b;
error:
	if(b) 
		bm_free(b);
	b = NULL;
	goto end;
}

static int bm_save_bmp(Bitmap *b, const char *fname);
static int bm_save_pcx(Bitmap *b, const char *fname);
#ifdef USEPNG
static int bm_save_png(Bitmap *b, const char *fname);
#endif
#ifdef USEJPG
static int bm_save_jpg(Bitmap *b, const char *fname);
#endif

int bm_save(Bitmap *b, const char *fname) {	
	/* If the filename contains ".bmp" save as BMP,
	   if the filename contains ".jpg" save as JPG,
		otherwise save as PNG */
	char *lname = strdup(fname), *c, 
		bmp = 0, jpg = 0, png = 0, pcx = 0;
	for(c = lname; *c; c++)
		*c = tolower(*c);
	bmp = !!strstr(lname, ".bmp");
	pcx = !!strstr(lname, ".pcx");
	jpg = !!strstr(lname, ".jpg") || !!strstr(lname, ".jpeg");
	png = !bmp && !jpg && !pcx;
	free(lname);
		
#ifdef USEPNG
	if(png)
		return bm_save_png(b, fname);
#else
	(void)png;
#endif	
#ifdef USEJPG
	if(jpg)
		return bm_save_jpg(b, fname);
#else
	(void)jpg;
#endif	
	if(pcx)
		return bm_save_pcx(b, fname);
	return bm_save_bmp(b, fname);
}

static int bm_save_bmp(Bitmap *b, const char *fname) {	
	struct bmpfile_magic magic = {{'B','M'}}; 
	struct bmpfile_header hdr;
	struct bmpfile_dibinfo dib;
	FILE *f;
	
	int rs, padding, i, j;
	char *data;

	padding = 4 - ((b->w * 3) % 4);
	if(padding > 3) padding = 0;
	rs = b->w * 3 + padding;	
	assert(rs % 4 == 0);
		
	f = fopen(fname, "wb");
	if(!f) return 0;	
		
	hdr.creator1 = 0;
	hdr.creator2 = 0;
	hdr.bmp_offset = sizeof magic + sizeof hdr + sizeof dib;

	dib.header_sz = sizeof dib;
	dib.width = b->w;
	dib.height = b->h;
	dib.nplanes = 1;
	dib.bitspp = 24;
	dib.compress_type = 0;	
	dib.hres = 2835;
	dib.vres = 2835;
	dib.ncolors = 0;
	dib.nimpcolors = 0;
	
	dib.bmp_bytesz = rs * b->h;
	hdr.filesz = hdr.bmp_offset + dib.bmp_bytesz;
	
	fwrite(&magic, sizeof magic, 1, f); 
	fwrite(&hdr, sizeof hdr, 1, f);
	fwrite(&dib, sizeof dib, 1, f);
		
	data = malloc(dib.bmp_bytesz);
	if(!data) {
		fclose(f);
		return 0;
	}
	memset(data, 0x00, dib.bmp_bytesz);
	
	for(j = 0; j < b->h; j++) {
		for(i = 0; i < b->w; i++) {
			int p = ((b->h - (j) - 1) * rs + (i)*3);
			data[p+2] = BM_GETR(b, i, j);
			data[p+1] = BM_GETG(b, i, j);
			data[p+0] = BM_GETB(b, i, j);
		}
	}
	
	fwrite(data, 1, dib.bmp_bytesz, f);
	
	free(data);
	
	fclose(f);
	return 1;
}

#ifdef USEPNG
/*
http://zarb.org/~gc/html/libpng.html
http://www.labbookpages.co.uk/software/imgProc/libPNG.html
*/
static Bitmap *bm_load_png_fp(FILE *f) {
	Bitmap *bmp = NULL;
	
	unsigned char header[8];
	png_structp png = NULL;
	png_infop info = NULL;
	int number_of_passes;
	png_bytep * rows = NULL;

	int w, h, ct, bpp, x, y;

	if((fread(header, 1, 8, f) != 8) || png_sig_cmp(header, 0, 8)) {
		goto error;
	}
	
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) {
		goto error;
	}
	info = png_create_info_struct(png);
	if(!info) {	
		goto error;
	}
	if(setjmp(png_jmpbuf(png))) {
		goto error;
	}
	
	png_init_io(png, f);
	png_set_sig_bytes(png, 8);
	
	png_read_info(png, info);
	
	w = png_get_image_width(png, info);
	h = png_get_image_height(png, info);
	ct = png_get_color_type(png, info);
	
	bpp = png_get_bit_depth(png, info);
	assert(bpp == 8);(void)bpp;
	
	if(ct != PNG_COLOR_TYPE_RGB && ct != PNG_COLOR_TYPE_RGBA) {
		goto error;
	}
	
	number_of_passes = png_set_interlace_handling(png);
	(void)number_of_passes;
	
	bmp = bm_create(w,h);
	
	if(setjmp(png_jmpbuf(png))) {
		goto error;
	}
	
	rows = malloc(h * sizeof *rows);
	for(y = 0; y < h; y++)
		rows[y] = malloc(png_get_rowbytes(png,info));

	png_read_image(png, rows);
	
	/* Convert to my internal representation */
	if(ct == PNG_COLOR_TYPE_RGBA) {
		for(y = 0; y < h; y++) {
			png_byte *row = rows[y];
			for(x = 0; x < w; x++) {
				png_byte *ptr = &(row[x * 4]);
				BM_SET(bmp, x, y, ptr[0], ptr[1], ptr[2], ptr[3]);
			}
		}
	} else if(ct == PNG_COLOR_TYPE_RGB) {
		for(y = 0; y < h; y++) {
			png_byte *row = rows[y];
			for(x = 0; x < w; x++) {
				png_byte *ptr = &(row[x * 3]);
				BM_SET(bmp, x, y, ptr[0], ptr[1], ptr[2], 0xFF);
			}
		}
	}

	goto done;
error:
	if(bmp) bm_free(bmp);
	bmp = NULL;
done:
	if (info != NULL) png_free_data(png, info, PNG_FREE_ALL, -1);
	if (png != NULL) png_destroy_read_struct(&png, NULL, NULL);
	if(rows) {
		for(y = 0; y < h; y++) {
			free(rows[y]);
		}
		free(rows);
	}
	return bmp;
}

static int bm_save_png(Bitmap *b, const char *fname) {
	
	png_structp png = NULL;
	png_infop info = NULL;
	int y, rv = 1;
	
	FILE *f = fopen(fname, "wb");
	if(!f) {
		return 0;
	}
	
	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) {
		goto error;
	}
	
	info = png_create_info_struct(png);
	if(!info) {
		goto error;
	}
	
	if(setjmp(png_jmpbuf(png))) {
		goto error;
	}
	
	png_init_io(png, f);
	
	if(setjmp(png_jmpbuf(png))) {
		goto error;
	}
	
	png_set_IHDR(png, info, b->w, b->h, 8, PNG_COLOR_TYPE_RGBA,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, 
		PNG_FILTER_TYPE_BASE);
		
	png_write_info(png, info);
	
	png_bytep row = malloc(4 * b->w * sizeof *row);
	int x;
	for(y = 0; y < b->h; y++) {
		png_bytep r = row;
		for(x = 0; x < b->w; x++) {
			*r++ = BM_GETR(b,x,y);
			*r++ = BM_GETG(b,x,y);
			*r++ = BM_GETB(b,x,y);
			*r++ = BM_GETA(b,x,y);
		}
		png_write_row(png, row);
	}
	
	if(setjmp(png_jmpbuf(png))) {
		goto error;
	}
	
	goto done;
error:
	rv = 0;
done:
	if(info) png_free_data(png, info, PNG_FREE_ALL, -1);
	if(png) png_destroy_write_struct(&png, NULL);
    fclose(f);
	return rv;
}
#endif

#ifdef USEJPG
struct jpg_err_handler {
	struct jpeg_error_mgr pub;
	jmp_buf jbuf;
};

static void jpg_on_error(j_common_ptr cinfo) {
	struct jpg_err_handler *err = (struct jpg_err_handler *) cinfo->err;
	longjmp(err->jbuf, 1);
}

static Bitmap *bm_load_jpg_fp(FILE *f) {
	struct jpeg_decompress_struct cinfo;
	struct jpg_err_handler jerr;
	Bitmap *bmp = NULL;
	int i, j, row_stride;
	unsigned char *data;
	JSAMPROW row_pointer[1];
		
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpg_on_error;
	if(setjmp(jerr.jbuf)) {
		jpeg_destroy_decompress(&cinfo);
		return NULL;
	}
	jpeg_create_decompress(&cinfo);	
	jpeg_stdio_src(&cinfo, f);
	
	jpeg_read_header(&cinfo, TRUE);	
	cinfo.out_color_space = JCS_RGB;
	
	bmp = bm_create(cinfo.image_width, cinfo.image_height);
	if(!bmp) {
		return NULL;
	}
	row_stride = bmp->w * 3; 
	
	data = malloc(row_stride);
	if(!data) {
		return NULL;
	}
	memset(data, 0x00, row_stride);
	row_pointer[0] = data;
	
	jpeg_start_decompress(&cinfo);
	
	for(j = 0; j < cinfo.output_height; j++) {
		jpeg_read_scanlines(&cinfo, row_pointer, 1);		
		for(i = 0; i < bmp->w; i++) {
			unsigned char *ptr = &(data[i * 3]);
			BM_SET(bmp, i, j, ptr[0], ptr[1], ptr[2], 0xFF);
		}
	}
	free(data);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	return bmp;
}
static int bm_save_jpg(Bitmap *b, const char *fname) {
	struct jpeg_compress_struct cinfo;
	struct jpg_err_handler jerr;
	FILE *f;
	int i, j;
	JSAMPROW row_pointer[1];
	int row_stride;
	unsigned char *data;
	
	if(!(f = fopen(fname, "wb"))) {
		return 0;
	}
	
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpg_on_error;
	if(setjmp(jerr.jbuf)) {
		jpeg_destroy_compress(&cinfo);
		return 0;
	}
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, f);
	
	cinfo.image_width = b->w;
	cinfo.image_height = b->h;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	
	jpeg_set_defaults(&cinfo);
	/*jpeg_set_quality(&cinfo, 100, TRUE);*/
	
	row_stride = b->w * 3; 
	
	data = malloc(row_stride);
	if(!data) {
		fclose(f);
		return 0;
	}
	memset(data, 0x00, row_stride);
		
	jpeg_start_compress(&cinfo, TRUE);
	for(j = 0; j < b->h; j++) {
		for(i = 0; i < b->w; i++) {
			data[i*3+0] = BM_GETR(b, i, j);
			data[i*3+1] = BM_GETG(b, i, j);
			data[i*3+2] = BM_GETB(b, i, j);
		}
		row_pointer[0] = data;
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	
	free(data);
	
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	
	fclose(f);	
	return 1;
}
#endif

#ifdef USESDL
/* Some functions to load graphics through the SDL_RWops
    related functions.
*/

static Bitmap *bm_load_bmp_rw(SDL_RWops *rw);
static Bitmap *bm_load_pcx_rw(SDL_RWops *rw);
#  ifdef USEPNG
static Bitmap *bm_load_png_rw(SDL_RWops *rw);
#  endif
#  ifdef USEJPG
static Bitmap *bm_load_jpg_rw(SDL_RWops *rw);
#  endif

Bitmap *bm_load_rw(SDL_RWops *rw) {	
	unsigned char magic[2];	
    long start = SDL_RWtell(rw);
    long isbmp = 0, ispng = 0, isjpg = 0, ispcx = 0;
    if(SDL_RWread(rw, magic, sizeof magic, 1) == 1) {
		if(magic[0] == 'B' && magic[1] == 'M') 
			isbmp = 1;
		else if(magic[0] == 0xFF && magic[1] == 0xD8)
			isjpg = 1;
		else if(magic[0] == 0x0A)
			ispcx = 1;
		else
			ispng = 1; /* Assume PNG by default. 
					JPG and BMP magic numbers are simpler */
	}
	SDL_RWseek(rw, start, RW_SEEK_SET);
    
#  ifdef USEJPG
	if(isjpg)
		return bm_load_jpg_rw(rw);
#  else
	(void)isjpg;
#  endif
#  ifdef USEPNG
	if(ispng)
		return bm_load_png_rw(rw);
#  else
	(void)ispng;
#  endif
	if(ispcx)
		return bm_load_pcx_rw(rw);
	if(isbmp) 
        return bm_load_bmp_rw(rw);
    return NULL;
}

static Bitmap *bm_load_bmp_rw(SDL_RWops *rw) {
    struct bmpfile_magic magic; 
	struct bmpfile_header hdr;
	struct bmpfile_dibinfo dib;
	struct bmpfile_colinfo *palette = NULL;
	
	Bitmap *b = NULL;
	
	int rs, i, j;
	char *data = NULL;
	
	long start_offset = SDL_RWtell(rw);
		
	if(SDL_RWread(rw, &magic, sizeof magic, 1) != 1) {
		return NULL;
	}
	if(magic.magic[0] != 'B' || magic.magic[1] != 'M') {
		return NULL;
	}
	
	if(SDL_RWread(rw, &hdr, sizeof hdr, 1) != 1) {
		return NULL;
	}
	
	if(SDL_RWread(rw, &dib, sizeof dib, 1) != 1) {
		return NULL;
	}
	
	if((dib.bitspp != 8 && dib.bitspp != 24) || dib.compress_type != 0) {
		/* Unsupported BMP type. TODO (maybe): support more types? */
		return NULL;
	}
	
	b = bm_create(dib.width, dib.height);
	if(!b) {
		return NULL;
	}
	
	if(dib.bitspp <= 8) {
		if(!dib.ncolors) {
			dib.ncolors = 1 << dib.bitspp;
		}		
		palette = calloc(dib.ncolors, sizeof *palette);		
		if(!palette) {
			goto error;
		}
		if(SDL_RWread(rw, palette, sizeof *palette, dib.ncolors) != dib.ncolors) {
			goto error;
		}
	}
		
	if(SDL_RWseek(rw, hdr.bmp_offset + start_offset, RW_SEEK_SET) < 0) {
		goto error;
	}

	rs = ((dib.width * dib.bitspp / 8) + 3) & ~3;
	assert(rs % 4 == 0);
	
	data = malloc(rs * b->h);
	if(!data) {
		goto error;
	}
	
	if(dib.bmp_bytesz == 0) {		
		if(SDL_RWread(rw, data, 1, rs * b->h) != rs * b->h) {
			goto error;
		}
	} else {		
		if(SDL_RWread(rw, data, 1, dib.bmp_bytesz) != dib.bmp_bytesz) {
			goto error;
		}
	}
		
	if(dib.bitspp == 8) {
		for(j = 0; j < b->h; j++) {
			for(i = 0; i < b->w; i++) {
				uint8_t p = data[(b->h - (j) - 1) * rs + i];
				assert(p < dib.ncolors);				
				BM_SET(b, i, j, palette[p].r, palette[p].g, palette[p].b, palette[p].a);
			}
		}			
	} else {	
		for(j = 0; j < b->h; j++) {
			for(i = 0; i < b->w; i++) {
				int p = ((b->h - (j) - 1) * rs + (i)*3);			
				BM_SET(b, i, j, data[p+2], data[p+1], data[p+0], 0xFF);
			}
		}
	}
	
end:
	if(data) free(data);	
	if(palette != NULL) free(palette);
	
	return b;
error:
	if(b) 
		bm_free(b);
	b = NULL;
	goto end;
}
#  ifdef USEPNG
/* 
Code to read a PNG from a SDL_RWops
http://www.libpng.org/pub/png/libpng-1.2.5-manual.html 
http://blog.hammerian.net/2009/reading-png-images-from-memory/
*/
static void read_rwo_data(png_structp png_ptr, png_bytep data, png_size_t length) {
    SDL_RWops *rw = png_get_io_ptr(png_ptr);
    SDL_RWread(rw, data, 1, length);
}

static Bitmap *bm_load_png_rw(SDL_RWops *rw) {
    	Bitmap *bmp = NULL;
	
	unsigned char header[8];
	png_structp png = NULL;
	png_infop info = NULL;
	int number_of_passes;
	png_bytep * rows = NULL;

	int w, h, ct, bpp, x, y;

	if((SDL_RWread(rw, header, 1, 8) != 8) || png_sig_cmp(header, 0, 8)) {
		goto error;
	}
	
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png) {
		goto error;
	}
	info = png_create_info_struct(png);
	if(!info) {	
		goto error;
	}
	if(setjmp(png_jmpbuf(png))) {
		goto error;
	}
	
	png_init_io(png, NULL);
    png_set_read_fn(png, rw, read_rwo_data);
        
	png_set_sig_bytes(png, 8);
	
	png_read_info(png, info);
	
	w = png_get_image_width(png, info);
	h = png_get_image_height(png, info);
	ct = png_get_color_type(png, info);
	
	bpp = png_get_bit_depth(png, info);
	assert(bpp == 8);(void)bpp;
	
	if(ct != PNG_COLOR_TYPE_RGB && ct != PNG_COLOR_TYPE_RGBA) {
		goto error;
	}
	
	number_of_passes = png_set_interlace_handling(png);
	(void)number_of_passes;
	
	bmp = bm_create(w,h);
	
	if(setjmp(png_jmpbuf(png))) {
		goto error;
	}
	
	rows = malloc(h * sizeof *rows);
	for(y = 0; y < h; y++)
		rows[y] = malloc(png_get_rowbytes(png,info));

	png_read_image(png, rows);
	
	/* Convert to my internal representation */
	if(ct == PNG_COLOR_TYPE_RGBA) {
		for(y = 0; y < h; y++) {
			png_byte *row = rows[y];
			for(x = 0; x < w; x++) {
				png_byte *ptr = &(row[x * 4]);
				BM_SET(bmp, x, y, ptr[0], ptr[1], ptr[2], ptr[3]);
			}
		}
	} else if(ct == PNG_COLOR_TYPE_RGB) {
		for(y = 0; y < h; y++) {
			png_byte *row = rows[y];
			for(x = 0; x < w; x++) {
				png_byte *ptr = &(row[x * 3]);
				BM_SET(bmp, x, y, ptr[0], ptr[1], ptr[2], 0xFF);
			}
		}
	}

	goto done;
error:
	if(bmp) bm_free(bmp);
	bmp = NULL;
done:
	if (info != NULL) png_free_data(png, info, PNG_FREE_ALL, -1);
	if (png != NULL) png_destroy_read_struct(&png, NULL, NULL);
	if(rows) {
		for(y = 0; y < h; y++) {
			free(rows[y]);
		}
		free(rows);
	}
	return bmp;
}
#  endif /* USEPNG */
#  ifdef USEJPG
/*
Code to read a JPEG from an SDL_RWops.
Refer to jdatasrc.c in libjpeg's code.
See also 
http://www.cs.stanford.edu/~acoates/decompressJpegFromMemory.txt
*/

#define JPEG_INPUT_BUFFER_SIZE  4096
struct rw_jpeg_src_mgr {
    struct jpeg_source_mgr pub;
    SDL_RWops *rw;
    JOCTET *buffer;
    boolean start_of_file;
};

static void rw_init_source(j_decompress_ptr cinfo) {
    struct rw_jpeg_src_mgr *src = (struct rw_jpeg_src_mgr *)cinfo->src;
    src->start_of_file = TRUE;
}

static boolean rw_fill_input_buffer(j_decompress_ptr cinfo) {
    struct rw_jpeg_src_mgr *src = (struct rw_jpeg_src_mgr *)cinfo->src;
    size_t nbytes = SDL_RWread(src->rw, src->buffer, 1, JPEG_INPUT_BUFFER_SIZE);
    
    if(nbytes <= 0) {
        /*if(src->start_of_file) 
            ERREXIT(cinfo, JERR_INPUT_EMPTY);
        WARNMS(cinfo, JWRN_JPEG_EOF);*/
        src->buffer[0] = (JOCTET)0xFF;
        src->buffer[1] = (JOCTET)JPEG_EOI;
        nbytes = 2;
    }
    
    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = nbytes;
    
    src->start_of_file = TRUE;
    return TRUE;
}

static void rw_skip_input_data(j_decompress_ptr cinfo, long nbytes) {
    struct rw_jpeg_src_mgr *src = (struct rw_jpeg_src_mgr *)cinfo->src;
    if(nbytes > 0) {
        while(nbytes > src->pub.bytes_in_buffer) {
            nbytes -= src->pub.bytes_in_buffer;
            (void)(*src->pub.fill_input_buffer)(cinfo);
        }
        src->pub.next_input_byte += nbytes;
        src->pub.bytes_in_buffer -= nbytes;
    }
}

static void rw_term_source(j_decompress_ptr cinfo) {
    /* Apparently nothing to do here */
}

static void rw_set_source_mgr(j_decompress_ptr cinfo, SDL_RWops *rw) {
    struct rw_jpeg_src_mgr *src;
    if(!cinfo->src) {
        cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof *src);
        src = (struct rw_jpeg_src_mgr *)cinfo->src;
        src->buffer = (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, JPEG_INPUT_BUFFER_SIZE * sizeof(JOCTET));
    }
    
    src = (struct rw_jpeg_src_mgr *)cinfo->src;
    
    src->pub.init_source = rw_init_source;
    src->pub.fill_input_buffer = rw_fill_input_buffer;
    src->pub.skip_input_data = rw_skip_input_data;
    src->pub.term_source = rw_term_source;
    src->pub.resync_to_restart = jpeg_resync_to_restart;
    
    src->pub.bytes_in_buffer = 0;
    src->pub.next_input_byte = NULL;
        
    src->rw = rw;
}

static Bitmap *bm_load_jpg_rw(SDL_RWops *rw) {
    struct jpeg_decompress_struct cinfo;
	struct jpg_err_handler jerr;
	Bitmap *bmp = NULL;
	int i, j, row_stride;
	unsigned char *data;
	JSAMPROW row_pointer[1];
		
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpg_on_error;
	if(setjmp(jerr.jbuf)) {
		jpeg_destroy_decompress(&cinfo);
		return NULL;
	}
	jpeg_create_decompress(&cinfo);	
    
	/* jpeg_stdio_src(&cinfo, f); */
    rw_set_source_mgr(&cinfo, rw);
	
	jpeg_read_header(&cinfo, TRUE);	
	cinfo.out_color_space = JCS_RGB;
	
	bmp = bm_create(cinfo.image_width, cinfo.image_height);
	if(!bmp) {
		return NULL;
	}
	row_stride = bmp->w * 3; 
	
	data = malloc(row_stride);
	if(!data) {
		return NULL;
	}
	memset(data, 0x00, row_stride);
	row_pointer[0] = data;
	
	jpeg_start_decompress(&cinfo);
	
	for(j = 0; j < cinfo.output_height; j++) {
		jpeg_read_scanlines(&cinfo, row_pointer, 1);		
		for(i = 0; i < bmp->w; i++) {
			unsigned char *ptr = &(data[i * 3]);
			BM_SET(bmp, i, j, ptr[0], ptr[1], ptr[2], 0xFF);
		}
	}
	free(data);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	return bmp;
}
#  endif /* USEJPG */

#endif /* USESDL */

/* PCX support
http://web.archive.org/web/20100206055706/http://www.qzx.com/pc-gpe/pcx.txt
http://www.shikadi.net/moddingwiki/PCX_Format
*/
struct rbg_triplet {
	unsigned char r, g, b;
};

struct pcx_header {
	char manuf;
	char version;
	char encoding;
	char bpp;
	unsigned short xmin, ymin, xmax, ymax;
	unsigned short vert_dpi, hori_dpi;
	
	union {
		unsigned char bytes[48];
		struct rbg_triplet rgb[16];
	} palette;
	
	char reserved;
	char planes;
	unsigned short bytes_per_line;
	unsigned short paltype;
	unsigned short hscrsize, vscrsize;
	char pad[54];
};

static Bitmap *bm_load_pcx_fp(FILE *f) {
	struct pcx_header hdr;
	Bitmap *b = NULL;
	int y;
	
	struct rbg_triplet rgb[256];
		
	if(fread(&hdr, sizeof hdr, 1, f) != 1) {
		return NULL;
	}
	if(hdr.manuf != 0x0A) {
		return NULL;
	}
	/*
	printf("Version: %d\n", hdr.version);
	printf("Encoding: %d\n", hdr.encoding);
	printf("BPP: %d\n", hdr.bpp);
	printf("Window: %d %d - %d %d\n", hdr.xmin, hdr.ymin, hdr.xmax, hdr.ymax);
	printf("DPI: %d %d\n", hdr.vert_dpi, hdr.hori_dpi);
	printf("planes %d; bytes_per_plane: %d\n", hdr.planes, hdr.bytes_per_line);
	printf("paltype: %d\n", hdr.paltype);
	*/
	if(hdr.version != 5 || hdr.encoding != 1 || hdr.bpp != 8 || (hdr.planes != 1 && hdr.planes != 3)) {
		/* TODO: We might wat to support these PCX types at a later stage. */
		return NULL;
	}
	
	if(hdr.planes == 1) {
		long pos = ftell(f);
		char pbyte;
		
		fseek(f, -769, SEEK_END);
		if(fread(&pbyte, sizeof pbyte, 1, f) != 1) {
			return NULL;
		}
		if(pbyte != 12) {
			return NULL;
		}
		if(fread(&rgb, sizeof rgb[0], 256, f) != 256) {
			return NULL;
		}
		
		fseek(f, pos, SEEK_SET);
	}
	
	b = bm_create(hdr.xmax - hdr.xmin + 1, hdr.ymax - hdr.ymin + 1);
	
	for(y = 0; y < b->h; y++) {
		int p;
		for(p = 0; p < hdr.planes; p++) {
			int x = 0;
			while(x < b->w) {
				int cnt = 1;
				int i = fgetc(f);
				if(i == EOF)
					goto read_error;
				
				if((i & 0xC0) == 0xC0) {
					cnt = i & 0x3F;
					i = fgetc(f);
					if(i == EOF)
						goto read_error;
				}
				if(hdr.planes == 1) {
					int c = (rgb[i].r << 16) | (rgb[i].g << 8) | rgb[i].b; 
					while(cnt--) {
						bm_set(b, x++, y, c);
					}
				} else {
					while(cnt--) {
						int c = bm_get(b, x, y);
						switch(p) {
						case 0: c |= (i << 16); break;
						case 1: c |= (i << 8); break;
						case 2: c |= (i << 0); break;
						}
						bm_set(b, x++, y, c);
					}
				}
			}
		}
	}
	
	return b;
read_error:
	bm_free(b);
	return NULL;	
}

#ifdef USESDL
static Bitmap *bm_load_pcx_rw(SDL_RWops *rw) {
	struct pcx_header hdr;
	Bitmap *b = NULL;
	
	struct rbg_triplet rgb[256];
		
	if(SDL_RWread(rw, &hdr, sizeof hdr, 1) != 1) {
		return NULL;
	}
	if(hdr.manuf != 0x0A) {
		return NULL;
	}
	if(hdr.version != 5 || hdr.encoding != 1 || hdr.bpp != 8 || (hdr.planes != 1 && hdr.planes != 3)) {
		return NULL;
	}
	
	if(hdr.planes == 1) {
		long pos = SDL_RWtell(rw);
		
		SDL_RWseek(rw, -769, RW_SEEK_END);
		char pbyte;
		if(SDL_RWread(rw, &pbyte, sizeof pbyte, 1) != 1) {
			return NULL;
		}
		if(pbyte != 12) {
			return NULL;
		}
		if(SDL_RWread(rw, &rgb, sizeof rgb[0], 256) != 256) {
			return NULL;
		}
		
		SDL_RWseek(rw, pos, SEEK_SET);
	}
	
	b = bm_create(hdr.xmax - hdr.xmin + 1, hdr.ymax - hdr.ymin + 1);
	
	int y;
	for(y = 0; y < b->h; y++) {
		int p;
		for(p = 0; p < hdr.planes; p++) {
			int x = 0;
			while(x < b->w) {
				int cnt = 1;
				char i;
				if(SDL_RWread(rw, &i, sizeof i, 1) != 1) 
					goto read_error;
				
				if((i & 0xC0) == 0xC0) {
					cnt = i & 0x3F;
					if(SDL_RWread(rw, &i, sizeof i, 1) != 1) 
					goto read_error;
				}
				if(hdr.planes == 1) {
					int c = (rgb[(int)i].r << 16) | (rgb[(int)i].g << 8) | rgb[(int)i].b; 
					while(cnt--) {
						bm_set(b, x++, y, c);
					}
				} else {
					while(cnt--) {
						int c = bm_get(b, x, y);
						switch(p) {
						case 0: c |= (i << 16); break;
						case 1: c |= (i << 8); break;
						case 2: c |= (i << 0); break;
						}
						bm_set(b, x++, y, c);
					}
				}
			}
		}
	}
	
	return b;
read_error:
	bm_free(b);
	return NULL;	
}
#endif /* USESDL */

/* Returns the index of the color c in the PCX pallette.
	If c is not in the palette, but *nentries is less than 256 a new
	entry in the palette will be created, otherwise it will return the
	colour in the palette closest to c.
 */
static int get_pcx_pal_idx(struct rbg_triplet rgb[], int *nentries, int c) {
	int r = (c >> 16) & 0xFF;
	int g = (c >> 8) & 0xFF;
	int b = (c >> 0) & 0xFF;
	
	int i, mini = 0;
	
	double mindist = DBL_MAX;
	for(i = 0; i < *nentries; i++) {
		int dr, dg, db;
		double dist;
		
		if(rgb[i].r == r && rgb[i].g == g && rgb[i].b == b) {
			/* Found an exact match */
			return i;
		}
		/* Compute the distance between c and the current palette entry */
		dr = rgb[i].r - r;
		dg = rgb[i].g - g;
		db = rgb[i].b - b;
		dist = sqrt(dr * dr + dg * dg + db * db);
		if(dist < mindist) {
			mindist = dist;
			mini = i;
		}
	}
	
	if(*nentries < 256) {		
		(*nentries)++;
		i = *nentries;
		rgb[i].r = r;
		rgb[i].g = g;
		rgb[i].b = b;
		return i;
	}
	
	return mini;
}

static int bm_save_pcx(Bitmap *b, const char *fname) {	
	FILE *f;
	struct pcx_header hdr;
	int nentries = 0;
	struct rbg_triplet rgb[256];
	int q, y;
	
	if(!b) 
		return 0;
	
	f = fopen(fname, "wb");
	if(!f) 
		return 0;
		
	memset(&hdr, 0, sizeof hdr);
	
	hdr.manuf = 0x0A;
	hdr.version = 5;
	hdr.encoding = 1;
	hdr.bpp = 8;
	
	hdr.xmin = 0;
	hdr.ymin = 0;
	hdr.xmax = b->w - 1;
	hdr.ymax = b->h - 1;
	
	hdr.vert_dpi = b->h;
	hdr.hori_dpi = b->w;
	
	hdr.reserved = 0;
	hdr.planes = 1;
	hdr.bytes_per_line = hdr.xmax - hdr.xmin + 1;
	hdr.paltype = 1;
	hdr.hscrsize = 0;
	hdr.vscrsize = 0;
	
	memset(&rgb, 0, sizeof rgb);
	
	if(fwrite(&hdr, sizeof hdr, 1, f) != 1) {
		fclose(f);
		return 0;
	}
	
	/* This is my poor man's color quantization hack:
		Sample 128 random pixels from the bitmap and
		generate a palette from that. The remainder of
		the palette can be generated as the save goes
		on.
		Otherwise the palette is generated only from the
		first 256 unique pixels in the image, which my
		have undesirable results.
	*/
	for(q = 0; q < 128; q++) {
		int c = bm_get(b, rand()%b->w, rand()%b->h);
		get_pcx_pal_idx(rgb, &nentries, c);
	}
	
	/* FIXME: We can use Floyd–Steinberg dithering to get
	better results, but it will require some rework of
	the get_pcx_pal_idx() function. 
	See bm_reduce_palette() below. */
	for(y = 0; y < b->h; y++) {
		int x = 0;
		while(x < b->w) {
			int i, cnt = 1;
			int c = bm_get(b, x++, y);			
			while(x < b->w && cnt < 63) {
				int n = bm_get(b, x, y);
				if(c != n) 
					break;
				x++;
				cnt++;				
			}
			
			i = get_pcx_pal_idx(rgb, &nentries, c);
			if(cnt == 1 && i < 192) {
				fputc(i, f);
			} else {
				fputc(0xC0 | cnt, f);
				fputc(i, f);
			}
		}
	}
	
	fputc(12, f);	
	if(fwrite(rgb, sizeof rgb[0], 256, f) != 256) {
		goto write_error;
	}
	
	fclose(f);
	return 1;
	
write_error:
	fclose(f);
	return 0;	
}

Bitmap *bm_copy(Bitmap *b) {
	Bitmap *out = bm_create(b->w, b->h);
	memcpy(out->data, b->data, BM_BLOB_SIZE(b));
	
	out->r = b->r; out->g = b->g; out->b = b->b;
	
	/* Caveat: The input bitmap is technically the owner
	of its own font, so we can't just copy the pointer
	*/
	/* out->font = b->font */
	out->font = NULL;
	
	memcpy(&out->clip, &b->clip, sizeof b->clip);
	
	return out;
}

void bm_free(Bitmap *b) {
	if(!b) return;
	if(b->data) free(b->data);
	if(b->font && b->font->dtor)
		b->font->dtor(b->font);
	free(b);
}

Bitmap *bm_bind(int w, int h, unsigned char *data) {	
	Bitmap *b = malloc(sizeof *b);
	
	b->w = w;
	b->h = h;
	
	b->clip.x0 = 0;
	b->clip.y0 = 0;
	b->clip.x1 = w;
	b->clip.y1 = h;
		
	b->data = data;
	
	b->font = NULL;
#ifndef NO_FONTS
	bm_std_font(b, BM_FONT_NORMAL);
#endif

	bm_set_color(b, 255, 255, 255);
	bm_set_alpha(b, 255);
	
	return b;
}

void bm_rebind(Bitmap *b, unsigned char *data) {
    b->data = data;
}

void bm_unbind(Bitmap *b) {
	if(!b) return;
	if(b->font && b->font->dtor)
		b->font->dtor(b->font);
	free(b);
}

void bm_flip_vertical(Bitmap *b) {
	int y;
	size_t s = BM_ROW_SIZE(b);
	unsigned char *trow = malloc(s);
	for(y = 0; y < b->h/2; y++) {
		unsigned char *row1 = &b->data[y * s];
		unsigned char *row2 = &b->data[(b->h - y - 1) * s];
		memcpy(trow, row1, s);
		memcpy(row1, row2, s);
		memcpy(row2, trow, s);
	}
	free(trow);
}

int bm_get(Bitmap *b, int x, int y) {	
	int *p;
	assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
	p = (int*)(b->data + y * BM_ROW_SIZE(b) + x * BM_BPP);
	return *p;
}

void bm_set(Bitmap *b, int x, int y, int c) {	
	int *p;
	assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
	p = (int*)(b->data + y * BM_ROW_SIZE(b) + x * BM_BPP);
	*p = c;
}

void bm_set_rgb(Bitmap *b, int x, int y, unsigned char R, unsigned char G, unsigned char B) {
	assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
	BM_SET(b, x, y, R, G, B, b->a);
}

void bm_set_rgb_a(Bitmap *b, int x, int y, unsigned char R, unsigned char G, unsigned char B, unsigned char A) {
	assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
	BM_SET(b, x, y, R, G, B, A);
}

unsigned char bm_getr(Bitmap *b, int x, int y) {
	return BM_GETR(b,x,y);
}

unsigned char bm_getg(Bitmap *b, int x, int y) {
	return BM_GETG(b,x,y);
}

unsigned char bm_getb(Bitmap *b, int x, int y) {
	return BM_GETB(b,x,y);
}

unsigned char bm_geta(Bitmap *b, int x, int y) {
	return BM_GETA(b,x,y);
}

Bitmap *bm_fromXbm(int w, int h, unsigned char *data) {
	int x,y;
		
	Bitmap *bmp = bm_create(w, h);
	
	int byte = 0;	
	for(y = 0; y < h; y++)
		for(x = 0; x < w;) {			
			int i, b;
			b = data[byte++];
			for(i = 0; i < 8 && x < w; i++) {
				unsigned char c = (b & (1 << i))?0x00:0xFF;
				BM_SET(bmp, x++, y, c, c, c, c);
			}
		}
	return bmp;
}

void bm_clip(Bitmap *b, int x0, int y0, int x1, int y1) {
	if(x0 > x1) {
		int t = x1;
		x1 = x0;
		x0 = t;
	}
	if(y0 > y1) {
		int t = y1;
		y1 = y0;
		y0 = t;
	}
	if(x0 < 0) x0 = 0;
	if(x1 > b->w) x1 = b->w;
	if(y0 < 0) y0 = 0;
	if(y1 > b->h) y1 = b->h;
	
	b->clip.x0 = x0;
	b->clip.y0 = y0;
	b->clip.x1 = x1;
	b->clip.y1 = y1;
}

void bm_unclip(Bitmap *b) {
	b->clip.x0 = 0;
	b->clip.y0 = 0;
	b->clip.x1 = b->w;
	b->clip.y1 = b->h;
}

void bm_blit(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, int w, int h) {
	int x,y, i, j;
	
	if(sx < 0) {
		int delta = -sx;
		sx = 0;
		dx += delta;
		w -= delta;
	}
	
	if(dx < dst->clip.x0) {
		int delta = dst->clip.x0 - dx;
		sx += delta;
		w -= delta;
		dx = dst->clip.x0;
	}

	if(sx + w > src->w) {
		int delta = sx + w - src->w;
		w -= delta;
	}
	
	if(dx + w > dst->clip.x1) {
		int delta = dx + w - dst->clip.x1;
		w -= delta;
	}

	if(sy < 0) {
		int delta = -sy;
		sy = 0;
		dy += delta;
		h -= delta;
	}
	
	if(dy < dst->clip.y0) {
		int delta = dst->clip.y0 - dy;
		sy += delta;
		h -= delta;
		dy = dst->clip.y0;
	}
	
	if(sy + h > src->h) {
		int delta = sy + h - src->h;
		h -= delta;
	}
	
	if(dy + h > dst->clip.y1) {
		int delta = dy + h - dst->clip.y1;
		h -= delta;
	}
	
	if(w <= 0 || h <= 0)
		return;
	if(dx >= dst->clip.x1 || dx + w < dst->clip.x0)
		return;
	if(dy >= dst->clip.y1 || dy + h < dst->clip.y0)
		return;
	if(sx >= src->w || sx + w < 0)
		return;
	if(sy >= src->h || sy + h < 0)
		return;
	
	if(sx + w > src->w) {
		int delta = sx + w - src->w;
		w -= delta;
	}
	
	if(sy + h > src->h) {
		int delta = sy + h - src->h;
		h -= delta;
	}
	
	assert(dx >= 0 && dx + w <= dst->clip.x1);
	assert(dy >= 0 && dy + h <= dst->clip.y1);	
	assert(sx >= 0 && sx + w <= src->w);
	assert(sy >= 0 && sy + h <= src->h);
	
	j = sy;
	for(y = dy; y < dy + h; y++) {		
		i = sx;
		for(x = dx; x < dx + w; x++) {
			int r = BM_GETR(src, i, j),
				g = BM_GETG(src, i, j),
				b = BM_GETB(src, i, j),
				a = BM_GETA(src, i, j);
			BM_SET(dst, x, y, r, g, b, a);
			i++;
		}
		j++;
	}
}

void bm_maskedblit(Bitmap *dst, int dx, int dy, Bitmap *src, int sx, int sy, int w, int h) {
	int x,y, i, j;

	if(sx < 0) {
		int delta = -sx;
		sx = 0;
		dx += delta;
		w -= delta;
	}
	
	if(dx < dst->clip.x0) {
		int delta = dst->clip.x0 - dx;
		sx += delta;
		w -= delta;
		dx = dst->clip.x0;
	}

	if(sx + w > src->w) {
		int delta = sx + w - src->w;
		w -= delta;
	}
	
	if(dx + w > dst->clip.x1) {
		int delta = dx + w - dst->clip.x1;
		w -= delta;
	}

	if(sy < 0) {
		int delta = -sy;
		sy = 0;
		dy += delta;
		h -= delta;
	}
	
	if(dy < dst->clip.y0) {
		int delta = dst->clip.y0 - dy;
		sy += delta;
		h -= delta;
		dy = dst->clip.y0;
	}
	
	if(sy + h > src->h) {
		int delta = sy + h - src->h;
		h -= delta;
	}
	
	if(dy + h > dst->clip.y1) {
		int delta = dy + h - dst->clip.y1;
		h -= delta;
	}
	
	if(w <= 0 || h <= 0)
		return;
	if(dx >= dst->clip.x1 || dx + w < dst->clip.x0)
		return;
	if(dy >= dst->clip.y1 || dy + h < dst->clip.y0)
		return;
	if(sx >= src->w || sx + w < 0)
		return;
	if(sy >= src->h || sy + h < 0)
		return;
	
	if(sx + w > src->w) {
		int delta = sx + w - src->w;
		w -= delta;
	}
	
	if(sy + h > src->h) {
		int delta = sy + h - src->h;
		h -= delta;
	}
	
	assert(dx >= 0 && dx + w <= dst->clip.x1);
	assert(dy >= 0 && dy + h <= dst->clip.y1);	
	assert(sx >= 0 && sx + w <= src->w);
	assert(sy >= 0 && sy + h <= src->h);
	
	j = sy;
	for(y = dy; y < dy + h; y++) {		
		i = sx;
		for(x = dx; x < dx + w; x++) {
			int r = BM_GETR(src, i, j),
				g = BM_GETG(src, i, j),
				b = BM_GETB(src, i, j),
				a = BM_GETA(src, i, j);
			if(r != src->r || g != src->g || b != src->b)
				BM_SET(dst, x, y, r, g, b, a);
			i++;
		}
		j++;
	}
}

void bm_blit_ex(Bitmap *dst, int dx, int dy, int dw, int dh, Bitmap *src, int sx, int sy, int sw, int sh, int mask) {
	int x, y, ssx, sdx;
	int ynum = 0;	
	int xnum = 0;
	
	/*
	Uses Bresenham's algoritm to implement a simple scaling while blitting.
	See the article "Scaling Bitmaps with Bresenham" by Tim Kientzle in the
	October 1995 issue of C/C++ Users Journal
	
	Or see these links:
		http://www.drdobbs.com/image-scaling-with-bresenham/184405045
		http://www.compuphase.com/graphic/scale.htm	
	*/
	
	if(sw == dw && sh == dh) {
		/* Special cases, no scaling */
		if(mask) {
			bm_maskedblit(dst, dx, dy, src, sx, sy, dw, dh);
		} else {
			bm_blit(dst, dx, dy, src, sx, sy, dw, dh);
		}
		return;
	}
	
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;
	
	/* Clip on the Y */
	y = dy;
	while(y < dst->clip.y0 || sy < 0) {
		ynum += sh;
		while(ynum > dh) {
			ynum -= dh;
			sy++;
		}
		y++;
	}
	
	if(dy >= dst->clip.y1 || dy + dh < dst->clip.y0 || sy >= src->h)
		return;
	
	/* Clip on the X */
	x = dx;
	while(x < dst->clip.x0 || sx < 0) {
		xnum += sw;
		while(xnum > dw) {
			xnum -= dw;
			sx++;
		}
		x++;
	}
	
	if(dx >= dst->clip.x1 || dx + dw < dst->clip.x0 || sx >= src->w)
		return;
	
	ssx = sx; /* Save sx for the next row */
	sdx = x;
	for(; y < dy + dh; y++){		
		if(sy >= src->h || y >= dst->clip.y1)
			break;
		xnum = 0;			
		sx = ssx;
		
		assert(y >= dst->clip.y0 && sy >= 0);
		for(x = sdx; x < dx + dw; x++) {	
			int r, g, b, a;
			if(sx >= src->w || x >= dst->clip.x1)
				break;
			assert(x >= dst->clip.x0 && sx >= 0);
			
			r = BM_GETR(src, sx, sy);
			g = BM_GETG(src, sx, sy);
			b = BM_GETB(src, sx, sy);
				a = BM_GETA(src, sx, sy);
			if(!mask || r != src->r || g != src->g || b != src->b)
				BM_SET(dst, x, y, r, g, b, a);
					
			xnum += sw;
			while(xnum > dw) {
				xnum -= dw;
				sx++;
			}
		}
		ynum += sh;
		while(ynum > dh) {
			ynum -= dh;
			sy++;
		}
	}
}

void bm_smooth(Bitmap *b) {
	Bitmap *tmp = bm_create(b->w, b->h);
	unsigned char *t = b->data;
	int x, y;
	
	/* http://prideout.net/archive/bloom/ */
	int kernel[] = {1,4,6,4,1};
	
	assert(b->clip.y0 < b->clip.y1);
	assert(b->clip.x0 < b->clip.x1);
	
	for(y = 0; y < b->h; y++)
		for(x = 0; x < b->w; x++) {			
			int p, k, c = 0;
			float R = 0, G = 0, B = 0, A = 0;
			for(p = x-2, k = 0; p < x+2; p++, k++) {
				if(p < 0 || p >= b->w)
					continue;
				R += kernel[k] * BM_GETR(b,p,y);
				G += kernel[k] * BM_GETG(b,p,y);
				B += kernel[k] * BM_GETB(b,p,y);
				A += kernel[k] * BM_GETA(b,p,y);
				c += kernel[k];
			}
			BM_SET(tmp, x, y, R/c, G/c, B/c, A/c);
		}
	
	for(y = 0; y < b->h; y++)
		for(x = 0; x < b->w; x++) {			
			int p, k, c = 0;
			float R = 0, G = 0, B = 0, A = 0;
			for(p = y-2, k = 0; p < y+2; p++, k++) {
				if(p < 0 || p >= b->h)
					continue;
				R += kernel[k] * BM_GETR(tmp,x,p);
				G += kernel[k] * BM_GETG(tmp,x,p);
				B += kernel[k] * BM_GETB(tmp,x,p);
				A += kernel[k] * BM_GETA(tmp,x,p);
				c += kernel[k];
			}
			BM_SET(tmp, x, y, R/c, G/c, B/c, A/c);
		}
	
	b->data = tmp->data;
	tmp->data = t;
	bm_free(tmp);
}

void bm_apply_kernel(Bitmap *b, int dim, float kernel[]) {
	Bitmap *tmp = bm_create(b->w, b->h);
	unsigned char *t = b->data;
	int x, y;	
	int kf = dim >> 1;
	
	assert(b->clip.y0 < b->clip.y1);
	assert(b->clip.x0 < b->clip.x1);
	
	for(y = 0; y < b->h; y++) {
		for(x = 0; x < b->w; x++) {			
			int p, q, u, v;
			float R = 0, G = 0, B = 0, A = 0, c = 0;
			for(p = x-kf, u = 0; p <= x+kf; p++, u++) {
				if(p < 0 || p >= b->w)
					continue;
				for(q = y-kf, v = 0; q <= y+kf; q++, v++) {
					if(q < 0 || q >= b->h)
						continue;				
					R += kernel[u + v * dim] * BM_GETR(b,p,q);
					G += kernel[u + v * dim] * BM_GETG(b,p,q);
					B += kernel[u + v * dim] * BM_GETB(b,p,q);
					A += kernel[u + v * dim] * BM_GETA(b,p,q);
					c += kernel[u + v * dim];
				}
			}
			R /= c; if(R > 255) R = 255;if(R < 0) R = 0;
			G /= c; if(G > 255) G = 255;if(G < 0) G = 0;
			B /= c; if(B > 255) B = 255;if(B < 0) B = 0;
			A /= c; if(A > 255) A = 255;if(A < 0) A = 0;
			BM_SET(tmp, x, y, R, G, B, A);
		}
	}
	
	b->data = tmp->data;
	tmp->data = t;
	bm_free(tmp);
}

void bm_swap_colour(Bitmap *b, unsigned char sR, unsigned char sG, unsigned char sB, unsigned char dR, unsigned char dG, unsigned char dB) {
	int x,y;
	for(y = 0; y < b->h; y++)
		for(x = 0; x < b->w; x++) {			
			if(BM_GETR(b,x,y) == sR && BM_GETG(b,x,y) == sG && BM_GETB(b,x,y) == sB) {
				int a = BM_GETA(b, x, y);
				BM_SET(b, x, y, dR, dG, dB, a);
			}
		}
}

/*
Image scaling functions: 
 - bm_resample() : Uses the nearest neighbour
 - bm_resample_blin() : Uses bilinear interpolation.
 - bm_resample_bcub() : Uses bicubic interpolation.
Bilinear Interpolation is better suited for making an image larger.
Bicubic Interpolation is better suited for making an image smaller.
http://blog.codinghorror.com/better-image-resizing/

*/
Bitmap *bm_resample(const Bitmap *in, int nw, int nh) {
	Bitmap *out = bm_create(nw, nh);
	int x, y;
	for(y = 0; y < nh; y++)
		for(x = 0; x < nw; x++) {
			int sx = x * in->w/nw;
			int sy = y * in->h/nh;
			assert(sx < in->w && sy < in->h);
			BM_SET(out, x, y, BM_GETR(in,sx,sy), BM_GETG(in,sx,sy), BM_GETB(in,sx,sy), BM_GETA(in,sx,sy));
		}
	return out;
}

/* http://rosettacode.org/wiki/Bilinear_interpolation */
static double lerp(double s, double e, double t) {
    return s + (e-s)*t;
}
static double blerp(double c00, double c10, double c01, double c11, double tx, double ty) {
    return lerp(
        lerp(c00, c10, tx),
        lerp(c01, c11, tx),
        ty);
}

Bitmap *bm_resample_blin(const Bitmap *in, int nw, int nh) {
	Bitmap *out = bm_create(nw, nh);
	int x, y;
	for(y = 0; y < nh; y++)
		for(x = 0; x < nw; x++) {
            int C[4], c;
            double gx = (double)x * in->w/(double)nw;
			int sx = (int)gx;
			double gy = (double)y * in->h/(double)nh;
			int sy = (int)gy;
            int dx = 1, dy = 1;            
			assert(sx < in->w && sy < in->h);
            if(sx + 1 >= in->w){ sx=in->w-1; dx = 0; }
            if(sy + 1 >= in->h){ sy=in->h-1; dy = 0; }
            for(c = 0; c < 4; c++) {
                int p00 = BM_GETN(in,c,sx,sy);
                int p10 = BM_GETN(in,c,sx+dx,sy);
                int p01 = BM_GETN(in,c,sx,sy+dy);
                int p11 = BM_GETN(in,c,sx+dx,sy+dy);
                C[c] = (int)blerp(p00, p10, p01, p11, gx-sx, gy-sy);
            }                         
			BM_SET(out, x, y, C[0], C[1], C[2], C[3]);
		}
	return out;
}

/*
http://www.codeproject.com/Articles/236394/Bi-Cubic-and-Bi-Linear-Interpolation-with-GLSL
except I ported the GLSL code to straight C
*/
static double triangular_fun(double b) {
    b = b * 1.5 / 2.0;
    if( -1.0 < b && b <= 0.0) {
        return b + 1.0;
    } else if(0.0 < b && b <= 1.0) {
        return 1.0 - b;
    } 
    return 0;
}

Bitmap *bm_resample_bcub(const Bitmap *in, int nw, int nh) {
	Bitmap *out = bm_create(nw, nh);
	int x, y;
    
	for(y = 0; y < nh; y++)
    for(x = 0; x < nw; x++) {

        double sum[4] = {0.0, 0.0, 0.0, 0.0}; 
        double denom[4] = {0.0, 0.0, 0.0, 0.0}; 
        
        double a = (double)x * in->w/(double)nw;
        int sx = (int)a;
        double b = (double)y * in->h/(double)nh;
        int sy = (int)b;     
        
        int m, n, c, C;
        for(m = -1; m < 3; m++ )
        for(n = -1; n < 3; n++) {
            double f = triangular_fun((double)sx - a);
            double f1 = triangular_fun(-((double)sy - b));
            for(c = 0; c < 4; c++) {
                int i = sx+m;
                int j = sy+n;
                if(i < 0) i = 0;
                if(i >= in->w) i = in->w - 1;
                if(j < 0) j = 0;
                if(j >= in->h) j = in->h - 1;
                C = BM_GETN(in, c, i, j);
                sum[c] = sum[c] + C * f1 * f;
                denom[c] = denom[c] + f1 * f;
            }
        }
        
        BM_SET(out, x, y, sum[0]/denom[0], sum[1]/denom[1], sum[2]/denom[2], sum[3]/denom[3]);            
    }
	return out;
}

void bm_set_color(Bitmap *bm, unsigned char r, unsigned char g, unsigned char b) {
	bm->r = r;
	bm->g = g;
	bm->b = b;
}

void bm_set_alpha(Bitmap *bm, int a) {
	if(a < 0) a = 0;
	if(a > 255) a = 255;
	bm->a = a;
}

void bm_adjust_rgba(Bitmap *bm, float rf, float gf, float bf, float af) {
	int x, y;
	for(y = 0; y < bm->h; y++)
		for(x = 0; x < bm->w; x++) {
			float R = BM_GETR(bm,x,y);
			float G = BM_GETG(bm,x,y);
			float B = BM_GETB(bm,x,y);
			float A = BM_GETA(bm,x,y);
			BM_SET(bm, x, y, rf * R, gf * G, bf * B, af * A);
		}
}

/* Lookup table for bm_color_atoi() 
 * This list is based on the HTML and X11 colors on the
 * Wikipedia's list of web colors:
 * http://en.wikipedia.org/wiki/Web_colors
 * I also felt a bit nostalgic for the EGA graphics from my earliest
 * computer memories, so I added the EGA colors (prefixed with "EGA") from here:
 * http://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter
 *
 * Keep the list sorted because a binary search is used.
 *
 * bm_color_atoi()'s text parameter is not case sensitive and spaces are 
 * ignored, so for example "darkred" and "Dark Red" are equivalent.
 */
static const struct color_map_entry {
	const char *name;
	int color;
} color_map[] = {
	{"ALICEBLUE", 0xF0F8FF},
	{"ANTIQUEWHITE", 0xFAEBD7},
	{"AQUA", 0x00FFFF},
	{"AQUAMARINE", 0x7FFFD4},
	{"AZURE", 0xF0FFFF},
	{"BEIGE", 0xF5F5DC},
	{"BISQUE", 0xFFE4C4},
	{"BLACK", 0x000000},
	{"BLANCHEDALMOND", 0xFFEBCD},
	{"BLUE", 0x0000FF},
	{"BLUEVIOLET", 0x8A2BE2},
	{"BROWN", 0xA52A2A},
	{"BURLYWOOD", 0xDEB887},
	{"CADETBLUE", 0x5F9EA0},
	{"CHARTREUSE", 0x7FFF00},
	{"CHOCOLATE", 0xD2691E},
	{"CORAL", 0xFF7F50},
	{"CORNFLOWERBLUE", 0x6495ED},
	{"CORNSILK", 0xFFF8DC},
	{"CRIMSON", 0xDC143C},
	{"CYAN", 0x00FFFF},
	{"DARKBLUE", 0x00008B},
	{"DARKCYAN", 0x008B8B},
	{"DARKGOLDENROD", 0xB8860B},
	{"DARKGRAY", 0xA9A9A9},
	{"DARKGREEN", 0x006400},
	{"DARKKHAKI", 0xBDB76B},
	{"DARKMAGENTA", 0x8B008B},
	{"DARKOLIVEGREEN", 0x556B2F},
	{"DARKORANGE", 0xFF8C00},
	{"DARKORCHID", 0x9932CC},
	{"DARKRED", 0x8B0000},
	{"DARKSALMON", 0xE9967A},
	{"DARKSEAGREEN", 0x8FBC8F},
	{"DARKSLATEBLUE", 0x483D8B},
	{"DARKSLATEGRAY", 0x2F4F4F},
	{"DARKTURQUOISE", 0x00CED1},
	{"DARKVIOLET", 0x9400D3},
	{"DEEPPINK", 0xFF1493},
	{"DEEPSKYBLUE", 0x00BFFF},
	{"DIMGRAY", 0x696969},
	{"DODGERBLUE", 0x1E90FF},
	{"EGABLACK", 0x000000},
	{"EGABLUE", 0x0000AA},
	{"EGABRIGHTBLACK", 0x555555},
	{"EGABRIGHTBLUE", 0x5555FF},
	{"EGABRIGHTCYAN", 0x55FFFF},
	{"EGABRIGHTGREEN", 0x55FF55},
	{"EGABRIGHTMAGENTA", 0xFF55FF},
	{"EGABRIGHTRED", 0xFF5555},
	{"EGABRIGHTWHITE", 0xFFFFFF},
	{"EGABRIGHTYELLOW", 0xFFFF55},
	{"EGABROWN", 0xAA5500},
	{"EGACYAN", 0x00AAAA},
	{"EGADARKGRAY", 0x555555},
	{"EGAGREEN", 0x00AA00},
	{"EGALIGHTGRAY", 0xAAAAAA},
	{"EGAMAGENTA", 0xAA00AA},
	{"EGARED", 0xAA0000},
	{"EGAWHITE", 0xAAAAAA},
	{"FIREBRICK", 0xB22222},
	{"FLORALWHITE", 0xFFFAF0},
	{"FORESTGREEN", 0x228B22},
	{"FUCHSIA", 0xFF00FF},
	{"GAINSBORO", 0xDCDCDC},
	{"GHOSTWHITE", 0xF8F8FF},
	{"GOLD", 0xFFD700},
	{"GOLDENROD", 0xDAA520},
	{"GRAY", 0x808080},
	{"GREEN", 0x008000},
	{"GREENYELLOW", 0xADFF2F},
	{"HONEYDEW", 0xF0FFF0},
	{"HOTPINK", 0xFF69B4},
	{"INDIANRED", 0xCD5C5C},
	{"INDIGO", 0x4B0082},
	{"IVORY", 0xFFFFF0},
	{"KHAKI", 0xF0E68C},
	{"LAVENDER", 0xE6E6FA},
	{"LAVENDERBLUSH", 0xFFF0F5},
	{"LAWNGREEN", 0x7CFC00},
	{"LEMONCHIFFON", 0xFFFACD},
	{"LIGHTBLUE", 0xADD8E6},
	{"LIGHTCORAL", 0xF08080},
	{"LIGHTCYAN", 0xE0FFFF},
	{"LIGHTGOLDENRODYELLOW", 0xFAFAD2},
	{"LIGHTGRAY", 0xD3D3D3},
	{"LIGHTGREEN", 0x90EE90},
	{"LIGHTPINK", 0xFFB6C1},
	{"LIGHTSALMON", 0xFFA07A},
	{"LIGHTSEAGREEN", 0x20B2AA},
	{"LIGHTSKYBLUE", 0x87CEFA},
	{"LIGHTSLATEGRAY", 0x778899},
	{"LIGHTSTEELBLUE", 0xB0C4DE},
	{"LIGHTYELLOW", 0xFFFFE0},
	{"LIME", 0x00FF00},
	{"LIMEGREEN", 0x32CD32},
	{"LINEN", 0xFAF0E6},
	{"MAGENTA", 0xFF00FF},
	{"MAROON", 0x800000},
	{"MEDIUMAQUAMARINE", 0x66CDAA},
	{"MEDIUMBLUE", 0x0000CD},
	{"MEDIUMORCHID", 0xBA55D3},
	{"MEDIUMPURPLE", 0x9370DB},
	{"MEDIUMSEAGREEN", 0x3CB371},
	{"MEDIUMSLATEBLUE", 0x7B68EE},
	{"MEDIUMSPRINGGREEN", 0x00FA9A},
	{"MEDIUMTURQUOISE", 0x48D1CC},
	{"MEDIUMVIOLETRED", 0xC71585},
	{"MIDNIGHTBLUE", 0x191970},
	{"MINTCREAM", 0xF5FFFA},
	{"MISTYROSE", 0xFFE4E1},
	{"MOCCASIN", 0xFFE4B5},
	{"NAVAJOWHITE", 0xFFDEAD},
	{"NAVY", 0x000080},
	{"OLDLACE", 0xFDF5E6},
	{"OLIVE", 0x808000},
	{"OLIVEDRAB", 0x6B8E23},
	{"ORANGE", 0xFFA500},
	{"ORANGERED", 0xFF4500},
	{"ORCHID", 0xDA70D6},
	{"PALEGOLDENROD", 0xEEE8AA},
	{"PALEGREEN", 0x98FB98},
	{"PALETURQUOISE", 0xAFEEEE},
	{"PALEVIOLETRED", 0xDB7093},
	{"PAPAYAWHIP", 0xFFEFD5},
	{"PEACHPUFF", 0xFFDAB9},
	{"PERU", 0xCD853F},
	{"PINK", 0xFFC0CB},
	{"PLUM", 0xDDA0DD},
	{"POWDERBLUE", 0xB0E0E6},
	{"PURPLE", 0x800080},
	{"RED", 0xFF0000},
	{"ROSYBROWN", 0xBC8F8F},
	{"ROYALBLUE", 0x4169E1},
	{"SADDLEBROWN", 0x8B4513},
	{"SALMON", 0xFA8072},
	{"SANDYBROWN", 0xF4A460},
	{"SEAGREEN", 0x2E8B57},
	{"SEASHELL", 0xFFF5EE},
	{"SIENNA", 0xA0522D},
	{"SILVER", 0xC0C0C0},
	{"SKYBLUE", 0x87CEEB},
	{"SLATEBLUE", 0x6A5ACD},
	{"SLATEGRAY", 0x708090},
	{"SNOW", 0xFFFAFA},
	{"SPRINGGREEN", 0x00FF7F},
	{"STEELBLUE", 0x4682B4},
	{"TAN", 0xD2B48C},
	{"TEAL", 0x008080},
	{"THISTLE", 0xD8BFD8},
	{"TOMATO", 0xFF6347},
	{"TURQUOISE", 0x40E0D0},
	{"VIOLET", 0xEE82EE},
	{"WHEAT", 0xF5DEB3},
	{"WHITE", 0xFFFFFF},
	{"WHITESMOKE", 0xF5F5F5},
	{"YELLOW", 0xFFFF00},
	{"YELLOWGREEN", 0x9ACD32},
	{NULL, 0}
};

int bm_color_atoi(const char *text) {	
	int col = 0;
	
    if(!text) return 0;
    
	while(isspace(text[0]))
		text++;
	
	if(tolower(text[0]) == 'r' && tolower(text[1]) == 'g' && tolower(text[2]) == 'b') {
		/* Color is given like RGB(r,g,b) */
		int v,i;
		text += 3;
		if(text[0] != '(') return 0;
		text++;
		
		for(i = 0; i < 3; i++) {
			v = 0;
			while(isspace(text[0]))
				text++;
			while(isdigit(text[0])) {
				v = v * 10 + (text[0] - '0');
				text++;
			}
			while(isspace(text[0]))
				text++;
			if(text[0] != ",,)"[i]) return 0;
			text++;
			col = (col << 8) + v;
		}		
		return col;
	} else if(isalpha(text[0])) {
		const char *q, *p;
		
		int min = 0, max = ((sizeof color_map)/(sizeof color_map[0])) - 1;
		while(min <= max) {
			int i = (max + min) >> 1, r;
			
			p = text;
			q = color_map[i].name;
			
			/* Hacky case insensitive strcmp() that ignores spaces in p */
			while(*p) {
				if(*p == ' ') p++;
				else {
					if(tolower(*p) != tolower(*q))
						break; 
					p++; q++;
				}
			}			
			r = tolower(*p) - tolower(*q);
	
			if(r == 0) 
				return color_map[i].color;
			else if(r < 0) { 
				max = i - 1;
			} else {
				min = i + 1;
			}
		}
		/* Drop through: You may be dealing with a colour like 'a6664c' */
	} else if(text[0] == '#') {
		text++;		
		if(strlen(text) == 3) {
			/* Special case of #RGB that should be treated as #RRGGBB */
			while(text[0]) {
				int c = tolower(text[0]);
				if(c >= 'a' && c <= 'f') {
					col = (col << 4) + (c - 'a' + 10); 
					col = (col << 4) + (c - 'a' + 10); 
				} else {
					col = (col << 4) + (c - '0');
					col = (col << 4) + (c - '0');
				}		
				text++;
			}
			return col;
		}
	} else if(text[0] == '0' && tolower(text[1]) == 'x') {
		text += 2;
	} else if(tolower(text[0]) == 'h' && tolower(text[1]) == 's' && tolower(text[2]) == 'l') {
		/* Not supported yet.
		http://en.wikipedia.org/wiki/HSL_color_space 
		*/
		return 0;
	}
		
	while(isxdigit(text[0])) {
		int c = tolower(text[0]);
		if(c >= 'a' && c <= 'f') {
			col = (col << 4) + (c - 'a' + 10); 
		} else {
			col = (col << 4) + (c - '0');
		}		
		text++;
	}	
	return col;
}

void bm_set_color_s(Bitmap *bm, const char *text) {	
	bm_set_color_i(bm, bm_color_atoi(text));
}

void bm_set_color_i(Bitmap *bm, int col) {	
	bm_set_color(bm, (col >> 16) & 0xFF, (col >> 8) & 0xFF, (col >> 0) & 0xFF);
}

void bm_get_color(Bitmap *bm, int *r, int *g, int *b) {
	*r = bm->r;
	*g = bm->g;
	*b = bm->b;
}

int bm_get_color_i(Bitmap *bm) {
	return (bm->a << 24) | (bm->r << 16) | (bm->g << 8) | (bm->b << 0);
}

int bm_picker(Bitmap *bm, int x, int y) {
	int c;
	if(x < 0 || x >= bm->w || y < 0 || y >= bm->h) 
		return 0;
	c = bm_get(bm, x, y);
	bm->r = (c >> 16) & 0xFF;
	bm->g = (c >> 8) & 0xFF;
	bm->b = (c >> 0) & 0xFF;
	bm->a = (c >> 24) & 0xFF;
    return c;
}

int bm_color_is(Bitmap *bm, int x, int y, int r, int g, int b) {
	return BM_GETR(bm,x,y) == r && BM_GETG(bm,x,y) == g && BM_GETB(bm,x,y) == b;
}

double bm_cdist(int color1, int color2) {
	int r1, g1, b1;
	int r2, g2, b2;
	int dr, dg, db;
	r1 = (color1 >> 16) & 0xFF; g1 = (color1 >> 8) & 0xFF; b1 = (color1 >> 0) & 0xFF;
	r2 = (color2 >> 16) & 0xFF; g2 = (color2 >> 8) & 0xFF; b2 = (color2 >> 0) & 0xFF;
	dr = r1 - r2;
	dg = g1 - g2;
	db = b1 - b2;
	return sqrt(dr * dr + dg * dg + db * db);
}

int bm_lerp(int color1, int color2, double t) {
	int r1, g1, b1;
	int r2, g2, b2;
	int r3, g3, b3;
	
	if(t <= 0.0) return color1;
	if(t >= 1.0) return color2;
	
	r1 = (color1 >> 16) & 0xFF; g1 = (color1 >> 8) & 0xFF; b1 = (color1 >> 0) & 0xFF;
	r2 = (color2 >> 16) & 0xFF; g2 = (color2 >> 8) & 0xFF; b2 = (color2 >> 0) & 0xFF;
	
	r3 = (r2 - r1) * t + r1;
	g3 = (g2 - g1) * t + g1;
	b3 = (b2 - b1) * t + b1;
	
	return (r3 << 16) | (g3 << 8) | (b3 << 0);
}

int bm_brightness(int color, double adj) {
	int r, g, b;
	if(adj < 0.0) return 0;
	
	r = (color >> 16) & 0xFF; 
	g = (color >> 8) & 0xFF; 
	b = (color >> 0) & 0xFF;
	
	r = (int)((double)r * adj);
	if(r > 0xFF) r = 0xFF;
	
	g = (int)((double)g * adj);
	if(g > 0xFF) g = 0xFF;
	
	b = (int)((double)b * adj);
	if(b > 0xFF) b = 0xFF;
	
	return (r << 16) | (g << 8) | (b << 0);
}

int bm_width(Bitmap *b) {
	return b->w;
}

int bm_height(Bitmap *b) {
	return b->h;
}

void bm_clear(Bitmap *b) {
	int i, j;
	for(j = 0; j < b->h; j++) 
		for(i = 0; i < b->w; i++) {
			BM_SET(b, i, j, b->r, b->g, b->b, b->a);
		}
}

void bm_putpixel(Bitmap *b, int x, int y) {
	if(x < b->clip.x0 || x >= b->clip.x1 || y < b->clip.y0 || y >= b->clip.y1) 
		return;
	BM_SET(b, x, y, b->r, b->g, b->b, b->a);
}

void bm_line(Bitmap *b, int x0, int y0, int x1, int y1) {
	int dx = x1 - x0;
	int dy = y1 - y0;
	int sx, sy;
	int err, e2;
	
	if(dx < 0) dx = -dx;
	if(dy < 0) dy = -dy;
	
	if(x0 < x1) 
		sx = 1; 
	else 
		sx = -1;
	if(y0 < y1) 
		sy = 1; 
	else 
		sy = -1;
	
	err = dx - dy;
	
	for(;;) {
		/* Clipping can probably be more effective... */
		if(x0 >= b->clip.x0 && x0 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1) 
			BM_SET(b, x0, y0, b->r, b->g, b->b, b->a);
			
		if(x0 == x1 && y0 == y1) break;
		
		e2 = 2 * err;
		
		if(e2 > -dy) {
			err -= dy;
			x0 += sx;
		}		
		if(e2 < dx) {
			err += dx;
			y0 += sy;
		}		
	}
}

void bm_rect(Bitmap *b, int x0, int y0, int x1, int y1) {
	bm_line(b, x0, y0, x1, y0);
	bm_line(b, x1, y0, x1, y1);
	bm_line(b, x1, y1, x0, y1);
	bm_line(b, x0, y1, x0, y0);
}

void bm_fillrect(Bitmap *b, int x0, int y0, int x1, int y1) {
	int x,y;
	if(x1 < x0) {
		x = x0;
		x0 = x1;
		x1 = x;
	}
	if(y1 < y0) {
		y = y0;
		y0 = y1;
		y1 = y;
	}
	for(y = MAX(y0, b->clip.y0); y < MIN(y1 + 1, b->clip.y1); y++) {		
		for(x = MAX(x0, b->clip.x0); x < MIN(x1 + 1, b->clip.x1); x++) {
			assert(y >= 0 && y < b->h && x >= 0 && x < b->w);
			BM_SET(b, x, y, b->r, b->g, b->b, b->a);
		}
	}	
}

void bm_dithrect(Bitmap *b, int x0, int y0, int x1, int y1) {
	int x,y;
	if(x1 < x0) {
		x = x0;
		x0 = x1;
		x1 = x;
	}
	if(y1 < y0) {
		y = y0;
		y0 = y1;
		y1 = y;
	}
	for(y = MAX(y0, b->clip.y0); y < MIN(y1 + 1, b->clip.y1); y++) {		
		for(x = MAX(x0, b->clip.x0); x < MIN(x1 + 1, b->clip.x1); x++) {
            if((x + y) % 2) continue;
			assert(y >= 0 && y < b->h && x >= 0 && x < b->w);
			BM_SET(b, x, y, b->r, b->g, b->b, b->a);
		}
	}	
}

void bm_circle(Bitmap *b, int x0, int y0, int r) {
	int x = -r;
	int y = 0;
	int err = 2 - 2 * r;
	do {
		int xp, yp;
		
		/* Lower Right */
		xp = x0 - x; yp = y0 + y;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b, b->a);
		
		/* Lower Left */
		xp = x0 - y; yp = y0 - x;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b, b->a);
		
		/* Upper Left */
		xp = x0 + x; yp = y0 - y;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b, b->a);
		
		/* Upper Right */
		xp = x0 + y; yp = y0 + x;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b, b->a);
				
		r = err;
		if(r > x) {
			x++;
			err += x*2 + 1;
		}
		if(r <= y) {
			y++;
			err += y * 2 + 1;
		}
	} while(x < 0);
}

void bm_fillcircle(Bitmap *b, int x0, int y0, int r) {
	int x = -r;
	int y = 0;
	int err = 2 - 2 * r;
	do {
		int i;
		for(i = x0 + x; i <= x0 - x; i++) {
			/* Maybe the clipping can be more effective... */
			int yp = y0 + y;
			if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
				BM_SET(b, i, yp, b->r, b->g, b->b, b->a);
			yp = y0 - y;
			if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
				BM_SET(b, i, yp, b->r, b->g, b->b, b->a);			
		}		
		
		r = err;
		if(r > x) {
			x++;
			err += x*2 + 1;
		}
		if(r <= y) {
			y++;
			err += y * 2 + 1;
		}
	} while(x < 0);
}

void bm_ellipse(Bitmap *b, int x0, int y0, int x1, int y1) {
	int a = abs(x1-x0), b0 = abs(y1-y0), b1 = b0 & 1;
	long dx = 4 * (1 - a) * b0 * b0,
		dy = 4*(b1 + 1) * a * a;
	long err = dx + dy + b1*a*a, e2;
	
	if(x0 > x1) { x0 = x1; x1 += a; }
	if(y0 > y1) { y0 = y1; }
	y0 += (b0+1)/2; 
	y1 = y0 - b1;
	a *= 8*a;
	b1 = 8 * b0 * b0;
	
	do {
		if(x1 >= b->clip.x0 && x1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x1, y0, b->r, b->g, b->b, b->a);	
		
		if(x0 >= b->clip.x0 && x0 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x0, y0, b->r, b->g, b->b, b->a);	
		
		if(x0 >= b->clip.x0 && x0 < b->clip.x1 && y1 >= b->clip.y0 && y1 < b->clip.y1)
			BM_SET(b, x0, y1, b->r, b->g, b->b, b->a);	
		
		if(x1 >= b->clip.x0 && x1 < b->clip.x1 && y1 >= b->clip.y0 && y1 < b->clip.y1)
			BM_SET(b, x1, y1, b->r, b->g, b->b, b->a);	
		
		e2 = 2 * err;
		if(e2 <= dy) {
			y0++; y1--; err += dy += a;
		}
		if(e2 >= dx || 2*err > dy) {
			x0++; x1--; err += dx += b1;
		}	
	} while(x0 <= x1);
	
	while(y0 - y1 < b0) {
		if(x0 - 1 >= b->clip.x0 && x0 - 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x0 - 1, y0, b->r, b->g, b->b, b->a);
		
		if(x1 + 1 >= b->clip.x0 && x1 + 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x1 + 1, y0, b->r, b->g, b->b, b->a);
		y0++;
		
		if(x0 - 1 >= b->clip.x0 && x0 - 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x0 - 1, y1, b->r, b->g, b->b, b->a);
		
		if(x1 + 1 >= b->clip.x0 && x1 + 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x1 + 1, y1, b->r, b->g, b->b, b->a);	
		y1--;
	}
}

void bm_roundrect(Bitmap *b, int x0, int y0, int x1, int y1, int r) {
	int x = -r;
	int y = 0;
	int err = 2 - 2 * r;
	int rad = r;
	
	bm_line(b, x0 + r, y0, x1 - r, y0);
	bm_line(b, x0, y0 + r, x0, y1 - r);
	bm_line(b, x0 + r, y1, x1 - r, y1);
	bm_line(b, x1, y0 + r, x1, y1 - r);	
	
	do {
		int xp, yp;
		
		/* Lower Right */
		xp = x1 - x - rad; yp = y1 + y - rad;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b, b->a);
		
		/* Lower Left */
		xp = x0 - y + rad; yp = y1 - x - rad;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b, b->a);
		
		/* Upper Left */
		xp = x0 + x + rad; yp = y0 - y + rad;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b, b->a);
		
		/* Upper Right */
		xp = x1 + y - rad; yp = y0 + x + rad;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b, b->a);
		
		r = err;
		if(r > x) {
			x++;
			err += x*2 + 1;
		}
		if(r <= y) {
			y++;
			err += y * 2 + 1;
		}
	} while(x < 0);
}

void bm_fillroundrect(Bitmap *b, int x0, int y0, int x1, int y1, int r) {
	int x = -r;
	int y = 0;
	int err = 2 - 2 * r;
	int rad = r;
	do {
		int xp, xq, yp, i;
		
		xp = x0 + x + rad;
		xq = x1 - x - rad;
		for(i = xp; i <= xq; i++) {
			yp = y1 + y - rad;
			if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
				BM_SET(b, i, yp, b->r, b->g, b->b, b->a);
			yp = y0 - y + rad;
			if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
				BM_SET(b, i, yp, b->r, b->g, b->b, b->a);
		}
		
		r = err;
		if(r > x) {
			x++;
			err += x*2 + 1;
		}
		if(r <= y) {
			y++;
			err += y * 2 + 1;
		}
	} while(x < 0);
	
	for(y = MAX(y0 + rad + 1, b->clip.y0); y < MIN(y1 - rad, b->clip.y1); y++) {
		for(x = MAX(x0, b->clip.x0); x <= MIN(x1,b->clip.x1 - 1); x++) {
			assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
			BM_SET(b, x, y, b->r, b->g, b->b, b->a);
		}			
	}
}

/* Bexier curve with 3 control points.
 * See http://devmag.org.za/2011/04/05/bzier-curves-a-tutorial/
 * I tried the more optimized version at
 * http://members.chello.at/~easyfilter/bresenham.html
 * but that one had some caveats.
 */
void bm_bezier3(Bitmap *b, int x0, int y0, int x1, int y1, int x2, int y2) {
	int lx = x0, ly = y0;
	int steps = 12;
	double inc = 1.0/steps;
	double t = inc, dx, dy;
	
	do {		
		dx = (1-t)*(1-t)*x0 + 2*(1-t)*t*x1 + t*t*x2;
		dy = (1-t)*(1-t)*y0 + 2*(1-t)*t*y1 + t*t*y2;
		bm_line(b, lx, ly, dx, dy);
		lx = dx;
		ly = dy;
		t += inc;
	} while(t < 1.0);
	bm_line(b, dx, dy, x2, y2);
}

void bm_fill(Bitmap *b, int x, int y) {
	/* TODO: The function does not take the clipping into account. 
	 * I'm not really sure how to handle it, to be honest.
	 */	
	struct node {int x; int y;} 
		*queue,
		n;		
	int qs = 0, /* queue size */
		mqs = 128; /* Max queue size */
	int sr, sg, sb; /* Source colour */
	int dr, dg, db; /* Destination colour */
			
	bm_get_color(b, &dr, &dg, &db);
	bm_picker(b, x, y);
	bm_get_color(b, &sr, &sg, &sb);
	
	/* Don't fill if source == dest
	 * It leads to major performance problems otherwise
	 */
	if(sr == dr && sg == dg && sb == db)
		return;
		
	queue = calloc(mqs, sizeof *queue);
	if(!queue)
		return;
		
	n.x = x; n.y = y;
	queue[qs++] = n;
	
	while(qs > 0) {
		struct node w,e, nn;
		int i;
		
		n = queue[--qs];
		w = n;
		e = n;
		
		if(!bm_color_is(b, n.x, n.y, sr, sg, sb))
			continue;
		
		while(w.x > 0) {			
			if(!bm_color_is(b, w.x-1, w.y, sr, sg, sb)) {
				break;
			}
			w.x--;
		}
		while(e.x < b->w - 1) {
			if(!bm_color_is(b, e.x+1, e.y, sr, sg, sb)) {
				break;
			}
			e.x++;
		}
		for(i = w.x; i <= e.x; i++) {
			assert(i >= 0 && i < b->w);
			BM_SET(b, i, w.y, dr, dg, db, b->a);			
			if(w.y > 0) {
				if(bm_color_is(b, i, w.y - 1, sr, sg, sb)) {
					nn.x = i; nn.y = w.y - 1;
					queue[qs++] = nn;
					if(qs == mqs) {
						mqs <<= 1;
						queue = realloc(queue, mqs * sizeof *queue);
						if(!queue)
							return;
					}
				}
			}
			if(w.y < b->h - 1) {
				if(bm_color_is(b, i, w.y + 1, sr, sg, sb)) {
					nn.x = i; nn.y = w.y + 1;
					queue[qs++] = nn;
					if(qs == mqs) {
						mqs <<= 1;
						queue = realloc(queue, mqs * sizeof *queue);
						if(!queue)
							return;
					}
				}
			}
		}		
	}
	free(queue);
}

static void fs_add_factor(Bitmap *b, int x, int y, int er, int eg, int eb, double f) {
	int c, R, G, B;
	if(x < 0 || x >= b->w || y < 0 || y >= b->h)
		return;
	c = bm_get(b, x, y);
	
	R = ((c >> 16) & 0xFF) + f * er;
	G = ((c >> 8) & 0xFF) + f * eg;
	B = ((c >> 0) & 0xFF) + f * eb;
	
	if(R > 255) R = 255;
	if(R < 0) R = 0;
	if(G > 255) G = 255;
	if(G < 0) G = 0;
	if(B > 255) B = 255;
	if(B < 0) B = 0;
	
	bm_set_rgb(b, x, y, R, G, B);
}

void bm_reduce_palette(Bitmap *b, int palette[], size_t n) {
	/* Floyd–Steinberg dithering
		http://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering 
	*/
	int x, y;
	if(!b) 
		return;
	for(y = 0; y < b->h; y++) {
		for(x = 0; x < b->w; x++) {
			int i, m = 0;
			int r1, g1, b1;
			int r2, g2, b2;
			int er, eg, eb;
			int newpixel, oldpixel = bm_get(b, x, y);		
			double md = bm_cdist(oldpixel, palette[m]);	
			for(i = 1; i < n; i++) {
				double d = bm_cdist(oldpixel, palette[i]);
				if(d < md) {
					md = d;
					m = i;
				}
			}
			newpixel = palette[m];
			
			bm_set(b, x, y, newpixel);
			
			r1 = (oldpixel >> 16) & 0xFF; g1 = (oldpixel >> 8) & 0xFF; b1 = (oldpixel >> 0) & 0xFF;
			r2 = (newpixel >> 16) & 0xFF; g2 = (newpixel >> 8) & 0xFF; b2 = (newpixel >> 0) & 0xFF;
			er = r1 - r2; eg = g1 - g2; eb = b1 - b2;

			fs_add_factor(b, x + 1, y    , er, eg, eb, 7.0 / 16.0);
			fs_add_factor(b, x - 1, y + 1, er, eg, eb, 3.0 / 16.0);
			fs_add_factor(b, x    , y + 1, er, eg, eb, 5.0 / 16.0);
			fs_add_factor(b, x + 1, y + 1, er, eg, eb, 1.0 / 16.0);
			
		}
	}
}

/** FONT FUNCTIONS **********************************************************/

void bm_set_font(Bitmap *b, BmFont *font) {
	if(b->font && b->font->dtor)
		b->font->dtor(b->font);
	b->font = font;	
}

int bm_text_width(Bitmap *b, const char *s) {
	int len = 0, max_len = 0;
	int glyph_width;
	
	if(!b->font || !b->font->width)
		return 0;
		
	glyph_width = b->font->width(b->font);
	while(*s) {
		if(*s == '\n') {
			if(len > max_len)
				max_len = len;
			len = 0;
		} else if(*s == '\t') {
			len+=4;
		} else if(isprint(*s)) {
			len++;
		}
		s++;
	}
	if(len > max_len)
		max_len = len;
	return max_len * glyph_width;
}

int bm_text_height(Bitmap *b, const char *s) {
	int height = 1;
	int glyph_height;
	if(!b->font || !b->font->height)
		return 0;
	glyph_height = b->font->height(b->font);
	while(*s) {
		if(*s == '\n') height++;
		s++;
	}
	return height * glyph_height;
}

int bm_puts(Bitmap *b, int x, int y, const char *text) {
	if(!b->font || !b->font->puts)
		return 0;
	return b->font->puts(b, x, y, text);
}

int bm_printf(Bitmap *b, int x, int y, const char *fmt, ...) {
	char buffer[256];
	va_list arg;
	if(!b->font || !b->font->puts) return 0;
	va_start(arg, fmt);
  	vsnprintf(buffer, sizeof buffer, fmt, arg);
  	va_end(arg);
	return bm_puts(b, x, y, buffer);
}

/** XBM FONT FUNCTIONS ******************************************************/

typedef struct {
	const unsigned char *bits;
	int spacing;
} XbmFontInfo;

static void xbmf_putc(Bitmap *b, XbmFontInfo *info, int x, int y, char c) {
	int frow, fcol, byte, col;
	int i, j;
	if(!info || !info->bits || c < 32 || c > 127) return;
	c -= 32;
	fcol = c >> 3;
	frow = c & 0x7;
	byte = frow * FONT_WIDTH + fcol;
	
	col = bm_get_color_i(b);
	
	for(j = 0; j < 8 && y + j < b->clip.y1; j++) {
		if(y + j >= b->clip.y0) {
			char bits = info->bits[byte];
			for(i = 0; i < 8 && x + i < b->clip.x1; i++) {
				if(x + i >= b->clip.x0 && !(bits & (1 << i))) {
					bm_set(b, x + i, y + j, col);
				}
			}
		}
		byte += FONT_WIDTH >> 3;
	}
}

static int xbmf_puts(Bitmap *b, int x, int y, const char *text) {
	XbmFontInfo *info;
	int xs = x;
	if(!b->font) return 0;
	info = b->font->data;
	while(text[0]) {
		if(text[0] == '\n') {
			y += 8;
			x = xs;
		} else if(text[0] == '\t') {
			/* I briefly toyed with the idea of having tabs line up,
			 * but it doesn't really make sense because
			 * this isn't exactly a character based terminal.
			 */
			x += 4 * info->spacing;
		} else if(text[0] == '\r') {
			/* why would anyone find this useful? */
			x = xs;
		} else {
			xbmf_putc(b, info, x, y, text[0]);
			x += info->spacing;
		}
		text++;
		if(y > b->h) { 
			/* I used to check x >= b->w as well,
			but it doesn't take \n's into account */
			return 1;
		}
	}
	return 1;
}
static void xbmf_dtor(struct bitmap_font *font) {
	XbmFontInfo *info = font->data;
	free(info);
	free(font);
}
static int xbmf_width(struct bitmap_font *font) {
	XbmFontInfo *info = font->data;
	return info->spacing;
}
static int xbmf_height(struct bitmap_font *font) {
	return 8;
}

BmFont *bm_make_xbm_font(const unsigned char *bits, int spacing) {
	BmFont *font;
	XbmFontInfo *info;
	font = malloc(sizeof *font);
	if(!font) 
		return NULL;
	info = malloc(sizeof *info);
	if(!info) {
		free(font);
		return NULL;
	}
	
	info->bits = bits;
	info->spacing = spacing;
	
	font->type = "XBM";
	font->puts = xbmf_puts;
	font->dtor = xbmf_dtor;
	font->width = xbmf_width;
	font->height = xbmf_height;
	font->data = info;
	
	return font;
}

#ifndef NO_FONTS

static struct xbm_font_info {
	const char *s;
	int i;
	const unsigned char *bits;
	int spacing;
} xbm_font_infos[] = {
	{"NORMAL", BM_FONT_NORMAL, normal_bits, 6},
	{"BOLD", BM_FONT_BOLD, bold_bits, 8},
	{"CIRCUIT", BM_FONT_CIRCUIT, circuit_bits, 7},
	{"HAND", BM_FONT_HAND, hand_bits, 7},
	{"SMALL", BM_FONT_SMALL, small_bits, 5},
	{"SMALL_I", BM_FONT_SMALL_I, smallinv_bits, 7},
	{"THICK", BM_FONT_THICK, thick_bits, 6},
	{NULL, 0}
};

void bm_std_font(Bitmap *b, enum bm_fonts font) {
	struct xbm_font_info *info = &xbm_font_infos[font];
	BmFont * bfont = bm_make_xbm_font(info->bits, info->spacing);
	bm_set_font(b, bfont);
}

int bm_font_index(const char *name) {
	int i = 0;
	char buffer[12], *c = buffer;
	do {
		*c++ = toupper(*name++);
	} while(*name && c - buffer < sizeof buffer - 1);
	*c = '\0';
	
	while(xbm_font_infos[i].s) {
		if(!strcmp(xbm_font_infos[i].s, buffer)) {
			return xbm_font_infos[i].i;
		}
		i++;
	}
	return BM_FONT_NORMAL;
}

const char *bm_font_name(int index) {
	int i = 0;
	while(xbm_font_infos[i].s) {
		i++;
		if(xbm_font_infos[i].i == index) {
			return xbm_font_infos[i].s;
		}
	}
	return xbm_font_infos[0].s;
}
#endif /* NO_FONTS */