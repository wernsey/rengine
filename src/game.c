#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <unistd.h>  /* may not be portable */

#ifdef WIN32
#include <SDL2/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "bmp.h"
#include "ini.h"
#include "game.h"
#include "particles.h"
#include "utils.h"
#include "states.h"
#include "resources.h"
#include "log.h"

/* Some Defaults *************************************************/

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_BPP 24

#define VIRT_WIDTH 320
#define VIRT_HEIGHT 240

#define DEFAULT_FPS 33

#define DEFAULT_APP_TITLE "Rengine"

#define PARAM(x) (#x)

/* Globals *************************************************/

static int screenWidth = SCREEN_WIDTH, 
	screenHeight = SCREEN_HEIGHT, 
	screenBpp = SCREEN_BPP;
int fps = DEFAULT_FPS;

SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;
SDL_Texture *tex = NULL;

int quit = 0;

static int fullscreen = 0,
	resizable = 0, borderless = 0;
	
/* FIXME: Get the filter mode working again.
filter = GL_NEAREST;
*/

static struct bitmap *bmp = NULL;

struct ini_file *game_ini = NULL;

static Uint32 frameStart;

struct game_state *get_demo_state(const char *name); /* demo.c */

int mouse_x = 0, mouse_y = 0;
int mouse_btns = 0, mouse_clck = 0;

char keys[SDL_NUM_SCANCODES];

/* Functions *************************************************/

int init(const char *appTitle, int virt_width, int virt_height) {
	int flags = SDL_WINDOW_SHOWN;
	
	rlog("Creating Window.");
	
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		rerror("SDL_Init: %s", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);
	
	if(!fullscreen) {
		if(resizable)
			flags |= SDL_WINDOW_RESIZABLE;
		else if(borderless)
			flags |= SDL_WINDOW_BORDERLESS;
	}
	
	win = SDL_CreateWindow(appTitle, 100, 100, screenWidth, screenHeight, flags);	
	if(!win) {
		rerror("SDL_CreateWindow: %s", SDL_GetError());
		return 0;
	}
	
	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(!ren) {
		rerror("SDL_CreateRenderer: %s", SDL_GetError());
		return 0;
	}	
	
	rlog("Window Created.");
	
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
	if(key == SDL_SCANCODE_ESCAPE) {
		quit = 1;
		return 1;
	} else if (key == SDL_SCANCODE_F11) {
		if(!fullscreen) {		
			if(SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
				rerror("Unable to set window to fullscreen: %s", SDL_GetError());
			} else {
				fullscreen = !fullscreen;
			}
		} else {	
			if(SDL_SetWindowFullscreen(win, 0) < 0) {
				rerror("Unable to set window to windowed: %s", SDL_GetError());
			} else {
				fullscreen = !fullscreen;
			}
		}
		return 1;
	} else if(key == SDL_SCANCODE_F12) {
		const char *filename = "save.bmp";
		bm_save(bmp, filename);
		rlog("Screenshot saved as %s", filename);
		return 1;
	}
	return 0;
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

/* advanceFrame() is kept separate so that it 
 * can be exposed to the scripting system later
 */
void advanceFrame() {				
	SDL_Event event;	
	Uint32 end;
	int new_btns;
	
	update_particles(bmp, 0, 0);
	
	render();
		
	end = SDL_GetTicks();
	assert(end > frameStart);
	
	if(end - frameStart < 1000/fps)
		SDL_Delay(1000/fps - (end - frameStart));
	
	frameStart = SDL_GetTicks();
	
	new_btns = SDL_GetMouseState(&mouse_x, &mouse_y);
	
	/* clicked = buttons that were down last frame and aren't down anymore */
	mouse_clck = mouse_btns & ~new_btns; 
	mouse_btns = new_btns;
	
	mouse_x = screen_to_virt_x(mouse_x);
	mouse_y = screen_to_virt_y(mouse_y);	
	
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
			}
			
		} else if(event.type == SDL_KEYUP) {
			int index = event.key.keysym.scancode;			
			assert(index < SDL_NUM_SCANCODES);			
			keys[index] = 0;
						
		}  else if(event.type == SDL_MOUSEBUTTONDOWN) {
			// REMOVE ME: Dummy code to draw random stuff on the screen /
			if(event.button.button == SDL_BUTTON_LEFT) {
				int i;
				for(i = 0; i < 20; i++) {
					add_particle(mouse_x, mouse_y, (float)(rand()%8 - 3)/2, (float)(rand()%8 - 3)/2, rand()%33 + 10, bm_lerp(0xFFFFFF, 0x00FF00, (double)rand()/RAND_MAX));					
				}
			} else if(event.button.button == SDL_BUTTON_RIGHT) {
			}			
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

int kb_hit() {
	int i;
	for(i = 1; i < SDL_NUM_SCANCODES; i++) {
		if(keys[i])
			return i;
	} 
	return 0;
}

void usage(const char *name) {
	/* Unfortunately this doesn't work in Windows. 
	MinGW does give you a stderr.txt though.
	*/
	fprintf(stderr, "Usage: %s [options]\n", name);
	fprintf(stderr, "where options:\n");
	fprintf(stderr, " -p pakfile  : Load game from PAK file.\n");
	fprintf(stderr, " -g inifile  : Use a game INI file instead of a pak file.\n");
	fprintf(stderr, " -l rlogfile  : Use specific rlog file.\n");
}

int main(int argc, char *argv[]) {
	int opt;
	int virt_width = VIRT_WIDTH,
		virt_height = VIRT_HEIGHT;
	
	const char *appTitle = DEFAULT_APP_TITLE;
	
	const char *pak_filename = "game.pak"; 
	
	const char *game_filename = "game.ini";
	
	const char *rlog_filename = "rlogfile.txt";
	
	const char *startstate;
	
	int demo = 0;
	
	SDL_version compiled, linked;
	
	while((opt = getopt(argc, argv, "p:g:l:d?")) != -1) {
		switch(opt) {
			case 'p': {
				pak_filename = optarg;
			} break;
			case 'g' : {
				game_filename = optarg;
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
	
	log_init(rlog_filename);
	
	/* Don't quite know how to use this in Windows yet.
	SDL_rlogSetAllPriority(SDL_rlog_PRIORITY_WARN);
	SDL_rlog("Testing rlog capability.");
	*/
	
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	rlog("SDL version %d.%d.%d (compile)", compiled.major, compiled.minor, compiled.patch);
	rlog("SDL version %d.%d.%d (link)", linked.major, linked.minor, linked.patch);
	
	re_initialize();
	
	if(!demo) {
		if(pak_filename) {
			rlog("Loading game PAK file: %s", pak_filename);	
			if(!rs_read_pak(pak_filename)) {
				rerror("Unable to open PAK file '%s'; Playing demo mode.", pak_filename);
				goto start_demo;
			}
		} else {
			rlog("Not using a pak file. Using %s instead.", game_filename);
		}		
		
		game_ini = re_get_ini(game_filename);
		if(game_ini) {	
			appTitle = ini_get(game_ini, "init", "appTitle", "Rengine");
			
			screenWidth = atoi(ini_get(game_ini, "screen", "width", PARAM(SCREEN_WIDTH)));
			screenHeight = atoi(ini_get(game_ini, "screen", "height", PARAM(SCREEN_HEIGHT)));	
			screenBpp = atoi(ini_get(game_ini, "screen", "bpp", PARAM(SCREEN_BPP)));	
			resizable = atoi(ini_get(game_ini, "screen", "resizable", "0"));	
			borderless = atoi(ini_get(game_ini, "screen", "borderless", "0"));	
			fullscreen = atoi(ini_get(game_ini, "screen", "fullscreen", "0"));	
			fps = atoi(ini_get(game_ini, "screen", "fps", PARAM(DEFAULT_FPS)));
			if(fps <= 0)
				fps = DEFAULT_FPS;
			
			/*filter = !my_stricmp(ini_get(game_ini, "screen", "filter", "nearest"), "linear")? GL_LINEAR: GL_NEAREST;*/
				
			virt_width = atoi(ini_get(game_ini, "virtual", "width", PARAM(VIRT_WIDTH)));
			virt_height = atoi(ini_get(game_ini, "virtual", "height", PARAM(VIRT_HEIGHT)));
			
			startstate = ini_get(game_ini, "init", "startstate", NULL);
			if(startstate) {
				if(!set_state(startstate)) {
					rerror("Unable to set initial state: %s", startstate);
					quit = 1;
				}
			} else {
				rerror("No initial state in %s", game_filename);
				quit = 1;
			}
		} else {
			rerror("No game INI");
			quit = 1;
		}
	} else {
start_demo:
		rlog("Starting demo mode");
		current_state = get_demo_state("demo");
		if(!current_state->init(current_state))
			quit = 1;
	}
	
	rlog("Initialising...");
		
	if(!init(appTitle, virt_width, virt_height)) {
		return 1;
	}
	
	SDL_Log("Test rlog message");
	
	frameStart = SDL_GetTicks();	
	
	rlog("Event loop starting...");

	/* If I used SDL_WINDOW_FULLSCREEN in SDL_CreateWindow() it 
	had some strange problems when you shutdown the engine. */
	if(fullscreen) {
		if(SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
			rerror("Unable to set window to fullscreen: %s", SDL_GetError());
		} 
	}
	
	while(!quit) {
		if(current_state->update) 
			current_state->update(current_state, bmp);	
		
		if(!current_state) {
			break;
		}
		
		advanceFrame();
	}
	
	rlog("Event loop stopped.");
	
	if(current_state && current_state->deinit)
		current_state->deinit(current_state);	

	/*
	if(fullscreen && SDL_SetWindowFullscreen(win, 0) < 0) {
		fprintf(rlog_file, "rerror: Unable to reset window to windowed: %s\n", SDL_GetError());
		fflush(rlog_file);
	}*/
	
	bm_free(bmp);
	
	clear_particles();
	
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	
	SDL_Quit();
	
	ini_free(game_ini);
	
	re_clean_up();
	
	rlog("Engine shut down.");
	
	log_deinit();
	
	return 0;
}
