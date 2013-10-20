all: engine editor

engine:
	cd src && make

editor:
	cd editor && make
	
clean:
	cd src && make clean
	cd editor && make clean