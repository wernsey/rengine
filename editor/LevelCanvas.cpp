#include <stdio.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "LevelCanvas.h"

#include "utils.h"

LevelCanvas::LevelCanvas(int x, int y, int w, int h, const char *l) 
: BMCanvas(x, y, w, h, l), _map(0), tc(0), layer(0), _drawBarriers(false) {
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
		case FL_PUSH: {
			take_focus();
			
			if(Fl::event_button() == FL_LEFT_MOUSE) {
				if(!_map) 
					return 1;
					
				int mx = (Fl::event_x() - x())/zoom();
				int my = (Fl::event_y() - y())/zoom();
				
				dragStartX = mx/_map->tw;
				dragStartY = my/_map->th;
			}
			return 1;
		}
		case FL_LEAVE: 
		{
			dragStartX = -1;
			dragStartY = -1;
			return 1;
		}
		case FL_DRAG: 
		{
			int mx = (Fl::event_x() - x())/zoom();
			int my = (Fl::event_y() - y())/zoom();
			
			dragX = mx/_map->tw;
			dragY = my/_map->th;
			redraw();
			return 1;
		}
		case FL_RELEASE: {
			
				take_focus();
				
				if(!_map) 
					return 1;
				
				int mx = (Fl::event_x() - x())/zoom();
				int my = (Fl::event_y() - y())/zoom();
				
				int col = mx/_map->tw;
				int row = my/_map->th;	
							
				if(Fl::event_button() == FL_LEFT_MOUSE) {
					
					if(dragStartX < 0 || dragStartY < 0)
						return 1;
					
					tileset *ts = tc->getTileset();
					if(!ts) 
						return 1;
					int tsi = ts_index_of( ts->name );
					if(tsi < 0) 
						return 1;
					
					int ti = tc->selectedIndex();
					
					int x1 = MY_MIN(dragStartX, col), y1 = MY_MIN(dragStartY, row), 
						x2 = MY_MAX(dragStartX, col), y2 = MY_MAX(dragStartY, row);
					
					for(int y = y1; y <= y2; y++) {
						for(int x = x1; x <= x2; x++) {						
							int ptsi, pti;
							map_get(_map, layer, x, y, &ptsi, &pti);
							
							if(ptsi == tsi && pti == ti) {
								/* You delete a tile by placing it again 
								(therefore, deleting a tile can also be done through a double click) */
								map_set(_map, layer, x, y, 0, -1);
							} else {
								map_set(_map, layer, x, y, tsi, ti);
							}
						}
					}
					
				} else if(Fl::event_button() == FL_RIGHT_MOUSE) {	
					
					selCol = mx/_map->tw;
					selRow = my/_map->th;
					
					if(select_callback)
						select_callback(this);
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
	
	if(_drawBarriers) {
		pen("green");
		for(int j = 0; j < _map->nr; j++)
			for(int i = 0; i < _map->nc; i++) {
				map_cell *c = map_get_cell(_map, i, j);
				if(c->flags & TS_FLAG_BARRIER) {
					for(int y = j * _map->th; y < (j + 1) * _map->th; y++)
						for(int x = i * _map->tw; x < (i + 1) * _map->tw; x++) {
							if((x + y) % 2) 
								putpixel(x, y);
						}
				}
			}
	}
	
	
	if(selRow >= 0 && selCol >= 0) {
		int x = selCol * _map->tw;
		int y = selRow * _map->th;
		pen("white");
		rect(x, y, x + _map->tw - 1, y +  _map->th - 1);
	}
	
	
	if(dragStartX >= 0 && dragStartY >= 0) {
		int x1 = dragStartX * _map->tw;
		int y1 = dragStartY * _map->th;
		int x2 = (dragX + 1) * _map->tw;
		int y2 = (dragY + 1) * _map->th;
		pen("red");
		rect(x1, y1, x2, y2);
		
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
