| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Using AI Commands

This section describes commands you can send as messages to AIs to instruct them to perform specific actions. For additional information about sending commands from DEdit, see [Using Object Commands ](../Dedit/WorkWith/WObj/mObj.md#UsingObjectMessages)in the [Working With Objects ](../Dedit/WorkWith/WObj/mObj.md)section.

| #### Command | #### Description |
| --- | --- |
| **ACTIVATE ** | Activates an inactive AI. |
| **ADDGOAL <goal> ** | Adds a goal to the AI's goalset. |
| **ALIGNMENT <key> ** | Changes the AI's alignment. |
| **ALWAYSACTIVATE <1 or 0> ** | Sets the AI to always active, or allows them to become activated by an Activate message. |
| **ARMORMOD <mod> ** | Changes an AIs armor value. |
| **CANDAMAGE <bool> ** | Specifies whether or not the AI can take damage. |
| **CANTALK <1 or 0> ** | Specifies whether or not the AI can talk to the player. |
| **CINERACT <Animation Name> ** | Used to specify an animation for the AI to play in cinematics. The animation gets played once. |
| **CINERACTLOOP <Animation Name> ** | Used to specify an animation for the AI to play in cinematics. The animation gets looped. |
| **DAMAGEMASK <key> ** | Specifies an integer value indicating what cannot damage the AI. Typically the value indicates a damage type. Damage types not listed will damage the AI as normal. |
| **DEBUG <level> ** | Sets the debug level for an AI. |
| **DESTROY ** | Destroyes the AI as if it were killed. |
| **DETACH <attach pos> ** | Detaches an attachment from the AI at a specified attachment position. |
| **FACEDIR <X,Y,Z> ** | Causes the AI to face in a specific direction. |
| **FACEOBJECT <object> ** | Causes the AI to face a specific object. |
| **FACEPOS <X,Y,Z> ** | Causes the AI to face in a specific direction. |
| **FACETARGET ** | Causes the AI to face its current target. |
| **FOVBIAS <fov> ** | Alters the AI's field of view. |
| **GADGET <ammo id> ** | Adds a gadget to the AI's inventory. |
| **GOALSCRIPT <script> ** | Specifies a goalscript for the AI to follow. |
| **GOALSET <goalset> ** | Specifies a goalset for the AI to follow. |
| **GRAVITY <1 or 0> ** | Toggles gravity for the AI. |
| **HEALTHMOD <mod> ** | Modfies the AI's health. |
| **HIDDEN <bool> ** | Specifies an AI as hidden or non-hidden. When Hidden an AI is still visible, but becomes intangible. |
| **IGNOREVOLUMES <1-or-0> ** | Specifies whether or not the AI should ignore AI volumes and path through walls. |
| **MOVE <X,Y,Z> ** | Specifies an X,Y,Z set of world coordinates the AI must move to. |
| **PLAYSOUND <sound name> ** | Plays a specified sound at the position of the AI. |
| **PRESERVEACTIVATECMDS <activateon or activateoff> ** | Activate On / Activate Off. Specifies whether the AI should continue accepting activation commands. |
| **REMOVE ** | Cleanly removes an AI from the level and from memory. |
| **REMOVEGOAL <goal> ** | Removes the specified goal from the AI's goalset. |
| **SENSES <1 or 0> ** | Toggles the AI's senses on or off. |
| **SOLID <bool> ** | Specifies an AI as solid or intangible. |
| **TARGET <object> ** | Specifies a target object for the AI to attack. |
| **TELEPORT <teleport point> | <vector> ** | Teleports the AI to the specified TeleportPoint node, or to a specific X,Y,Z coordinate. |
| **TRACKLOOKAT ** | Specifies to use head tracking to look at a target. |
| **TRACKNONE ** | Specifies no head tracking. |
| **VISIBLE <bool> ** | Specifies an AI as visible or invisible. |
| **ChaseSeekPlayer <bool> ** | Boolean value specifying the AI should chase the player. |
| **NeverGiveUp <bool> ** | Boolean value specifying the AI should never give up when chasing the player. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: WAI\Comnds.md)2006, All Rights Reserved.
