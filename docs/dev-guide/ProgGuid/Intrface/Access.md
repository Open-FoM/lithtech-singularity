| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Accessing Interface Instances

The Interface Manager provides the **define_holder **and **define_holder_to_instance **macros as a method of accessing a specific interface instance.

>

#define define_holder(interface_name, ptr_var);

The macro takes the name of the interface class that is desired, and the name of a pointer variable that will be set to point at an instance of an implementation of that interface. The pointer variable should be global (either static or non-static). After the **define_holder **macro has been used, the pointer variable can be used to access functions and variables in the interface class.

---

## Accessing Engine Interfaces

The most common usage of the **define_holder **macro is to access engine interfaces and the many functions that they expose. The **define_holder **macro is used to access interfaces that have only a single implementation in the engine. Engine interfaces are instantiated by the engine. You cannot instantiate your own versions of engine interfaces. Instead, you must acquire a pointer to the interface instances created by the engine. After you have this pointer, you can use it to call the interface’s functions as desired.

#### To obtain a pointer to an engine interface

1. Include the appropriate header file for the interface (for example, iltclient.h for the **ILTClient **interface).
2. Declare a pointer variable of the appropriate interface type.
3. Use the **define_holder_to_instance **or **define_holder **macro to set the pointer variable to the interface instance.

#### Example Code

The following code snippet shows how to access the **ILTClient **interface instance.

>

#include "iltclient.h"

static ILTClient *g_pLTClient;

define_holder(ILTClient, g_pLTClient);

First, the header file containing the **ILTClient **definition is included. Then, a global static pointer is declared. Finally, the define_holder macro is used to set the **g_pLTClient **variable to the appropriate address of the **ILTClient **interface instance. The **g_pLTClient **pointer can now be used within that .CPP file to call **ILTClient **interface functions.

For engine interfaces that have both a client and a server implementation, you must use the **define_holder_to_instance **macro. This macro takes as the third parameter the instance name. For example, the engine instantiates the **ILTCommon **interface with the Client and Server instance names. To access the Client instance use the following type of code.

>

#include "ILTCommon.h"

static ILTCommon *g_pLTCommonClient;

define_holder_to_instance(ILTCommon, g_pLTCommonClient, Client);

Using **define_holder **is equivalent to using **define_holder_to_instance **with the third parameter as **Default **.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Intrface\Access.md)2006, All Rights Reserved.
