| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Using HLTSOUND

This section discusses the **HTLSOUND **handle used to keep track of sounds played in your game. This section contains the following topics related to tracking and playing sounds using the **HTLSOUND **handle:

- [About HLTSOUND ](#AboutHLTSOUND)
- [HLTSOUND Functions ](#HLTSOUNDFunctions)
- [Example HLTSOUND Code ](#ExampleHLTSOUNDCode)

  - [Tracking Variables ](#TrackingVariables)
  - [Startup Sound ](#StartupSound)
  - [Looping Fire Sound ](#LoopingFireSound)
  - [Wind-Down Sound ](#WindDownSound)

---

## About HLTSOUND

The **HLTSOUND **handle is used to keep track of sounds played in your game. Unless you explicitly instruct LithTech to give you a valid **HLTSOUND **to the sound, LithTech will delete the sound as soon as it expires. Also, your code will have no way to access the sound before it is deleted. Therefore, if you want to control when the sound is deleted or have the ability to alter the sound, you must acquire an **HLTSOUND **to it.

To instruct LithTech to give you an **HLTSOUND **to a sound, you must set the **PLAYSOUND_GETHANDLE **flag in the **PlaySoundInfo **structure’s **m_dwFlags **member variable. If this is specified, LithTech returns a valid **HLTSOUND **in the **PlaySoundInfo **structure’s **m_hSound **member variable, and LithTech will not remove the sound when it finishes playing. Subsequently, the sound can be accessed and/or modified using this handle.

If the **PLAYSOUND_GETHANDLE **flag is set as explained above, the **HLTSOUND **is also returned in the **ILTSoundMgr::PlaySound **function’s **hResult **out-parameter.

Because LithTech does not automatically delete a sound if you have an **HLTSOUND **to it, you must manually do so yourself. In your game code, be sure to use the **ILTSoundMgr::KillSound **function to delete such a sound after you are through with it.

---

## HLTSOUND Functions

Using the **HLTSOUND **to a played sound, you can use the following functions to modify the sound while it is being played or after it has expired. If you do not have an **HLTSOUND **to a sound, you have no way to access it to use these functions.

>

ILTClient::GetSoundData

ILTClient::GetSoundOffset

ILTClient::GetSoundTimer

ILTClient::IsDone

ILTSoundMgr functions that use an HLTSOUND handle:

GetSoundDuration

IsSoundDone

KillSound

KillSoundLoop

PlaySound

Various ILTClientSoundMgr functions also use HLTSOUND:

GetSoundPosition

SetSoundPosition

[Top ](#top)

---

## Example HLTSOUND Code

This example demonstrates the use of **HLTSOUND **functionality by implementing a gunfire sound with wind-up, looping, and wind-down sounds. The code implements this as follows:

1. When the fire button is pushed, a wind-up sound is played once. The code for this is implemented in the **CMD_ACTION1 **case block of the **IClientShell::OnCommandOn **callback function.
2. If the fire button is held down, a looping gunfire sound is played. The code for this is implemented in the **CLTClientShell::PollInput **function located in the **IClientShell::Update **event notification function. The **ILTClient::IsCommandOn **function is used to query LithTech to determine if the fire button is currently depressed.
3. When the fire button is released, a wind-down sound is played. The code for this is implemented in the **CMD_ACTION1 **case block of the **IClientShell::OnCommandOff **event notification function.

This type of logic is useful for just about any moving object, such as vehicles and spinning minigun barrels.

### Tracking Variables

First, the code declares a variable to track the sound.

>

HLTSOUND m_hFireSound;

This is the **HLTSOUND **that will be used to access the various sounds. As one sound stops and another starts, **m_hFireSound **is reassigned to the new sound.

Because the wind-up sound is a special case, the code declares a special flag that will be used to track when the wind-up sound is playing.

>

bool m_bFireWindup;

Both of these variables are declared in ltclientshell.h and initialized in CLTClientShell’s default constructor.

### Startup Sound

The wind-up sound is played when the fire button is pressed. This is implemented in the **CMD_ACTION1 **case block of the **IClientShell::OnCommandOn **event notification function.

>
```
case CMD_ACTION1:
```

```
{
```

```
       g_pLTClient->CPrint("CMD_ACTION1: start");
```

```
       m_bFireWindup = true;
```

```
       //
```

```
       // Play wind-up sound.
```

```
       //
```

```
       PlaySoundInfo snd; // sound info description
```

```
       // Initialize sound description
```

```
       PLAYSOUNDINFO_INIT(snd);
```

```
       // This sound will play locally in the client’s head, only on the
```

```
       // client, and we want to have control over the volume levels.
```

```
       // Specifying PLAYSOUND_GETHANDLE ensures that the sound is
```

```
       // not deleted upon completion.
```

```
       snd.m_dwFlags = PLAYSOUND_LOCAL | PLAYSOUND_CLIENT | PLAYSOUND_CTRL_VOL | PLAYSOUND_GETHANDLE;
```

```
       // Specify properties of this sound.
```

```
       strncpy(snd.m_szSoundName, "sounds/fire_start.wav", _MAX_PATH - 1);
```

```
       snd.m_nPriority = 254;
```

```
       snd.m_nVolume = 40;
```

```
       // Play the sound. This time we're keeping the handle to be used later.
```

```
       g_pSoundMgr->PlaySound(&snd, m_hFireSound);
```

```
}
```

First, the **m_bFireWindup **variable is set to TRUE to indicate that the startup sound is being played.

A **PlaySoundInfo **structure is created and initialized. Several flags are set:

>

**PLAYSOUND_LOCAL **—Play a local sound (in the client’s head).

**PLAYSOUND_CLIENT **—Play the sound only on this client.

**PLAYSOUND_CTRL_VOL **—Enable volume control.

**PLAYSOUND_GETHANDLE **—Do not automatically delete the sound. Return an **HLTSOUND **to the sound so the code can track it.

Three of the structure’s member variables are also explicitly assigned values: **m_szSoundName **, **m_nPriority **, and **m_nVolume **(setting the **PLAYSOUND_CTRL_VOL **enables the code to control the volume of the sound). Other member variables retain their default values.

Finally, the sound is played using the **PlaySound **function. The **HLTSOUND **for the sound is returned by LithTech to the **m_hFireSound **variable. This will be used to track and query the wind-up sound.

### Looping Fire Sound

The looping sound is the gunfire report of the fully automatic weapon. Therefore, the code must query LithTech to determine if the fire button is depressed. If it is, the sound keeps playing. If it isn’t, the sound stops and the wind-down sound plays.

Because the example wind-up sound has a slightly abrupt transition into the looping sound, the example starts the looping sound before the startup sound is finished. This could have been avoided by editing the startup sound. However, this provides an opportunity to demonstrate other uses of **HLTSOUND **.

The looping sound code is located in the **PollInput **function in LTClientShell.cpp (PollInput is called in the **IClientShell::Update **callback function).

>
```
if (g_pLTClient->IsCommandOn(CMD_ACTION1))
```

```
{
```

```
       g_pLTClient->CPrint("ACT_FIRE: loop");
```

```
       // For each time you start/loop/stop the sequence of sounds, this
```

```
       // loop sound is only started ONCE, due to this if() statement.
```

```
       // This way, you don't continually start new sounds ON TOP OF each
```

```
       // other.
```

```
       if (m_bFireWindup && m_hFireSound)
```

```
       {
```

```
              // Clear these, because if not, GetSoundOffset() won't clear
```

```
              // them if it fails.
```

```
              dwFireOffset = dwFireDuration = 0;
```

```
              // Get sound duration and current position in playing
```

```
              // buffer.
```

```
              g_pLTClient->GetSoundOffset(m_hFireSound, &dwFireOffset,                             &dwFireDuration);
```

```
              // Because the transition from startup sound to looping
```

```
              // sound is a little abrupt in this sample, start the
```

```
              // looping sound 90% of the way into the startup sound.
```

```
              if (((double)dwFireOffset >
```

```
                     (double)dwFireDuration * double(0.9)))
```

```
              {
```

```
                     // Kill the startup sound and clear our trigger
```

```
                     // flags.
```

```
                     g_pSoundMgr->KillSound(m_hFireSound);
```

```
                     m_hFireSound = NULL;
```

```
                     m_bFireWindup = false;
```

```
                     // Play looping sound
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
                     // This sound plays in the client's head, only on the
```

```
                     // client, uses volume control, preserves the sound
```

```
                     // until KillSound() is called, and loops.
```

```
                     snd.m_dwFlags=      PLAYSOUND_LOCAL |
```

```
                                         PLAYSOUND_CLIENT |
```

```
                                         PLAYSOUND_CTRL_VOL    |
```

```
                                         PLAYSOUND_GETHANDLE    |
```

```
                                         PLAYSOUND_LOOP;
```

```
                     // Specify properties of this ambient sound.
```

```
                     strncpy(snd.m_szSoundName, "sounds/fire_loop.wav",                                   _MAX_PATH - 1);
```

```
                     snd.m_nPriority      = 254;
```

```
                     snd.m_nVolume        = 70;
```

```
                     // Play the looping sound and store the    handle in                          // m_hFireSound.
```

```
                     g_pSoundMgr->PlaySound(&snd, m_hFireSound);
```

```
              }
```

```
       }
```

```
}
```

The looping sound is played if the fire button is down. The code, therefore, queries LithTech to determine if this is the case (using the **ILTClient::IsCommandOn **function with the **CMD_ACTION1 **type).

The code then checks to see if the wind-up sound is currently playing ( **m_bFireWindup **is TRUE) and makes sure that an **HLTSOUND **for the startup sound exists. If **m_bFireWindup **is FALSE, nothing else is done in the **CMD_ACTION1 **if block. The example will only start the looping sound if the startup sound is playing. This prevents the code from continually starting the looping sound on top of itself.

If the startup sound is playing, **ILTClient::GetSoundOffset **is used to retrieve the sound duration and current position in the playing buffer. (Note that the **HLTSOUNDm_hFireSound **handle is used in this function.) If the startup sound is less than 90% complete, nothing more is done in the **CMD_ACTION1 **if block. If the startup sound is over 90% complete, the wind-up sound is killed by calling **KillSound(m_hFireSound) **, the **m_hFireSound **variable is cleared, and the **m_bFireWindup **Boolean variable is set to FALSE (since the wind-up sound is no longer playing). A **PlaySoundInfo **structure is created, initialized and set with the appropriate flags and values.

Finally, the looping sound is played using the **PlaySound **function. Again, the **HLTSOUND **(the recycled **m_hFireSound **variable) is retrieved for the sound for future use. From this point, the sound is not checked or altered in any way until the fire key is released, which triggers **IClientShell::OnCommandOff **.

### Wind-Down Sound

To complete the gunfire sequence, the code must play the wind-down sound when the fire button is released. This is implemented in the **CMD_ACTION1 **case block of the **IClientShell::OnCommandOff **callback function (in LTClientShell.cpp).

>
```
case CMD_ACTION1:
```

```
{
```

```
       HLTSOUND hSound;     // dummy handle for PlaySound()
```

```
       g_pLTClient->CPrint("ACT_FIRE: stop");
```

```
       g_pLTClient->CPrint("");
```

```
       // If m_hFireSound exists, then we need to kill it first and
```

```
       // clear our trigger flags.
```

```
       if (m_hFireSound)
```

```
       {
```

```
              g_pSoundMgr->KillSound(m_hFireSound);
```

```
              m_bFireWindup = false;
```

```
              m_hFireSound = NULL;
```

```
       }
```

```
       //
```

```
       // Play wind-down sound
```

```
       //
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
       // This sound plays in the client's head, only on the client, and
```

```
       // with sound control.
```

```
       snd.m_dwFlags =      PLAYSOUND_LOCAL |
```

```
                            PLAYSOUND_CLIENT |
```

```
                            PLAYSOUND_CTRL_VOL;
```

```
       // Specify properties of this ambient sound.
```

```
       strncpy(snd.m_szSoundName, "sounds/fire_stop.wav", _MAX_PATH - 1);
```

```
       snd.m_nPriority      = 254;
```

```
       snd.m_nVolume        = 70;
```

```
       // Play the stop/wind-down sound. The handle is returned to
```

```
       // hSound, but because PLAYSOUND_GETHANDLE was not specified, the
```

```
       // hSound is not guaranteed to be valid.
```

```
       g_pSoundMgr->PlaySound(&snd, hSound);
```

```
}
```

Because the wind-down sound is the last sound in the sequence of gunfire sounds, the code does not need to keep track of it after it has finished playing. Therefore, a dummy **HLTSOUND **is declared to be used in the **PlaySound **call at the end of the block.

Next, the wind-up or looping fire sound must be stopped. If **m_hFireSound **exists then one of the two previous sounds is playing. The code calls the **KillSound **function to stop the previous sound. Both the tracking variables, **m_bFireWindup **and **m_hFireSound **, are cleared for subsequent use.

A **PlaySoundInfo **structure is created, initialized, and set with appropriate flags and values. Because this is the last sound in the gunfire sequence, the code does not need to keep track of it. LithTech can delete the wind-down sound as soon as it is finished playing. Therefore, the **PLAYSOUND_GETHANDLE **flag is not set.

Finally, the wind-down sound is played using the **PlaySound **function.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Audio\UsingHLT.md)2006, All Rights Reserved.
