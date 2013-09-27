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

# Different executables, and -lopengl32 is required for Windows
ifeq ($(OS),Windows_NT)
GAME_BIN = bin/game.exe
EDIT_BIN = bin/editor.exe
LFLAGS += `sdl-config --libs` -lopengl32
RES = rengine.res
else
GAME_BIN = bin/game
EDIT_BIN = bin/editor
LFLAGS += `sdl-config --libs` -lGL
RES = 
endif

EXECUTABLES = $(GAME_BIN) $(EDIT_BIN)

SOURCES= bmp.c game.c ini.c utils.c pak.c particles.c \
	states.c demo.c resources.c musl.c mustate.c hash.c \
	lexer.c tileset.c map.c json.c

FONTS = fonts/bold.xbm fonts/circuit.xbm fonts/hand.xbm fonts/normal.xbm \
		fonts/small.xbm fonts/smallinv.xbm fonts/thick.xbm

OBJECTS=$(SOURCES:.c=.o)

all: $(EXECUTABLES)

debug:
	make "BUILD=debug"
	
profile:
	make "BUILD=profile"
	
.PHONY : game

game: $(GAME_BIN)

$(GAME_BIN): $(OBJECTS) $(RES) bin
	$(CC) -o $@ $(OBJECTS) $(RES) $(LFLAGS) 
	
bin:
	mkdir $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

bmp.o: bmp.c bmp.h $(FONTS)
demo.o: demo.c bmp.h states.h game.h resources.h rengine.xbm
game.o: game.c bmp.h ini.h game.h particles.h utils.h states.h resources.h
hash.o: hash.c hash.h
ini.o: ini.c ini.h utils.h
json.o: json.c json.h lexer.h hash.h utils.h
lexer.o: lexer.c lexer.h
map.o: map.c tileset.h bmp.h map.h json.h utils.h
musl.o: musl.c musl.h
mustate.o: mustate.c musl.h bmp.h states.h game.h ini.h resources.h utils.h
pak.o: pak.c pak.h
particles.o: particles.c bmp.h
resources.o: resources.c pak.h bmp.h ini.h game.h utils.h hash.h
states.o: states.c ini.h bmp.h states.h utils.h game.h particles.h resources.h
tileset.o: tileset.c bmp.h tileset.h lexer.h json.h utils.h
utils.o: utils.c 

rengine.res : rengine.rc
	windres $^ -O coff -o $@

rengine.rc : rengine.ico

###############################################

fltk-config = fltk-config

CPPFLAGS = `$(fltk-config) --cxxflags` -c -I . -I./editor
LPPFLAGS = `$(fltk-config) --ldflags` 

# Link with static libstdc++, otherwise you need to have
# libstdc++-6.dll around.
LPPFLAGS += -static-libstdc++

.PHONY : editor

editor: $(EDIT_BIN)

$(EDIT_BIN): main.o editor.o BMCanvas.o LevelCanvas.o TileCanvas.o \
				bmp.o tileset.o map.o lexer.o json.o hash.o utils.o
	g++ -o $@  $^ $(LPPFLAGS)
	
editor.o : editor.cxx editor.h 
	g++ -c $(CPPFLAGS) $< -o $@

editor.cxx editor.h : editor/editor.fl
	fluid -c $^
	
BMCanvas.o: editor/BMCanvas.cpp editor/BMCanvas.h bmp.h
	g++ -c $(CPPFLAGS) $< -o $@

LevelCanvas.o: editor/LevelCanvas.cpp editor/LevelCanvas.h editor/TileCanvas.h \
				bmp.h tileset.h map.h
	g++ -c $(CPPFLAGS) $< -o $@
	
TileCanvas.o: editor/TileCanvas.cpp editor/TileCanvas.h bmp.h tileset.h
	g++ -c $(CPPFLAGS) $< -o $@
	
main.o: editor/main.cpp editor/LevelCanvas.h editor/BMCanvas.h \
		editor/TileCanvas.h editor/TileCanvas.h editor.h bmp.h tileset.h map.h
	g++ -c $(CPPFLAGS) $< -o $@

###############################################

.PHONY : clean

clean:
	-rm -rf $(EXECUTABLES)
	-rm -rf *.o editor.cxx editor.h rengine.res
	-rm -rf *~ gmon.out
