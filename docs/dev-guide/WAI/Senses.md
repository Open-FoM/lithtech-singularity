| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Using AI Senses

Each character AI in the NOLF2 game code has senses that allow them to percieve the environment bounded within the AIVolumes belonging to the AIRegion they are spawned it. This section defines each of an AI's senses, and explains how to change them using the AttributeOverrides property.

This section contains the following AISenses topics:

- [About AI Senses ](#AboutAISenses)
- [Customizing AI Senses ](#CustomizingAISenses)
- [Testing AI Senses ](#TestingAISenses)

---

## About AI Senses

Each AI in the NOLF2 game code refers to an attribute template in the AIButes.txt file. These attributes templates specify whether each sense is on off. Any sense not listed in the template is off by default. For each sense that is on, a distance is specified that sets the range for the AI to be triggered by the sense.

Sense Stimuli

Events that stimulate AI senses are listed at the bottom of the AIButes.txt file in the Attributes folder. Each stimulus takes a number of required or optional parameters. The following table describes each of the stimuli that an AI can receive.

>
| #### Stimulus | #### Description |
| --- | --- |
| **Name ** | Name of the stimulus. The name must match an enumeration in the C++ game code. |
| **Sense ** | The AIs sense that the stimulus triggers. Must match one of the sense listed in the Customizing AI Senses table in the following topic. |
| **RequiredAlignment ** | LIKE or HATE. This stimulus triggers the AIs senses that LIKE or HATE the source of the stimulus. TOLERATE is assumed, so AIs with neutral alignments will be triggered by either LIKE or HATE. |
| **Distance ** | Radius from the source of the stimulus that triggers the AIs response. AIs sense the stimulus when the AIs sense distance overlaps with the stimulus distance. This is a base distance which you can further modify in code (for example you can modify the distance depending on the material the AI is walking on). If the distance should be handled entirely in code, then set the distance to 1.00. |
| **Duration ** | Time in seconds that the stimulus exists. Zero means specifies that the stimulus last forever, or until deleted in the code. |
| **AlarmLevel ** | Priority setting for how alarming this stimulus is. Set this value between zero and four, with four being the most alarming. |
| **ReactionDelay ** | Optional parameter specifying the time in seconds that the AI delays before reacting to a fully stimulated sense. The default time is zero. |
| **StimulationIncreaseRateAlert ** | Optional parameter specifying the rate the stimulation gradually increases when the AI is alert. The default value, 100.00, forces immediate full stimulation. |
| **StimulationIncreaseRateUnalert ** | Optional parameter specifying the stimulation gradually increases when the AI is not alert. The default value, 100.00, forces immediate full stimulation. |
| **StimulationDecreaseRateAlert ** | Optional parameter specifying the rate the AIs stimulation gradually decreases when they are alert. The default value, 0.00, specifies the stimulation never decreases. |
| **StimulationDecreaseRateUnalert ** | Optional parameter specifying the rate the AIs stimulation gradually decreases when they are not alert. The default value, 0.00, specifies the stimulation never decreases. |
| **StimulationThreshold ** | Optional parameter specifying the range describing the amount of stimulation needed for partial and full stimulation. The default value, [1.00, 1.00] specifies that no partial stimulation exists. |
| **FalseStimulationLimit ** | Optional parameter specifying the number of false stimulations before an AIs sense is forced to full stimulation. False stimulations occur when an AIs sense reaches partial stimulation, and then falls back to zero stimulation. |
| **MaxResponders ** | Optional parameter specifying the maximum number of AIs that can respond to this stimulus. |
|  |  |
| ### Currently Existing NOLF2 Stimuli |  |
| #### Stimulus | #### Description |
| **EnemyVisible ** | Trigger Sense: SeeEnemy. AIs constantly emanate this stimulus while they are alive. |
| **EnemyFootprintVisible ** | Trigger Sense: SeeEnemyFootprint. Footprints of AIs constantly emanate this stimulus as long as they exist. |
| **EnemyWeaponFireSound ** | Trigger Sense: HearEnemyWeaponFire. AIs create this stimulus when they fire their weapons. |
| **EnemyWeaponImpactSound ** | Trigger Sense: HearEnemyWeaponFire. This stimulus is created when an AIs weapon fire hits something. |
| **EnemyWeaponImpactVisible ** | Trigger Sense: SeeEnemyWeaponImpact. This stimulus is created when an AIs weaopn fire hits another AI. |
| **EnemyFootstepSound ** | Trigger Sense: HearEnemyFootstep. This stimulus is created when an AIs movement animation triggers a Model String. |
| **EnemyDisturbanceSound ** | Trigger Sense: HearEnemyDisturbance. This stimulus is created when an AI uses the coin. |
| **AllyDeathVisible ** | Trigger Sense: SeeAllyDeath. AIs constantly emanate this stimulus while they are dead. |
| **AllyDeathSound ** | Trigger Sense: HearAllyDeath. AIs create this stimulus when they are dying. |
| **AllyPainSound ** | Trigger Sense: HearAllyPain. AIs create this stimulus when they are in pain. |
| **AllyWeaponFireSound ** | Trigger Sense: HearAllyWeaponFire. AIs create this stimulus when they fire their weapons. |

---

## Customizing AI Senses

You can customize AI senses through commands and through the AttributeOverrides property of the CAIHuman node in DEdit. When you change an AIs senses, DEdit shows a green radius indicating the perception range of the AI. Use this radius indicator to adjust the AIs senses to the appropriate level.

The following table describes each of the senses in the CAIHuman AttributesOverrides property:

>
| #### Sense | #### Description |
| --- | --- |
| **Senses ** | Setting senses to TRUE sets all senses to true. Setting sense to FALSE sets all senses to FALSE except those senses you override with a TRUE value. |
| **CanSeeEnemy ** | See a live enemy (TRUE or FALSE) |
| **SeeEnemyDistance ** | Distance radius value |
| **CanSeeAllyDeath ** | See ally die (TRUE or FALSE) |
| **SeeAllyDistance ** | Distance radius value |
| **CanHearAllyDeath ** | Hear an ally cry out when dying (TRUE or FALSE) |
| **HearAllyDeathDistance ** | Distance radius value |
| **CanHearAllyPain ** | Hear ally cry out when damaged (TRUE or FALSE) |
| **HearAllyPainDistance ** | Distance radius value |
| **CanSeeEnemyFlashlight ** | See enemy flashlight (TRUE or FALSE) |
| **SeeEnemyFlashlightDistance ** | Distance radius value |
| **CanSeeEnemyFootprint ** | See enemy footprint (TRUE or FALSE) |
| **SeeEnemyFootprintDistance ** | Distance radius value |
| **CanHearEnemyFootstep ** | Hear enemy footsteps (TRUE or FALSE) |
| **HearEnemyFootstepDistance ** | Distance radius value |
| **CanHearEnemyDisturbance ** | Hear enemy move when not moving quietly (TRUE or FALSE) |
| **HearEnemyDisturbanceDistance ** | Distance radius value |
| **CanHearEnemyWeaponImpact ** | Hear enemy weapon impact with world geometry (TRUE or FALSE) |
| **HearEnemyWeaponImpactDistance ** | Distance radius value |
| **CanHearAllyWeaponFire ** | Hear ally fire a weapon (TRUE or FALSE) |
| **HearAllyWeaponFireDistance ** | Distance radius value |

---

## Testing AI Senses

Once you have changed an AIs senses, they operate differently, and you will want to test the AI to ensure they perform the actions in the game you want them to perform. For instance, you may want to increase an AIs ability to see the enemy at a distance so they can perform well as a sniper. At the same time you don't want the AI to be capable of seeing too far or they will shoot the character before it is logical for them to see the character. In order to test this, you will have to run the game and act as a target to see when the AI fires at you.

For information about consol commands and hotkeys you can use when testing AI, see [Debugging AIs ](Debugin.md).

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: WAI\Senses.md)2006, All Rights Reserved.
