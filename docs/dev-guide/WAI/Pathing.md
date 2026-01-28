| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Using Regions and Volumes

To control where your AI characters can move and navigate, you must use the AIRegion and AIVolume objects. Typically, you will create many AIVolume objects in your level. Each AIVolume is linked to an AIRegion, which controls the AIVolumes an AI can search, and also controls how they respond to alarms.

All AI characters must exist within an AIVolume or they cannot navigate or move within the level.

This section contains the following topics on regions and volumes:

- [Specifying an AI Region ](#SpecifyinganAIRegion)
- [Specifying an AIVolume ](#SpecifyinganAIVolume)

---

## Specifying an AI Region

The AIRegion node controls the set of AIVolumes an AI can search. All AIVolume nodes must be bound to an AIRegion. The placement of AIRegion nodes is not important, only the volumes that are bound to them.

Regions also allow you to specify which AIs respond to an alarm. When you place an alarm Prop object in your level, you can select the regions that go on alert, respond to the alarm, and which regions get searched after the AIs respond to the alarm.

---

## Specifying an AIVolume

This section lists the properties and placement information for the AIVolume nodes available through DEdit using the NOLF 2 game code. For additional information about AIVolumes, see [Placing AI Volumes ](AItutor.md#PlacingAIVolumes)in the [AI Tutorial ](AItutor.md)section.

One major point to make about AIVolumes in general is that NO AIVolume should ever overlap. You should always place AIVolumes, regardless of their type, directly on the border of another AIVolume. The only exception to this rule is the AIVolumePlayerInfo volume which can be placed anywhere in any other volume.

AIvolumes have the following general properties:

| #### Property | #### Description |
| --- | --- |
| **Lit ** | This Boolean value allows you to determine the lit or unlit status of a room when the level starts. AIs are still able to see the player when Lit is False, however they cannot see other objects in the volume. |
| **Region ** | Allows you to specify the AIRegion node for a volume. All AIVolumes must be bound to a region. |
| **PreferredPath ** | This Boolean value allows you to specify whether or not the AI prefers this volume as a path when patrolling. Robots only tread on preferred paths. |
| **LightSwitchNode ** | This optional property allows you to point to a lightswitch prefab that lights or unlights the volume. You must specify the lightswitch prefab using the .node suffix (for example PrefabLightSwitch.node). |

The following table describes the AIVolumes that currently exist in the NOLF2 game code:

| #### Volume | #### Description |
| --- | --- |
| **AIVolume ** | For all AI, the standard AI volume specifies a volume in which an AI can exist. AIVolumes must be directly next to one another for AIs to pass between them. AIVolumes must be bound to an AIRegion node to specify the region the AI can sense across. - **Minimum Size: **48 X 16 X 48. Making doors smaller than 48 units wide is allowed. The recommended size for door volumes is 32 units deep and less than 48 units wide. - **Placement: **Place at least 16 units off the ground. In order for an AI to pass between volumes, the volumes must have an abutment of a minimum of 48 units. AIVolumes may overlap, but ideally you should place the borders directly on top of each other. AIVolume that overlap may cause errors, and AIVolume borders that are not in contact with each other results in a separation that AIs cannot pass through. For specific instructions on placeing AIVolumes, see [Placing AI Volumes ](AItutor.md#PlacingAIVolumes)in the [AI Tutorial ](AItutor.md)section. |
| **AIVolumeAmbientLife ** | Ambient life specifies a volume that animals such as rats and cockroaches can move in. Normal CAI that are human cannot move into AmbientLife volumes, so they are often used to send rats and rabbits into areas that humans cannot travel (such as under beds, and into conduits). |
| **AIVolumeJumpOver ** | For Ninjas only, this volume allows an AI to jump over an obstacle or from ledge to ledge. - **Placement: **JumpOver volumes require 48 DEdit units on each side of the obstacle or ledge. The blue arrow must point along the path of the JumpOver volume. The AI reaches the height of its jump arc at the top middle of the volume. |
| **AIVolumeJumpUp ** | For Ninjas and Soldiers only, this volume flags an AI of an area where they can jump straight up or down from a ledge or roof. |
| **AIVolumeJunction ** | Used in areas where an AI can take more than one path. AIs chasing the player can "lose" the player if the player passes through a junction. The AI decides which direction to go based on the settings of the junction. - **VolumeNorth: **Specifies the volume that borders to the north of the junction in the Top view. - **VolumeSouth: **Specifies the volume that borders to the south of the junction in the Top view. - **VolumeWest: **Specifies the volume that borders to the west of the junction in the Top view. - **VolumeEast: **Specifies the volume that borders to the east of the junction in the Top view. - **Volume<direction>Actions: **The following four actions exist for each of the four directions listed above: - **Nothing— **The AI performs no action other than choosing another direction. - **Continue— **The AI continues through the volume (used for hallways). - **Search— **The AI performs a search in the adjoining volume, and searches through the rest of the AIRegion the volume is in. (You must place at least one search node in each region). - **Peak— **The AI performs a peaking animation and then selects another direction. - **Volume<direction>Chance: **Determines the percentage chance that the AI selects a particular action when they enter the junction. Use decimal percentages to specify (for example 0.5 for 50%). You are allowed to have multiple actions for each possible direction. |
| **AIVolumeLadder ** | Used to flag AIs with climbing animations of a volume that they can climb. |
| **AIVolumeLedge ** | Used to flag an AI that it should fall from the side of a building or ledge. Point the blue arrow toward the side of the ledge to specify the direction the AI of the fall. |
| **AIVolumeStairs ** | Used to flag an AI that it should roll downward when it gets killed. This volume does not control the stair-stepping animation of an AI, which is preformed by default when a standard AIVolume node exists over sloped or staired world geometry. - **Placement: **Place this volume 16 units above the stairs stretching down to the bottom of the last step (or edge of a slope). Point the blue arrow in the volume in the direction you want the AI to roll (typically the bottom stair or bottom of the slope). |
| **AIVolumeTeleport ** | Teleport volumes specify an area AIs can teleport from. You must specify a destination volume. |
| **AIVolumePlayerInfo ** | A subclass of AIInformationVolume, the AIVolumePlayerInfo is used to tell the AI when they can and cannot see the player. The ViewNodes property points to View nodes that allow the AI to see the player when they are inside the player volume. This allows the player to become hidden from an AI when they move into the volume, but still tells the AI that there is a location they can move where they can see the player. When the player moves into an AIVolumePlayerInfo volume, the AIs targeting the player or chasing the player immediatly check the ViewNodes parameter and move to the nodes where they can then view the player. If no nodes are specified in the ViewNodes property, then they AI simply ignores the character, or searches the areas when disturbed. - **Placement: **Typically, PlayerInfo volumes fill an entire room from wall to wall and floor to cieling. - **SenseMask: ** - **None— **Does not apply a sense mask. - **EnemyPermitted— **Masks an enemy AIs senses so that a player can walk freely within the volume without being attacked (unless the player attacks the enemy). - **On: **This Boolean value determines the On/Off state of the PlayerInfo volume. - **Hiding: **When True, the player is hidden from the AI (unless the AI gets very close to the character). Turn Hiding on to simulate a dark room. - **HidingCrouch: **When True, the player is hidden (see above) only when they are crouched. - **ViewNodes: **Specifies the AINodeView nodes used by AIs when they detect the player using this AIVolumePlayerInfo volume. This allows AIs to move to a location (the AINodeView node) where they can see the player. For example, if you place the AIVolumePlayerInfo volume behind a pillar, then place the view node at an angle where the AI can see behind the pillar. When the player fires from within the AINodePlayerInfo volume, the AI automatically check the volume to see if there is a AINodeView node associated with it. If so, then the AI move to the AINodeView node to fire at the player. You must only point to AINodeView nodes, do not attempt to use this feature with AINodeVantage or AINodeCover nodes. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: WAI\Pathing.md)2006, All Rights Reserved.
