| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Publishing Objects to DEdit

All objects must be initialized using the **BEGIN_CLASS **and **END_CLASS* **macros (defined in ltserverobj.h). These macros initialize an object in LithTech and instruct DEdit to include the object in its various menus and dialog boxes. Universal properties are automatically exposed to DEdit. The optional property macros, **ADD_*PROP **and **ADD_*PROPFLAG **, define which custom properties are exposed to DEdit for modification.

| **Note: ** | To prevent an object from appearing in DEdit menus, use the **CF_HIDDEN **flag with the **END_CLASS_DEFAULT_FLAGS **macro. |
| --- | --- |

This section contains the following topics related to publishing objects to DEdit:

- [Adding a Macro Block ](#AddingaMacroBlock)
- [Adding Custom Properties ](#AddingCustomProperties)
- [Publishing to DEdit Example ](#PubExample)

---

## Adding a Macro Block

The following code example shows a typical macro block:

>
```
BEGIN_CLASS(Classname)
```

```
[add_properties]
```

```
...
```

```
END_CLASS_DEFAULT(Classname,Parentclass,NULL,NULL);
```

This macro block is traditionally placed at the beginning of the .cpp file for a game object class.

The **BEGIN_CLASS **macro takes the name of the game object class to be published to DEdit.

The **END_CLASS_DEFAULT **macro takes the name of the game object class, the name of the game object’s parent class, and two null values (These are holdovers from the original C version of LithTech, where you needed to provide the two required functions. In the current C++ version of LithTech, they are already defined as you derive from **BaseClass **at your top level.)

Optionally, you can use the **END_CLASS_DEFAULT_FLAGS **macro instead of **END_CLASS_DEFAULT **. This allows you to add a combination of **CF_ **flags after the second NULL.

**CF_HIDDEN **—Instances of the class cannot be placed in DEdit.

**CF_NORUNTIME **—This class does not get used at runtime. LithTech will not instantiate these objects out of the world file.

**CF_STATIC **—This is a special class that the server creates an instance of at the same time that it creates the server shell. The object is always around. This can be used as an alternative to adding code to the server shell. Using a **CF_STATIC **object can make the code easier to reuse.

**CF_ALWAYSLOAD **—Objects of this class and sub-classes are always loaded from the level file and cannot be saved to a save game.

---

## Adding Custom Properties

The **BEGIN_CLASS/END_CLASS_DEFAULT **block is enough to register the object and universal properties in the DEdit menus and make them accessible to the level designer. However, if you want to give the level designer the power to modify the custom properties of the game object, you must use the **ADD_*PROP **(or **ADD_*PROP_FLAG **) macros inside the block.

The **ADD_*PROP **macros add a custom property with default values to the DEdit interface for the object.

**ADD_REALPROP(name, val) **—Adds a real floating point property named name, with val as the default.

**ADD_STRINGPROP(name, val) **—Adds a string property named name, with val as the default.

**ADD_VECTORPROP(name) **—Adds a vector property named name. No default.

**ADD_VECTORPROP_VAL(name, defX, defY, defZ) **—Adds a vector property named name, with the defX, defY, and defZ default X, Y and Z values.

**ADD_LONGINTPROP(name, val) **—Adds a long integer property with name name, and default value val.

**ADD_ROTATIONPROP(name) **—Adds a rotation property named name

**ADD_BOOLPROP(name, val) **—Adds a Boolean property named name, with val default setting.

**ADD_COLORPROP(name, valR, valG, valB) **—Adds a color property named name, with the default R, G and B values specified.

**ADD_OBJECTPROP(name, val) **—Adds an object property, a link to another object where val is the name of the object.

To affect how DEdit displays the custom properties on a given object, use the **ADD_*PROP_FLAG **macros instead of the **ADD_*PROP **macros. With these, you can append a combination of **PF_* flags **.

**PF_HIDDEN **—The property does not show up in DEdit.

**PF_RADIUS **—The property is a number to use as radius for drawing circle. There can be more than one.

**PF_DIMS **—The property is a vector to use as dimensions for drawing a box. There can be only one.

**PF_DISTANCE **—This property defines a measurement or other value that is in, or relative to, world coordinates. If this flag is specified, any scaling done to the world as a whole for unit conversion will also be applied to this property.

**PF_FIELDOFVIEW **—The property is a field of view.

**PF_LOCALDIMS **—Causes DEdit to show dimensions rotated with the object. Used with PF_DIMS.

**PF_GROUPOWNER **—This property owns the group it is in.

**PF_GROUP1…6 **—This property is in group 1-6. This is used for grouping properties into subscreens.

**PF_FOVRADIUS **—If PF_FIELDOFVIEW is set, this defines the radius for it.

**PF_OBJECTLINK **—If the object is selected, DEdit draws a line to any objects referenced by name in **PF_OBJECTLINK **properties. It will not draw any more than **MAX_OBJECTLINK_OBJECTS **.

**PF_FILENAME **—This indicates to DEdit that a string property is a filename in the resource.

**PF_BEZIERPREVTANGENT **—This vector property changes draws an object’s path from this vector to the previous vector as a bezier curve.

**PF_BEZIERNEXTTANGENT **—This vector property changes draws an object’s path from this vector to the next vector as a bezier curve.

**PF_STATICLIST **—This string property has a populatable combo box with dropdown-list style (that is, a list box with no edit control).

**PF_DYNAMICLIST **—This string property has a populatable combo box with dropdown style (that is, a list box with edit control).

[Top ](#top)

---

## Publishing to DEdit Example

The following code example publishes the **CNamelessThug **object to DEdit. It exposes two custom properties: **Model **and **Suspicious **.

>
```
BEGIN_CLASS( CNamelessThug )
```

```
       ADD_STRINGPROP_FLAG( Model, "" , PF_FILENAME      )
```

```
       ADD_BOOLPROP( Suspicious, LTFALSE )
```

```
END_CLASS_DEFAULT( CNamelessThug, BaseClass, NULL, NULL )
```

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SimObj\PubObj.md)2006, All Rights Reserved.
