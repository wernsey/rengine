 [Lua](http://www.lua.org/home.html) is a popular scripting language for games. 
 Through Rengine's Lua State you get access to a Lua interpreter, with most of 
 Rengine's functionality exposed.



 Links:

*  The [Lua Homepage](http://www.lua.org/home.html)
*  A great introduction to Lua is the book [Programming In Lua](http://www.lua.org/pil/contents.html)


#  Global functions 
 These functions have global scope in the Lua script.
####  log(message)
 Writes a message to Rengine's log file.
####  import(path)
 Loads a Lua script from the resource on the specified path.
 This function is needed because the standard Lua functions like
 require() and dofile() are disabled in the Rengine sandbox.
####  setTimeout(func, millis)
 Waits for `millis` milliseconds, then calls `func`
####  onUpdate(func)
 Registers the function `func` to be called every frame
 when Rengine draws the screen.
####  atExit(func)
 Registers the function `func` to be called when the
 current state is exited.

 These functions should not do any drawing.
####  advanceFrame()
 Low-level function to advance the animation frame on the screen.

 It flips the back buffer, retrieves user inputs and processes system 
 events.

 In general you should not call this function directly, since Rengine
 does the above automatically. This function is provided for the special
 case where you need a Lua script to run in a loop, and during each 
 iteration update the screen and get new user input.

 An example of such a case is drwaing a GUI completely in a Lua script.

 It does not clear the screen - you'll need to do that yourself.
 Timeouts set through `setTimeout()` will also be processed.
#  Game object
 Functions in the `Game` scope
####  Game.changeState(newstate)
 Changes the game's [[state|State Machine]] to the state identified by `newstate`
####  Game.getStyle(style, [default])
 Retrieves a specific [[Style]] from the [[game.ini]] file.
#  BmpObj
 The Bitmap object encapsulates a bitmap in the engine.
 Instances of BmpObj are drawn to the screen with the [[G.blit()|Lua-state#gblitbmp-dx-dy-sx-sy-w-h]] function
####  Bmp(filename)
 Loads the bitmap file specified by `filename` from the
 [[Resources|Resource Management]] and returns it
 encapsulated within a `BmpObj` instance.
####  BmpObj:__tostring()
 Returns a string representation of the `BmpObj` instance.
####  BmpObj:__gc()
 Garbage collects the `BmpObj` instance.
####  BmpObj:clone()
 Returns a copy of the `BmpObj` instance.
####  BmpObj:setMask(color)
 Sets the color used as a mask when the bitmap is drawn to the screen.
####  BmpObj:width()
 Returns the width of the bitmap
####  BmpObj:height()
 Returns the height of the bitmap
####  BmpObj:adjust(R,G,B, [A]), BmpObj:adjust(v)  
 Adjusts the color of the bitmap by multiplying the RGB value of each pixel 
 in the bitmap with the factors R, G, B and A.

 If A is not specified, it is taken as 1.0 (leaving the alpha channel unchanged).

 If only one parameter `v` is specified, `v` is used for the R, G and B values.
 In this case, the alpha channel is also treated as 1.0.
####  R,G,B = BmpObj:getColor([x,y])
 Gets the [[color|Colors]] used as mask by the bitmap.

 If the `x,y` parameters are supplied, the color of the
 pixel at `x,y` is returned.
#  Map
 The Map object provides access to the Map through
 a variety of functions.



 These fields are also available:

*  `Map.BACKGROUND` - Constant for the Map's background layer. See `Map.render()`
*  `Map.CENTER` - Constant for the Map's center layer. See `Map.render()`
*  `Map.FOREGROUND` - Constant for the Map's foreground layer. See `Map.render()`
*  `Map.ROWS` - The number of rows in the map.
*  `Map.COLS` - The number of columns in the map.
*  `Map.TILE_WIDTH` - The width (in pixels) of the cells on the map.
*  `Map.TILE_HEIGHT` - The height (in pixels) of the cells on the map.


 The `Map` object is only available if the `map`
 parameter has been set in the state's configuration
 in the `game.ini` file.
####  Map.render(layer, [scroll_x, scroll_y])
 Renders the specified layer of the map (background, center or foreground).
 The center layer is where all the action in the game occurs and sprites and so on moves around.
 The background is drawn behind the center layer for objects in the background. It needs to be drawn first,
 so that the center and foreground layers are drawn over it.
 The foreground layer is drawn last and contains objects in the foreground. 
####  Map.cell(r,c)
 Returns a cell on the map at row `r`, column `c` as 
 a `CellObj` instance.

 `r` must be between `1` and `Map.ROWS` inclusive.
 `c` must be between `1` and `Map.COLS` inclusive.
#  CellObj
 The CellObj encapsulates an individual cell on the 
 map. You then use the methods of the CellObj to manipulate
 the cell.



 Cell Objects are obtained through the `Map.cell(r,c)` function.
####  CellObj:__tostring()
 Returns a string representation of the CellObj
####  CellObj:__gc()
 Frees the CellObj when it is garbage collected.
####  CellObj:set(layer, si, ti)
 Sets the specific layer of the cells encapsulated in the CellObj to 
 the `si,ti` value, where

*  `si` is the Set Index
*  `ti` is the Tile Index


####  CellObj:getId()
 Returns the _id_ of a cell as defined in the Rengine Editor
####  CellObj:getClass()
 Returns the _class_ of a cell as defined in the Rengine Editor
####  CellObj:isBarrier()
 Returns whether the cell is a barrier
####  CellObj:setBarrier(b)
 Sets whether the cell is a barrier
#  G
 `G` is the Graphics object that allows you to draw primitives on the screen. 

 You can only call these functions when the screen is being drawn. That's to say, you can only call then inside 
 functions registered through `onUpdate()`
 If you call them elsewhere, you will get the error _"Call to graphics function outside of a screen update"_



 These fields are also available:

*  `G.FPS` - The configured frames per second of the game (see [[game.ini]]).
*  `G.SCREEN_WIDTH` - The configured width of the screen.
*  `G.SCREEN_HEIGHT` - The configured height of the screen.


####  G.setColor("color"), G.setColor(R, G, B), G.setColor()
 Sets the [[color|Colors]] used to draw the graphics primitives

 `G.setColor("color")` sets the color to the specified string value.

 `G.setColor(R, G, B)` sets the color to the specified R, G, B values.
 R,G,B values must be in the range [0..255].

 `G.setColor()` sets the color to the foreground specified in the styles.
####  R,G,B = G.getColor()
 Gets the [[color|Colors]] used to draw the graphics primitives.

####  G.clip(x0,y0, x1,y1)
 Sets the clipping rectangle when drawing primitives.
####  G.unclip()
 Resets the clipping rectangle when drawing primitives.
####  G.pixel(x,y)
 Plots a pixel at `x,y` on the screen.
####  G.line(x0, y0, x1, y1)
 Draws a line from `x0,y0` to `x1,y1`
####  G.rect(x0, y0, x1, y1)
 Draws a rectangle from `x0,y0` to `x1,y1`
####  G.fillRect(x0, y0, x1, y1)
 Draws a filled rectangle from `x0,y0` to `x1,y1`
####  G.circle(x, y, r)
 Draws a circle centered at `x,y` with radius `r`
####  G.fillCircle(x, y, r)
 Draws a filled circle centered at `x,y` with radius `r`
####  G.ellipse(x0, y0, x1, y1)
 Draws an ellipse from `x0,y0` to `x1,y1`
####  G.roundRect(x0, y0, x1, y1, r)
 Draws a rectangle from `x0,y0` to `x1,y1`
 with rounded corners of radius `r`
####  G.fillRoundRect(x0, y0, x1, y1, r)
 Draws a rectangle from `x0,y0` to `x1,y1`
 with rounded corners of radius `r`
####  G.curve(x0, y0, x1, y1, x2, y2)
 Draws a Bezier curve from `x0,y0` to `x2,y2`
 with `x1,y1` as the control point.
 Note that it doesn't pass through `x1,y1`
####  G.lerp(col1, col2, frac)
 Returns a [[color|Colors]] that is some fraction `frac` along 
 the line from `col1` to `col2`.
 For example, `G.lerp("red", "blue", 0.33)` will return a color
 that is 1/3rd of the way from red to blue.
####  G.setFont(font)
 Sets the [[font|Fonts]] used for the `G.print()` function. 
####  G.print(x,y,text)
 Prints the `text` to the screen, with its top left position at `x,y`.
####  G.textDims(text)
 Returns the width,height in pixels that the `text`
 will occupy on the screen.

**Example:**
```
 local w,h = G.textDims(message);
```
####  G.blit(bmp, dx, dy, [sx, sy, [dw, dh, [sw, sh]]])
 Draws an instance `bmp` of `BmpObj` to the screen at `dx, dy`.

 `sx,sy` specify the source x,y position and `dw,dh` specifies the
 width and height of the destination area to draw.

 `sx,sy` defaults to `0,0` and `dw,dh` defaults to the entire 
 source bitmap.

 If `sw,sh` is specified, the bitmap is scaled so that the area on the 
 source bitmap from `sx,sy` with dimensions `sw,sh` is drawn onto the
 screen at `dx,dy` with dimensions `dw, dh`.
#  Keyboard
 Keyboard Input Functions 
####  Keyboard.down([key])
 Checks whether a key is down on the keyboard.
 The parameter `key` is the name of specific key.
 See http://wiki.libsdl.org/SDL_Scancode for the names of all the possible keys.
 If `key` is omitted, the function returns true if _any_ key is down.
####  Keyboard.pressed(key)
 Checks whether a key has been pressed during the last frame.
 The parameter `key` is the name of specific key.
 See http://wiki.libsdl.org/SDL_Scancode for the names of all the possible keys.
####  Keyboard.released(key)
 Checks whether a key has been released during the last frame.
 The parameter `key` is the name of specific key.
 See http://wiki.libsdl.org/SDL_Scancode for the names of all the possible keys.
####  Keyboard.reset()
 Resets the keyboard input.
#  Mouse
 Mouse input functions.



 These constants are used with `Mouse.down()` and `Mouse.click()`
 to identify specific mouse buttons:

*  `Mouse.LEFT`
*  `Mouse.MIDDLE`
*  `Mouse.RIGHT`


####  x,y = Mouse.position()
 Returns the `x,y` position of the mouse.

**Example:**
```
 local x,y = Mouse.position();
```
####  Mouse.down(btn)
 Returns true if the button `btn` is down.
####  Mouse.click(btn)
 Returns true if the button `btn` was clicked.
 A button is considered clicked if it was down the
 previous frame and is not anymore this frame.
#  SndObj
 The sound object that encapsulates a sound (WAV) file in the engine.
####  Wav(filename)
 Loads the WAV file specified by `filename` from the
 [[Resources|Resource Management]] and returns it
 encapsulated within a `SndObj` instance.
####  SndObj:__tostring()
 Returns a string representation of the `SndObj` instance.
####  SndObj:__gc()
 Garbage collects the `SndObj` instance.
#  Sound
 The `Sound` object exposes functions through which 
 sounds can be played an paused in your games.
####  Sound.play(sound)
 Plays a sound.
 `sound` is a `SndObj` object previously loaded through
 the `Wav(filename)` function.
 It returns the `channel` the sound is playing on, which can be used
 with other `Sound` functions.
####  Sound.loop(sound, [n])
 Loops a sound `n` times. 
 If `n` is omitted, the sound will loop indefinitely.
 `sound` is a `SndObj` object previously loaded through
 the `Wav(filename)` function.
 It returns the `channel` the sound is playing on, which can be used
 with other `Sound` functions.
####  Sound.pause([channel])
 Pauses a sound.
 `channel` is the channel previously returned by
 `Sound.play()`. If it's omitted, all sound will be paused.
####  Sound.resume([channel])
 Resumes a paused sound on a specific channel.
 `channel` is the channel previously returned by
 `Sound.play()`. If it's omitted, all sound will be resumed.
####  Sound.halt([channel])
 Halts a sound playing on a specific channel.
 `channel` is the channel previously returned by
 `Sound.play()`. If it's omitted, all sound will be halted.
####  Sound.playing([channel])
 Returns whether the sound on `channel` is currently playing.
 If `channel` is omitted, it will return an integer containing the
 number of channels currently playing.
####  Sound.paused([channel])
 Returns whether the sound on `channel` is currently paused.
 If `channel` is omitted, it will return an integer containing the
 number of channels currently paused.
####  Sound.volume([channel,] v)
 Sets the volume 
 `v` should be a value between `0` (minimum volume) and `1` (maximum volume).
 If `channel` is omitted, the volume of all channels will be set.
 It returns the volume of the `channel`. If the channel is not specified, it
 will return the average volume of all channels. To get the volume of a channel
 without changing it, use a negative value for `v`.
#  MusicObj
 The music object that encapsulates a music file in the engine.
####  Music(filename)
 Loads the music file specified by `filename` from the
 [[Resources|Resource Management]] and returns it
 encapsulated within a `MusObj` instance.
####  MusicObj:play([loops])
 Plays a piece of music.
####  MusicObj:halt()
 Stops music.
####  MusicObj:__tostring()
 Returns a string representation of the `MusicObj` instance.
####  MusicObj:__gc()
 Garbage collects the `MusObj` instance.
#  GameDB
 The _Game Database_. 
 These functions are used to store key-value pairs in a
 globally available memory area. The purpose is twofold:

*  To share data between different game states.
*  To save game-specific variables in the "Save Game"/"Load Game" functionality.


####  GameDB.set(key, value)
 Saves a key-value pair in the game database.

 `key` and `value` are converted to strings internally.
####  GameDB.get(key, [default])
 Retrieves the value associated with the `key` from the game database (as a string).

 If the key does not exist in the database it will either return `default` 
 if it is provided, otherwise `nil`.
####  GameDB.has(key)
 Returns `true` if the `key` is stored in the Game database.
####  LocalDB.set(key, value)
 Saves a key-value pair in the game database.

 `key` and `value` are converted to strings internally.
####  LocalDB.get(key, [default])
 Retrieves the value associated with the `key` from the game database (as a string).

 If the key does not exist in the database it will either return `default` 
 if it is provided, otherwise `nil`.
####  LocalDB.has(key)
 Returns `true` if the `key` is stored in the Game database.




