| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Using AI Attributes

AI attributes include what an AI carries, what their default weapons are, how tough they are, and what search items they carry. This section describes each of the attributes you can modify, and provides some general guidlines for performing the modification.

- [Setting General AttributeOverrides ](#SettingGeneralAttributeOverrides)
- [Setting Attachments ](#SettingAttachments)
- [Setting SearchProperties ](#SettingSearchProperties)
- [Setting GeneralInventory ](#SettingGeneralInventory)

---

## Setting General AttributeOverrides

General AttributeOverrides are located in the AttributeOverrides property of the CAIHuman object. The following table describes each of the attributes and the values you can use to modify them. Values not listed in this table that appear in DEdit are not supported and were not used in the creation of NOLF 2.

All the attributes listed below are optional. If you leave the values blank, then they get filled in by the aibutes.txt attributes file dependent upon the ModelTemplate you have selected. The sample values listed do not indicate an average, but only represent one possible value taken from AttributeTemplate1 of the aibutes.txt file located in the Attributes folder. These values indicate the standard attribute values for a SovietSoldier AI.

Ideally, to globally change attributes for all AIs of a specific ModelTemplate, you should edit the aibutes.txt file and create a new entry for the character.

| #### Attribute | #### Description |
| --- | --- |
| **SoundRadius ** | Specifies the default sound radius projected by the AI when they speak. Sample value = 1500 |
| **HitPoints ** | Specifies the number of hitpoints the AI starts with. Range is zero to infinity. Initial values depend on difficulty settings. Sample value = 25 |
| **Armor ** | Specifies the starting armor value for the AI. Range is zero to infinity. Initial values depend on difficulty settings. NOLF 2 does not use armor settings, so initial values in NOLF 2 are always zero. Sample value = 0 |
| **Accuracy ** | Specifies the accuracy of the AI when they fire at a target. Range is zero to one (indicating percent of the time the AI hits the target). Initial values are dependent upon difficulty. If the AI is not trying to hit the target, then they intentionally try to miss, however if the target moves in front of the weapon they may still be hit. Sample value = 0.5 |
| **FullAccuracyRadius ** | Specifies radius within which the AI never tries to miss. Range is zero to infinity. Initial values depend on difficulty settings. Sample value = 256 |
| **AccuracyMissPerturb ** | Specifies how far the AI aims to the left or right when intentionally trying to miss. Range is zero to infinity. Sample value = 64 |
| **MaxMovementAccuracyPerturb ** | Specifies the maximum range the AI is off target when firing at a moving target. Range is zero to infinity. Sample value = 10 |
| **MovementAccuracyPerturbTime ** | Specifies the amount of time required for an AIs aim to catch up with a moving target. Range is zero to infinity. Sample value = 3 |
| **JumpSpeed ** | Specifies how fast AI moves when movement for animation is of type Jump (in animationshuman.txt). Range is zero to infinity. Sample value = 280 |
| **WalkSpeed ** | Specifies how fast AI moves when movement for animation is of type Walk (in animationshuman.txt). Range is zero to infinity. Sample value = 60 |
| **SwimSpeed ** | Specifies how fast AI moves when movement for animation is of type Swim (in animationshuman.txt). Range is zero to infinity. Sample value = 0 |
| **RunSpeed ** | Specifies how fast AI moves when movement for animation is of type Run (in animationshuman.txt). Range is zero to infinity. Sample value = 700 |

[Top ](#top)

---

## Setting Attachments

Each AI in the game is capable of taking a number of attachments. The Attachments property in the CAIHuman object allows you to select an attachment you can attach to a model socket.

| **Note: ** | Selecting the wrong attachment for a model results in errors such as the model hanging in the air on fire. If this occurs, then it is likely that the attachment you have selected is not designed for the ModelTemplate. |
| --- | --- |

[Top ](#top)

---

## Setting SearchProperties

The SearchProperties property in each CAI object allows you to control what the player finds when they search the body of a dead AI. The RandomItemSet list gives you a selection of body types, with each body type containing a variety of items selected randomly when the body gets searched. Rats, for instance, should be set to "None" so players do not find usable items when searching rat bodies. Soviet soldiers should be set to "Bodies Soviet."

The randomly selected search items are located in the searchitems.txt file of the Attributes folder. You can edit this file, along with the ClientRes.rc and ClientRes.h files to add new gear items.

You can also create specific items to appear when the player searches an AIs body. These items are typically weapons, gear, intelligence, or key items that you want a player to find when they search a particular AI.

#### To create a specific search item

1. Place a gear or weapon item anywhere in your level.
2. In the gear or weapon item, set Template to FALSE.
3. In the SearchProperties for any CAI, type the name of the gear or weapon item template in the SpecificItems box.

| **Note: ** | Even with Respawn set to TRUE, you may need to create specific unique gear items for each AI in order to get them to appear. Item Respawn in the Single Player game may not work properly. |
| --- | --- |

[Top ](#top)

---

## Setting GeneralInventory

The GeneralInventory property of CAI is not used in NOLF2.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: WAI\Attribs.md)2006, All Rights Reserved.
