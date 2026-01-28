| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# File Structure

Jupiter games require several different files to run. Some of Jupiter's files contain engine code, and other files contain game codethat you must create. Example files can be found in the game code, located in the following directory:

>

<installation directory>\Development\TO2\

## Engine Code Files

These files contain engine code.

**LITHTECH.EXE **—This is the core of the Jupiter system. It functions as the client, and is the program that actually displays what the player sees on the screen, handles input from the player and communicates it to the game, and so on. It also contains all of the server code for Jupiter. When running a single player game or hosting a network game, this functions as the server. LithTech.exe loads cshell.dll and cres.dll. It also loads object.lto and sres.dll when functioning as a server.

**SERVER.DLL **—This is the server module for Jupiter. The server-only parts of Jupiter are separated into server.dll. This allows the developer to write a program to load the server portion of Jupiter and function as a standalone application. The server.dll file loads object.lto and sres.dll when run as a standalone server.

## Game Code Files

These files contain game-specific code. The game code versions of these are located in the \Development\TO2\Game\ directory .

**CSHELL.DLL **—This file must implement a client shell. IClientShell is a Jupiter class that has certain functions Jupiter knows it can call into, such as the main update function, or the initialization function.

**CRES.DLL **—This is a standard Windows resource DLL, containing strings and other resource information. Instead of hard-coding dialog and text into your game, it is a good idea to use this DLL for things like a string-table to facilitate localization. This file should also contain graphics for hardware 2D cursor support.

**CRESL.DLL **—This file is optional. If cresl.dll is present, any resources in cres.dll that are duplicated in cresl.dll will be used from cresl.dll instead of cres.dll. You can put data that has been localized into other languages into this file and thus not have to change any of your game to port it to another language.

**OBJECT.LTO **—You must include all the server logic code in this file, such as artificial intelligence routines for enemies and scorekeeping for multiplayer games. This file should also define all the objects in your game. The DEdit tool can read them from this file directly. This allows a programmer to make a change to what properties a level designer can set on an object and have the changes immediately reflected in the level-editing package.

**SRES.DLL **—This file is rarely used, but is the server equivalent of cres.dll.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Prog\FileStru.md)2006, All Rights Reserved.
