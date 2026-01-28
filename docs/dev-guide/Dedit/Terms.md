| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Basic DEdit Terminology

The following terms are used when describing specific operations using the DEdit tool. You can find these and other additional gaming terminology in the [Appendix G: Glossary ](../Appendix/Apnd-G/Glossary.md).

| **Brush ** | The basic unit of world geometry. A brush is made up of planes that define its faces, lines that define its edges, and vertices that define its corners. You can manipulate either an entire brush at once or any of the vertices, lines, or planes in the brush. |
| --- | --- |
| **Mode ** | DEdit has several different modes that allow you to change certain elements of your world (that is, just brushes or just objects) without accidentally affecting others. They’re designed to simplify interacting with your world while editing, so it’s important to choose the right mode for your task. |
| **Objects ** | Anything a player interacts with that moves, lights up, shoots, growls, or goes into the player’s inventory is an object. DEdit allows you to place any kind of object that your game code or the engine code can create. |
| **Prefab ** | A prefab is like a primitive, but more complex. If you build a street lamp complete with textures and a light source that gives off just the right shade of light, you can select the lamp and its associated source (or any group of objects and brushes inside DEdit) and save it as a prefab. Thereafter, you can copy and paste that streetlamp into your world without having to rebuild the whole object. |
| **Primitive ** | A simple shape that you can add to the world as the foundation of a more complex shape. Typical primitives are things such as cubes, pyramids, and cylinders. |
| **Processing ** | When you process your world, DEdit creates a second version of the world with some information removed (parts only you need to know about), and other information added (parts only Jupiter needs). Processing is a necessary step you’ll take in getting your world up and running in the game, and is often the first place where you will discover problems with your level. |
| **Project ** | A project in DEdit represents the sum of all the resources in the game: all the code, all the textures, all the sounds, all the worlds, and so on. Unlike other world geometry editors, DEdit first opens the project, rather than a world file. |
| **Texture ** | In DEdit, textures are used like wallpaper, paint, or plaster to cover the raw plywood of your walls, floors, ceilings, doors and so forth. Without a texture, your brush will appear flat-shaded in the game, almost always with unpleasant results. Just as you wouldn’t want a house with raw plywood walls, you always want to apply textures to the brushes in your level. |
| **Unit ** | The basis of measurement in DEdit and Jupiter. Units don’t equate directly to real-world values. Instead, the game designers can select a scale of game units to real-world measurements. As an example, in No One Lives Forever 2 the following values are used: railings are 48 units tall, a chest-high box would be 64 units, and a typical doorway would be 128 units high. |
| **World ** | When your players walk around in your game, it is a world that they’re standing in. Worlds divide your game up into sections where different parts of the game take place. In other games, these are sometimes referred to as maps, levels, scenes, or episodes. World-editing is the primary focus of DEdit and each world is stored in a separate file on the hard drive. |
| **WorldGeometry ** | Parts of the world that act as walls, floors and sky are generally made of world geometry. Such geometry is solid, immovable, and generally never changes. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Dedit\Terms.md)2006, All Rights Reserved.
