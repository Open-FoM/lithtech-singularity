| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# IServerShell Class

The **IServerShell **class is used by the Jupiter engine to notify the game code of events. You must derive a new class from **IServerShell **to provide communication between Jupiter and your game code.

Your **IServerShell **-derived class must include the **declare_interface **macro at the top of the class definition. This macro enables functions required for proper communication with Jupiter.

## IServerShell Functions

There are many event callback functions in IServerShell. You may implement each of these functions to provide any desired functionality whenever Jupiter sends a notification of a specific event. Some of these functions are more important than others. Depending on the functionality you want to offer in your game, some of these functions may be left un-implemented. However, implementation of certain functions is imperative to basic game functionality.

The event callback functions include:

- CacheFiles
- FileLoadNotify
- OnAddClient
- OnClientEnterWorld
- OnClientExitWorld
- OnCommandOff
- OnCommandOn
- OnMessage
- OnObjectMessage
- OnPlaybackFinish
- OnRemoveClient
- PostStartWorld
- PreStartWorld
- ProcessPacket
- ServerAppMessageFn
- Update

[Top ](#top)

---

## Instantiating Your IServerShell-derived Class

The IServerShell instance should be instantiated on the stack.

To instantiate your IServerShell-derived class, you must declare a global pointer and then use the Interface Manager’s define_interface macro.

>

define_interface(impl_class, IServerShell);

The impl_class parameter is the name of your derived class. The second parameter, interface_class, is the name of the class from which the impl_class parameter is derived (in this case, IServerShell).

[Top ](#top)

---

## OnClientEnterWorld

Jupiter calls the **IServerShell::OnClientEnterWorld **function when a client enters a world. You must create an object to represent the new client. Returning NULL will deny the client the ability to enter the world.

| **Note: ** | You must implement **OnClientEnterWorld **in the Object.lto file. |
| --- | --- |

The following example shows how to implement **OnClientEnterWorld **:

>

LPBASECLASS OnClientEnterWorld(HCLIENT hClient, void *pClientData, uint32 clientDataLen)

{

// Your game code goes here

}

Jupiter passes three parameters to **OnClientEnterWorld **:

**hClient **—A handle to an HCLIENT instance representing the client that has just entered the world.

**pClientData **—A pointer to an address containing the StartGameRequest data for the client that just entered the world.

**clientDataLen **—The length of the data identified by the pClientData pointer parameter.

The function must return a handle to a new object that is associated with the new client. This object will be the client’s representative on the server. If you do not return a valid BaseClass derived object from this function, the client will not be added to the world.

[Top ](#top)

---

## OnClientExitWorld

Jupiter calls the **OnClientExitWorld **function when a client exits a world.

| **Note: ** | You must implement **OnClientExitWorld **in the Object.lto file. |
| --- | --- |

The following example shows how to implement **OnClientExitWorld **:

>

void OnClientExitWorld(HCLIENT hClient)

{

//Your game code goes here

}

Jupiter provides your game code with a handle to the client that has just exited the world ( **hClient **). You should use this handle to delete the associated client object from your game.

[Top ](#top)

---

## Update

The Jupiter engine constantly calls the **IServerShell::Update **function.

Jupiter calls the **Update **function as often as possible. The frequency with which the function is called is dependant upon the hardware limitations of the computer and the number of operations the game is currently processing.

>
**Note: **The Update function should be implemented in the Object.lto.

[Top ](#top)

---

## void Update()

Jupiter does not pass any information into the Update function, nor does it expect any return value. Jupiter merely calls the function to give you the opportunity to conduct game operations as often as possible.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SrvShell\ISShell.md)2006, All Rights Reserved.
