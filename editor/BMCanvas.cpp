#include <FL/fl_draw.H>

#include "BMCanvas.h"

BMCanvas::BMCanvas(int x, int y, int w, int h, const char *label) 
: Fl_Widget(x, y, w, h, label), _zoom(1.0) {
	bmp = bm_create(w, h);
}

BMCanvas::~BMCanvas() {
	bm_free(bmp);
}

static void Draw_Image_Cb(void *data, int x, int y, int w, uchar *buf)
{	
	BMCanvas *canvas = static_cast<BMCanvas *>(data);
	bitmap *bmp = canvas->getBitmap();
	float zoom = canvas->zoom();
	if(!bmp) return;
	while(w--) {
		*buf++ = bm_getr(bmp, x/zoom, y/zoom);
		*buf++ = bm_getg(bmp, x/zoom, y/zoom);
		*buf++ = bm_getb(bmp, x/zoom, y/zoom);
		x++;
	}
}

void BMCanvas::draw()
{
	paint();
	fl_draw_image(Draw_Image_Cb, this, x(), y(), w(), h());
}

void BMCanvas::resize(int x, int y, int w, int h)
{
	bm_free(bmp);
	bmp = bm_create(w, h);
	Fl_Widget::resize(x, y, w, h);
}

void BMCanvas::paint() {
}

void BMCanvas::zoom(float z) { 
	
	if(z < 0.005) z = 1.0;
	
	float factor = (z/_zoom);
	
	_zoom = z; 
	
	resize(x(), y(), w() * factor, h() * factor);
	parent()->redraw();
}

float BMCanvas::zoom() { 
	return _zoom; 
}

bitmap *BMCanvas::getBitmap() { 
	return bmp; 
}

void BMCanvas::pen(int r, int g, int b)
{
	bm_set_color(bmp, r, g, b);
}

void BMCanvas::pen(int c)
{
	bm_set_color_i(bmp, c);
}

void BMCanvas::pen(const char *s)
{
	bm_set_color_s(bmp, s);
}

void BMCanvas::pen(int *r, int *g, int *b)
{
	bm_get_color(bmp, r, g, b);
}

void BMCanvas::picker(int x, int y) {
	bm_picker(bmp, x, y);
}

void BMCanvas::clear() {
	bm_clear(bmp);
}

void BMCanvas::putpixel(int x, int y) {
	bm_putpixel(bmp, x, y);
}

void BMCanvas::line(int x0, int y0, int x1, int y1) {
	bm_line(bmp, x0, y0, x1, y1);
}

void BMCanvas::rect(int x0, int y0, int x1, int y1) {
	bm_rect(bmp, x0, y0, x1, y1);
}

void BMCanvas::fillrect(int x0, int y0, int x1, int y1) {
	bm_fillrect(bmp, x0, y0, x1, y1);
}

void BMCanvas::circle(int x, int y, int r) {
	bm_circle(bmp, x, y, r);
}

void BMCanvas::smooth() {
	bm_smooth(bmp);
}

void BMCanvas::blit(int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h) {
	bm_blit(bmp, dx, dy, src, sx, sy, w, h);
}

void BMCanvas::maskedblit(int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h) {
	bm_maskedblit(bmp, dx, dy, src, sx, sy, w, h);
}
	