#ifndef LEVEL_CANVAS_H
#define LEVEL_CANVAS_H

#include "BMCanvas.h"
#include "TileCanvas.h"
#include "map.h"

class LevelCanvas;

typedef void (*level_select_callback)(LevelCanvas *canvas);

class LevelCanvas : public BMCanvas {
public:
	LevelCanvas(int x, int y, int w, int h, const char *l = "Level");

	virtual ~LevelCanvas();

	virtual int handle(int event);

	virtual void paint();

	void setTileCanvas(TileCanvas *tc) {this->tc = tc;}
	TileCanvas *getTileCanvas() { return tc; }

	void newMap(int nr, int nc, int tw, int th, int nl);
		
	void setLayer(int l) {layer = l;}
	int getLayer() { return layer; }

	map *getMap() { return _map; }
	void setMap(map *);
	
	void setVisible(int layer, bool v);
	
	void setSelectCallback(level_select_callback select_callback) {this->select_callback = select_callback;}
	
	int row() { return selRow; }
	int col() { return selCol; }
	
	bool drawBarriers() { return _drawBarriers;}
	void drawBarriers(bool d) { _drawBarriers = d;}
	bool drawMarkers() { return _drawMarkers;}
	void drawMarkers(bool d) { _drawMarkers = d;}	
	
private:
	map *_map;
	TileCanvas *tc;

	int dragStartX, dragStartY;
	int dragX, dragY;

	int selRow, selCol;

	bool visible[3];

	int layer;

	bool _drawBarriers;
	bool _drawMarkers;

	level_select_callback select_callback;
};

#endif