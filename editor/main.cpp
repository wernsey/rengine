#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "editor.h"
#include "log.h"

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
	tileSetSelect->clear();
	tiles->setMap(canvas->getMap());
	tiles->setTileset(NULL);
	tiles->redraw();
	
	new_map_dlg->hide();
}

void new_btn_cancel_cb(Fl_Button* w, void*) {
	new_map_dlg->hide();
}

void new_cb(Fl_Menu_* w, void*) {	
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
	if(!m) {
		fl_alert("Unable to open map %s", filename);
		return;
	}
	
	map_file = filename;
	canvas->setMap(m);
	tiles->setMap(canvas->getMap());
	tiles->setTileset(NULL);
	
	tileSetSelect->clear();
	tile_collection *tc = &canvas->getMap()->tiles;
	for(int i = 0; i < ts_get_num(tc); i++) {
		tileset *ts = ts_get(tc, i);
		tileSetSelect->add(ts->name);
	}
}

void save_cb(Fl_Menu_* w, void*p) {
	map *m = canvas->getMap();
	if(!m) {
		rlog("Not saving map, as none is defined yet.");
		return;
	}
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

void chdir_cb(Fl_Menu_* w, void*) {	
	// FIXME: Do we change directories with a map being edited?
	char cwd[128];	
	getcwd(cwd, sizeof cwd);	
	const char *dir = fl_dir_chooser("Choose Working Directory", cwd, 0);
	if(dir) {
		char buffer[128];
		snprintf(buffer, sizeof buffer, "Editor - %s", dir);
		main_window->label(buffer);
		chdir(dir);
	}
}

void quit_cb(Fl_Menu_* w, void*) {
	// FIXME: Changed? Save before exit (yes/no)	
	exit(0);
}

/*****************************************************************************************
 * TILE SET MENU ************************************************************************/

void tilesetadd_cb(Fl_Menu_* w, void*) {
	if(!canvas->getMap()) return;
	char * filename = fl_file_chooser("Choose Bitmap", "Bitmap files (*.bmp)", "", 1);
	if(filename != NULL) {
		tile_collection *tc = &canvas->getMap()->tiles;
		if(ts_find(tc, filename)) {
			fl_alert("Tileset %s is already\nin the collection.", filename);
			return;
		}		
		int index = ts_add(tc, filename);
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
	if(!canvas->getMap()) return;
	tile_collection *tc = &canvas->getMap()->tiles;
	if(tileset_file)
		ts_save_all(tc, tileset_file);
	else
		tilesetsaveas_cb(w, p);
}

void tilesetsaveas_cb(Fl_Menu_* w, void*) {
	if(!canvas->getMap()) return;
	char * filename = fl_file_chooser("Choose Filename For Tileset", "Tileset files (*.tls)", "", 1);	
	if(filename != NULL) {
		tile_collection *tc = &canvas->getMap()->tiles;
		if(ts_save_all(tc, filename)) {			
			tileset_file = filename;
		} else {
			fl_alert("Unable to export tileset to %s", filename);
		}
	}
}

void tilesetload_cb(Fl_Menu_* w, void*) {	
	if(!canvas->getMap()) return;
	tile_collection *tc = &canvas->getMap()->tiles;
	int start = ts_get_num(tc);
	char * filename = fl_file_chooser("Choose Tileset", "Tileset files (*.tls)", "", 1);
	if(!filename)
		return;
	if(!ts_load_all(tc, filename)) {
		fl_alert("Unable to import tileset %s", filename);
		return;
	}
	tileset_file = filename;
	for(int i = start; i < ts_get_num(tc); i++) {
		tileset *ts = ts_get(tc, i);
		tileSetSelect->add(ts->name);
	}
}

/*****************************************************************************************
 * MAP WIDGETS **************************************************************************/

void mapClass_cb(Fl_Input *in, void*) {
	int x = canvas->col(), y = canvas->row();
	map *m = canvas->getMap();
	if(!m) return;
	struct map_cell * c = map_get_cell(m, x, y);
	if(c->clas)
		free(c->clas);
	c->clas = strdup(mapClass->value());
}

void mapId_cb(Fl_Input *in, void*) {
	int x = canvas->col(), y = canvas->row();
	map *m = canvas->getMap();
	if(!m) return;
	struct map_cell * c = map_get_cell(m, x, y);
	if(c->id)
		free(c->id);
	c->id = strdup(mapId->value());
}

void mapBarrier_cb(Fl_Check_Button*, void*) {
	int x = canvas->col(), y = canvas->row();
	map *m = canvas->getMap();
	struct map_cell * c = map_get_cell(m, x, y);
	
	if(mapBarrier->value())
		c->flags |= TS_FLAG_BARRIER;
	else
		c->flags &= ~TS_FLAG_BARRIER;
	
	if(canvas->drawBarriers())
		canvas->redraw();
}

void map_select_cb(LevelCanvas *canvas) {	
	int x = canvas->col(), y = canvas->row();
	char buffer[128];
	
	map *m = canvas->getMap();
	struct map_cell * c = map_get_cell(m, x, y);
	
	mapId->value(c->id?c->id:"");
	mapClass->value(c->clas?c->clas:"");
	mapBarrier->value(c->flags & TS_FLAG_BARRIER);
	
	char sub[3][20];
	for(int i = 0; i < 3; i++) {
		int si, ti;		
		map_get(m, i, x, y, &si, &ti);		
		snprintf(sub[i], sizeof sub[i], "l:%d si:%d ti:%d", i, si, ti);
	}	
	snprintf(buffer, sizeof buffer, "x:%d y:%d [%s] [%s] [%s]", canvas->col(), canvas->row(), sub[0], sub[1], sub[2]);
	
	mapStatus->value(buffer);
}

void mapDrawBarrier_cb(Fl_Check_Button *b, void*) {
	canvas->drawBarriers(b->value() == 1);		
	canvas->redraw();
}

/*****************************************************************************************
 * TILE SET WIDGETS *********************************************************************/

void tileset_cb(Fl_Browser*w, void*p) {
	int i = w->value();
	tile_collection *tc = &canvas->getMap()->tiles;
	struct tileset *ts = ts_find(tc, w->text(i));
	
	if(!ts) 
		return;
	
	tiles->setTileset(ts);
	
	tile_select_cb(tiles);
	
	tiles->parent()->redraw();
}

void tile_select_cb(TileCanvas *tileCanvas) {
	tileset *ts = tileCanvas->getTileset();	
	if(!ts) return;
	tile_collection *tc = &canvas->getMap()->tiles;
	tile_meta *meta = ts_has_meta(tc, ts, canvas->row(), canvas->col());
	if(meta) {
		tilesClass->value(meta->clas);
		tileIsBarrier->value(meta->flags & TS_FLAG_BARRIER);
	} else {
		tilesClass->value("");
		tileIsBarrier->value(0);
	}
	char buffer[128];
	snprintf(buffer, sizeof buffer, "si: %d ti:%d", ts_index_of(tc, ts->name ), tiles->selectedIndex());
	tilesStatus->value(buffer);
}

void tileClass_cb(Fl_Input*in, void*p) {
		
	if(!ts_valid_class(in->value())) {
		fl_alert("Class must be alphanumeric and less than %d characters", TS_CLASS_MAXLEN);
		return;
	}
	
	tileset *ts = tiles->getTileset();	
	if(!ts) return;
	
	tile_collection *tc = &canvas->getMap()->tiles;
	tile_meta *meta = ts_get_meta(tc, ts, tiles->row(), tiles->col());
	if(!meta) {
		fl_alert("out of memory :(");
		return;
	} 
	
	free(meta->clas);
	meta->clas = strdup(in->value());
}

void tileBarrier_cb(Fl_Check_Button*w, void*p) {
	tile_collection *tc = &canvas->getMap()->tiles;
	tileset *ts = tiles->getTileset();	
	if(!ts) return;
	
	tile_meta *meta = ts_get_meta(tc, ts, tiles->row(), tiles->col());
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
	
	log_init("editor.log");
	atexit(log_deinit);
	
	rlog("Starting editor.");
	
	make_window();
	
	tiles->setSelectCallback(tile_select_cb);
		
	canvas->setSelectCallback(map_select_cb);
	canvas->setTileCanvas(tiles);	
	canvas->setLayer(0);
	
	main_window->show(argc, argv);
	
	return Fl::run();
}
