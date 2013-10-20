all: engine level_editor

engine:
	cd src && make

level_editor:
	cd editor && make
	
clean:
	cd src && make clean;
	cd editor && make clean