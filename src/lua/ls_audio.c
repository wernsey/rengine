#ifdef WIN32
#include <SDL.h>
#include <SDL_mixer.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#endif

#include "log.h"
#include "resources.h"

/*1 SndObj
 *# The sound object that encapsulates a sound (WAV) file in the engine.
 */

/*@ Wav(filename)
 *# Loads the WAV file specified by {{filename}} from the
 *# [[Resources|Resource Management]] and returns it
 *# encapsulated within a `SndObj` instance.
 */
static int new_wav_obj(lua_State *L) {
	const char *filename = luaL_checkstring(L,1);
	
	struct Mix_Chunk **cp = lua_newuserdata(L, sizeof *cp);	
	luaL_setmetatable(L, "SndObj");
	
	*cp = re_get_wav(filename);
	if(!*cp) {
		luaL_error(L, "Unable to load WAV file '%s'", filename);
	}
	return 1;
}

/*@ SndObj:__tostring()
 *# Returns a string representation of the `SndObj` instance.
 */
static int wav_tostring(lua_State *L) {	
	struct Mix_Chunk **cp = luaL_checkudata(L,1, "SndObj");
	struct Mix_Chunk *c = *cp;
	lua_pushfstring(L, "SndObj[%p]", c->abuf);
	return 1;
}

/*@ SndObj:__gc()
 *# Garbage collects the `SndObj` instance.
 */
static int gc_wav_obj(lua_State *L) {
	/* No need to free the Mix_Chunk: It's in the resource cache. */
	return 0;
}

static void wav_obj_meta(lua_State *L) {
	/* Create the metatable for MyObj */
	luaL_newmetatable(L, "SndObj");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index"); /* SndObj.__index = SndObj */
			
	lua_pushcfunction(L, wav_tostring);
	lua_setfield(L, -2, "__tostring");	
	lua_pushcfunction(L, gc_wav_obj);
	lua_setfield(L, -2, "__gc");	
	
	/* The global method Wav() */
	lua_pushcfunction(L, new_wav_obj);
	lua_setglobal(L, "Wav");
}

/*1 Sound
 *# The {{Sound}} object exposes functions through which 
 *# sounds can be played an paused in your games.
 */

/*@ Sound.play(sound)
 *# Plays a sound.
 *# {{sound}} is a {{SndObj}} object previously loaded through
 *# the {{Wav(filename)}} function.
 *# It returns the {{channel}} the sound is playing on, which can be used
 *# with other {{Sound}} functions.
 */
static int sound_play(lua_State *L) {
	struct Mix_Chunk **cp = luaL_checkudata(L,1, "SndObj");
	struct Mix_Chunk *c = *cp;
	int ch = Mix_PlayChannel(-1, c, 0);
	if(ch < 0) {
		luaL_error(L, "Sound.play(): %s", Mix_GetError());
	}
	lua_pushinteger(L, ch);
	return 1;
}

/*@ Sound.loop(sound, [n])
 *# Loops a sound {{n}} times. 
 *# If {{n}} is omitted, the sound will loop indefinitely.
 *# {{sound}} is a {{SndObj}} object previously loaded through
 *# the {{Wav(filename)}} function.
 *# It returns the {{channel}} the sound is playing on, which can be used
 *# with other {{Sound}} functions.
 */
static int sound_loop(lua_State *L) {
	struct Mix_Chunk **cp = luaL_checkudata(L,1, "SndObj");
	struct Mix_Chunk *c = *cp;
	
	int n = -1;
	if(lua_gettop(L) > 0) {
		n = luaL_checkinteger(L, 2);
	}
	if(n < 1) n = 1;
	
	int ch = Mix_PlayChannel(-1, c, n - 1);
	if(ch < 0) {
		luaL_error(L, "Sound.loop(): %s", Mix_GetError());
	}
	lua_pushinteger(L, ch);
	return 1;
}

/*@ Sound.pause([channel])
 *# Pauses a sound.
 *# {{channel}} is the channel previously returned by
 *# {{Sound.play()}}. If it's omitted, all sound will be paused.
 */
static int sound_pause(lua_State *L) {
	int channel = -1;
	if(lua_gettop(L) > 0) {
		channel = luaL_checkinteger(L, 1);
	}
	Mix_Pause(channel);
	return 0;
}

/*@ Sound.resume([channel])
 *# Resumes a paused sound on a specific channel.
 *# {{channel}} is the channel previously returned by
 *# {{Sound.play()}}. If it's omitted, all sound will be resumed.
 */
static int sound_resume(lua_State *L) {
	int channel = -1;
	if(lua_gettop(L) > 0) {
		channel = luaL_checkinteger(L, 1);
	}
	Mix_Resume(channel);
	return 0;
}

/*@ Sound.halt([channel])
 *# Halts a sound playing on a specific channel.
 *# {{channel}} is the channel previously returned by
 *# {{Sound.play()}}. If it's omitted, all sound will be halted.
 */
static int sound_halt(lua_State *L) {
	int channel = -1;
	if(lua_gettop(L) > 0) {
		channel = luaL_checkinteger(L, 1);
	}
	if(Mix_HaltChannel(channel) < 0) {
		luaL_error(L, "Sound.pause(): %s", Mix_GetError());
	}
	return 0;
}

/*@ Sound.playing([channel])
 *# Returns whether the sound on {{channel}} is currently playing.
 *# If {{channel}} is omitted, it will return an integer containing the
 *# number of channels currently playing.
 */
static int sound_playing(lua_State *L) {
	if(lua_gettop(L) > 0) {
		int channel = luaL_checkinteger(L, 1);
		lua_pushboolean(L, Mix_Playing(channel));
	}
	lua_pushinteger(L, Mix_Playing(-1));
	return 1;
}

/*@ Sound.paused([channel])
 *# Returns whether the sound on {{channel}} is currently paused.
 *# If {{channel}} is omitted, it will return an integer containing the
 *# number of channels currently paused.
 */
static int sound_paused(lua_State *L) {
	if(lua_gettop(L) > 0) {
		int channel = luaL_checkinteger(L, 1);
		lua_pushboolean(L, Mix_Paused(channel));
	}
	lua_pushinteger(L, Mix_Paused(-1));
	return 1;
}

/*@ Sound.volume([channel,] v)
 *# Sets the volume 
 *# {{v}} should be a value between {{0}} (minimum volume) and {{1}} (maximum volume).
 *# If {{channel}} is omitted, the volume of all channels will be set.
 *# It returns the volume of the {{channel}}. If the channel is not specified, it
 *# will return the average volume of all channels. To get the volume of a channel
 *# without changing it, use a negative value for {{v}}.
 */
static int sound_volume(lua_State *L) {
	int channel = -1;
	double v = -1;
	int vol = -1;
	if(lua_gettop(L) > 1) {
		channel = luaL_checkinteger(L, 1);
		v = luaL_checknumber(L, 2);	
	} else {
		v = luaL_checknumber(L, 1);	
	}
	if(v < 0) {
		vol = -1;
	} else {
		if(v > 1.0){
			v = 1.0;
		}
		vol = v * MIX_MAX_VOLUME;
	}		
	lua_pushinteger(L, Mix_Volume(channel, vol));
	return 1;
}

static const luaL_Reg sound_funcs[] = {
  {"play",  sound_play},
  {"loop",  sound_loop},
  {"pause",  sound_pause},
  {"resume",  sound_resume},
  {"halt",  sound_halt},
  {"playing",  sound_playing},
  {"paused",  sound_paused},
  {"volume",  sound_volume},
  {0, 0}
};

/*1 MusicObj
 *# The music object that encapsulates a music file in the engine.
 */

/*@ Music(filename)
 *# Loads the music file specified by {{filename}} from the
 *# [[Resources|Resource Management]] and returns it
 *# encapsulated within a `MusObj` instance.
 */
static int new_mus_obj(lua_State *L) {
	const char *filename = luaL_checkstring(L,1);
	
	Mix_Music **mp = lua_newuserdata(L, sizeof *mp);	
	luaL_setmetatable(L, "MusicObj");
	
	*mp = re_get_mus(filename);
	if(!*mp) {
		luaL_error(L, "Unable to load music file '%s'", filename);
	}
	return 1;
}

/*@ MusicObj:play([loops])
 *# Plays a piece of music.
 */
static int mus_play(lua_State *L) {	
	Mix_Music **mp = luaL_checkudata(L,1, "MusicObj");
	Mix_Music *m = *mp;
	
	int loops = -1;
	
	if(lua_gettop(L) > 1) {
		loops = luaL_checkinteger(L, 2);
	}
	
	if(Mix_PlayMusic(m, loops) == -1) {
		rerror("Unable to play music: %s", Mix_GetError());
	}	
		
	return 0;
}

/*@ MusicObj:halt()
 *# Stops music.
 */
static int mus_halt(lua_State *L) {	
	luaL_checkudata(L,1, "MusicObj");
	
	Mix_HaltMusic();
		
	return 0;
}

/*@ MusicObj:__tostring()
 *# Returns a string representation of the `MusicObj` instance.
 */
static int mus_tostring(lua_State *L) {	
	Mix_Music **mp = luaL_checkudata(L,1, "MusicObj");
	Mix_Music *m = *mp;
	lua_pushfstring(L, "MusicObj[%p]", m);
	return 1;
}

/*@ MusicObj:__gc()
 *# Garbage collects the `MusObj` instance.
 */
static int gc_mus_obj(lua_State *L) {
	/* No need to free the Mix_Music: It's in the resource cache. */
	return 0;
}

static void mus_obj_meta(lua_State *L) {
	/* Create the metatable for MusicObj */
	luaL_newmetatable(L, "MusicObj");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index"); /* MusicObj.__index = MusicObj */
			
	lua_pushcfunction(L, mus_play);
	lua_setfield(L, -2, "play");	
	lua_pushcfunction(L, mus_halt);
	lua_setfield(L, -2, "halt");	
	
	lua_pushcfunction(L, mus_tostring);
	lua_setfield(L, -2, "__tostring");	
	lua_pushcfunction(L, gc_mus_obj);
	lua_setfield(L, -2, "__gc");	
	
	/* The global method Music() */
	lua_pushcfunction(L, new_mus_obj);
	lua_setglobal(L, "Music");
}


void register_sound_functions(lua_State *L) {    
	luaL_newlib(L, sound_funcs);
	lua_setglobal(L, "Sound");	
	wav_obj_meta(L);	
	mus_obj_meta(L);
}