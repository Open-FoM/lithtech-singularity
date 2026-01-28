| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# —QUICK START—
Viewing Your Model in Jupiter

This section describes how to quickly view one of your models in Jupiter. The following general steps describe how to import and convert models created in Maya or Max and view them in DEdit:

1. [Install the Model Exporter ](#StepOne)
2. [Export your model ](#StepTwo)
3. [Convert the .LTA File to an .LTB File ](#StepThree)
4. [Copying Your Resource Files to the Project Folder ](#StepFour)
5. [Convert your textures in DEdit ](#StepFive)
6. [Place the Model in the World Using DEdit ](#StepSix)
7. [Process and run the world through DEdit ](#StepSeven)

---

## Step One—Install the Model Exporter

Jupiter includes a Model Exporter plug-in for both the Max and Maya modeling applications. You must install the plug-in for one of the applications using the directions below:

### To Install for use with MAX

If you use MAX3, then from the \Development\Tools\Plugins\ folder, copy the MaxModelExport30.dle file to the plug-in path used by MAX.

If you use MAX4, then from the \Development\Tools\Plugins\ folder, copy the MaxModelExport40.dle file to the plug-in path used by MAX.

### To Install for use with Maya

From the \Development\Tools\Plugins\ folder, copy the MayaModelExport25.mll (or MayaModelExport30.mll if you are using Maya 3) file to the plug-in path used by Maya.

Also from the \Development\Tools\Plugins\ folder, copy the LithTechModelExportOptions.mel file to the scripts path used by Maya, typically \scripts\others .

## Step Two—Export Your Model

Using your modeling application, you must export your model to the format that ModelEdit can utilize. Follow the instructions below for the modeling program you use.

### To Export from MAX

1. In MAX, open your model file.
2. In the File menu, select Export.
3. Enter a filename and select LithTech Model (*.LTA, *.LTC).
4. Click Save.
5. In theLithTech Model Exporterdialog, name the animation and select the UseMAX Texture Information. In the Base Texture Path directory, browse to the folder where you have stored your texture files in the Targa (.TGA) format. Click Use Path and then click OK.

### To Export from Maya

1. In Maya, open your model file.
2. In the Window menu, select **Setting>Preferences **and click the Plug-in Manager.
3. In the check box associated with LithTech Model Exporter, select Load.
4. In the File menu, select Export All.
5. Click the Options button and selectFileType as .LTA, name the animation, and click OK.

## Step Three—Convert the .LTA File to an .LTB File

In ModelEdit, you must convert the exported .LTA file into a compiled .LTB file DEdit can understand.

#### To convert the LTA file

1. Open ModelEdit.
2. In the File menu, select Open and find the .LTA file you created in the previous step and then click OK.
3. In the File menu, select Compile for Platform and selectPC D3D.
4. In the Save As dialog, provide a filename. In the Save As Type field, select .LTB Files, and then click Save. When the compile is complete, click ENTERto continue.
5. Close ModelEdit.

## Step Four—Copying Your Resource Files to the Project Folder

All resource files for your model must reside in a folder in the project area. This project area is \Development\TO2\Game\ .

#### To copy your resource files

- Copy the .LTA and .LTB files you created to the \Chars\ folder.
- Copy the .TGA texture files for your model into the \Chars\ folder.

## Step Five—Convert Your Textures in DEdit

#### To convert your textures

1. Open DEdit.
2. In the File menu, select Open Project. Navigate to the TO2 project located at \Development\TO2\Game\to2.dep .
3. In the Project Window, open the Textures tab. In the upper Textures tab area, open the Chars folder.
4. In the middle of the Textures tab area, right-click and select Import TGA Files. Navigate to your .TGA file(s) in the Chars folder and click OK.
5. In the Project Window **, **open the Worlds tab, select the World\Retail\SinglePlayer\ folder and open the world called c06s02 by double-clicking it.

## Step Six—Place the Model in the World Using DEdit

In DEdit, you must place the model in the world. The TO2 development build includes several useful objects, including Prop, which allows the placement of a model.

#### To place the model in the World

1. In DEdit, press and hold the X key as you position the green marker where you want the object to appear. Release the X key.
2. Right-click and choose Add then Object.
3. In the Select Object Type dialog, select Prop, then click OK.
4. In the Project Window, open the Properties tab. You must specify the Filename property. The Filename property is the name of the .LTB file for the model you want to see in the sample. Click on the B (for browse) button next to the Filename field. Browse and find your .LTB model file in \Development\TO2\Game\Chars .
5. In the Properties tab, you must also specify the Skin property. The Skin property is the filename for the model’s texture. Click on the B (for browse) button next to the Skin field. Browse and find your .DTX texture file in \Development\TO2\Game\Chars .

## Step Seven—Process and Run your World Through DEdit

To process and run your world you must pefrom the following three steps. The proceeding text describes each of the following three steps in greater detail.

1. Set up DEdit’s Run/Process Tab
2. Process the world
3. Run the world

### Step One—Set up DEdit’s Run/Process Tab

DEdit can only store the run information for one project at a time. It takes a few steps to configure, so it’s best to use it only for your main project.

  1. Click Options in the Edit menu.
  2. In the DEdit Options dialog, click the Run tab.
  3. In the Executable box, enter the complete path to the version of LITHTECH.EXE that your game uses. Use the Browse button to find it. If you have a debug and a release version, you should specify the one you use most frequently here.
  4. In the Working directory box, enter the location from which your game is run. Usually, your executable, project directory and config files will all be in one place, and that place is the location of your working directory. If your project’s executables and project directory are stored in different locations, consult your programmers for more information on what the correct working directory is for your project.
  5. In the Program Arguments box, enter all the needed parameters to run your game. This includes loading the engine .REZ file, the game .REZ file, any config files in addition to AUTOEXEC.CFG, and any supplemental commands specific to your game. DEdit will, if asked, pass a %WorldName% parameter that tells the game to load the current level when it’s started from DEdit, which is very handy.
  6. Click OK.

### Step Two—Process the World

  1. In DEdit, in the World menu, select Process or click the toolbar button that looks like a brick wall.
  2. In the Processing Options dialog, set the Project Directory field. Click the **… **button to browse and navigate to the file \Development\TO2\Game\to2.dep . Select the file to2.dep and click Open.
  3. Click OK to begin processing the level.
  4. When the Processor is finished, it will show an OK button. Click OK to exit.

### Step Three—Run Your World

  - In DEdit, in the World menu, select Run (CTRL+ALT+R), or click the toolbar button that looks like a rocket.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ToolOvrv\ModView.md)2006, All Rights Reserved.
