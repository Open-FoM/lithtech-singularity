| ### Content Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Glossary

This section contains of list of terms commony used in developing LithTech games, and video games in general. These terms may be specific to a product or tool, or they may be common terms used in video game development, graphic design, and level design. Use this section to define terms you are not familiar with. If this section does not contain the term you are looking for, then try one of the following definition sites:

- [TechWeb Encyclopedia ](http://www.techweb.com/encyclopedia/)
- [Acronym Finder ](http://www.acronymfinder.com/)

This section is divided into the following terminology sub-sections:

- [Game Development Terms ](#GameDevelopmentTerms)
- [Acronyms ](#Acronyms)
- [File Extensions ](#FileExtensions)

---

## Game Development Terms

| **Brush ** | In DEdit, the brush is the basic unit of world geometry. A brush is made up of planes that define its faces, lines that define its edges, and vertices that define its corners. You can manipulate either an entire brush at once or any of the vertices, lines, or planes in the brush. |
| --- | --- |
| **Basis Vector ** | Only used for per-pixel lighting (bump mapping). Specifies the tangent space (or texture space) at each vertex allowing you to put lights in the texture space. |
| **Chroma Key ** | Term for the alpha mask "layer" applied to textures (.DTX files). Chroma keys are specific colors that are not rendered and appear transparent. Textures with alpha layers get used to create a variety of effects, such as a fencing or windows with bars. |
| **Decal ** | Decals are textures you can apply over the top of world geometry using the Decal object. Decals operate like spraypaint, covering the geometry of the specified brush(s) in your level. |
| **Flat lighting ** | In Flat lighting, Processor ray-casts to the vertices of the face currently being lit and then samples the light value of each vertex. It then averages those values and creates a single uniform color across the whole brush. This is fast to process and fast to render, but often ugly. |
| **Game Objects ** | A resource type. Game objects are similar to .DEP simulation objects, but are constructed for a specific game. |
| **Gouraud shading ** | Gouraud shading does the same ray-cast to the vertices of the brush as is done for Flat shading, but instead of averaging the colors, it blends their values together gradually, so that you get varying shades that spready out from the vertices to the center of the polygon, creating a smooth light effect. It can be very useful for smoothing sorfaces that would normally look faceted when lightmapped. It is the most common lighting mode for brushes in the Jupiter engine and although it's only average speed when processed, it is as fast as flat shading when rendered, which is to say very fast. |
| **Interpolation ** | Refers to the computer estimating or filling in data between two points. For instance, in an animation, interpolation occurs between two images to calculate and display the appropriate states of the moving object between the start state and the end state. |
| **Lambert lighting ** | In the Lambertian lighting model (which is closer to the real world than traditional game lighting models) the amount of light reflected by a surface depends on the ray of light's angle of incidence with the surface it's hitting. The closer to dead on (90 degrees) the angle is, the more light will be reflected from the surface. The closer the ray is to being parallel with the surface, the less light the surface will reflect. Thus, light fades more rapidly from a point source near a large flat surface, and it falls off more rapidly on the rounded sides of a column as well. Usually this is applied to Gouraud-shaded polygons. However, you can also apply it to your lightmaps as well using the Lambertian Lightmaps checkbox. This is a new feature introduced in Jupiter. |
| **Level ** | See [World ](#World). |
| **Lightmaps ** | Lightmaps are a texture layer of relatively low resolution compared to the parent texture that's alpha-blended onto the world polygons to create lighting. It can be highly detailed because it doesn't rely on vertices for color storage. However, since it uses bitmapped textures, lightmapping requires an additional rendering pass per polygon and also uses much more memory than the previous two. It is expensive to render and also to process, but its detailed shadows mean it's still one of the preferred lighting modes. |
| **Mode ** | DEdit has several different editing modes: Object, Brush, and Geometry. Each mode allows you to change certain elements of your world (that is, just brushes, objects, or vertices) without accidentally affecting others. They’re designed to simplify interacting with your world while editing, so it’s important to choose the right mode for your task. |
| **Models ** | Models in Jupiter are in a custom format, the .LTA file. This contains the mesh/geometry for the model and other information used by the game engine. You create these files in an external editor such as Maya or 3D Studio Max, then save them in the .LTA file format using a plug-in. Use ModelEdit to modify files in the .LTA format once they’ve been created. |
| **Objects ** | In DEdit, anything a player interacts with that moves, lights up, shoots, growls, or goes into the player’s inventory is an object. DEdit allows you to place any kind of object that your game code or the engine code can create. |
| **Prefab ** | In DEdit, aprefab is like a primitive, but more complex. If you build a street lamp complete with textures and a light source that gives off just the right shade of light, you can select the lamp and its associated source (or any group of objects and brushes inside DEdit) and save it as a prefab. Thereafter, you can copy and paste that streetlamp into your world without having to rebuild the whole object. |
| **Primitive ** | In DEdit, a prefab is a simple shape that you can add to the world as the foundation of a more complex shape. Typical primitives are things such as cubes, pyramids, and cylinders. |
| **Processing ** | In DEdit, when you process your world, a second version of the world is created with some information removed (such as parts that only you need to know about), and other information added (such as parts only Jupiter needs). Processing is a necessary step in getting your world up and running in the game, and is often the first place where you will discover problems with your level. |
| **Project ** | In DEdit, a project represents the sum of all the resources in the game: all the code, all the textures, all the sounds, all the worlds, and so on. Unlike other world geometry editors, DEdit first opens the project, rather than a world file. |
| **Render Styles ** | LithTech Render Styles provide a consistent mechanism to customize how models are rendered. The customization encompasses parts of both lighting and rasterization from the basic render states and texture passes to dynamic UV generation and lighting material properties. Programmers can use Render Styles Editor to create unique render styles that enable visually stunning models. Artists can use the Render Styles Viewer to preview any render styles that you apply to your model. Jupiter includes numerous render styles located in the \Development\Tools\RenderStyles\ folder. |
| **Shadow meshing ** | Shadow meshing is an abandoned method for creating extra polygons for use in Gouraud shading on the world on the fly. If you have a specific use for a feature that will automatically split polygons according to their light and shadows, ask us for more details. However, this code is officially not being developed, so we would have to work with you to make sure it fits your needs and that you can maintain any changes you need to make to it. |
| **Simulation Objects ** | Objects that programmers construct using code. Some of this code exists in the engine and doesn’t appear in the project directories. However, many objects are created in code written for the specific game. These objects are called game objects (to distinguish them from the engine objects inside the engine code) and are stored in a file called OBJECT.LTO in the same directory as the project’s .DEP file. |
| **Sprites ** | Sprites (.SPR files) consist of a series of .DTX files linked together as an animation with a set frame rate. They’re commonly used for animations and special effects such as smoke, bullet holes, and liquid droplets. |
| **Super-sampling ** | Super-sampling forces Processor to take multiple passes at each lightmap pixel and incorporate a bit of the data from neighboring pixels. Using super-sampling can be expensive (especially if you have a sampling rate of 4 or more) but it results in much smoother-looking lightmaps. Supersampling can almost eliminate jagged stair-stepping in lightmaps. A value of 1-4 is common. |
| **Texture ** | In DEdit, textures are used like wallpaper, paint, or plaster to cover the raw plywood of your walls, floors, ceilings, doors and so forth. Without a texture, your brush will appear flat-shaded in the game, almost always with unpleasant results. Just as you wouldn’t want a house with raw plywood walls, you always want to apply textures to the brushes in your level. You can create .tga textures in a variety of different raster based pixel editors and import them into DEdit where they are converted to .dtx files. |
| **Texture Flags ** | Texture flags specify a texture definition contained in the surface.txt file. Use texture flags to determine the attributes for a specific texture. For more informaton, see [Assigning Attributes to Textures ](../../Dedit/TextrAln.md#AttachingAttributestoTextures). |
| **Texture Clamping ** | To disable bilinear filtering for a specific texture or face. |
| **Unit ** | The basis of measurement in DEdit and Jupiter. Units don’t equate directly to real-world values. Instead, the game designers can select a scale of game units to real-world measurements. As an example, in No One Lives Forever 2 the following values are used: railings are 48 units tall, a chest-high box would be 64 units, and a typical doorway would be 128 units high. |
| **Weight Sets ** | Used in weighted animation blending, weight sets allow you to create custom blends between two concurrent animations. |
| **World ** | When your players walk around in your game, it is a world that they’re standing in. Worlds divide your game up into sections where different parts of the game take place. In other games, these are sometimes referred to as maps, levels, scenes, or episodes. World-editing is the primary focus of DEdit and each world is stored in a separate file on the hard drive. |
| **World Geometry ** | A Jupiter world is composed of several different classes of geometry including: basic brushes, terrain brushes, WorldModels, and other types such as triggers, special effects, and sky. |
| **World Models ** | World Models are applied to objects that need some of the properties of models (translucency, mobility, destructibility) but need to be closely physically fitted to a place in the world, or need per-polygon collisions. They are used for doors, windows, elevators and moving machinery. |

[Top ](#top)

---

## Acronyms

| **BSP ** | Binary Space Partition—A BSP Tree is structure used to determine the visiblility of objects in the game and allow increased game speed by rendering only what is visible to the player. |
| --- | --- |
| **FFD ** | Free Form Deformation—A form of soft object animation where the vertices within a mesh are altered to create an animation. FFDs do not export well for use with LithTech. |
| **FOV ** | Field of View—A property in some lighting objects (such as DirLight) that creates an arc containing the light. Chaning the FOV property widens or narrows the width of the light displayed by the object. |
| **FPS ** | First Person Shooter—A game in which the player takes the first person perspective. This acronym is commonly applied to most games involving the player shooting through a viewing window that represents what their character sees. |
| **HUD ** | Heads Up Display—A display or interface that supplies information and provides controls. Typically HUDs are used to show character status, equipment, and targeting reticles. |
| **LOD ** | Level of Detail—Refers to the polygon count and how realistic objects look when they are rendered. The higher the polygon count, the higher the level of detail. A delicate balance exists between LOD and the speed allowed by system resources to process the game. |
| **MEL ** | Maya Embedded Language—The scripting language used by the Maya application. LithTech has developed plugins for Maya that you can access through the MEL interface. |
| **MMP ** | Massively Multi-Player—A term used to indicate games played by multiple players over an internet or local network connection. |
| **PVS ** | Potentially Visible Set— |
| **UVW ** | Not an acronym, but representative of the X/Y/Z coordinates for a texture. UVW mapping is done to determine how a texture gets projected on a polygon. |

[Top ](#top)

---

## File Extensions

| **.BUT ** | Designates an ASCII text files containing data used by different classes and objects. |
| --- | --- |
| **.CFG ** | Designates a configuration file. |
| **.CFV ** | Designates a list of stored color animations favorites used by the FxEd tool. |
| **.DAT ** | Designates a compiled World file created and built in DEdit from an LTA file. DAT files cannot be edited, only run. |
| **.DEP ** | Designates a DEdit project containing references to all of the objects and components of your world. |
| **.DTX ** | Designates a texture file created by importing a .PCX or .TGA file to DEdit and saving it as a texture. DTX files also contain flags used by the engine and the game. Textures can be any size up to 32 bits. |
| **.FCF ** | Designates an FxEd file. These files are automatically generated when you save the current effects project. |
| **.FXD ** | Designates the FxEd .DLL file containing all the game-side code for Jupiter's special effects. |
| **.FXF ** | Designates a listing of all the stored special effects created by designers using FxEd. |
| **.KFV ** | Designates the FxEd list of stored animation key favorites. |
| **.LTA ** | Prefabs, Worlds, and models all use the .LTA file extension. World .LTA files contain the uncompiled editable source code for a World. World LTA files are edited using DEdit and compiled into .dat files when you build them. Prefab .LTA files contain a collection of brushes and code objects. Prefab.LTA files are created and edited with DEdit. Model .LTA files are created in Maya or Max and exported using the appropriate plugin. Do not confuse World and Prefab .LTA files with model .LTA files, if you attempt to open a model .LTA file in DEdit, you may encounter an error. |
| **.LTB ** | Designates a LithTech binary file. These files contain packed models that have been optimized for display in the game. ModelEdit generates .LTB files. You cannot edit or modify these files, you can only replace them by recompiling. |
| **.LTC ** | Designates an .LTA file that has been compressed to save disk space. This format does not save space in the final game or increase performance. They are intended to enable designers to load worlds faster and reduce the amount of space taken up on their machines. .LTC files are typically used for world files. |
| **.LTO ** | Designates a file (commonly called the OBJECT.LTO file) containing game objects (similar to simulation objects but created for a specific game). |
| **.LYT ** | Desginates a log file used by DEdit to store information about the state of a project, when it was last opened, which levels were opened, any various other propreties. DEdit uses this file when it opens a world to set the view and settings of the world back to their state when the world was last saved. |
| **.MEL ** | Designates a file containing a Maya Embedded Language script. Files with this extension are associated exclusively with the Maya application. For more information about MEL files, consult the Maya documentation. |
| **.MFV ** | Designates a list of stored motion type favorites in the FxEd tool. |
| **.PCX ** | A common raster-based file format for graphic images. |
| **.PS ** | Designates a postscript file. |
| **.REZ ** | Designates a LithTech resource file. The lithrez.exe tool creates these files to contain all the files from a project. .REZ files record the directory structure and heirarchy of your project, and they are commonly used to contain files for the retail version of a game. This prevents users from tampering with the retail versions, and also makes installation and file tracking easier. It is not possible to easily edit files inside a .REZ file, so they are typically made as a part of the build and testing process near the end of a project. |
| **.RPD ** | Designates a re-processing data file. .RPD files get used by the processor to store information about a level it has processed. When you re-process a level, the processor loads the .RPD file to skip the re-processing of data that has already been processed (although it may not always be able to skip the processing depending on the changes that you have made to your level). .RPD files are created each time you process a level. |
| **.SFV ** | Designates a list of stored scale and size animation favorites for use by the FxEd tool. |
| **.SPR ** | Designates a sprite file containing a series of .DTX files linked together with a set frame rate. |
| **.TFG ** | Designates a Texture Group Effect. .TFG files get generated by DEdit, and store the name of the texture script and texture parameters. .TFG files allow you to apply the same texture effect settings to multiple brushes in your levels. |
| **.TFS ** | Designates a Texture Effect Script file. .TGS files are compiled binary scripts containing the run-time infomration for a single texture or surface effect. Examples include panning, rotation, and scaling of a texture on a brush face. .TFS files are generated by the TSCompiler.exe tool. |
| **.TGA ** | Texture files. The .TGA graphic file format is used by many raster-based pixel editors such as Photoshop and PaintshopPro. You can import .TGA files into DEdit and use them as textures. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Appendix\Apnd-G\Glossary.md)2006, All Rights Reserved.
