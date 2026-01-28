| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# IClientShell Class

The **IClientShell **class is used by Jupiter to notify the game code of events. You must derive a new class from **IClientShell **to provide communication from Jupiter to your game code.

Your **IClientShell **-derived class must include the **declare_interface **macro at the top of the class definition. This macro provides functions required for proper communication with Jupiter.

## IClientShell Functions

There are many event callback functions in **IClientShell **. You may implement each of these functions to provide any desired functionality whenever Jupiter sends notification of a specific event. Some of these functions are more important than others. Depending on the functionality you want to offer in your game, some of these functions may be left un-implemented. However, implementation of certain functions are imperative to basic game functionality. For example, if you do not override **OnEngineInitialized **, it will always return **LT_ERROR **, immediately shutting down the game.

The notification functions available in **IClientShell **include:

**OnCommandOff **—Called when commands (defined in AUTOEXEC.CFG) are released.

**OnCommandOn **—Called when commands (defined in AUTOEXEC.CFG) are triggered.

**OnEngineInitialized **—Called when Jupiter is initialized. Required function.

**OnEngineTerm **—Called when Jupiter is terminated.

**OnEnterWorld **—Called when the player enters a world. This is useful for creating or initiliazing lists for referencing engine objects.

**OnEvent **—This function gives you a chance to hook into Jupiter’s event handling to trap events such as the renderer being reinitialized, or the game losing or regaining focus.

**OnExitWorld **—Calledwhen the player exits a world. This is useful for cleaning up lists for referencing engine objects. When the player exits a world, the game loses handles to every engine object and so it is important to clear out references to them.

**OnKeyDown **—Used to access PC virtual key data. DO NOT USE THESE IF YOU INTEND TO PORT YOUR GAME TO ANOTHER PLATFORM. Virtual keys are specific to Windows and, unlike Jupiter commands (see OnCommandOn/OnCommandOff), are not portable to any other computer or console configuration.

**OnKeyUp **—Used to access PC virtual key data. DO NOT USE THESE IF YOU INTEND TO PORT YOUR GAME TO ANOTHER PLATFORM. Virtual keys are specific to Windows and, unlike Jupiter commands (see OnCommandOn/OnCommandOff), are not portable to any other computer or console configuration.

**OnMessage **—Called when the server game code sends a normal server-to-client message to the client.

**OnModelKey **—Used for client-side MODELKEY handling. Model keys are strings coupled with frames of model animation that are used to target events.

**OnObjectMove **—Used to aid in handling client-side physics and movement prediction.

**OnObjectRemove **—Called when an object is removed.

**OnObjectRotate **—Used to aid in handling client-side physics and movement prediction.

**OnTouchNotify **—Used for client-side collision detection.

**PostUpdate **—Called each update cycle after Update and each engine object have been updated.

**PreLoadWorld **—Called before a world is loaded, and anything drawn to the screen is automatically displayed after the call exits. This is useful for displaying a “Now Loading…” screen or something similar.

**PreUpdate **—Called each update cycle, just before Update. Currently, nothing happens between this call and the call to Update, so this function is primarily used for conceptual organization.

**SpecialEffectNotify **—Called when the server sends a special effect message to the client.

**Update **—Called as frequently as possible. You should use this function to conduct frequent game operations.

[Top ](#top)

---

## Instantiating Your IClientShell-derived Class

The **IClientShell **instance can be created on the heap (using the new operator or malloc) or instantiated on the stack.

To instantiate your **IClientShell **-derived class, you must declare a global pointer and then use the **define_interface **macro.

>

impl_class *g_pImpl_Class = NULL;

define_interface(impl_class, interface_class);

The **imp_class **parameter is the name of your derived class. The interface_class parameter is the name of the class from which the **imp_class **parameter is derived (in this case, **IClientShell **).

[Top ](#top)

---

## OnEngineInitialized

The **OnEngineInitialized **function is called when Jupiter has been started and fully initialized. At this point, the Jupiter engine has loaded the client .dll’s, ensured that the proper operating system support is available, and loaded and parsed the autoexec.cfg file. Any console variables and key bindings defined in the autoexec.cfg and/or the command-line have been set.

The game implementation of this function must set the proper display resolution and initialize the renderer on the PC. The game should also be set to an initial state. In general, this would mean loading a startup screen and giving the user some options.

>

LTRESULT OnEngineInitialized(RMode *pMode, LTGUID *pAppGuid)

{

// Your game code goes here

// Return something other than LT_OK if you want to abort the game

}

Jupiter passes in a pointer to the default rendering mode ( **pMode **) to initialize the renderer. Rendering modes are defined using **RMode **structures. The **RMode **identified by the **pMode **parameter is created by Jupiter based on the following:

- The 3d graphics card. If Jupiter finds a 3D graphics card, the **pMode->m_bHardware **variable is set to TRUE.
- Default resolution and bitdepth. Command line parameters get the highest priority and will be used if specified.
- Console variable settings ( **RenderDLL **, **ScreenWidth **, and **ScreenHeight **) in the autoexec.cfg file.

If none of these can be found, Jupiter defaults to 640x480 with 16-bit color (the lowest possible resolution that Jupiter can use).

Jupiter does not automatically initialize the renderer. Your game code must do this by calling the **ILTClient::SetRenderMode **function.

The **pAppGuid **out-parameter is a pointer to a **LTGUID **structure. You should fill in the GUID before returning from this function. Jupiter will store this information and use it to determine which net games can be queried and connected to.

If the **OnEngineInitialized **function returns anything other than **LT_OK **, LithTech shuts down immediately.

[Top ](#top)

---

## Set the Rendering Mode

The OnEngineInitalized function receives a rendering mode from Jupiter. However, Jupiter does not automatically initialize the renderer. Your implementation of the **OnEngineInitialized **function must explicitly set the rendering mode by calling the **ILTClient::SetRenderMode **function.

>

LTRESULT (*SetRenderMode)(RMode *pMode);

This function is accessible to your code using the **ILTClient **pointer, **g_pLTClient **. The **pMode **parameter is a pointer to an **RMode **structure identifying the rendering mode with which you would like the initialize the renderer. Your code may use the **RMode **structure passed in to the **OnEngineInitialized **function, or you may create and utilize other **RMode **structures of your choice. However, Jupiter may not use the rendering mode you requested based on internal engine logic and hardware/software limitations. After calling **SetRenderMode **, you can confirm the actual rendering mode used with the **ILTClient::GetRenderMode **function.

[Top ](#top)

---

## Update

The Jupiter engine calls the **IClientShell::Update **function on each update loop. It is the main function that is called while Jupiter is running. This is where your game should poll input, perform any client game-code logic, and render the scene.

Jupiter calls the **Update **function as often as possible. The frequency with which the function is called is dependant upon the hardware limitations of the computer and the number of operations the game is currently processing.

[Top ](#top)

---

## void Update()

Jupiter does not pass any information into the **Update **function, nor does it expect any return value. Jupiter merely calls the function to give the client shell the opportunity to conduct game operations as often as possible.

[Top ](#top)

---

## Render

The **CgameClientShell::RenderCamera **function uses client engine API calls to clear the screen, start a 3D optimized block, render the camera, end the 3D block, and flip the screen buffer.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\CltShell\ICShell.md)2006, All Rights Reserved.
