| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Debugging AIs

This section provides some general information and helpful hints on debugging AIs after you have placed them in a level.

- [About Debugging ](#AboutDebuggingAI)
- [Consol Commands ](#ConsolCommands)
- [Mode and Setting Keys ](#ModeandSettingKeys)

---

## About Debugging AI

Debugging AI is a process that often involves a great deal of testing. Placing AINodeUseObject nodes, for example, may require several attempts before you get the AI to use the target object in an appropriate manner.

AIVolume placement is another task that requires testing. Simply placing AIVolumes over an entire room results in AIs walking across furniture and other objects. At the same time, placing AINodeUseObjects that target objects outside of an AIVolume will cause an AI to play their animations in the wrong location, or fail to use the node at all.

AIVolumes near doors should leave an unvolumed area for the doors to open. This prevents AIs from walking through them or attempting to fire through the doors. Another important factor is the placement of the door volume. The WorldModel controlling the door should always be placed in its own volume to specify to the AI where to open the door. Failure to put WoldModels in their own limited AIVolumes results in the AI opening the door from the edge of the AIVolume the WorldModel is contained in. For example, if a RotatingSwitch for a door is in one large AIVolume, the AI can activate the switch from the edge of the volume rather from the location of the switch. For an illustration of door volume placement, see [AIVolumes and Doors ](AItutor.md#AIVolumesandDoors)in the [AI Tutorial ](AItutor.md).

---

## Consol Commands

The following table lists commands that you can enter in the console to assist you in debugging AIs:

| #### Command | #### Description |
| --- | --- |
| serv AISenseInfo 1 | Supplies debugging information about an AI's sensory information. |
| serv AIAccuracyInfo 1 | Supplies debugging information about an AI's accuracy. |
| trigger <name> "debug <level>" | Activates general debugging information where <name> specifies the name of an AI, and <level> specifies the debug level. |
| stimulus <stimulus type> [distance multiplier] | Sets a stimulus from the world. For a list of Stimulus types, see the [Currently Existing NOLF2 Stimuli ](Senses.md#CurrentlyExistingNOLF2Stimuli)table. For example, stimulus enemyFootstepSound creates an enemy footstep sound for the AI to hear. You can add a distance multiplier to this command, for example: stimulus enemyFootstepSound 5 specifies an enemy footstep sound at five times the distance. |
| renderstimulus <1 or 0> | Rrenders spheres in the game that represent stimuli in the world. This allows you to visualize the stimuli that AIs are reacting to. When this command is activated, the ten closest stimuli to the player get rendered as red spheres. The spheres size indicates the distance of the stimulus, although the maximum size is capped at one hundred DEdit units to keep them viewable. |
| aishowjunctions <1 or 0> | This command must be placed in your s_autoexec.cfg file. It allows you to see how the AI is responding to junctions. |

[Top ](#top)

---

## Mode and Setting Keys

The following table lists debug keys you can press during runtime to assist you in debugging AIs:

| #### Key | #### Description |
| --- | --- |
| F11 | Toggles through display of AI volumes, AI paths, and AI volumes and paths. |
| SHIFT + F11 | Toggles thorugh display of AINodes. |
| F7 | Gives the player all gear and equipment. |
| F5 | Displays AI names. Point the crosshairs at any particular AI to display its goals and scripts. |
| F2 | Activates clipping mode. In this mode you are undetectable and intangible. You cannot interact with the world, but you can view AI performing their actions without risk of disturbing them. |
| F1 | Activates ghost mode. In this mode you are simply invisible. AI will not fire at you, but they can hear you and become disturbed. You can also interact with props, objects, and WorldModels in this mode. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: WAI\Debugin.md)2006, All Rights Reserved.
