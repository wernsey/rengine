Rengine
=======

A Retro Game Engine.

Rengine is intended for _Retro-style_ games. The word _style_ is used
because while games running under Rengine will look retro, they are by
no means limited to the CPU and memory constraints of the computers of
yore.

Rengine also includes a 2D level editor for editing maps.

The home of Rengine is at https://github.com/wernsey/rengine

Technical
---------

At the lowest level, Rengine displays a single OpenGL quad on the screen.

It uses the resolution of this texture to render low resolution (retro) 
graphics onto a modern high resolution display. Rengine contains lots of 
code for manipulating said texture. 

It uses SDL for cross-platform operation.

Dependencies
------------

Rengine is dependant upon these 3rd party libraries:
* SDL (2.0.0) - Cross-platform low level game library - http://www.libsdl.org
* Lua (5.2.2) - Scripting language - http://www.lua.org
* FLTK (1.3.2) - Cross-platform GUI toolkit, used for the map editor - http://www.fltk.org

On Linux systems, these libraries can be found in your package manager.

On Windows, Rengine is built with [MinGW](http://mingw.org/). See the
webpages of the packages mentioned above for details on how to compile
them under Windows.

License
-------

Rengine is distributed under the terms of the MIT License. See the LICENSE 
file for details.

Also refer to the [Wikipedia article](http://en.wikipedia.org/wiki/MIT_License)
for more information.
