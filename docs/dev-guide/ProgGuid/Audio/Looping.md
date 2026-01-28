| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Looping Sounds with Markers

Looping sounds play until they reach the last sample of the sound then start over from the first sample. They continue to do this until they are killed by the game or the level is unloaded.

| **Note: ** | Looping sounds with markers are not available for the PlayStation 2 platform. |
| --- | --- |

There is a special kind of looping sound that can contain markers. These markers specify the loop begin sample and the loop end sample of the loop. This technique of looping a sound is good for sounds that have ramp-up and ramp-down effect, such as an elevator lift might sound like it charges up before it loops. Then when it hits its destination, it might cool down. The charge up sound would be placed before a begin loop marker and the cool down sound would be after an end loop marker.

If LithTech is told to loop a sound that contains markers, it will start the sound from the first sample. When it hits the loop end sample, it rewinds to the loop begin sample and continues playing from there. The sound is then looped between these two markers until **ILTSoundMgr::KillSound **or **ILTSoundMgr::KillSoundLoop **is called. **ILTSoundMgr::KillSound **will just stop the sound. If **ILTSoundMgr::KillSoundLoop **is called, the sound will continue to play but the next time it hits the loop end marker, it will ignore it and continue to play until the last sample of the sound. LithTech will then automatically clean up the sound. The game can discard its **HLTSOUND **after it calls either **ILTSoundMgr::KillSound **or **ILTSoundMgr::KillSoundLoop **.

Markers exist in the actual sound file and are placed using a tool such as CoolEdit®.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Audio\Looping.md)2006, All Rights Reserved.
