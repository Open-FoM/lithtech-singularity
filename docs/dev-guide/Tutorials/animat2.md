| ### Programming Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# —Tutorial—
Animations 2

The Animations2 tutorial is an extension of the foundation created by Animations. This tutorial describes how to play more complex model animations in a multiplayer environment. More specifically this article covers playing animations using separate animation trackers for upper and lower body. This allows the ability to play an attack animation while playing walking/running animations. This tutorial can be found in the following directory:

-
\Samples\tutorials\animations2.

This section contains the following simple animation topics:

- [Starting the Tutorial ](#StartingtheTutorial)
- [Client-side Changes ](#ClientsideChanges)
- [Server-side Changes ](#ServersideChanges)

---

## Starting the Tutorial

As with Projectiles, when you startup Animations, the only thing visible is text from the console. You must specify the game type that you want to start. Enter one of the following options into the console:

- **Normal— **normal single player game. type: normal
- **Server— **start this as a multiplayer server. This is NOT a standalone server. type: server
- **Join— **connect to a server. type: join <ip.ip.ip.ip> or join <computername>

---

## Client-side Changes

#### Playerclnt
The first step in adding support for animation blending is setting up the animation trackers and setting the weight sets. In this example you are setting up upper and lower body animation trackers. However, you are not limited to lower and upper body animation trackers. For example, you could add trackers for animations such as a character's eyes blinking, which is done in No One Lives Forever 2. For the purposes of this tutorial, only support for upper and lower body animations is being added. The following code shows how to add this support.

**Playerclnt.h **
You must first create the member variables for keeping track of your trackers and weight sets.

>
```

class CPlayerClnt
{
...
private:
...
    ANIMTRACKERID m_idUpperBodyTracker;
    ANIMTRACKERID m_idLowerBodyTracker;
    HMODELWEIGHTSET m_hWeightUpper;
    HMODELWEIGHTSET m_hWeightLower;
...
}

```

**Playerclnt.cpp **
You now need to set and use the member variables created in the previous header.

>
```

void CPlayerClnt::CreatePlayer()
{
...
    if(m_hObject)
    {
        // Set up animation trackers
        m_idUpperBodyTracker = ANIM_UPPER;
        g_pLTCModel->AddTracker(m_hObject, m_idUpperBodyTracker);
        m_idLowerBodyTracker = ANIM_LOWER;
        g_pLTCModel->AddTracker(m_hObject, m_idLowerBodyTracker);
        // Set up weight sets
        if ( LT_OK == g_pLTCModel->FindWeightSet(m_hObject, "Upper", m_hWeightUpper) )
        {
            g_pLTCModel->SetWeightSet(m_hObject, m_idUpperBodyTracker, m_hWeightUpper);
        }
        if ( LT_OK == g_pLTCModel->FindWeightSet(m_hObject, "Lower", m_hWeightLower) )
        {
            g_pLTCModel->SetWeightSet(m_hObject, m_idLowerBodyTracker, m_hWeightLower);
        }
    }
}

```

**shared\animids.h **
In the previous code, you use the variables **ANIM_UPPER **and **ANIM_LOWER **when you create your animation trackers. For convienience, these variables are defined in animids.h (as shown in the following code snippet).

>
```

enum
{
    ANIM_NONE  = 0,
    ANIM_UPPER = 1,
    ANIM_LOWER = 2
};

```

**Note **: **ANIMTRACKERIDs **are defined as type **uint8 **or unsigned char. This ID number gets used when dealing with animation trackers. There is a global animation tracker define by the engine, called **MAIN_TRACKER **. The ID number for **MAIN_TRACKER **is defined as 255 (0xFF). The weightset for **MAIN_TRACKER **consists of all the nodes in the model. When **MAIN_TRACKER **gets passed in for any given animation, it gets applied to all nodes in the model.

In order to support multiple animations, you must modify your **PlayAnimation **function to handle multiple animation trackers. Just as you have done previously, in **PlayAnimation **you must check to see if the animation you want to play is already playing. If so, you do not set the animation again. The following code has been modified to include support for using multiple trackers.

**Playerclnt.h **
Instead of keeping track of the current animation as a member variable, check the current animation on a per/tracker basis when **PlayAnimation **is called. This gives you maximum flexibility.

>
```

void    PlayAnimation(const char* sAnimName, uint8 nTracker, bool bLoop = LTTRUE);

```

**Playerclnt.cpp **

>
```

void CPlayerClnt::PlayAnimation(const char* sAnimName, uint8 nTracker, bool bLoop)
{
    HMODELANIM hAnim = g_pLTClient->GetAnimIndex(m_hObject, (char *)sAnimName);

    HMODELANIM hCurAnim;
    g_pLTCModel->GetCurAnim(m_hObject, nTracker, hCurAnim);
    if(hAnim != hCurAnim)
    {
        g_pLTCModel->SetCurAnim(m_hObject, nTracker, hAnim);
        g_pLTCModel->SetLooping(m_hObject, nTracker, bLoop);
        // Send animation to server
        ILTMessage_Write *pMessage;
        LTRESULT nResult = g_pLTCCommon->CreateMessage(pMessage);
        if( LT_OK == nResult)
        {
            pMessage->IncRef();
            pMessage->Writeuint8(MSG_CS_ANIM);
            pMessage->WriteString(sAnimName);
            pMessage->Writeuint8(nTracker);
            pMessage->Writebool(bLoop);
            g_pLTClient->SendToServer(pMessage->Read(), 0);
            pMessage->DecRef();
        }
    }
}

```

**Playerclnt.cpp **
In addition to upgrading the movement code, the "attack" code has also been modified. Now when you click the left mouse button, your Cate player model plays a throwing animation. To make this new functionality work, add the following code.

>
```

bool CPlayerClnt::Attack()
{
    if(m_bAttacking)
    {
        return false;
    }
    // Play anims
    PlayAnimation("UMFi", m_idUpperBodyTracker, LTFALSE);
    m_bAttacking = true;
    return m_bAttacking;
}

```

On every update, check to see if the attack animation has completed. If it has, then reset **m_bAttacking **to false. This allows "attack" to get called again by **ltclientshell **.

>
```

void CPlayerClnt::UpdateAttacking(uint8 nTracker)
{
        HMODELANIM hCurAnim;
        g_pLTCModel->GetCurAnim(m_hObject, nTracker, hCurAnim);
        uint32 nAnimLen, nAnimTime;
        g_pLTCModel->GetCurAnimLength(m_hObject, nTracker, nAnimLen);
        g_pLTCModel->GetCurAnimTime(m_hObject, nTracker, nAnimTime);
        if(nAnimTime >= nAnimLen)
        {
            PlayAnimation("LSt", m_idUpperBodyTracker);
            m_bAttacking = false;
        }
}

```

## Server-side Changes
You need to update your server-side player model with the same changes as your client-side representation for two reasons. First you do so for multiplayer support, secondly you want to create your projectiles at a certain point in your "attack" animation. This is done using model string keys and is discussed in this section.

**PlayerSrvr.cpp **
The code for creating the animation trackers should look very familiar. The only new addition is that you are storing a handle to the socket on your model's "RightHand" socket. You need the socket handle in order to get its position and rotation use when you create your projectile.

>
```

uint32 CPlayerSrvr::InitialUpdate(void *pData, float fData)
{
    // Set up animation trackers
    m_idUpperBodyTracker = ANIM_UPPER;
    g_pLTSModel->AddTracker(m_hObject, m_idUpperBodyTracker);

    m_idLowerBodyTracker = ANIM_LOWER;
    g_pLTSModel->AddTracker(m_hObject, m_idLowerBodyTracker);

    // Set up weight sets

    if ( LT_OK == g_pLTSModel->FindWeightSet(m_hObject, "Upper", m_hWeightUpper) )
    {
        g_pLTSModel->SetWeightSet(m_hObject, m_idUpperBodyTracker, m_hWeightUpper);
    }

    if ( LT_OK == g_pLTSModel->FindWeightSet(m_hObject, "Lower", m_hWeightLower) )
    {
        g_pLTSModel->SetWeightSet(m_hObject, m_idLowerBodyTracker, m_hWeightLower);
    }
    // Get hand socket
    LTRESULT SocketResult = g_pLTSModel->GetSocket(m_hObject, "RightHand", hRightHandSocket);
    if(LT_OK == SocketResult)
    {
        g_pLTServer->CPrint("Got the RightHand socket");
    }

    return 1;
}

```

You also need to update your **PlayAnimation **function to support multiple animation trackers.

>
```

void CPlayerSrvr::PlayAnimation(const char* sAnimName, uint8 nTracker, bool bLooping)
{
        HMODELANIM hAnim = g_pLTServer->GetAnimIndex(m_hObject, (char *)sAnimName);
        g_pLTSModel->SetCurAnim(m_hObject, nTracker, hAnim);
        g_pLTSModel->SetLooping(m_hObject, nTracker, bLooping);
}

```

In ModelEdit, you can setup model strings during certain keyframes. You can listen for these strings in game code. To do so, you need to enable the **FLAG_MODELKEYS **object flag.

>
```

uint32 CPlayerSrvr::PreCreate(void *pData, float fData)
{
...
    pStruct->m_Flags |= FLAG_MODELKEYS;
...
}

```

Now that you have told the engine you want to receive these events, you will receive a **MID_MODELSTRINGKEY **engine message when a model string key is encountered during the playback of an animation. The following code shows how to handle reception of the **MID_MODELSTRINGKEY **engine message:

>
```

uint32 CPlayerSrvr::EngineMessageFn(uint32 messageID, void *pData, float fData)
{
...
    case MID_MODELSTRINGKEY:
        {
            ArgList* pArgList = (ArgList*)pData;
	    char szBuffer[256];
	    sprintf(szBuffer, "");
	
	    for ( int i = 0 ; i < pArgList->argc ; i++ )
	    {
	        if(strcmp("Throw", pArgList->argv[i]) == 0)
		{
		    //create projectile
	            CreateProjectile();
	        }
	    }
            return 1;
        }break;
...
}

```

The final change is in the **CreateProjectile **function. This is where you use the socket handle that you saved earlier. You need to get the position and rotation information that the socket is currently in. Use this information to place the projectile created in the following function.

>
```

void CPlayerSrvr::CreateProjectile()
{
...
LTransform tSocketTransform;
LTRESULT SocketTrResult = g_pLTSModel->GetSocketTransform(m_hObject,
                                                          hRightHandSocket,
                                                          tSocketTransform,
                                                          LTTRUE);
...
    ocs.m_Pos        = tSocketTransform.m_Pos;
...
}

```

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Tutorials\animat2.md)2006, All Rights Reserved.
