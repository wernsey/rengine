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
	
	b->clip.x0 = 0;
	b->clip.y0 = 0;
	b->clip.x1 = w;
	b->clip.y1 = h;
		
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
		
	FILE *f = fopen(fname, "wb");
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
	
	out->r = b->r; out->g = b->g; out->b = b->b;
	out->font = b->font;
	out->font_spacing = b->font_spacing;
	memcpy(&out->clip, &b->clip, sizeof b->clip);
	
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

void bm_clip(struct bitmap *b, int x0, int y0, int x1, int y1) {
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

void bm_unclip(struct bitmap *b) {
	b->clip.x0 = 0;
	b->clip.y0 = 0;
	b->clip.x1 = b->w;
	b->clip.y1 = b->h;
}

void bm_blit(struct bitmap *dst, int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h) {
	int x,y, i, j;

	if(dx < dst->clip.x0) {
		int delta = dst->clip.x0 - dx;
		sx += delta;
		w -= delta;
		dx = dst->clip.x0;
	}
	
	if(dx + w > dst->clip.x1) {
		int delta = dx + w - dst->clip.x1;
		w -= delta;
	}

	if(dy < dst->clip.y0) {
		int delta = dst->clip.y0 - dy;
		sy += delta;
		h -= delta;
		dy = dst->clip.y0;
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
				b = BM_GETB(src, i, j);
			BM_SET(dst, x, y, r, g, b);
			i++;
		}
		j++;
	}
}

void bm_maskedblit(struct bitmap *dst, int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h) {
	int x,y, i, j;

	if(dx < dst->clip.x0) {
		int delta = dst->clip.x0 - dx;
		sx += delta;
		w -= delta;
		dx = dst->clip.x0;
	}
	
	if(dx + w > dst->clip.x1) {
		int delta = dx + w - dst->clip.x1;
		w -= delta;
	}

	if(dy < dst->clip.y0) {
		int delta = dst->clip.y0 - dy;
		sy += delta;
		h -= delta;
		dy = dst->clip.y0;
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
				b = BM_GETB(src, i, j);
			if(r != src->r || g != src->g || b != src->b)
				BM_SET(dst, x, y, r, g, b);
			i++;
		}
		j++;
	}
}

void bm_blit_ex(struct bitmap *dst, int dx, int dy, int dw, int dh, struct bitmap *src, int sx, int sy, int sw, int sh, int mask) {
	int x, y;
	int ssx = sx; 
	int ynum = 0;
	
	if(sw == dw && sh == dh) {
		if(mask) {
			bm_maskedblit(dst, dx, dy, src, sx, sy, dw, dh);
		} else {
			bm_blit(dst, dx, dy, src, sx, sy, dw, dh);
		}
		return;
	}
	
	if(sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return;
	if(dx >= dst->clip.x1 || dx + dw < dst->clip.x0)
		return;
	if(dy >= dst->clip.y1 || dy + dh < dst->clip.y0)
		return;
	
	/*
	Uses Bresenham's algoritm to implement a simple scaling while blitting.
	See the article "Scaling Bitmaps with Bresenham" by Tim Kientzle in the
	October 1995 issue of C/C++ Users Journal
	
	Or see these links:
		http://www.drdobbs.com/image-scaling-with-bresenham/184405045
		http://www.compuphase.com/graphic/scale.htm	
	*/	
	for(y = dy; y < dy + dh; y++){
		int xnum = 0;			
		sx = ssx;			
		
		if(sy >= src->h || y >= dst->clip.y1)
			break;
		
		for(x = dx; x < dx + dw; x++) {
			
			if(sx >= src->w || x >= dst->clip.x1)
				break;
			
			/* FIXME: The clipping can probably be better */
			if(x >= dst->clip.x0 && x < dst->clip.x1 && y >= dst->clip.y0 && y < dst->clip.y1
				&& sx >= 0 && sx < src->w && sy >= 0 && sy < src->h) {
				int r = BM_GETR(src, sx, sy),
					g = BM_GETG(src, sx, sy),
					b = BM_GETB(src, sx, sy);
				if(!mask || r != src->r || g != src->g || b != src->b)
					BM_SET(dst, x, y, r, g, b);
			}
			
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

void bm_smooth(struct bitmap *b) {
	struct bitmap *tmp = bm_create(b->w, b->h);
	unsigned char *t = b->data;
	int x, y;
	assert(b->clip.y0 < b->clip.y1);
	assert(b->clip.x0 < b->clip.x1);
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
	if(r < 0) r = 0;
	if(r > 255) r = 255;
	if(g < 0) g = 0;
	if(g > 255) g = 255;
	if(b < 0) b = 0;
	if(b > 255) b = 255;
	bm->r = r;
	bm->g = g;
	bm->b = b;
}

/* Lookup table for bm_color_atoi() 
 * This list is based on the HTML and X11 colors on the
 * Wikipedia's list of web colors
 * http://en.wikipedia.org/wiki/Web_colors
 * Keep the list sorted because a binary search is used.
 * Keep the name in uppercase to keep bm_color_atoi() case insensitive.
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
		const char *q;
		char buffer[32], *p;
		for(q = text, p = buffer; *q && p - buffer < sizeof buffer - 1; q++, p++) {
			*p = toupper(*q);
		}
		*p = 0;
		
		int min = 0, max = ((sizeof color_map)/(sizeof color_map[0])) - 1;
		while(min <= max) {
			int i = (max + min) >> 1;
			int r = strcmp(buffer, color_map[i].name);
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

int bm_get_color_i(struct bitmap *bm) {
	return (bm->r << 16) | (bm->g << 8) | (bm->b << 0);
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
	if(x < b->clip.x0 || x >= b->clip.x1 || y < b->clip.y0 || y >= b->clip.y1) 
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
		if(x0 >= b->clip.x0 && x0 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1) 
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
	for(y = MAX(y0, b->clip.y0); y < MIN(y1 + 1, b->clip.y1); y++) {		
		for(x = MAX(x0, b->clip.x0); x < MIN(x1 + 1, b->clip.x1); x++) {
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
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Lower Left */
		xp = x0 - y; yp = y0 - x;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Upper Left */
		xp = x0 + x; yp = y0 - y;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Upper Right */
		xp = x0 + y; yp = y0 + x;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
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
			if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
				BM_SET(b, i, yp, b->r, b->g, b->b);
			yp = y0 - y;
			if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
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
		if(x1 >= b->clip.x0 && x1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x1, y0, b->r, b->g, b->b);	
		
		if(x0 >= b->clip.x0 && x0 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x0, y0, b->r, b->g, b->b);	
		
		if(x0 >= b->clip.x0 && x0 < b->clip.x1 && y1 >= b->clip.y0 && y1 < b->clip.y1)
			BM_SET(b, x0, y1, b->r, b->g, b->b);	
		
		if(x1 >= b->clip.x0 && x1 < b->clip.x1 && y1 >= b->clip.y0 && y1 < b->clip.y1)
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
		if(x0 - 1 >= b->clip.x0 && x0 - 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x0 - 1, y0, b->r, b->g, b->b);
		
		if(x1 + 1 >= b->clip.x0 && x1 + 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x1 + 1, y0, b->r, b->g, b->b);
		y0++;
		
		if(x0 - 1 >= b->clip.x0 && x0 - 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
			BM_SET(b, x0 - 1, y1, b->r, b->g, b->b);
		
		if(x1 + 1 >= b->clip.x0 && x1 + 1 < b->clip.x1 && y0 >= b->clip.y0 && y0 < b->clip.y1)
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
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Lower Left */
		xp = x0 - y + rad; yp = y1 - x - rad;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Upper Left */
		xp = x0 + x + rad; yp = y0 - y + rad;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
			BM_SET(b, xp, yp, b->r, b->g, b->b);
		
		/* Upper Right */
		xp = x1 + y - rad; yp = y0 + x + rad;
		if(xp >= b->clip.x0 && xp < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
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
			if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
				BM_SET(b, i, yp, b->r, b->g, b->b);
			yp = y0 - y + rad;
			if(i >= b->clip.x0 && i < b->clip.x1 && yp >= b->clip.y0 && yp < b->clip.y1)
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
	
	for(y = MAX(y0 + rad + 1, b->clip.y0); y < MIN(y1 - rad, b->clip.y1); y++) {
		for(x = MAX(x0, b->clip.x0); x <= MIN(x1,b->clip.x1 - 1); x++) {
			assert(x >= 0 && x < b->w && y >= 0 && y < b->h);
			BM_SET(b, x, y, b->r, b->g, b->b);
		}			
	}
}

/* Bexier curve with 3 control points.
 * See http://devmag.org.za/2011/04/05/bzier-curves-a-tutorial/
 * I tried the more optimized version at
 * http://members.chello.at/~easyfilter/bresenham.html
 * but that one had some caveats.
 */
void bm_bezier3(struct bitmap *b, int x0, int y0, int x1, int y1, int x2, int y2) {
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

void bm_fill(struct bitmap *b, int x, int y) {
	/* TODO: The function does not take the clipping into account. 
	 * I'm not really sure how to handle it, to be honest.
	 */	
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

static struct {
	const char *s;
	int i;
} font_names[] = {
	{"NORMAL", BM_FONT_NORMAL},
	{"BOLD", BM_FONT_BOLD},
	{"CIRCUIT", BM_FONT_CIRCUIT},
	{"HAND", BM_FONT_HAND},
	{"SMALL", BM_FONT_SMALL},
	{"SMALL_I", BM_FONT_SMALL_I},
	{"THICK", BM_FONT_THICK},
	{NULL, 0}
};

int bm_font_index(const char *name) {
	int i = 0;
	char buffer[12], *c = buffer;
	do {
		*c++ = toupper(*name++);
	} while(*name && c - buffer < sizeof buffer - 1);
	*c = '\0';
	
	while(font_names[i].s) {
		if(!strcmp(font_names[i].s, buffer)) {
			return font_names[i].i;
		}
		i++;
	}
	return BM_FONT_NORMAL;
}

const char *bm_font_name(int index) {
	int i = 0;
	while(font_names[i].s) {
		i++;
		if(font_names[i].i == index) {
			return font_names[i].s;
		}
	}
	return font_names[0].s;
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
	for(j = 0; j < 8 && y + j < b->clip.y1; j++) {
		if(y + j >= b->clip.y0) {
			char bits = b->font[byte];
			for(i = 0; i < 8 && x + i < b->clip.x1; i++) {
				if(x + i >= b->clip.x0 && !(bits & (1 << i))) {
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
	va_start(arg, fmt);
  	vsnprintf(buffer, sizeof buffer, fmt, arg);
  	va_end(arg);
	bm_puts(b, x, y, buffer);
}

void bm_putcs(struct bitmap *b, int x, int y, int s, char c) {
	int frow, fcol, byte;
	int i, j;
	if(c < 32 || c > 127) return;
	c -= 32;
	fcol = c >> 3;
	frow = c & 0x7;
	
	for(j = 0; j < (8 << s) && y + j < b->clip.y1; j++) {
		byte = frow * FONT_WIDTH + fcol + (j >> s) * (FONT_WIDTH >> 3);		
		if(y + j >= b->clip.y0) {
			char bits = b->font[byte];
			for(i = 0; i < (8 << s) && x + i < b->clip.x1; i++) {
				if(x + i >= b->clip.x0 && !(bits & (1 << (i >> s)))) {
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
	va_start(arg, fmt);
  	vsnprintf(buffer, sizeof buffer, fmt, arg);
  	va_end(arg);
	if(s > 0)
		bm_putss(b, x, y, s, buffer);
	else
		bm_puts(b, x, y, buffer);
}

