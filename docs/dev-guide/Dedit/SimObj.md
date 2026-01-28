| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Adding Simulation Objects

Of course, there’s more to a map than just the brushes it contains. A map with just brushes is like a movie with no actors: It’s a still life. In order to have weapons, enemies, and even lights, you must add objects to the map that tell the game engine where to place these things. A map can’t even be started if you don’t designate a player start point in the map.

This section deals with adding objects like weapons, enemies, and lights, as well as other types of props, to your maps. Most of the objects mentioned above are referred to as simulation objects (or, more simply just as objects). A simulation object can be defined as an object that is defined not of brushes but by code (either in the game or the engine). Jupiter has certain classes of object that any game built on it can use.

Most games also define their own sets of simulation objects, some of which are brand new, and some of which are subclasses or children of the engine classes. A subclass incorporates most or all of the behaviors and attributes of the parent, but also adds some new behaviors of its own. In the case of most Jupiter games, for example, you can add a Door (“Make this brush move.”), you can add a Door-subclass RotatingDoor (“Make this brush move and turn on a point.”), or you can add a RotatingDoor-subclass DestructibleRotatingDoor (“Make this brush move, turn and explode when shot.”). Each of these subclasses takes the properties and behaviors of the object above them in the tree (their parent) and extends them in new directions. You can still use the root classes in cases where you don’t need the added properties of the child objects, and often will.

This tree system is easy to see when you look at the Add Object dialog inside a Jupiter game. For example, each of the AI classes derives from the root class, but each type of enemy, civilian and major character is its own subclass, many of which have their own subclasses for different armies or different versions of the character.

>
| **Note: ** | This section assumes you have already created the Big1.LTA file in the [Using Brushes ](Brushes.md)tutorial. |
| --- | --- |

#### To add a simulation object to your level

1. Switch back to your Big1.LTA file, or open it if you have closed it.
2. Begin by moving the green marker so that it’s centered in the archway you built earlier and about 64 units above the floor.
3. Right-click in a viewport and select Add Object from the Context menu. The Add Object dialog appears. This dialog is where you select and add objects to your level.
4. Select the **GameStartPoint **object from the list and click OK. Now, if you look closely at the center of the green marker, there’s a small square located there. If a simulation object doesn’t have a discrete volume that’s specified by the player, this square is how it will be represented in DEdit. Many objects such as models have a volume associated with them and are therefore represented by a box that’s the right size for them.

**GameStartPoint **is the first object you should add to your level, since without a start point your game doesn’t know where to start the player off in the level. The reason for raising the start point 64 units off of the floor is to ensure that the player has room to stand when he or she appears. If there’s not enough room for the player to appear, the start point is too low to the floor, or if the start point is inside a solid object, the player will just fall out of the bottom of the world.

[Top ](#top)

---

## Binding Simulation Objects to Brushes

Another way of creating a simulation object is by binding it with a brush or group of brushes.

#### To bind brushes

1. Select the brush or brushes you want to bind
2. Right-click and select Bind to Object from the Context menu.
3. The object list appears and you can then select the simulation object you want to bind to.

By binding brushes to a simulation object, you tell the simulation object to use the brushes to define its structure or its volume.

Two general classes of bound objects commonly appear in Jupiter: WorldModels and Volume Brushes.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Dedit\SimObj.md)2006, All Rights Reserved.
