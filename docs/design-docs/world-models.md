**TO2: WorldModels **

****

HYPERLINK \l "_Overview" Overview …………………………………………………………………………………1

HYPERLINK \l "_General" General ……………………………………………………………………………….…..1

HYPERLINK \l "_Creation" Creation ……………………………………………………………………………….….2

HYPERLINK \l "_Damage_(Top_|" Damage …………………………………………………………………………………..2

HYPERLINK \l "_Attachments" Attachments ……………………………………………………………………….……..3

HYPERLINK \l "_States" States ……………………………………………………………………………………..3

HYPERLINK \l "_Messages" Messages ………………………………………………………………………………...3

HYPERLINK \l "_Options" Options …………………………………………………………………………………...4

HYPERLINK \l "_Commands" Commands ……………………………………………………………………………….4

HYPERLINK \l "_Sounds" Sounds ……………………………………………………………………………………4

HYPERLINK \l "_Animated_Lightmaps" Animated Lightmaps …………………………………………………………………….4

HYPERLINK \l "_Objects" Objects ……………………………………………………………………………………5

HYPERLINK \l "_WorldModel__(Top" WorldModel

HYPERLINK \l "_RotatingWorldModel_(Top_|" RotatingWorldModel

HYPERLINK \l "_SlidingWorldModel_(Top_|" SlidingWorldModel

HYPERLINK \l "_SpinningWorldModel_(Top_|" SpinningWorldModel

HYPERLINK \l "_RotatingDoor__(Top" RotatingDoor

HYPERLINK \l "_SlidingDoor__(Top" SlidingDoor

HYPERLINK \l "_RotatingSwitch__(Top" RotatingSwitch

HYPERLINK \l "_SlidingSwitch__(Top" SlidingSwitch

****

**Overview **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

This document details the new WorldModel system created for The Operative 2, what features every object has and how each new object should be used. This paper is intended mainly for Level Designers as a reference tool but all are welcome to read and comment on its contents. There is also a HYPERLINK "..\\Development\\TO2\\Game\\Worlds\\EngineeringReference\\NewWorldModelObjects.dat" reference level in the EngineeringReference directory under Worlds that shows different things that can be achieved using the new objects. As changes are made to the system this document and the reference level will be updated to reflect any changes or additions.



From NOLF it was learned that using Door objects on everything that needed to be a world model was very prone to bugs. Moving and rotating the Doors, Switches and other WorldModels was also a painstaking task in NOLF. For TO2 we decided to rework the WorldModel objects so they are easier to use and will hopefully be less filled with bugs, when used correctly. A lot of the same features that were in the NOLF Door object can be found in the new TO2 WorldModel objects.



**General **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

The naming convention should be mentioned first. There are three basic types, WorldModels, Doors, and Switches. Each of these three basic types has different movement types as prefixes describing how they behave. “Rotating” means the objects will rotate around a point a specified number of degrees around each axis (much like the NOLF HingedDoor). “Sliding” objects will move a specified number of units in a specified direction (much like the NOLF Door), think of the behavior of a desk drawer. “Spinning” objects will spin around a point in a specified number of seconds on each axis (much like the NOLF RotatingWorldModel), think of a ceiling fans behavior. Each movement type has identical properties and behaves exactly the same so when you are learning how to use a RotatingWorldModel you are also learning the RotatingSwitch and RotatingDoor. **However, even though a RotatingWorldModel can be used exactly as a RotatingDoor you should NOT put a RotatingWorldModel where a Door should go, and vice versa **. Use a Door only for Doors, use Switches only for Switches, and WorldModels for everything else. Game objects, specifically AI and Players, need to know if they are interacting with a Door, Switch or something else.



Most basic functionality is shared for every new WorldModel object. All of the new objects have common features and properties that you can edit. They are all created the same way, they all move and rotate based on their local coordinate system and they can all receive certain messages. “Active” WorldModels, that is any Rotating, Sliding or Spinning objects, can also send commands to other objects and play sounds depending on what state they are in.



**Creation **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

To create an instance of a WorldModel just bind a WorldModel object to a brush or a set of brushes. There are several property values you can edit to achieve the object you want. BlendModes is a property drop down list containing several blend operations for your WorldModel object. WorldModels can have Additive blending, they can be Chromakeyed, or they can be Translucent. None is the default BlendMode for newly created WorldModel objects. You can also edit several other properties such as visibility and solidity. If you have a fairly complex brush that the player will be walking on or moving around you may want to set the BoxPhysics flag to FALSE. You can override the surface flags set by the texture mapped to the brush(s) by simply selecting the desired surface in the SurfaceOverride property dropdown list.



Once a WorldModel is positioned and behaves as you would like you can rotate it by selecting both the object and all brushes it is bound to then select Rotate Object in the right click menu in DEdit. You can rotate the WorldModel on all axis and in any direction. The WorldModel will now move or rotate with the same behavior around its own local coordinate system.



**Damage **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

All WorldModels can handle damage. Each one has a subset property labeled DamageProperties. There are several different values and flags that can all be edited to define how the WorldModel behaves when damaged. Here is a list of the properties that are included in the DamageProperties subset and what each on means:

HitPoints - Number of hit points the WorldModel has when created.

MaxHitPoints - Max number of hit points the WorldModel can ever have.

Armor - Amount of armor that the WorldModel has when it is created.

MaxArmor - Max amount of armor the WorldModel can ever have.

DamageTriggerCounter - How many times the object must be damaged before the damage message will be sent.

DamageTriggerTarget - Name of the object that will receive the DamageTriggerMessage.

DamageTriggerMessage - Command string that is sent when the WorldModel receives damage from any source.

DamageTriggerNumSends - Specifies how many times the damage message will be sent.

DamageMessage - Command string that is sent to the object that caused the damage.

DeathTriggerTarget - Name of the object that will receive the DeathTriggerMessage.

DeathTriggerMessage - Command string that is sent when the WorldModel has been destroyed.

PlayerDeathTriggerTarget - Name of the object that receives the PlayerDeathTriggerMessage.

PlayerDeathTriggerMessage - Command string that is sent when the player destroys the WorldModel.

KillerMessage - Command string is sent to the object that destroys this WorldModel.

CanHeal - Toggles whether the WorldModel can be healed.

CanRepair - Toggles whether the WorldModel can be repaired.

CanDamage - Toggles whether the WorldModel can be damaged.

Mass - Sets the mass of the WorldModel within the game.

NeverDestroy - Toggles whether the WorldModel can be destroyed.



**Attachments **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

All WorldModels can have attachments (you can attach models, props, and other WorldModel objects) much like Doorknobs in the NOLF Doors. When attached to a WordlModel, objects will move and rotate with the WorldModel. To have an object attach to a WorldModel when the game loads, add the object you want to attach and position it in DEdit. Enter the name of the Object you want to attach in the Attachments property field, you can have as many objects attached to a WorldModel as you want. List all the objects you want attached separated by a semicolon (ex. Prop1; Prop2; SlidingWorldModel0). During runtime of the game you can attach and detach objects through messages.



**States **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

All “Active” WorldModel objects (Rotating, Sliding, and Spinning) have different states they go through. These states are: PowerOn, On, PowerOff, Off. Doors have similar states with different names: Opening, Open, Closing, Closed. By default, this can be changed in the Options property subset, all objects start in the ‘Off’ or ‘Closed’ state. When positioning and rotating the object in DEdit you are setting its ‘Off’ position. By entering in values for the Movement or Rotation properties you are setting how and where the object will move to when fully ‘On’ or ‘Open’. Sliding WorldModel objects will be moving in the direction you specified while in the ‘PowerOn’ state and when they have moved to the distance you specified they are considered ‘On’. While moving back to their original position and rotation they are in the ‘PowerOff’ state and when they have reached their original position and are at rest they are considered ‘Off’. Rotating and Sliding objects will stay still in their ‘On’ positions and rotations, but SpinningWorldModels will continue to spin at the specified rotations per second while ‘On’.



**Messages **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

All WorldModels can receive “ATTACH” and “DETACH” messages sent from other objects. To send an ATTACH just send a message to a WorldModel like you would other objects and enter the name of the object you want to ATTACH:

(Ex. msg WorldModelName (ATTACH Prop0)). Only one object can be attached at a time using this method. When another ATTACH message is sent to the same WorldModel the first attachment is detached and the new object is attached. To detach the object you attached with an ATTACH message, send the object a DETACH message. DETACH takes no additional parameters.

(Ex. msg WorldModelName DETACH)



All “Active” WorldModel objects (Rotating, Sliding, and Spinning) can also receive messages to

turn ‘On’ and ‘Off’. Messages that “Active” WorldModels can receive are:

ON - If not already in the On or PowerOn states, puts the WorldModel in the PowerOn state.

TRIGGERON - same as ON

OFF - If not already in the Off or PowerOff states, puts the WorldModel in

the PowerOff state.

TRIGGEROFF - same as OFF

TRIGGER - Toggles the state. Basically the same as when a player presses use against this object. If the WorldModel is in the ‘On’ or ‘PowerOn’

state it will immediately switch to ‘PowerOff’. If it’s in the ‘Off’ or ‘PowerOff’ state, then it will switch to ‘PowerOn’.

LOCK - Locks the object. Once locked the object cannot be activated.

UNLOCK - Unlocks the object so it can now be activated by the player and messages.



These messages take no other parameters so an example to toggle a SpinningWorldModels current state would be: msg SpinningWorldModelName TRIGGER



**Options **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

All “Active” WorldModel objects (Rotating, Sliding, and Spinning) have several options that can be changed in the Options property subset group. These control how interactive the objects are and set some behavior. PlayerActivate and AIActivate, these let the object know who, if anybody, can turn them ‘On’ or Close them. If these are FLASE then only other objects can interact with it by sending it messages, if PlayerActivate is TRUE then a player can trigger the object by pressing “use”. StartOn, when TRUE, puts the object in the PowerOn or Opening state as soon as the game loads. TriggerOff toggles weather or not the object can be turned off. RemainOn, if TRUE, will keep the object in its’ ‘On’ state until told to turn off, either by the player or another object. Locked, when TRUE, will start the object off as being locked. While Locked the object cannot turn on or off until it is unlocked through a message. Waveform is a dropdown list of movement types that the object will use to rotate or move.

****

**Commands **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

All “Active” WorldModel objects (Rotating, Sliding, and Spinning) can send commands based on what state they are in. There are 5 commands listed in the Commands property subset group: OnCommand, OffCommand, PowerOnCommand, PowerOffComand, LockedComand. Doors have similar commands with different names: OpenCommand, ClosedCommand, OpeningCommand, ClosingCommand. Each command line can have multiple commands, separated by semicolons, entered and each one will be executed every time the WorldModel object first enters the corresponding state. To have a command executed as soon as on object is activated, either by the player pressing “use” or by receiving an ‘On’ message, simply enter the command in the PowerOnCommand or OpeningCommand line. If you would like a command executed at the end of an objects movement or rotation enter it in the OnCommand line. The LockedCommand will be executed if the object is locked and a player or message tries to activate it.



**Sounds **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

All “Active” WorldModel objects (Rotating, Sliding, and Spinning) can play sounds based on what state they are in. There are 5 sounds listed in the Sounds property subset group: PowerOnSound, OnSound, PowerOffSound, OffSound, LockedSound. Doors have similar sounds with different names: OpeningSound, OpenSound, ClosingSound and ClosedSound. Other properties that can be edited in this subset group are the relative sound position, the radius the sound fades off to and weather or not the sounds will loop. The LockedSound will never loop. Each sound is a single .wav file that is easily picked through a browser. The sounds will start playing every time the WorldModel object first enters the corresponding state. To have a sound start playing every time the object starts to turn off or close enter it in the PowerOffSound or ClosingSound. The LockedSound will play if the object is locked and the player or a message tries to activate it.



**Animated Lightmaps **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

All “Active” WorldModel objects (Rotating, Sliding, and Spinning) can have animated lightmaps associated with their movement or rotation. To create animated lightmaps you must first add a KeyframerLight object. Set the properties of the KeyframerLight as desired, position it close to the WorldModel object you would like to lightmap, if it’s a directional light then make sure the light is pointed in the right direction. Setting ShadowMap to FALSE typically looks much better. Once the KeyframerLight is set just type the name of the KeyframerLight object in the ShadowLights property. You can enter up to 8 lights in the ShadowLights property separated by semicolons. Now just enter the number of frames you would like have in the animation, up to 128. Sometimes less frames actually look better but experiment and use what works best. **Remember that the more animations and the more frames in the animations is a real memory hog. Too many animated lightmaps is also a real frame rate killer! **



**Objects **( **HYPERLINK \l "_top" **Top )

HYPERLINK \l "_WorldModel__(Top" WorldModel

HYPERLINK \l "_RotatingWorldModel_(Top_|" RotatingWorldModel

HYPERLINK \l "_SlidingWorldModel_(Top_|" SlidingWorldModel

HYPERLINK \l "_SpinningWorldModel_(Top_|" SpinningWorldModel

HYPERLINK \l "_RotatingDoor__(Top" RotatingDoor

HYPERLINK \l "_SlidingDoor__(Top" SlidingDoor

HYPERLINK \l "_RotatingSwitch__(Top" RotatingSwitch

HYPERLINK \l "_SlidingSwitch__(Top" SlidingSwitch



**WorldModel **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

**Features: **

This is the basic “non-active” WorldModel object. It does not move or rotate, does not play any sounds and cannot execute any commands. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected.

****

**Use: **

Any place where you need a stationary WorldModel, this is the object you should use. An example would be a desk that you want to damage or to be able to handle attachments. This object is also very well suited for keyframing. If you want a keyframed WorldModel object you should use this object.



**HYPERLINK \l "_Creation_(Top_|" ****Creation ****: **

This object is created normally. Simply bind this object to a brush or groups of brushes and then edit the properties as fit.



**HYPERLINK \l "_Damage_(Top_|" ****Damage ****: **

This object can handle damage. Edit the DamageProperties subset to define behavior.

****

**HYPERLINK \l "_Attachments_(Top_|" ****Attachments ****: **

This object can handle attachments. Enter all the objects you want attached to the WorldModel, separated by a semicolon, in the Attachments property.

****

**HYPERLINK \l "_States_(Top_|" ****States ****: **

None – This object does not move or rotate so there is no need for states.

****

**HYPERLINK \l "_Messages_(Top_|" ****Messages ****: **

WorldModels can receive these messages.

ATTACH - Attaches the object specified in the message

DETACH - Detach the object attached with the ATTACH message.



**HYPERLINK \l "_Options_(Top_|" ****Options ****: **

None – The player and AI cannot activate this object. It has no movement or rotation properties.



**HYPERLINK \l "_Commands_(Top_|" ****Commands ****: **

None – This object cannot execute any commands.



**HYPERLINK \l "_Sounds_(Top_|" ****Sounds ****: **

None – This object cannot play any sounds.



**HYPERLINK \l "_Animated_Lightmaps_(Top" ****Animated Lightmaps **:

This object does not support animated light maps.



**RotatingWorldModel **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

**Features: **

This WorldModel can rotate around a point a specified number of degrees around each axis. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This WorldModel can also have an animated light map associated with it.



**Use: **

Any place you want a WorldModel that should swing like it is hinged to a wall. Kitchen cabinets or window shutters are a good example of what these can be used for but of course there are many uses.



**HYPERLINK \l "_Creation_(Top_|" ****Creation ****: **

This object is created normally. Simply bind this object to a brush or group of brushes. To set its rotation properties just edit the vector labeled RotationAngles and either position the bound object where you would like to rotate around or link to a Point object positioned where you want the object to rotate around. Enter the number of degrees, positive or negative, around each axis you would like this object to rotate. Using the kitchen cabinet as an example you would create your brush in the closed position, bind a RotatingWorldModel to the brush, move the bound object to either the left or right edge of the brush, and then edit the vector (0.0 140.0 0.0). This cabinet door will now open out 140 degrees.



**HYPERLINK \l "_Damage_(Top_|" ****Damage ****: **

This object can handle damage. Edit the DamageProperties subset to define behavior.

****

**HYPERLINK \l "_Attachments_(Top_|" ****Attachments ****: **

This object can handle attachments. Enter all the objects you want attached to the RotatingWorldModel, separated by a semicolon, in the Attachments property.

****

**HYPERLINK \l "_States_(Top_|" ****States ****: **

On - When the RotatingWorldModel as fully rotated to the degree amounts

specified in the RotationAngles property, it is considered ‘On’.

PowerOn - While rotating towards the ‘On’ position it is considered to be in the

‘PowerOn’ state.

Off - By default RotatingWorldModels start in the ‘Off’ position, and are

considered ‘Off’ while in this position.

PowerOff - While rotating from the ‘On’ position towards the ‘Off’ position the

RotatingWorldModel is in the ‘PowerOff’ state.

****

**HYPERLINK \l "_Messages_(Top_|" ****Messages ****: **

A RotatingWorldModel can receive these messages.

ATTACH - Attaches the object specified in the message

DETACH - Detach the object attached with the ATTACH message.

ON - If not already in the On or PowerOn states, puts the WorldModel in the PowerOn state.

OFF - If not already in the Off or PowerOff states, puts the WorldModel in

the PowerOff state.

TRIGGER - Toggles the state. Basically the same as when a player presses use against this object. If the WorldModel is in the ‘On’ or ‘PowerOn’

state it will immediately switch to ‘PowerOff’. If it’s in the ‘Off’ or ‘PowerOff’ state, then it will switch to ‘PowerOn’.

LOCK - Locks the object. Once locked the object cannot be activated.

UNLOCK - Unlocks the object so it can now be activated by the player and other messages.



**HYPERLINK \l "_Options_(Top_|" ****Options ****: **

A RotatingWorldModel can have these options edited.

PlayerActivate - Toggles if the Player can interact with the RotatingWorldModel or not.

StartOn - When TRUE the RotatingWorldModel is in the ‘On’ position at load time

TriggerOff -Toggles weather or not the player can directly turn a

RotatingWorldModel ‘Off’.

RemainOn - If this is TRUE the RotatingWorldModel will stay in the ‘On’ position

until directly told to turn off, either by a player or a message.

ForceMove - When TRUE the RotatingWorldModel will rotate through the player and

other objects.

Locked - Toggles weather this object starts locked or not.

RotateAway - If TRUE the RotatingWorldModel will swing away from the player.

Waveform - Defines how the object Rotates.



**HYPERLINK \l "_Commands_(Top_|" ****Commands ****: **

A RotatingWorldModel can send these commands when in the corresponding state.

OnCommand

OffCommand

PowerOnCommand

PowerOffComand

LockedComand



**HYPERLINK \l "_Sounds_(Top_|" ****Sounds ****: **

A RotatingWorldModel can play these sounds when in the corresponding state.

PowerOnSound

OnSound

PowerOffSound

OffSound

LockedSound





**HYPERLINK \l "_Animated_Lightmaps_(Top" ****Animated Lightmaps **:

This object supports animated light maps.





**SlidingWorldModel **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

**Features: **

This WorldModel can slide, or move, a specified number of units in a specified direction. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This WorldModel can also have an animated light map associated with it.



**Use: **

Any place you want a WorldModel that should move in a specific direction. Desk drawers are a good example of what these can be used for but of course there are many uses.



**HYPERLINK \l "_Creation_(Top_|" ****Creation ****: **

This object is created normally. Simply bind this object to a brush or group of brushes. To set its movement properties just edit the vector labeled MoveDir and set the distance you would like this SlidingWorldModel to move to in the property labeled MoveDist. A MoveDir vector of (0.0 1.0 0.0 ) will move the SlidingWorldModel along its own local Y axis, typically straight up. This vector can be edited to point in any direction you want (0.5 0.5 0.0) will move the SlidingWorldModel diagonally along its X and Y axis. The MoveDist property is the distance the SlidingWorldModel will travel in DEdit units. Using the desk drawer as an example you would create your brush in the closed position, bind a SlidingWorldModel to the brush and then edit the MoveDir vector (0.0 0.0 1.0). Now specify how far you want the object to move by making MoveDist 64.0. The desk drawer will now slide 64 units out in its local Z axis.



**HYPERLINK \l "_Damage_(Top_|" ****Damage ****: **

This object can handle damage. Edit the DamageProperties subset to define behavior.



**HYPERLINK \l "_Attachments_(Top_|" ****Attachments ****: **

This object can handle attachments. Enter all the objects you want attached to the SlidingWorldModel, separated by a semicolon, in the Attachments property.

****

**HYPERLINK \l "_States_(Top_|" ****States ****: **

On - When the SlidingWorldModel is fully moved to the distance

specified in the MoveDist property, it is considered ‘On’.

PowerOn - While moving towards the ‘On’ position it is considered to be in the

‘PowerOn’ state.

Off - By default SlidingWorldModels start in the ‘Off’ position, and are

considered ‘Off’ while in this position.

PowerOff - While moving from the ‘On’ position towards the ‘Off’ position the

SlidingWorldModel is in the ‘PowerOff’ state.

****

**HYPERLINK \l "_Messages_(Top_|" ****Messages ****: **

A SlidingWorldModel can receive these messages.

ATTACH - Attaches the object specified in the message

DETACH - Detach the object attached with the ATTACH message.

ON - If not already in the On or PowerOn states, puts the WorldModel in the PowerOn state.

OFF - If not already in the Off or PowerOff states, puts the WorldModel in

the PowerOff state.

TRIGGER - Toggles the state. Basically the same as when a player presses use against this object. If the WorldModel is in the ‘On’ or ‘PowerOn’

state it will immediately switch to ‘PowerOff’. If it’s in the ‘Off’ or ‘PowerOff’ state, then it will switch to ‘PowerOn’.

LOCK - Locks the object. Once locked the object cannot be activated.

UNLOCK - Unlocks the object so it can now be activated by the player and other messages.



**HYPERLINK \l "_Options_(Top_|" ****Options ****: **

A SlidingWorldModel can have these options edited.

PlayerActivate - Toggles if the Player can interact with the SlidingWorldModel or not.

StartOn - When TRUE the SlidingWorldModel is in the ‘On’ position at load time

TriggerOff -Toggles weather or not the player can directly turn a

SlidingWorldModel ‘Off’.

RemainOn - If this is TRUE the SlidingWorldModel will stay in the ‘On’ position

until directly told to turn off, either by a player or a message.

ForceMove - When TRUE the SlidingWorldModel will move through the player and

other objects.

Locked - Toggles weather this object starts locked or not.

Waveform - Defines how the object moves.



**HYPERLINK \l "_Commands_(Top_|" ****Commands ****: **

A SlidingWorldModel can send these commands when in the corresponding state.

OnCommand

OffCommand

PowerOnCommand

PowerOffComand

LockedComand



**HYPERLINK \l "_Sounds_(Top_|" ****Sounds ****: **

A SlidingWorldModel can play these sounds when in the corresponding state.

PowerOnSound

OnSound

PowerOffSound

OffSound

LockedSound





**HYPERLINK \l "_Animated_Lightmaps_(Top" ****Animated Lightmaps **:

This object supports animated light maps.





**SpinningWorldModel **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

**Features: **

This WorldModel can spin around a point a in the specified amount of time to make one rotation around each axis. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This WorldModel can also have an animated light map associated with it.



**Use: **

Any place you want a WorldModel that should spin around a central point. Ceiling fans or a Rolodex are good examples of what these can be used for but of course there are many uses.



**HYPERLINK \l "_Creation_(Top_|" ****Creation ****: **

This object is created normally. Simply bind this object to a brush or group of brushes. To set its rotation properties just edit the vector labeled RotationAngles and either position the bound object where you would like to spin around or link to a Point object positioned where you want the object to spin around. Enter the amount of time, in seconds to make one rotation around each axis, you would like this object to spin. Using the ceiling fan as an example you would create your brush in the closed position, bind a SpinningWorldModel to the brush, move the bound object to the center of the object (by default it is), and then edit the RotationAngles vector (0.0 4.0 0.0). This fan will now spin around its Y axis once every 4 seconds.

****

**HYPERLINK \l "_Damage_(Top_|" ****Damage ****: **

This object can handle damage. Edit the DamageProperties subset to define behavior.

****

**HYPERLINK \l "_Attachments_(Top_|" ****Attachments ****: **

This object can handle attachments. Enter all the objects you want attached to the RotatingWorldModel, separated by a semicolon, in the Attachments property.

****

**HYPERLINK \l "_States_(Top_|" ****States ****: **

On - When the SpinningWorldModel is spinning at the desired rate

specified in the RotationAngles property, it is considered ‘On’.

PowerOn - While spinning around the point picking up speed towards the specified rate it is considered to be in the ‘PowerOn’ state.

Off - By default RotatingWorldModels start in the ‘Off’ position, and are

considered ‘Off’ when they are no longer spinning.

PowerOff - While spinning from the specified rate slowly towards a resting position the SpinningWorldModel is in the ‘PowerOff’ state.

****

**HYPERLINK \l "_Messages_(Top_|" ****Messages ****: **

A SpinningWorldModel can receive these messages.

ATTACH - Attaches the object specified in the message

DETACH - Detach the object attached with the ATTACH message.

ON - If not already in the On or PowerOn states, puts the WorldModel in the PowerOn state.

OFF - If not already in the Off or PowerOff states, puts the WorldModel in

the PowerOff state.

TRIGGER - Toggles the state. Basically the same as when a player presses use against this object. If the WorldModel is in the ‘On’ or ‘PowerOn’

state it will immediately switch to ‘PowerOff’. If it’s in the ‘Off’ or ‘PowerOff’ state, then it will switch to ‘PowerOn’.

LOCK - Locks the object. Once locked the object cannot be activated.

UNLOCK - Unlocks the object so it can now be activated by the player and other messages.



**HYPERLINK \l "_Options_(Top_|" ****Options ****: **

A SpinningWorldModel can have these options edited.

PlayerActivate - Toggles if the Player can interact with the SpinningWorldModel or not.

StartOn - When TRUE the SpinningWorldModel will start spinning at load time

TriggerOff -Toggles weather or not the player can directly turn a

SpinningWorldModel ‘Off’.

RemainOn - If this is TRUE the SpinningWorldModel will keep on spinning until directly told to turn off, either by a player or a message.

ForceMove - When TRUE the SpinningWorldModel will rotate through the player and

other objects.

Locked - Toggles weather this object starts locked or not.

Waveform - Defines how the object Spins.



**HYPERLINK \l "_Commands_(Top_|" ****Commands ****: **

A SpinningWorldModel can send these commands when in the corresponding state.

OnCommand

OffCommand

PowerOnCommand

PowerOffComand

LockedComand



**HYPERLINK \l "_Sounds_(Top_|" ****Sounds ****: **

A SpinningWorldModel can play these sounds when in the corresponding state.

PowerOnSound

OnSound

PowerOffSound

OffSound

LockedSound





**HYPERLINK \l "_Animated_Lightmaps_(Top" ****Animated Lightmaps **:

This object supports animated light maps.





**RotatingDoor **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

**Features: **

This Door can rotate around a point a specified number of degrees around each axis. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This Door can also have an animated light map associated with it. Doors also have the ability to open another door they are linked to and they can open and close portals when they open and close.



**Use: **

Any place you want a Door that should swing like it is hinged to a wall. An office Door or a car Door are good examples of what these can be used for but of course there are many uses.



**HYPERLINK \l "_Creation_(Top_|" ****Creation ****: **

This object is created normally. Simply bind this object to a brush or group of brushes. To set its rotation properties just edit the vector labeled RotationAngles and either position the bound object where you would like to rotate around or link to a Point object positioned where you want the object to rotate around. Enter the number of degrees, positive or negative, around each axis you would like this object to rotate. Using the office door as an example you would create your brush in the closed position, bind a RotatingDoor to the brush, move the bound object to either the left or right edge of the brush, and then edit the vector (0.0 90.0 0.0). This office door will now open 90 degrees.

If you want this door to open another door whenever it is activated to open simply use the DoorLink property’s ObjectBrowser to find the door name you would like. Typically Door1 will link to Door 2 and Door2 will link to Door1, this way doors right next to each other will open more naturally. If you want this door to control the opening and closing of a portal just type in the name of the portal brush you want in the PortalName property.

****

**HYPERLINK \l "_Damage_(Top_|" ****Damage ****: **

This object can handle damage. Edit the DamageProperties subset to define behavior.

****

**HYPERLINK \l "_Attachments_(Top_|" ****Attachments ****: **

This object can handle attachments. Enter all the objects you want attached to the RotatingDoor, separated by a semicolon, in the Attachments property. A very good use for this would be a doorknob and a peephole.

****

**HYPERLINK \l "_States_(Top_|" ****States ****: **

Open - When the RotatingDoor is fully rotated to the degree amounts

specified in the RotationAngles property, it is considered ‘Open’.

Opening - While rotating towards the ‘Open’ position it is considered to be in the

‘Opening’ state.

Closed - By default a RotatingDoor starts in the ‘Closed’ position, and are

considered ‘Closed’ whenever in this position.

Closing - While rotating from the ‘Open’ position towards the ‘Closed’ position the

RotatingDoor is in the ‘Closing’ state.

****

**HYPERLINK \l "_Messages_(Top_|" ****Messages ****: **

A RotatingDoor can receive these messages.

ATTACH - Attaches the object specified in the message

DETACH - Detach the object attached with the ATTACH message.

ON - If not already in the Open or Opening states, puts the Door in the Opening state.

OFF - If not already in the Closed or Closing states, puts the Door in

the Closing state.

TRIGGER - Toggles the state. Basically the same as when a player presses use against this object. If the Door is in the ‘Open’ or ‘Opening’

state it will immediately switch to ‘Closing’. If it’s in the ‘Closed’ or ‘Closing’ state, then it will switch to ‘Opening’.

LOCK - Locks the object. Once locked the object cannot be activated.

UNLOCK - Unlocks the object so it can now be activated by the player and other messages.



**HYPERLINK \l "_Options_(Top_|" ****Options ****: **

A RotatingDoor can have these options edited.

PlayerActivate - Toggles if the Player can interact with the RotatingDoor or not.

AIActivate - Toggles if AI can interact with the door or not.

StartOpen - When TRUE the RotatingDoor is in the ‘Open’ position at load time.

TriggerClose -Toggles weather or not the player can directly Close a RotatingDoor.

RemainOpen - If this is TRUE the RotatingDoor will stay in the ‘Open’ position

until directly told to close, either by a player or a message.

ForceMove - When TRUE the RotatingDoor will rotate through the player and

other objects.

Locked - Toggles weather this object starts locked or not.

RotateAway - If TRUE the RotatingDoor will swing away from the player.

Waveform - Defines how the object Rotates.



**HYPERLINK \l "_Commands_(Top_|" ****Commands ****: **

A RotatingDoor can send these commands when in the corresponding state.

OpenCommand

ClosedCommand

OpeningCommand

ClosingComand

LockedComand



**HYPERLINK \l "_Sounds_(Top_|" ****Sounds ****: **

A RotatingDoor can play these sounds when in the corresponding state.

OpeningSound

OpenSound

ClosingSound

ClosedSound

LockedSound

****

**HYPERLINK \l "_Animated_Lightmaps_(Top" ****Animated Lightmaps **:

This object supports animated light maps.





**SlidingDoor **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

**Features: **

This Door can slide, or move, a specified number of units in a specified direction. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This Door can also have an animated light map associated with it. Doors also have the ability to open another door they are linked to and they can open and close portals when they open and close.



**Use: **

Any place you want a Door that should move in a specific direction. Sliding glass doors or those sliding Asian doors are a very good example of what these can be used for but of course there are many uses.



**HYPERLINK \l "_Creation_(Top_|" ****Creation ****: **

This object is created normally. Simply bind this object to a brush or group of brushes. To set its movement properties just edit the vector labeled MoveDir and set the distance you would like this SlidingDoor to move to in the property labeled MoveDist. A MoveDir vector of (0.0 1.0 0.0 ) will move the SlidingDoor along its own local Y axis, typically straight up. This vector can be edited to point in any direction you want (0.5 0.5 0.0) will move the SlidingDoor diagonally along its X and Y axis. The MoveDist property is the distance the SlidingDoor will travel in DEdit units. Using the sliding glass door as an example you would create your brush in the closed position, bind a SlidingDoor to the brush and then edit the MoveDir vector (1.0 0.0 0.0). Now specify how far you want the object to move by making MoveDist 128.0. The sliding door will now slide 128 units out in its local X axis. If you want this door to open another door whenever it is activated to open simply use the DoorLink property’s ObjectBrowser to find the door name you would like. Typically Door1 will link to Door 2 and Door2 will link to Door1, this way doors right next to each other will open more naturally. If you want this door to control the opening and closing of a portal just type in the name of the portal brush you want in the PortalName property.



**HYPERLINK \l "_Damage_(Top_|" ****Damage ****: **

This object can handle damage. Edit the DamageProperties subset to define behavior.



**HYPERLINK \l "_Attachments_(Top_|" ****Attachments ****: **

This object can handle attachments. Enter all the objects you want attached to the SlidingDoor, separated by a semicolon, in the Attachments property. A very good use for this would be a doorknob and a peephole.



****

**HYPERLINK \l "_States_(Top_|" ****States ****: **

Open - When the SlidingDoor is fully moved to the distance

specified in the MoveDist property, it is considered ‘On’.

Opening - While moving towards the ‘Open’ position it is considered to be in the

‘Opening’ state.

Closed - By default a SlidingDoor starts in the ‘Closed’ position, and are

considered ‘Closed’ whenever in this position.

Closing - While moving from the ‘Open’ position towards the ‘Closed’ position the

SlidingDoor is in the ‘Closing’ state.

****

**HYPERLINK \l "_Messages_(Top_|" ****Messages ****: **

A SlidingDoor can receive these messages.

ATTACH - Attaches the object specified in the message

DETACH - Detach the object attached with the ATTACH message.

ON - If not already in the Open or Opening states, puts the Door in the Opening state.

OFF - If not already in the Closed or Closing states, puts the Door in

the Closing state.

TRIGGER - Toggles the state. Basically the same as when a player presses use against this object. If the Door is in the ‘Open’ or ‘Opening’

state it will immediately switch to ‘Closing’. If it’s in the ‘Closed’ or ‘Closing’ state, then it will switch to ‘Opening’.

LOCK - Locks the object. Once locked the object cannot be activated.

UNLOCK - Unlocks the object so it can now be activated by the player and other messages.



**HYPERLINK \l "_Options_(Top_|" ****Options ****: **

A SlidingDoor can have these options edited.

PlayerActivate - Toggles if the Player can interact with the SlidingDoor or not.

AIActivate - Toggles if AI can interact with the door or not.

StartOpen - When TRUE the SlidingDoor is in the ‘Open’ position at load time.

TriggerClose -Toggles weather or not the player can directly Close a SlidingDoor.

RemainOpen - If this is TRUE the SlidingDoor will stay in the ‘Open’ position

until directly told to close, either by a player or a message.

ForceMove - When TRUE the SlidingDoor will rotate through the player and

other objects.

Locked - Toggles weather this object starts locked or not.

Waveform - Defines how the object Moves.



**HYPERLINK \l "_Commands_(Top_|" ****Commands ****: **

A SlidingDoor can send these commands when in the corresponding state.

OpenCommand

ClosedCommand

OpeningCommand

ClosingComand

LockedComand



**HYPERLINK \l "_Sounds_(Top_|" ****Sounds ****: **

A SlidingDoor can play these sounds when in the corresponding state.

OpeningSound

OpenSound

ClosingSound

ClosedSound

LockedSound



**HYPERLINK \l "_Animated_Lightmaps_(Top" ****Animated Lightmaps **:

This object supports animated light maps.





**RotatingSwitch **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

**Features: **

This Switch can rotate around a point a specified number of degrees around each axis. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This Switch can also have an animated light map associated with it.





**Use: **

Any place you want a Switch that should swing like it is hinged to the floor. Levers are a good example of what these can be used for but of course there are many uses.



**HYPERLINK \l "_Creation_(Top_|" ****Creation ****: **

This object is created normally. Simply bind this object to a brush or group of brushes. To set its rotation properties just edit the vector labeled RotationAngles and either position the bound object where you would like to rotate around or link to a Point object positioned where you want the object to rotate around. Enter the number of degrees, positive or negative, around each axis you would like this object to rotate. Using the lever as an example you would create your brush in the closed position, bind a RotatingSwitch to the brush, move the bound object to the bottom edge of the brush, and then edit the vector (0.0 0.0 -45.0). This lever will now rotate -45 degrees around it’s Z axis.

****

**HYPERLINK \l "_Damage_(Top_|" ****Damage ****: **

This object can handle damage. Edit the DamageProperties subset to define behavior.

****

**HYPERLINK \l "_Attachments_(Top_|" ****Attachments ****: **

This object can handle attachments. Enter all the objects you want attached to the RotatingSwitch, separated by a semicolon, in the Attachments property. A good example of this would be a handle for the lever.

****

**HYPERLINK \l "_States_(Top_|" ****States ****: **

On - When the RotatingSwitch as fully rotated to the degree amounts

specified in the RotationAngles property, it is considered ‘On’.

PowerOn - While rotating towards the ‘On’ position it is considered to be in the

‘PowerOn’ state.

Off - By default RotatingSwitches start in the ‘Off’ position, and are

considered ‘Off’ while in this position.

PowerOff - While rotating from the ‘On’ position towards the ‘Off’ position the

RotatingSwitch is in the ‘PowerOff’ state.

****

**HYPERLINK \l "_Messages_(Top_|" ****Messages ****: **

A RotatingSwitch can receive these messages.

ATTACH - Attaches the object specified in the message

DETACH - Detach the object attached with the ATTACH message.

ON - If not already in the On or PowerOn states, puts the Switch in the PowerOn state.

OFF - If not already in the Off or PowerOff states, puts the Switch in

the PowerOff state.

TRIGGER - Toggles the state. Basically the same as when a player presses use against this object. If the Switch is in the ‘On’ or ‘PowerOn’

state it will immediately switch to ‘PowerOff’. If it’s in the ‘Off’ or ‘PowerOff’ state, then it will switch to ‘PowerOn’.

LOCK - Locks the object. Once locked the object cannot be activated.

UNLOCK - Unlocks the object so it can now be activated by the player and other messages.



**HYPERLINK \l "_Options_(Top_|" ****Options ****: **

A RotatingSwitch can have these options edited.

PlayerActivate - Toggles if the Player can interact with the RotatingSwitch or not.

StartOn - When TRUE the RotatingSwitch is in the ‘On’ position at load time

TriggerOff -Toggles weather or not the player can directly turn a

RotatingSwitch ‘Off’.

RemainOn - If this is TRUE the RotatingSwitch will stay in the ‘On’ position

until directly told to turn off, either by a player or a message.

ForceMove - When TRUE the RotatingSwitch will rotate through the player and

other objects.

Locked - Toggles weather this object starts locked or not.

RotateAway - If TRUE the RotatingSwitch will swing away from the player.

Waveform - Defines how the object Rotates.



**HYPERLINK \l "_Commands_(Top_|" ****Commands ****: **

A RotatingSwitch can send these commands when in the corresponding state.

OnCommand

OffCommand

PowerOnCommand

PowerOffComand

LockedComand



**HYPERLINK \l "_Sounds_(Top_|" ****Sounds ****: **

A RotatingSwitch can play these sounds when in the corresponding state.

PowerOnSound

OnSound

PowerOffSound

OffSound

LockedSound

****

**HYPERLINK \l "_Animated_Lightmaps_(Top" ****Animated Lightmaps **:

This object supports animated light maps.





**SlidingSwitch **( **HYPERLINK \l "_top" **Top **| HYPERLINK \l "_Objects" **Objects )

**Features: **

This Switch can slide, or move, a specified number of units in a specified direction. The player can interact with this object or it can be controlled only through other objects. Sounds can be played and commands can be executed when certain states are reached. Like all of the new objects this can handle Attachments, can have BlendModes, has Damage properties, and a surface type can be selected. This WorldModel can also have an animated light map associated with it.



**Use: **

Any place you want a Switch that should move in a specific direction. Push buttons are a good example of what these can be used for but of course there are many uses.



**HYPERLINK \l "_Creation_(Top_|" ****Creation ****: **

This object is created normally. Simply bind this object to a brush or group of brushes. To set its movement properties just edit the vector labeled MoveDir and set the distance you would like this SlidingSwitch to move to in the property labeled MoveDist. A MoveDir vector of (0.0 0.0 1.0 ) will move the SlidingSwitch along its own local Z axis, typically forward. This vector can be edited to point in any direction you want (0.5 0.5 0.0) will move the SlidingSwitch diagonally along its X and Y axis. The MoveDist property is the distance the SlidingSwitch will travel in DEdit units. Using the push button as an example you would create your brush in the closed position, bind a SlidingSwitch to the brush and then edit the MoveDir vector (0.0 0.0 1.0). Now specify how far you want the object to move by making MoveDist 8.0. The push button will now slide 8 units out in its local Z axis.



**HYPERLINK \l "_Damage_(Top_|" ****Damage ****: **

This object can handle damage. Edit the DamageProperties subset to define behavior.



**HYPERLINK \l "_Attachments_(Top_|" ****Attachments ****: **

This object can handle attachments. Enter all the objects you want attached to the SlidingSwitch, separated by a semicolon, in the Attachments property.

****

**HYPERLINK \l "_States_(Top_|" ****States ****: **

On - When the SlidingSwitch is fully moved to the distance

specified in the MoveDist property, it is considered ‘On’.

PowerOn - While moving towards the ‘On’ position it is considered to be in the

‘PowerOn’ state.

Off - By default SlidingSwitches start in the ‘Off’ position, and are

considered ‘Off’ while in this position.

PowerOff - While moving from the ‘On’ position towards the ‘Off’ position the

SlidingSwitch is in the ‘PowerOff’ state.

****

**HYPERLINK \l "_Messages_(Top_|" ****Messages ****: **

A SlidingSwitch can receive these messages.

ATTACH - Attaches the object specified in the message

DETACH - Detach the object attached with the ATTACH message.

ON - If not already in the On or PowerOn states, puts the Switch in the PowerOn state.

OFF - If not already in the Off or PowerOff states, puts the Switch in

the PowerOff state.

TRIGGER - Toggles the state. Basically the same as when a player presses use against this object. If the Switch is in the ‘On’ or ‘PowerOn’

state it will immediately switch to ‘PowerOff’. If it’s in the ‘Off’ or ‘PowerOff’ state, then it will switch to ‘PowerOn’.

LOCK - Locks the object. Once locked the object cannot be activated.

UNLOCK - Unlocks the object so it can now be activated by the player and other messages.



**HYPERLINK \l "_Options_(Top_|" ****Options ****: **

A SlidingSwitch can have these options edited.

PlayerActivate - Toggles if the Player can interact with the SlidingSwitch or not.

StartOn - When TRUE the SlidingSwitch is in the ‘On’ position at load time

TriggerOff -Toggles weather or not the player can directly turn a

SlidingSwitch ‘Off’.

RemainOn - If this is TRUE the SlidingSwitch will stay in the ‘On’ position

until directly told to turn off, either by a player or a message.

ForceMove - When TRUE the SlidingSwitch will move through the player and

other objects.

Locked - Toggles weather this object starts locked or not.

Waveform - Defines how the object moves.



**HYPERLINK \l "_Commands_(Top_|" ****Commands ****: **

A SlidingSwitch can send these commands when in the corresponding state.

OnCommand

OffCommand

PowerOnCommand

PowerOffComand

LockedComand



**HYPERLINK \l "_Sounds_(Top_|" ****Sounds ****: **

A SlidingSwitch can play these sounds when in the corresponding state.

PowerOnSound

OnSound

PowerOffSound

OffSound

LockedSound



**HYPERLINK \l "_Animated_Lightmaps_(Top" ****Animated Lightmaps **:

This object supports animated light maps.





****

****

****
