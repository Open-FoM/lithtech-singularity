| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# ILTServer

The **ILTServer **class provides significant functionality to your game code. You cannot instantiate **ILTServer **in your code. Jupiter instantiates the **ILTServer **interface. The [SETUP_SERVERSHELL ](Setup.md)macro gives your game code a pointer ( **g_pLTServer **) to Jupiter’s ILTServer instance.

Use this pointer to access the **ILTServer **functions. For example, use the **ILTServer::KickClient **function to kick a client off of the server.

Various **ILTServer **functions are discussed in this book. The **ILTServer **class is defined in the iltserver.h header file.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SrvShell\ILTSrvr.md)2006, All Rights Reserved.
