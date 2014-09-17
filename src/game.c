#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <unistd.h>  /* may not be portable, works with MinGW on Windows */
#include <errno.h>

#ifdef WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#endif

#include "bmp.h"
#include "ini.h"
#include "game.h"
#include "utils.h"
#include "states.h"
#include "resources.h"
#include "log.h"
#include "gamedb.h"
#include "sound.h"

/* Some Defaults *************************************************/

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define VIRT_WIDTH 320
#define VIRT_HEIGHT 240

#define DEFAULT_FPS 33

#define DEFAULT_APP_TITLE "Rengine"

#define GAME_INI		"game.ini"

#define PARAM(x) (#x)

/* Globals *************************************************/

/* Set quit to 1 from anywhere to break out of the main loop */
int quit = 0;

/* The width/height of the window (in windowed mode) */
static int screenWidth = SCREEN_WIDTH,
	screenHeight = SCREEN_HEIGHT;

/* The desired frame rate */
int fps = DEFAULT_FPS;

/* The dimensions of the virtual screen */
int virt_width = VIRT_WIDTH,
    virt_height = VIRT_HEIGHT;

/* SDL globals */
static SDL_Window *win = NULL;
static SDL_Renderer *ren = NULL;
static SDL_Texture *tex = NULL;

/* The start time of the current animation frame */
static Uint32 frameStart;

unsigned int frame_counter = 0;

/* The filter (nearest or linear) to use when rendering the virtual screen */
static const char *filter = "0";

/* The bitmap on which all drawing occurs.
It is rendered to `tex` once per frame */
static struct bitmap *bmp = NULL;

/* The game.ini configuration file */
struct ini_file *game_ini = NULL;

struct game_state *get_demo_state(const char *name); /* demo.c */

/* The mouse state */
int show_cursor = 1;
int mouse_x = 0, mouse_y = 0;
int mouse_btns = 0, mouse_clck = 0;

/* The state of the keyboard */
char keys[SDL_NUM_SCANCODES];       /* keys that are currently down */

/* The starting directory */
char initial_dir[256];

/* Functions *************************************************/

int init(const char *appTitle, int flags) {
	flags |= SDL_WINDOW_SHOWN;

	rlog("Creating Window.");

	win = SDL_CreateWindow(appTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, flags);
	if(!win) {
		rerror("SDL_CreateWindow: %s", SDL_GetError());
		return 0;
	}

	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(!ren) {
		rerror("SDL_CreateRenderer: %s", SDL_GetError());
		return 0;
	}

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, filter);

	rlog("Window Created.");

    if(SDL_ShowCursor(show_cursor) < 0) {
        rerror("SDL_ShowCursor: %s", SDL_GetError());
    }

	bmp = bm_create(virt_width, virt_height);

	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, bmp->w, bmp->h);
	if(!tex) {
		rerror("SDL_CreateTexture: %s", SDL_GetError());
		return 0;
	}
	rlog("Texture Created.");

	reset_keys();

	return 1;
}

int handleSpecialKeys(SDL_Scancode key) {
	if(key == SDL_SCANCODE_ESCAPE && (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT])) {
		quit = 1;
		return 1;
	} else if (key == SDL_SCANCODE_F11) {
		if(!(SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
			if(SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP) < 0)
				rerror("Unable to set window to fullscreen: %s", SDL_GetError());
		} else if(SDL_SetWindowFullscreen(win, 0) < 0) {
			rerror("Unable to set window to windowed: %s", SDL_GetError());
		}
		return 1;
	} else if(key == SDL_SCANCODE_F12) {
        char filename[256];
        snprintf(filename, sizeof filename, "%s/save.png", initial_dir);
		bm_save(bmp, filename);
		rlog("Screenshot saved as %s", filename);
		return 1;
	}
	return 0;
}

struct bitmap *get_screen() {
    return bmp;
}

int screen_to_virt_x(int in) {
	return in * bmp->w / screenWidth;
}

int screen_to_virt_y(int in) {
	return in * bmp->h / screenHeight;
}

void render() {
	/* FIXME: Docs says SDL_UpdateTexture() be slow */
	SDL_UpdateTexture(tex, NULL, bmp->data, bmp->w*4);
	SDL_RenderClear(ren);
	SDL_RenderCopy(ren, tex, NULL, NULL);
	SDL_RenderPresent(ren);
}

static SDL_Scancode last_key = SDL_SCANCODE_UNKNOWN;

int kb_hit() {
	return last_key != SDL_SCANCODE_UNKNOWN;
}

int readkey() {
	return last_key;
}

/* advanceFrame() is kept separate so that it
 * can be exposed to the scripting system later
 */
void advanceFrame() {
	SDL_Event event;
	Uint32 end;
	int new_btns, cursor;

    frame_counter++;

	render();

	end = SDL_GetTicks();
	assert(end > frameStart);

	if(end - frameStart < 1000/fps)
		SDL_Delay(1000/fps - (end - frameStart));

	frameStart = SDL_GetTicks();

    cursor = SDL_ShowCursor(-1);
    if(cursor < 0) {
        rerror("SDL_ShowCursor: %s", SDL_GetError());
        cursor = 0;
    }

    if(cursor > 0) {
        int mx, my;
        new_btns = SDL_GetMouseState(&mx, &my);

        /* clicked = buttons that were down in the previous frame and aren't down anymore */
        mouse_clck = mouse_btns & ~new_btns;
        mouse_btns = new_btns;

        mouse_x = screen_to_virt_x(mx);
        mouse_y = screen_to_virt_y(my);
    } else {
        /* Ignore the mouse if the cursor is gone */
        new_btns = 0;
        mouse_clck = 0;
        mouse_btns = 0;
        mouse_x = -1;
        mouse_y = -1;
    }
	
	last_key = SDL_SCANCODE_UNKNOWN;

	while(SDL_PollEvent(&event)) {
		if(event.type == SDL_QUIT) {
			quit = 1;
		} else if(event.type == SDL_KEYDOWN) {
			/* FIXME: Descision whether to stick with scancodes or keycodes? */
			int index = event.key.keysym.scancode;

			/* Special Keys: F11, F12 and Esc */
			if(!handleSpecialKeys(event.key.keysym.scancode)) {
				/* Not a special key: */
				assert(index < SDL_NUM_SCANCODES);
				keys[index] = 1;
                last_key = index;
			}

		} else if(event.type == SDL_KEYUP) {
			int index = event.key.keysym.scancode;
			assert(index < SDL_NUM_SCANCODES);
			keys[index] = 0;
		/*} else if(event.type == SDL_MOUSEBUTTONDOWN) {
		} else if(event.type == SDL_MOUSEMOTION) {
            mouse_x = screen_to_virt_x(event.motion.x);
            mouse_y = screen_to_virt_y(event.motion.y);*/
		} else if(event.type == SDL_WINDOWEVENT) {
			switch(event.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
				screenWidth = event.window.data1;
				screenHeight = event.window.data2;
				rlog("Window resized to %dx%d", screenWidth, screenHeight);
				break;
			default: break;
			}
		}
	}
}

void reset_keys() {
	int i;
	for(i = 0; i < SDL_NUM_SCANCODES; i++) {
		keys[i] = 0;
	}
}

void usage(const char *name) {
	/* Unfortunately this doesn't work in Windows.
	MinGW does give you a stderr.txt though.
	*/
	fprintf(stderr, "Usage: %s [options]\n", name);
	fprintf(stderr, "where options:\n");
	fprintf(stderr, " -p pakfile  : Load game from PAK file.\n");
	fprintf(stderr, " -g dir      : Use a directory containing a game.ini\n");
	fprintf(stderr, "               file instead of a pak file.\n");
	fprintf(stderr, " -l logfile  : Use specific log file.\n");
}

int main(int argc, char *argv[]) {
	int opt;

	int fullscreen = 0, resizable = 0, borderless = 0;

	const char *appTitle = DEFAULT_APP_TITLE;

	const char *game_dir = NULL;
	const char *pak_filename = "game.pak";

	const char *rlog_filename = "rengine.log";

	const char *startstate;

	struct game_state *gs = NULL;

	int demo = 0;

	SDL_version compiled, linked;

	log_init(rlog_filename);

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		rerror("SDL_Init: %s", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);

	while((opt = getopt(argc, argv, "p:g:l:d?")) != -1) {
		switch(opt) {
			case 'p': {
				pak_filename = optarg;
			} break;
			case 'g' : {
				game_dir = optarg;
				pak_filename = NULL;
			} break;
			case 'l': {
				rlog_filename = optarg;
			} break;
			case 'd': {
				demo = 1;
			} break;
			case '?' : {
				usage(argv[0]);
				return 1;
			}
		}
	}

	if(!getcwd(initial_dir, sizeof initial_dir)) {
		rerror("error in getcwd(): %s", strerror(errno));
		return 1;
	}
	rlog("Running engine from %s", initial_dir);

	if(!gdb_new()) {
		rerror("Unable to create Game Database");
		return 1;
	}
	re_initialize();
	states_initialize();
	if(!snd_init()) {
		rerror("Terminating because of audio problem.");
		return 1;
	}

	/* Don't quite know how to use this in Windows yet.
	SDL_LogSetAllPriority(SDL_rlog_PRIORITY_WARN);
	SDL_Log("Testing Log capability.");
	*/
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	rlog("SDL version %d.%d.%d (compile)", compiled.major, compiled.minor, compiled.patch);
	rlog("SDL version %d.%d.%d (link)", linked.major, linked.minor, linked.patch);

	if(!demo) {
		if(pak_filename) {
			rlog("Loading game PAK file: %s", pak_filename);
			if(!rs_read_pak(pak_filename)) {
				rerror("Unable to open PAK file '%s'; Playing demo mode.", pak_filename);
				goto start_demo;
			}
		} else {
			rlog("Not using a PAK file. Using '%s' instead.", game_dir);
			if(chdir(game_dir)) {
				rerror("Unable to change to '%s': %s", game_dir, strerror(errno));
				return 1;
			}
		}

		game_ini = re_get_ini(GAME_INI);
		if(game_ini) {
			appTitle = ini_get(game_ini, "init", "appTitle", "Rengine");

			screenWidth = atoi(ini_get(game_ini, "screen", "width", PARAM(SCREEN_WIDTH)));
			screenHeight = atoi(ini_get(game_ini, "screen", "height", PARAM(SCREEN_HEIGHT)));
			resizable = atoi(ini_get(game_ini, "screen", "resizable", "0")) ? SDL_WINDOW_RESIZABLE : 0;
			borderless = atoi(ini_get(game_ini, "screen", "borderless", "0")) ? SDL_WINDOW_BORDERLESS : 0;
			fullscreen = atoi(ini_get(game_ini, "screen", "fullscreen", "0")) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
			fps = atoi(ini_get(game_ini, "screen", "fps", PARAM(DEFAULT_FPS)));
			if(fps <= 0)
				fps = DEFAULT_FPS;

			filter = !my_stricmp(ini_get(game_ini, "screen", "filter", "nearest"), "linear")? "1": "0";

			virt_width = atoi(ini_get(game_ini, "virtual", "width", PARAM(VIRT_WIDTH)));
			virt_height = atoi(ini_get(game_ini, "virtual", "height", PARAM(VIRT_HEIGHT)));

            show_cursor = atoi(ini_get(game_ini, "mouse", "show-cursor", PARAM(1)))? 1 : 0;

			startstate = ini_get(game_ini, "init", "startstate", NULL);
			if(startstate) {
                gs = get_state(startstate);
				if(!gs) {
					rerror("Unable to get initial state: %s", startstate);
					return 1;
				}
			} else {
				rerror("No initial state in %s", GAME_INI);
				return 1;
			}
		} else {
			rerror("Unable to load %s", GAME_INI);
			return 1;
		}
	} else {
start_demo:
		rlog("Starting demo mode");
		gs = get_demo_state("demo");
	}

	rlog("Initialising...");

	if(!init(appTitle, fullscreen | borderless | resizable)) {
		return 1;
	}

    assert(gs);
	rlog("Entering initial state...");
    if(!change_state(gs)) {
        rlog("Quiting, because of earlier problems with the initial state");
        return 1;
    }

    frameStart = SDL_GetTicks();

	rlog("Event loop starting...");

	while(!quit) {
		gs = current_state();
		if(!gs) {
			break;
		}

		if(gs->update)
			gs->update(gs, bmp);

		advanceFrame();
	}

	rlog("Event loop stopped.");

	gs = current_state();
	if(gs && gs->deinit)
		gs->deinit(gs);

	bm_free(bmp);

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);

	SDL_Quit();

	ini_free(game_ini);

	re_clean_up();

	gdb_save("dump.db"); /* For testing the game database functionality. Remove later. */

	gdb_close();
	snd_deinit();
	rlog("Engine shut down.");

	return 0;
}

const char *about_text =
"Rengine: Copyright (c) 2013 Werner Stoop\n"
"Rengine is based in part on the work of these projects:\n"
"SDL and SDL_mixer (c) 1997-2013 Sam Lantinga\n"
"Lua (c) 1994 Lua.org, PUC-Rio\n"
"libogg and libvorbis (c) 2002-2008 Xiph.org Foundation\n"
"libpng (c) Contributing Authors and Group 42, Inc.\n"
"zlib (c) 1995-2013 Jean-loup Gailly and Mark Adler\n"
"libjpeg (c) 1991-2014, Thomas G. Lane, Guido Vollbeding\n"
"FLTK project (http://www.fltk.org)"
;

