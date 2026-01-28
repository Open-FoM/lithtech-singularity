| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Creating World Models

WorldModels change the geometry that they’re bound with into an object that can be moved, destroyed and manipulated. Usually, brushes can never move or be changed in any way. WorldModels are the basis of things like doors, exploding walls and elevators. Some props may also be constructed out of WorldModels as well.

This section contains the following topics related to the creation of world models:

- [Volume Brushes ](#VolumeBrushes)
- [World Models ](#WorldModels)
- [Creation ](#Creation)
- [Damage ](#Damage)
- [Attachments ](#Attachments)
- [States ](#States)
- [Messages ](#Messages)
- [Options ](#Options)
- [Commands ](#Commands)
- [Sounds ](#Sounds)
- [Animated Lightmaps ](#AnimatedLightmaps)

---

## Volume Brushes

Volume brushes convert the brushes they’re bound to into spaces that affect players standing inside them. They’re the basis for things such as water, zero gravity rooms, poison gas and teleporters. In order to fill a pool with water, you would make an empty pool. Then you would create a brush that takes up the space of the water in the pool. Last, you would bind the brush to a volume simulation object with water properties.

There are many different types of objects and they tend to vary dramatically from game to game. It’s a very good idea to ask your game’s coders to carefully document their work and to write explanations on the use of their custom objects, since they are the ones with the greatest understanding of the specific objects used in your game. DEdit itself can provide help on properties on the fly using a file called Classhlp.BUT.

[Top ](#top)

---

## WorldModels

There are three basic types of WorldModel objects: **WorldModels **, **Doors **, and **Switches **. Each of these three basic types has different movement types as prefixes describing how they behave.

- **Rotating **means the objects will rotate around a point a specified number of degrees around each axis. For example, a common door.
- **Sliding **objects will move a specified number of units in a specified direction. An example of this is the behavior of a desk drawer.
- **Spinning **objects will spin around a point in a specified number of seconds on each axis. An example of this is the behavior of a ceiling fan.

Each movement type has identical properties and behaves exactly the same so when you are learning how to use a RotatingWorldModel you are also learning the RotatingSwitch and RotatingDoor. However, even though a RotatingWorldModel can be used exactly as a RotatingDoor you should NOT put a RotatingWorldModel where a Door should go, and vice versa. Use a Door only for Doors, use Switches only for Switches, and WorldModels for everything else. Game objects, specifically AI and Players, need to know if they are interacting with a Door, Switch or something else.

Most basic functionality is shared for every new WorldModel object. All of the new objects have common features and properties that you can edit. They are all created the same way, they all move and rotate based on their local coordinate system and they can all receive certain messages. Active WorldModels, that is any Rotating, Sliding or Spinning objects, can also send commands to other objects and play sounds depending on what state they are in.

For information on the WorldModel objects included with Jupiter, see [Appendix E: WorldModel Reference ](../Appendix/Apnd-E/WorldMod.md).

[Top ](#top)

---

## Creation

To create an instance of a WorldModel just bind a **WorldModel **object to a brush or a set of brushes. There are several property values you can edit to achieve the object you want.

**BlendModes **is a property drop down list containing several blend operations for your **WorldModel **object. WorldModels can have Additive blending, they can be Chromakeyed, or they can be Translucent. None is the default BlendMode for newly created WorldModel objects.

You can also edit several other properties such as visibility and solidity. If you have a fairly complex brush that the player will be walking on or moving around you may want to set the **BoxPhysics **flag to FALSE.

You can override the surface flags set by the texture mapped to the brush(es) by simply selecting the desired surface in the **SurfaceOverride **property dropdown list.

Once a WorldModel is positioned and behaves as you would like you can rotate it by selecting both the object and all brushes it is bound to then select Rotate Object in the right click menu in DEdit. You can rotate the WorldModel on all axes and in any direction. The WorldModel will now move or rotate with the same behavior around its own local coordinate system.

[Top ](#top)

---

## Damage

All WorldModels can handle damage. Each one has a subset property labeled **DamageProperties **. There are several different values and flags that can all be edited to define how the WorldModel behaves when damaged. The following list of **DamageProperties **describe the damage types available for each WorldModel:

>

**Armor **—Amount of armor that the WorldModel has when it is created.

**CanDamage **—Toggles whether the WorldModel can be damaged.

**CanHeal **—Toggles whether the WorldModel can be healed.

**CanRepair **—Toggles whether the WorldModel can be repaired.

**DamageMessage **—Command string that is sent to the object that caused the damage.

**DamageTriggerCounter **—How many times the object must be damaged before the damage message will be sent.

**DamageTriggerMessage **—Command string that is sent when the WorldModel receives damage from any source.

**DamageTriggerNumSends **—Specifies how many times the damage message will be sent.

**DamageTriggerTarget **—Name of the object that will receive the DamageTriggerMessage.

**DeathTriggerMessage **—Command string that is sent when the WorldModel has been destroyed.

**DeathTriggerTarget **—Name of the object that will receive the DeathTriggerMessage.

**HitPoints **—Number of hit points the WorldModel has when created.

**KillerMessage **—Command string is sent to the object that destroys this WorldModel.

**Mass **—Sets the mass of the WorldModel within the game.

**MaxArmor **—Max amount of armor the WorldModel can ever have.

**MaxHitPoints **—Max number of hit points the WorldModel can ever have.

**NeverDestroy **—Toggles whether the WorldModel can be destroyed.

**PlayerDeathTriggerMessage **—Command string that is sent when the player destroys the WorldModel.

**PlayerDeathTriggerTarget **—Name of the object that receives the PlayerDeathTriggerMessage.

[Top ](#top)

---

## Attachments

All WorldModels can have attachments (you can attach models, props, and other WorldModel objects). When attached to a WordlModel, objects will move and rotate with the WorldModel.

#### To have an object attach to a WorldModel when the game loads

1. Add the object you want to attach and position it in DEdit.
2. In the Attachments property field enter the name of the Object you want to attach.
3. Repeat the previous steps for each object you want to attach. You can have as many objects attached to a WorldModel as you want. List all the objects you want attached separated by a semicolon (for example: Prop1; Prop2; SlidingWorldModel0 ).

During runtime of the game you can attach and detach objects through messages.

[Top ](#top)

---

## States

All Active WorldModel objects (Rotating, Sliding, and Spinning) have different states they go through. These states are:

- Off
- On
- PowerOff
- PowerOn

Doors have similar states with different names:

- Closed
- Closing
- Open
- Opening

By default, this can be changed in the **Options **property subset. All objects start in the Off or Closed state.

When positioning and rotating the object in DEdit you are setting its Off position. By entering in values for the **Movement **or **Rotation **properties you are setting how and where the object will move to when fully On or Open.

Sliding WorldModel objects will be moving in the direction you specified while in the PowerOn state and when they have moved to the distance you specified they are considered On. While moving back to their original position and rotation they are in the PowerOff state and when they have reached their original position and are at rest they are considered Off.

Rotating and Sliding objects will stay still in their On positions and rotations, but SpinningWorldModels will continue to spin at the specified rotations per second while On.

[Top ](#top)

---

## Messages

All WorldModels can receive **ATTACH **and **DETACH **messages sent from other objects.

To send an **ATTACH **just send a message to a WorldModel like you would other objects and enter the name of the object you want to **ATTACH **:

>
```
msg WorldModelName (ATTACH Prop0);
```

Only one object can be attached at a time using this method. When another **ATTACH **message is sent to the same WorldModel the first attachment is detached and the new object is attached.

To detach the object you attached with an **ATTACH **message, send the object a **DETACH **message. **DETACH **takes no additional parameters.

>
```
msg WorldModelName DETACH
```

All Active WorldModel objects (Rotating, Sliding, and Spinning) can also receive messages to turn On and Off. Messages that Active WorldModels can receive are:

>

**LOCK **—Locks the object. Once locked the object cannot be activated.

**ON **—If not already in the On or PowerOn states, puts the WorldModel in the PowerOn state.

**OFF **—If not already in the Off or PowerOff states, puts the WorldModel in the PowerOff state.

**TRIGGER **—Toggles the state. Basically the same as when a player presses use against this object. If the WorldModel is in the On or PowerOn state it will immediately switch to PowerOff. If it’s in the Off or PowerOff state, then it will switch to PowerOn.

**TRIGGEROFF **—Same as OFF.

**TRIGGERON **—Same as ON.

**UNLOCK **—Unlocks the object so it can now be activated by the player and messages.

These messages take no other parameters so an example to toggle a SpinningWorldModels current state would be:

>
```
msg SpinningWorldModelName TRIGGER

```

[Top ](#top)

---

## Options

All Active WorldModel objects (Rotating, Sliding, and Spinning) have several options that can be changed in the Options property subset group. These options control how interactive the objects are and set some behavior.

>

**Locked **—When TRUE, will start the object off as being Locked. While Locked the object cannot turn on or off until it is unlocked through a message.

**PlayerActivate and AIActivate **—These let the object know who, if anybody, can turn them On or Close them. If these are FALSE then only other objects can interact with it by sending it messages. If PlayerActivate is TRUE then a player can trigger the object by pressing “use”.

**RemainOn **—If TRUE, will keep the object in its’ On state until told to turn off, either by the player or another object.

**StartOn **—When TRUE, puts the object in the PowerOn or Opening state as soon as the game loads.

**TriggerOff **—Toggles whether or not the object can be turned Off.

**Waveform **—A dropdown list of movement types that the object will use to rotate or move.

[Top ](#top)

---

## Commands

All Active WorldModel objects (Rotating, Sliding, and Spinning) can send commands based on what state they are in. There are 5 commands listed in the Commands property subset group:

- LockedComand
- OffCommand
- OnCommand
- PowerOffComand
- PowerOnCommand

Doors have similar commands with different names:

- ClosedCommand
- ClosingCommand
- OpenCommand
- OpeningCommand

Each command line can have multiple commands, separated by semicolons, entered and each one will be executed every time the WorldModel object first enters the corresponding state.

To have a command executed on an object as soon as it is activated, either by the player pressing “use” or by receiving an On message, simply enter the command in the PowerOnCommand or OpeningCommand line.

If you would like a command executed at the end of an object’s movement or rotation enter it in the OnCommand line.

The LockedCommand will be executed if the object is locked and a player or message tries to activate it.

[Top ](#top)

---

## Sounds

All Active WorldModel objects (Rotating, Sliding, and Spinning) can play sounds based on what state they are in. There are 5 sounds listed in the Sounds property subset group:

- LockedSound
- OffSound
- OnSound
- PowerOffSound
- PowerOnSound

Doors have similar sounds with different names:

- ClosedSound
- ClosingSound
- OpeningSound
- OpenSound

Other properties that can be edited in this subset group are the relative sound position, the radius the sound fades off to and whether or not the sounds will loop.

The **LockedSound **will never loop.

Each sound is a single .WAV file that is easily picked through a browser. The sounds will start playing every time the WorldModel object first enters the corresponding state.

To have a sound start playing every time the object starts to turn off or close enter it in the **PowerOffSound **or **ClosingSound **. The **LockedSound **will play if the object is locked and the player or a message tries to activate it.

[Top ](#top)

---

## Animated Lightmaps

All Active WorldModel objects (Rotating, Sliding, and Spinning) can have animated lightmaps associated with their movement or rotation.

#### To create animated lightmaps

1. Add a **KeyframerLight **object.
2. Set the properties of the **KeyframerLight **as desired.
3. Position it close to the **WorldModel **object you would like to lightmap. If it’s a directional light then make sure the light is pointed in the right direction.
4. Setting **ShadowMap **to FALSE typically looks much better.
5. Once the **KeyframerLight **is set just type the name of the **KeyframerLight **object in the **ShadowLights **property. You can enter up to 8 lights in the **ShadowLights **property separated by semicolons.

Enter the number of frames you would like have in the animation, up to 128. Sometimes less frames actually look better but experiment and use what works best. Remember that the more animations and the more frames in the animation is a real memory hog. Too many animated lightmaps is also a real frame rate killer!

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Dedit\WorldMod.md)2006, All Rights Reserved.
