#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>

#include "editor.h"

// GLOBALS //////////////////////////////////////////////////////

char *map_file = NULL;

char *tileset_file = NULL;

int g_mapwidth = 20, g_mapheight = 20;
int g_tilewidth = 16, g_tileheight = 16;

// PROTOTYPES //////////////////////////////////////////////////////

void tileset_cb(Fl_Browser*w, void*p);
void tile_select_cb(TileCanvas *canvas);

// CALLBACKS //////////////////////////////////////////////////////

void quit_cb(Fl_Menu_* w, void*) {
	// FIXME: Changed? Save before exit (yes/no)	
	exit(0);
}

void tilesetadd_cb(Fl_Menu_* w, void*) {
	char * filename = fl_file_chooser("Choose Bitmap", "Bitmap files (*.bmp)", "", 1);
	if(filename != NULL) {
		if(ts_find(filename)) {
			fl_alert("Tileset %s is already\nin the collection.", filename);
			return;
		}		
		int index = ts_add(filename, g_tilewidth, g_tileheight, 0);
		if(index < 0) {
			fl_alert("Unable to load %s", filename);
			return;
		}
		
		tileSetSelect->add(filename);
		tileSetSelect->select(tileSetSelect->size());
		tileset_cb(tileSetSelect, NULL);
	}
}

void tileset_cb(Fl_Browser*w, void*p) {
	int i = w->value();
	struct tileset *ts = ts_find(w->text(i));
	
	if(!ts) return;
	
	tiles->setTileset(ts);
	
	tile_select_cb(tiles);
	
	tiles->parent()->redraw();
}

void tile_select_cb(TileCanvas *canvas) {
	tileset *ts = canvas->getTileset();	
	if(!ts) return;
	tile_meta *meta = ts_has_meta(ts, canvas->row(), canvas->col());
	if(meta) {
		tilesClass->value(meta->clas);
		tileIsBarrier->value(meta->flags & TS_FLAG_BARRIER);
	} else {
		tilesClass->value("");
		tileIsBarrier->value(0);
	}
}

void zoomOutCb(Fl_Button *w, void *p) {
	BMCanvas *c = static_cast<BMCanvas *>(w->user_data());
	if(c->zoom() > 1.0)
		c->zoom(c->zoom() - 1.0);
}

void zoomInCb(Fl_Button *w, void *p) {
	BMCanvas *c = static_cast<BMCanvas *>(w->user_data());
	if(c->zoom() < 10.0)
		c->zoom(c->zoom() + 1.0);
}

int main(int argc, char *argv[]) {
	
	Fl_Double_Window* win = make_window();
	
	win->show(argc, argv);
	
	return Fl::run();
}
