| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Types Of Geometry

The term “geometry” in Jupiter actually describes several very different classes of objects: Models, WorldModels and World Geometry. It’s useful to know and remember the difference between these three classes of geometry, since they are made very differently and can be used in very different ways.

There is overlap between the various classes. You can build a vehicle as either a Model or a WorldModel, depending on how flexibly you need it to move. You could even build a vehicle out of World Geometry if it was never going to move. Likewise, trees can be made as any of the three depending on what’s most convenient. For complexity, the Model is preferable. For interactive, solid world objects, the WorldModel is usually best. For solidity and visibility control, World Geometry is best.

LithTech supports the following classes of geometry objects:

- [Models ](#Models)
- [WorldModels ](#WorldModels)
- [World Geometry ](#WorldGeometry)

---

## Models

Models are, as the name implies, objects built in 3D Studio or another external modeling package. Their mesh is stored in an .LTA file that is exported from that package and loaded by Jupiter.

Models are usually used for objects in the world that have a high polygon count, require complex animation or would be very difficult to make inside DEdit. Players, weapons and props like dishes and chairs usually are made with a model. Doors, walls and other large, structural features are usually not.

Models can’t block visibility the way world geometry can, and they generally don’t integrate with the brushes in the world. Instead, they sit on top of brushes. This makes them very suitable for props like chairs and desks, since you can quickly place a large number of them with less work. It makes them unsuitable, however, for making a whole building or parts of terrain.

---

## WorldModels

WorldModels are a hybrid: they consist of an object with a brush or brushes bound to it, which provide its mesh. The brushes are built and textured inside of DEdit, which allows better integration with the rest of the level. Unlike regular brushes, though, WorldModels can be translucent, can move and can be destroyed and removed from the level.

WorldModels are often used for parts of a level like large mechanical props, vehicles, doors, glass and fences. They usually don’t block visibility but can be made to do so with customized game code to an extent. They’re not useful for character models, since they’re not easily deformable.

[Top ](#top)

---

## World Geometry

World Geometry is made of brushes. These brushes can be built and textured in DEdit or constructed in any supported 3D package. World Geometry makes up the vast bulk of almost all worlds in a DEdit game. It’s used for walls, ceilings, columns, ground, sky and many other solid, immovable objects in a world. World geometry is very cheap to render.

However, due to World Geometry’s limitations (it can’t move or be translucent), world geometry must be bound up in a WorldModel to make things like doors and models are more appropriate for complex objects like chandeliers or moving trees.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ToolOvrv\Geomty.md)2006, All Rights Reserved.
