| **Training Session Notes ** |
| --- |



Title: **Custom Game Objects **

Author: David Koenig

Initial Date: 11/07/2002

****

There are two ways to create an instance of an object in the world:



DEdit

Programmatically at runtime



**Object Classifications **:



Client-side objects - Visible only to that client

Server-side objects - Visible to all clients



**Code Example **:



HCLASS hClass = g_pLTServer -> GetClass ("Projectile");

ObjectCreateStruct ocs ;

ocs.m_ObjectType = OT_MODEL ;

ocs.m_Flags = FLAG_VISIBLE ;

ocs.m_Rotation .Init();

ocs.m_Pos .Init();

strncpy ( ocs.m_Filename , "Models/GenCan.ltb", MAX_CS_FILENAME_LEN -1);

strncpy ( ocs.m_SkinName , "ModelTextures/GenCan1.dtx", MAX_CS_FILENAME_LEN -1);

Projectile * pObject = ( Projectile *) g_pLTServer -> CreateObject ( hClass , & ocs );



****

| **Object Types ** |  |
| --- | --- |
| Type | Description |
| OT_NORMAL | This object type is invisible, and is not transferred to the client automatically. Normal uses for this object are things such as waypoints, or anything that needs a rotation and position, without the need to be visible, or have a model or world geometry associated with it. |
| OT_MODEL | This object type is used when you want to load a model. Generally you associate code with the model, for thing such as projectiles or player characters. |
| OT_WORLDMODEL | This object type has world geometry associated with it. Examples might be, doors, triggers, or breakable lights. |
| OT_SPRITE | This object type is used when you want to load a sprite (billboard) object. You might do this for bullet marks, or flames. |
| OT_LIGHT | This object type is a dynamic light. You might create one of these for special effects. Perhaps your character is holding a torch in which the light source is to follow that character around. |
| OT_CAMERA | This object type represents a 3D camera object. You must have at least one of these objects created on the client-side in order to view a 3D world. |



| **Object Types Continued… ** |  |
| --- | --- |
| Type | Description |
| OT_PARTICLESYSTEM | This object is a standard particle system. This object is rarely used. Generally ClientFX is used for placing particle systems within a world. |
| OT_POLYGRID | This object type represents a grid of vertices, which can be manipulated at runtime to create dynamic effects such as advanced water systems. |
| OT_LINESYSTEM | This object type allows you to create lines in 3D space. The most common application of this object is for debugging purposes. For example you could create a box object to represent the dimensions of an object in the world. |
| OT_CONTAINER | This object type is very similar to the OT_WORLDMODEL object. The difference is that this object is meant to contain other objects. Generally this object is used for trigger objects, or water volumes which need to affect the physics of player objects. |
| OT_CANVAS | This object type is unique in that it allows you to specify a callback function witch is called during rendering to allow you to render custom objects. This is most often used in conjunction with the ILTDrawPrim interface. |
| OT_VOLUMEEFFECT | This object type is used for specifying the bounding dimensions of a particle system effect. Example: Snow or Rain. |

****

****

****

****

**Engine Messages: **

The engine posts these messages, to the object, when certain events take place in order to let developers handle those events.

****

| **Engine Messages ** |  |
| --- | --- |
| Message | Description |
| MID_PRECREATE | This message posts right before the object is created in order to give developers access to the object’s ObjectCreateStruct . |
| MID_OBJECTCREATED | Once the object has been placed in the world, it receives this message. |
| MID_INITIALUPDATE | The object receives this message upon it’s first update. |
| MID_UPDATE | The object receives this message at the beginning of each update. You must call SetNextUpdate() each time, in order for the object to receive future update messages. |
| MID_TOUCHNOTIFY | When an object is touched by another object, this message is posted. |
| MID_LINKBROKEN | When an “inter object link” link is broken between two objects, this message is posted on the remaining object. |
| MID_MODELSTRINGKEY | In ModelEdit, you can attach strings to key frames in an animation. When one of these strings is reached within an animation at runtime, this message is posted to the object. |
| MID_CRUSH | This message is posted if the object is sandwiched between and object and the world. |
| MID_LOADOBJECT | This message is called when a saved game is loaded so that you can restore the previous state of an object. |
| MID_SAVEOBJECT | This message is called when a game is save so that you can store the current state of an object. |
| MID_AFFECTPHYSICS | This message is posted if the physics of this object is to be affected by another object. |
| MID_PARENTATTACHMENTREMOVED | If the parent object of an attachment is removed from the world, the remaining child object will receive this message. You might use this to destroy an object that was attached to another object, when the parent object is destroyed. |
| MID_ACTIVATING | If an object is becoming active this frame, it will receive this message. |
| MID_DEACTIVATING | If an object is becoming inactive this frame, it will receive this message. |
| MID_ALLOBJECTSCREATED | When the level is finished loading and once all objects are created in the world, this message gets posted to each object. |
| MID_TRANSFORMHINT | Posted when the engine is providing the movement encoding transform hint for a node. |

****

****

****

****

****

**Object Flags: **

Each object has a set of flags associated with it, which defines rendering, physics, and other miscellaneous behaviors. By default all flags are not applied. You must specifically specify each flag that you want the object to use.



| **Object Flags ** |  |
| --- | --- |
| Flag | Description |
| FLAG_VISIBLE | This specifies that the object should be visible. |
| FLAG_SHADOW | This specifies that the object should cast a shadow on the world. This applies only to models. |
| FLAG_ROTATABLESPRITE | This specifies that a sprite is allowed to rotate to face the camera. This applies only to sprite objects. |
| FLAG_ONLYLIGHTWORLD | This specifies that a light should ignore models when lighting. This applies only to light objects. |
| FLAG_REALLYCLOSE | This specifies that the object is to be transformed into camera space instead of world space. Used for player view weapons. |
| FLAG_ONLYLIGHTOBJECTS | This specifies that a light should ignore the world when lighting. This applies only to light objects. |
| FLAG_NOLIGHT | This specifies that the object is to ignore all lights and render as full bright. |
| FLAG_RAYHIT | If an object is non-solid, but needs to be able to receive ray cast hits, enable this flag. |
| FLAG_SOLID | This enables physics for the object. |
| FLAG_BOXPHYSICS | This specifies that the object should use an Axis Aligned Bounding Box for physics. |
| FLAG_CLIENTNONSOLID | This specifies that the object be non-solid on the client. |
| FLAG_TOUCH_NOTIFY | This enables MID_TOUCHNOTIFY messages from the server. |
| FLAG_STAIRSTEP | When an object hits a short piece of the world, such as a step, if it is a certain height, the object is moved vertically to match the height of the step, if this flag is enabled. |
| FLAG_GOTHRUWORLD | Objects can pass through the outer shell of the world with this flag enabled. |
| FLAG_POINTCOLLIDE | The dimensions of the object are reduced to relative coordinates 0,0,0 of the object. |
| FLAG_MODELKEYS | This enables MID_MODELSTRINGKEY messages. |
| FLAG_TOUCHABLE | This object will only receive MID_TOUCHNOTIFY messages. Object’s hit by this object will not receive them. |
| FLAG_FORCECLIENTUPDATE | This forces the server to send position/rotation information to the client regarding this object. Useful if you want information about an invisible object sent to the client. |
| FLAG_REMOVEIFOUTSIDE | If this object is outside of the world’s shell, remove it. |
| FLAG_CONTAINER | This enables the MID_AFFECTPHYSICS engine message. |



| **Secondary Object Flags ** |  |
| --- | --- |
| Flag | Description |
| FLAG2_FORCEDYNAMICLIGHTWORLD | This flag forces a light to dynamically light the world. You generally want to use this flag if you place a dynamic light in the world. This only applies to OT_LIGHT objects. |
| FLAG2_ADDITIVE | This specifies that the sprite should be additively blended. This only applies to OT_SPRITE objects. |
| FLAG2_MULTIPLY | This specifies that the sprite should be multiplicative blended. This only applies to OT_SPRITE objects. |
| FLAG2_SKYOBJECT | This object is meant to be a part of the sky. |
| FLAG2_DYNAMICDIRLIGHT | This flag specifies that the object is a dynamic directional light. This only applies to OT_LIGHT objects. |
| FLAG2_PLAYCOLLIDE | This enables an axis aligned cylinder for the object’s bounding dimensions. This is usually enabled for player objects. It allows for more realistc collisions against world geometry, and lessens the potential for getting stuck in corners. |
| FLAG2_FORCETRANSLUCENT | This flag should be enabled if the object is meant to have any translucency. It forces the object to be considered for translucency. |
| FLAG2_DISABLEPREDICTION | This object is to not calculate client-side prediction information. You might use this in situations that an object could become out of sync with the client. It forces exact position information. |
| FLAG2_SERVERDIMS | This forces the server to send dimension information to the client for this object. |
| FLAG2_SPECIALNONSOLID | Objects with this flag set are non-solid to other objects with this flag set. |





Copyright 2002,2003 LithTech Inc. PAGE 1 of NUMPAGES 5
