#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "editor.h"

/*****************************************************************************************
 * GLOBALS ******************************************************************************/

char *map_file = NULL;

char *tileset_file = NULL;

int g_mapwidth = 20, g_mapheight = 20;
int g_tilewidth = 16, g_tileheight = 16;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/*****************************************************************************************
 * PROTOTYPES ***************************************************************************/

void tileset_cb(Fl_Browser*w, void*p);
void tile_select_cb(TileCanvas *canvas);

/*****************************************************************************************
 * FILE MENU CALLBACKS ******************************************************************/

void new_btn_ok_cb(Fl_Button* w, void*) {
	g_mapwidth = MAX((new_map_width->value()), 1);
	g_mapheight = MAX((new_map_height->value()), 1);
	g_tilewidth = MAX((new_tile_width->value()), 1);
	g_tileheight = MAX((new_tile_height->value()), 1);
		
	canvas->newMap(g_mapheight, g_mapwidth, g_tilewidth, g_tileheight, 3);
	canvas->redraw();
	canvas->parent()->redraw();
	
	tileset_file = NULL;
	ts_free_all();
	tileSetSelect->clear();
	tiles->setTileset(NULL);
	tiles->redraw();
	
	new_map_dlg->hide();
}

void new_btn_cancel_cb(Fl_Button* w, void*) {
	new_map_dlg->hide();
}

void new_cb(Fl_Menu_* w, void*) {
	char buffer[30];
	
	new_map_width->value(g_mapwidth);
	
	new_map_height->value(g_mapheight);
	
	new_tile_width->value(g_tilewidth);
	
	new_tile_height->value(g_tileheight);
	
	new_map_dlg->show();
}

void open_cb(Fl_Menu_* w, void*) {
	char * filename = fl_file_chooser("Choose Map", "Map files (*.map)", "", 1);
	if(!filename)
		return;
	
	struct map * m = map_load(filename);
	if(!m)
		fl_alert("Unable to open map %s", filename);
	
	map_file = filename;
	canvas->setMap(m);
	
	tileSetSelect->clear();
	for(int i = 0; i < ts_get_num(); i++) {
		tileset *ts = ts_get(i);
		tileSetSelect->add(ts->name);
	}
}

void save_cb(Fl_Menu_* w, void*p) {
	map *m = canvas->getMap();
	if(!m) 
		return;
	if(map_file)
		map_save(m, map_file);
	else
		saveas_cb(w, p);
}

void saveas_cb(Fl_Menu_* w, void*) {
	map *m = canvas->getMap();
	if(!m) 
		return;
	char * filename = fl_file_chooser("Choose Filename For Map", "Map files (*.map)", "", 1);	
	if(filename != NULL) {
		if(map_save(m, filename)) {			
			map_file = filename;
		} else {
			fl_alert("Unable to save map to %s", filename);
		}
	}
}

void quit_cb(Fl_Menu_* w, void*) {
	// FIXME: Changed? Save before exit (yes/no)	
	exit(0);
}

/*****************************************************************************************
 * TILE SET MENU ************************************************************************/

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

void tilesetsave_cb(Fl_Menu_* w, void*p) {
	if(tileset_file)
		ts_save_all(tileset_file);
	else
		tilesetsaveas_cb(w, p);
}

void tilesetsaveas_cb(Fl_Menu_* w, void*) {
	char * filename = fl_file_chooser("Choose Filename For Tileset", "Tileset files (*.tls)", "", 1);	
	if(filename != NULL) {
		if(ts_save_all(filename)) {			
			tileset_file = filename;
		} else {
			fl_alert("Unable to export tileset to %s", filename);
		}
	}
}

void tilesetload_cb(Fl_Menu_* w, void*) {	
	int start = ts_get_num();
	char * filename = fl_file_chooser("Choose Tileset", "Tileset files (*.tls)", "", 1);
	if(!filename)
		return;
	
	if(!ts_load_all(filename)) {
		fl_alert("Unable to import tileset %s", filename);
		return;
	}
	tileset_file = filename;
	for(int i = start; i < ts_get_num(); i++) {
		tileset *ts = ts_get(i);
		tileSetSelect->add(ts->name);
	}
}

/*****************************************************************************************
 * TILE SET WIDGETS *********************************************************************/

void tileset_cb(Fl_Browser*w, void*p) {
	int i = w->value();
	struct tileset *ts = ts_find(w->text(i));
	
	if(!ts) 
		return;
	
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
	char buffer[128];
	snprintf(buffer, sizeof buffer, "si: %d ti:%d", ts_index_of( ts->name ), tiles->selectedIndex());
	tilesStatus->value(buffer);
}

void tileClass_cb(Fl_Input*w, void*p) {
	Fl_Input *in = static_cast<Fl_Input *>(w);
		
	if(!ts_valid_class(in->value())) {
		fl_alert("Class must be alphanumeric and less than %d characters", TS_CLASS_MAXLEN);
		return;
	}
	
	tileset *ts = tiles->getTileset();	
	if(!ts) return;
	
	tile_meta *meta = ts_get_meta(ts, tiles->row(), tiles->col());
	if(!meta) {
		fl_alert("out of memory :(");
		return;
	} 
	
	free(meta->clas);
	meta->clas = strdup(in->value());
}

void tileBarrier_cb(Fl_Check_Button*w, void*p) {
	tileset *ts = tiles->getTileset();	
	if(!ts) return;
	
	tile_meta *meta = ts_get_meta(ts, tiles->row(), tiles->col());
	if(!meta) {
		fl_alert("out of memory :(");
		return;
	} 
	
	if(tileIsBarrier->value())
		meta->flags |= TS_FLAG_BARRIER;
	else
		meta->flags &= ~TS_FLAG_BARRIER;
	
	if(tiles->drawBarriers())
		tiles->redraw();
}

void tileDrawBarrier_cb(Fl_Check_Button*w, void*p) {
	tiles->drawBarriers(w->value() == 1);		
	tiles->redraw();
}

/*****************************************************************************************
 * OTHER CALLBACKS **********************************************************************/

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

/*****************************************************************************************
 * THE MAIN FUNCTION ********************************************************************/

int main(int argc, char *argv[]) {
	
	make_window();
	
	tiles->setSelectCallback(tile_select_cb);
	
	canvas->setTileCanvas(tiles);
	
	canvas->setLayer(0);
	
	main_window->show(argc, argv);
	
	return Fl::run();
}
