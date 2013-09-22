#ifndef LEVEL_CANVAS_H
#define LEVEL_CANVAS_H

#include "BMCanvas.h"
#include "TileCanvas.h"
#include "map.h"

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
	
private:
	map *_map;
	TileCanvas *tc;

	bool visible[3];

	int layer;
};

#endif