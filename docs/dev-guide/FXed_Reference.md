| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# FxED

****

## Overview

This document details the new system for creating special effects for The Operative 2. It covers how to use the FxED application to create a group of FX and details the properties of each individual FX. I will go over some common uses for each FX and which FX should be used to create a specific special effect. This paper is intended as a reference tool for anyone with a creative mind who would like to create some cool looking special effects but all are welcome to read and comment on its contents. As changes are made to FxED, the individual FX and the system as a whole this document will be updated to reflect any changes or additions.

Creating special effects for previous games meant having an engineer and an artist sit down together to create and FX in code and then expose a few properties for tweaking in a text file. This severely limited the types of special effects we could have and limited the behavior of the FX we did have. For TO2 we decided to use the FxED application to create and edit all of our special effects. With FxED you can specify the length of a whole group of FX, the length of an individual FX, the scale, color and alpha at any point in time for an individual FX and you can even create a motion path for each individual FX to follow without setting up lots of key framers in DEdit.

## General

You will see the word FX and GroupFX used in this document. FX refers to the individual objects that are considered special effects. A FX can be a ParticleSystemFX, SpriteFX, or even a NullFX. A GroupFX are a collection of FX. A GroupFX may contain one or many FX. When working with FxED you are creating GroupFX by adding FX to them.

Each FX has a set of properties that you can edit. Some of these properties are common between all the FX but most are specific to the individual FX. I will explain how to use each property of each FX later on.

## Getting Started

FxED is the application used to create the GroupFX that will be used in the game. When FxED runs for the first time on your computer you will have to fill out a dialog box of information telling FxED where to look for resources and the file containing the FX. If you wish to run the game from within FxED you will need to edit the “App to execute…” to be the full path name of your Lithtech.exe file. You can also enter in optional information into the command line if you wish to run the game in a window or load a specific level. You must fill out the “Location of resource file…” box. This tells FxED where to look for the resources such as .ltb’s and .spr’s, this should be the full path of your Game directory. The “Location and name of clientfx.fxd…” box also needs to be filled out in order to get the list of FX you can use. This should be the full path form the “Location of resource file…” box with “\clientfx.fxd” added to the end. FxED is now ready for you to work with and will never ask you for this information again. If you made a mistake in entering in the info or would like to change some options for the command line you can do so by selecting “Edit Game Info” form the File menu.

You will now want to open the file that contains all the GroupFX for the game. Click on the open icon and open the clientfx.fxf file found on the ClientFX directory of your Game directory. You will now have a list of GroupFX that you can open and edit.

## Creating A GroupFX

To create a new GroupFX simply click on the “new” icon and a blank GroupFX will appear. By default all GroupFX start off with a Phase Length of 10000. Phase Length is the lifetime of the GroupFX in milliseconds so each GroupFX is 10 seconds long by default. To change this, just click on the button that says 10000 and enter in a new time. To start adding FX to this GroupFX you will first need to add a Track. Tracks are holders for Keys. Keys are the specific individual FX. Each Key needs to be placed on a Track, you can have multiple Keys per Track or you can have each Track contain only one Key. Once you have added a Track by right clicking in the gray area of the GroupFX and selecting Add Track you will need to add a Key. Right click on an empty space in a Track and select Add Key, a popup box will appear with a list of FX you can add for this Key. Select the type of FX you would like to add and it will then appear on your Track. You can now move this Key around your Track so you can set when you would like this FX to begin playing. You can also lengthen the FX by grabbing its edges and growing or shrinking the Key to the desired length. To have the FX play for the whole duration of the GroupFX right click on the Key and select Expand Key from the menu. If you have many FX that are of the same type you may want to differentiate them from each other by giving them each a unique name. To do this right click on the key and select “Name Key” from the popup menu and enter in a custom name for this Key.

Now that you have your Key set to the desired length and start time you can edit the properties of the FX by right clicking on the Key and selecting Edit Key. A popup dialog box appears with several different properties listed for you to edit. From the same right click menu you can select options to edit the scale of the FX, the color of the FX and create motion for the FX. Not all FX types expose the ability to edit all of these options. A list of FX and properties that you can edit are listed below.

## Color Keys

For FX that allow you to create and edit color keys you access this interface by right clicking on a Key in the GroupFX and select “Edit Color Keys” from the menu. This will bring up a dialog box with a large colored bar numbered from 0 to the lifetime of the Key. The full time of the FX is represented in the bar along the bottom the, the alpha of the FX at a particular time is represented on the left side. From here you can create new color keys or edit the two preexisting keys. There are always at least two color keys per FX, one for the very beginning of the FX and one for the end of the FX. By default these two keys are set to the color white and are fully opaque. To create a new key, simply click in the color bar at the time you wish the new key to be and at the alpha you want the new key to be. You are then prompted to choose a color for the key. After a key is created you can move it around to the time and alpha desired by left clicking the key dragging it to the desired values. You can change the color of a color key by right clicking on one and choosing a color from the color menu. To delete a key simply select it by left clicking and then click the Delete Key button. You cannot delete the two preexisting beginning and end keys. If you create a set of color keys that you wish to use for other FX you can save them by clicking on the “Add To Favorites” button, you can select from a list of favorite color keys by selecting the choose button.

## Scale Keys

For FX that allow you to create and edit scale keys you access this interface by right clicking on a Key in the GroupFX and select “Edit Scale Keys” from the menu. This will bring up a dialog box with a large white bar numbered from 0 to the lifetime of the FX. The full lifetime of the FX is represented in the bar at the bottom and the Scale of the FX at a particular time is represented on the left side, ranging from Min Scale to Max Scale. From here you can create new scale keys or edit the two preexisting keys. There are always at least two scale keys per FX, one for the very beginning of the FX and one for the end of the FX. By default these two keys are set to a scale of 1. To change the Min and Max scale for this FX click on the numbered buttons for Min Scale and Max Scale and then enter a new value. To create new scale keys, click in the white bar at the time and scale you desire, you may move the key to the desired time and scale after creating by clicking and dragging the key. To delete a key, right click on it and then select “delete key” from the menu. You cannot delete the two preexisting beginning and end keys. If you wish to start over again click the “Reset” button. This will delete all but the two beginning and end keys and set them to be at 10% of the Max Scale value. If you create a set of scale keys that you wish to use for other FX you can save them by clicking on the “Add To Favorites” button, you can select from a list of favorite scale keys by selecting the choose button.

## FX

## ParticleSystem

### Features

This is probably the most used FX in the list. The particle system emits individual particles based on the properties that you fill out. This FX allows you to edit it’s Color Keys, Scale Keys and Motion Keys. The particles are always facing the camera and will never rotate. You can specify the particles to use a texture or a sprite. The Color Keys and Scale Keys for this FX are per particle, not per system. This means the particle will interpolate between its scale and color keys based on its own lifetime.

****

### Use

The ParticleSystems can be used almost anywhere for anything. They can be used as smoke from a steam pipe, fire, sparks and smoke form a gun and many other possibilities. You can set them up to be an impact FX and emit a puff of smoke when a bullet hits a wall, or as bubbles in the water.

### Color Keys

This FX allows you to edit its color keys. The Color Keys for a Particle System are per particle. So every particle will use the color keys based on their own lifespan.

### Scale Keys

This FX allows you to edit its Scale Keys. The Scale Keys for a Particle System are per particle. So every particle will use the scale keys based on their own lifespan.

****

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### Sprite

The ParticleSystem can use an .spr file as its particles and each particle will loop through the textures in the sprite. To select a ,spr file click the button and browse through the Resource Browser and select the desired file.

#### Texture

If no Sprite is given, the ParticleSystem can use a single .dtx as its file for the particles to display. To select a .dtx file click the button and select the file through the Resource Browser.

#### EmissionInterval

This controls the length of time, in seconds, between emissions of the system. Typically this is set low so the system is constantly emitting particles, however if you want particle to be emitted in bursts raise the value.

#### ParticlesPerEmission

Every time the system emits particles, based on the “EmissionInterval”, it will emit this many particles.

#### GravityAcceleration

This controls the speed and direction the particles will float or sink. A negative value will make the particles sink to the ground and a positive value will float the particles up towards the sky. Gravity is always in relation to the world, no matter what the systems rotation is.

#### MinParticleLifeSpan

Each particle will last for at least this amount of time, in seconds.

#### MaxParticleLifeSpan

Each particle will last no longer than this amount of time, in seconds. The life span of each particle is a random number between the Min/MaxParticleLifeSpan. If you want every particle to last the same amount of time, making these exactly the same.

#### MinRadius

This is the minimum distance from the center of the system that the particles will be emitted.

#### MaxRadius

This is the maximum distance from the center of the system that the particles will be emitted.

#### EmissionPlane

This is the normal vector of the plane that the particles are emitted from. By default this is a plane parallel to the systems Up vector. If you are using a ParticleSystem as an impact FX this should be set to 0.0 0.0 1.0.

#### MinParticleVelocity

This controls the minimum speed and direction that each particle will have when they are emitted. The velocity is relative to the system so if you never want particles to be emitted in the downward direction of the system, make the Y component positive. For impact FX the Z component should be positive so particles never get emitted into the surface they hit.

#### MaxParticleVelocity

This controls the maximum speed and direction that each particle will have when they are emitted.

#### Type

A dropdown list box of predefined emission types for the system:

- Sphere – The particles will emit in all directions around the center of the system a distance no less than MinRadius from the center and no more than MaxRadius from the center.
- Point - The particles will emit from the same random point on the emission plane at a distance of no less than MinRadius from the center and no more than MaxRadius from the center. If you wish to have the particles emit from the direct center of the system make Min/MaxRadius 0.0
- Circle - The particles will emit in a circle on the emission plane at a distance of no less than MinRadius from the center and no more than MaxRadius From the center. Playing with Min/MaxRadius can yield an emission pattern of a circle, doughnut, or ring.

#### Bounce

If set to Yes, each particle will collide with and bounce off geometry and solid objects in a realistic manner. NOTE: This can dramatically decrease frames and slow the game. This should be used sparingly and only with systems that have few active particles.

#### FlipRenderingOrder

By default the newest particle particles are drawn first, over the older particles in the system. You can change the order so older particles are drawn over newer ones by setting this to yes.

#### BlendMode

Depending on the texture or sprite used you may wish to set the system to use Additive or Multiply blending.

#### MoveParticlesWithSystem

If a particle system is moving or rotating, the particles will move and rotate with the system. You can override this behavior by setting this property to No. For speed reasons, if a system is staying still (ie. Not moving or rotating ), keep this value set to Yes.

## Sprite

### Features

The Sprite FX displays a sprite specified through a .spr file. The sprite can rotate, be flat against a wall, or always face the camera.

****

### Use

There are many uses for a sprite, one of the most common is to put a rotation on it and use it as a smoke or gas cloud. You can also use it as a smoke puff coming from the barrel of a gun.

### Color Keys

This FX allows you to edit its color keys. The Sprite’s color and alpha will change over time by interpolating between the color keys.

### Scale Keys

This FX allows you to edit its Scale Keys. The Sprite’s scale will change over time by interpolating between the scale keys.

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### Sprite

This is the sprite, .spr, file used for the Sprite FX. To select a ,spr file click the button and browse through the Resource Browser and select the desired file.

#### Normal

This is the normal the sprite will be facing when set to face along the normal. By default this is set to be the forward vector of the sprite.

#### Facing

A dropdown list of presets that tell how the sprite will be oriented.

- CameraFacing – The sprite will always be oriented so its forward is facing the camera.
- AlongNormal – The sprite will face in the direction specified by the Normal Property.
- ParentAlign – If the FX has a parent then the sprite will have the same rotation as the parent object.

#### BlendMode

Depending on the sprite used you may wish to set it to use Additive or Multiply blending.

****

#### DisableZ

When this is set to Yes the sprite will look as though it is being drawn over everything else.

#### DisableLight

This controls whether or not dynamic lights affect the color of the sprite.

****

## Null

### Features

This FX is used solely to give some random motion to other FX within a GroupFX. Nothing gets drawn for this FX; its only purpose is to keep track of a position for other FX to use. Other FX, a single FX or multiple FX, should motion link to this FX to give them a position to follow.

****

### Use

Other FX should motion link to this FX. It should not be used as a stand alone FX. You can motion link any FX to this Null FX to give them a random motion. PolyTrails should motion link to this FX to create sparks and trails. You can link multiple FX within a GroupFX to this FX so it gives them a since of motion together.

### Color Keys

This FX does not allow you to edit its color keys.

### Scale Keys

This FX does not allow you to edit its Scale Keys.

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### GravityAcceleration

This controls the speed and direction the FX will float or sink. A negative value will make the FX sink to the ground and a positive value will float the FX up towards the sky. Gravity is always in relation to the world, no matter what the FX rotation is.

#### MinVelocity

This controls the minimum speed and direction the FX will have in relation to its self and not the world.

#### MaxVelocity

This controls the maximum speed and direction the FX will have. The velocity is determined by getting a random value between the MinVelocity and MaxVelocity so each time the FX is played the motion will be different.

#### Bounce

If set to Yes, the FX will collide and bounce off geometry and solid objects in a realistic manner.

****

## LTBModel

### Features

This FX allows you to display a model that can fade in and out, scale, rotate and change the default animation length.

****

### Use

The LTBModel can be used as a chunk of something flying through the air, a dead body or you can set it up so many LTBModels are used as a gib FX.

### Color Keys

This FX allows you to edit its color keys. The Model’s color and alpha will change over time by interpolating between the color keys. Alpha will only work on models with an additive RenderStyle associated with it.

### Scale Keys

This FX allows you to edit its Scale Keys. The Model’s scale will change over time by interpolating between the scale keys.

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### Model

This is the model, .ltb, file used for the LTBModel FX. To select an .ltb file click the button and browse through the Resource Browser and select the desired file.

****

#### Skin 0-4

The skin properties are used to texture the model. Each skin should correspond to the texture index of the models textures set in model edit. To select a skin, click the button and select the .dtx or .spr you wish to use.

#### Normal

This is the normal the model will be facing when set to face along the normal. By default this is set to be the forward vector of the model.

#### Facing

A dropdown list of presets that tell how the sprite will be oriented.

- CameraFacing – The model will always be oriented so its forward is facing the camera.
- AlongNormal – The model will face in the direction specified by the Normal Property.
- ParentAlign – If the FX has a parent then the model will have the same rotation as the parent object.

****

#### OverrideAniLength

You can manually set the length of the animation or let the length of the FX determine the length by setting this to Yes.

#### AniLength

If OverrideAniLength is set to Yes and this is non-zero then this length, in seconds, will be used to set the length of the animation. If this is 0.0 and OverrideAniLength is set to Yes, then the lifespan of the FX is used to set the length of the animation.

#### RenderStyle 0-3

The renderstyle properties are used to set renderstyles on the model. Each renderstyle should correspond to the renderstyle index of the model set in model edit.

****

## DynaLight

### Features

This FX creates a Dynamic light with the color and scale specified. The color, scale and Intensity of the light can change over time.

****

### Use

The DynaLight FX can be used to show something is bright and emitting light. A good example of this is for a MuzzleFlash or for a fire or explosion.

### Color Keys

This FX allows you to edit its color keys. Intensity of the light is controlled through the color keys by setting the Alpha. Fully Opaque is fully bright and Fully Translucent is No light. The Light’s color and intensity will change over time by interpolating between the color keys.

### Scale Keys

This FX allows you to edit its Scale Keys. The Light’s scale will change over time by interpolating between the scale keys. The scale is in DEdit units so for this FX the scale should be at least 100 to even show up. Because changing the scale of a dynamic light forces the engine to recalculate the lightmaps, it is better to fade a DynaLight in and out using the intensity scale. And it looks better to J

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### Flicker

This will give the light a slight flicker feel to it by slightly changing the color and intensity.

****

## PlaySound

### Features

This FX allows you to play a sound at anytime during the GroupFX. You can control the radius, volume and whether or not the sound loops.

****

### Use

Any time you want a specific sound to play like a fire crackling, or a steam vent you can use this to play a sound.

### Color Keys

This FX doesn’t allow you to edit its color keys.

****

### Scale Keys

This FX doesn’t allow you to edit its scale keys.

****

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### Sound

This is the file, .wav, used as the sound to play. To select a sound, click the button and select the .wav you wish to use.

#### Loop

If this is set to Yes, then the sound will loop for the duration of the GroupFX.

#### PlayLocal

If set to Yes, the sound will fell as though its playing in the players head.

#### InnerRadius

If the player is within this distance from the center of the FX they will hear the sound at full volume.

#### OuterRadius

If the player is between the OuterRadius and the InnerRadius they will hear an interpolated volume for the sound. If outside this distance the player will not hear the sound.

#### Volume

This is the full volume of the sound heard within the InnerRadius.

#### Priority

How important is this sound. If there are too many sounds playing at once the lowest priority sounds are not played.

#### PitchShift

This is a multiplier to the playback frequency of the sound, which will shift its pitch. If you had a sound recorded at 22,000 Hz and set its pitch shift to 1.1f, then the output sound would be 24,2000 Hz and make the sound have a higher pitch.

****

## CamJitter

### Features

This FX will shake the camera around as thought the player is in an earthquake.

****

### Use

This FX is good for simulating a shaky ground. When an explosion occurs it is a nice effect to shake the camera.

### Color Keys

This FX does not allow you to edit its color keys.

### Scale Keys

This FX allows you to edit its Scale Keys. The Scale Keys for a CamJitter FX controls how intense the shaking is. This allows you to fade the shaking off rather than just abruptly stopping it.

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### InnerRadius

If the player is within this distance from the center of the FX they will feel the shaking at full force.

#### OuterRadius

If the player is between the OuterRadius and the InnerRadius they will feel an interpolated shake. If outside this distance the player will not feel the shake.

****

## WonkyFX

### Features

This FX plays with the FOV of the camera to create a wavy, watery effect. The cameras X, and Y FOV increases and decreases.

****

### Use

The WonkyFX probably wouldn’t be used very much and might only be useful for underwater type effects, or hallucination, type effects.

### Color Keys

This FX does not allow you to edit its Color keys.

### Scale Keys

This FX does not allow you to edit its Scale keys.

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### xMultiplier

This controls the amount of fluctuation in the xFOV. Increasing this value increases how far the xFOV stretches.

#### yMultiplier

This controls the amount of fluctuation in the yFOV. Increasing this value increases how far the yFOV stretches.

#### InnerRadius

If the player is within this distance from the center of the FX they will feel the wobble effect at full force.

#### OuterRadius

If the player is between the OuterRadius and the InnerRadius they will feel an interpolated wobble. If outside this distance the player will not feel the wobble.

****

## PolyTrail

### Features

This FX will draw a series of polygons following a path to create a motion trail behind the position the FX is following. This FX should be motion linked to another FX to give it more of a moving feel or attached to a node or socket of a model to give it a moving feel.

****

### Use

Motion link a PolyTrail FX to a NullFX and you can create sparks from a bullet hitting metal, or electrical sparks from the end of a downed power line. Attach this FX to a grenade or rocket to produce a motion trail or even a smoke trail behind it. Attach this FX to a socket on a model, like a sword, to show the path of motion.

### Color Keys

This FX allows you to edit its color keys. The Color Keys for a PolyTrail are per trail section and not the trail as a whole. So the section closest to the head will be the color of the first key and the tail section will be the color of the last key.

### Scale Keys

This FX does not allow you to edit its Scale keys.

****

****

### Properties

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### Texture

The PolyTrail can use a .dtx as its file for the trail to display. To select a .dtx file click the button and select the file through the Resource Browser.

#### TrailWidth

This is the width of the trail. Make this small for effects like sparks.

#### WidthStyle

A dropdown list how the trail is drawn:

- Constant – The trail is always at the constant width set by TrailWidth.
- SmallToBig – The trail starts small and ends big.
- SmallToSmall – The trail starts small, is big in the middle, and is small at the end again.
- BigToSmall – The trail starts big and ends small.

#### TrailLen

This is the Length of the trail given by number of segments. For a straight line trial this should be 1 but for a very curvy trail this should be much highr.

#### SectionLifespan

This represents the lifetime of each trail segment in seconds.

#### UAdd

An offset for the texture coordinates.

#### SectionInterval

The time spans between trail segments, in seconds. Every time this interval is hit a new trail segment is used.

#### BlendMode

A drop down list of various blend mode operations that can be performed to get the desired effect.

#### AlphaTest

A drop down list of various alpha tests to perform when determining weather or not to draw the trail.

#### ColorOp

A drop down list of various color operations such as Additive and Multiply to perform when determining how to draw the trail.

#### FillMode

A drop down list with two options for filling the trail. Fill just draws the trail normally and Wireframe draws lines to outline the polygons of the trail.

****

## PlayRandomSound

### Features

Just like the PlaySound FX except this FX will let you play a random sound from a selected set of sounds. You provide the base sound and then it randomly grabs sound with that base and a number appended at the end.

****

### Use

If there are several sounds available for an effect and you don’t want to use just one for fear of being redundant this is what you should use. This FX is good to use if a single GroupFX is used and played quite often to give it a more random and non set feel.

### Color Keys

This FX doesn’t allow you to edit its color keys.

****

### Scale Keys

This FX doesn’t allow you to edit its scale keys.

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### Sound

This is the file, .wav, used as the base sound to play. To select a sound, click the button and select the .wav you wish to use.

#### NumRand

This is how many sounds there are to use and the sound will be played by getting a random number between 0 and NumRand.

#### Loop

If this is set to Yes, then the sound will loop for the duration of the GroupFX.

#### PlayLocal

If set to Yes, the sound will fell as though its playing in the players head.

#### InnerRadius

If the player is within this distance from the center of the FX they will hear the sound at full volume.

#### OuterRadius

If the player is between the OuterRadius and the InnerRadius they will hear an interpolated volume for the sound. If outside this distance the player will not hear the sound.

#### Volume

This is the full volume of the sound heard within the InnerRadius.

#### Priority

How important is this sound. If there are too many sounds playing at once the lowest priority sounds are not played.

#### PitchShift

This is a multiplier to the playback frequency of the sound, which will shift its pitch. If you had a sound recorded at 22,000 Hz and set its pitch shift to 1.1f, then the output sound would be 24,2000 Hz and make the sound have a higher pitch.

****

## SpriteSystem

### Features

This FX is almost identical to the ParticleSystem FX except that the Sprites can rotate about there own axis. The Sprite system emits individual sprites based on the properties that you fill out. This FX allows you to edit it’s Color Keys, Scale Keys and Motion Keys. The Color Keys and Scale Keys for this FX are per sprite, not per system. This means the sprite will interpolate between its scale and color keys based on its own lifetime.

****

### Use

The SpriteSystem can be used almost anywhere that a Particle system can be used for and more. The major difference of the Sprite system is that the individual sprites can rotate which can create a since of movement in smoke and gas clouds. Typically ParticleSystems look and better and give better performance but for things like gas clouds and smoke puffs, adding a rotation makes all the difference in the world.

### Color Keys

This FX allows you to edit its color keys. The Color Keys for a Sprite System are per sprite. So every sprite will use the color keys based on their own lifespan.

### Scale Keys

This FX allows you to edit its Scale Keys. The Scale Keys for a Sprite System are per Sprite. So every sprite will use the scale keys based on their own lifespan.

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### Sprite

The SpriteSystem can use a .spr file as its sprite and each sprite will loop through the textures in the sprite. To select a ,spr file click the button and browse through the Resource Browser and select the desired file.

#### EmissionInterval

This controls the length of time, in seconds, between emissions of the system. Typically this is set low so the system is constantly emitting sprites; however if you want sprites to be emitted in bursts, raise the value.

#### ParticlesPerEmission

Every time the system emits sprites, based on the “EmissionInterval”, it will emit this many sprites.

#### MinSpriteRotation

The minimum rotations a sprite will rotate around each axis.

#### MaxSpriteRotation

The maximum rotation a sprite will rotate around each axis. The sprites will get its rotation by randomizing between Min/MaxSpriteRotation.

#### GravityAcceleration

This controls the speed and direction the sprites will float or sink. A negative value will make the sprites sink to the ground and a positive value will float the sprites up towards the sky. Gravity is always in relation to the world, no matter what the systems rotation is.

#### MinSpriteLifeSpan

Each sprite will last for at least this amount of time, in seconds.

#### MaxSpriteLifeSpan

Each sprite will last no longer than this amount of time, in seconds. The life span of each sprite is a random number between the Min/MaxSpriteLifeSpan. If you want every Sprite to last the same amount of time, make these exactly the same.

#### MinRadius

This is the minimum distance from the center of the system that the sprites will be emitted.

#### MaxRadius

This is the maximum distance from the center of the system that the sprites will be emitted.

#### EmissionPlane

This is the normal vector of the plane that the sprites are emitted from. By default this is a plane parallel to the systems Up vector. If you are using a SpriteSystem as an impact FX this should be set to 0.0 0.0 1.0.

#### MinSpriteVelocity

This controls the minimum speed and direction that each sprite will have when they are emitted. The velocity is relative to the system so if you never want sprites to be emitted in the downward direction of the system, make the Y component positive. For impact FX the Z component should be positive so sprites never get emitted into the surface they hit.

#### MaxParticleVelocity

This controls the maximum speed and direction that each sprite will have when they are emitted.

#### EmissionType

A dropdown list box of predefined emission types for the system:

Sphere – The sprites will emit in all directions around the center of the system a distance no less than MinRadius from the center and no more than MaxRadius from the center.

Point - The sprites will emit from the same random point on the emission plane at a distance of no less than MinRadius from the center and no more than MaxRadius from the center. If you wish to have the sprites emit from the direct center of the system make Min/MaxRadius 0.0

Circle - The sprites will emit in a circle on the emission plane at a distance of no less than MinRadius from the center and no more than MaxRadius From the center. Playing with Min/MaxRadius can yield an emission pattern of a circle, doughnut, or ring.

****

#### AType

A dropdown list for determining the additive type to use for the sprites. Norm uses no additive blending and Add uses additive to render the sprites

#### StretchU

An offset to use to stretch the textures of the sprite.

#### StretchV

An offset to use to stretch the textures of the sprite.

****

## CreateFX

### Features

This a unique FX in that it doesn’t render anything or really do anything but start another premade GroupFX. You can specify the name of a GroupFX and whether or not the new FX will loop so you can combine different GroupFX together quickly and easily. You have the option to use a blinding flare by editing the Blind properties. A blinding flare will disable the Z and be drawn over everything else if there is a clear path to the camera.

****

### Use

Whenever you want to start a preexisting GroupFX at a certain time within another GroupFX you can use this FX to set it up. The new GroupFX will begin playing at the time specified by this FX’s Key and will the run independently of the GroupFX that contains it. This could mainly be used if you have an explosion that you want to play and then a looping cloud of smoke in the middle of the explosion to keep playing after the explosion finishes.

h3>Color Keys

This FX does not allow you to edit its Color keys.

### Scale Keys

This FX does not allow you to edit its Scale keys.

****

### Properties

****

#### FXName

This is the name of the preexisting GroupFX that you wish to play.

#### Loop

If set to Yes, the newly created GroupFX will loop and continue to play after the GroupFX that contains it is finished.

****

## FlareSprite

### Features

The FlareSprite FX displays a sprite specified through a .spr file and will scale and fade the sprite based on angels of the camera and sprite. The sprite can rotate, be flat against a wall, or always face the camera.

****

### Use

The most common use for a FlareSprite is as a “halo” of sorts for a light.

### Color Keys

This FX allows you to edit its color keys. The Sprite’s color and alpha will change over time by interpolating between the color keys.

### Scale Keys

This FX doesn’t allow you to edit its Scale Keys.

****

### Properties

****

#### UpdatePos

This property is a drop down list of predefined ways to update the FX position

- Fixed – This is the most common option and will keep the FX at a fixed position.
- Follow – If the FX is motion linked to another FX UpdatePos must be set to follow
- PlayerView – If you are using this FX as a MuzzleFlash, select PlayerView.
- NodeAttach – If this is selected the FX will attach to the specified node.
- SocketAttach - If this is selected the FX will attach to the specified socket.
- PV_NodeAttach – This will attach the FX to a node in a PlayerView model.
- PV_SocketAttach – This will attach the FX to a socket on a PlayerView model.

#### UpdateInterval

This is the length, in seconds, between updates for this FX. This is normally set very low so the FX can update every frame. If constant updating is not needed then the UpdateInterval should be set higher.

#### AttachName

If any of the “Attach” options were selected for UpdatePos this property must be filled out so the FX knows which socket or node to attach to.

#### Offset

If the value of this vector property is anything besides zero the FX will be placed at this position offset from its center.

#### RotateAdd

If the value of this property is anything besides zero the FX will rotate this amount around the axis every update.

#### Sprite

This is the sprite, .spr, file used for the Sprite FX. To select a ,spr file click the button and browse through the Resource Browser and select the desired file.

#### Normal

This is the normal the sprite will be facing when set to face along the normal. By default this is set to be the forward vector of the sprite.

#### Facing

A dropdown list of presets that tell how the sprite will be oriented.

- CameraFacing – The sprite will always be oriented so its forward is facing the camera.
- AlongNormal – The sprite will face in the direction specified by the Normal Property.
- ParentAlign – If the FX has a parent then the sprite will have the same rotation as the parent object.

#### BlendMode

Depending on the sprite used you may wish to set it to use Additive or Multiply blending.

****

#### DisableZ

When this is set to Yes the sprite will look as though it is being drawn over everything else.

#### DisableLight

This controls whether or not dynamic lights affect the color of the sprite.

#### MinAngle

This is the minimum angle that the FX will use to calculate the scale, color and alpha of the Sprite. If the angle is less than this the sprite will interpolate its color, scale and alpha from 0, being the max, and MinAngle being the minimum. If the angle is greater than MinAngle, the FlareSprite is invisible.

#### ObjectAngle

This is a dropdown list to choose which objet to use to calculate the angle. If Sprite is chosen the MinAngle is compared against the angle from the sprites forward to the camera. If Camera is chosen then the angle used is from the forward direction of the camera to the center of the sprite.

#### MinAlpha

When the angle is between 0 and the MinAngle the alpha will go no lower than MinAlpha.

#### MaxAlpha

When the angle is between 0 and MinAngle the alpha will go no larger than MaxAlpha.

#### MinScale

When the angle is between 0 and the MinAngle the scale will go no lower than MinScale.

#### MaxScale

When the angle is between 0 and MinAngle the scale will go no larger than MaxScale.

#### BlindObjectAngle

You can choose which object to use for the angle, sprite or camera, to calculate the scale.

#### BlindSpriteAngle

This is the angle from the Sprite to the camera that is compared against for creating the blinding flare. If the angle is greater the BlindSpriteAngle then the blinding flare is not created.

#### BlindCameraAngle

This is the angle from the Camera to the Sprite that is compared against for creating the blinding flare. If the angle is greater the BlindCameraAngle then the blinding flare is not created.

#### BlindMaxScale

If the angle from the Sprite’s forward to the Camera is less than BlindSpriteAngle and if the angle from the Camera’s forward to the Sprite is less than BlindCameraAngle then the angle, specified by BlindObjectAngle, is used to calculate the scale of the sprite form MaxScale to BlindMaxScale.

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: FXed_Reference.md)2006, All Rights Reserved.
