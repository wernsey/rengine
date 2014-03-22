#include <stdio.h>

#include <FL/Fl.H>

#include "TileCanvas.h"

TileCanvas::TileCanvas(int x, int y, int w, int h) 
: BMCanvas(x, y, w, h, "Tile View"), _map(0), selRow(0), selCol(0), 
	_drawBarriers(false), select_callback(0) {
	tiles = 0;
	zoom(1.0);
}

TileCanvas::~TileCanvas() {
	
}

int TileCanvas::handle(int event) {
	switch(event) {
		case FL_RELEASE: {
				take_focus();
				if(tiles && Fl::event_button() == FL_LEFT_MOUSE) {
					int mx = (Fl::event_x() - x())/zoom();
					int my = (Fl::event_y() - y())/zoom();
					
					selCol = mx/_map->tiles.tw;
					selRow = my/_map->tiles.th;
					
					if(select_callback)
						select_callback(this);
					
					redraw();
					
					return 1;
				} else if(Fl::event_button() == FL_RIGHT_MOUSE) {
					
				}
			}break;
		case FL_FOCUS : return 1;
		case FL_UNFOCUS : return 1;
	}
	return Fl_Widget::handle(event);
}

void TileCanvas::paint() {
	pen("black");
	clear();
	if(tiles) {
		blit(0, 0, tiles->bm, 0, 0, tiles->bm->w, tiles->bm->h);
		
		if(_drawBarriers) {
			pen("lime");
			for(int n = 0; n < tiles->nmeta; n++) {
				struct tile_meta *m = &tiles->meta[n];
				if(m->flags & TS_FLAG_BARRIER) {				
					int tr = tiles->bm->w / _map->tiles.tw;
					int row = m->ti / tr;
					int col = m->ti % tr;
					
					for(int y = row * _map->tiles.th; y < (row + 1) * _map->tiles.th; y++)
						for(int x = col * _map->tiles.tw; x < (col + 1) * _map->tiles.tw; x++) {
							if((x + y) % 2) 
								putpixel(x, y);
						}
				}			
			}	
		}
		
		if(selRow >= 0 && selCol >= 0) {
			int x = selCol * (_map->tiles.tw + tiles->border);
			int y = selRow * (_map->tiles.th + tiles->border);
			pen("white");
			rect(x, y, x + _map->tiles.tw - 1, y + _map->tiles.th - 1);
		}
	}
}

int TileCanvas::selectedIndex() {
	if(selRow < 0 || selCol < 0) {
		return -1;
	}
	int tr = tiles->bm->w / _map->tiles.tw;
	return selRow * tr + selCol;
}

void TileCanvas::deselect() {
	selRow = 0;
	selCol = 0;
	redraw();
}

void TileCanvas::setMap(map *m) { 
	_map = m;
}

void TileCanvas::setTileset(tileset *tiles) {	
	this->tiles = tiles;
	if(tiles) {
		resize(x(), y(), tiles->bm->w*zoom(), tiles->bm->h*zoom());
	}
}
