| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Client-side Objects

LithTech implements simulation objects in the server shell. Simulation objects do not exist in the client shell because the BaseClass class is not supported there. However, you can emulate simulation objects by creating client-side objects in your game code in the client shell.

Client-side objects are not complete simulation objects because they do not include the game object portion of the simulation object, since the **BaseClass **class is not supported on the client. This means that client-side objects require some extra work to be done in your game code.

You should only create client-side objects for items that exist solely on the client. Such things as menus, HUD displays, and targeting crosshairs are good candidates for client-side objects.

This section contains the following topics related to the creation of client-side objects:

- [Defining Client-side Objects ](#DefiningClientSideObjects)
- [Creating Client-side Objects ](#CreatingClientSideObjects)
- [Using Client-Side Objects ](#UsingClientSideObjects)

---

## Defining Client-side Objects

Client-side objects are not derived from the **BaseClass **class. Your game code must define a client-side object from scratch.

Client-side objects do not have access to the **EngineMessageFn **or **ObjectMessageFn **notification functions. Aggregate functionality is also not available to client-side objects. Your implementation of a client-side object may or may not replicate some or all of this functionality based on the requirements of the client-side object.

Client-side objects do benefit from client-side engine objects. Universal properties are available to them.

[Top ](#top)

---

## Creating Client-side Objects

You can create a client-side object using the **ILTClient::CreateObject **function, defined in iltclient.h.

>

HLOCALOBJ (*CreateObject)(

ObjectCreateStruct *pStruct);

The **pStruct **parameter is an **ObjectCreateStruct **that is used to instantiate a client-side engine object.

The function returns a handle to the newly instantiated client-side engine object. You can use this handle to access the engine object’s member variables.

Since **BaseClass **is not available on the client, there are no object-centric notification functions, such as **EngineMessageFn **or **ObjectMessageFn **. You can manually replicate this functionality using the **IClientShell::Update **notification function with appropriate object cases within it.

[Top ](#top)

---

## Using Client-Side Objects

You can access and modify the client-side engine object’s member variables using the **ILTClient::GetObject* **and **SetObject* **functions.

>

ILTClient::GetObjectFlags

ILTClient::GetObjectPos

ILTClient::GetObjectRotation

ILTClient::SetObjectFlags

ILTClient::SetObjectPos

ILTClient::SetObjectPosAndRotation

ILTClient::SetObjectRotation

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SimObj\Client.md)2006, All Rights Reserved.
