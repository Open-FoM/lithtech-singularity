| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# 3D Studio Max Brush Exporter

This exporter plug-in for 3D Studio MAX provides a full-featured approach to generating terrain for use within LithTech. The user can model terrain using any MAX geometry modeling techniques. Textures can be applied and mapped using MAX tools. The end result is an .LTA file that retains all the Max texture mapping and model hierarchy information.

This section contains the following topics related to exporting brushes created in 3D Studio Max:

- [Installation ](#Intall)
- [Texture Setup ](#TextureSetup)
- [Creating Level Geometry ](#CreateLevelGeometry)
- [Applying Textures to Brushes ](#ApplyTexturesToBrushes)
- [Export Options ](#ExporterDialog)

---

## Installation

The \%install%\tools\plugins\MAXWorldExport.dle plug-in file must be placed within a plug-in path used by 3D Studio Max. After that, the plug-in is accessed through the **File>Export **and **File>Export Selected **menu commands within 3D Studio.

---

## Texture Setup

The textures you apply in 3D Studio must be the same resolution as the textures you use within your DEdit project. They must also exist in a directory tree that has exactly the same layout as your DEdit texture directory tree. The following example path shows what a sample DEdit resource directory might look like:

- c:\coolgame\resource

  - \worlds
  - \textures

    - \wood

      - \wood1.dtx (a 256x256 .dtx file)

    - \metal

      - \brushedalum.dtx (a 128x128 .dtx file)

The corresponding MAX texture tree for the previous DEdit resource directory would look as follows:

- d:\artresources\maxstuff

  - \textures

    - \wood

      - \wood1.tga (a 256x256 .tga file)

    - \metal

      - \brushedalum.jpg (a 128x128 .jpg file)

When exporting the terrain, you would enter d:\artresources\maxstuff as your base texture path. This way, textures in the .LTA file would be referred to as textures\wood\wood1.dtx and textures\metal\brushedalum.dtx and DEdit would correctly find the textures that have been applied within MAX.

| **Note: ** | you only need corresponding MAX textures for those that you will be applying to your terrain. |
| --- | --- |

[Top ](#top)

---

## Creating Level Geometry

Any geometry you can see within the MAX viewports should export well. The exporter will not handle render-specific geometry, such as displacement maps.

The hierarchy of objects is retained in the .LTA file. If you group several objects together, they will show up in DEdit as Container nodes for each of the objects, along with a parent container grouping them together. This is true of normal linked objects as well.

[Top ](#top)

---

## Applying Textures to Brushes

Once you have the corresponding texture directory set up, you can begin texturing your brushes within MAX. Apply texture coordinates using whatever tools you want. Only the first UV channel is used by the exporter.

Texture coordinates alone aren’t enough information for the exporter to generate correct mappings within the LTA. You must also apply textures of the correct resolution through the material editor. All materials assigned to the geometry must be “Standard” materials, or a “Multi/Sub-Object” materials with only Standard materials in the sub-material slots. Each of the materials that is referred to by a face in the scene must have a single “Bitmap” texture within its diffuse map slot. The Bitmap texture must use a texture in the texture tree that you set up following the above instructions.

If any of the exported faces doesn’t have the materials assigned correctly to it, you will be warned once the export has completed. The .LTA file will still contain all the correct geometry, but texture information will use defaults for faces without valid materials.

[Top ](#top)

---

## Export Options

**Use MAX Texture Information **
If this option is checked, the exporter will attempt to generate LithTech mapping coordinate information from the UV coordinates and materials assigned to the objects. If it isn’t checked, default mapping coordinates and textures will be applied to the LTA. Defaults to on.

**Pivot Point to Origin **
If this option is checked, all exported objects will be placed at the origin in the LTA file, ignoring their relative positioning within MAX. Defaults to off.

**Base Texture Path **
This path is stripped off the front of the full path of all textures that are exported. This allows the user to have their MAX texture files in a directory other than their LithTech resources directory, but still have DEdit read the textures from the resources directory. Clicking on the button to the right of the edit field will bring up the standard MAX directory selection dialog.

**Export Animated WorldModels **
Animated objects in MAX are saved to the .LTA file as animated worldmodels. KeyFramer and Key objects will be placed as needed in the .LTA file. Any object with set keys in MAX will become its own WorldModel, along with it’s children. If any of the children are also animated, they will become separate WorldModels, and so on. Time settings in MAX will be used for determining the time values for Key objects in the .LTA file.

The Exporter will not delete any vertices when merging triangles into polygons. For example, when exporting a cylinder created in MAX, an extra triangle will be added to the end-caps of the created brush. This is because MAX adds a vertex in the center of the end-caps. The designer should remove any vertices that are in the middle of polygons and not connected to any visible edges.

Texture support on brushes works the same as it does for the terrain export. Only planar projections will work for brush faces that have more than 3 vertices. Texture coordinate tools can still be used, but each face must have a uniform mapping across the entire face in order to export correctly.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: MaxExp\MaxExp.md)2006, All Rights Reserved.
