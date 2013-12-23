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

CFLAGS += `sdl2-config --cflags` -I /usr/local/include -I ../include -I ..
LFLAGS += -llua -lSDL2_mixer

# Different executables, and -lopengl32 is required for Windows
ifeq ($(OS),Windows_NT)
GAME_BIN = ../bin/game.exe
LFLAGS += `sdl2-config --libs` -lopengl32
RES = rengine.res
else
GAME_BIN = ../bin/game
LFLAGS += `sdl2-config --libs` -lGL
RES = 
endif

EXECUTABLES = $(GAME_BIN)

SOURCES= bmp.c game.c ini.c utils.c pak.c particles.c \
	states.c demo.c resources.c musl.c mustate.c hash.c \
	lexer.c tileset.c map.c json.c luastate.c log.c

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
 ../include/ini.h ../include/game.h ../include/particles.h \
 ../include/utils.h ../include/states.h ../include/resources.h \
 ../include/log.h
hash.o: hash.c ../include/hash.h
ini.o: ini.c ../include/ini.h \
 ../include/utils.h
json.o: json.c ../include/json.h ../include/lexer.h \
 ../include/hash.h ../include/utils.h
lexer.o: lexer.c ../include/lexer.h
luop.o: luop.c 
map.o: ../src/map.c ../include/tileset.h \
 ../include/bmp.h ../include/map.h ../include/json.h \
 ../include/utils.h ../include/log.h
luastate.o: luastate.c ../include/bmp.h \
 ../include/states.h ../include/map.h ../include/game.h ../include/ini.h \
 ../include/resources.h ../include/tileset.h ../include/utils.h \
 ../include/particles.h ../include/log.h
musl.o: musl.c ../include/musl.h
mustate.o: mustate.c ../include/bmp.h \
 ../include/states.h ../include/musl.h ../include/game.h ../include/ini.h \
 ../include/resources.h ../include/utils.h ../include/particles.h \
 ../include/log.h
pak.o: pak.c ../include/pak.h
particles.o: particles.c ../include/bmp.h
resources.o: resources.c ../include/pak.h \
 ../include/bmp.h ../include/ini.h ../include/game.h ../include/utils.h \
 ../include/hash.h ../include/log.h
states.o: states.c ../include/ini.h \
 ../include/bmp.h ../include/states.h ../include/utils.h \
 ../include/game.h ../include/particles.h ../include/resources.h \
 ../include/log.h ../include/hash.h
tileset.o: tileset.c ../include/bmp.h ../include/tileset.h \
 ../include/lexer.h ../include/json.h ../include/utils.h \
 ../include/log.h
utils.o: utils.c

rengine.res : ../rengine.rc
	windres $^ -O coff -o $@

rengine.rc : ../rengine.ico

###############################################

.PHONY : clean

clean:
	-rm -rf $(EXECUTABLES)
	-rm -rf *.o rengine.res
	-rm -rf *~ gmon.out
