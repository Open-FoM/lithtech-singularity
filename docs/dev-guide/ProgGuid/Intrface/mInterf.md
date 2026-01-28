| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Interface Manager

The Interface Manager provides a consistent, modular method by which game and engine code can define, implement, instantiate, and retrieve interfaces. To retrieve engine interface instances, your game code must use the Interface Manager. Also, your game code must use implement the **IClientShell **and **IServerShell **interfaces using the Interface Manager.

An interface is a C++ class that contains functions that can be called. An interface implementation is a C++ class that can be instantiated and that is either an interface or is derived from an interface class. Interface instantiations are actual instances of an interface implementation class.

The Interface Manager exposes a set of macros that provide the following functionality:

- interface versioning
- instance interface implementations expected by the engine
- access engine interface instances

Each of the preceeding macros are defined in the ltmodule.h header file.

This section provides information on the following topics:

| [Implementing Classes ](ImpClass.md) | Interfaces that are part of the Interface Manager system must derive from a base class called IBase. Interface implementations must use certain macros to add functionality required by the Interface Manager. Not all interfaces in your game code are required to be part of the Interface Manager system. |
| --- | --- |
| [Accessing Interface Instances ](Access.md) | The engine instantiates and manages many interface instances (such as ILTPhysics, ILTClient, ILTServer, and so on). These engine interface instances expose functions that your game code uses to conduct various operations. The Interface Manager provides a consistent method to access interface instances and call the exposed interface functions. |
| [Class Hierarchies ](Hierarch.md) | Describes some of the different interface hierarchy arrangements you can use when implementing your class structure. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Intrface\mInterf.md)2006, All Rights Reserved.
