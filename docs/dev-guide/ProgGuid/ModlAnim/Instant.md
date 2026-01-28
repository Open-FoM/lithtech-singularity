| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Instantiating a Model

Models are represented in game code by simulation objects of type **OT_MODEL **. To instantiate a model, follow these steps:

1. Create an **ObjectCreateStruct **structure.
2. Set the **m_ObjectType **member variable to **OT_MODEL **.
3. Set the **m_Filename **to the path to the model’s .LTB file.
4. Set the other **ObjectCreateStruct **member variables as desired.
5. Call **ILTServer::CreateObject **, passing in the **ObjectCreateStruct **as a parameter. The function returns a handle to the resulting **BaseClass **-derived simulation object.

After you have created the model, you can manipulate it as any other simulation object. For more information, see the [Simulation Object ](../SimObj/mSimObj.md)section.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\ModlAnim\Instant.md)2006, All Rights Reserved.
