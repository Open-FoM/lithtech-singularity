| ### Content Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Appendix C: Console Variables

Console variables set a wide variety of parameters in a Jupiter game. A large number of these are very useful for debugging purposes. Others are used for communications settings and other engine-related behavior.

Console variable values are checked by Jupiter at various times depending on the specific console variable. Some are checked only at game initialization, and therefore changing them during the game will not have any effect on that game session. It may, however, effect future game sessions as the value is saved to the autoexec.cfg file. Others are checked every frame.

This appendix provides a list of the must useful console variables. Valid values listed in angle brackets. Default values are listed in parentheses. Use 1 for TRUE and 0 for FALSE.

## Autoexec.cfg

The autoexec.cfg file is the repository for console variable values used in your game. This file must be located in the same folder as the LithTech.exe file.

You can set console variable values in this file in three ways:

1. Manually pre-set the values in a text editor and save it prior to game start up.
2. Use the Console to reset values while the game is running.
3. Programmatically change the values in your game code.

Jupiter overwrites the new values over existing values in the autoexec.cfg file when the game shuts down. Console variables that did not exist in the autoexec.cfg file prior to game startup will not be added to the file even if they are set in the console or programmatically.

To pre-set the values for any console command in the .CFG file, use the following syntax:

>

command value

The **command **argument is the console command and the **value **argument is the value to which it is set.

## The Console

For debugging purposes, console variables can be manually set during a running game using the Console. The Console is a text-based entry window in which you can assign console variable values. To access the console, press the TILDE (~) key in a running Jupiter application. In the Console you can directly access the client and server portions of Jupiter to give commands and to use debugging options.

### Programmatic Console Variable Access and Modification

Console variables can be programmatically accessed and modified using a variety of functions. **HCONSOLEVAR **, defined in the iltserver.h header file, is a handle to a console variable.

The following functions are defined in the iltclient.h header file:

>
```
HCONSOLEVAR ILTClient::GetConsoleVar(char *pName);
```

```
LTRESULT ILTClient::GetSConValueFloat(char *pName, float &val);
```

```
LTRESULT ILTClient::GetSConValueString(char *pName, char *valBuf, uint32 bufLen);
```

The following functions are defined in the iltcsbase.h header file:

>
```
float GetVarValueFloat(HCONSOLEVAR hVar);
```

```
char* ILTCSBase::GetVarValueString(HCONSOLEVAR hVar);
```

The following functions are defined in the iltserver.h header file:

>
```
void (*SetGameConVar)(char *pName, char *pVal);
```

```
HCONVAR (*GetGameConVar)(char *pName);
```

```
void (*RunGameConString)(char *pString);
```

### Input

**AddAction **—Sets a game action name to an integer. A game action is a name that defines a specific input action for a game. For instance, FirePistol could be a game action bound to a mouse click event.

**EnableDevice **—Enables an input device for recognition by Jupiter.

**RangeBind **—Binds specific inputs to specific commands.

### Miscellaneous

**ListCommands **—Lists in the console all available commands.

**LockPVS(0) **—When set, locks the PVS (Potentially Visible Set). Useful for finding what is in your PVS at any given point in a level.

**OptimizeSurfaces(0) **—Enable/disable the creation of the tiles for optimized surfaces.

**Quit **—Exit the Jupiter game.

**Serv **—Prefix a console variable or command to run it on the server.

**Worlds **—Use this console command to start a new world. Specify the path to the world in the .rez file (or directory) but do not specify an extension.

### Object Rendering Options

**DrawCanvases(1) **— Enable/disable the drawing of canvases.

**DrawLineSystems(1) **—Enable/disable the drawing of line systems.

**DrawParticles(1) **—Enable/disable the drawing of particles.

**DrawPolygrids(1) **—Enable/disable the drawing of poly grids.

**DrawSky(1) **—Enable/disable the drawing of the sky.

**DrawSprites(1) **—Enable/disable the drawing of sprites.

**DrawWorldModels(1) **— Enable/disable the drawing of world models.

### Performance

**ShowFrameRate **—Prints the frame rate to the console.

**ShowMemStats **—Prints information about memory usage to the console.

**ShowPolyCounts **—Prints information about polygon drawing to the console.

**ShowTickCounts<0-9> **—Prints information about where time is being spent in the engine each frame.

### Rendering Options

**32bitTextures(0) **—Enables selection of 32-bit Texture Format.

**BitDepth **—<16, 24, 32> (16) Screen Pixel Bit Depth. You must **RestartRender **for the new **BitDepth **to take effect. **BitDepth **will fall back to a lower value if necessary.

**RestartRender **—This is a console command that will recreate the renderer and all data associated with it. This must be run after any changes to the other Rendering Options to make them take effect.

**ScreenHeight(480) **—Screen Height. You must **RestartRender **for the new **ScreenHeight **to take effect.

**ScreenWidth(640) **—Screen Width. You must **RestartRender **for the new **ScreenWidth **to take effect.

### World Rendering Options

**WireFrame(0) **—When true, draws the world and objects in WireFrame mode.

**ShowSplits(0) **—Shows the polygon splits on polygons in the world as a different color when **DrawFlat **is on.

**DrawCurrentLeaf(0) **—Highlights the polygons that form the current leaf of the BSP. As the player moves around the world, the highlighting reveals how the world’s hulls are organized.

**DrawFlat(0) **—Draws all of the polygons in the world as a flat color.

**Saturate(0) **—Use Saturate blend for World Geometry.

**LightMapsOnly(0) **—Draws only the LightMaps and not the color texture on the world.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Appendix\Apnd-C\Console.md)2006, All Rights Reserved.
