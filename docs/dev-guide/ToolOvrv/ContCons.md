| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Content Considerations

When creating content for a game powered by Jupiter, you will be working with the following classes of art and code assets:

**Texturing **—Textures are the bitmapped graphic image files used to add detail and color to models, world geometry, sprites, and surfaces. Textures are also used as a basis for special effects and as UI elements.
Jupiter uses its own form of texture, the .DTX file. Textures can be up to 32-bit images, although you can obviously choose to make lower-color textures. The .DTX files also contain flags for additional information used by the engine and the game. You can create your textures in Adobe Photoshop or PC Paint Shop Pro and save them as ZSoft PC Paintbrush (.PCX) or Truevision Targa (.TGA) files. You then convert them into Jupiter .DTX files by importing them into DEdit.

**Game Objects **—Simply put, anything a player interacts with that moves, lights up, shoots, growls, or goes into the player’s inventory is an object. In DEdit, you can place any kind of object that your game’s code or the engine’s code can create. Objects, also referred to as "Entities", must be created by your programmers. The Jupiter game code provides many powerful and useful game objects. These objects are called "game objects" (to distinguish them from the engine objects inside the engine code), and are stored in a file called Object.lto in the same directory as the project’s .DEP file.

**World Geometry **—A Jupiter world is composed of several different classes of geometry, each with a different purpose:

- **Basic brushes **, which are static, are used to build the non-moving parts of a world, such as the walls, floor, or ceiling of a building or dungeon. They also block the engine’s line of sight, which is important for performance optimization.
- **Terrain brushes **have far fewer restrictions on their shape than basic brushes. Although they don’t block visibility, terrain brushes can be concave shapes or even one-sided, such as an outdoor field with gentle rolling hills.
- **WorldModels **are used for objects that need some of the properties of models (translucency, mobility, destructibility) but need to be closely physically fitted to a place in the world, or need per-polygon collisions. They are used for doors, windows, elevators and moving machinery.
- **Other types **of world geometry exist that the engine uses for specialized purposes such as changing the visibility blocking in an area, specifying the space for triggers and special effects, or displaying the sky.

**Lighting **—Jupiter provides several lighting modes, including lightmapping on the world, Gouraud and flat shading on the world and on models, and Direct3D hardware lighting on models. Lights can have various shapes and emission characteristics. You can create dynamic lights that light up the world and the models.

**Visibility **—Engine visibility is a factor that determines how efficiently your world will render and run. Before you begin building your levels, you should understand how the engine decides what is visible and what is not.

**Modeling **—Use 3D Studio Max or Maya to create models and animations and then prepare them for use in Jupiter using the ModelEdit tool. The Max and Maya exporters included with Jupiter support many features of Max and Maya.

**Render Styles **—LithTech Render Styles provide a consistent mechanism to customize how models are rendered. The customization encompasses parts of both lighting and rasterization from the basic render states and texture passes to dynamic UV generation and lighting material properties. Programmers can use Render Styles Editor to create unique render styles that enable visually stunning models. Artists can use the Render Styles Viewer to preview any render styles that you apply to your model. Jupiter includes numerous render styles that can be found in the \Development\Tools\RenderStyles\ folder.

**Sound **—You can use the sound creation application of your choice to compose .WAV format sounds for your Jupiter game.

**DirectMusic **—You can use Microsoft’s DirectMusic Producer to compose interactive musical scores for your Jupiter game. Jupiter supports the bulk of DirectMusic features for DirectX 7 and 8.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ToolOvrv\ContCons.md)2006, All Rights Reserved.
