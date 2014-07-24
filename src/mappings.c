#ifdef WIN32
#	include <SDL.h>
#else
#	include <SDL2/SDL.h>
#endif

int scancode_to_ascii(int code, int shift, int  caps, int numl) {
	/* Sorry: This assumes an American keyboard layout.
	I'm not sure what the best way would be to handle this,
	but I have neither the experience nor the equipment 
	necessary to make it work with other keyboard layout. */
	
	if(code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z) {
		if(shift ^ caps)			
			return code - SDL_SCANCODE_A + 'A';
		return code - SDL_SCANCODE_A + 'a';
	}
    
    if(numl && code >= SDL_SCANCODE_KP_1 && code <= SDL_SCANCODE_KP_0) {
        if(code == SDL_SCANCODE_KP_0)
            return '0';
        return code - SDL_SCANCODE_KP_1 + '1';
    }
    
	switch(code) {
		case SDL_SCANCODE_1 : return shift?'!':'1';
		case SDL_SCANCODE_2 : return shift?'@':'2';
		case SDL_SCANCODE_3 : return shift?'#':'3';
		case SDL_SCANCODE_4 : return shift?'$':'4';
		case SDL_SCANCODE_5 : return shift?'%':'5';
		case SDL_SCANCODE_6 : return shift?'^':'6';
		case SDL_SCANCODE_7 : return shift?'&':'7';
		case SDL_SCANCODE_8 : return shift?'*':'8';
		case SDL_SCANCODE_9 : return shift?'(':'9';
		case SDL_SCANCODE_0 : return shift?')':'0';
		case SDL_SCANCODE_SLASH : return shift?'?':'/';
		case SDL_SCANCODE_SEMICOLON : return shift?':':';';
		case SDL_SCANCODE_LEFTBRACKET : return shift?'{':'[';
		case SDL_SCANCODE_RIGHTBRACKET : return shift?'}':']';
		case SDL_SCANCODE_COMMA : return shift?'<':',';
		case SDL_SCANCODE_PERIOD : return shift?'>':'.';
		case SDL_SCANCODE_MINUS : return shift?'_':'-';
		case SDL_SCANCODE_EQUALS : return shift?'+':'=';
		case SDL_SCANCODE_APOSTROPHE : return shift?'"':'\'';
		case SDL_SCANCODE_BACKSLASH : return shift?'|':'\\';
		case SDL_SCANCODE_GRAVE : return shift?'~':'`';
		
		case SDL_SCANCODE_SPACE : return ' ';
		case SDL_SCANCODE_RETURN : 
		case SDL_SCANCODE_KP_ENTER : return '\n';
		case SDL_SCANCODE_TAB : return '\t';
		case SDL_SCANCODE_BACKSPACE : return '\b';
		case SDL_SCANCODE_DELETE : return 0x7F;
		case SDL_SCANCODE_ESCAPE : return 0x1B;
    
		case SDL_SCANCODE_KP_PLUS : return '+';
		case SDL_SCANCODE_KP_MINUS : return '-';
		case SDL_SCANCODE_KP_MULTIPLY : return '*';
		case SDL_SCANCODE_KP_DIVIDE : return '/';
		case SDL_SCANCODE_KP_PERIOD : return '.';
		default: return '\0';
	}
}