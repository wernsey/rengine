#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_File_Chooser.H>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "LevelCanvas.h"
#include "TileCanvas.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// GLOBALS //////////////////////////////////////////////////////

Fl_Window *window;

LevelCanvas *canvas;

TileCanvas *tiles;

Fl_Input *tilesClass;	
Fl_Check_Button *tileIsBarrier;

Fl_Window *new_map_dlg;
Fl_Input *new_map_width, *new_map_height, *new_tile_width, *new_tile_height;

Fl_Button *new_btn_ok, *new_btn_cancel;

Fl_Select_Browser *tileSetSelect;

char *map_file = NULL;

char *tileset_file = NULL;

int g_mapwidth = 20, g_mapheight = 20;
int g_tilewidth = 16, g_tileheight = 16;

// PROTOTYPES //////////////////////////////////////////////////////

void tileset_cb(Fl_Widget*w, void*p);
void tile_select_cb(TileCanvas *canvas);
void tilesetsaveas_cb(Fl_Widget* w, void*);
void saveas_cb(Fl_Widget* w, void*);

// CALLBACKS //////////////////////////////////////////////////////

void new_btn_ok_cb(Fl_Widget* w, void*) {
	g_mapwidth = MAX(atoi(new_map_width->value()), 1);
	g_mapheight = MAX(atoi(new_map_height->value()), 1);
	g_tilewidth = MAX(atoi(new_tile_width->value()), 1);
	g_tileheight = MAX(atoi(new_tile_height->value()), 1);
		
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

void new_btn_cancel_cb(Fl_Widget* w, void*) {
	new_map_dlg->hide();
}

void new_cb(Fl_Widget* w, void*) {
	char buffer[30];
	
	sprintf(buffer, "%d", g_mapwidth);
	new_map_width->value(buffer);
	
	sprintf(buffer, "%d", g_mapheight);
	new_map_height->value(buffer);
	
	sprintf(buffer, "%d", g_tilewidth);
	new_tile_width->value(buffer);
	
	sprintf(buffer, "%d", g_tileheight);
	new_tile_height->value(buffer);
	
	new_map_dlg->show();
}

void open_cb(Fl_Widget* w, void*) {
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

void save_cb(Fl_Widget* w, void*p) {
	map *m = canvas->getMap();
	if(!m) 
		return;
	if(map_file)
		map_save(m, map_file);
	else
		saveas_cb(w, p);
}

void saveas_cb(Fl_Widget* w, void*) {
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

void changeDirectory(const char *dir) {
	// FIXME: Do we change directories with a map being edited?
	if(dir) {
		char buffer[128];
		snprintf(buffer, sizeof buffer, "Editor - %s", dir);
		window->label(buffer);
		chdir(dir);
	}
}

void chdir_cb(Fl_Widget* w, void*) {
	char cwd[128];	
	getcwd(cwd, sizeof cwd);	
	const char *dir = fl_dir_chooser("Choose Working Directory", cwd, 0);
	changeDirectory(dir);
}

void quit_cb(Fl_Widget* w, void*) {
	// FIXME: Changed? Save before exit (yes/no)	
	exit(0);
}

void tilesetadd_cb(Fl_Widget* w, void*) {
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

void tilesetsave_cb(Fl_Widget* w, void*p) {
	if(tileset_file)
		ts_save_all(tileset_file);
	else
		tilesetsaveas_cb(w, p);
}

void tilesetsaveas_cb(Fl_Widget* w, void*) {
	char * filename = fl_file_chooser("Choose Filename For Tileset", "Tileset files (*.tls)", "", 1);	
	if(filename != NULL) {
		if(ts_save_all(filename)) {			
			tileset_file = filename;
		} else {
			fl_alert("Unable to export tileset to %s", filename);
		}
	}
}

void tilesetload_cb(Fl_Widget* w, void*) {	
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

void layerSelect_cb(Fl_Widget*w, void*p) {
	Fl_Select_Browser *s = static_cast<Fl_Select_Browser *>(w);
	int i = s->value() - 1;
	canvas->setLayer(i);
}
	
void zoomOutCb(Fl_Widget*w, void*p) {
	BMCanvas *c = static_cast<BMCanvas *>(w->user_data());
	if(c->zoom() > 1.0)
		c->zoom(c->zoom() - 1.0);
}

void zoomInCb(Fl_Widget*w, void*p) {
	BMCanvas *c = static_cast<BMCanvas *>(w->user_data());
	if(c->zoom() < 10.0)
		c->zoom(c->zoom() + 1.0);
}

void tileset_cb(Fl_Widget*w, void*p) {
	Fl_Select_Browser *s = static_cast<Fl_Select_Browser *>(w);
	int i = s->value();
	struct tileset *ts = ts_find(s->text(i));
	
	if(!ts) return;
	
	tiles->setTileset(ts);
	
	tile_select_cb(tiles);
	
	tiles->parent()->redraw();
}

void tileClass_cb(Fl_Widget*w, void*p) {
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

void tileBarrier_cb(Fl_Widget*w, void*p) {
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

void tileDrawBarrier_cb(Fl_Widget*w, void*p) {
	Fl_Check_Button *b = static_cast<Fl_Check_Button *>(w);
	tiles->drawBarriers(b->value() == 1);	
	
	tiles->redraw();
}

int main(int argc, char *argv[]) {
	
	window = new Fl_Window(640, 480, "Editor");
	
	Fl::scheme("gtk+");

	Fl_Menu_Item menuitems[] = {
			{ "&File",              0, 0, 0, FL_SUBMENU },
				{ "&New File",        0, (Fl_Callback *)new_cb },
				{ "&Open File...",    FL_CTRL + 'o', (Fl_Callback *)open_cb },
				{ "&Save File",       FL_CTRL + 's', (Fl_Callback *)save_cb },
				{ "Save File &As...", FL_CTRL + FL_SHIFT + 's', (Fl_Callback *)saveas_cb, 0, FL_MENU_DIVIDER },
				{ "Set Working &Directory", FL_CTRL + FL_SHIFT + 'd', (Fl_Callback *)chdir_cb, 0, FL_MENU_DIVIDER },
				
				{ "E&xit", FL_CTRL + 'q', (Fl_Callback *)quit_cb, 0 },
				{ 0 },
			
			/*
			{ "&Edit", 0, 0, 0, FL_SUBMENU },
				{ "&Undo",       FL_CTRL + 'z', (Fl_Callback *)undo_cb, 0, FL_MENU_DIVIDER },
				{ "Cu&t",        FL_CTRL + 'x', (Fl_Callback *)cut_cb },
				{ "&Copy",       FL_CTRL + 'c', (Fl_Callback *)copy_cb },
				{ "&Paste",      FL_CTRL + 'v', (Fl_Callback *)paste_cb },
				{ "&Delete",     0, (Fl_Callback *)delete_cb },
				{ 0 },
			*/

			{ "&Map", 0, 0, 0, FL_SUBMENU },
				{ 0 },

			{ "&Tileset", 0, 0, 0, FL_SUBMENU },
				{ "&Add",     0, (Fl_Callback *)tilesetadd_cb, 0, FL_MENU_DIVIDER },
				{ "&Export",     0, (Fl_Callback *)tilesetsave_cb },
				{ "Export &As",     0, (Fl_Callback *)tilesetsaveas_cb },
				{ "&Import",     0, (Fl_Callback *)tilesetload_cb },
				{ 0 },
				
			{ 0 }
		};
		
	Fl_Menu_Bar *m = new Fl_Menu_Bar(0, 0, 640, 25);
	m->copy(menuitems);
	
	Fl_Scroll levelScroll(5,30,400,260);
	canvas = new LevelCanvas(5,30,400,260);	
	levelScroll.color(FL_BLUE);
	levelScroll.end();
	
	// See "Common Widgets and Attributes" in the FLTK docs
	Fl_Button levelZoomOut(5,levelScroll.y() + levelScroll.h() + 5,20,20, "@line");
	levelZoomOut.user_data(canvas);
	levelZoomOut.callback(zoomOutCb);
	levelZoomOut.tooltip("Zoom Out");	
	Fl_Button levelZoomIn(25,levelScroll.y() + levelScroll.h() + 5,20,20, "@+");
	levelZoomIn.user_data(canvas);
	levelZoomIn.callback(zoomInCb);
	levelZoomIn.tooltip("Zoom In");
		
	Fl_Check_Button levelShowWalls(levelZoomIn.x() + levelZoomIn.w() + 5, levelZoomIn.y(), 100, levelZoomIn.h(), "Show Barriers");
		
	Fl_Input levelClass(levelScroll.x(), levelZoomIn.y() + levelZoomIn.h() + 20, levelScroll.w()/3, 20, "Class:");	
	levelClass.align(FL_ALIGN_TOP |FL_ALIGN_LEFT);
	
	Fl_Input levelId(levelScroll.x() + levelScroll.w()/3 + 5, levelZoomIn.y() + levelZoomIn.h() + 20, levelScroll.w()/3, 20, "ID:");	
	levelId.align(FL_ALIGN_TOP |FL_ALIGN_LEFT);	
	
	Fl_Check_Button isBarrier(levelScroll.x() + 2*levelScroll.w()/3 + 5, levelZoomIn.y() + levelZoomIn.h() + 20, levelScroll.w()/3, 20, "Barrier");
		
	int h = isBarrier.y() + isBarrier.h() + 20;
	Fl_Select_Browser layerSelect(levelScroll.x(), h, levelClass.w() + levelId.w() + 5, window->h() - h - 5, "Layer");
	layerSelect.align(FL_ALIGN_TOP);
	
	layerSelect.add("back");
	layerSelect.add("centre");
	layerSelect.add("fore");

	layerSelect.callback(layerSelect_cb);
	
	Fl_Scroll tileScroll(410,30,640 - 410 - 5,260);
	tiles = new TileCanvas(300,30,300,300);
	tileScroll.color(FL_BLUE);
	tileScroll.end();
	
	tiles->setSelectCallback(tile_select_cb);
	
	canvas->setTileCanvas(tiles);
	
	Fl_Button tileZoomOut(410,tileScroll.y() + tileScroll.h() + 5,20,20, "@line");
	tileZoomOut.user_data(tiles);
	tileZoomOut.callback(zoomOutCb);
	tileZoomOut.tooltip("Zoom Out");	
	Fl_Button tileZoomIn(430,tileScroll.y() + tileScroll.h() + 5,20,20, "@+");
	tileZoomIn.user_data(tiles);
	tileZoomIn.callback(zoomInCb);
	tileZoomIn.tooltip("Zoom In");
	
	Fl_Check_Button tilesShowWalls(tileZoomIn.x() + tileZoomIn.w() + 5, tileZoomIn.y(), 100, tileZoomIn.h(), "Show Barriers");
	tilesShowWalls.callback(tileDrawBarrier_cb);
	
	tilesClass = new Fl_Input(tileScroll.x(), tileZoomIn.y() + tileZoomIn.h() + 20, tileScroll.w()/2, 20, "Class:");	
	tilesClass->align(FL_ALIGN_TOP |FL_ALIGN_LEFT);
	tilesClass->callback(tileClass_cb);
		
	tileIsBarrier = new Fl_Check_Button(tilesClass->x() + tilesClass->w() + 5, tilesClass->y(), tileScroll.w()/2, 20, "Barrier");
	tileIsBarrier->callback(tileBarrier_cb);
	
	tileSetSelect = new Fl_Select_Browser(tileScroll.x(), layerSelect.y(), tileScroll.w(), layerSelect.h(), "Tile Set");
	tileSetSelect->align(FL_ALIGN_TOP);
	
	tileSetSelect->callback(tileset_cb);
	
	window->resizable(window);
	
	window->end();
	window->show(argc, argv);	
	
	/******* New Level Dialog ********/
	new_map_dlg = new Fl_Window(300, 160, "New Map");
	new_map_dlg->set_modal();

	new_map_width = new Fl_Input(80, 10, 200, 25, "Map Width");
	new_map_height = new Fl_Input(80, 40, 200, 25, "Map Height");

	new_tile_width = new Fl_Input(80, 70, 200, 25, "Tile Width");
	new_tile_height = new Fl_Input(80, 100, 200, 25, "Tile Height");
	
	new_btn_ok = new Fl_Button(80, 130, 90, 25, "OK");
	new_btn_ok->callback(new_btn_ok_cb);
	new_btn_cancel = new Fl_Button(200, 130, 90, 25, "Cancel");
	new_btn_cancel->callback(new_btn_cancel_cb);
	
	new_map_dlg->end();
	
	return Fl::run();
}
