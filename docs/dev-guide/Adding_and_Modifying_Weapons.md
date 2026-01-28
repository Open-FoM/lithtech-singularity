| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

**Adding and Modifying Weapons in No One Lives Forever 2 **

****

**

## Overview
**

This document will describe how to modify existing weapons and adding new ones to the game No One Lives Forever 2 (NOLF2). It will go over the necessary properties for the weapons, mods and ammo and how to add ammo and mods to a weapon. The main file used for the weapon, ammo and mod definitions is *weapons.txt *found in your NOLF2\Game\Attributes directory. This file will also take a look at *FX.txt *so you know how to create and modify the special FX for the weapons.

**

## Weapons.txt
**

The first section you will see in weapons.txt is the WeaponOrder list. The WeaponOrder list allows you to specify the order of all weapons and gadgets used in the game and is also used to categorize the weapons into classes. The class numbers refer to the NextWeapon_XX command binding. The NextWeapon_XX command will cycle through all the weapons in the XX class. When adding or changing weapons you may want to reorganize the weapon classes so like weapons are grouped together. The last entries in the WeaponOrder list are the non-player weapons, weapons that are only used by AI.

After the WeaponOrder list is the AutoSwitch list. This is used to set a priority to weapons when auto switching. When the player picks up a weapon the AutoSwitch list is used to determine if they should switch to that weapon. Leave any weapons off the list that you feel the player would never want that game to automatically switch to.

**

## MP_Weapons.txt
**

NOLF2 SinglePlayer and Cooperative game types strictly used weapons.txt for all of its weapon definitions. The other multiplayer game types, Deathmatch, TeamDeathmatch and DoomsDay also used an override file, mp_weapons.txt the allows you to override certain properties from weapons.txt for use in those game types. This allowed us to balance singleplayer and multiplayer game types separately. Only the properties that are going to be overridden need to be listed in the weapon definition in mp_weapons.txt. (see mp_weapons.txt for more info).

**

## Ammo
**

The ammo section is where all of the ammo definitions are listed. The order of the ammo list is in order of creation and doesn’t pertain to the weapons that use the ammo or the priority of the ammo. When creating a new ammo definition you must add it to the end of the already existing list with a tag that is one greater than the previous definition. If you wish to edit an existing ammo definition, you may change any of the properties, listed below, to the desired value. However, you cannot change the name of an ammo definition since that name is used to reference the ammo in the weapon definitions. If you wish to edit any property that is an ID of a localized string (stored in CRes.dll) or when adding a new ammo definition you must edit the string table in CRes.dll and rebuild the game code. (see the ” Building the No One Lives Forever 2 Game” document on how to build the game.)

**

## Ammo Properties
**

| **Name ** | (string) | This is the user friendly name that is used to reference the ammo in the weapon definitions and from within DEdit |
| --- | --- | --- |
| **NameId ** | (integer) | This is an ID that maps to a localized string for the name of the ammo (seen in game). |
| **DescId ** | (integer) | This is an ID that maps to a localized string for the description of the ammo. |
| **Type ** | (integer) | Specifies the type of ammo. 0 – Vector (bullet) 1 – Projectile (grenade) 3 – Gadget (code breaker) |
| **Icon ** | (string) | File path and name of the interface icon fro the ammo that will be displayed when the ammo is selected. |
| **MaxAmount ** | (integer) | Specifies the maximum amount of ammo that the player can carry at any given time. |
| **SelectionAmount ** | (integer) | Specifies the amount of this ammo in a weapon when a weapon is picked up in game. |
| **SpawnedAmount ** | (integer) | Specifies the amount of ammo that will be in an ammo box when this ammo spawns in an ammo box. (level designers can override this value). |
| **InstDamage ** | (integer) | Specifies the amount of Instantaneous damage done by this type of ammo. |
| **InstDamageType ** | (string) | Specifies the type of Instantaneous damage done by this type of ammo. (See valid DamageTypes in DamageTypes.txt). |
| **AreaDamage ** | (integer) | Specifies the Max amount of Area damage done by this type of ammo (i.e., damage done if you are in the center of the damage). |
| **AreaDamageType ** | (string) | Specifies the type of Area damage done by this type of ammo. (See valid DamageTypes in DamageTypes.txt). |
| **AreaDamageRadius ** | (integer) | Specifies the radius of effect for the AreaDamage. |
| **ProgDamage ** | (float) | Specifies the amount of Progressive damage done by this type of ammo. This is the amount of damage that is done per second. |
| **ProgDamageType ** | (string) | Specifies the type of Progressive damage done by this type of ammo. (See valid DamageTypes in DamageTypes.txt). |
| **ProgDamageDuration ** | (float) | Specifies how long the Progressive damage is applied to the victim. |
| **ProgDamageRadius ** | (float) | Specifies the radius of effect for the Progressive damage (only used if ProgDamageLifetime is greater than 0.0). |
| **ProgDamageLifetime ** | (float) | Specifies the time the progressive damage radius of effect lasts (for things like poison gas). |
| **FireRecoilMult ** | (float) | This multiplier modifies the FireRecoilPitch value in a weapon firing this type of ammo (see **FireRecoilPitch **in Weapon definition below) |
| **ImpactFXName ** | (string) | Specifies the impact special fx. The value of this property MUST correspond to the name of an ImpactFX specified in the FX.txt file. |
| **UWImpactFXName ** | (string) | Specifies the under water impact special fx. The value of this property MUST correspond to the name of an ImpactFX specified in the FX.txt file. |
| **ProjectileFXName ** | (string) | Specifies the a projectile special fx. The value of this property MUST correspond to the name of a ProjectileFX specified in the FX.txt file. |
| **MoveableImpactOverrideFXName ** | (string) | Specifies an effect to play if the impact occurs on a moveable object, such as a model or a door. This way you can have effects that 'stick' but not stick to moveable objects so they won't hang in the air. |
| **FireFXName ** | (string) | Specifies a fire special fx. The value of this property MUST correspond to the name of a FireFX specified in the FX.txt file. |
| **TracerFXName ** | (string) | Specifies a tracer special fx. The value of this property MUST correspond to the name of a TracerFX specified in the FX.txt file. |
| **WeaponAniOverride ** | (string) | Not used in NOLF2 and is unsupported |
| **CanBeDeflected ** | (integer) | Ammo can be deflected by weapon. To deflect, the weapon must have a model string key "begindeflect" followed by a key "enddeflect". |
| **DeflectSurfaceType ** | (string) | When ammo is deflected, the impact fx used will be based on this surface type. Surface types found in surface.txt. |
| **CanAdjustInstDamage ** | (integer) | If set to 1 the InstDamage amount may be adjusted programatically, if set to 0 the value specified by InstDamage will always be used. |
| **CanServerRestrict ** | (integer) | If set to 0 to not allow server options to restrict this item from the level. Default: 1. |

**

## Weapons
**

The weapon section is where all of the weapon definitions are listed. The order of the weapon list is in order of creation and doesn’t pertain to the WeaponOrder list mentioned above. When creating a new weapon definition you must add it to the end of the already existing list with a tag that is one greater than the previous definition. If you wish to edit an existing weapon definition, you may change any of the properties, listed below, to the desired value. However, you cannot change the name of a weapon definition since that name is used in the WeaponOrder list and within DEdit. If you wish to edit any property that is an ID of a localized string (stored in CRes.dll) or when adding a new weapon definition you must edit the string table in CRes.dll and rebuild the game code. (see the ” Building the No One Lives Forever 2 Game” document on how to build the game.)

**

## Weapon Properties
**

| **Name ** | (string) | User friendly name used in WeaponOrder above and in DEdit. |
| --- | --- | --- |
| **NameId ** | (integer) | Id that maps to the weapon Localizable name (stored in CRes.dll) |
| **DescriptionId ** | (integer) | Id that maps to the weapon localized description (stored in CRes.dll) |
| **IsAmmoNoPickupId ** | (integer) | Id that maps to the string that is displayed for IsAmmo = 1 type weapons when the weapon can not be picked up (stored in CRes.dll) |
| **ClientWeaponType ** | (integer) | Weapon type: 0 - none (NEVER use this number) 1 - vector (bullet type weapons) 2 - projectile (like crossbows) |
| **AniType ** | (integer) | Classify the weapon into a style of animation when holding the weapon. Players and AI will hold their weapons depending on the animation type of the weapon. Values : 0 - rifle 1 - pistol 2 - melee 3 - Throw 4 - Rifle2 5 - Rifle3 6 - Holster 7 - Place 8 - Gadget 9 - Throwing Stars |
| **Icon ** | (string) | Filename of the texture used for an icon associated with the weapon. for Normal icons ".dtx" is appended to the name. for Unselected icons "U.dtx" is appended to the name. for Disabled icons "D.dtx" is appended to the name. for Silhouette icons "X.dtx" is appended to the name. |
| **Pos ** | (vector) | The player-view weapon model pos (offset from player model in x, y, and z) |
| **MuzzlePos ** | (vector) | The player-view weapon model muzzle pos (offset from the player model in x, y, and z). This is used to determine where to place muzzle fx. |
| **BreachOffset ** | (vector) | The player-view weapon breach offset which is relative to the above model muzzle pos. This is used to determine where shell casings will be ejected from the player-view weapon. |
| **MinPerturb ** | (integer) | The minimum amount of perturb on each vector or projectile fired by the weapon. A value in range from MinPerturb to MaxPerturb is added to the Up and Right components of the firing direction vector based on the current accuracy of the player/ai firing the weapon. |
| **MaxPerturb ** | (integer) | The maximum amount of perturb on each vector or projectile fired by the weapon. A value in range from MinPerturb to MaxPerturb is added to the Up and Right components of the firing direction vector based on the current accuracy of the player/ai firing the weapon. |
| **Range ** | (integer) | The range of the weapon. The weapon is ineffective if shooting at targets beyond the range. |
| **Recoil ** | (vector) | The amount the weapon will recoil when fired. |
| **VectorsPerRound ** | (integer) | The number of vectors (rays cast) per round when the weapon is fired. Used for things like a shotgun. |
| **PVModel ** | (string) | File path and name of the player-view weapon model. |
| **PVSkin0-N ** | (string) | Name of the player-view weapon skin(s) (NOTE: You can specify "Hands" if you want to use the player-view hands texture associated with the current player model style). There can be any number of skins: PVSkin0, PVSkin1, …, PVSkinN. The PVSkinX number relates to the texture indices in the piece info dialog in ModelEdit. |
| **PVRenderStyle0-N ** | (string) | Name of the render style file ltb(s). There can be any number of render styles: PVRenderStyle0, PVRenderStyle1, ..., PVRenderStyleN. The PVRenderStyleX number relates to the renderstyle index in the piece info dialog in ModelEdit. |
| **PVMuzzleFXName ** | (string) | Specifies the optional player-view muzzle fx for this weapon. The value of this property MUST correspond to a name specified in the MuzzleFX definitions in the Attributes\fx.txt file. |
| **PVAttachFXName0-10 ** | (string) | Specifies the optional player-view fx attachment for this weapon. The value of this property MUST correspond to an effect created with FXEd. |
| **HHModel ** | (string) | File path and name of the hand-held (and powerup) weapon model. |
| **HHSkin0-N ** | (string) | Name of the hand-held (and powerup) weapon skin(s). There can be any number of skins: HHSkin0, HHSkin1, ..., HHSkinN. The HHSkinX number relates to the texture indices in the piece info dialog in ModelEdit. |
| **HHRendeStyle0-N ** | (string) | Name of the render style file ltb(s). There can be any number of render styles: RenderStyle0, HHRenderStyle1, ..., HHRenderStyleN. The HHRenderStyleX number relates to the renderstyle index in the piece info dialog in ModelEdit. |
| **HHModelScale ** | (vector) | The scale of the hand-held (and powerup) weapon model. |
| **HHMuzzleFXName ** | (string) | Specifies the optional hand-held muzzle fx for this weapon. The value of this property MUST correspond to a name specified in the MuzzleFX definitions in the Attributes\fx.txt file. |
| **FireSnd ** | (string) | Name of the sound that is played when the weapon is fired. |
| **AltFireSnd ** | (string) | Name of the sound that is played when the weapon is alt-fired. ( **ALT Fire was not used in NOLF 2 and is unsupported **) |
| **SilencedFireSnd ** | (string) | Name of the sound that is played when the weapon is fired with a silencer. |
| **DryFireSnd ** | (string) | Name of the sound that is played when the weapon is dry fired. |
| **ReloadSnd1-3 ** | (string) | Name of the sound that is played when the weapon is reloaded. (mapped to SOUND_KEY 1 – SOUND_KEY 3) |
| **SelectSnd ** | (string) | Name of the sound that is played when the weapon is selected. (mapped to SOUND_KEY 4) |
| **DeselectSnd ** | (string) | Name of the sound that is played when the weapon is deselected. (mapped to SOUND_KEY 5) |
| **MiscSound1-5 ** | (string) | Optional miscellaneous sounds to play during weapon animations. (mapped to SOUND_KEY 10 – SOUND_KEY 14) |
| **FireSndRadius ** | (integer) | The radius of volume falloff that fire sounds are played at |
| **AIFireSndRadius ** | (integer) | The radius of volume that fire sounds make, w.r.t to AI's senses |
| **WeaponSndRadius ** | (integer) | The radius of volume falloff that all other sounds, non-fire sounds, are played at |
| **EnvironmentMap ** | (integer) | Specifies whether or not the weapon uses environment mapping. |
| **InfiniteAmmo ** | (integer) | Specifies whether or not the weapon has unlimited ammo. (1 = true, 0 = false). Things like swords should have unlimited ammo. |
| **ShotsPerClip ** | (integer) | The number of rounds the weapon can fire before it must be reloaded. |
| **FireRecoilPitch ** | (float) | Specifies the amount the weapon should pitch (in degrees) when fired. |
| **FireRecoilDelay ** | (float) | Specifies the time (in seconds) it should take for the weapon's pitch to get back to normal after recoiling from firing. |
| **AmmoName# ** | (string) | Specifies the type of ammo available for this weapon. The value of this property MUST correspond to a name of an AMMO definition. If the weapon uses multiple ammo types, you may specify multiple AmmoName properties. For example: AmmoName0 = “9mm fmj” AmmoName1 = “9mm dum dum” Note: The first (or only) ammo type specified is considered the “default” ammo type. Every weapon MUST have at least one ammo type specified. |
| **ModName# ** | (string) | Specifies the optional mods available for this weapon. The value of this property MUST correspond to the name of a MOD definition. If the weapon uses multiple mods, you may specify multiple ModName properties. For example: ModName0 = “silencer” ModName1 = “scope” |
| **PVFXName# ** | (string) | Specifies the optional player-view fx available for this weapon. The value of this property MUST correspondd to a name specified in the PVFX definitions in the Attributes\fx.txt file. If the weapon uses multiple pv fx, you may specify multiple PVFXName properties. |
| **AIWeaponType ** | (integer) | Classify the weapon for use by AI Values : 0 - None 1 - Ranged 2 - Melee 3 - Thrown |
| **AIMinBurstInterval ** | (float) | Specifies the range of time (in seconds) an AI will wait in between firing a burst from a weapon. IE, the AI will shoot somewhere between Min/AIMaxBurstShots, then wait Min/AIMaxBurstInterval seconds, then shoot again. |
| **AIMaxBurstInterval ** | (float) |  |
| **AIMinBurstShots ** | (integer) | Specifies the range of shots an AI will fire when it is firing a burst from a weapon. IE, the AI will shoot somewhere between Min/AIMaxBurstShots, then wait Min/AIMaxBurstInterval seconds, then shoot again. |
| **AIMaxBurstShots ** | (integer) |  |
| **AIAnimatesReload ** | (integer) | Specifies whether or not an AI animates reloading this weapon. 1 = true, 0 = false) |
| **LooksDangerous ** | (integer) | Specifies whether or not the weapon/gadget looks dangerous to AIs. (1 = true, 0 = false) |
| **FireDelay ** | (integer) | Specifies the minimum delay in milliseconds between fire keys. The server will not allow shots to fire quicker than the FireDelay. |
| **HideWhenEmpty ** | (integer) | Specifies whether or not the weapon/gadget should be hidden when out of ammo (also implies that the deselect animation shouldn't be played when auto-switching from this weapon when it runs out of ammo) (1 = true, 0 = false) |
| **IsAmmo ** | (integer) | Specifies whether or not the weapon/gadget should be treated as ammo. In other words is the weapon placed/thrown (e.g., grenade, bear trap, etc.). If set to true the weapon's pick-up message won't be displayed when the weapon is picked up unless the weapon can't be picked up. If the weapon can't be picked up the IsAmmoNoPickupId string is displayed. This should be used for "thrown" weapons so the player doesn't see both the weapon pickup message AND the ammo pick up message. (1 = true, 0 = false) |
| **HolsterAttachment ** | (string) | Specifies node and model to attch when weapon is holstered. For example: HolsterAttachment = "BACK AK47_holster" |
| **HiddenPiece0-N ** | (string) | Names of Gadget model pieces to hide/show when the gadget is fired. There can be any number of pieces to hide/show. |
| **FireAnimRateScale ** | (float) | Speeds up or slows down the rate of the Fire animation. By default the scale is 1.0 which will play the animation at normal speed, 2.0 will double the speed, and 0.5 will half the speed. |
| **ReloadAnimRateScale ** | (float) | Speeds up or slows down the rate of the Reload animation. By default the scale is 1.0 which will play the animation at normal speed, 2.0 will double the speed, and 0.5 will half the speed. |
| **PowerupFX ** | (string) | Name of the FxED created FX to be played when the item respawns. This FX will constantly loop as long as the item is waiting to be picked up. |
| **RespawnWaitVisible ** | (integer) | (0 or 1) If 0 the item will become invisible once it is picked up and is waiting to respawn. Otherwise the item will remain visible. |
| **RespawnWaitFX ** | (string) | Name of an FxED created FX to play once the item is picked up and is waiting to respawn. |
| **RespawnWaitSkin0-N ** | (string) | Name of the skin files that will be associted with the mode once it is picked up and is waiting to respawn. |
| **RespawnWaitRenderstyle0-N ** | (string) | Name of the renderstyle that will be associated with the model once it is pickedup and is waiting to respawn. |
| **CanServerRestrict ** | (integer) | If set to 0 to not allow server options to restrict this item from the level. Default: 1. |

****

**

## Mods
**

The mod section is where all of the mod definitions are listed. The order of the mod list is in order of creation and doesn’t pertain to the weapons that use the mod or the priority of the mod. When creating a new mod definition you must add it to the end of the already existing list with a tag that is one greater than the previous definition. If you wish to edit an existing mod definition, you may change any of the properties, listed below, to the desired value. However, you cannot change the name of a mod definition since that name is used in the weapon definitions and within DEdit. If you wish to edit any property that is an ID of a localized string (stored in CRes.dll) or when adding a new mod definition you must edit the string table in CRes.dll and rebuild the game code. (see the ” Building the No One Lives Forever 2 Game” document on how to build the game.)

| **Name ** | (string) | User friendly name used in DEdit. |
| --- | --- | --- |
| **Socket ** | (string) | Name of the weapon model socket to attach the mod model to. You can add and edit sockets on a model within ModelEdit. |
| **NameId ** | (integer) | Id that maps to the localized string for the name of the Mod. |
| **DescriptionId ** | (integer) | Id that maps to the localized string for the description of the mod. |
| **Icon ** | (string) | Name of the interface icon associated with the ammo type. |
| **Type ** | (integer) | Specifies the type of mod. Valid values are: 0 – Silencer 1 – Scope |
| **AttachModel ** | (string) | File path and name of the model attachment associated with the mod. |
| **AttachSkin0-N ** | (string) | Name of the skin(s) associated with the mod attachment model. There can be any number of skins: AttachSkin0, AttachSkin1, …, AttachSkinN |
| **AttachRenderStyle0-N ** | (string) | Name of the render style file ltb(s). There can be any number of render styles: AttachRenderStyle0, AttachRenderStyle1, …, AttachRenderStyleN |
| **AttachScale ** | (vector) | Scale of the mod attachment model. |
| **ZoomLevel ** | (integer) | Level the weapon can zoom to (Valid levels are 1, 2, and 3. 0 = can’t zoom). Ignored for non-scope mods. |
| **NightVision ** | (integer) | Does the mod provide night vision (1 = true, 0 = false). Ignored for non-scope mods. **(Night vision was not used in NOLF2 and is unsupported) ** |
| **Integrated ** | (integer) | Is the mod built into the weapon (1 = true, 0 = false). If this is true, any weapon that lists this mod in its list of mods will always have this mod. |
| **Priority ** | (integer) | This is used for weapons that have more than one mod of the same type (e.g., two scopes). This field indicates which should be used if the weapon has both mods (the highest priority is used). NOTE: this is a number between 0 and 255, that should default to 0. |
| **ZoomInSound ** | (string) | Name of the sound that is played when the weapon is zoomed in. |
| **ZoomOutSound ** | (string) | Name of the sound that is played when the weapon is zoomed out. |
| **PickUpSound ** | (string) | Name of the sound that is played when the mod is picked up (either absolute path to sound or name of soundbutes sound) |
| **RespawnSound ** | (string) | Name of the sound that is played when the mod respawns in multiplayer (either absolute path to sound or name of the soundbutes.txt sound) |
| **PowerupModel ** | (string) | Name of the powerup model associated with the mod. This is what players see in the game when picking up the mod. |
| **PowerupSkin0-N ** | (string) | ame of the powerup skin(s) associated with the mod. There can be any number of skins: PowerupSkin0, PowerupSkin1, ..., PowerupSkinN |
| **PowerupRenderStyle0-N ** | (string) | Name of the render style file ltb(s). There can be any number of render styles: PowerupRenderStyle0, PowerupRenderStyle1, ..., PowerupRenderStyleN |
| **PowerupModelScale ** | (float) | The scaling factor used with the powerup model. (same model as attach model) |
| **TintColor ** | (vector) | Color to tint the screen when mod is picked up (r, g, b) |
| **TintTime ** | (float) | Duration in seconds of screen tint when gear is picked up |
| **AISilencedFireSndRadius ** | (integer) | The radius of volume that silenced fire sounds make, w.r.t to AI's senses NOTE: this value only works with Type = 0 mods. |
| **PowerupFX ** | (string) | Name of the FxED created FX to be played when the item respawns. This FX will constantly loop as long as the item is waiting to be picked up. |
| **RespawnWaitVisible ** | (integer) | (0 or 1) If 0 the item will become invisible once it is picked up and is waiting to respawn. Otherwise the item will remain visible. |
| **RespawnWaitFX ** | (string) | Name of an FxED created FX to play once the item is picked up and is waiting to respawn. |
| **RespawnWaitSkin0-N ** | (string) | Name of the skin files that will be associted with the mode once it is picked up and is waiting to respawn. |
| **RespawnWaitRenderStyle0-N ** | (string) | Name of the renderstyle that will be associated with the model once it is pickedup and is waiting to respawn. |

**

## Creating New Weapons
**

When adding a new weapon here are a few things to keep in mind. Your model will need a select, deselect, reload, idle_0 and fire animations. You may also have idle_1 and idle_2 for multiple idle animations, these animations are played less often and may be more pronounced. You can also have prefire and postfire animations that, if present, will play before and after the fire animation. These animations can be used to start and stop looping sounds, be used to show a ramp up and ramp down time for the weapon and may also be used to simulate holding an object to throw, like the grenades. In order for the weapon to actually fire a round it must contain a fire_key frame string on one of the keyframes of the model for one of the prefire, fire, or postfire animations. All the valid framestrings and what the are used for are listed below.

| **FIRE_KEY ** | Lets the weapon know when to fire. |
| --- | --- |
| **SOUND_KEY (integer) ** | Tells the weapon to play a sound that is associated with the given number. Valid numbers and their mapping to the sound files in the weapon definition are: 1 – ReloadSnd 2 – ReloadSnd2 3 – ReloadSnd3 4 – SelectSnd 5 – DeselectSnd 6 – FireSnd 7 – DryFireSnd 8 – AltFireSnd 9 – SilencedFireSnd 10 – MiscSnd1 11 – MiscSnd2 12 – MiscSnd3 13 – MiscSnd4 14 – MiscSnd5 |
| **BUTE_SOUND_KEY (string) ** | Tells the weapon to play a buted sound of the given name. (see soundbutes.txt for more info on soundbutes) |
| **LOOP_SOUND_KEY (integer) ** | Same as SOUND_KEY except the sound will begin to loop. To stop the looping sound use the frame string “LOOP_SOUND_KEY stop”. Usually you want the sound to begin looping during its prefire animation and stop during its postfire animation. |
| **FX (string) ** | Play the specifed FxEd created fx. |
| **FIREFX_KEY (string) ** | Fires the weapon and plays the specified FxEd create FX. Same as doing “FIRE_KEY; FIREFX_KEY (fx name)” |
| **FLASHLIGHT (string) ** | Lets the weapon turn the flashlight on and off. Valid strings: ON OFF |
| **DEFLECT (float) ** | When a weapon hits this key it specifies the time, in seconds, that this weapon can deflect hits from other weapons that are deflecting. |
| **HIDE_PIECE_KEY (string) ** | Sets the specified piece name invisible. |
| **SHOW_PIECE_KEY (string) ** | Sets the specified piece name visible again. |
| **HIDE_PVATTACHFX (integer) ** | Hides specifed PVAttachFXName# on the weapon |
| **SHOW_PVATTACHFX (integer) ** | Shows the specified PVAttachFXName#. |
| **HIDE_PVATTACHMENT (integer) ** | Hides the specified PlayerViewAttachmentX (set on a per player model basis. See modelbutes.txt for more info) |
| **SHOW_PVATTACHMENT (integer) ** | Shows the specified PlayerViewAttachmentX. (set on a per player model basis. See modelbutes.txt for more info) |

****

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Adding_and_Modifying_Weapons.md)2006, All Rights Reserved.
