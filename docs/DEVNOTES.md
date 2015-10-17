This file contains a variety of development notes. I'll keep my FIXMEs
and TODOs in here.

# Engine

## Lua

## SDL

Docs says `SDL_UpdateTexture()` be slow. Do something different.

FIXME: I should rather use `SDL_GetKeyboardState()` to get the state of
the keys, rather than my own solution. Shouldn't be too difficult.

See here: http://wiki.libsdl.org/SDL_GetKeyboardState

## Game

Integrate FreeType fonts with the rest of the application. The files `bmpfont.c`
and `bmpfont.h` already implements the ability to load and render FreeType fonts
on a bitmap, but the API is not exposed elsewhere.

Also, rendering FreeType fonts to the bitmap does not respect the bitmap's
clipping rectangle.

Allow the using of bitmap cursors.

~~The scheme of pushing/popping states has some implications on the
save game system. And on the sprite system I intend to add later.~~

At the moment I'm seriously considering removing the ability to push/pop
states. The reason is that being able to push/pop states doesn't really 
add any benefit that can't be achieved through the Game Database.

Pros of bing able to push/pop states:
* You can easily do things like the player walking through a town then 
entering a store (pushing the town state), and when he leaves the store
again popping the state to return the player to the town.
* If you pop a state, you already have that state's resources in memory.
Likewise, if you push a state that reuses an old state's resources then
they can be reused.

Cons:
* You need to somehow save the stack of states when saving the game, and
then reliably restore it again when the game is loaded.
* What about music that's playing when you change states through 
pushing/popping?

If I do this, I should also remove that functionality from the resources
module.

For now, I've just surrounded the functions with `#ifdef 0`s. I can decide
later. It seems that the ability to push/pop states are still useful if you
want to change state to a system menu (Like configuring your input device
and so on).

I should consider changing ini.c to use hash.c hash tables rather than its
own built in binary trees. The first obvious reason is the O(1) lookup, but
another advantage will be the ability to iterate through the INI file using
`ht_next()`

## Graphics

The `bm_fill()` function in `bmp.c` doesn't take the clipping rectangle
into account. I'm not quite sure how to handle it.

The `bmp.c` can do with an `bm_arc(bmp, start_angle, end_angle, radius)`
function.

`bmp.c` is also still missing a `bm_fillellipse()` function. It is not
high on my list of priorities.

There are some inconsistencies in how function parameters are applied. For
example, in bm_rect() the x1,y1 parameters are inclusive, while in bm_clip() 
the x1,y1 parameters are exclusive. I'm thinking that the clipping parameters
should be made inclusive, but it is a bigger change.

## Resources

Wrt. the PAK file module, the ZIP file format turns out to be
not that different, so I should look into it more closely.

## Input

I should have a way to map physical input
(like key pressed, mouse moved or screen swiped) to an abstracted input
(like move left, jump, open door). This will help if I ever get around
to porting it to devices without keyboards and mouses.

## Musl

I should remove Musl from Rengine completely. The thing I had in mind when
I put it in can now be done in Lua as the engine evolved. Keeping Musl now
forces me to maintain two scripting language bindings.

## Audio

In order to compile SDL_mixer with support for Ogg Vorbis (under MinGW), I 
had to set the CFLAGS and LDFLAGS environment variables when I ran `./configure`
in SDL_mixer:

	# cd libvorbis-1.3.4/
	./configure
	make
	make install
	# cd libogg-1.3.1/
	./configure
	make
	make install
	# cd SDL2_mixer-2.0.0
	export CFLAGS=-I/usr/local/include/
	export LDFLAGS=-L/usr/local/lib
	./configure
	make
	make install

# Editor

Changing the Working Directory affects the tilesets, so you should rather
set the working directory when you start a new project. I should simply
disable the menu option after you've started editing the level. This
implies that there should be a `SetDirty()` function that changes the
titlebar, causes the user to be prompted for a save on exit, and disables
the set working directory menu option.

# Other Tools

## Pakr

Pakr is a small utility to create id Software style 
[PAK files](http://en.wikipedia.org/wiki/PAK_%28file_format%29), such as 
those found in Quake.

Rengine has functionality to read all its resources from PAK files,
and this is the intended mechanism for redistributing Rengine-based games.

```
Usage: $ ./pakr [options] pakfile [files...]
where options:
 -d dir      : Create/Overwrite pakfile from directory dir.
 -c          : Create/Overwrite pakfile from files.
 -x          : Extract pakfile into current directory.
 -a          : Append files to pakfile.
 -u          : dUmps the contents of a file.
 -t          : Dumps the contents of a text file.
 -o file     : Set the output file for -u and -t.
 -h          : Include hidden files when using the -d option.
 -v          : Verbose mode. Each -v increase verbosity.

If no options are specified, the file is just listed.
If the -o option is not used, files are written to stdout.
The extract option doesn't attempt to preserve the directory structure.
```

More information on the file format can be found here:
http://debian.fmi.uni-sofia.bg/~sergei/cgsr/docs/pak.txt

## Bace

Bace (pronounced "bake") is used in the Rengine build process to 
convert Lua scripts into C code so that they can be _baked_ into the
final executable.

```
Usage: $ ./bace [options] infile
where options:
 -o file     : Set the output file (default: stdout)
 -n name     : Name of the created variable.
 -z          : Append a '\0' to to the end of the generated array.
 -v          : Verbose mode. Each -v increase verbosity.
 ```

# Documentation

## Wiki Documentation 

I use the script `mddoc.awk` to generate Markdown for 
[Rengine's wiki](https://github.com/wernsey/rengine/wiki) on GitHub.

To use the script, invoke it like so:
```
> awk -f mddoc.awk src/luastate.c > output.md
```
and then paste the contents of `output.md` in GitHub's wiki editor.
