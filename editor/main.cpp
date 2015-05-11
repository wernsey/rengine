#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Color_Chooser.H>

#include <errno.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "editor.h"
#include "log.h"
#include "paths.h"

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
		
	canvas->zoom(1.0);
	canvas->newMap(g_mapheight, g_mapwidth, g_tilewidth, g_tileheight, 3);
	canvas->parent()->redraw();
	
	tileset_file = NULL;
	tileSetSelect->clear();
	tiles->setMap(canvas->getMap());
	tiles->setTileset(NULL);
	tiles->redraw();
	
	if(map_file) free(map_file);
	map_file = NULL;
	
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
	char * filepath = fl_file_chooser("Choose Map", "Map files (*.map)", "", 1);
	if(!filepath)
		return;
	
	char file_dir[FL_PATH_MAX];
	
	const char *file = fl_filename_name(filepath);
	char *dir_end = strrchr(filepath, '/');
	if(dir_end) {		
		strncpy(file_dir, filepath, dir_end - filepath);
		file_dir[dir_end - filepath] = '\0';
		rlog("Changing working directory to %s", file_dir);
		if(chdir(file_dir)) {
			rerror("chdir: %s\n", strerror(errno));
		}		
		if(!getcwd(file_dir, sizeof file_dir)) {
			rerror("getcwd: %s\n", strerror(errno));
		}
	} else {
		strcpy(file_dir, "");
	}
	
	rlog("Opening map file %s", filepath);
	
	struct map * m = map_load(file, 1);
	if(!m) {
		fl_alert("Unable to open map %s", filepath);
		return;
	}
	
	char path[FL_PATH_MAX];
	if(!getcwd(path, sizeof path)) {
		rerror("getcwd: %s\n", strerror(errno));
	} else {
		rlog("Working directory is now '%s'", path);
	}
	
	if(map_file) free(map_file);
	
	/* At this point, 
	file_dir contains something like 'D:\sandbox\rengine\bin\game\maps'
	path contains something like 'D:\sandbox\rengine\bin\game'
	and we want map_file to be maps\thismap.map relative to the game\ directory
	*/
	rlog("Map paths: '%s' '%s'", path, file_dir);
	char buffer[FL_PATH_MAX];
	int len = sizeof buffer;
	if(file_dir[0] && relpath(path, file_dir, buffer, len)) {
		len -= strlen(buffer);
		strncat(buffer, file, len);
		for(char *c = buffer; c[0]; c++)
			if(c[0] == '\\')
				c[0] = '/';	
		map_file = strdup(buffer);
		/* I had the idea once to store the absolute path in map_file, but
			it broke down when the relative path is computed later. */
	} else {
		map_file = strdup(filepath);
	}
	rlog("Map file is now '%s'", map_file);
	
	canvas->setMap(m);
	tileSetSelect->clear();
	tiles->setMap(canvas->getMap());
	tiles->setTileset(NULL);
	tiles->redraw();
	
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
	if(map_file) {
		map_save(m, map_file);
	} else
		saveas_cb(w, p);
}

void saveas_cb(Fl_Menu_* w, void*) {
	map *m = canvas->getMap();
	if(!m) 
		return;
	char * filename = fl_file_chooser("Choose Filename For Map", "Map files (*.map)", "", 1);	
	if(filename != NULL) {
		if(map_save(m, filename)) {		
			if(map_file) free(map_file);
			map_file = strdup(filename);
		} else {
			fl_alert("Unable to save map to %s", filename);
		}
	}
}

void chdir_cb(Fl_Menu_* w, void*) {	
	// FIXME: Do we change directories with a map being edited?
	// You need to take the "New Map" option into account as well.
	char cwd[128];	
	if(!getcwd(cwd, sizeof cwd)) {
		rerror("getcwd: %s\n", strerror(errno));
	}
	const char *dir = fl_dir_chooser("Choose Working Directory", cwd, 0);
	if(dir) {
		rlog("Changing working directory to %s", dir);
		char buffer[128];
		snprintf(buffer, sizeof buffer, "Editor - %s", dir);
		main_window->label(buffer);
		if(chdir(dir)) {
			rerror("chdir: %s\n", strerror(errno));
		}
	}
}

// http://stackoverflow.com/questions/8131510/fltk-closing-window
void main_window_callback(Fl_Widget *w, void *p) {
	if (Fl::event()==FL_SHORTCUT && Fl::event_key()==FL_Escape) 
		return;
	
	int result = 0;

	result = fl_choice("Do you want to save before quitting?", 
                           "Don't Save",  // 0
                           "Save",        // 1
                           "Cancel"       // 2
                           );
    if (result == 0) {  
		// Close without saving
        main_window->hide();
    } else if (result == 1) {  
		// Save and close       
		save_cb(NULL, p);	
        main_window->hide();
    } else if (result == 2) {
		// Cancel
    }
}

void quit_cb(Fl_Menu_* w, void *p) {
	main_window->do_callback(w, p);
}

/*****************************************************************************************
 * TILE SET MENU ************************************************************************/

void tilesetadd_cb(Fl_Menu_* w, void*) {
	if(!canvas->getMap()) return;
	char * filename = fl_file_chooser("Choose Bitmap", "Bitmap files (*.bmp)\tPNG files (*.png)", "", 1);
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

void tileset_reload_img_cb(Fl_Menu_*, void*) {
	tileset *ts = tiles->getTileset();	
	if(!ts) 
		return;
	bitmap *bm = ts->bm;
	unsigned int mask = bm_get_color(bm);
	
	ts->bm = bm_load(ts->name);
	if(ts->bm) {
		bm_set_color(ts->bm, mask);
		bm_free(bm);
		canvas->redraw();
		tiles->redraw();
	} else {
		ts->bm = bm;
		fl_alert("Unable to reload tileset %s", ts->name);
	}
}

void tileset_props_cb(Fl_Menu_*, void*) {	
	tileset *ts = tiles->getTileset();	
	if(!ts) 
		return;
	
	int r, g, b;
	
	bm_get_color_rgb(ts->bm, &r, &g, &b);
	mask_color_box->color(fl_rgb_color(r,g,b));
	
	tile_border_input->value(ts->border);
	
	tile_props_dlg->show();
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
	if(strlen(mapClass->value()) == 0)
		c->clas = NULL;
	else
		c->clas = strdup(mapClass->value());
	if(canvas->drawBarriers())
		canvas->redraw();
}

void mapId_cb(Fl_Input *in, void*) {
	int x = canvas->col(), y = canvas->row();
	map *m = canvas->getMap();
	if(!m) return;
	struct map_cell * c = map_get_cell(m, x, y);
	if(c->id)
		free(c->id);
	if(strlen(mapId->value()) == 0)
		c->id = NULL;
	else
		c->id = strdup(mapId->value());
	if(canvas->drawBarriers())
		canvas->redraw();
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
	snprintf(buffer, sizeof buffer, "r:%d c:%d [%s] [%s] [%s]", canvas->row() + 1, canvas->col() + 1, sub[0], sub[1], sub[2]);
	
	mapStatus->value(buffer);
}

void mapDrawBarrier_cb(Fl_Check_Button *b, void*) {
	canvas->drawBarriers(b->value() == 1);		
	canvas->redraw();
}

void mapDrawMarkers_cb(Fl_Check_Button *b, void*) {
	canvas->drawMarkers(b->value() == 1);		
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
	rlog("TileCanvas selected");
	tileset *ts = tileCanvas->getTileset();	
	if(!ts) return;
	tile_collection *tc = &canvas->getMap()->tiles;
	tile_meta *meta = ts_has_meta(tc, ts, tileCanvas->row(), tileCanvas->col());
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
	if(strlen(in->value()) > 0)
		meta->clas = strdup(in->value());
	else
		meta->clas = NULL;
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
 * TILESET PROPERTIES CALLBACKS *********************************************************/

void tile_props_ok_cb(Fl_Button*, void*) {
	unsigned int col = mask_color_box->color() >> 8;
	
	tileset *ts = tiles->getTileset();	
	if(ts) {
		bm_set_color(ts->bm, col);
		ts->border = tile_border_input->value();
	}
	
	tile_props_dlg->hide();
	canvas->redraw();
	tiles->redraw();
}

void tile_props_cancel_cb(Fl_Button*, void*) {
	tile_props_dlg->hide();
}

void mask_color_click_cb(Fl_Button*, void*) {
	
	int c = mask_color_box->color() >> 8;
	int ri = (c >> 16) & 0xFF;
	int gi = (c >> 8) & 0xFF;
	int bi = (c) & 0xFF;
	
	double r = ri / 255.0, g = gi / 255.0, b = bi / 255.0;	
	
	if(fl_color_chooser("New Mask Color", r, g, b)) {
		int ri = r * 255.0, gi = g * 255.0, bi = b * 255.0;
		mask_color_box->color(fl_rgb_color(ri,gi,bi));
		mask_color_box->redraw();
	}		
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
	
	rlog("Starting editor.");
	
	make_window();
	
	tiles->setSelectCallback(tile_select_cb);
		
	canvas->setSelectCallback(map_select_cb);
	canvas->setTileCanvas(tiles);	
	canvas->setLayer(0);
	
	main_window->callback(main_window_callback);
	
	main_window->show(argc, argv);
	
	return Fl::run();
}
