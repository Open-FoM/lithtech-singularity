| **Training Session Notes ** |
| --- |



Title: **DEdit For Programmers **

Author: David Koenig

Initial Date: 11/06/2002

****

Level designers can expect to spend a great amount of time in DEdit. The object system within Jupiter allows for programmers to make this a more enjoyable and productive time. The object system exposes several interactive "hooks" which DEdit is aware of.



**Object Headers **

In order to expose object functionality to level designers using DEdit, each inter-actable server-side object must contain a header defining custom properties of the object. That is done through a combination of macro functions and flags.



**Header Example **

****

BEGIN_CLASS ( WorldProperties )

ADD_LONGINTPROP_FLAG (FarZ, 10000, PF_DISTANCE )

ADD_COLORPROP_FLAG (BackgroundColor, 0.0f, 0.0f, 0.0f, NULL )

PROP_DEFINEGROUP (FogProps, PF_GROUP (1))

ADD_BOOLPROP_FLAG (FogEnable, false , PF_GROUP (1))

ADD_COLORPROP_FLAG (FogColor, 1.0f, 1.0f, 1.0f, PF_GROUP (1))

ADD_LONGINTPROP_FLAG (FogNearZ, 0, PF_DISTANCE | PF_GROUP (1))

ADD_LONGINTPROP_FLAG (FogFarZ, 2000, PF_DISTANCE | PF_GROUP (1))

ADD_BOOLPROP_FLAG (SkyFogEnable, false , PF_GROUP (1))

ADD_LONGINTPROP_FLAG (SkyFogNearZ, 0, PF_DISTANCE | PF_GROUP (1))

ADD_LONGINTPROP_FLAG (SkyFogFarZ, 2000, PF_DISTANCE | PF_GROUP (1))

PROP_DEFINEGROUP (SkyPanProps, PF_GROUP (2))

ADD_BOOLPROP_FLAG (SkyPanEnable, false , PF_GROUP (2))

ADD_STRINGPROP_FLAG (SkyPanTexture, "", PF_FILENAME | PF_GROUP (2))

ADD_LONGINTPROP_FLAG (SkyPanAutoPanX, 0, PF_DISTANCE | PF_GROUP (2))

ADD_LONGINTPROP_FLAG (SkyPanAutoPanZ, 0, PF_DISTANCE | PF_GROUP (2))

ADD_REALPROP_FLAG (SkyPanScaleX, 1.0, PF_GROUP (2))

ADD_REALPROP_FLAG (SkyPanScaleZ, 1.0, PF_GROUP (2))

ADD_REALPROP_FLAG (SkyScale, 1.0, NULL )

END_CLASS_DEFAULT_FLAGS ( WorldProperties , BaseClass , NULL , NULL , CF_ALWAYSLOAD )



**Flag Types **

Class Flags

Property Flags

****

| **Class Flags ** |  |
| --- | --- |
| Type | Description |
| CF_HIDDEN | If this flag is specified, the object cannot be placed in DEdit. |
| CF_NORUNTIME | This object will not be instantiated at runtime. |
| CF_STATIC | This object remains when a level change happens. |
| CF_ALWAYSLOAD | This object is to be loaded from the world file every time it loads. Even when loading a saved game. |
| CF_WORLDMODEL | This object is a world model. |
| CF_CLASSONLY | This object does not have an HOBJECT associated with it. |





| **Property Flags ** |  |
| --- | --- |
| Type | Description |
| PF_HIDDEN | This property is to be hidden from the level designers in DEdit. |
| PF_RADIUS | This property is to draw a radial visualization based on the value of this field. |
| PF_DIMS | This property is to draw a bounding box visualization based on the value of this field. |
| PF_FIELDOFVIEW | This property specifies the field of view for a frustum radius. |
| PF_LOCALDIMS | The bounding dimensions of the object will be rotated with the object. |
| PF_FOVRADIUS | This property is to draw a frustum visualization based on the value of this field. |
| PF_OBJECTLINK | This property is to draw a three dimensional line to the named object listed in this field. |
| PF_FILENAME | This property is a filename. This adds file browse functionality to the property GUI. |
| PF_BEZIERPREVTANGENT | Defines the previous point in the bezier path. |
| PF_BEZIERNEXTTANGENT | Defines the next point in the bezier path. |
| PF_STATICLIST | This property is a drop-down style widget. |
| PF_DYNAMICLIST | This property is a dynamic drop-down style widget. Level Designers are allowed to add entries to this list. |
| PF_MODEL | This property specifies that this field includes a model filename, and thus renders the specified model. |
| PF_NOTIFYCHANGE | This property is to notify DEdit when a change is made to its’ value. |
| PF_TEXTUREEFFECT | This property is a Texture Effect widget. |



**Object Plug-ins ******

Object plug-ins, are hooks, which DEdit is able to use to allow more control over object properties from within DEdit. There are three functions in the IObjectPlugin class which developers can override in order to obtain that functionality:



virtual LTRESULT PreHook_EditStringList()

This hook function is called when an object is loaded. It’s used to allow developers to fill static and dynamic list widgets with game specific data. For example, you might have a “power-up” object. This would allow you to read data from a text file and fill the different power-up types into the list, which would then be available to the level designers. This allows the designer to simply pick the type from a list instead of keeping track of the exact names of type.



virtual LTRESULT PreHook_Dims()

This hook function is called when the dimensions of an object are changed.



virtual LTRESULT PreHook_PropChanged()

This hook function is called when a property has changed. It requires that the PF_NOTIFYCHANGE property flag is enabled. This is generally used to validate data that the level designer has input.

Copyright 2002,2003 LithTech Inc. PAGE 1 of NUMPAGES 2





****
