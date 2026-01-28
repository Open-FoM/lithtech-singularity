| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Initializing the Sound Engine

You must initialize the LithTech sound engine on the client to add support for sound. This section discusses the following topics involving initialization of the sound system:

- [Initializing the Sound Engine ](#InitializingTheSoundEngine)
- [Using the InitiSoundInfo Structure ](#InitSoundInfo)
- [Using the InitSound Function ](#UsingtheInitSoundFunction)
- [Example Sound Initialization Code ](#ExampleSoundInitializationCode)

---

## Initializing the Sound Engine

Initialization of the LithTech sound engine typically occurs via the game’s **IClientShell::OnEngineInitialized **function. You cannot play any sound through LithTech until the sound engine is initialized.

#### To initialize the sound engine

1. Create an [InitSoundInfo ](#InitSoundInfo)structure and fill it out with the options your game will use.
2. Call [ILTClientSoundMgr::InitSound ](#UsingtheInitSoundFunction).

[Top ](#top)

---

## Using the InitSoundInfo Structure

The **InitSoundInfo **structure identifies a 3D sound provider. It also defines several options and settings that determine how sounds are played in your game. The structure is defined in ILTSoundMgr.h.

>

struct InitSoundInfo

{

char m_sz3DProvider[_MAX_PATH+1];

uint8 m_nNumSWVoices;

uint8 m_nNum3DVoices;

unsigned short m_nSampleRate;

unsigned short m_nBitsPerSample;

unsigned long m_dwFlags;

unsigned long m_dwResults;

unsigned short m_nVolume;

float m_fDistanceFactor;

float m_fDopplerFactor;

inline void Init();

};

The value for **m_nNum3DVoices **must not exceed the value for **uiMax3DVoices **passed in to the **ILTClientSoundMgr::ReleaseSound3DProviderList **function.

[Top ](#top)

---

## Using the InitSound Function

After creating an **InitSoundInfo **structure, pass it in to the **ILTClientSoundMgr::InitSound **function to initialize a sound driver. The **InitSound **function is defined in the ILTSoundMgr.h file.

>

ILTClientSoundMgr::InitSound(

InitSoundInfo *pSoundInfo

);

After successfully calling this function, the game code can play sounds.

[Top ](#top)

---

## Example Sound Initialization Code

In this example, a global **ILTClientSoundMgr **pointer is created near the top of ClientInterfaces.cpp.

>

ILTClientSoundMgr* g_pLTCSoundMgr = NULL;

This global pointer is then assigned to LithTech’s sound interface using the Interface Manager’s **define_holder **macro:

>

define_holder(ILTClientSoundMgr, g_pLTCSoundMgr);

The **ILTClientSoundMgr **interface is used to play sounds later in the example.

The **InitSoundInfo **structure is created in the LTClientShell.cpp in the **CLTClientShell::InitSound **function:

>

LTRESULT CLTClientShell::InitSound()

{

InitSoundInfo sndInfo;

sndInfo.Init();

//Initialize sound with defaults.

return g_pLTCSoundMgr->InitSound(&sndInfo);

}

For this basic example, the **Init **function is used to initialize the **InitSoundInfo **structure with default values. It is likely that your game may require a more customized **InitSoundInfo **structure.

The audio device identified in the **InitSoundInfo **structure is then initialized using the **ILTClientSoundMgr::InitSound **function. The example can now play sounds.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Audio\InitSnd.md)2006, All Rights Reserved.
