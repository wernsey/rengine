# Rengine

An Open-Source Retro Game Engine.

Rengine is intended for _Retro-style_ games. The word _style_ is used
because while games running under Rengine will look retro, they are by no
means limited to the CPU and memory constraints of the computers of yore.

Rengine also includes a 2D level editor for editing maps.

* The home of Rengine is at https://github.com/wernsey/rengine
* Documentation can be found on the [Rengine wiki](https://github.com/wernsey/rengine/wiki)
    page on GitHub.

Rengine is distributed under the terms of the MIT License, which means that
it can be used for just about any purpose. See below for details.

# Technical

It uses SDL for cross-platform operation.

Rengine implements a state machine where the parameters of the states
are read from a configuration file.

Some states simply display a static screen and waits for user input, while
others invoke a scripting engine to draw graphics and control the game.

## Dependencies

Rengine is dependant upon these 3rd party libraries. The version numbers in
parenthesis are the latest specific versions that rengine was built and
tested against:

* SDL (2.0.1) - Cross-platform low level game library -
http://www.libsdl.org

* Lua (5.3.1) - Scripting language - http://www.lua.org

* SDL_mixer (2.0.0) - Mixer library (audio) for SDL -
http://www.libsdl.org/projects/SDL_mixer/

* libvorbis (1.3.5) and libogg (1.3.2) - Open audio encoding and
    streaming technology - https://www.xiph.org/downloads/

* libpng (1.6.18) - Portable Network Graphics reference library
    used for support of the PNG file format
    http://www.libpng.org/pub/png/libpng.html

* zlib (1.2.8) - Compression library, used by libpng.
    http://www.zlib.net/

* libjpeg (release 9a) - Library for working with JPEG image files.
    http://www.ijg.org/

* freetype (2.6) - Library for loading and rendering fonts.
    http://www.freetype.org/

* FLTK (1.3.3) - Cross-platform GUI toolkit, used for the map editor -
http://www.fltk.org

On Linux systems, these libraries can be found in your package manager.
For example, on Ubuntu 14.04, the following should suffice:

    $ sudo apt-get install libsdl2-dev liblua5.2-dev \
               zlib1g-dev libpng12-dev libjpeg-dev \
               libsdl2-mixer-dev libogg-dev libvorbis-dev \
               libfreetype6-dev libfltk1.3-dev

Since SDL2 is relatively new, it may not be available in your
package manager, and you may have to compile SDL2 and SDL2_mixer yourself
(I know that SDL2 is not available in Debian _Wheezy_, for instance).
In this case, first install the abovementioned packages that are
available, then for both those libraries download the sources and run

   ./configure
   make
   sudo make install

If you've compiled SDL yourself and Rengine complains about `No such audio device`
then you need to install `libasound2-dev` like so

    $ sudo apt-get install libasound2-dev

and recompile SDL2 (`libpulse-dev` may also work - Haven't tried it - more
info [here](http://www.gamedev.net/topic/646010-sdl2-mixer-no-such-audio-device-solved/)).

If you receive an error `cannot find -lGL`, you need to install OpenGL development libraries:

    $ sudo apt-get install libgl1-mesa-dev

On Windows, Rengine is built with [MinGW](http://mingw.org/). See the
webpages of the packages mentioned above for details on how to compile
them under Windows.

## License

Rengine is distributed under the terms of the MIT License.

The `LICENSE.md` file contains the specific details, along with the
details of the library dependencies.

Here is a summary of the licenses of Rengine's 3rd party dependencies:
* SDL2 is licensed under the zlib license.
* Lua is licensed under the MIT license
* libogg and libvorbis are licensed under the New BSD license.
* libpng is licensed under the [libpng license](http://en.wikipedia.org/wiki/Libpng_License)
* zlib is licensed under the [zlib license](http://en.wikipedia.org/wiki/Zlib_license).
* libjpeg uses a custom free software license.
* FreeType uses a [BSD-style license](http://git.savannah.gnu.org/cgit/freetype/freetype2.git/tree/docs/FTL.TXT)
* FLTK is provided under the terms of the LGPL with an exception for
    static linking.

There are no restrictions on any games produced with the engine.
