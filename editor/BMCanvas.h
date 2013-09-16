#ifndef BM_CANVAS_H
#define BM_CANVAS_H

#include <FL/Fl_Widget.H>

#include "bmp.h"

class BMCanvas : public Fl_Widget {
public:
	BMCanvas(int x, int y, int w, int h, const char *label = 0);

	virtual ~BMCanvas();

	virtual void draw();

	virtual void paint();

	virtual void resize(int x, int y, int w, int h);

	/* See, FL_Widget already has color() methods,
	 * So I needed something else for drawing on the
	 * bitmap.
	 */
	void pen(int r, int g, int b);
	void pen(int c);
	void pen(const char *s);
	
	void pen(int *r, int *g, int *b);
	
	void picker(int x, int y);
	
	void clear();
	
	void putpixel(int x, int y);
	
	void line(int x0, int y0, int x1, int y1);
	
	void rect(int x0, int y0, int x1, int y1);
	
	void fillrect(int x0, int y0, int x1, int y1);
	
	void circle(int x, int y, int r);
	
	void smooth();
	
	void blit(int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h);

	void maskedblit(int dx, int dy, struct bitmap *src, int sx, int sy, int w, int h);

	void zoom(float z);
	float zoom();
	
	bitmap *getBitmap();

protected:
	bitmap *bmp;

private:
	float _zoom;
};

#endif
