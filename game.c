#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <unistd.h>  /* may not be portable */

#ifdef WIN32
#include <SDL.h>
#include <SDL_opengl.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#endif

#include "bmp.h"
#include "ini.h"
#include "game.h"
#include "particles.h"
#include "utils.h"
#include "states.h"
#include "resources.h"

/* Some Defaults *************************************************/

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_BPP 24

#define VIRT_WIDTH 320
#define VIRT_HEIGHT 240

#define DEFAULT_FPS 33

#define DEFAULT_APP_TITLE "Rengine '80"

#define PARAM(x) (#x)

/* Globals *************************************************/

static GLuint texID;

static int screenWidth = SCREEN_WIDTH, 
	screenHeight = SCREEN_HEIGHT, 
	screenBpp = SCREEN_BPP, 
	fps = DEFAULT_FPS;

int quit = 0;

static int fullscreen = 0, resizable = 0, filter = GL_NEAREST;

static struct bitmap *bmp = NULL;

struct ini_file *game_ini = NULL;

static Uint32 frameStart;

FILE *log_file;

extern struct game_state demo_state; /* demo.c */

int mouse_x = 0, mouse_y = 0;
int mouse_btns = 0, mouse_clck = 0;

char keys[SDLK_LAST];

/* Functions *************************************************/

int textureFromBmp(struct bitmap *b) {
		
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	
	/* Please Explain: Weren't texture sizes supposed
	to have dimensions that are powers of 2?
	eg. 16x16, 64x64 and so on?
	See lesson 8 of lazyfoo
	*/
		
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, b->w, b->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, b->data);
	
	/* Apparently, these are important: (GL_NEAREST/GL_LINEAR) */
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ); 
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );
		
	glBindTexture(GL_TEXTURE_2D, 0);
	GLenum error = glGetError();
	if(error != GL_NO_ERROR) {
		fprintf(log_file, "error: unable to create texture from struct bitmap\n");
		return 0;
	}
	
	return 1;
}

int initGL() {
	GLenum error;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( 0.0, screenWidth, screenHeight, 0.0, 1.0, -1.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glClearColor(0.f, 0.f, 0.2f, 1.f);
	
	glEnable(GL_TEXTURE_2D);
	
	error = glGetError();
	if(error != GL_NO_ERROR) { 
		fprintf(log_file, "error: OpenGL - \n"); /* FIXME: Better error handling? */
		return 0;
	}
	
	return 1;
}

int init(const char *appTitle) {
	int flags = resizable?SDL_OPENGL | SDL_RESIZABLE:SDL_OPENGL;
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		return 0;
	}
	
	if(SDL_SetVideoMode(screenWidth, screenHeight, screenBpp, flags) == NULL) {
		return 0;
	}
	
	if(!initGL()) {
		return 0;
	}
	
	SDL_WM_SetCaption(appTitle, NULL);
	
	reset_keys();
	
	return 1;
}

void handleKeys(SDLKey key) {
	if(key == SDLK_ESCAPE) {
		quit = 1;
	} else if (key == SDLK_F1) {
		
		if(!fullscreen) {
			/* TODO: What resolution fullscreen? These hardcoded values are for my laptop specifically */			
			if(SDL_SetVideoMode(1600, 900, screenBpp, SDL_OPENGL | SDL_FULLSCREEN) == NULL) {
				fprintf(log_file, "error: unable to switch to fullscreen mode");
				return;
			}
			glViewport(0, 0, 1600, 900);
			fullscreen = !fullscreen;
		} else {
			int flags = resizable?SDL_OPENGL | SDL_RESIZABLE:SDL_OPENGL;
			if(SDL_SetVideoMode(screenWidth, screenHeight, screenBpp, flags) == NULL) {
				fprintf(log_file, "error: unable to switch to windowed mode");
				return;
			}
			glViewport(0, 0, screenWidth, screenHeight);
			fullscreen = !fullscreen;
		}
	} else if(key == SDLK_F12) {
		const char *filename = "save.bmp";
		bm_save(bmp, filename);
		fprintf(log_file, "Screenshot saved as %s\n", filename);
	}
}

int screen_to_virt_x(int in) {
	return in * bmp->w / screenWidth;
}

int screen_to_virt_y(int in) {
	return in * bmp->h / screenHeight;
}

void render() {
		
	/* Unlock the texture so it can be used for rendering */
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0,  bmp->w, bmp->h, GL_RGBA, GL_UNSIGNED_BYTE, bmp->data );
	glBindTexture(GL_TEXTURE_2D, 0);
	
	/* Clear the screen; not really neccessary for what I'm doing */
	glClear(GL_COLOR_BUFFER_BIT);
	
	/* Set up the ortho view */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();	
	glOrtho( 0.0, screenWidth, screenHeight, 0.0, 1.0, -1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	/* Draw a single quad with our texture that fills the entire screen */
	glBindTexture(GL_TEXTURE_2D, texID);
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex2f( 0, 0);	
		glTexCoord2f(1,0); glVertex2f( screenWidth, 0);
		glTexCoord2f(1,1); glVertex2f( screenWidth, screenHeight);
		glTexCoord2f(0,1); glVertex2f( 0, screenHeight);
	
		/* To put something in the foreground:
		glTexCoord2f(0,0); glVertex3f( 0.25, 0.25, 0.25);	
		glTexCoord2f(1,0); glVertex3f( 0.75, 0.25, 0.25);
		glTexCoord2f(1,1); glVertex3f( 0.75, 0.75, 0.25);
		glTexCoord2f(0,1); glVertex3f( 0.25, 0.75, 0.25);
		*/
	glEnd();
	
	SDL_GL_SwapBuffers();
	
	/* Lock the texture so that the next frame can get rendered */
	glBindTexture(GL_TEXTURE_2D, texID);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp->data);
	glBindTexture(GL_TEXTURE_2D, 0);
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
			
			/* Handle special keys */			
			handleKeys(event.key.keysym.sym);
			
			assert(event.key.keysym.sym < SDLK_LAST);
			keys[event.key.keysym.sym] = 1;
			
		} else if(event.type == SDL_KEYUP) {
			
			assert(event.key.keysym.sym < SDLK_LAST);			
			keys[event.key.keysym.sym] = 0;
						
		} /* else if(event.type == SDL_MOUSEBUTTONDOWN) {
			// REMOVE ME: Dummy code to draw random stuff on the screen /
			if(event.button.button == SDL_BUTTON_LEFT) {
				int i;
				for(i = 0; i < 20; i++) {
					add_particle(mouse_x, mouse_y, (float)(rand()%8 - 3)/2, (float)(rand()%8 - 3)/2, rand()%33 + 10, bm_gradient(0xFFFFFF, 0x00FF00, (double)rand()/RAND_MAX));					
				}
			} else if(event.button.button == SDL_BUTTON_RIGHT) {
			}			
		}  */
		else if(event.type == SDL_VIDEORESIZE) {
			int flags = resizable?SDL_OPENGL | SDL_RESIZABLE:SDL_OPENGL;
			screenWidth = event.resize.w;
			screenHeight = event.resize.h;
			if(SDL_SetVideoMode(screenWidth, screenHeight, screenBpp, flags) == NULL) {
				fprintf(log_file, "error: unable to handle video resize event");
				quit = 1;
			}
			glViewport(0, 0, screenWidth, screenHeight);
		}
	}
}

void reset_keys() {	
	int i;
	for(i = 0; i < SDLK_LAST; i++) {
		keys[i] = 0;
	}
}

int kb_hit() {
	int i;
	for(i = 1; i < SDLK_LAST; i++) {
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
	fprintf(stderr, " -l logfile  : Use specific log file.\n");
}

int main(int argc, char *argv[]) {
	int opt;
	int virt_width = VIRT_WIDTH,
		virt_height = VIRT_HEIGHT;
	
	const char *appTitle = DEFAULT_APP_TITLE;
	
	const char *pak_filename = "game.pak"; 
	
	const char *game_filename = "game.ini";
	
	const char *log_filename = "logfile.txt";
	
	const char *startstate;
	
	int demo = 0;
	
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
				log_filename = optarg;
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
	
	log_file = fopen(log_filename, "w");
	if(!log_file) {
		log_file = stdout;
	}
	
	re_initialize();
	
	if(!demo) {
		if(pak_filename) {
			fprintf(log_file, "info: Loading game PAK file: %s\n", pak_filename);	
			if(!rs_read_pak(pak_filename)) {
				fprintf(log_file, "error: Unable to open PAK file '%s'; Playing demo mode\n", pak_filename);
				demo = 1;
			}
		} else {
			fprintf(log_file, "info: Not using a pak file. Using %s instead.\n", game_filename);
		}		
	}
	
	if(!demo) {
		game_ini = re_get_ini(game_filename);
		if(game_ini) {	
			appTitle = ini_get(game_ini, "init", "appTitle", "Rengine '80");
			
			screenWidth = atoi(ini_get(game_ini, "screen", "width", PARAM(SCREEN_WIDTH)));
			screenHeight = atoi(ini_get(game_ini, "screen", "height", PARAM(SCREEN_HEIGHT)));	
			screenBpp = atoi(ini_get(game_ini, "screen", "bpp", PARAM(SCREEN_BPP)));	
			resizable = atoi(ini_get(game_ini, "screen", "resizable", "0"));	
			fps = atoi(ini_get(game_ini, "screen", "fps", PARAM(DEFAULT_FPS)));
			if(fps <= 0)
				fps = DEFAULT_FPS;
			
			filter = !my_stricmp(ini_get(game_ini, "screen", "filter", "nearest"), "linear")? GL_LINEAR: GL_NEAREST;
				
			virt_width = atoi(ini_get(game_ini, "virtual", "width", PARAM(VIRT_WIDTH)));
			virt_height = atoi(ini_get(game_ini, "virtual", "height", PARAM(VIRT_HEIGHT)));
			
			startstate = ini_get(game_ini, "init", "startstate", NULL);
			if(startstate) {
				if(!set_state(startstate)) {
					fprintf(log_file, "error: Unable to set initial state: %s\n", startstate);
					quit = 1;
				}
			} else {
				fprintf(log_file, "error: No initial state in %s\n", game_filename);
				quit = 1;
			}
		} else {
			fprintf(log_file, "warn: No game INI. Using defaults; Playing demo mode\n");
			quit = 1;
		}
	} else {
		fprintf(log_file, "info: Starting demo mode\n");
		current_state = &demo_state;
		if(!current_state->init(current_state))
			quit = 1;
	}
	
	fprintf(log_file, "info: Initialising...\n");
		
	if(!init(appTitle)) {
		return 1;
	}
	
	bmp = bm_create(virt_width, virt_height);	
	if(!textureFromBmp(bmp)) {
		return 1;
	}
	
	/* Lock the texture so that the first frame can be drawn on it */
	glBindTexture(GL_TEXTURE_2D, texID);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp->data);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	frameStart = SDL_GetTicks();	
	
	fprintf(log_file, "info: Event loop starting...\n");
	
	while(!quit) {
		if(current_state->update) 
			current_state->update(current_state, bmp);	
		
		if(!current_state) {
			break;
		}
		
		advanceFrame();
	}
	
	fprintf(log_file, "info: Event loop stopped.\n");
	
	if(current_state && current_state->deinit)
		current_state->deinit(current_state);	
	
	bm_free(bmp);	
	
	glDeleteTextures(1, &texID);
	
	clear_particles();
	
	SDL_Quit();
	
	ini_free(game_ini);
	
	re_clean_up();
	
	fprintf(log_file, "info: Engine shut down.\n");
	
	if(log_file != stdout)
		fclose(log_file);
	
	return 0;
}
