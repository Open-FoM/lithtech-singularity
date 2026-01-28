| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Model and Animation Exporters

The engine supports the basic skinning operators available in the supported modelers. However, all modeling packages have many more features than the game engine does. Exporting new animated models into an existing .LTA file lets you create a file containing many animations. To do this requires that the hierarchy of the model being exported is identical to that of the existing file. You can import animations from other models with matching skeletons. Each animation must be exported as an .LTA file. In ModelEdit you select multiple .LTA files when you import.

All exporters work similarly, but because of the feature differences of each package, each exporter exerts different restrictions on modelers. The modeler is going to have to become familiar with the limitations of the packages in relation to the LithTech model format. When first starting try making simple models, test them in ModelEdit and a simple game level to see how it all fits. The following applications and utilities are capable of exporting animated models:

- [3D Studio Max ](#3DStudioMax)
- [Maya ](#Maya)

  - [Exporting Models from Maya ](#ExporterDialog)
  - [Maya Exporter MEL Commands ](#ExporterMELCommand)

---

## 3D Studio Max

Models can be skeletally animated using Biped or MAX native bones. In MAX 3. *x *, the Physique modifier must be used as the deforming modifier, but MAX 4. *x *also supports the native Skin modifier. It is a good idea to try a test model when trying new animation techniques to make sure the exporter supports those techniques.

Models animated with the Physique modifier must be linearly deformed, which means that you cannot use the muscling features of Physique for any meshes that will be skeletally animated by LithTech. Another important setting is the **Vertex - Link Assignment **option. This option must be set to **Rigid **. Multiple links can still affect the vertex position, but this will ensure that the bones remain rigid, rather than using the default spline-based bones. If **Deformable **is used instead, there will be minor differences between the mesh seen in MAX and the mesh seen in ModelEdit. This option is usually set when a mesh is first bound to a node hierarchy. It may also be modified in the Envelope sub-object mode within the **Active Blending **section of the rollout.

Models animated with the Skin modifier cannot use any of the Deformer Gizmos. This includes the Joint Angle deformer, Bulge Angle deformer, and the Morph Angle deformer.

The MAX model exporter supports specifying extra information to be exported by using the **User Defined Properties **tab of the **Object Properties **dialog. Open this dialog by right clicking on an object and selecting **Properties **. These properties can be used to force nodes to export, create sockets, and assign Render Styles and texture indices. Some of these actions can also be done with prepended strings on object names. Here is an sample list of all the properties (although no real model would have all of these properties set at once):

- Texture0=5
- Texture1=2
- Texture2=3
- Texture3=9
- RenderStyle=2
- ForcedNode=true
- Socket=true
- VertexAnimated=true

The **TextureN **properties specify the texture indices for the piece (as seen in the Piece Info dialog in ModelEdit.)

The **RenderStyle **property specifies the Render Style index for the piece (as seen in the Piece Info dialog in ModelEdit.)

By default, the exporter will only export nodes that are bound to a physique or skin modifier. All parents of bound nodes will also be exported in order to ensure proper animation. If there is a node that isn't used by the deformer modifier but you would still like it (and its parents) to be exported, specify the **ForcedNode=true **user property. The same result can be achieved by having the object name begin with **n_ **.

An object can be turned into a socket by specifying the **Socket=true **user property, or by having the object name begin with **s_ **. The specified object should not also be a node used for skeletal animation. The best technique is probably to create a Dummy object and make a bone its parent. The socket object should not be animated other than movement caused by parent object animation.

An object can be specified as being a vertex animated piece by using the **VertexAnimated=true **user property, or by having the object name begin with **d_ **.

Textures can optionally be assigned to objects in MAX and show up immediately in ModelEdit. To do this, assign a Standard material to the object and place a Bitmap texture in the diffuse slot of the material. At least one texture index for the object must be set up using the user properties as described above. The texture base path should be entered the same way as described in the [3D Studio MAX Brush Exporter ](../MaxExp/MaxExp.md)section of this manual, but a brief summary follows:

If you have this texture assigned in MAX:

>

\%install%\Max Scenes\ProjectModels\textures\wood\pine1.tga

And this texture in your LithTech resources:

>

\%install%\Project\d3drez\textures\wood\pine1.dtx

You would enter \%install%\Max Scenes\ProjectModels as your base texture path so that the project relative filename textures\wood\pin1.dtx would be stored in the model .LTA file.

---

## Maya

The Maya exporter supports polygon models that use the smooth skinning operation on Maya’s native joints. Do not use FFDs or any other deforming method unless you plan to export the piece as vertex animated. Bone scaling (stretching) is lost in the exporter. It is usually best to triangulate your models before exporting, but the exporter will also do this for you if you forget. Everything that is to be exported must have a smooth skinning operation done on it. Remember to load the plug-in from the Window\General Editors\Plug-in Manager dialog.

The Maya model exporter supports specifying extra information to be exported by adding custom attributes to nodes. These custom attributes are added within the Attribute Editor in Maya. These attributes can be used to force nodes to export, create sockets, and assign Render Styles and texture indices. Some of these actions can also be done with prepended strings on object names. Here is a list of all the custom attributes and their types (although no real model would have all of these set at once):

- Texture0 (integer)
- Texture1 (integer)
- Texture2 (integer)
- Texture3 (integer)
- RenderStyle (integer)
- ForcedNode (boolean)
- Socket (boolean)

The **TextureN **attributes specify the texture indices for the piece (as seen in the **Piece Info **dialog in ModelEdit.)

The **RenderStyle **attribute specifies the Render Style index for the piece (as seen in the **Piece Info **dialog in ModelEdit.)

By default, the exporter only exports nodes that are bound to a skin cluster. All parents of bound nodes are also exported in order to ensure proper animation. If there is a node that isn't used by the skin cluster but you would still like it (and its parents) to be exported, create the **ForcedNode **user attribute and set it to TRUE. The same result can be achieved by having the object name begin with **n_ **.

An object can be turned into a socket by specifying the Socket attribute and setting it to TRUE, or by having the object name begin with **s_ **. The specified object should not also be a node used for skeletal animation. The best technique is probably to create a Locator and make a bone its parent. The socket object should not be animated other than movement caused by parent object animation.

An object can be specified as being a vertex animated piece by having the object name begin with **d_ **.

Textures can optionally be assigned to objects in Maya and show up immediately in ModelEdit. To do this, assign a file texture in the color slot of a material and assign it to the piece. At least one texture index for the object must be set up using the custom attributes as described above. The texture base path should be entered the same way as described in the [3D Studio MAX Brush Exporter ](../MaxExp/MaxExp.md)section of this manual, but a brief summary follows:

If you have this texture assigned in Maya:

>

\%install%\Maya Scenes\ProjectModels\textures\wood\pine1.tga

And this texture in your LithTech resources:

>

\%install%\Project\d3drez\textures\wood\pine1.dtx

You would enter C:\Maya Scenes\ProjectModels as your base texture path so that the project relative filename textures\wood\pin1.dtx is stored in the model .LTA file.

### Exporting Models from Maya

The easiest way to use the exporter is to use the File/Export All... and File/Export Selection... menu options in Maya. These will launch a dialog with all the options supported by the exporter except for animation appending.

Optionally, the lithtechModelExport MEL command can also export the model. This supports all the options the UI front-end does, as well as the ability to append animations to existing model files. If you wish to append animations to a model from within Maya, you must use the MEL exporter command directly.

The following list displays the selections from the exporter dialog box followed by the MEL syntax to use if you are using an MEL command.

**Animation Name **—The name of the animation as it shows up in ModelEdit.

>

lithtechModelExport animation name

**Scale Modifier **—The exported model is scaled by the amount entered in this edit box. Defaults to 1.0.

>

lithtechModelExport -scale <float value>

**Ignore Bind Pose **—This should only be checked if you encounter problems with unmatched input and output geometry for a skin cluster. If this happens, the first animation to be exported MUST have the object in its bind pose at the first frame. If not, the geometry will NOT animate correctly within ModelEdit or the engine. When checked, the exporter considers the first frame of the animation to be the bind pose. By default this is not checked, and the exporter attempts to automatically find the bind pose information.

>

lithtechModelExport -ignoreBindPose

**Use Playback Range **—When on, the exporter only exports keyframes within the currently selected playback range. Otherwise the exporter tries to find the full range of keyframes and export them all. Defaults to on.

>

lithtechModelExport -usePlaybackRange

**Use Maya Texture Information **—When on, the exporter looks for assigned textures and adds this information to the file for use within ModelEdit. Note that this requires at least one 'TextureN' custom attribute on pieces that you wish to export texture information for. Defaults to off.

>

lithtechModelExport -useTexInfo

**Base Texture Path **—This path is stripped off the front of the full path of all textures that are exported. For example, if your LithTech world has a texture \%install%\project\d3drez\textures\wood\pine1.dtx and you have an equivalent Maya texture at \%install%\Maya Scenes\textures\wood\pine1.tga , you would enter \%install%\Maya Scenes as the base texture path. This will result in textures\wood\pine1.dtx being stored in the .LTA file.

>

lithtechModelExport -useTexInfo <base path>

### Maya Exporter MEL Commands

As noted in the previous topic, the Maya Embedded Language (MEL) command for exporting models to an .LTA file is **lithtechModelExport **. You can use this command in conjunction with the following dash delimited options to export models usable with DEdit.

>
```
lithtechModelExport [-all]
```

```
            [-usePlaybackRange]
```

```
            [-ignoreBindPose]
```

```
            [-append]
```

```
            [-scale <float value>]
```

```
            [-useTexInfo <base path>]
```

```
            animation name
```

```
            filename
```

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ToolOvrv\Exportr.md)2006, All Rights Reserved.
