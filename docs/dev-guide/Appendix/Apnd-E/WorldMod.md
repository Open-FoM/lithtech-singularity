| ### Content Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Appendix E: WorldModel Reference

This appendix provides information on the various WorldModel objects provided with Jupiter.

- [WorldModel ](#WorldModel)
- [RotatingWorldModel ](#RotatingWorldModel)
- [SlidingWorldModel ](#SlidingWorldModel)
- [SpinningWorldModel ](#SpinningWorldModel)
- [RotatingDoor ](#RotatingDoor)
- [SlidingDoor ](#SlidingDoor)
- [RotatingSwitch ](#RotatingSwitch)
- [SlidingSwitch ](#SlidingSwitch)

---

## WorldModel

This is the basic non-active WorldModel object. It does not move or rotate, does not play any sounds and cannot execute any commands. It can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected.

Use this object any place where you need a stationary WorldModel. An example would be a desk that you want to damage or to be able to handle attachments. This object is also very well suited for keyframing. If you want a keyframed WorldModel object you should use this object.

| **Creation ** | This object is created normally. Simply bind this object to a brush or groups of brushes and then edit the properties as you see fit. |
| --- | --- |
| **Damage ** | This object can handle damage. Edit the DamageProperties subset to define behavior. |
| **Attachments ** | This object can handle attachments. Enter all the objects you want attached to the WorldModel, separated by a semicolon, in the Attachments property. |
| **States ** | None. This object does not move or rotate so there is no need for states. |
| **Messages ** | WorldModels can receive the following messages: - **ATTACH ** Attaches the object specified in the message - **DETACH ** Detach the object attached with the ATTACH message. |
| **Options ** | None. The player and AI cannot activate this object. It has no movement or rotation properties. |
| **Commands ** | None. This object cannot execute any commands. |
| **Sounds ** | None. This object cannot play any sounds. |
| **Animated Lightmaps ** | This object does not support animated light maps. |

[Top ](#top)

---

## RotatingWorldModel

This WorldModel can rotate around a point a specified number of degrees around each axis. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. It can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This WorldModel can also have an animated light map associated with it.

Use this object any place you want a WorldModel that should swing like it is hinged to a wall. Kitchen cabinets or window shutters are a good examples of appropriate uses for a RotatingWorldModel.

| **Creation ** | This object is created normally. Simply bind this object to a brush or group of brushes. To set its rotation properties just edit the vector labeled **RotationAngles **and either position the bound object where you would like to rotate around or link to a Point object positioned where you want the object to rotate around. Enter the number of degrees, positive or negative, around each axis you would like this object to rotate. Using the kitchen cabinet as an example you would create your brush in the closed position, bind a RotatingWorldModel to the brush, move the bound object to either the left or right edge of the brush, and then edit the vector (0.0 140.0 0.0). This cabinet door will now open out 140 degrees. |
| --- | --- |
| **Damage ** | This object can handle damage. Edit the **DamageProperties **subset to define behavior. |
| **Attachments ** | This object can handle attachments. Enter all the objects you want attached to the RotatingWorldModel, separated by a semicolon, in the Attachments property. |
| **States ** | - **Off **By default RotatingWorldModels start in the Off position, and are considered Off while in this position. - **On **When the RotatingWorldModel as fully rotated to the degree amounts specified in the **RotationAngles **property, it is considered On. - **PowerOff **While rotating from the On position towards the Off position the RotatingWorldModel is in the PowerOff state. - **PowerOn ** While rotating towards the On position it is considered to be in the PowerOn state. |
| **Messages ** | A RotatingWorldModel can receive the following messages: - **ATTACH ** Attaches the object specified in the message - **DETACH **Detach the object attached with the ATTACH message. - **LOCK **Locks the object. Once locked the object cannot be activated. - **OFF **If not already in the Off or PowerOff states, puts the WorldModel in the PowerOff state. - **ON ** If not already in the On or PowerOn states, puts the WorldModel in the PowerOn state. - **TRIGGER **Toggles the state. Basically the same as when a player presses use against this object. If the WorldModel is in the On or PowerOn state it will immediately switch to PowerOff. If it’s in the Off or PowerOff state, then it will switch to PowerOn. - **UNLOCK **Unlocks the object so it can be activated by the player and other messages. |
| **Options ** | A RotatingWorldModel has the following editable options: - **ForceMove **When TRUE the RotatingWorldModel will rotate through the player and other objects. - **Locked **Toggles whether this object starts locked or not. - **PlayerActivate **Toggles if the Player can interact with the RotatingWorldModel or not. - **RemainOn **If this is TRUE the RotatingWorldModel will stay in the On position until directly told to turn off, either by a player or a message. - **RotateAway **If TRUE the RotatingWorldModel will swing away from the player. - **StartOn **When TRUE the RotatingWorldModel is in the On position at load time. - **TriggerOff **Toggles whether or not the player can directly turn a RotatingWorldModel Off. - **Waveform **Defines how the object Rotates. |
| **Commands ** | A RotatingWorldModel can send these commands when in the corresponding state. - **LockedComand ** - **OffCommand ** - **OnCommand ** - **PowerOffComand ** - **PowerOnCommand ** |
| **Sounds ** | A RotatingWorldModel can play these sounds when in the corresponding state. - **LockedSound ** - **OffSound ** - **OnSound ** - **PowerOffSound ** - **PowerOnSound ** |
| **Animated Lightmaps ** | This object supports animated light maps. |

[Top ](#top)

---

## SlidingWorldModel

This WorldModel can slide, or move, a specified number of units in a specified direction. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. It can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This WorldModel can also have an animated light map associated with it.

Use this object any place you want a WorldModel that should move in a specific direction. Desk drawers are a good example of what these can be used for but of course there are many uses.

| **Creation ** | This object is created normally. Simply bind this object to a brush or group of brushes. To set its movement properties just edit the vector labeled **MoveDir **and set the distance you would like this SlidingWorldModel to move to in the property labeled **MoveDist **. A **MoveDir **vector of (0.0 1.0 0.0) will move the SlidingWorldModel along its own local Y axis, typically straight up. This vector can be edited to point in any direction you want (0.5 0.5 0.0) will move the SlidingWorldModel diagonally along its X and Y axis. The **MoveDist **property is the distance the SlidingWorldModel will travel in DEdit units. Using the desk drawer as an example you would create your brush in the closed position, bind a SlidingWorldModel to the brush and then edit the **MoveDir **vector (0.0 0.0 1.0). Now specify how far you want the object to move by making **MoveDist **64.0. The desk drawer will now slide 64 units out in its local Z axis. |
| --- | --- |
| **Damage ** | This object can handle damage. Edit the **DamageProperties **subset to define behavior. |
| **Attachments ** | This object can handle attachments. Enter all the objects you want attached to the SlidingWorldModel, separated by a semicolon, in the **Attachments **property. |
| **States ** | **Off **—By default SlidingWorldModels start in the Off position, and are considered Off while in this position. **On **—When the SlidingWorldModel is fully moved to the distance specified in the MoveDist property, it is considered On. **PowerOff **—While moving from the On position towards the Off position the SlidingWorldModel is in the PowerOff state. **PowerOn **—While moving towards the On position it is considered to be in the PowerOn state. |
| **Messages ** | A SlidingWorldModel can receive the following messages. > **ATTACH **—Attaches the object specified in the message **DETACH **—Detach the object attached with the ATTACH message. **LOCK **—Locks the object. Once locked the object cannot be activated. **OFF **—If not already in the Off or PowerOff states, puts the WorldModel in the PowerOff state. **ON **—If not already in the On or PowerOn states, puts the WorldModel in the PowerOn state. **TRIGGER **—Toggles the state. Basically the same as when a player presses use against this object. If the WorldModel is in the On or PowerOn state it will immediately switch to PowerOff. If it’s in the Off or PowerOff state, then it will switch to PowerOn. **UNLOCK **—Unlocks the object so it can now be activated by the player and other messages. |
| **Options ** | A SlidingWorldModel has the following editable options: > **ForceMove **—When TRUE the SlidingWorldModel will move through the player and other objects. **Locked **—Toggles weather this object starts locked or not. **PlayerActivate **—Toggles if the Player can interact with the SlidingWorldModel or not. **RemainOn **—If this is TRUE the SlidingWorldModel will stay in the On position until directly told to turn off, either by a player or a message. **StartOn **—When TRUE the SlidingWorldModel is in the On position at load time **TriggerOff **—Toggles weather or not the player can directly turn a SlidingWorldModel Off. **Waveform **—Defines how the object moves. |
| **Commands ** | A SlidingWorldModel can send the following commands when in the corresponding state: - **LockedComand ** - **OffCommand ** - **OnCommand ** - **PowerOffComand ** - **PowerOnCommand ** |
| **Sounds ** | A SlidingWorldModel can play the following sounds when in the corresponding state: - **LockedSound ** - **OffSound ** - **OnSound ** - **PowerOffSound ** - **PowerOnSound ** |
| **Animated Lightmaps ** | SlidingWorldModel objects support animated light maps. |

[Top ](#top)

---

## SpinningWorldModel

This WorldModel can spin around a point a in the specified amount of time to make one rotation around each axis. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This WorldModel can also have an animated light map associated with it.

Use this object any place you want a WorldModel that should spin around a central point. Ceiling fans or a Rolodex are good examples of what these can be used for but of course there are many uses.

| **Creation ** | This object is created normally. Simply bind this object to a brush or group of brushes. To set its rotation properties just edit the vector labeled **RotationAngles **and either position the bound object where you would like to spin around or link to a Point object positioned where you want the object to spin around. Enter the amount of time, in seconds, to make one rotation around each axis, you would like this object to spin. Using the ceiling fan as an example you would create your brush in the closed position, bind a SpinningWorldModel to the brush, move the bound object to the center of the object (by default it is), and then edit the **RotationAngles **vector (0.0 4.0 0.0). This fan will now spin around its Y axis once every 4 seconds. |
| --- | --- |
| **Damage ** | This object can handle damage. Edit the **DamageProperties **subset to define behavior. |
| **Attachments ** | This object can handle attachments. Enter all the objects you want attached to the RotatingWorldModel, separated by a semicolon, in the **Attachments **property. |
| **States ** | **Off **—By default RotatingWorldModels start in the Off position, and are considered Off when they are no longer spinning. **On **—When the SpinningWorldModel is spinning at the desired rate specified in the **RotationAngles **property, it is considered On. **PowerOff **—While spinning from the specified rate slowly towards a resting position the SpinningWorldModel is in the PowerOff state. **PowerOn **—While spinning around the point picking up speed towards the specified rate it is considered to be in the PowerOn state. |
| **Messages ** | A SpinningWorldModel can receive the following messages: > **ATTACH **—Attaches the object specified in the message **DETACH **—Detach the object attached with the ATTACH message. **LOCK **—Locks the object. Once locked the object cannot be activated. **OFF **—If not already in the Off or PowerOff states, puts the WorldModel in the PowerOff state. **ON **—If not already in the On or PowerOn states, puts the WorldModel in the PowerOn state. **TRIGGER **—Toggles the state. Basically the same as when a player presses use against this object. If the WorldModel is in the On or PowerOn state it will immediately switch to PowerOff. If it’s in the Off or PowerOff state, then it will switch to PowerOn. **UNLOCK **—Unlocks the object so it can now be activated by the player and other messages. |
| **Options ** | A SpinningWorldModel has the following editable options: > **ForceMove **—When TRUE the SpinningWorldModel will rotate through the player and other objects. **Locked **—Toggles weather this object starts locked or not. **PlayerActivate **—Toggles if the Player can interact with the SpinningWorldModel or not. **RemainOn **—If this is TRUE the SpinningWorldModel will keep on spinning until directly told to turn off, either by a player or a message. **StartOn **—When TRUE the SpinningWorldModel will start spinning at load time **TriggerOff **—Toggles weather or not the player can directly turn a SpinningWorldModel Off. **Waveform **—Defines how the object Spins. |
| **Commands ** | A SpinningWorldModel can send the following commands when in the corresponding state: - **LockedComand ** - **OffCommand ** - **OnCommand ** - **PowerOffComand ** - **PowerOnCommand ** |
| **Sounds ** | A SpinningWorldModel can play these sounds when in the corresponding state. - **LockedSound ** - **OffSound ** - **OnSound ** - **PowerOffSound ** - **PowerOnSound ** |
| **Animated Lightmaps ** | SpinningWorldModel objects support animated light maps. |

[Top ](#top)

---

## RotatingDoor

This Door can rotate around a point a specified number of degrees around each axis. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. It can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This Door can also have an animated light map associated with it. Doors also have the ability to open another door they are linked to and they can open and close portals when they open and close.

Use RotatingDoor any place you want a Door that should swing like it is hinged to a wall. An office Door or a car Door are good examples of what these can be used for but of course there are many uses.

| **Creation ** | This object is created normally. Simply bind this object to a brush or group of brushes. To set its rotation properties just edit the vector labeled **RotationAngles **and either position the bound object where you would like to rotate around or link to a Point object positioned where you want the object to rotate around. Enter the number of degrees, positive or negative, around each axis you would like this object to rotate. Using the office door as an example you would create your brush in the closed position, bind a RotatingDoor to the brush, move the bound object to either the left or right edge of the brush, and then edit the vector (0.0 90.0 0.0). This office door will now open 90 degrees. If you want this door to open another door whenever it is activated to open simply use the **DoorLink **property’s **ObjectBrowser **to find the door name you would like. Typically Door1 will link to Door 2 and Door2 will link to Door1. This way doors right next to each other will open more naturally. If you want this door to control the opening and closing of a portal just type in the name of the portal brush you want in the **PortalName **property. |
| --- | --- |
| **Damage ** | This object can handle damage. Edit the **DamageProperties **subset to define behavior. |
| **Attachments ** | This object can handle attachments. Enter all the objects you want attached to the RotatingDoor, separated by a semicolon, in the **Attachments **property. A very good use for this would be a doorknob and a peephole. |
| **States ** | **Closed **—By default a RotatingDoor starts in the Closed position, and are considered Closed whenever in this position. **Closing **—While rotating from the Open position towards the Closed position the RotatingDoor is in the Closing state. **Open **—When the RotatingDoor is fully rotated to the degree amounts specified in the RotationAngles property, it is considered Open. **Opening **—While rotating towards the Open position it is considered to be in the Opening state. |
| **Messages ** | A RotatingDoor can receive the following messages: > **ATTACH **—Attaches the object specified in the message **DETACH **—Detach the object attached with the **ATTACH **message. **LOCK **—Locks the object. Once locked the object cannot be activated. **OFF **—If not already in the Closed or Closing states, puts the Door in the Closing state. **ON **—If not already in the Open or Opening states, puts the Door in the Opening state. **TRIGGER **—Toggles the state. Basically the same as when a player presses use against this object. If the Door is in the Open or Opening state it will immediately switch to Closing. If it’s in the Closed or Closing state, then it will switch to Opening. **UNLOCK **—Unlocks the object so it can now be activated by the player and other messages. |
| **Options ** | A RotatingDoor has the following editable options: > **AIActivate **—Toggles if AI can interact with the door or not. **ForceMove **—When TRUE the RotatingDoor will rotate through the player and other objects. **Locked **—Toggles weather this object starts locked or not. **PlayerActivate **—Toggles if the Player can interact with the RotatingDoor or not. **RemainOpen **—If this is TRUE the RotatingDoor will stay in the Open position until directly told to close, either by a player or a message. **RotateAway **—If TRUE the RotatingDoor will swing away from the player. **StartOpen **—When TRUE the RotatingDoor is in the Open position at load time. **TriggerClose **—Toggles weather or not the player can directly Close a RotatingDoor. **Waveform **—Defines how the object Rotates. |
| **Commands ** | A RotatingDoor can send the following commands when in the corresponding state: - **LockedComand ** - **ClosedCommand ** - **ClosingComand ** - **OpenCommand ** - **OpeningCommand ** |
| **Sounds ** | A RotatingDoor can play the following sounds when in the corresponding state: - **LockedSound ** - **ClosedSound ** - **ClosingSound ** - **OpenSound ** - **OpeningSound ** |
| **Animated Lightmaps ** | This object supports animated light maps. |

[Top ](#top)

---

## SlidingDoor

This Door can slide, or move, a specified number of units in a specified direction. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This Door can also have an animated light map associated with it. Doors also have the ability to open another door they are linked to and they can open and close portals when they open and close.

Use SlidingDoor any place you want a Door that should move in a specific direction. Sliding glass doors or those sliding Asian doors are a very good example of what these can be used for, but of course there are many uses.

| **Creation ** | This object is created normally. Simply bind this object to a brush or group of brushes. To set its movement properties, just edit the vector labeled **MoveDir **and set the distance you would like this SlidingDoor to move in the property labeled **MoveDist **. A **MoveDir **vector of (0.0 1.0 0.0 ) will move the SlidingDoor along its own local Y axis, typically straight up. This vector can be edited to point in any direction you want (0.5 0.5 0.0) will move the SlidingDoor diagonally along its X and Y axis. The **MoveDist **property is the distance the SlidingDoor will travel in DEdit units. Using the sliding glass door as an example, you would create your brush in the closed position, bind a SlidingDoor to the brush and then edit the **MoveDir **vector (1.0 0.0 0.0). Now specify how far you want the object to move by making **MoveDist **128.0. The sliding door will now slide 128 units out in its local X axis. If you want this door to open another door whenever it is activated, simply use the **DoorLink **property’s **ObjectBrowser **to find the door name you would like. Typically Door1 will link to Door 2 and Door2 will link to Door1, this way doors right next to each other will open more naturally. If you want this door to control the opening and closing of a portal, just type in the name of the portal brush you want in the **PortalName **property. |
| --- | --- |
| **Damage ** | This object can handle damage. Edit the **DamageProperties **subset to define behavior. |
| **Attachments ** | This object can handle attachments. Enter all the objects you want attached to the SlidingDoor, separated by a semicolon, in the **Attachments **property. A very good use for this would be a doorknob and a peephole. |
| **States ** | **Closed **—By default a SlidingDoor starts in the Closed position, and are considered Closed whenever in this position. **Closing **—While moving from the Open position towards the Closed position the SlidingDoor is in the Closing state. **Open **—When the SlidingDoor is fully moved to the distance specified in the MoveDist property, it is considered On. **Opening **—While moving towards the Open position it is considered to be in the Opening state. |
| **Messages ** | A SlidingDoor can receive the following messages: > **ATTACH **—Attaches the object specified in the message **DETACH **—Detach the object attached with the **ATTACH **message. **LOCK **—Locks the object. Once locked the object cannot be activated. **OFF **—If not already in the Closed or Closing states, puts the Door in the Closing state. **ON **—If not already in the Open or Opening states, puts the Door in the Opening state. **TRIGGER **—Toggles the state. Basically the same as when a player presses use against this object. If the Door is in the Open or Opening state it will immediately switch to Closing. If it’s in the Closed or Closing state, then it will switch to Opening. **UNLOCK **—Unlocks the object so it can now be activated by the player and other messages. |
| **Options ** | A SlidingDoor has the following editable options: > **AIActivate **—Toggles if AI can interact with the door or not. **ForceMove **—When TRUE the SlidingDoor will rotate through the player and other objects. **Locked **—Toggles weather this object starts locked or not. **PlayerActivate **—Toggles if the Player can interact with the SlidingDoor or not. **RemainOpen **—If this is TRUE the SlidingDoor will stay in the Open position until directly told to close, either by a player or a message. **StartOpen **—When TRUE the SlidingDoor is in the Open position at load time. **TriggerClose **—Toggles weather or not the player can directly Close a SlidingDoor. **Waveform **—Defines how the object Moves. |
| **Commands ** | A SlidingDoor can send the following commands when in the corresponding state. - **LockedComand ** - **ClosedCommand ** - **ClosingComand ** - **OpenCommand ** - **OpeningCommand ** |
| **Sounds ** | A SlidingDoor can play the following sounds when in the corresponding state. - **LockedSound ** - **ClosedSound ** - **ClosingSound ** - **OpeningSound ** - **OpenSound ** |
| **Animated Lightmaps ** | This object supports animated light maps. |

[Top ](#top)

---

## RotatingSwitch

This Switch can rotate around a point a specified number of degrees around each axis. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. It can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This Switch can also have an animated light map associated with it.

Use RotatingSwitch any place you want a Switch that should swing like it is hinged to the floor. Levers are a good example of what these can be used for but of course there are many uses.

| **Creation ** | This object is created normally. Simply bind this object to a brush or group of brushes. To set its rotation properties just edit the vector labeled **RotationAngles **and either position the bound object where you would like to rotate around or link to a Point object positioned where you want the object to rotate around. Enter the number of degrees, positive or negative, around each axis you would like this object to rotate. Using the lever as an example you would create your brush in the closed position, bind a RotatingSwitch to the brush, move the bound object to the bottom edge of the brush, and then edit the vector (0.0 0.0 -45.0). This lever will now rotate -45 degrees around its Z axis. |
| --- | --- |
| **Damage ** | This object can handle damage. Edit the **DamageProperties **subset to define behavior. |
| **Attachments ** | This object can handle attachments. Enter all the objects you want attached to the RotatingSwitch, separated by a semicolon, in the **Attachments **property. A good example of this would be a handle for the lever. |
| **States ** | **Off **—By default RotatingSwitches start in the Off position, and are considered Off while in this position. **On **—When the RotatingSwitch as fully rotated to the degree amounts specified in the RotationAngles property, it is considered On. **PowerOff **—While rotating from the On position towards the Off position the RotatingSwitch is in the PowerOff state. **PowerOn **—While rotating towards the On position it is considered to be in the PowerOn state. |
| **Messages ** | A RotatingSwitch can receive the following messages: > **ATTACH **—Attaches the object specified in the message **DETACH **—Detach the object attached with the ATTACH message. **LOCK **—Locks the object. Once locked the object cannot be activated. **OFF **—If not already in the Off or PowerOff states, puts the Switch in the PowerOff state. **ON **—If not already in the On or PowerOn states, puts the Switch in the PowerOn state. **TRIGGER **—Toggles the state. Basically the same as when a player presses use against this object. If the Switch is in the On or PowerOn state it will immediately switch to PowerOff. If it’s in the Off or PowerOff state, then it will switch to PowerOn. **UNLOCK **—Unlocks the object so it can now be activated by the player and other messages. |
| **Options ** | A RotatingSwitch has the following editable options: > **ForceMove **—When TRUE the RotatingSwitch will rotate through the player and other objects. **Locked **—Toggles weather this object starts locked or not. **PlayerActivate **—Toggles if the Player can interact with the RotatingSwitch or not. **RemainOn **—If this is TRUE the RotatingSwitch will stay in the On position until directly told to turn off, either by a player or a message. **StartOn **—When TRUE the RotatingSwitch is in the On position at load time **TriggerOff **—Toggles weather or not the player can directly turn a RotatingSwitch Off. **RotateAway **—If TRUE the RotatingSwitch will swing away from the player. **Waveform **—Defines how the object Rotates. |
| **Commands ** | A RotatingSwitch can send the following commands when in the corresponding state: - **LockedComand ** - **OffCommand ** - **OnCommand ** - **PowerOffComand ** - **PowerOnCommand ** |
| **Sounds ** | A RotatingSwitch can play the following sounds when in the corresponding state. - **LockedSound ** - **OffSound ** - **OnSound ** - **PowerOffSound ** - **PowerOnSound ** |
| **Animated Lightmaps ** | This object supports animated light maps. |

[Top ](#top)

---

## SlidingSwitch

This Switch can slide, or move, a specified number of units in a specified direction. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. It can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This WorldModel can also have an animated light map associated with it.

Use SlidingSwitch any place you want a Switch that should move in a specific direction. Push buttons are a good example of what these can be used for but of course there are many uses.

| **Creation ** | This object is created normally. Simply bind this object to a brush or group of brushes. To set its movement properties just edit the vector labeled **MoveDir **and set the distance you would like this SlidingSwitch to move to in the property labeled **MoveDist **. A **MoveDir **vector of (0.0 0.0 1.0 ) will move the SlidingSwitch along its own local Z axis, typically forward. This vector can be edited to point in any direction you want (0.5 0.5 0.0) will move the SlidingSwitch diagonally along its X and Y axis. The **MoveDist **property is the distance the SlidingSwitch will travel in DEdit units. Using the push button as an example you would create your brush in the closed position, bind a SlidingSwitch to the brush and then edit the **MoveDir **vector (0.0 0.0 1.0). Now specify how far you want the object to move by making **MoveDist **8.0. The push button will now slide 8 units out in its local Z axis. |
| --- | --- |
| **Damage ** | This object can handle damage. Edit the **DamageProperties **subset to define behavior. |
| **Attachments ** | This object can handle attachments. Enter all the objects you want attached to the SlidingSwitch, separated by a semicolon, in the **Attachments **property. |
| **States ** | **Off **—By default SlidingSwitches start in the Off position, and are considered Off while in this position. **On **—When the SlidingSwitch is fully moved to the distance specified in the **MoveDist **property, it is considered On. **PowerOff **—While moving from the On position towards the Off position the SlidingSwitch is in the PowerOff state. **PowerOn **—While moving towards the On position it is considered to be in the PowerOn state. |
| **Messages ** | A SlidingSwitch can receive the following messages: > **ATTACH **—Attaches the object specified in the message **DETACH **—Detach the object attached with the **ATTACH **message. **LOCK **—Locks the object. Once locked the object cannot be activated. **OFF **—If not already in the Off or PowerOff states, puts the Switch in the PowerOff state. **ON **—If not already in the On or PowerOn states, puts the Switch in the PowerOn state. **TRIGGER **—Toggles the state. Basically the same as when a player presses use against this object. If the Switch is in the On or PowerOn state it will immediately switch to PowerOff. If it’s in the Off or PowerOff state, then it will switch to PowerOn. **UNLOCK **—Unlocks the object so it can now be activated by the player and other messages. |
| **Options ** | A SlidingSwitch has the following editable options: > **ForceMove **—When TRUE the SlidingSwitch will move through the player and other objects. **Locked **—Toggles weather this object starts locked or not. **PlayerActivate **—Toggles if the Player can interact with the SlidingSwitch or not. **RemainOn **—If this is TRUE the SlidingSwitch will stay in the On position until directly told to turn off, either by a player or a message. **StartOn **—When TRUE the SlidingSwitch is in the On position at load time **TriggerOff **—Toggles weather or not the player can directly turn a SlidingSwitch Off. **Waveform **—Defines how the object moves. |
| **Commands ** | A SlidingSwitch can send the following commands when in the corresponding state: - **LockedComand ** - **OffCommand ** - **OnCommand ** - **PowerOffComand ** - **PowerOnCommand ** |
| **Sounds ** | A SlidingSwitch can play the following sounds when in the corresponding state: - **LockedSound ** - **OffSound ** - **OnSound ** - **PowerOffSound ** - **PowerOnSound ** |
| **Animated Lightmaps ** | This object supports animated light maps. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Appendix\Apnd-E\WorldMod.md)2006, All Rights Reserved.
