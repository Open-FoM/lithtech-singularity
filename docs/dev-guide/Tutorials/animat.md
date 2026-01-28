| ### Programming Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# —Tutorial—
Creating Animations

The Creating Animations tutorial is an extension of the foundation created by Projectiles. This tutorial focuses on how to play simple model animations, and how to do so in a multiplayer environment. More specifically this section covers playing simple animations for movement event such as walking or standing idle. This tutorial exists in the following directory:

>

\Samples\tutorials\animations.

This section contains the following simple animation topics:

- [Starting the Tutorial ](#StartingtheTutorial)
- [Client-side Changes ](#ClientsideChanges)
- [Server-side Changes ](#ServersideChanges)

---

## Starting the Tutorial

As with Projectiles, when you startup Animations, the only thing visible is text from the console. You need to specify the game type that you want to start. The options are listed below:

- **Normal— **normal single player game. type: normal
- **Server— **start this as a multiplayer server. This is NOT a standalone server. type: server
- **Join— **connect to a server. type: join <ip.ip.ip.ip> or join <computername>

---

## Client-side Changes

#### Playerclnt.cpp
The first step you must take is to use a model which has the animation set that you want to use. For this task the Animations tutorial uses the Cate Archer player model from No One Lives Forever 2.

>
```

void CPlayerClnt::CreatePlayer()
{
...
    strcpy(objCreateStruct.m_Filename, "Models\\player_action.ltb");
    strcpy(objCreateStruct.m_SkinNames[0], "ModelTextures\\CateAction.dtx");
    strcpy(objCreateStruct.m_SkinNames[1], "ModelTextures\\CateActionHead.dtx");
    strcpy(objCreateStruct.m_SkinNames[2], "ModelTextures\\CateActionHair.dtx");
    strcpy(objCreateStruct.m_SkinNames[3], "ModelTextures\\CateActionLash.dtx");
...
}

```

For this tutorial, you want to have the model play the appropriate animation in response to your player's movement. So, when the player moves forward, you want to play a "walking foward" animation. When your player move backwards, you want to play a "walking backwards" animation, and so forth.

With that in mind, a general purpose "PlayAnimation" function has been created to call when you want the animation to change. Notice that all you need to do is pass in the string name of the animation to play. A member variable has been added to keep track of the current animation. (m_hCurAnim) This variable gets used to prevent calling the same animation two frames in a row. If you set the same animation every frame, you would only see the first frame of that animation. To prevent that from happening, a check exists to ignore redundant animation requests. For example, if the "walk forward" animation is playing, if the player continues to move forward, then the animation gets set once and continues to loop. If however, the player starts moving backwards, then the requested animation and the current animation becomes different, at which point the animation is allowed to change.

>
```

void CPlayerClnt::PlayAnimation(const char* sAnimName)
{
       HMODELANIM hAnim = g_pLTClient->GetAnimIndex(m_hObject, (char *)sAnimName);
       if(hAnim != m_hCurAnim)
       {
           m_hCurAnim = hAnim;
           g_pLTClient->SetModelAnimation(m_hObject, hAnim);
           g_pLTClient->SetModelLooping(m_hObject, LTTRUE);
           ...
       }
}

```

Now that your PlayAnimation() functionallity has been added, you need to hook it in to your movement code. Notice that you only need to add one line to each movement code. For your animation name, pass in "LRF", which is the same name as used in ModelEdit for the walk forward animation. Add a call to PlayAnimation with the appropriate animation for each movement code, forward, backwards, left, right, and idle.

>
```

void CPlayerClnt::UpdateMovement()
{
...
    // Look at our input flags
    if( m_dwInputFlags & MOVE_FORWARD)
    {
      vVel += rRot.Forward() * MOVEMENT_RATE;
      PlayAnimation("LRF");
    }
...
}

```

There is one more consideration to be made, and that is the multiplayer aspect. The game server needs to be informed of your choice of currently playing model animations, so that it can send that information to each of the connected clients. This information is not automatically propagated to the server, so you need to write a small amount of client-side code to handle this.

To create this code, first create a message object, and set the message type. MSG_CS_ANIM is defined in msgids.h which is covered in the next section. Next you must write the animation string name to the message. Finally, you send the message to the server.

>
```

void CPlayerClnt::PlayAnimation(const char* sAnimName)
{
...
       if(hAnim != m_hCurAnim)
       {
       ...
            // Send animation to server
            ILTMessage_Write *pMessage;
            LTRESULT nResult = g_pLTCCommon->CreateMessage(pMessage);

            if( LT_OK == nResult)
            {
                pMessage->IncRef();
                pMessage->Writeuint8(MSG_CS_ANIM);
                pMessage->WriteString(sAnimName);
                g_pLTClient->SendToServer(pMessage->Read(), 0);
                pMessage->DecRef();
            }
	 }
...
}

```

#### msgids.h
You must add one more additional message type to your enumeration. This allows both the client and the server to deal with your custom animation change messages.

>
```

enum EMessageID
{
...	
        MSG_CS_ANIM,                // client->server
...		
};

```

---

## Server-side Changes

#### ltservershell.cpp
You only need to make two changes to your servershell in order to allow other players to see your client-set animations. First you need to change the loaded model to match the model that is loaded on the client-side.

>
```

LPBASECLASS CLTServerShell::OnClientEnterWorld(HCLIENT hClient, void *pClientData, uint32 nClientDataLen)
{
...
    strcpy(objCreateStruct.m_Filename, "Models\\player_action.ltb");
    strcpy(objCreateStruct.m_SkinNames[0], "ModelTextures\\CateAction.dtx");
    strcpy(objCreateStruct.m_SkinNames[1], "ModelTextures\\CateActionHead.dtx");
    strcpy(objCreateStruct.m_SkinNames[2], "ModelTextures\\CateActionHair.dtx");
    strcpy(objCreateStruct.m_SkinNames[3], "ModelTextures\\CateActionLash.dtx");
...
}

```

The second change is to handle the MSG_CS_ANIM message sent from the client.

>
```

void CLTServerShell::OnMessage(HCLIENT hSender, ILTMessage_Read *pMessage)
{
...
    case MSG_CS_ANIM:
        {
            char sAnimName[128];
            pMessage->ReadString(sAnimName, 128);
            HMODELANIM hAnim = g_pLTServer->GetAnimIndex(hPlayer, (char *)sAnimName);
            g_pLTServer->SetModelAnimation(hPlayer, hAnim);
            g_pLTServer->SetModelLooping(hPlayer, LTTRUE);
        }
        break;
...
}

```

This is all that is required to implemented the use of simple animations with multiplayer considerations using the LithTech Jupiter System.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Tutorials\animat.md)2006, All Rights Reserved.
