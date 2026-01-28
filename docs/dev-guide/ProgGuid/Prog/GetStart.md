| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Getting Started

Jupiter includes both engine code and the game code for the No One Lives Forever sequel. You can use the latter as a starting point for your own game and as a guide to understanding how various coding tasks are accomplished using Jupiter. This section provides to following directions for getting started developing your own game using Jupiter:

- [Component Locations ](#componentlocations)
- [Building the Jupiter Engine ](#buildingtheengine)
- [Building the Tools ](#buildingthetools)
- [Building the Game Code ](#buildingthegamecode)
- [Running the Game ](#runningthegame)

---

## Component Locations

The various Jupiter components are located in the following directories:

| \Engine\ | Engine source code. |
| --- | --- |
| \Game\ | Game source code. |
| \Development\TO2\Game\ | Game resources. |
| \Development\Tools\ | Pre-built tools. |



---

## Building the Jupiter Engine

#### To build the engine (source code licensees only)

1. Open a DOS window.
2. Switch to the \LithTech\LT_Jupiter_Src\Engine\Runtime\ directory.
3. Run the build.bat file

You can also open the build.bat file from an explorer window, but you won't be able to review the output after the build process has completed.

[Top ](#top)

---

## Building the Tools

#### To build the engine tools (source code licensees only)

1. Open a DOS window.
2. Switch to the \LithTech\LT_Jupiter_Src\Engine\Tools\ directory.
3. Run the build.bat file.

In order for the exporter plugins to build, you will need to copy over the Max and/or Maya SDK files. For source licensees, see the readme files in the \Development\Tools\Plugins\ sub-folders for more information.

[Top ](#top)

---

## Building the Game Code

Before building the Jupiter Engine you must have installed Microsoft’s DirectX 8.1 SDK. This can be obtained from Microsoft’s website.

#### To build the game code

1. Launch Microsoft Visual Studio v6.0.
2. Select Open Workspace from the File menu.
3. Browse to the \LithTech\LT_Jupiter_Bin\Game\ folder (or \LithTech\LT_Jupiter_Src\Game\ for source licensees).
4. Open the TO2.dsw project file.
5. If any Source Control dialogs appear, click No to dismiss them.
6. Select Build | Batch Build from the menu.
7. Check all project targets except for Final and Demo.
8. Select the Rebuild All button.

You may only want to build all Debug or Release targets.

[Top ](#top)

---

## Running the Game

To run the game, you can run the game launcher from one of the following two directories:

>

\LithTech\LT_Jupiter_Bin\Development\TO2\NOLF2.exe

>

-or for source licensees-

\LithTech\LT_Jupiter_Src\Development\TO2\NOLF2.exe

#### Command Line Parameters

Here is a list of command line parameters available for launching LithTech. All parameters should be passed using -<command>. In addition to the commands listed below, console variables may also be passed by using +<command and parameters>. So for example you can run LithTech using the following syntax at the command line:

LithTech –rez resource.rez +windowed 1 –WorkingDir c:\somedir\

For a list of important console variables, see [Appendix C: Console Variables ](../../Appendix/Apnd-C/Console.md).

**Breakonerror **—Determines if LithTech should break when an error occurs. This is primarily used for debugging purposes.

**config <config filename> **—Specifies the configuration file that will be used to configure LithTech. If none is specified, it defaults to autoexec.cfg. The autoexec.cfg file is always processed, even if an alternate configuration file is specified with this command.

**DebugStructBanks **—Used primarily for debugging memory leaks, this causes the memory banks to be allocated and released normally.

**Host **—Forces the host flag to be set so that LithTech will act as though it is hosting the current game.

**Launch **—This command allows the launch to be handled by the launch.dll file, which exports a function GetLithTechCommandLine that is called to parse the command line.

**noinput **—Disables the use of input to LithTech.

**rez <rez file> **—Specifies the resource that will be used. This can be either a .rez file or a resource directory. Multiple files can be specified, each with it’s own rez command. Subsequent files override duplicate resource information in previous .rez files.

**Windowtitle <title> **—Specifies the title of the window that LithTech creates.

**Workingdir <directory> **—Changes the current working directory to that specified.

**World <world name> **—Specifies the world that will be loaded upon initialization.

**Windowed **—This is a console variable and is added to the command line prepended with a +. It runs the game in a window as opposed to full screen.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Prog\GetStart.md)2006, All Rights Reserved.
