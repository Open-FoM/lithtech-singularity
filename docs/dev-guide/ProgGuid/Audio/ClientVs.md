| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Client-Side vs. Server-Side

If a sound is played from inside the object.lto file, it is a server-side sound. If it is played within cshell.dll, it is a client-side sound. Server-side sounds are tracked on the server and any players that are within the sound’s radius will hear it. Client-side sounds can only be heard by the client that played the sound. It is best to minimize the number of server-side sounds to reduce network traffic for multi-player games. Good candidates for client side only sounds are menu sounds and in-game interface sounds.

The **ILTSoundMgr **class provides functions to manage sounds from the server. The **ILTClientSoundMgr **class derives from **ILTSoundMgr **and extends that interface by giving the client-side game code the ability to access and modify sound attributes on the client.

## Server-side Sound Tracking

When a server-side sound is played, it is sent to clients within the sound’s outer radius. If no clients are within this distance, the server will keep track of the sound if any of the following flags are set: **PLAYSOUND_LOOP **, **PLAYSOUND_ATTACHED **, **PLAYSOUND_GETHANDLE **, **PLAYSOUND_TIMESYNC **, **PLAYSOUND_TIME **.

Tracked sounds will continue to progress in time. If a client comes within the outer radius at a later time before the sound has ended, the client will be notified about the sound and where to start playing.

Because of possible lag between a server and client, the client may play a sound from a position behind where the server is playing. To help make up for this lag, set the **PLAYSOUND_TIMESYNC **flag.

## HLTSOUND on the Server

If a server-side sound is played with **PLAYSOUND_GETHANDLE **, then the sound object belongs to the game. It must be removed by the game with **ILTSoundMgr::KillSound **or **ILTSoundMgr::KillSoundLoop **(for looping sound with markers).

A sound played without the **PLAYSOUND_GETHANDLE **is handled by LithTech and removed automatically. A server-side sound that is removed either automatically or by the game will notify all clients playing the sound to remove their copies.

## Example Sound Server-Side Code

The sound engine does not need to be initialized on the server because it does not actually play sounds. However, if you want to play server-controlled sounds and track or modify them you must create a sound interface for the server.

The following code can be found in ServerInterfaces.cpp. To provide server tracking of sounds, the example declares a global **ILTSoundMgr **pointer: **g_pSoundMgr **.

>

ILTSoundMgr* g_pLTSSoundMgr = NULL;

define_holder_to_instance(ILTSoundMgr, g_pLTSSoundMgr, Server);

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Audio\ClientVs.md)2006, All Rights Reserved.
