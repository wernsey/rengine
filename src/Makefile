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
	CFLAGS = -Wall -O2 -DNDEBUG
	LFLAGS = -s 
	endif
endif

INCLUDE_PATH = -I /usr/local/include -I ../include -I ..

CFLAGS += `sdl2-config --cflags` $(INCLUDE_PATH) -DUSESDL -DUSEPNG -DUSEJPG
LFLAGS += -lSDL2_mixer -lpng -lz -ljpeg

# Different executables, and -lopengl32 is required for Windows
ifeq ($(OS),Windows_NT)
GAME_BIN = ../bin/game.exe
PAKR_BIN = ../bin/pakr.exe
BACE_BIN = ../bin/bace.exe
LFLAGS += `sdl2-config --libs` -lopengl32 -llua
RES = rengine.res
else
GAME_BIN = ../bin/game
PAKR_BIN = ../bin/pakr
BACE_BIN = ../bin/bace
LFLAGS += `sdl2-config --libs` -lGL -llua5.2
RES = 
endif

EXECUTABLES = $(GAME_BIN) $(PAKR_BIN)

SOURCES= bmp.c game.c ini.c utils.c pak.c \
	states.c demo.c resources.c musl.c mustate.c hash.c \
	lexer.c tileset.c map.c json.c luastate.c log.c \
	gamedb.c sound.c paths.c mappings.c \
    lua/ls_audio.c lua/ls_game.c lua/ls_map.c lua/ls_gamedb.c \
    lua/ls_bmp.c lua/ls_gfx.c lua/ls_input.c \
	base.x.c 

FONTS = fonts/bold.xbm fonts/circuit.xbm fonts/hand.xbm fonts/normal.xbm \
		fonts/small.xbm fonts/smallinv.xbm fonts/thick.xbm

OBJECTS=$(SOURCES:.c=.o)

all: ../bin $(EXECUTABLES)

debug:
	make "BUILD=debug"
	
profile:
	make "BUILD=profile"
	
.PHONY : game

game: $(GAME_BIN)

$(GAME_BIN): $(OBJECTS) $(RES) ../bin
	$(CC) -o $@ $(OBJECTS) $(RES) $(LFLAGS) 
	
../bin:
	mkdir $@
	
.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

bmp.o: bmp.c ../include/bmp.h ../fonts/bold.xbm \
 ../fonts/circuit.xbm ../fonts/hand.xbm ../fonts/normal.xbm \
 ../fonts/small.xbm ../fonts/smallinv.xbm ../fonts/thick.xbm
demo.o: demo.c ../include/bmp.h ../include/states.h \
 ../include/game.h ../include/resources.h ../include/log.h \
 ../rengine.xbm
game.o: game.c ../include/bmp.h \
 ../include/ini.h ../include/game.h \
 ../include/utils.h ../include/states.h ../include/resources.h \
 ../include/log.h ../include/gamedb.h ../include/sound.h
hash.o: hash.c ../include/hash.h
ini.o: ini.c ../include/ini.h \
 ../include/utils.h
json.o: json.c ../include/json.h ../include/lexer.h \
 ../include/hash.h ../include/utils.h
lexer.o: lexer.c ../include/lexer.h
luop.o: luop.c 
map.o: ../src/map.c ../include/tileset.h \
 ../include/bmp.h ../include/map.h ../include/json.h \
 ../include/utils.h ../include/log.h ../include/paths.h 
luastate.o: luastate.c ../include/bmp.h \
 ../include/states.h ../include/map.h ../include/game.h ../include/ini.h \
 ../include/resources.h ../include/tileset.h ../include/utils.h \
 ../include/log.h ../include/gamedb.h
musl.o: musl.c ../include/musl.h
mustate.o: mustate.c ../include/bmp.h \
 ../include/states.h ../include/musl.h ../include/game.h ../include/ini.h \
 ../include/resources.h ../include/utils.h \
 ../include/log.h ../include/gamedb.h
pak.o: pak.c ../include/pak.h
resources.o: resources.c ../include/pak.h \
 ../include/bmp.h ../include/ini.h ../include/utils.h \
 ../include/hash.h ../include/log.h
states.o: states.c ../include/ini.h \
 ../include/bmp.h ../include/states.h ../include/utils.h \
 ../include/game.h ../include/resources.h \
 ../include/log.h ../include/hash.h
tileset.o: tileset.c ../include/bmp.h ../include/tileset.h \
 ../include/resources.h ../include/lexer.h ../include/json.h \
 ../include/utils.h ../include/log.h
utils.o: utils.c
gamedb.o: gamedb.c ../include/gamedb.h ../include/ini.h ../include/states.h \
  ../include/game.h
sound.o: sound.c ../include/resources.h ../include/log.h
paths.o: paths.c ../include/utils.h
log.o: log.c ../include/log.h

lua/ls_audio.o: lua/ls_audio.c ../include/resources.h ../include/log.h
lua/ls_game.o: lua/ls_game.c ../include/game.h ../include/luastate.h ../include/states.h
lua/ls_map.o: lua/ls_map.c ../include/tileset.h ../include/map.h ../include/luastate.h
lua/ls_gamedb.o: lua/ls_gamedb.c ../include/gamedb.h
lua/ls_bmp.o: lua/ls_bmp.c ../include/luastate.h ../include/bmp.h ../include/resources.h
lua/ls_gfx.o: lua/ls_gfx.c ../include/luastate.h ../include/bmp.h ../include/game.h  ../include/states.h
lua/ls_input.o: lua/ls_input.c ../include/luastate.h ../include/log.h ../include/game.h 

# Utilities ###################################

$(PAKR_BIN) : pakr.o pak-nosdl.o utils.o
	$(CC) -o $@ $^
	
pak-nosdl.o: pak.c ../include/pak.h
	$(CC) -c $(INCLUDE_PATH) $< -o $@
	
pakr.o : ../utils/pakr.c ../include/pak.h ../include/utils.h
	$(CC) -c $(INCLUDE_PATH) $< -o $@

$(BACE_BIN) : bace.o
	$(CC) -o $@ $^
	
bace.o : ../utils/bace.c
	$(CC) -c $< -o $@
	
# Resources ###################################
	
rengine.res : ../rengine.rc
	windres $^ -O coff -o $@

rengine.rc : ../rengine.ico

# Generated Sources ###########################

base.x.c : ../scripts/base.lua $(BACE_BIN)
	$(BACE_BIN) -n base_lua -z -o $@ $< 

###############################################

.PHONY : clean

clean:
	-rm -rf $(EXECUTABLES) $(BACE_BIN)
	-rm -rf *.o lua/*.o rengine.res
	-rm -rf *.x.c *.x.h
	-rm -rf *~ gmon.out
