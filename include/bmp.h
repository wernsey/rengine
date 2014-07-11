/*! bmp.h
 *# Low-level routines to manipulate bitmap graphic files.\n
 *# It supports BMP files without any third party dependencies.
 *# PNG support is optional through libpng (http://www.libpng.org/pub/png/libpng.html). Use -DUSEPNG when compiling.
 *# JPG support is optional through libjpeg (http://www.ijg.org/). Use -DUSEJPG when compiling.
 *# 
 *@ References:
 *{
 ** http://en.wikipedia.org/wiki/BMP_file_format
 ** http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
 ** http://members.chello.at/~easyfilter/bresenham.html
 ** http://en.wikipedia.org/wiki/Flood_fill
 ** http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 ** http://web.archive.org/web/20110706093850/http://free.pages.at/easyfilter/bresenham.html
 ** http://damieng.com/blog/2011/02/20/typography-in-8-bits-system-fonts
 *} 
 *=
 *[
 *# Author: Werner Stoop
 *# This is free and unencumbered software released into the public domain.
 *# http://unlicense.org/
 *]
 *=
 */
 
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
 
/*@ struct bitmap
 *# Structure containing a bitmap image.
 */
struct bitmap {	
	/* Dimesions of the bitmap */
	int w, h;	
	
	/* The actual pixel data in RGBA format */
	unsigned char *data;
		
	/* Color for the pen, of the canvas */
	unsigned char r,g,b,a;
	
	/* XBM font. See font.xbm */
	const unsigned char *font;
	
	/* The width of eac character in the font */
	int font_spacing;
	
	/* Clipping Rectangle 
	 * (x0,y0) is inclusive.
	 * (x1,y1) is exclusive.
	 */
	struct {
		int x0, y0;
		int x1, y1;
	} clip;
};

/*@ struct bitmap *bm_create(int w, int h)
 *# Creates a bitmap of the specified dimensions
 */
struct bitmap *bm_create(int w, int h);

/*@ void bm_free(struct bitmap *b)
 *# Destroys a bitmap previously created with bm_create()
 */
void bm_free(struct bitmap *b);

/*@ struct bitmap *bm_load(const char *filename)
 *# Loads a bitmap file {{filename}} into a bitmap structure.\n
 *# It tries to detect the file type from the first bytes in the file.
 *# BMP support is always enabled, while JPG and PNG support is optional.\n
 *# Returns NULL if the file could not be loaded.
 */
struct bitmap *bm_load(const char *filename);

/*@ struct bitmap *bm_load_fp(FILE *f)
 *# Loads a bitmap from a {{FILE*}} that's already open.\n
 *# BMP support is always enabled, while JPG and PNG support is optional.\n
 *# Returns {{NULL}} if the file could not be loaded.
 */
struct bitmap *bm_load_fp(FILE *f);

/*@ int bm_save(struct bitmap *b, const char *fname)
 *# Saves the bitmap {{b}} to a BMP, JPG or PNG file named {{fname}}.\n
 *# If the filename contains {{".bmp"}} or {{".jpg"}} the file is 
 *# saved as a BMP or JPG, otherwise the PNG format is the default.
 *# It can only save to JPG or PNG if JPG or PNG support is enabled
 *# at compile time, otherwise it saves to a BMP file.\n
 *# Returns 1 on success, 0 on failure.
 */
int bm_save(struct bitmap *b, const char *fname);

/*@ struct bitmap *bm_copy(struct bitmap *b)
 *# Creates a duplicate of the bitmap structure 
 */
struct bitmap *bm_copy(struct bitmap *b);

/*@ void bm_flip_vertical(struct bitmap *b)
 *# Flips the bitmap vertically.
 */
void bm_flip_vertical(struct bitmap *b);

/*@ void bm_clip(struct bitmap *b, int x0, int y0, int x1, int y1)
 *# Sets the clipping rectangle on the bitmap from {{x0,y0}} (inclusive)
 *# to {{x1,y1}} exclusive.
 */
void bm_clip(struct bitmap *b, int x0, int y0, int x1, int y1);

/*@ void bm_unclip(struct bitmap *b)
 *# Resets the bitmap {{b}}'s clipping rectangle.
 */
void bm_unclip(struct bitmap *b);

/*@ void bm_set(struct bitmap *b, int x, int y, unsigned char R, unsigned char G, unsigned char B)
 *# Sets a pixel at x,y in the bitmap b to the specified R,G,B color
 */ 
void bm_set(struct bitmap *b, int x, int y, unsigned char R, unsigned char G, unsigned char B);

/*@ void bm_set_a(struct bitmap *b, int x, int y, unsigned char R, unsigned char G, unsigned char B, unsigned char A)
 *# Sets a pixel at x,y in the bitmap b to the specified R,G,B,A color
 */ 
void bm_set_a(struct bitmap *b, int x, int y, unsigned char R, unsigned char G, unsigned char B, unsigned char A);

/*@ unsigned char bm_getr(struct bitmap *b, int x, int y)
 *# Retrieves the R value of the pixel at x,y in bitmap b 
 */
unsigned char bm_getr(struct bitmap *b, int x, int y);

/*@ unsigned char bm_getg(struct bitmap *b, int x, int y)
 *# Retrieves the G value of the pixel at x,y in bitmap b 
 */
unsigned char bm_getg(struct bitmap *b, int x, int y);

/*@ unsigned char bm_getb(struct bitmap *b, int x, int y)
 *# Retrieves the B value of the pixel at x,y in bitmap b 
 */
unsigned char bm_getb(struct bitmap *b, int x, int y);

/*@ unsigned char bm_geta(struct bitmap *b, int x, int y)
 *# Retrieves the A (alpha) value of the pixel at x,y in bitmap b 
 */
unsigned char bm_geta(struct bitmap *b, int x, int y);

/*@ void bm_set_color(struct bitmap *bm, unsigned char r, unsigned char g, unsigned char b)
 *# Sets the colour of the pen to (r,g,b)
 */
void bm_set_color(struct bitmap *bm, unsigned char r, unsigned char g, unsigned char b);

/*@ void bm_set_alpha(struct bitmap *bm, int a)
 *# Sets the alpha value of the pen to {{a}}
 */
void bm_set_alpha(struct bitmap *bm, int a);

/*@ void bm_adjust_rgba(struct bitmap *bm, float rf, float gf, float bf, float af)
 *# Multiplies the RGBA values of each pixel in the bitmap with {{rf,gf,bf,af}}
 */
void bm_adjust_rgba(struct bitmap *bm, float rf, float gf, float bf, float af);

/*@ void bm_set_color_s(struct bitmap *bm, const char *text)
 *# Sets the colour of the pen to a colour represented by text.
 *# The text can be in the HTML format, like #RRGGBB or one of
 *# a variety of colour names, eg.: "white", "black", "red".
 *# See {{bm_color_atoi()}} for more details on the format of
 *# the string.
 */
void bm_set_color_s(struct bitmap *bm, const char *text);

/*@ int bm_color_atoi(const char *text)
 *# Converts a text string like "#FF00FF" or "white" to
 *# an integer of the form 0xFF00FF.
 *# The shorthand #RGB format is also supported
 *# (eg. #0fb, which is the same as #00FFBB)
 *# Additionally, it also supports the syntax "RGB(x,y,z)".
 *# The list of supported colors are based on the wikipedia's
 *# list of HTML and X11 Web colors:
 *# http://en.wikipedia.org/wiki/Web_colors
 */
int bm_color_atoi(const char *text);

/*@ void bm_set_color_i(struct bitmap *bm, int col)
 *# Sets the colour of the pen to a colour represented 
 *# by an integer, like 0x00RRGGBB
 */
void bm_set_color_i(struct bitmap *bm, int col);

/*@ void bm_get_color(struct bitmap *bc, int *r, int *g, int *b)
 *# Retrieves the pen colour into &r, &g and &b.
 */
void bm_get_color(struct bitmap *bm, int *r, int *g, int *b);

/*@ int bm_get_color_i(struct bitmap *bc)
 *# Retrieves the pen colour.
 */
int bm_get_color_i(struct bitmap *bm);

/*@ void bm_picker(struct bitmap *bm, int x, int y)
 *# Sets the colour of the pen to the colour of the pixel at <x,y>
 *# on the bitmap.
 *# The pen colour can then be retrieved through {{bm_get_color()}}.
 */
void bm_picker(struct bitmap *bm, int x, int y);

/*@ int bm_color_is(struct bitmap *bm, int x, int y, int r, int g, int b) 
 *# Returns 1 if the colour at <x,y> on the bitmap is (r,g,b), 0 otherwise
 */
int bm_color_is(struct bitmap *bm, int x, int y, int r, int g, int b); 

/*@ int bm_lerp(int color1, int color2, double t)
 *# Computes the color that is a distance {{t}} along the line between 
 *# color1 and color2.\n
 *# If {{t}} is 0 it returns color1. If t is 1.0 it returns color2.
 */
int bm_lerp(int color1, int color2, double t);

/*@ int bm_brightness(int color, double adj)
 *# Adjusts the brightness of a {{color}} by a factor of {{adj}}.\n
 *# That is {{adj > 1.0}} makes the color brighter, while
 *# {{adj < 1.0}} makes the color darker.
 */
int bm_brightness(int color, double adj);

/*@ struct bitmap *bm_fromXbm(int w, int h, unsigned char *data)
 *# Creates a struct bitmap object from XBM data 
 */
struct bitmap *bm_fromXbm(int w, int h, unsigned char *data);

/*@ void bm_blit(struct bitmap *dst, int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h)
 *# Blits an area of w*h pixels at sx,sy on the src bitmap to 
 *# dx,dy on the dst bitmap.
 */
void bm_blit(struct bitmap *dst, int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h);

/*@ void bm_maskedblit(struct bitmap *dst, int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h)
 *# Blits an area of w*h pixels at sx,sy on the src bitmap to 
 *# dx,dy on the dst bitmap.
 *# Pixels on the src bitmap that matches the src bitmap colour are not blitted.
 */
void bm_maskedblit(struct bitmap *dst, int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h);

/*@ void bm_blit_ex(struct bitmap *dst, int dx, int dy, int dw, int dh, struct bitmap *src, int sx, int sy, int sw, int sh, int mask)
 *# Extended blit function. Blits an area of sw*sh pixels at sx,sy from the {{src}} bitmap to 
 *# dx,dy on the {{dst}} bitmap into an area of dw*dh pixels, stretching or shrinking the blitted area as neccessary.
 *# If {{mask}} is non-zero, pixels on the src bitmap that matches the src bitmap colour are not blitted.
 */
void bm_blit_ex(struct bitmap *dst, int dx, int dy, int dw, int dh, struct bitmap *src, int sx, int sy, int sw, int sh, int mask);

/*@ void bm_smooth(struct bitmap *b)
 *# Smoothes the bitmap by essentially applying a 5x5 Gaussian filter.
 */
void bm_smooth(struct bitmap *b);

/*@ void bm_apply_kernel(struct bitmap *b, int dim, float kernel)
 *# Applies a {{dim}} x {{dim}} kernel to the image.
 */
void bm_apply_kernel(struct bitmap *b, int dim, float kernel[]);

/*@ struct bitmap *bm_resample(const struct bitmap *in, int nw, int nh)
 *# Creates a new bitmap of dimensions nw*nh that is a scaled
 *# (linearly resampled) version of the input bitmap. 
 *# The input bimap remains untouched.
 */
struct bitmap *bm_resample(const struct bitmap *in, int nw, int nh);

/*@ void bm_swap_colour(struct bitmap *b, unsigned char sR, unsigned char sG, unsigned char sB, unsigned char dR, unsigned char dG, unsigned char dB)
 *# Replaces all pixels of colour [sR,sG,sB] in bitmap b with the colour [dR,dG,dB]
 */
void bm_swap_colour(struct bitmap *b, unsigned char sR, unsigned char sG, unsigned char sB, unsigned char dR, unsigned char dG, unsigned char dB);

/*@ int bm_width(struct bitmap *b)
 *# Retrieves the width of the bitmap {{b}}
 */
int bm_width(struct bitmap *b);

/*@ int bm_height(struct bitmap *b)
 *# Retrieves the height of the bitmap {{b}}
 */
int bm_height(struct bitmap *b);

/*@ void bm_clear(struct bitmap *b)
 *# Clears the bitmap to the colour of the pen.
 */
void bm_clear(struct bitmap *b);

/*@ void bm_putpixel(struct bitmap *b, int x, int y)
 *# Draws a single pixel at <x,y> using the pen colour.
 */
void bm_putpixel(struct bitmap *b, int x, int y);

/*@ void bm_line(struct bitmap *b, int x0, int y0, int x1, int y1)
 *# Draws a line from <x0,y0> to <x1,y1> using the colour of the pen.
 */
void bm_line(struct bitmap *b, int x0, int y0, int x1, int y1);

/*@ void bm_rect(struct bitmap *b, int x0, int y0, int x1, int y1)
 *# Draws a rectangle from <x0,y0> to <x1,y1> using the pen colour
 */
void bm_rect(struct bitmap *b, int x0, int y0, int x1, int y1);

/*@ void bm_fillrect(struct bitmap *b, int x0, int y0, int x1, int y1)
 *# Draws a filled rectangle from <x0,y0> to <x1,y1> using the pen colour
 */
void bm_fillrect(struct bitmap *b, int x0, int y0, int x1, int y1);

/*@ void bm_dithrect(struct bitmap *b, int x0, int y0, int x1, int y1)
 *# Draws a dithered rectangle from <x0,y0> to <x1,y1> using the pen colour
 */
void bm_dithrect(struct bitmap *b, int x0, int y0, int x1, int y1);

/*@ void bm_circle(struct bitmap *b, int x0, int y0, int r)
 *# Draws a circle of radius {{r}} centered at <x,y> using the pen colour
 */
void bm_circle(struct bitmap *b, int x0, int y0, int r);

/*@ void bm_fillcircle(struct bitmap *b, int x0, int y0, int r)
 *# Draws a filled circle of radius {{r}} centered at <x,y> using the pen colour
 */
void bm_fillcircle(struct bitmap *b, int x0, int y0, int r);

/*@ void bm_ellipse(struct bitmap *b, int x0, int y0, int x1, int y1)
 *# Draws an ellipse that occupies the rectangle from <x0,y0> to 
 *# <x1,y1>, using the pen colour
 */
void bm_ellipse(struct bitmap *b, int x0, int y0, int x1, int y1);

/* MISSING: bm_fillellipse */

/*@ void bm_round_rect(struct bitmap *b, int x0, int y0, int x1, int y1, int r)
 *# Draws a rect from <x0,y0> to <x1,y1> using the pen colour with rounded corners 
 *# of radius {{r}}
 */
void bm_roundrect(struct bitmap *b, int x0, int y0, int x1, int y1, int r);

/*@ void bm_fill_round_rect(struct bitmap *b, int x0, int y0, int x1, int y1, int r)
 *# Draws a filled rect from <x0,y0> to <x1,y1> using the pen colour with rounded corners 
 *# of radius {{r}}
 */
void bm_fillroundrect(struct bitmap *b, int x0, int y0, int x1, int y1, int r);

/* FIXME: The bezier works kinda-sorta for specific values of the control points, but
 * it is unreliable, so I'd replace it with something more robust at a later stage.
 */
void bm_bezier3(struct bitmap *b, int x0, int y0, int x1, int y1, int x2, int y2);

/*@ void bm_fill(struct bitmap *b, int x, int y)
 *# Floodfills from <x,y> using the pen colour.\n
 *# The colour of the pixel at <x,y> is used as the source colour.
 *# The colour of the pen is used as the target colour.
 *N The function does not take the clipping into account.
 */
void bm_fill(struct bitmap *b, int x, int y);

/*@ bm_set_font(struct bitmap *b, const unsigned char *font, int spacing)
 *# Changes the font used to render text on the bitmap. {{font}} should be
 *# an XBM char array that should match the layout of font.xbm.
 *# {{spacing}} is the width of the characters in pixels (typically 6, 7 or 8).
 *# If you only want to change the spacing, set {{font}} to {{NULL}}.
 *# If you want to keep the current spacing, set it to {{0}}
 */
void bm_set_font(struct bitmap *b, const unsigned char *font, int spacing);

/*@ enum bm_fonts
 *# Built-in fonts that can be set with {{bm_std_font()}}
 *{
 ** {{BM_FONT_NORMAL}} - A default font.
 ** {{BM_FONT_BOLD}} - A bold font.
 ** {{BM_FONT_CIRCUIT}} - A "computer" font that looks like a circuit board.
 ** {{BM_FONT_HAND}} - A font that looks like hand writing.
 ** {{BM_FONT_SMALL}} - A small font.
 ** {{BM_FONT_SMALL_I}} - A variant of the small font with the foreground and background inverted.
 ** {{BM_FONT_THICK}} - A thicker variant of the normal font.
 *}
 */
enum bm_fonts {
	BM_FONT_NORMAL,
	BM_FONT_BOLD,
	BM_FONT_CIRCUIT,
	BM_FONT_HAND,
	BM_FONT_SMALL,
	BM_FONT_SMALL_I,
	BM_FONT_THICK
};

/*@ void bm_std_font(struct bitmap *b, enum bm_fonts font)
 *# Sets the font to one of the standard (built-in) ones.
 */
void bm_std_font(struct bitmap *b, enum bm_fonts font);

/*@ int bm_font_index(const char *name)
 *# Returns the index of on of the standard (built-in) fonts identified
 *# by the {{name}}.
 */
int bm_font_index(const char *name);

/*@ const char *bm_font_name(int index);
 *# Returns the name of the standard (built-in) font identified
 *# by {{index}}, which should fall in the range of the {{enum bm_fonts}}.
 */
const char *bm_font_name(int index);

/*@ int bm_text_width(struct bitmap *b, const char *s)
 *# Returns the width in pixels of a string of text.
 */
int bm_text_width(struct bitmap *b, const char *s);

/*@ int bm_text_height(struct bitmap *b, const char *s)
 *# returns the height in pixels of a string of text.
 */
int bm_text_height(struct bitmap *b, const char *s);

/*@ void bm_putc(struct bitmap *b, int x, int y, char c)
 *# Draws a single ASCII character {{c}} at position x,y on the bitmap.
 */
void bm_putc(struct bitmap *b, int x, int y, char c);

/*@ void bm_puts(struct bitmap *b, int x, int y, const char *s)
 *# Prints the string {{s}} at position x,y on the bitmap b.
 */
void bm_puts(struct bitmap *b, int x, int y, const char *text);

/*@ void bm_printf(struct bitmap *b, int x, int y, const char *fmt, ...)
 *# Prints a printf()-formatted style string to the bitmap b,
 *# {{fmt}} is a {{printf()}}-style format string (It uses vsnprintf() internally).
 */
void bm_printf(struct bitmap *b, int x, int y, const char *fmt, ...);

/*@ void bm_putcs(struct bitmap *b, int x, int y, int s, char c)
 *# Draws a single ASCII character {{c}} at position x,y on the bitmap,
 *# scaled by 2^s
 */
void bm_putcs(struct bitmap *b, int x, int y, int s, char c);

/*@ void bm_putss(struct bitmap *b, int x, int y, int s, const char *text)
 *# Prints the string {{s}} at position x,y on the bitmap b,
 *# scaled by 2^s.
 */
void bm_putss(struct bitmap *b, int x, int y, int s, const char *text);

/*@ void bm_printfs(struct bitmap *b, int x, int y, int s, const char *fmt, ...)
 *# Prints a printf()-formatted style string to the bitmap b,,
 *# scaled by 2^s
 *# {{fmt}} is a {{printf()}}-style format string (It uses vsnprintf() internally).
 */
void bm_printfs(struct bitmap *b, int x, int y, int s, const char *fmt, ...);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
