#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include "bmp.h"

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

#define FONT_WIDTH 96

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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

#define BM_BPP			4
#define BM_BLOB_SIZE(B)	(B->w * B->h * BM_BPP)
#define BM_ROW_SIZE(B)	(B->w * BM_BPP)

#define BM_SET(BMP, X, Y, R, G, B) do { \
		int _p = ((Y) * BM_ROW_SIZE(BMP) + (X)*BM_BPP);	\
		BMP->data[_p++] = R;\
		BMP->data[_p++] = G;\
		BMP->data[_p++] = B;\
	} while(0)

#define BM_GETR(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 0])
#define BM_GETG(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 1])
#define BM_GETB(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 2])
#define BM_GETA(B,X,Y) (B->data[((Y) * BM_ROW_SIZE(B) + (X) * BM_BPP) + 3])

struct bitmap *bm_create(int w, int h) {	
	struct bitmap *b = malloc(sizeof *b);
	
	b->w = w;
	b->h = h;
		
	b->data = malloc(BM_BLOB_SIZE(b));
	memset(b->data, 0x00, BM_BLOB_SIZE(b));
	
	bm_std_font(b, BM_FONT_NORMAL);
	bm_set_color(b, 255, 255, 255);
	
	return b;
}

struct bitmap *bm_load(const char *filename) {	
	struct bitmap *bmp;
	FILE *f = fopen(filename, "rb");
	if(!f) {
		return NULL;
	}
	bmp = bm_load_fp(f);
	
	fclose(f);
	
	return bmp;
}

struct bitmap *bm_load_fp(FILE *f) {	
	struct bmpfile_magic magic; 
	struct bmpfile_header hdr;
	struct bmpfile_dibinfo dib;
	struct bmpfile_colinfo *palette = NULL;
	
	struct bitmap *b = NULL;
	
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
				BM_SET(b, i, j, palette[p].r, palette[p].g, palette[p].b);
			}
		}			
	} else {	
		for(j = 0; j < b->h; j++) {
			for(i = 0; i < b->w; i++) {
				int p = ((b->h - (j) - 1) * rs + (i)*3);			
				BM_SET(b, i, j, data[p+2], data[p+1], data[p+0]);
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

int bm_save(struct bitmap *b, const char *fname) {	
	struct bmpfile_magic magic = {{'B','M'}}; 
	struct bmpfile_header hdr;
	struct bmpfile_dibinfo dib;
	
	int rs, padding, i, j;
	char *data;

	padding = 4 - ((b->w * 3) % 4);
	if(padding > 3) padding = 0;
	rs = b->w * 3 + padding;	
	assert(rs % 4 == 0);
		
	FILE *f = fopen(fname, "w");
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

struct bitmap *bm_copy(struct bitmap *b) {
	struct bitmap *out = bm_create(b->w, b->h);
	memcpy(out->data, b->data, BM_BLOB_SIZE(b));
	return out;
}

void bm_free(struct bitmap *b) {
	if(b->data) free(b->data);
	free(b);
}

void bm_set(struct bitmap *b, int x, int y, unsigned char R, unsigned char G, unsigned char B) {
	assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
	BM_SET(b, x, y, R, G, B);
}

unsigned char bm_getr(struct bitmap *b, int x, int y) {
	return BM_GETR(b,x,y);
}

unsigned char bm_getg(struct bitmap *b, int x, int y) {
	return BM_GETG(b,x,y);
}

unsigned char bm_getb(struct bitmap *b, int x, int y) {
	return BM_GETB(b,x,y);
}

struct bitmap *bm_fromXbm(int w, int h, unsigned char *data) {
	int x,y;
		
	struct bitmap *bmp = bm_create(w, h);
	
	int byte = 0;	
	for(y = 0; y < h; y++)
		for(x = 0; x < w;) {			
			int i, b;
			b = data[byte++];
			for(i = 0; i < 8 && x < w; i++) {
				unsigned char c = (b & (1 << i))?0x00:0xFF;
				BM_SET(bmp, x++, y, c, c, c);
			}
		}
	return bmp;
}

void bm_blit(struct bitmap *dst, int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h) {
	int x,y, i, j;

	if(dx < 0) {
		sx += -dx;
		w -= -dx;
		dx = 0;
	}
	
	if(dx + w > dst->w) {
		int delta = dx + w - dst->w;
		w -= delta;
	}

	if(dy < 0) {
		sy += -dy;
		h -= -dy;
		dy = 0;
	}
	
	if(dy + h > dst->h) {
		int delta = dy + h - dst->h;
		h -= delta;
	}
	
	if(w <= 0 || h <= 0)
		return;
	if(dx >= dst->w || dx + w < 0)
		return;
	if(dy >= dst->h || dy + h < 0)
		return;
	
	if(sx + w > src->w) {
		int delta = sx + w - src->w;
		w -= delta;
	}
	
	if(sy + h > src->h) {
		int delta = sy + h - src->h;
		h -= delta;
	}
	
	assert(dx >= 0 && dx + w <= dst->w);
	assert(dy >= 0 && dy + h <= dst->h);	
	assert(sx >= 0 && sx + w <= src->w);
	assert(sy >= 0 && sy + h <= src->h);
	
	j = sy;
	for(y = dy; y < dy + h; y++) {		
		i = sx;
		for(x = dx; x < dx + w; x++) {
			int r = BM_GETR(src, i, j),
				g = BM_GETG(src, i, j),
				b = BM_GETB(src, i, j);
			BM_SET(dst, x, y, r, g, b);
			i++;
		}
		j++;
	}
}

void bm_maskedblit(struct bitmap *dst, int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h) {
	int x,y, i, j;

	if(dx < 0) {
		sx += -dx;
		w -= -dx;
		dx = 0;
	}
	
	if(dx + w > dst->w) {
		int delta = dx + w - dst->w;
		w -= delta;
	}

	if(dy < 0) {
		sy += -dy;
		h -= -dy;
		dy = 0;
	}
	
	if(dy + h > dst->h) {
		int delta = dy + h - dst->h;
		h -= delta;
	}
	
	if(w <= 0 || h <= 0)
		return;
	if(dx >= dst->w || dx + w < 0)
		return;
	if(dy >= dst->h || dy + h < 0)
		return;
	
	if(sx + w > src->w) {
		int delta = sx + w - src->w;
		w -= delta;
	}
	
	if(sy + h > src->h) {
		int delta = sy + h - src->h;
		h -= delta;
	}
	
	assert(dx >= 0 && dx + w <= dst->w);
	assert(dy >= 0 && dy + h <= dst->h);	
	assert(sx >= 0 && sx + w <= src->w);
	assert(sy >= 0 && sy + h <= src->h);
	
	j = sy;
	for(y = dy; y < dy + h; y++) {		
		i = sx;
		for(x = dx; x < dx + w; x++) {
			int r = BM_GETR(src, i, j),
				g = BM_GETG(src, i, j),
				b = BM_GETB(src, i, j);
			if(r != src->r || g != src->g || b != src->b)
				BM_SET(dst, x, y, r, g, b);
			i++;
		}
		j++;
	}
}

void bm_smooth(struct bitmap *b) {
	struct bitmap *tmp = bm_create(b->w, b->h);
	unsigned char *t = b->data;
	int x, y;
	for(y = 0; y < b->h; y++)
		for(x = 0; x < b->w; x++) {
			int p, q, c = 0, R = 0, G = 0, B = 0;
			for(p = x-1; p < x+1; p++)
				for(q = y-1; q < y+1; q++) {
					if(p < 0 || p >= b->w || q < 0 || q >= b->h)
						continue;
					R += BM_GETR(b,p,q);
					G += BM_GETG(b,p,q);
					B += BM_GETB(b,p,q);
					c++;					
				}
			BM_SET(tmp, x, y, R/c, G/c, B/c);
		}
	
	b->data = tmp->data;
	tmp->data = t;
	bm_free(tmp);
}

void bm_swap_colour(struct bitmap *b, unsigned char sR, unsigned char sG, unsigned char sB, unsigned char dR, unsigned char dG, unsigned char dB) {
	int x,y;
	for(y = 0; y < b->h; y++)
		for(x = 0; x < b->w; x++) {			
			if(BM_GETR(b,x,y) == sR && BM_GETG(b,x,y) == sG && BM_GETB(b,x,y) == sB)
				BM_SET(b, x, y, dR, dG, dB);
		}
}

struct bitmap *bm_resample(const struct bitmap *in, int nw, int nh) {
	struct bitmap *out = bm_create(nw, nh);
	int x, y;
	for(y = 0; y < nh; y++)
		for(x = 0; x < nw; x++) {
			int sx = x * in->w/nw;
			int sy = y * in->h/nh;
			assert(sx < in->w && sy < in->h);
			BM_SET(out, x, y, BM_GETR(in,sx,sy), BM_GETG(in,sx,sy), BM_GETB(in,sx,sy));
		}
	return out;
}

void bm_set_color(struct bitmap *bm, int r, int g, int b) {
	bm->r = r;
	bm->g = g;
	bm->b = b;
}

/* Lookup table for bm_color_atoi() 
 * A good reference for the various HTML colours is at
 * http://www.w3schools.com/html/html_colornames.asp
 * (I haven't used all of them)
 */
static const struct color_map_entry {
	const char *name;
	int color;
} color_map[] = {
	{"black", 0x000000},
	{"white", 0xFFFFFF},
	{"gray", 0x808080},
	{"grey", 0x808080},
	{"dimgray", 0x696969},
	{"dimgray", 0x696969},
	{"silver", 0xC0C0C0},
	{"red", 0xFF0000},
	{"darkred", 0x8B0000},
	{"maroon", 0x800000},
	{"green", 0x00FF00},
	{"lime", 0x00FF00},
	{"darkgreen", 0x008B00},
	{"blue", 0x0000FF},
	{"darkblue", 0x00008B},
	{"navy", 0x000080},
	{"cyan", 0x00FFFF},
	{"darkcyan", 0x008B8B},
	{"aqua", 0x00FFFF},
	{"darkaqua", 0x008B8B},
	{"teal", 0x008080},
	{"magenta", 0xFF00FF},
	{"darkmagenta", 0x8B008B},
	{"fuchsia", 0xFF00FF},
	{"darkfuchsia", 0x8B008B},
	{"purple", 0x800080},
	{"pink", 0xFFC0CB},
	{"yellow", 0xFFFF00},
	{"darkyellow", 0x8B8B00},
	{"olive", 0x808000},
	{"orange", 0xFFA500},
	{"darkorange", 0xFF8C00},
	{NULL, 0}
};

int bm_color_atoi(const char *text) {
	const struct color_map_entry *cm = color_map;
	int col = 0;
	while(cm->name) {
		/* Poor man's stricmp(): */
		const char *n = cm->name;
		const char *t = text;
		while(n[0] && tolower(n[0]) == tolower(t[0])) {
			n++;
			t++;
		}
		
		if(n[0] == '\0' && t[0] == '\0') {			
			return cm->color;
		}
		cm++;
	}
	if(text[0] == '#') text++;
	
	while(text[0]) {
		int c = tolower(text[0]);
		if(c >= 'a' && c <= 'f') {
			col = (col << 4) + (c - 'a' + 10); 
		} else if(isdigit(c)) {
			col = (col << 4) + (c - '0');
		} else {
			col <<= 4; 
		}		
		text++;
	}	
	return col;
}

void bm_set_color_s(struct bitmap *bm, const char *text) {	
	bm_set_color_i(bm, bm_color_atoi(text));
}

void bm_set_color_i(struct bitmap *bm, int col) {	
	bm_set_color(bm, (col >> 16) & 0xFF, (col >> 8) & 0xFF, (col >> 0) & 0xFF);
}

void bm_get_color(struct bitmap *bm, int *r, int *g, int *b) {
	*r = bm->r;
	*g = bm->g;
	*b = bm->b;
}

void bm_picker(struct bitmap *bm, int x, int y) {
	if(x < 0 || x >= bm->w || y < 0 || y >= bm->h) 
		return;
	bm->r = BM_GETR(bm, x, y);
	bm->g = BM_GETG(bm, x, y);
	bm->b = BM_GETB(bm, x, y);
}

int bm_color_is(struct bitmap *bm, int x, int y, int r, int g, int b) {
	return BM_GETR(bm,x,y) == r && BM_GETG(bm,x,y) == g && BM_GETB(bm,x,y) == b;
}

int bm_gradient(int color1, int color2, double t) {
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

int bm_width(struct bitmap *b) {
	return b->w;
}

int bm_height(struct bitmap *b) {
	return b->h;
}

void bm_clear(struct bitmap *b) {
	int i, j;
	for(j = 0; j < b->h; j++) 
		for(i = 0; i < b->w; i++) {
			BM_SET(b, i, j, b->r, b->g, b->b);
		}
}

void bm_putpixel(struct bitmap *b, int x, int y) {
	if(x < 0 || x >= b->w || y < 0 || y >= b->h) 
		return;
	BM_SET(b, x, y, b->r, b->g, b->b);
}

void bm_line(struct bitmap *b, int x0, int y0, int x1, int y1) {
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
		if(x0 >= 0 && x0 < b->w && y0 >= 0 && y0 < b->h) 
			BM_SET(b, x0, y0, b->r, b->g, b->b);
			
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

void bm_rect(struct bitmap *b, int x0, int y0, int x1, int y1) {
	bm_line(b, x0, y0, x1, y0);
	bm_line(b, x1, y0, x1, y1);
	bm_line(b, x1, y1, x0, y1);
	bm_line(b, x0, y1, x0, y0);
}

void bm_fillrect(struct bitmap *b, int x0, int y0, int x1, int y1) {
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
	for(y = MAX(y0, 0); y <= MIN(y1, b->h); y++) {		
		for(x = MAX(x0, 0); x <= MIN(x1, b->w); x++) {
			assert(y >= 0 && y < b->h && x >= 0 && x < b->w);
			BM_SET(b, x, y, b->r, b->g, b->b);
		}
	}	
}

void bm_circle(struct bitmap *b, int x0, int y0, int r) {
	int x = -r;
	int y = 0;
	int err = 2 - 2 * r;
	do {
		int xp, yp;
		
		/* Lower Right */
		xp = x0 - x; yp = y0 + y;
		if(xp >= 0 && xp < b->w && yp >= 0 && yp < b->h)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Lower Left */
		xp = x0 - y; yp = y0 - x;
		if(xp >= 0 && xp < b->w && yp >= 0 && yp < b->h)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Upper Left */
		xp = x0 + x; yp = y0 - y;
		if(xp >= 0 && xp < b->w && yp >= 0 && yp < b->h)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Upper Right */
		xp = x0 + y; yp = y0 + x;
		if(xp >= 0 && xp < b->w && yp >= 0 && yp < b->h)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
				
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

void bm_fillcircle(struct bitmap *b, int x0, int y0, int r) {
	int x = -r;
	int y = 0;
	int err = 2 - 2 * r;
	do {
		int i;
		for(i = x0 + x; i <= x0 - x; i++) {
			/* Maybe the clipping can be more effective... */
			int yp = y0 + y;
			if(i >= 0 && i < b->w && yp >= 0 && yp < b->h)
				BM_SET(b, i, yp, b->r, b->g, b->b);
			yp = y0 - y;
			if(i >= 0 && i < b->w && yp >= 0 && yp < b->h)
				BM_SET(b, i, yp, b->r, b->g, b->b);			
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

void bm_ellipse(struct bitmap *b, int x0, int y0, int x1, int y1) {
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
		if(x1 >= 0 && x1 < b->w && y0 >= 0 && y0 < b->h)
			BM_SET(b, x1, y0, b->r, b->g, b->b);	
		
		if(x0 >= 0 && x0 < b->w && y0 >= 0 && y0 < b->h)
			BM_SET(b, x0, y0, b->r, b->g, b->b);	
		
		if(x0 >= 0 && x0 < b->w && y1 >= 0 && y1 < b->h)
			BM_SET(b, x0, y1, b->r, b->g, b->b);	
		
		if(x1 >= 0 && x1 < b->w && y1 >= 0 && y1 < b->h)
			BM_SET(b, x1, y1, b->r, b->g, b->b);	
		
		e2 = 2 * err;
		if(e2 <= dy) {
			y0++; y1--; err += dy += a;
		}
		if(e2 >= dx || 2*err > dy) {
			x0++; x1--; err += dx += b1;
		}	
	} while(x0 <= x1);
	
	while(y0 - y1 < b0) {
		if(x0 - 1 >= 0 && x0 - 1 < b->w && y0 >= 0 && y0 < b->h)
			BM_SET(b, x0 - 1, y0, b->r, b->g, b->b);
		
		if(x1 + 1 >= 0 && x1 + 1 < b->w && y0 >= 0 && y0 < b->h)
			BM_SET(b, x1 + 1, y0, b->r, b->g, b->b);
		y0++;
		
		if(x0 - 1 >= 0 && x0 - 1 < b->w && y0 >= 0 && y0 < b->h)
			BM_SET(b, x0 - 1, y1, b->r, b->g, b->b);
		
		if(x1 + 1 >= 0 && x1 + 1 < b->w && y0 >= 0 && y0 < b->h)
			BM_SET(b, x1 + 1, y1, b->r, b->g, b->b);	
		y1--;
	}
}

void bm_roundrect(struct bitmap *b, int x0, int y0, int x1, int y1, int r) {
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
		if(xp >= 0 && xp < b->w && yp >= 0 && yp < b->h)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Lower Left */
		xp = x0 - y + rad; yp = y1 - x - rad;
		if(xp >= 0 && xp < b->w && yp >= 0 && yp < b->h)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Upper Left */
		xp = x0 + x + rad; yp = y0 - y + rad;
		if(xp >= 0 && xp < b->w && yp >= 0 && yp < b->h)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Upper Right */
		xp = x1 + y - rad; yp = y0 + x + rad;
		if(xp >= 0 && xp < b->w && yp >= 0 && yp < b->h)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
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

void bm_fillroundrect(struct bitmap *b, int x0, int y0, int x1, int y1, int r) {
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
			if(i >= 0 && i < b->w && yp >= 0 && yp < b->h)
				BM_SET(b, i, yp, b->r, b->g, b->b);
			yp = y0 - y + rad;
			if(i >= 0 && i < b->w && yp >= 0 && yp < b->h)
				BM_SET(b, i, yp, b->r, b->g, b->b);
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
	
	for(y = MAX(y0 + rad + 1, 0); y < MIN(y1 - rad, b->h); y++) {
		for(x = MAX(x0, 0); x <= MIN(x1, b->w - 1); x++) {
			assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
			BM_SET(b, x, y, b->r, b->g, b->b);
		}			
	}
}

/* This is the bezier with 3 control points from
 * http://members.chello.at/~easyfilter/bresenham.html
 * I've noticed that it doesn't work quite correctly 
 * if <x1,y1> falls outside the bounding box defined by
 * <x0,y0> - <x2,y2>, which is understandable considering
 * how it works in principle.
 * I need to doublecheck against the source, though.
 */
void bm_bezier3(struct bitmap *b, int x0, int y0, int x1, int y1, int x2, int y2) {
	int sx = x2 - x1, sy = y2 - y1;
	int xx = x0 - x1, yy = y0 - y1, xy;
	double dx, dy, err, cur = xx * sy - yy * sx;
	
	/* This assertion also fails from time to time */
	assert(xx * sx <= 0 && yy * sy <= 0);
	
	if(sx * sx + sy * sy > xx * xx + yy * yy) {
		x2 = x0;
		x0 = sx + x1;
		y2 = y0;
		y0 = sy + y1;
		cur = -cur;
	}
	
	if(cur != 0) {
		xx += sx; 
		xx *= sx = x0 < x2 ? 1 : -1;
		yy += sy;
		yy *= sy = y0 < y2 ? 1 : -1;
		xy = 2 * xx * yy;
		xx *= xx;
		yy *= yy;
		if(cur * sx * sy < 0) {
			xx = -xx;
			yy = -yy;
			xy = -xy;
			cur = -cur;
		}
		
		dx = 4.0 * sy * cur * (x1 - x0) + xx - xy;
		dy = 4.0 * sx * cur * (y0 - y1) + yy - xy;
		xx += xx;
		yy += yy;
		err = dx + dy + xy;
		do {
			if(x0 >= 0 && x0 < b->w && y0 >= 0 && y0 < b->h)
				BM_SET(b, x0, y0, b->r, b->g, b->b);
			if(x0 == x2 && y0 == y2)
				return;
			y1 = 2 * err < dx;
			if(2 * err > dy) {
				x0 += sx;
				dx -= xy;
				err += dy += yy;
			}
			if(y1) {
				y0 += sy; 
				dy -= xy;
				err += dx += xx;
			}
		} while (dy < dx);
	}
	bm_line(b, x0, y0, x2, y2);
}

void bm_fill(struct bitmap *b, int x, int y) {
	struct node {int x; int y;} 
		*queue,
		n = {x, y};
		
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
		
	queue[qs++] = n;
	
	while(qs > 0) {
		struct node w,e;
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
			BM_SET(b, i, w.y, dr, dg, db);			
			if(w.y > 0) {
				if(bm_color_is(b, i, w.y - 1, sr, sg, sb)) {
					struct node nn = {i, w.y - 1};
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
					struct node nn = {i, w.y + 1};
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

void bm_set_font(struct bitmap *b, const unsigned char *font, int spacing) {
	if(font)
		b->font = font;
	if(spacing > 0)
		b->font_spacing = spacing;
}

void bm_std_font(struct bitmap *b, enum bm_fonts font) {
	switch(font) {
		case BM_FONT_NORMAL  : bm_set_font(b, normal_bits, 6); break;
		case BM_FONT_BOLD    : bm_set_font(b, bold_bits, 8); break;
		case BM_FONT_CIRCUIT : bm_set_font(b, circuit_bits, 7); break;
		case BM_FONT_HAND    : bm_set_font(b, hand_bits, 7); break;
		case BM_FONT_SMALL   : bm_set_font(b, small_bits, 5); break;
		case BM_FONT_SMALL_I : bm_set_font(b, smallinv_bits, 7); break;
		case BM_FONT_THICK   : bm_set_font(b, thick_bits, 6); break;
	}
}

int bm_text_width(struct bitmap *b, const char *s) {
	int len = 0, max_len = 0;
	while(*s) {
		if(*s == '\n') {
			if(len > max_len)
				max_len = len;
			len = 0;
		} else if(isprint(*s)) {
			len++;
		}
		s++;
	}
	if(len > max_len)
		max_len = len;
	return max_len * b->font_spacing;
}

int bm_text_height(struct bitmap *b, const char *s) {
	int height = 1;
	while(*s) {
		if(*s == '\n') height++;
		s++;
	}
	return height * 8;
}

void bm_putc(struct bitmap *b, int x, int y, char c) {
	int frow, fcol, byte;
	int i, j;
	if(c < 32 || c > 127) return;
	c -= 32;
	fcol = c >> 3;
	frow = c & 0x7;
	byte = frow * FONT_WIDTH + fcol;
	for(j = 0; j < 8 && y + j < b->h; j++) {
		if(y + j >= 0) {
			char bits = b->font[byte];
			for(i = 0; i < 8 && x + i < b->w; i++) {
				if(x + i >= 0 && !(bits & (1 << i))) {
					BM_SET(b, x + i, y + j, b->r, b->g, b->b);
				}
			}
		}
		byte += FONT_WIDTH >> 3;
	}
}

void bm_puts(struct bitmap *b, int x, int y, const char *text) {
	int xs = x;
	while(text[0]) {
		if(text[0] == '\n') {
			y += 8;
			x = xs;
		} else if(text[0] == '\t') {
			/* I briefly toyed with the idea of having tabs line up,
			 * but it doesn't really make sense because
			 * this isn't exactly a character based terminal.
			 */
			x += b->font_spacing;
		} else if(text[0] == '\r') {
			/* why would anyone find this useful? */
			x = xs;
		} else {
			bm_putc(b, x, y, text[0]);
			x += b->font_spacing;
		}
		text++;
		if(y > b->h) { 
			/* I used to check x >= b->w as well,
			but it doesn't take \n's into account */
			return;
		}
	}
}

void bm_printf(struct bitmap *b, int x, int y, const char *fmt, ...) {
	char buffer[256];
	va_list arg;
	va_start (arg, fmt);
  	vsnprintf (buffer, sizeof buffer, fmt, arg);
  	va_end (arg);
	bm_puts(b, x, y, buffer);
}

void bm_putcs(struct bitmap *b, int x, int y, int s, char c) {
	int frow, fcol, byte;
	int i, j;
	if(c < 32 || c > 127) return;
	c -= 32;
	fcol = c >> 3;
	frow = c & 0x7;
	
	for(j = 0; j < (8 << s) && y + j < b->h; j++) {
		byte = frow * FONT_WIDTH + fcol + (j >> s) * (FONT_WIDTH >> 3);		
		if(y + j >= 0) {
			char bits = b->font[byte];
			for(i = 0; i < (8 << s) && x + i < b->w; i++) {
				if(x + i >= 0 && !(bits & (1 << (i >> s)))) {
					BM_SET(b, x + i, y + j, b->r, b->g, b->b);
				}
			}
		}
	}
}

void bm_putss(struct bitmap *b, int x, int y, int s, const char *text) {
	int xs = x;
	while(text[0]) {
		if(text[0] == '\n') {
			y += 8 << s;
			x = xs;
		} else if(text[0] == '\t') {
			/* I briefly toyed with the idea of having tabs line up,
			 * but it doesn't really make sense because
			 * this isn't exactly a character based terminal.
			 */
			x += b->font_spacing << s;
		} else if(text[0] == '\r') {
			/* why would anyone find this useful? */
			x = xs;
		} else {
			bm_putcs(b, x, y, s, text[0]);
			x += b->font_spacing << s;
		}
		text++;
		if(y > b->h) { 
			/* I used to check x >= b->w as well,
			but it doesn't take \n's into account */
			return;
		}
	}
}

void bm_printfs(struct bitmap *b, int x, int y, int s, const char *fmt, ...) {
	char buffer[256];
	va_list arg;
	va_start (arg, fmt);
  	vsnprintf (buffer, sizeof buffer, fmt, arg);
  	va_end (arg);
	if(s > 0)
		bm_putss(b, x, y, s, buffer);
	else
		bm_puts(b, x, y, buffer);
}

