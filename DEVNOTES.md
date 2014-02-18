This file contains a variety of development notes. I'll keep my FIXMEs
and TODOs in here.

# Engine

## Lua

We should consider disabling some _unsafe_ Lua built in functions such as
`dofile()`, `load()` and `loadfile()`, etc.

It appears that _Sandbox_ is the term you're looking for.
See http://lua-users.org/wiki/SandBoxes. Also read the discussion
of the topic at http://stackoverflow.com/q/966162/115589 and
http://stackoverflow.com/q/1224708/115589

It seems that the accepted solution is to not call `luaL_openlibs()`
and rather copying and modifying linit.c according to your needs.

## SDL

I still have to put audio into the engine. At the moment I have SDL_mixer
2.0.0 built, but without any support for additional file formats.

Docs says `SDL_UpdateTexture()` be slow. Do something different.

## Game

The scheme of pushing/popping states has some implications on the
save game system. And on the sprite system I intend to add later.

## Graphics

My Bitmap module should have a `#pragma pack()` at the
structure declaration. The PAK file module should have
it too. See the bottom of this section on the wikipedia:
http://en.wikipedia.org/wiki/Data_structure_alignment in the section
"Typical alignment of C structs on x86" (Seems it is not a big GCC issue,
rather a MSVC thing)

The `bm_fill()` function in `bmp.c` doesn't take the clipping rectangle
into account. I'm not quite sure how to handle it.

The `bmp.c` can do with an `bm_arc(bmp, start_angle, end_angle, radius)`
function.

`bmp.c` is also still missing a `bm_fillellipse()` function. it is not
high on my list of priorities.

## Resources

Also wrt the PAK file module, the ZIP file format turns out to be
not that different, so I should look into it more closely.

## Input

I should have a way to map physical input
(like key pressed, mouse moved or screen swiped) to an abstracted input
(like move left, jump, open door). This will help if I ever get around
to porting it to devices without keyboards and mouses.

## Musl

I don't like the fact that you have to `strdup()` the string you
return to Musl in the case of an external Musl function returning a
string. The interpreter should be modified so that Musl does a `strdup()`
after calling the external function in `fparams()` in musl.c (look for
`v->v.fun()`).

The current way of doing it has its pros, so I should think about it
first before committing to a course of action. The best place to think
about it is where I actually use the API (which at the moment is Rengine)

# Editor

FIXME: _"Save on exit"_ prompt.

Changing the Working Directory affects the tilesets, so you should rather
set the working directory when you start a new project. I should simply
disable the menu option after you've started editing the level. This
implies that there should be a `SetDirty()` function that changes the
titlebar, causes the user to be prompted for a save on exit, and disables
the set working directory menu option.

~~I should actually have the tilesets be part of the map structure. This
will help with managing resources when I implement the push/pop of game
states later. _Are the tileset bitmaps obtained through the resources
module?_~~

A menu option to reload the current tileset bitmap. The use case is that
you may have the file open in a paint program while you're editing your
level, and want to see how your changes affect the level.

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

I keep this file (and README.md) nicely formatted with this command:
```
fmt DEVNOTES.md > DEVNOTES.md~; mv DEVNOTES.md~ DEVNOTES.md
```

## Wiki Documentation 

I use the script `mddoc.awk` to generate Markdown for 
[Rengine's wiki](https://github.com/wernsey/rengine/wiki) on GitHub.

To use the script, invoke it like so:
```
> awk -f mddoc.awk src/luastate.c > output.md
```
and then paste the contents of `output.md` in GitHub's wiki editor.
