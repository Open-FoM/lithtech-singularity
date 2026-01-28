| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Simulation Objects

LithTech utilizes a system of objects to simulate items in the game environment. These simulation objects can be such things as tables, chairs, monsters, robots, levers, lights, players, donuts, signs, and more. Simulation objects need not be tangible. Starting positions, key framers, paths, and cameras are also simulation objects.

There are two primary implementations of objects in LithTech.

1. Your game code manually creates simulation objects for your game.
2. You use DEdit to create a world. The level designer places objects that have been defined in your game code and published to DEdit in the world. When the game starts, LithTech automatically creates the simulation objects, instantiating both game objects and associated engine objects.

This section introduces the basics of LithTech’s use of simulation objects.

| #### Simulation Object Topics | #### Description |
| --- | --- |
| [Engine Objects and Game Objects ](EngObj.md) | A simulation object is composed of a game object and an associated engine object. |
| [Defining Simulation Objects ](DefSimOb.md) | Before you can include a simulation object in your game, you must derive a new game object class from the BaseClass class. |
| [Publishing Simulation Objects to DEdit ](PubObj.md) | All simulation objects must be published to DEdit using the **BEGIN_CLASS **and **END_CLASS **macros, making them accessible to your level designers. |
| [Creating Simulation Objects ](Create.md) | Simulation objects can be manually created in your game code and are also automatically created based on world files. Creation of a simulation object results in the instantiation of a game object and its associated engine object. |
| [Using Simulation Objects ](Using.md) | After a simulation object has been created in your game you will often need to modify it. Accomplish this by accessing and modifying the game object and engine object member variables. |
| [Client-Side Objects ](Client.md) | You can create client-side objects in your game code. Client-side objects are not simulation objects because they do not include the game object portion of the simulation object, since the BaseClass class is not supported on the client. This means that client-side objects require some extra work to be done in your game code. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SimObj\mSimObj.md)2006, All Rights Reserved.
