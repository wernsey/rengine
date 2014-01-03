#include <stdio.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "LevelCanvas.h"

#include "log.h"
#include "utils.h"

LevelCanvas::LevelCanvas(int x, int y, int w, int h, const char *l) 
: BMCanvas(x, y, w, h, l), _map(0), tc(0), layer(0), _drawBarriers(false), _drawMarkers(false) {
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
				if(!_map || !tc->getTileset()) 
					return 1;
				
				int mx = (Fl::event_x() - x())/zoom();
				int my = (Fl::event_y() - y())/zoom();
				
				dragStartX = mx/_map->tiles.tw;
				dragStartY = my/_map->tiles.th;
			}
			return 1;
		}
		case FL_LEAVE: {
			dragStartX = -1;
			dragStartY = -1;
			return 1;
		}
		case FL_DRAG: {
			int mx = (Fl::event_x() - x())/zoom();
			int my = (Fl::event_y() - y())/zoom();
			
			dragX = mx/_map->tiles.tw;
			dragY = my/_map->tiles.th;
			redraw();
			return 1;
		}
		case FL_RELEASE: {
			
				take_focus();
				
				if(!_map) 
					return 1;
				
				int mx = (Fl::event_x() - x())/zoom();
				int my = (Fl::event_y() - y())/zoom();
				
				int col = mx/_map->tiles.tw;
				int row = my/_map->tiles.th;	
							
				if(Fl::event_button() == FL_LEFT_MOUSE) {
					
					if(dragStartX < 0 || dragStartY < 0)
						return 1;
					
					tileset *ts = tc->getTileset();
					if(!ts) 
						return 1;
					int tsi = ts_index_of(&_map->tiles, ts->name );
					if(tsi < 0) 
						return 1;
					
					int ti = tc->selectedIndex();
					
					struct tile_meta *meta = ts_has_meta_ti(&_map->tiles, ts, ti);
					
					int x1 = MY_MIN(dragStartX, col), y1 = MY_MIN(dragStartY, row), 
						x2 = MY_MAX(dragStartX, col), y2 = MY_MAX(dragStartY, row);
					
					int x, y;
					for(y = y1; y <= y2; y++) {
						for(x = x1; x <= x2; x++) {						
							int ptsi, pti;
							map_get(_map, layer, x, y, &ptsi, &pti);
														
							if(ptsi == tsi && pti == ti) {
								/* You delete a tile by placing it again 
								(therefore, deleting a tile can also be done through a double click) */							
								map_set(_map, layer, x, y, 0, -1);
							} else {
								map_set(_map, layer, x, y, tsi, ti);
								struct map_cell * c = map_get_cell(_map, x, y);
								if(meta) {
									if(meta->flags) {
										c->flags = meta->flags;
									}
									if(meta->clas) {
										if(c->clas)
											free(c->clas);
										c->clas = strdup(meta->clas);
									}
								}
							}
						}
					}
					
					selCol = x - 1;
					selRow = y - 1;					
					if(select_callback)
						select_callback(this);
					
				} else if(Fl::event_button() == FL_RIGHT_MOUSE) {	
					
					selCol = mx/_map->tiles.tw;
					selRow = my/_map->tiles.th;
					
					if(select_callback)
						select_callback(this);
				}				
				
				redraw();
				return 1;
				
			}break;
		case FL_KEYDOWN : {
			int key = Fl::event_key();
			switch(key) {
				case FL_Delete : {
					
					/* Wipe the cell */
					int x = col(), y = row();
					map *m = getMap();
					if(!m) return 1;
					struct map_cell * c = map_get_cell(m, x, y);
					if(c->id)
						free(c->id);
					c->id = 0;
					if(c->clas)
						free(c->clas);
					c->clas = 0;
					c->flags = 0;
					
					for(int i = 0; i <= m->nl; i++) {
						struct map_tile *tile = &c->tiles[i];	
						tile->si = 0;
						tile->ti = -1;
					}
					
					if(select_callback)
						select_callback(this);
					
					redraw();					
				} return 1;
				case FL_BackSpace : {
					
					/* Clear the tile's ID, class and flags */
					int x = col(), y = row();
					map *m = getMap();
					if(!m) return 1;
					struct map_cell * c = map_get_cell(m, x, y);
					if(c->id)
						free(c->id);
					c->id = 0;
					if(c->clas)
						free(c->clas);
					c->clas = 0;
					c->flags = 0;
					
					if(select_callback)
						select_callback(this);
					
					redraw();	
				} return 1;
			}
			return 0;
		} break;
		case FL_FOCUS : return 1;
		case FL_UNFOCUS : return 1;
	}	
	return Fl_Widget::handle(event);
}

void LevelCanvas::paint() {
	if(!_map) {
		return;
	}	
	
	pen("black");
	clear();
	pen("#404040");
	
	int w = _map->nc * _map->tiles.tw;
	int h = _map->nr * _map->tiles.th;
	
	for(int j = 0; j < _map->nr; j++) {
		line(0, j * _map->tiles.th, w, j * _map->tiles.th);
	}
	for(int j = 0; j < _map->nc; j++) {
		line(j * _map->tiles.tw, 0, j * _map->tiles.tw, h);
	}
	
	for(int i = 0; i < _map->nl; i++) {
		if(visible[i])
			map_render(_map, bmp, i, 0, 0);
	}
	
	if(_drawBarriers || _drawMarkers) {
		for(int j = 0; j < _map->nr; j++)
			for(int i = 0; i < _map->nc; i++) {
				map_cell *c = map_get_cell(_map, i, j);
				
				int x0 = i * _map->tiles.tw, 
					y0 = j * _map->tiles.th, 
					x1 = (i + 1) * _map->tiles.tw, 
					y1 = (j + 1) * _map->tiles.th;
				
				if(_drawBarriers && (c->flags & TS_FLAG_BARRIER)) {
					pen("lime");
					for(int y = y0; y < y1; y++)
						for(int x = x0; x < x1; x++) {
							if((x + y) % 2) 
								putpixel(x, y);
						}
				}
				if(_drawMarkers) {
					x0++; y0++;
					x1-=2; y1-=2;
					if(c->id) {
						pen("Blue");
						line(x0, y0, x1 - 1, y0);
						line(x0, y0, x0, y1 - 1);
						line(x0, y1 - 1, x1 - 1, y0);
					}
					if(c->clas) {
						pen("Red");
						line(x0 + 1, y1, x1, y1);
						line(x1, y0 + 1, x1, y1);
						line(x0 + 1, y1, x1, y0 + 1);
					}
				}
			}
	}
	
	if(selRow >= 0 && selCol >= 0) {
		int x = selCol * _map->tiles.tw;
		int y = selRow * _map->tiles.th;
		pen("white");
		rect(x, y, x + _map->tiles.tw - 1, y +  _map->tiles.th - 1);
	}
	
	if(dragStartX >= 0 && dragStartY >= 0) {		
		int x1 = MY_MIN(dragStartX, dragX), 
			y1 = MY_MIN(dragStartY, dragY), 
			x2 = MY_MAX(dragStartX, dragX) + 1, 
			y2 = MY_MAX(dragStartY, dragY) + 1;
		pen("red");
		rect(x1 * _map->tiles.tw, y1 * _map->tiles.th, x2 * _map->tiles.tw, y2 * _map->tiles.th);
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
	int w = m->tiles.tw * m->nc;
	int h = m->tiles.th * m->nr;
	resize(x(), y(), w, h);
	
	redraw();
}

void LevelCanvas::setVisible(int layer, bool v) {
	visible[layer] = v;
	redraw();
}
