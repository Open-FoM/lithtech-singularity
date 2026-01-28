| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# 3D Sound Providers

LithTech supports DirectX 8 DirectSound. To be used, the dx8.snd file must be in the current working directory of the game (the same folder as your game’s lithtech.exe file). The dx8.snd file is provided with the LithTech installation and is located in the \development\TO2\ folder.

It is not possible to mix and match hardware and software 3D sounds in Jupiter. Your game must choose one or the other when it initializes the sound engine as explained below. If using 3D hardware sound you can only use mono sounds.

You can use the **ILTClientSoundMgr::GetSound3DProviderLists **function(defined in iltclientsoundmgr.h) to determine if your sound card can support the number of hardware voices your game requires.

>

LTRESULT ILTClientSoundMgr::GetSound3DProviderLists(

Sound3DProvider *&pSound3DProviderList,

LTBOOL bVerify

);

This function returns a linked list (the **pSound3DProviderList **out-parameter) of Sound3DProvider structures identifying the providers available on the system. If the bVerify parameter is set to true, the pSound3DProviderList only includes providers that are completely supported by the system.

The **uiMax3DVoices **parameter lets you pass in a baseline number of hardware voices. If the soundcard does not meet this level then hardware 3D is not available. TO2 uses 32 voices. SBLive and newer cards support this level, so it's a reasonable setting. It's important that the baseline value is bigger than the number of hardware voices requested in **m_nNum3DVoices **parameter of the **SoundInfo **structure passed to the sound engine **ILTClientSoundMgr::InitSound **function, if the game wants to use hardware 3D.

This verification can take several seconds and may cause speaker popping.

To release the list when you are finished with the it, call **ILTClientSoundMgr::ReleaseSound3DProviderList **with the first item of the list.

The Sound3DProvider structure contains information about a 3D sound provider. It is defined in ltbasedefs.h.

>

struct Sound3DProvider

{

uint32 m_dwProviderID;

char m_szProvider[_MAX_PATH+1];

uint32 m_dwCaps;

Sound3DProvider * m_pNextProvider;

};

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Audio\3DSnds.md)2006, All Rights Reserved.
