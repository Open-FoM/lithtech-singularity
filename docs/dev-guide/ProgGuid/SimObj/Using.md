| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Using Simulation Objects

Interaction with simulation objects after they have been created is accomplished in two ways. The game code can access the engine object member variables using the HOBJECT handle. The game code can access the game object member variables using the BaseClass derived game object.

## Using the Engine Object

There are numerous API functions in **ILTServer **, **ILTModel **, **ILTCommon **and other interfaces that modify engine object member variables using **HOBJECT **s.

The **ILTServer::GetObject* **and **SetObject* **functions access and modify engine object variables. For example, **ILTServer::SetObjectPos **changes the location of an object.

## Using the Game Object

The game object is defined by your code; as such, you define its use. It may expose public functions that are called by other objects, or it may interact through object-to-object messages.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SimObj\Using.md)2006, All Rights Reserved.
