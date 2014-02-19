This file contains a variety of development notes. I'll keep my FIXMEs
and TODOs in here.

# Engine

## Lua

## SDL

At the moment I have SDL_mixer 2.0.0 built, but without any support for 
additional file formats.

Docs says `SDL_UpdateTexture()` be slow. Do something different.

## Game

The scheme of pushing/popping states has some implications on the
save game system. And on the sprite system I intend to add later.

## Graphics

The `bm_fill()` function in `bmp.c` doesn't take the clipping rectangle
into account. I'm not quite sure how to handle it.

The `bmp.c` can do with an `bm_arc(bmp, start_angle, end_angle, radius)`
function.

`bmp.c` is also still missing a `bm_fillellipse()` function. it is not
high on my list of priorities.

## Resources

Also wrt. the PAK file module, the ZIP file format turns out to be
not that different, so I should look into it more closely.

## Input

I should have a way to map physical input
(like key pressed, mouse moved or screen swiped) to an abstracted input
(like move left, jump, open door). This will help if I ever get around
to porting it to devices without keyboards and mouses.

## Musl

I should remove Musl from Rengine completely. The thing I had in mind when
I put it in can now be done in Lua as the engine involved. Having Musl now
forces me to maintain two scripting language bindings.

# Editor

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

## Wiki Documentation 

I use the script `mddoc.awk` to generate Markdown for 
[Rengine's wiki](https://github.com/wernsey/rengine/wiki) on GitHub.

To use the script, invoke it like so:
```
> awk -f mddoc.awk src/luastate.c > output.md
```
and then paste the contents of `output.md` in GitHub's wiki editor.
