| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Building and Running Jupiter

- [Building from the Command Line ](#BuildingfromtheCommandLine)
- [Building from Visual Studio ](#BuildingfromVisualStudio)
- [Running the Samples ](#RunningtheSamples)
- [Running No One Lives Forever™ 2 ](#RunningTheGame)
- [Debugging the Samples ](#DebuggingtheSamples)
- [Launching the Game Within DEdit ](#LaunchingTheGameWithinDEdit)

## Building from the Command Line

>

### To build the engine from the command line:

>

1. Open a DOS window.
2. Switch to the \TouchdownEntertainment\Jupiter\Engine\Runtime\ directory.
3. Run the **build.bat **file for Visual Studio .NET 2003 (v7.1) or the **build2002.bat **file for Visual Studio .NET 2002 (v7).
4. Open the build.log file to view the output.

You can also open the build.bat file from an explorer window, but you won't be able to review the output after the build process has completed.

### To build the engine tools from the command line:

>

1. Open a DOS window.
2. Switch to the \TouchdownEntertainment\Jupiter\Engine\Tools\ directory.
3. Run the **build.bat **file for Visual Studio .NET 2003 (v7.1) or the **build2002.bat **file for Visual Studio .NET 2002 (v7).
4. Open the build.log file to view the output.

In order for the exporter plugins to build, you will need to copy over the Max and/or Maya SDK files. See the readme files in the \Engine\Tools\Plugins\ sub-folders for more information.

### To build the samples from the command line:

>

1. Open a DOS window.
2. Switch to the \TouchdownEntertainment\Jupiter\Samples\ directory.
3. Run the **build.bat **file for Visual Studio .NET 2003 (v7.1) or the **build2002.bat **file for Visual Studio .NET 2002 (v7).
4. Open the build.log file to view the output.

### To build the No One Lives Forever™ 2 game code from the command line:

>

1. Open a DOS window.
2. Switch to the \TouchdownEntertainment\Jupiter\Game\ directory.
3. Run the **build.bat **file for Visual Studio .NET 2003 (v7.1) or the **build2002.bat **file for .NET 2002 (v7).
4. Open the build.log file to view the output.

## Building from Visual Studio

>

### Visual Studio .NET 2003 (v7.1) Instructions:

> To build the engine code from Visual Studio .NET 2003 (v7.1):

1. Right click on the \TouchdownEntertainment\Jupiter\Engine\Runtime\Engine.sln file and select **Open With Visual Studio .NET 2003 **( you may also double click on the sln file ).
2. If any **Source Control **dialogs appear, click **No **or **Cancel **to dismiss them.
3. Select **Rebuild Solution **from the **Build **menu.
To build the engine tools code from Visual Studio .NET 2003 (v7.1):

1. Right click on the \TouchdownEntertainment\Jupiter\Engine\Tools\Tools.sln file and select **Open With Visual Studio .NET 2003 **( you may also double click on the sln file ).
2. If any **Source Control **dialogs appear, click **No **or **Cancel **to dismiss them.
3. Select **Rebuild Solution **from the **Build **menu.
To build the samples from Visual Studio .NET 2003 (v7.1):

1. Right click on the project solution file (*.SLN) in the sample folder (Ex. \TouchdownEntertainment\Jupiter\Samples\base\samplebase\samplebase.sln) and select **Open With Visual Studio .NET 2003 **( you may also double click on the sln file ).
2. If any **Source Control **dialogs appear, click **No **or **Cancel **to dismiss them.
3. Select **Rebuild Solution **from the **Build **menu.
To build the No One Lives Forever™ 2 game code from Visual Studio .NET 2003 (v7.1):

1. Right click on the \TouchdownEntertainment\Jupiter\Game\TO2.sln file and select **Open With Visual Studio .NET 2003 **( you may also double click on the sln file ).
2. If any **Source Control **dialogs appear, click **No **or **Cancel **to dismiss them.
3. Select **Rebuild Solution **from the **Build **menu.

### Visual Studio .NET 2002 (v7) Instructions:

> To build the engine code from Visual Studio .NET 2002 (v7):

1. Right click on the \TouchdownEntertainment\Jupiter\Engine\Runtime\Engine2002.sln file and select **Open With Visual Studio .NET 2002 **( you may also double click on the sln file ).
2. If any **Source Control **dialogs appear, click **No **or **Cancel **to dismiss them.
3. Select **Rebuild Solution **from the **Build **menu.
To build the engine tools code from Visual Studio .NET 2002 (v7):

1. Right click on the \TouchdownEntertainment\Jupiter\Engine\Tools\Tools2002.sln file and select **Open With Visual Studio .NET 2002 **( you may also double click on the sln file ).
2. If any **Source Control **dialogs appear, click **No **or **Cancel **to dismiss them.
3. Select **Rebuild Solution **from the **Build **menu.
To build the samples from Visual Studio.NET 2002 (v7):

1. Right click on the project solution file (*2002.SLN) in the sample folder (Ex. \TouchdownEntertainment\Jupiter\Samples\base\samplebase\samplebase2002.sln). and select **Open With Visual Studio .NET 2002 **( you may also double click on the sln file ).
2. If any **Source Control **dialogs appear, click **No **or **Cancel **to dismiss them.
3. Select **Rebuild Solution **from the **Build **menu.
To build the No One Lives Forever™ 2 game code from Visual Studio .NET 2002 (v7):

1. Right click on the \TouchdownEntertainment\Jupiter\Game\TO22002.sln file and select **Open With Visual Studio .NET 2002 **( you may also double click on the sln file ).
2. If any **Source Control **dialogs appear, click **No **or **Cancel **to dismiss them.
3. Select **Rebuild Solution **from the **Build **menu.

## Running the Samples

>

### To run the samples from the command line:

>

To run the samples from the command line, first build the sample and then execute the **run.bat **file for Visual Studio .NET 2003 (v7.1) or the **run2002.bat **file for Visual Studio .NET 2002 (v7) in the root directory of the sample. ( Ex. \TouchdownEntertainment\Jupiter\Samples\base\samplebase\run.bat. This would run the Visual Studio .NET 2003 release build of the sample. )

## Running No One Lives Forever™ 2

>

To run No One Lives Forever™ 2, you can run the game launcher from:

\TouchdownEntertainment\Jupiter\Development\TO2\NOLF2.exe

## Debugging the Samples

>

### To debug the samples from Visual Studio .NET 2003 (v7.1):

>

To debug the samples using Visual Studio .NET 2003 (v7.1), follow the instructions in the previous section to build the engine. Next, run **copy_binaries.bat **in the \TouchdownEntertainment\Jupiter\Samples\bin folder. In Visual Studio .NET 2003, open the project properties for **cshell **and click the **Debugging **section and enter the following parameters:

**Command: **

> Enter the path and filename for the lithtech executable: \TouchdownEntertainment\Jupiter\Samples\bin\debug\lithtech.exe

**Command Arguments: **

> Enter the following text:

-rez engine.rez -rez ..\..\sample_type\sample_dir\rez -config ..\..\sample_type\sample_dir\bin\autoexec.cfg

Note that "sample_type\sample_dir" is the directory where the sample exists. For example, the command arguments for the SampleBase sample would be:

-rez engine.rez -rez ..\..\base\samplebase\rez -config ..\..\base\samplebase\bin\autoexec.cfg

**Working Directory: **

> Enter the path to the Samples bin\debug directory: \TouchdownEntertainment\Jupiter\Samples\bin\debug

### To debug the samples from Visual Studio .NET 2002 (v7):

>

To debug the samples using Visual Studio .NET 2002 (v7), follow the instructions in the previous section to build the engine. Next, run **copy_binaries2002.bat **in the \TouchdownEntertainment\Jupiter\Samples\bin folder. In Visual Studio .NET 2002, open the project properties for **cshell **and click the **Debugging **section and enter the following parameters:

**Command: **

> Enter the path and filename for the lithtech executable: \TouchdownEntertainment\Jupiter\Samples\bin\debug2002\lithtech.exe

**Command Arguments: **

> Enter the following text:

-rez engine.rez -rez ..\..\sample_type\sample_dir\rez -config ..\..\sample_type\sample_dir\bin\autoexec.cfg

Note that "sample_type\sample_dir" is the directory where the sample exists. For example, the command arguments for the SampleBase sample would be:

-rez engine.rez -rez ..\..\base\samplebase\rez -config ..\..\base\samplebase\bin\autoexec.cfg

**Working Directory: **

>

Enter the path to the Samples bin\debug2002 directory: \TouchdownEntertainment\Jupiter\Samples\bin\debug2002

## Launching the Game Within DEdit

>

### To launch No One Lives Forever™ 2 from DEdit:

1. Run DEdit.
2. Select **Edit **and then **Options **.
3. Select the **Run **tab.
4. Enter the following parameters:

**Executable: **

> Enter the path and filename for the lithtech executable: C:\TouchdownEntertainment\Jupiter\Development\TO2\Lithtech.exe

**Working directory: **

> Enter the path to the root of the NOLF2 game directory: C:\TouchdownEntertainment\Jupiter\Development\TO2

**Program arguments: **

> Enter the following text: -rez engine.rez -rez ..\..\Game +runworld %WorldName%

5. Select **OK **.
6. When you have a world open, you can now click the **Run World **button to launch the game.

### To launch a sample from DEdit:

1. Run DEdit.
2. Select **Edit **and then **Options **.
3. Select the **Run **tab.
4. Enter the following parameters:

**Executable: **

> Enter the path and filename for the lithtech executable: C:\TouchdownEntertainment\Samples\bin\Release\Lithtech.exe

**Working directory: **

> Enter the path to the lithtech executable: C:\TouchdownEntertainment\Samples\bin\Release

**Program arguments: **

> Enter the following text:

-rez engine.rez -rez ..\..\sample_type\sample_dir\rez -config ..\..\sample_type\sample_dir\bin\autoexec.cfg +runworld %WorldName%

Note that "sample_type\sample_dir" is the directory where the sample exists. For example, the program arguments for the SampleBase sample would be:

-rez engine.rez -rez ..\..\base\samplebase\rez -config ..\..\base\samplebase\bin\autoexec.cfg +runworld %WorldName%

5. Select **OK **.
6. When you have a world open, you can now click the **Run World **button to launch the game.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Intro\building_running.md)2006, All Rights Reserved.
