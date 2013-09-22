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

# -lopengl32 is required for Windows
ifeq ($(OS),Windows_NT)
LFLAGS += `sdl-config --libs` -lopengl32
else
LFLAGS += `sdl-config --libs` -lGL
endif

SOURCES= bmp.c game.c ini.c utils.c pak.c particles.c \
	states.c demo.c resources.c musl.c mustate.c hash.c \
	lexer.c tileset.c map.c json.c

FONTS = fonts/bold.xbm fonts/circuit.xbm fonts/hand.xbm \
		fonts/normal.xbm fonts/small.xbm fonts/smallinv.xbm fonts/thick.xbm

OBJECTS=$(SOURCES:.c=.o)
all: game editor

debug:
	make "BUILD=debug"
	
profile:
	make "BUILD=profile"

.PHONY : game

game: bin/game

bin/game: $(OBJECTS) bin
	$(CC) -o $@ $(OBJECTS) $(LFLAGS) 
	
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

lexer.o : lexer.h

tileset.o : tileset.h bmp.h lexer.h json.h utils.h

map.o : map.h tileset.h bmp.h json.h utils.h

json.o : json.h lexer.h hash.h utils.h

###############################################

fltk-config = fltk-config

CPPFLAGS = `$(fltk-config) --cxxflags` -c -I . -I./editor
LPPFLAGS = `$(fltk-config) --ldflags` 

# Link with static libstdc++, otherwise you need to have
# libstdc++-6.dll around.
LPPFLAGS += -static-libstdc++

.PHONY : editor

editor: bin/editor

bin/editor: main.o editor.o BMCanvas.o LevelCanvas.o TileCanvas.o bmp.o tileset.o map.o lexer.o json.o hash.o utils.o
	g++ -o $@  $^ $(LPPFLAGS)

main.o: editor/main.cpp editor.h
	g++ -c $(CPPFLAGS) $< -o $@
	
editor.o : editor.cxx editor.h 
	g++ -c $(CPPFLAGS) $< -o $@

editor.cxx editor.h : editor/editor.fl
	fluid -c $^

BMCanvas.o: editor/BMCanvas.cpp 
	g++ -c $(CPPFLAGS) $< -o $@

LevelCanvas.o: editor/LevelCanvas.cpp 
	g++ -c $(CPPFLAGS) $< -o $@

TileCanvas.o: editor/TileCanvas.cpp 
	g++ -c $(CPPFLAGS) $< -o $@

###############################################

.PHONY : clean

clean:
	-rm -rf bin/game bin/game.exe bin/editor bin/editor.exe
	-rm -rf *.o editor.cxx editor.h
	-rm -rf *~ gmon.out
