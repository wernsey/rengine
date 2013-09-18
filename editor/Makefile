fltk-config = fltk-config

CPPFLAGS = `$(fltk-config) --cxxflags` -c -I ../
LDFLAGS = `$(fltk-config) --ldflags` 

# Link with static libstdc++, otherwise you need to have
# libstdc++-6.dll around.
LDFLAGS += -static-libstdc++

ifeq ($(BUILD),debug)
# Debug mode: Unoptimized and with debuging symbols
CFLAGS += -Wall -O0 -g -DTEST
LDFLAGS +=
else
# Release mode: Optimized and stripped of debugging info
CFLAGS += -Wall -Os -DNDEBUG
LDFLAGS += -s
endif

all: editor

debug:
	make "BUILD=debug"

editor: main.o BMCanvas.o LevelCanvas.o TileCanvas.o bmp.o tileset.o map.o lexer.o json.o hash.o utils.o
	g++ -o $@  $^ $(LDFLAGS)
		
.c.o:
	gcc -c $(CFLAGS) $< -o $@

.cpp.o:
	g++ -c $(CPPFLAGS) $< -o $@

main.o : main.cpp ../bmp.h LevelCanvas.h

LevelCanvas.o : LevelCanvas.cpp LevelCanvas.h BMCanvas.h ../map.h TileCanvas.h

TileCanvas.o : TileCanvas.cpp TileCanvas.h BMCanvas.h ../tileset.h

BMCanvas.o : BMCanvas.cpp BMCanvas.h

bmp.o : ../bmp.c ../bmp.h
	gcc -c $(CFLAGS) -o $@ $<
	
tileset.o : ../tileset.c ../tileset.h ../bmp.h ../lexer.h ../json.h
	gcc -c $(CFLAGS) -o $@ $<
	
map.o : ../map.c ../map.h ../tileset.h ../bmp.h
	gcc -c $(CFLAGS) -o $@ $<
	
lexer.o : ../lexer.c ../lexer.h
	gcc -c $(CFLAGS) -o $@ $<
	
json.o : ../json.c ../json.h ../lexer.h ../hash.h ../utils.h
	gcc -c $(CFLAGS) -o $@ $<
	
utils.o : ../utils.c ../utils.h
	gcc -c $(CFLAGS) -o $@ $<
	
hash.o : ../hash.c ../hash.h
	gcc -c $(CFLAGS) -o $@ $<

.PHONY : clean

clean:
	-rm -rf editor editor.exe
	-rm -rf *.o
	-rm -rf test*.bmp
