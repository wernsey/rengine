#ifndef LEVEL_CANVAS_H
#define LEVEL_CANVAS_H

#include "BMCanvas.h"
#include "TileCanvas.h"
#include "map.h"

class LevelCanvas : public BMCanvas {
public:
	LevelCanvas(int x, int y, int w, int h);

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
	
private:
	map *_map;
	TileCanvas *tc;

	int layer;
};

#endif