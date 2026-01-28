| ### Programming Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# —Tutorial—
Special Effects1

The Special Effects tutorial combines the effects of the [Animations2 ](animat2.md)tutorial and sample base - clientfx sample. We recommend that you read the documentation for the [sample base ](../ProgGuid/Prog/SampleB.md)and [clientfx ](../ProgGuid/ClientFX/mCliFX.md)samples before working with this tutorial. The Special Effects sample code can be found in the following directory:

>

\Samples\tutorials\specialeffects1.

This section contains the following simple specialeffects topics:

- [Starting the Tutorial ](#StartingtheTutorial)
- [Server-side Changes ](#ServersideChanges)

---

## Starting the Tutorial

As with Animations2, when you startup the SpecialEffects1 sample code, the only thing visible is text from the console. You need to specify the game type that you want to start. The options are listed below:

- **Normal— **normal single player game. type: normal
- **Server— **start this as a multiplayer server. This is NOT a standalone server. type: server
- **Join— **connect to a server. type: join <ip.ip.ip.ip> or join <computername>

---

## Server-side Changes
The only new matierial to cover is your addition of a special effect being created upon a projectile being destroyed. (ie. hitting a wall or other player)

#### Projectile.cpp
The first thing to examine is the **TouchNotify **method of the Projectile class. When a collision is detected, you must determine if the object connected with is part of the world, or if the object is a game object. The following code shows the implementation of the **TouchNotify **method.

>
```

uint32 Projectile::TouchNotify(void *pData, float fData)
{
    HOBJECT hObj = (HOBJECT)pData;

    //Prevent projectile from hitting the owner.
    if((hObj != m_hOwner) && (m_hOwner != NULL))
    {
        CollisionInfo Info;
        memset(&Info, 0, sizeof(Info));
        g_pLTServer->GetLastCollision(&Info);

        LTRESULT result = g_pLTSPhysics->IsWorldObject( hObj );

        if (result == LT_YES)
        {
            //g_pLTServer->CPrint( "Projectile::I hit the world at poly %d", Info.m_hPoly);

            // Send clientfx message
            uint32 dwFxFlags = 0;
            PlayClientFX("CanImpact", m_hObject, LTNULL, LTNULL, dwFxFlags);
        }
        else
        {
            //g_pLTServer->CPrint( "Projectile::I hit an object %p -> %p", hObj, m_hOwner );

            // Send clientfx message
            uint32 dwFxFlags = 0;
            PlayClientFX("PlayerImpact", m_hObject, LTNULL, LTNULL, dwFxFlags);
        }
        g_pLTServer->RemoveObject(m_hObject);
    }
    return 1;
}

```

#### Serverutilities.cpp
The next step to perform is to send a message to the ClientFX system requesting an FX to play. The **PlayClientFX **function is stored in serverutilities.cpp, and is responsible for creating the clientfx message to send to your clientshell. The message then gets handled by the clientfx system. The **PlayClientFX **function is only meant to be used with non-looping FX. It is set up to create a "fire and forget" FX. The following code shows the implementation of the **PlayClientFX **function.

>
```

void PlayClientFX(char* szFXName,
                  HOBJECT hParent,
                  HOBJECT hTarget,
                  LTVector* pvPos,
                  LTRotation *prRot,
                  LTVector *pvTargetPos,
                  uint32 dwFlags)
{
	LTVector vPos;
	LTRotation rRot;
	if(hParent)
	{
		g_pLTServer->GetObjectPos(hParent, &vPos);
		g_pLTServer->GetObjectRotation(hParent, &rRot);
	}
	else if( !pvPos || !prRot )
	{
		g_pLTServer->CPrint("ERROR in PlayClientFX()!  Invalid position specified for FX: %s!", szFXName);
		return;
	}
	else
	{
		vPos = *pvPos;
		rRot = *prRot;
	}
	
	CAutoMessage cMsg;
	cMsg.Writeuint8(SFX_CLIENTFXGROUPINSTANT);
	cMsg.WriteString(szFXName);
	cMsg.Writeuint32(dwFlags);
	cMsg.WriteCompPos(vPos);
	cMsg.WriteCompLTRotation(rRot);
	cMsg.WriteObject(hParent);
	// Write the target information if we have any...
	
	if( hTarget || pvTargetPos )
	{
		cMsg.Writeuint8( true );
		cMsg.WriteObject( hTarget );
		LTVector vTargetPos;
		if( hTarget )
		{
			g_pLTServer->GetObjectPos( hTarget, &vTargetPos );
		}
		else
		{
			vTargetPos = *pvTargetPos;
		}
		cMsg.WriteCompPos( vTargetPos );
	}
	else
	{
		cMsg.Writeuint8( false );
	}
	g_pLTServer->SendSFXMessage(cMsg.Read(), vPos, 0);
}

```

These are the only changes needed in order to create one time FX using the ClientFX system. As mentioned above, please read the [Animations2 ](animat2.md)tutorial and [base\Clientfx ](../ProgGuid/ClientFX/mCliFX.md)sample.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Tutorials\SP_FX1.md)2006, All Rights Reserved.
