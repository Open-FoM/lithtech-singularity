| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Max .LTA World Importer

The Max World Importer plug-in complements the Max Terrain Exporter but is more versatile in that it will import any full .LTA level file into Max. Every brush and terrain object in the level will be imported into Max as an independent mesh, with textures aligned exactly as they were in DEdit.

This section contains the following topics related to importing worlds created in 3D Studio Max:

- [Installation ](#Installation)
- [Texture Setup ](#TextureSetup)
- [Importer Options ](#ImporterOptions)

---

## Installation

Place the plug-in file \%install%\tools\plugins\MaxWorldImport.dli within a plug-in path used by 3D Studio Max. After that, the plug-in is accessed through the File menu Import command within 3D Studio Max.

---

## Texture Setup

When an .LTA file is imported into Max, all textured surfaces show as green (until textured viewing is enabled in Max), and all untextured surfaces show as gray. The Max .LTA Importer assumes that for every .DTX texture file used in a level, there is an identically named texture stored in the same location in the .TGA format. If it can’t find the appropriate .TGA file for a surface, that surface will remain plain green even when rendering and displaying textures in the Viewport.

To batch convert all the .DTX textures in your project to .TGA files, copy dtxutil to your main resource directory and run it with the following command line switches:

dtxutil -r_dtx2tga [the texture directory] copy

| **Note: ** | The texture directory must be the actual directory name, and not the path of the directory. Currently, this converter will usually display various warnings and errors while it runs, but these can be safely ignored. |
| --- | --- |

[Top ](#top)

---

## Importer Options

**Merge Objects with the Current Scene **
If this option is checked, the importer retains the contents of the current scene being edited in Max, and inserts the imported .LTA level in it.

**Completely Replace the Current Scene **
If this option is checked, the importer erases the contents of the current scene being edited in Max, and replaces it with the imported .LTA level.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: MaxLTA\MaxLTA.md)2006, All Rights Reserved.
