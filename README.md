# Rengine

A Retro Game Engine.

Rengine is intended for _Retro-style_ games. The word _style_ is used
because while games running under Rengine will look retro, they are by no
means limited to the CPU and memory constraints of the computers of yore.

Rengine also includes a 2D level editor for editing maps.

* The home of Rengine is at https://github.com/wernsey/rengine
* Documentation can be found on the [Rengine wiki](https://github.com/wernsey/rengine/wiki)
	page on GitHub.

# Technical

It uses SDL for cross-platform operation.

Rengine implements a state machine where the parameters of the states
are read from a configuration file.

Some states simply display a static screen and waits for user input, while
others invoke a scripting engine to draw graphics and control the game.

## Dependencies

Rengine is dependant upon these 3rd party libraries:

* SDL (2.0.1) - Cross-platform low level game library -
http://www.libsdl.org

* Lua (5.2.2) - Scripting language - http://www.lua.org

* SDL_mixer (2.0.0) - Mixer library (audio) for SDL -
http://www.libsdl.org/projects/SDL_mixer/

* libvorbis-1.3.4 and libogg-1.3.1 - Open audio encoding and 
	streaming technology - https://www.xiph.org/downloads/

* FLTK (1.3.2) - Cross-platform GUI toolkit, used for the map editor -
http://www.fltk.org

On Linux systems, these libraries can be found in your package manager.

On Windows, Rengine is built with [MinGW](http://mingw.org/). See the
webpages of the packages mentioned above for details on how to compile
them under Windows.

## License

Rengine is distributed under the terms of the MIT License. See the
`LICENSE.md` file for details.

There are no restrictions on any games produced with the engine.

Also refer to the [Wikipedia
article](http://en.wikipedia.org/wiki/MIT_License) for more information.


