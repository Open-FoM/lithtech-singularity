| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Playing Sound Code Examples

This section includes three different examples of playing sounds:

- [Server-Controlled Ambient Sound ](#ServerControlledAmbient)
- [Client-Only Local Sound ](#ClientLocal)
- [Client-Only 3D Sound ](#Client3D)

---

## Server-Controlled Ambient Sound

This example creates a looping, ambient sound on the server that is played on all clients.

After the world is loaded, the **IClientShell::PostStartWorld **event notification function is called by LithTech. **PostStartWorld **is a good place to set up dynamic sounds and other global events that will be played on each client. Unless the **PLAYSOUND_GETHANDLE **flag is set, the sound will be deleted when it ends.

| **Note: ** | Most ambient sounds would typically be associated with a particular world, and thus they are usually created as a response to a message from the server (or more specifically, a “sound object” on the server that has been loaded from a world). PostStartWorld is used in this example only for convenience. |
| --- | --- |

The following code illustrates how to use the implement your **PostStartWorld **object to play a sound.

>
```
void CLTServerShell::PostStartWorld()
```

{

```
       HLTSOUND hSound;     // dummy handle for PlaySound()
```

```
       PlaySoundInfo snd;   // sound info description
```

```
       // Initialize sound description
```

```
       PLAYSOUNDINFO_INIT(snd);
```

```
       // This sound should loop constantly. Ambient means it has
```

```
       // distance falloff, but no left/right speaker shifting.
```

```
       snd.m_dwFlags = PLAYSOUND_AMBIENT | PLAYSOUND_LOOP;
```

```
       // Specify properties of this ambient sound.
```

```
       strncpy(snd.m_szSoundName, "sounds/cruise.wav", _MAX_PATH - 1);
```

```
       snd.m_nPriority      = 250;
```

```
       snd.m_fOuterRadius   = 500.0f;
```

```
       snd.m_fInnerRadius   = 50.0f;
```

```
       snd.m_vPosition      = LTVector(0.0f, 200.0f, -500.0f);
```

```
       // Play the sound on each client. The handle is returned to
```

```
       // hSound, but because PLAYSOUND_GETHANDLE was not specified,
```

```
       // the hSound is not guaranteed to be valid.
```

```
       g_pSoundMgr->PlaySound(&snd, hSound);
```

```
}
```

First, the **HLTSOUND **variable **hSound **is declared. This variable is required for the call to PlaySound, which will assign an address to it. However, the code does not use it for any other reason.

A **PlaySoundInfo **structure ( **snd **) is then declared and initialized (using the **PLAYSOUNDINFO_INIT **macro). The sound is set to ambient and looping by assigning the **PLAYSOUND_AMBIENT **and **PLAYSOUND_LOOP **flags to the **m_dwFlags **parameter. Several of the member variables ( **m_szSoundName **, **m_nPriority **, **m_fOuterRadius **, **m_fInnerRadius **, and **m_vPosition **) are then manually reset, over-riding the default settings.

The **ILTClientSoundMgr::PlaySound **function is called on the global interface pointer ( **g_pSoundMgr **) to play the sound on all the clients. Note that the **hSound **out-parameter is not guaranteed to be valid because the **PLAYSOUND_GETHANDLE **flag was not set.

[Top ](#top)

---

## Client-Only Local Sound

This example plays a sound just once (non-looping) and only locally (sounding as though it were in the client’s head). This type of sound is useful for HUD and powerup cues.

The sound is played when the View Reset key is pressed. Therefore, the **IClientShell::OnCommandOn **function supports sound in the **CMD_RESET_VIEW **value in the case statement.

>
```
// --- sound (begin block) ------------------------------------------- //
```

```
// Play a sound
```

```
HLTSOUND hSound;     // dummy handle for PlaySound()
```

```
PlaySoundInfo snd;   // sound info description
```

```
// Initialize sound description
```

```
PLAYSOUNDINFO_INIT(snd);
```

```
// Set flags to play in the client's head, and only play on this
```

```
// client. (i.e. no distance fading or left/right speaker shifts.)
```

```
snd.m_dwFlags = PLAYSOUND_LOCAL | PLAYSOUND_CLIENT;
```

```
// Specify properties of this sound.
```

```
strncpy(snd.m_szSoundName, "sound/footstep_metal.wav", _MAX_PATH - 1);
```

```
snd.m_nPriority = 254;
```

```
// Play the sound. The handle is returned to hSound,
```

```
// but because PLAYSOUND_GETHANDLE was not specified, the hSound is
```

```
// not guaranteed to be valid.
```

```
g_pLTCSoundMgr->PlaySound(&snd, hSound);
```

```
// --- sound (end block) --------------------------------------------- //
```

As with the server-controlled ambient sound example above, the local sound example begins by declaring the **HLTSOUND **variable **hSound **.

A **PlaySoundInfo **structure ( **snd **) is then declared and initialized (using the **PLAYSOUNDINFO_INIT **macro). The sound is set to local and client-only by assigning the **PLAYSOUND_LOCAL **and **PLAYSOUND_CLIENT **flags to the **m_dwFlags **parameter. Two of the member variables ( **m_szSoundName **and **m_nPriority **) are then manually reset, over-riding the default settings.

The **ILTClientSoundMgr::PlaySound **function is called on the global interface pointer ( **g_pSoundMgr **) to play the sound. Note that the **hSound **out-parameter is not guaranteed to be valid because the **PLAYSOUND_GETHANDLE **flag was not set. Unless the **PLAYSOUND_GETHANDLE **flag is set, the sound will be deleted when it ends.

[Top ](#top)

---

## Client-Only 3D Sound

This sound plays on the client at a 3D position. Like the ambient sound, the distance from the sound source determines the volume (given the inner and outer ranges). However, spatial separation between left and right speakers is possible with 3D sounds. For example, if the sound is to the right of the camera, most of the sound will come from the right speaker.

For this example, three looping 3D sounds are played at different areas of the room. To accomplish this, the sounds are played in the first notification to the **IClientShell::Update **function by calling the **FirstUpdate **function.

>
```
void CLTClientShell::FirstUpdate()
```

```
{
```

```
       g_pLTClient->CPrint("CLTClientShell::FirstUpdate");
```

```

       // --- sound (begin block) ------------------------------------------- //
```

```
 
```

```
       // Start music
```

```
       HLTSOUND hSound;     // dummy handle for PlaySound()
```

```
       PlaySoundInfo snd;   // sound info description
```

```
 
```

```
       // Initialize sound description
```

```
       PLAYSOUNDINFO_INIT(snd);
```

```
 
```

```
       // This looping sound plays at a 3D location with distance
```

```
       // falloff and left/right separation.
```

```
       snd.m_dwFlags = PLAYSOUND_3D | PLAYSOUND_LOOP;
```

```
 
```

```
       // Specify properties of this 3D sound.
```

```
       strncpy(snd.m_szSoundName, "sound/music_loop1.wav", _MAX_PATH - 1);
```

```
       snd.m_nPriority      = 255;
```

```
       snd.m_fOuterRadius   = 500.0f;
```

```
       snd.m_fInnerRadius   = 50.0f;
```

```
       snd.m_vPosition      = LTVector(-500.0f, 200.0f, 0.0f);
```

```
 
```

```
       // Play the sound. The handle is returned to hSound,
```

```
       // but because PLAYSOUND_GETHANDLE was not specified, the hSound is
```

```
       // not guaranteed to be valid.
```

```
       g_pLTCSoundMgr->PlaySound(&snd, hSound);
```

```
 
```

```
       // Do the same for music_loop2. All that's different is filename
```

```
       // and position. The rest of snd is still intact.
```

```
       strncpy(snd.m_szSoundName, "sound/music_loop2.wav", _MAX_PATH - 1);
```

```
       snd.m_vPosition = LTVector(500.0f, 200.0f, 0.0f);
```

```
       g_pLTCSoundMgr->PlaySound(&snd, hSound);
```

```
 
```

```
       // Do the same for music_loop3. All that's different is filename
```

```
       // and position. The rest of snd is still intact.
```

```
       strncpy(snd.m_szSoundName, "sound/music_loop3.wav", _MAX_PATH - 1);
```

```
       snd.m_vPosition = LTVector(0.0f, 200.0f, 500.0f);
```

```
       g_pLTCSoundMgr->PlaySound(&snd, hSound);
```

```
 
```

```
       // --- sound (end block) --------------------------------------------- //
```

```
}
```

After some object operations, the sound code begins. As with the other sound examples explained previously, the **HLTSOUND **is created and **PlaySoundInfo **structure is created and initialized. The first sound is set to 3D and looping by assigning the **PLAYSOUND_3D **and **PLAYSOUND_LOOP **flags to the **m_dwFlags **parameter. Several of the member variables ( **m_szSoundName **, **m_nPriority **, **m_fOuterRadius **, **m_fInnerRadius **, and **m_vPosition **) are then manually reset, over-riding the default settings.

The **ILTClientSoundMgr::PlaySound **function is called on the global interface pointer ( **g_pSoundMgr **) to play the first sound. Note that the hSound out-parameter is not guaranteed to be valid because the **PLAYSOUND_GETHANDLE **flag was not set.

For the second sound, the same **PlaySoundInfo **structure instance is used. However, the file name ( **m_szSoundName **) and position ( **m_vPosition **) are changed. The **PlaySound **function is then called again to begin playing the second sound.

The third sound again uses the same **PlaySoundInfo **structure and the **PlaySound **function, changing only the filename and position.

Even though the structure is re-used, the first and second sounds are unaffected. Repeated uses of the same **PlaySoundInfo **structure do not affect sounds started previously. To alter a sound, you have to use the handle to the sound ( **HLTSOUND **).

The priority level for all three of these looping sounds is the same (255). However, if you move between the sounds, you can still hear each of them if sufficient voices are available on the sound system.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Audio\Example.md)2006, All Rights Reserved.
