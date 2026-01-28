| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Engine Objects and Game Objects

Simulation object is an abstract term. There is no simulation object class in LithTech. A simulation object is composed of an engine object and a game object. These two objects, in combination, form a simulation object. Each simulation object must have both an engine object and an associated game object. Game objects can be manually or automatically instantiated. Engine objects are automatically instantiated by LithTech.

## Engine Object

The engine object keeps track of a simulation object for LithTech. Engine objects define LithTech related details of simulation objects such as the object’s physical manifestation and networking details. Engine objects contain information about the position, rotation, mass, velocity, acceleration, color, bounding box, and model of a simulation object. Engine objects are defined and instantiated by the engine code. The engine code for the engine objects resides on the server and on the client.

## Game Object

The game object defines the customized logical behavior of a simulation object. The code implementing such logic is written by you. For example, while the engine object defines the position of a lever, your game object defines the results of a player pulling that lever. Game objects also include the logic defining how and when doors open and close, the path a guard takes on patrol, what causes the dragon to awaken, and so on. All game objects derive from the BaseClass class defined in the ltengineobjects.h file. The game code for game objects resides in your server shell implementation in Object.lto.

## Messaging

LithTech provides communication from LithTech to game objects using a variety of event notification message types (the **MID_* **defines) and the **BaseClass::EngineMessageFn **. Communications from game object to game object are transmitted by the **BaseClass::ObjectMessageFn **and implemented in your game code.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SimObj\EngObj.md)2006, All Rights Reserved.
