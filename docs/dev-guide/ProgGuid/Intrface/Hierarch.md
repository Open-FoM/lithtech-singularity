| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Class Hierarchies

Implementation classes derived directly from the interface class, which is derived directly from IBase, are the simplest, and probably the most frequent, case. However, there are more complicated arrangements that can be made depending on what is needed. Here are a couple of the more useful arrangements that can be used. There are many more that are possible.

---

## Interface Hierarchies

Interfaces do not need to be directly derived from **IBase **. This might be useful if two interfaces have a subset of common functions declared, but have separate implementations of these functions. This is done currently with the **ILTClient **and **ILTServer **interfaces, which are both derived from **ILTCSBase **. When interfaces are derived from other interfaces (whether or not there are direct implementations of the base interface class), there is a special version of the **interface_version **macro that should be used in the derived class.

>

class IDerived : public IMyInterface {

public:

interface_version_derived(IDerived, IMyInterface, 1);

virtual void AddedFunc() = 0;

};

What the interface_version_derived macro does is combine the version number specified in the macro and combines it with the version number of the base class. The exact formula is:

>

100 * version_base + version_derived

So in the case of IDerived, the real version number would be 1701, since the version of **IMyInterface **was defined as 17. In this way if the version number of either the base or derived class changes, it has an effect on anyone who references the version number of the derived class. If the hierarchy is deeper than 4 levels, however, this system will run into problems.

---

## Stub Implementations

An implementation class does not need to be directly derived from the interface class it is implementing. Instead, there can be a class between the instantiated implementation and interface class that supplies empty or default bodies for the interface functions. Then the real implementation class can derive from the stub implementation class and implement only those functions it wants to.

The engine expects user game code to implement and instantiate the **IClientShell **and **IServerShell **interfaces. When creating the implementation of these interfaces, users can derive their implementation classes from the provided stub implementation classes, **IClientShellStub **and **IServerShellStub **.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Intrface\Hierarch.md)2006, All Rights Reserved.
