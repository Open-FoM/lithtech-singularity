| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Creating Simulation Objects

Simulation objects are created in two ways, manually in your game code and automatically by LithTech. You can manually create objects in your game code using **LTServer::CreateObject **. When the LithTech engine loads a world, it automatically creates simulation objects placed by level designers using DEdit.

This section contains the following two methods for creating simulation objects:

- [Manual Creation ](#ManualCreation)
- [Automatic Creation ](#AutomaticCreation)

---

## Manual Creation

You can manually create a simulation object in your game code using the **ILTServer::CreateObject **function (defined in iltserver.h). Before calling this function you must define the simulation object class using the **BEGIN_CLASS/END_CLASS **macros, as explained in the [Publishing Objects to DEdit ](PubObj.md)section.

>

LPBASECLASS (*CreateObject)(

HCLASS hClass,

ObjectCreateStruct *pStruct

);

The **hClass **parameter is a handle to the simulation object definition as set with the **BEGIN_CLASS/END_CLASS **macros. You can retrieve the **HCLASS **of a simulation object using the **ILTServer::GetClass **function.

The **pStruct **parameter is an **ObjectCreateStruct **that you have instantiated and populated with the desired values for the engine object. After simulation object creation, you can access and modify the engine object’s member variables using the **ILTServer::GetObject* **and **SetObject* **functions.

The function returns a **BaseClass **pointer to the newly instantiated game object.

---

## Automatic Creation

The simulation objects placed in a world by a level designer are automatically created by the engine when a world is loaded. Only those simulation objects placed in the world using DEdit are automatically created. Most simulation objects in a level are automatically created.

At a high level, a simulation object is automatically created as follows:

1. Using DEdit, a level designer adds a game object to a world.
2. DEdit stores the simulation object information in an .LTA file.
3. Processor.exe converts the .LTA file to a .DAT file. There is a .DAT file for each world.
4. When a world is loaded, LithTech.exe reads the .DAT file and instantiates the required game objects and engine objects and associates them with one another.

At a programmatic level, automatic creation of a simulation object proceeds as follows:

1. LithTech instantiates a new game object.
2. An **ObjectCreateStruct **is instantiated. LithTech assigns values to the structure’s member variables based on the simulation object’s universal properties set in DEdit by the level designer.

>
**Important: **Only the universal properties are automatically assigned by LithTech. Custom properties are not a part of the **ObjectCreateStruct **. If your simulation object includes any custom properties, your **EngineMessageFn **game code must read them manually. For more information about custom properties, see [Custom Properties ](DefSimOb.md#CustomProperties)and [Adding Custom Properties ](PubObj.md#AddingCustomProperties).

3. LithTech sends a **MID_PRECREATE **notification to the game object’s **EngineMessageFn **function. This notification includes a pointer to the **ObjectCreateStruct **. Your game code can use this pointer to read and modify the structure, if desired. You can use the **ILTServer::GetProp* **functions to acquire any custom property values and apply them to the game object.
4. LithTech instantiates an engine object.
5. LithTech applies the universal properties from the **ObjectCreateStruct **to the engine object.
6. LithTech sends a **MID_INITIALUPDATE **notification to game object’s **EngineMessageFn **. The game object’s **m_hObject **parameter is set to point to the engine object (thereby associating the engine object with the game object.

>
**Note: **The third parameter of the **MID_INITIALUPDATE **notification to the **EngineMessageFn **is the **INITIALUPDATE_WORLDFILE **flag. This indicates that the engine object and game object instance are created based on a .DAT file.)

7. The game object can now access and modify the associated engine object using the **m_hObject **handle.

>
**Note: **The **ILTServer::SetNextUpdate **function can be used to specify that the game object should receive a **MID_UPDATE **notification to the **EngineMessageFn **after a period of time. **SetNextUpdate **can be called at the end of each **MID_UPDATE **notification to receive updates regularly.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SimObj\Create.md)2006, All Rights Reserved.
