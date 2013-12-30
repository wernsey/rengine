
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <SDL.h>
#include <SDL_mixer.h>

#include "resources.h"
#include "log.h"

int snd_init() {
	int audio_rate = 22050;
	Uint16 audio_format = AUDIO_S16;
	int audio_channels = 2;
	int audio_buffers = 4096;
	
	rlog("Opening Audio.");
	
	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		rerror("Unable to open audio: %s", Mix_GetError());
		return 0;
	}
	return 1;
}

void snd_deinit() {
	rlog("Closing Audio.");
	Mix_CloseAudio();
}
