| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Collecting Input

Enabling input device recognition is a two-step process. First, input devices and commands must be bound so LithTech will send notifications when input occurs. Secondly, game code must process the input event notifications.

This section explains the following two steps:

- [About Collecting Input ](#AboutCollectingInput)
- [Editing Autoexec.cfg ](#Autoexeccfg)
- [Input Notification Game Code ](#InputNotificationGameCode)

---

## About Collecting Input

The LithTech engine handles the bulk of input collection operations. Generally, the process of capturing user input proceeds as follows:

1.
The user presses a key, moves the mouse, or operates some other input device.
2.
LithTech detects the input.
3.
LithTech determines whether or not the input is recognized by the game (using the information in the autoexec.cfg file).
4.
If the input is not recognized by the game, LithTech does not send an event notification and the process for this specific input event ends.
5.
If the input is recognized by the game, LithTech sends an event notification using the **IClientShell::OnCommandOn **and **IClientShell::OnCommandOff **functions, as appropriate.
6.
Game code conducts operations based on the input.

The input detection process is reactive. Game code waits for an event notification from LithTech. The game can also poll LithTech to determine the state of the input devices. This is accomplished using the **ILTClient::IsCommandOn **function.

[Top ](#top)

---

## Editing Autoexec.cfg

The autoexec.cfg file is a text file containing commands for LithTech. This file tells LithTech what type of input a game recognizes. Many options besides input handling can also be found in this file. You must set the input devices that you want to enable for your game in this file.

These files should be placed in the same directory as LithTech.exe. They are automatically loaded and run by LithTech.

#### To provide input recognition

1. Set names to command integers.
2. Enable the input device.
3. Bind input device to command integers.

### Step One—Set names for command integers

To ease readability, you must set descriptive names to integers that you have decided represent commands to LithTech.

The AddAction command is used to set a name to an integer. A game action is a name that defines a specific input action for a game. For instance, **FirePistol **could be a game action bound to a mouse click event.

#### AddAction action_name value

The **action_name **parameter may be any string (no spaces). This string is used to identify the command in the **rangebind **command described below.

The **value **parameter may be any integer between 0 and 255. Each different command must have a different integer. Do not use the same integer for more than one command.

Names should be set at the beginning of the autoexec.cfg. You should not alter the names in your game. However, additional bindings may be added.

### Step Two—Enable the input device

You must enable each device for which you want to receive input. To enable a device, use the enabledevice command in the autoexec.cfg file:

>
```
enabledevice **“**devicename**”**
```

The **devicename **parameter is the identifier for the input device. It may be one of the following values:

For PC:

>

```
##mouse
```

```
##keyboard
```

### Step Three—Bind input device to command integers

Finally, you must bind the specific inputs to specific commands using the **rangebind **command. A binding is the association of a specific object and range (such as a keyboard key, a mouse click, or a joystick movment) to a specific action (such as a crouch, open, or move forward).

>
```
rangebind “device_name” “object_name” min_value max_value “action_name”
```

The **device_name **parameter identifies the device to which the action will be set. The device must be enabled (using the enabledevice command) before using it in the rangebind command.

The **object_name **parameter sets the input that triggers the action. The LithTech engine supports all the **DIK_* **scan codes in the DirectInput dinput.h file. For maximum portability, use the scan codes whenever possible. For keyboard keys, the name of the key is also an option. For instance, use “E” to set the E key as a trigger for an action. LithTech recognizes the name of keys supported by Microsoft’s DirectInput.

The **min_value **and **max_value **parameters set ranges for the object. This is used primarily for joysticks. For keyboard key input, set both of these to 0.0.

The **action_name **parameter identifies the action that is triggered by this binding. The action name must be set using the **AddAction **command prior to using it in the **rangebind **command.

The last three parameters can be repeated in groups to add more actions. For example:

>
```
rangebind “#Controller” x 0 22767 left 42767 65535 right
```

#### Mouse and Joystick Axes

While the mouse and joystick buttons are enabled in the same manner as keyboard keys, the mouse and joystick axes require special definition. An axis is first bound to an in-game axis, and then it can be scaled for sensitivity.

The axes actions must be defined and the mouse/joystick device enabled as described above.

To bind the axes, the trigger names in the **rangebind **command must be set to a specific string: X-axis, Y-axis, and Z-axis. You must use a negative number to bind an axis. Commonly, -1, -2, and –3 correspond to X, Y, and Z.For example:

>
```
rangebind "##mouse" "Y-axis" 0.000000 0.000000 "Axis2"
```

After you have bound each axis, you can scale it using the scale command.

>
```
scale “device_name” “axis” “scaling_value”
```

The **device_name **variable is a string identifies the device being scaled, usually “ ##mouse ”.

The **axis **variable is a string that identifies the axis to scale. This can be X-axis, Y-axis, or Z-axis.

The **scaling_value **variable sets the scaling factor for the axis. This scaling value is usually set through a sensitivity slider inside the game (and coded with the **InputMgr::ScaleTrigger **function.

The mappings you have enabled will be all that your game receives. If you are not receiving messages, be sure to check that these bindings are set up properly.

#### User-specific Configuration Files

Your **IClientShell **class can also load user-specific .CFG files. This allows users to have separate preferences in the game. The command to load a config file is **ILTClient::ReadConfigFile **.

>
```
LTRESULT ILTClient::ReadConfigFile(char *pFileName);
```

[Top ](#top)

---

## Input Notification Game Code

Your game code must bind the input events to appropriate game actions using the **ILTClient **interface.

At launch, LithTech reads the autoexec.cfg file and sets up all of the devices that you listed with the appropriate bindings. Whenever LithTech detects input, it sends an event notification to your **IClientShell **instance if you have bound a command for that type of input in your game code.

The functions that are notified of input events include **IClientShell::OnCommandOn **and **IClientShell::OnCommandOff **. To support these functions, you must create the same command bindings in game code as you set in the autoexec.cfg file.

### OnCommandOn and OnCommandOff

LithTech calls your **IClientshell **instance on every press of a key that you have mapped to a command in the autoexec.cfg file. It will notify you of key presses and key releases using the **IClientShell::OnCommandOn **and **OnCommandOff functions **, respectively. These functions are defined in the IClientShell.h header file.

Each of these functions receives an integer, a range, and control data from LithTech. The integer identifies the specific command. The integer value for a given command is set in the autoexec.cfg file.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\CltShell\input.md)2006, All Rights Reserved.
