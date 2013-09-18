CC=gcc

ifeq ($(BUILD),debug)
# Debug mode: Unoptimized and with debugging symbols
CFLAGS = -Wall -O0 -g
LFLAGS = 
else
	ifeq ($(BUILD),profile)
	# Profile mode: Debugging symbols and profiling information.
	CFLAGS = -Wall -O0 -pg
	LFLAGS = -pg
	else
	# Release mode: Optimized and stripped of debugging info
	CFLAGS = -Wall -Os -DNDEBUG
	LFLAGS = -s 
	endif
endif

CFLAGS += `sdl-config --cflags`
LFLAGS += `sdl-config --libs` -lopengl32
SOURCES= bmp.c game.c ini.c utils.c pak.c particles.c \
	states.c demo.c resources.c musl.c mustate.c hash.c \
	lexer.c

FONTS = fonts/bold.xbm fonts/circuit.xbm fonts/hand.xbm \
		fonts/normal.xbm fonts/small.xbm fonts/smallinv.xbm fonts/thick.xbm

OBJECTS=$(SOURCES:.c=.o)
all: game 

debug:
	make "BUILD=debug"
	
profile:
	make "BUILD=profile"

game: $(OBJECTS) bin
	$(CC) -o bin/$@ $(OBJECTS) $(LFLAGS) 
	
bin:
	mkdir $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

bmp.o : bmp.h $(FONTS)

game.o : bmp.h ini.h game.h particles.h utils.h states.h resources.h

particles.o : bmp.h

ini.o : utils.h

pak.o : pak.h

states.o : states.h bmp.h ini.h utils.h game.h particles.h resources.h

resources.o : pak.h bmp.h ini.h game.h utils.h hash.h

demo.o : states.h bmp.h game.h resources.h

musl.o : musl.h

mustate.o : bmp.h states.h musl.h game.h ini.h resources.h utils.h

lexer.c : lexer.h

###############################################

.PHONY : clean

clean:
	-rm -rf bin/game bin/game.exe
	-rm -rf *.o 
	-rm -rf *~ gmon.out