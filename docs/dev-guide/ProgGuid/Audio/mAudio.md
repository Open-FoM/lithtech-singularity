| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Audio

The LithTech engine supports a wide range of sound manipulation and modification functionality. This functionality varies depending on the target hardware and platform.

The **ILTSoundMgr **class provides the primary sound functions that you will use in your game. Among other things, it allows you to determine the specific 3D sound providers available on the system, initialize the sound engine, initialize sound-related structures, play sounds, and alter sound properties.

LithTech uses DirectSound. The dx8.snd sound driver should be placed in the same directory as Lithtech.exe.

The **ILTSoundMgr **interface is instantiated by LithTech. You can acquire a pointer to it using the Interface Manager’s define_holder_to_instance macro:

>

define_holder_to_instance(ILTSoundMgr, g_pLTSSoundMgr, Server);

Before you can play sounds in your game you will need to determine whether or not to support 3D sound providers and initialize the sound engine. After this has been done, you can play sounds by preparing a sound structure and calling the **ILTSoundMgr::PlaySound **function.

By default, sounds are automatically deleted once they have been played. However, you have the option of keeping a handle to the sound. In this case, LithTech does not delete the sound and you can alter the sound in many ways. When you are done, delete the sound using the **ILTSoundMgr::KillSound **function.

This section discusses the following audio topics:

| #### Audio Topics | #### Description |
| --- | --- |
| [3D Sound Providers ](3DSnds.md) | LithTech provides two choices for 3D sound, DirectX Hardware and DirectX Software. |
| [Initializing the Sound Engine ](InitSnd.md) | Before you can play any sounds in your game you must initialize the LithTech sound engine using the **InitSoundInfo **structure and the **ILTSoundMgr::InitSound function **. |
| [About Playing Sounds ](PlaySnd.md) | For each sound that your game will play you must prepare a PlaySoundInfo structure and pass it to the **ILTSoundMgr::PlaySound **function. |
| [Playing Sound Code Examples ](Example.md) | Provides three examples of how to play sound using Jupiter. |
| [Using HLTSOUND ](UsingHLT.md) | The LithTech engine automatically deletes sounds after they are played unless you specifically instruct LithTech not to do so by requesting an **HLTSOUND **handle to the sound. If you have an **HLTSOUND **handle to a sound, you can alter the sound and/or play it again without repopulating the **PlaySoundInfo **structure. |
| [Client-Side vs. Server-Side ](ClientVs.md) | Both server-side and client-side sound controls are supported by the LithTech engine. |
| [Looping Sounds With Markers ](Looping.md) | The LithTech engine supports markers for looping sounds. |
| [Compressed Sounds ](Compress.md) | Information about using compressed sounds with the LithTech engine. |



[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Audio\mAudio.md)2006, All Rights Reserved.
