| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# About Playing Sounds

This section covers a variety of material related to playing sounds using Jupiter, and provides an instuctions on the following topics:

- [Types of Sounds ](#TypesofSounds)
- [The PlaySoundInfo Structure ](#ThePlaySoundInfoStructure)
- [Attaching a Sound to an Object ](#AttachingaSoundtoanObject)

  - [Applying Sound Filters ](#ApplyingSoundFilters)

- [Setting Sound Class Volume ](#SettingSoundClassVolume)

---

## Types of Sounds

After the sound engine has been initialized, your game code can play sounds. There are three types of sounds: local, ambient, and 3D. The following list describes each of these three types.

- **Local **
A local sound is something that has no distance or orientation and seems to be played inside the listener’s head.
- **Ambient **
An ambient sound has no orientation, but gets quieter with distance. Ambient sounds are used for things like the hum of machinery, wind, etc.
- **3D **
3D sounds have orientation (left and right speaker spatial separation) and distance.

| **Note: ** | The number of sounds played in one area can affect performance. |
| --- | --- |

Playing a sound is a two-step process.

1. Create a **PlaySoundInfo **structure.
2. Call **ILTSoundMgr::PlaySound **.

Both of these steps are discussed in more detail in the following topics.

[Top ](#top)

---

## The PlaySoundInfo Structure

The **PlaySoundInfo **structure defines a variety of options for sounds. The options you choose can drastically change the performance of the sound rendering. This structure must be populated for each sound to be played.

To initialize the **PlaySoundInfo **struct to default values, use the **PLAYSOUNDINFO_INIT **macro.

The **PlaySoundInfo **structure is defined in the ltbasedefs.h file.

>

struct PlaySoundInfo

{

uint32 m_dwFlags;

char m_szSoundName[_MAX_PATH+1];

HOBJECT m_hObject;

HLTSOUND m_hSound;

unsigned char m_nPriority;

float m_fOuterRadius;

float m_fInnerRadius;

uint8 m_nVolume;

float m_fPitchShift;

LTVector m_vPosition;

uint32 m_UserData;

uint8 m_nSoundVolumeClass;

};

[Top ](#top)

---

## Attaching a Sound to an Object

A sound can be attached to the position of an object by setting the **PLAYSOUND_ATTACHED **flag and assigning the **m_hObject **parameter to the object to attach to. If the host object is removed before the sound ends, the sound will be stopped and deleted. If that sound also had **PLAYSOUND_GETHANDLE **set, it will still be stopped but it will not be deleted. The game must still delete the sound.

#### Different Sound Types for the Same Sound on Different Clients

Some sounds (such as footsteps or dialogue) sound better when they are played locally to the client that played them, but 3D to everyone else. This can be accomplished by setting the **PLAYSOUND_3D **flag to play the sound in 3D.

#### To play the sound locally on any given client

1. Set the **PLAYSOUND_CLIENTLOCAL **and **PLAYSOUND_ATTACHED **flags in the **m_dwFlags **parameter.
2. Assign the walking player’s engine object to the **m_hObject **parameter.
3. Play the sound to all clients using **ILTSoundMgr::PlaySound **.

After creating an PlaySoundInfo structure, pass it in to the **ILTSoundMgr::PlaySound **function. The **PlaySound **function is defined in the ILTSoundMgr.h file.

>

LTRESULT PlaySound(

PlaySoundInfo *pPlaySoundInfo,

HLTSOUND &hResult

);

You pass in a pointer to the **PlaySoundInfo **structure you created in the **pPlaySoundInfo **parameter. The **hResult **out-parameter is a handle to this sound. You can also get a handle to this sound using the **m_hSound **member variable of the **PlaySoundInfo **structure. This handle is not guaranteed to be valid unless the **PLAYSOUND_GETHANDLE **flag is set in **m_dwFlags **.

[Top ](#top)

---

## Applying Sound Filters

Sound filters can be applied to sounds using the **ILTSoundMgr::SetSoundFilter **and **SetSoundFilterParam **functions. Only EAX 2.0 hardware sound filters are supported (although there is code to support DX8 software filters, they are currently too slow).

#### To use a filter

- place a volume brush in DEdit and then assign a filter to it.

Filters are defined in the soundfilters.txt file (located in the \development\TO2\game\attributes\ folder). Parameters are found in iltsound.h in the engine. At game load time, the sound filters are loaded from the this file and then referenced by their name in the **SetSoundFilter **and **SetSoundFilterParam **functions.

Additionally, to enable/disable a sound filter, use the **ILTSoundMgr::EnableSoundFilter **function.

[Top ](#top)

---

## Setting Sound Class Volume

The **ILTSoundMgr::SetSoundClassMultiplier **function can be used to set up sound classes that have a volume relative to the current global volume setting. For example, if the current global setting is .5 and you have a sound class called Weapons set to .5, all sounds with the Weapons class will play at .25 volume.

>

LTRESULT SetSoundClassMultiplier(

uint8 nSoundClass,

float fMult );

This function defines the sound class in the **nSoundClass **parameter and equates a volume multipler with the **fMult **parameter.

When you play a sound you reference the sound class in the **nSoundVolumeClass **parameter of the **PlaySoundInfo **structure passed in to **ILTSoundMgr::PlaySound **.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Audio\PlaySnd.md)2006, All Rights Reserved.
