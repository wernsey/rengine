#ifndef TILE_CANVAS_H
#define TILE_CANVAS_H

#include "BMCanvas.h"
#include "tileset.h"

class TileCanvas;

typedef void (*tile_select_callback)(TileCanvas *canvas);

class TileCanvas : public BMCanvas {
public:
	TileCanvas(int x, int y, int w, int h);

	virtual ~TileCanvas();

	virtual int handle(int event);

	virtual void paint(); 

	void setTileset(tileset *tiles);

	tileset *getTileset() {return tiles;}
	
	void setSelectCallback(tile_select_callback select_callback) {this->select_callback = select_callback;}

	int row() { return selRow; }
	int col() { return selCol; }
	
	int selectedIndex();
	
	void deselect();
	
	bool drawBarriers() { return _drawBarriers;}
	void drawBarriers(bool d) { _drawBarriers = d;}
	
private:
	tileset *tiles;

	int selRow, selCol;

	bool _drawBarriers;

	tile_select_callback select_callback;
};

#endif