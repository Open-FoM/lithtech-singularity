| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Server Shell

Server Shell is a term used to encompass all server-side game code operations and logic. This code is compiled into Object.lto. The TO2 server shell code is located in the GameServerShell.h header file and GameServerShell.cpp file. You can use these files as a base or an example for your own server shell.

The server shell communicates with the Jupiter engine to initiate a variety of operations. The Jupiter engine, in return, notifies the server shell of various events.

Messaging between the client shell and the server shell is initiated and handled by the game code. The engine is not involved with initiating client-to-server or server-to-client communication. However, the engine is used as the mechanism to communicate these messages.

This chapter introduces the basics of the server shell. It has the following major sections:

| #### Server Shell Topic | #### Description |
| --- | --- |
| [SETUP_SERVERSHELL ](Setup.md) | This macro sets up required DLL exports and some global variables. |
| [IServerShell ](ISShell.md) | Your code must create (or derive) an instance of this class. Jupiter sends notifications to this class to inform your game of various events. There are a variety of notification-handling functions that you may implement. Two important functions are **OnClientEnterWorld **and **OnClientExitWorld **. |
| [ILTServer ](ILTSrvr.md) | The **ILTServer **class is instantiated by Jupiter. You can obtain a pointer to the instance using the **define_holder **macro. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SrvShell\mSShell.md)2006, All Rights Reserved.
