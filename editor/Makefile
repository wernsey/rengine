
CC=gcc

fltk-config = fltk-config

# Different executables, and -lopengl32 is required for Windows
ifeq ($(OS),Windows_NT)
EDIT_BIN = ../bin/editor.exe
else
EDIT_BIN = ../bin/editor
endif

CFLAGS = -c -DEDITOR -I ../include -I .. -I /usr/local/include -DUSEPNG
CPPFLAGS = -c `$(fltk-config) --cxxflags` -c -I . -I./editor -I ../include
LDFLAGS = `$(fltk-config) --ldflags` -lpng -lz

ifeq ($(BUILD),debug)
# Debug mode: Unoptimized and with debugging symbols
CPPFLAGS += -Wall -O0 -g
LDFLAGS += 
else
	ifeq ($(BUILD),profile)
	# Profile mode: Debugging symbols and profiling information.
	CPPFLAGS += -Wall -O0 -pg
	LDFLAGS += -pg
	else
	# Release mode: Optimized and stripped of debugging info
	CPPFLAGS += -Wall -Os -DNDEBUG
	LDFLAGS += -s 
	endif
endif

# Link with static libstdc++, otherwise you need to have
# libstdc++-6.dll around.
LDFLAGS += -static-libstdc++

.PHONY : editor

editor: $(EDIT_BIN)

debug: 
	make "BUILD=debug"

$(EDIT_BIN): main.o editor.o BMCanvas.o LevelCanvas.o TileCanvas.o \
				bmp.o tileset.o map.o lexer.o json.o hash.o utils.o \
				log.o paths.o
	$(CXX) -o $@ $^ $(LDFLAGS)
	
editor.o : editor.cxx editor.h 
	$(CXX) $(CPPFLAGS) $< -o $@

editor.cxx editor.h : editor.fl
	fluid -c $^
	
BMCanvas.o: BMCanvas.cpp BMCanvas.h ../include/bmp.h
	$(CXX) $(CPPFLAGS) $< -o $@

LevelCanvas.o: LevelCanvas.cpp LevelCanvas.h TileCanvas.h \
				../include/bmp.h ../include/tileset.h ../include/map.h \
				../include/utils.h ../include/log.h
	$(CXX) $(CPPFLAGS) $< -o $@
	
TileCanvas.o: TileCanvas.cpp TileCanvas.h ../include/bmp.h ../include/tileset.h
	$(CXX) $(CPPFLAGS) $< -o $@
	
main.o: main.cpp LevelCanvas.h BMCanvas.h \
		TileCanvas.h TileCanvas.h editor.h ../include/bmp.h \
		../include/tileset.h ../include/map.h ../include/paths.h
	$(CXX) $(CPPFLAGS) $< -o $@
	
bmp.o: ../src/bmp.c ../include/bmp.h ../fonts/bold.xbm \
 ../fonts/circuit.xbm ../fonts/hand.xbm ../fonts/normal.xbm \
 ../fonts/small.xbm ../fonts/smallinv.xbm ../fonts/thick.xbm
	$(CC) $(CFLAGS) $< -o $@
tileset.o: ../src/tileset.c ../include/bmp.h ../include/tileset.h \
 ../include/lexer.h ../include/json.h ../include/utils.h \
 ../include/log.h
	$(CC) $(CFLAGS) $< -o $@
map.o: ../src/map.c ../include/tileset.h \
 ../include/bmp.h ../include/map.h ../include/json.h \
 ../include/utils.h ../include/log.h ../include/paths.h
	$(CC) $(CFLAGS) $< -o $@
lexer.o: ../src/lexer.c ../include/lexer.h
	$(CC) $(CFLAGS) $< -o $@
json.o: ../src/json.c ../include/json.h ../include/lexer.h \
 ../include/hash.h ../include/utils.h
	$(CC) $(CFLAGS) $< -o $@
hash.o: ../src/hash.c ../include/hash.h
	$(CC) $(CFLAGS) $< -o $@
utils.o: ../src/utils.c ../include/utils.h
	$(CC) $(CFLAGS) $< -o $@
log.o: ../src/log.c ../include/log.h
	$(CC) $(CFLAGS) $< -o $@
paths.o: ../src/paths.c ../include/utils.h
	$(CC) $(CFLAGS) $< -o $@

.PHONY : clean

clean:
	-rm -rf $(EDIT_BIN)
	-rm -rf *.o editor.cxx editor.h
	-rm -rf *~ gmon.out