#include <stdio.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "LevelCanvas.h"

LevelCanvas::LevelCanvas(int x, int y, int w, int h, const char *l) 
: BMCanvas(x, y, w, h, l), _map(0), tc(0), layer(0) {
	for(int i = 0; i < 3; i++)
		visible[i] = true;
}

LevelCanvas::~LevelCanvas() {
	if(_map) {
		map_free(_map);
	}
}

int LevelCanvas::handle(int event) {
	switch(event) {
		case FL_RELEASE: {
			
				take_focus();
				
				if(!_map) 
					return 1;
				
				int mx = (Fl::event_x() - x())/zoom();
				int my = (Fl::event_y() - y())/zoom();
				
				int col = mx/_map->tw;
				int row = my/_map->th;	
							
				if(Fl::event_button() == FL_LEFT_MOUSE) {
					
					tileset *ts = tc->getTileset();
					if(!ts) 
						return 1;
					int tsi = ts_index_of( ts->name );
					if(tsi < 0) 
						return 1;
					
					int ti = tc->selectedIndex();
					map_set(_map, layer, col, row, tsi, ti);
				} else if(Fl::event_button() == FL_RIGHT_MOUSE) {
					map_set(_map, layer, col, row, 0, -1);
				}				
				
				redraw();
				return 1;
				
			}break;
		case FL_FOCUS : return 1;
	}	
	return Fl_Widget::handle(event);
}

void LevelCanvas::paint() {
	
	if(!_map) {
		return;
	}	
	
	pen("black");
	clear();
	pen("#808080");
	
	int w = _map->nc * _map->tw;
	int h = _map->nr * _map->th;
	
	for(int j = 0; j < _map->nr; j++) {
		line(0, j * _map->th, w, j * _map->th);
	}
	for(int j = 0; j < _map->nr; j++) {
		line(j * _map->tw, 0, j * _map->tw, h);
	}
	
	for(int i = 0; i < _map->nl; i++) {
		if(visible[i])
			map_render(_map, bmp, i, 0, 0);
	}
}

void LevelCanvas::newMap(int nr, int nc, int tw, int th, int nl) {
	if(_map)
		map_free(_map);
	_map = map_create(nr, nc, tw, th, nl);	
	if(!_map) {
		fl_alert("Error creating new map");
	}
	int w = tw * nc;
	int h = th * nr;
	resize(x(), y(), w, h);
}

void LevelCanvas::setMap(map *m) {
	if(_map)
		map_free(_map);
	_map = m; 
	int w = m->tw * m->nc;
	int h = m->th * m->nr;
	resize(x(), y(), w, h);
	
	redraw();
}

void LevelCanvas::setVisible(int layer, bool v) {
	visible[layer] = v;
	redraw();
}
