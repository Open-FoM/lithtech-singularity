| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# ILTClient

The **ILTClient **class provides significant functionality to your game code. You cannot instantiate **ILTClient **in your code. Jupiter instantiates the **ILTClient **interface. The [SETUP_CLIENTSHELL ](setup.md)macro gives your game code a pointer to LithTech’s **ILTClient **instance.

Use this pointer to access the **ILTClient **functions. For example, to pause music in a game, you would call **ILTClient::PauseMusic **. Various **ILTClient **functions are discussed in throughout this book. The **ILTClient **class is defined in the iltclient.h header file.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\CltShell\ILTClnt.md)2006, All Rights Reserved.
