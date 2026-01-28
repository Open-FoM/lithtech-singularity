| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Maya Brush Exporter

This exporter plug-in for Maya allows creation of brushes and terrain within Maya. The plug-in consists of two parts. There is the UI front-end, which consists of a Maya file translator plug-in and the associated UI MEL script. Optionally, a user can directly call the MEL command lithtechWorldExport to manually export a scene.

This section contains the following topics related to exporting a brush created in Maya:

- [Installation ](#Installation)
- [Texture Setup ](#TextureSetup)
- [Creating Geometry ](#CreateGeometry)
- [Applying Textures to the Geometry ](#ApplyTextures)
- [Export Options ](#ExportingOptions)

---

## Installation

The \Development\tools\plugins\MayaWorldExport.mll plug-in file must be placed within a plug-in path used by Maya. The Development\tools\plugins\LithTechWorldExportOptions.mel exporter interface MEL script must be placed within a script path used by Maya. Finally, the plug-in must be loaded using the Maya Plug-in Managerdialog.

---

## Texture Setup

This procedure is the same as in the 3D Studio MAX world exporter. See the [3D Studio MAX Brush Exporter ](../MaxExp/MaxExp.md)section for more information.

[Top ](#top)

---

## Creating Geometry

The exporter only supports polygon geometry. NURBS and other procedural geometry will not export at all. If you want to model using other geometry types, be sure to convert them to polygonal meshes before exporting them. Maya scene hierarchy and grouping will be correctly represented in the exported world.

[Top ](#top)

---

## Applying Textures to the Geometry

Once you have the corresponding texture directory set up, you can begin texturing your terrain in Maya. Apply texture coordinates to the vertices using whatever tools you want.

Texture coordinates alone aren't enough information for the exporter to generate correct mappings in the LTA. You must also apply textures of the correct resolution to the faces of the mesh. This should be done by creating a material with a File texture assigned to the color channel. Place one of the converted textures into the file texture. Any faces without correct mapping information will generate simple default mappings.

[Top ](#top)

---

## Export Options

The easiest way to use the exporter is to use the **File>Export All **and **File>Export Selection **menu options in Maya. These launch a dialog with all the options supported by the exporter.

**Scale **
The exported scene will be scaled by the amount entered in this edit box. Defaults to 1.0.

**Export as Detail Brushes **
All brushes in the exported scene will have their DetailLevel property set or not, according to the value in this checkbox. Defaults to ON.

**Triangulate Brushes **
Forces triangulation of all faces on export. Best used as a debugging option, when you encounter concave faces. You can triangulate concave faces you encounter using Maya’s tools.

**Export Animated WorldModels **
Animated objects in MAX are saved to the .LTA file as animated worldmodels. KeyFramer and Key objects will be placed as needed in the .LTA file. Any object with set keys in MAX will become it’s own WorldModel, along with its children. If any of the children are also animated, they will become separate WorldModels, and so on. Time settings in MAX will be used for determining the time values for Key objects in the .LTA file.

**Base Texture Path **
This path is stripped off the front of the full path of all textures that are exported. For example, if your LithTech world has a texture d:\project\d3drez\textures\wood\pine1.dtx and you have an equivalent Maya texture at c:\Maya Scenes\textures\wood\pine1.tga , you would enter c:\Maya Scenes as the base texture path. This will result in textures\wood\pine1.dtx being stored in the .LTA file.

Exported brushes must follow the same rules for valid brushes as always. Concave faces are not valid and will cause the same processing issues as invalid brushes created in DEdit.

Texture support on brushes works the same as it does in DEdit. Only planar projections will work for brush faces with more than 3 vertices. Texture coordinate editing tools can still be used, but each face must have a planar mapping across the entire face in order to export correctly.

## Exporter MEL Command

Optionally, the MEL command lithtechWorldExport can be used to export the scene. This has the same options as the UI front-end.

lithtechWorldExport [-all]

[-asBrushes]

[-scale <int value>]

[-detailLevel <float value>]

[-useTexInfo <base path>]

filename

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: MayaExp\MayaExp.md)2006, All Rights Reserved.
