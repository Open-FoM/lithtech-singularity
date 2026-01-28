| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Implementing Classes

The Interface Manager uses two types of classes: interface classes and implementation classes. A class may be both an interface class and an implementation class.

---

## Interface Class

Interface classes managed by the Interface Manager must derive from a class called **IBase **. Ideally, an interface should contain only function pointers and pure virtual functions. Constructors/destructors, non-class type member variables, class type member variables that have constructors/destructors, inline functions, non-pure virtual functions, and non-virtual functions all have their own issues which may or may not cause problems depending on how they are used. Here is an example of an interface:

>

class IMyInterface : public IBase {

public:

interface_version(IMyInterface, 17);

//a function

virtual void foobar(int32 num) = 0;

};

The **interface_version **macro is required in the class and takes the name of the interface, and the version number of this interface. A version number can technically be any signed integer, but it is recommended that they be non-negative and less than 100. In this example, the version number is 17. Any time a new function is added to or removed from the interface, function parameters change, or the ordering of members is modified the version number should be changed.

It is possible to have classes between **IBase **and **IMyInterface **that are used as containers for function declarations that are going to be common between two related interfaces.

---

## Implementation Class

An implementation class is either a class derived from an interface class, or an interface class directly. When implementation classes are derived from an interface class (and not exposed for direct use themselves,) there are no restrictions to the kinds of variables or functions that they may have. However, the implementation class must always be able to be instantiated. Therefore, it must implement all pure virtual functions in the parent class. Here is an implementation of the interface **IMyInterface **shown above:

>

class IMyInterfaceImp : public IMyInterface {

public:

declare_interface(IMyInterfaceImp);

//constructor

IMyInterfaceImp();

//our function.

virtual void foobar(int32 num);

};

Class **IMyInterfaceImp **derives from **IMyInterface **, and overrides the interface’s pure virtual functions. It also includes the **declare_interface **macro. **declare_interface **defines some required virtual functions declared in **IBase **.

### Instantiating Interface Implementations

Interface implementation classes are instantiated using the **define_interface **or the **instantiate_interface **macros. These macros are used at global scope in a .cpp file.

The **define_interface **macro is the simpler of the two, and is used like this:

>

define_interface(IMyInterfaceImp, IMyInterface);

**define_interface **takes the name of the implementation class, and the name of the class that it implements. It performs the following operations:

- Declares a global variable of the implementation class type. This causes the constructor of the implementation class to be called. Remember though that the constructor is called before main(), so you are limited as to what you should be doing at that time. In general, all you should do is initialize function pointers.
- Declares a global variable of an internal type. The constructor of this variable places the instance of the implementation class into the Interface Manager.

The **instantiate_interface **macro is similar to define_interface, but has an additional parameter (instance_name).

>

instantiate_interface(impl_class, interface_class, instance_name);

The instance name of an implementation instance is used to identify a specific instance of an interface implementation when there is more than one. Several interfaces, such as **ILTCommon **and **ILTModel **, are instantiated twice each with the names Client and Server.

Using the **define_interface **macro is the same as using the **instantiate_interface **macro with the third parameter being **Default **.

### Creating Interface and Implementation Classes

#### To create the classes for the Interface Manager

1. Create a header file that defines your interface class. This interface class must be derived from **IBase **. The interface class should not contain any non-virtual functions. Use the **interface_version **macro to set the version number of the interface.
2. Create an implementation class derived from your interface class. At the top of this implementation class insert the **declare_interface **macro. Pass in the name of the implementation class.
3. In the .CCP file that defines the implementation class, use the **define_interface **or **instantiate_interface **macro to instantiate the implementation class and register it with the Interface Manager.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Intrface\ImpClass.md)2006, All Rights Reserved.
