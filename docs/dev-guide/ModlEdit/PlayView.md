| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Setting up a Player View Weapon

The player view (PV) hands, guns and vehicles for TO2 were modeled, UV textured and animated in Maya. Each weapon PV model was exported from Maya to an .LTA file and could then be opened in ModelEdit for final additions needed for the game code as well as tweaks to the animations. Finally, to get player view weapons to work properly in the game, outline the attributes for each weapon in the weapons.txt and the effects defined in the fx.txt.

#### Modeling

The PV weapon models in TO2 are somewhere between 1400 - 1800 triangles including the hand models.

#### Animating

Model exporters keep translation and rotation information but not scaling information. You should keyframe at least one piece or null object for every frame of the entire animation because fcurve information or similar non-keyframe interpolation information is not retained when exporting. The engine interpolates between keyframes linearly. The exporters only export keys for every keyframe in the scene.

| **Note: ** | Some types of animations, such as character or vehicle turning, are implemented in the game code rather than being built into the models. |
| --- | --- |

#### Exporting

In order for all of the geometry to be exported properly, all the pieces of the model and skeleton need to be part of the same hierarchy. Models should be exported to the \Game\Guns\models_pv directory.

#### Animation Naming

The animations for the weapons need to be very specific for the weapons to function properly in the game. You can use the TO2 models in the SDK to figure out what animations the game code refers to for different behaviors. If you prefer to use your own strategy for organizing weapon animations, you can change the game code and .BUT files to work however you wish.

#### PV Items in ModelEdit

After you export a PV (player-view) weapon or gadget or vehicle, use ModelEdit to add attributes, such as user dims, internal dims, sockets, command strings, and frame strings.

#### User Dims

User dims for all weapons in TO2 are the same: **X: **3.0, **Y: **3.0, **Z: **3.0. Because the models are so small, these dimensions do not need to be exact. However, the default user dimensions for models are intentionally large to remind you that they need to be set.

#### Internal Dims

The model’s internal radius must be set in order for it to draw properly. This is most easily done by selecting Calc Internal Radius from the Model menu, but you can also set this value by hand on the **Model>Set Internal Radius **menu. This value determines when the model appears and disappears from view. If you do not set an internal radius, your model will disappear as soon as its origin is out of view of the active camera. The internal radius should be exactly large enough to contain the model in any of its most extreme movements.

#### Muzzle Flashes

Muzzle flashes on weapons in TO2 are set up in the fx.txt file. The location of each weapon’s muzzle is specified in its section in the weapons.txt attribute file. Both these files are found in \Development\TO2\Game\Attributes .

#### Command Strings

Models can have a command string that is executed when it’s loaded. If you want to set per-model properties in your game code that aren’t defined in ModelEdit or the TO2 game code, you can add code to process any message you want to put in the model’s command string.

#### Frame Strings

Frame strings are added to provide animation based cues to the game code. These are entered by going to the frame of animation where their event occurs and typing the appropriate string in the Frame String box.

#### WEAPONS.TXT

The weapons.txt file found in the attributes folder defines all weapon and ammo properties: interface artwork, model, skin, position, and much more. For a better explanation, look at the weapons.txt for examples. The weapons.txt also has a brief description for what everything does.

#### FX.TXT

The fx.txt file found in the Attributes folder of your TO2 project defines all weapon effect properties: impact effects, muzzle flashes, and other effects like the flamethrower’s pilot light. For a better explanation, look at the fx.txt for examples. The weapons.txt also has a brief description for what everything does.

#### Positioning PV Models

The PV models are positioned in the engine by using the mpwpos command when you hit the key that has been mapped for 'talk.' Hit the key (T by default in TO2) to switch to manipulation mode with the currently selected weapon or gadget. From there you can manipulate the model position by using the character movement keys. When the model is in the desired position enter the mpwpos command again and the position for that specific model will be saved to the weapons.txt file.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ModlEdit\PlayView.md)2006, All Rights Reserved.
