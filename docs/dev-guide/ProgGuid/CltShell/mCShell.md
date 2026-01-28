| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Client Shell

Client Shell is a term used to encompass all client-side game code operations and logic. This code is compiled into CShell.dll. The TO2 client shell code is located in the GameClientShell.h header file and GameClientShell.cpp file. You can use these files as a base or an example for your own client shell.

The client shell communicates with the Jupiter to initiate a variety of operations. Jupiter, in return, notifies the client shell of various events, such as engine initialization, messages from the server, input status changes, and so on.

Messaging between the client shell and the server shell is initiated and handled by the game code. Jupiter is not involved with initiating client-to-server or server-to-client game code-related communication. However, Jupiter is used as the mechanism to communicate these messages.

This section introduces the client shell through the following topics:

| #### Client Shell Topic | #### Description |
| --- | --- |
| [SETUP_CLIENTSHELL ](setup.md) | This macro sets up required DLL exports and some global variables. |
| [IClientShell ](ICShell.md) | Your code must include an IClientShell-derived class instance, whether it is one of your own creation or merely based on the TO2 code. Jupiter sends notifications to this derived class to inform your game of various events. There are a variety of notification-handling functions that you may implement. The two most important of these are OnEngineInitialized and Update. You will use this class as a starting point for your game code. |
| [ILTClient ](ILTClnt.md) | The ILTClient class is instantiated by Jupiter. You can obtain a pointer to the instance using the define_holder_to_instance macro. |
| [Collecting Input ](input.md) | Describes how to collect input from the users hardware interface and associate the input with game code. Collecting input is largely a function of the Client Shell. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\CltShell\mCShell.md)2006, All Rights Reserved.
