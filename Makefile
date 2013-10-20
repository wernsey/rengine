ifeq ($(BUILD),debug)
	TARGET = debug
else
	TARGET =
endif

all: engine level_editor

debug: 
	make "BUILD=debug"

engine:
	cd src && make  $(TARGET)



level_editor:
	cd editor && make $(TARGET)
	
clean:
	cd src && make clean;
	cd editor && make clean
	