| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# ClientFX Sample
The ClientFX sample shows a working example of how you can use the ClientFX system.

The ClientFX sample project is located in the following directory:

>

\Samples\base\clientfx.

This sample allows developers to browse and view effects created by artists. This sample reads through all of the effects in the current .fxf file and creates a graphical selection menu allowing you to scroll through and play any effects in that list. You can use this sample as a viewer application for the listed effects. Please refer to the [Using ClientFX ](UsingFX.md)section for a detailed description of the components of the ClientFX system.

This section contains the following ClientFx topics:

- [Client-side Changes ](#ClientsideChanges)
- [Server-side Changes ](#ServersideChanges)

---

## Client-side Changes

#### ltclientshell.cpp
First you must initialize the ClientFXDB object. This object holds all of the information for the artist created FX.

>
```
LTRESULT CLTClientShell::OnEngineInitialized(RMode *pMode, LTGUID *pAppGuid)
{
...
    // Initialize Our ClientFX Database
    if(!CClientFXDB::GetSingleton().Init(g_pLTClient))
    {
        g_pLTClient->ShutdownWithMessage( "Could not init ClientFXDB!" );
        return LT_ERROR;
    }
...
}

```

Next, you initialize the ClientFXMgr object. This object is used to control the ClientFX system. m_ClientFXMgr is a member variable added to your clientshell in the form: CClientFXMgr m_ClientFXMgr;
You also want to give ClientFX access to your camera.

>
```
LTRESULT CLTClientShell::OnEngineInitialized(RMode *pMode, LTGUID *pAppGuid)
{
...
    // Init the ClientFX mgr... (this must become before most other classes)
    if( !m_ClientFXMgr.Init( g_pLTClient ) )
    {
        // Make sure ClientFX.fxd is built and in the game dir
        g_pLTClient->ShutdownWithMessage( "Could not init ClientFXMgr!" );
        return LT_ERROR;
    }
    // We need to make sure to setup the camera for the FX Mgr
    m_ClientFXMgr.SetCamera(m_hCamera);
...
}

```

You must now tell the ClientFXMgr control object that you want to update and render the currently active clientfx FX.

>
```
LTRESULT CLTClientShell::Render()
{
...
     //update all the effects status as well as any that might effect the camera
    m_ClientFXMgr.UpdateAllActiveFX( m_bRender );
    // Render the effects
    m_ClientFXMgr.RenderAllActiveFX( m_bRender );
...
}

```

If the application loses window focus, then you must inform ClientFx of this event.

>
```
void CLTClientShell::OnEvent(uint32 dwEventID, uint32 dwParam)
{
..
    case LTEVENT_RENDERTERM:
        {
            m_bRender = false;

            // Let the ClientFx mgr know the renderer is shutting down
            m_ClientFXMgr.OnRendererShutdown();
        }
        break;
..
}

```

ClientFx needs to know about any messages attached to objects. This is commonly done on the server when an effect is created. ClientFx uses these message to associate FX with objects. This is useful the the FX is supposed to follow the object around. A "rocket smoke trail" is an example of that.

>
```
void CLTClientShell::SpecialEffectNotify(HLOCALOBJ hObj, ILTMessage_Read *pMessage)
{
..
    m_ClientFXMgr.OnSpecialEffectNotify( hObj, pMessage );
..
}

```

When an object is removed, ClientFx needs to know. This allows the system to remove any FX that were attached to that object.

>
```
void CLTClientShell::OnObjectRemove(HLOCALOBJ hObj)
{
...
    m_ClientFXMgr.OnObjectRemove( hObj );
...
}

```

When the client exits from the game world, ClientFX needs to know about it, so that all the currently actvie effects can be shutdown and their instances removed from memory.

>
```
void CLTClientShell::OnExitWorld()
{
...
    // Stop all the client FX
    m_ClientFXMgr.ShutdownAllFX();
...
}

```

[Top ](#top)

---

## Server-side Changes

#### ServerSpecialFX object
The only major change to our server-side code is the addition of the ServerSpecialFX object. This object is placeable in DEdit by level designers. This object reads the .fcf to build a dynamic list of effects, from which level designers can select from instead of needing to type the effect's name manually. This is done through the IObjectPlugin class.

In the TurnOn() function we set the object special effects message, as mentioned above. This message gets sent to the client, which in turn gets sent to the ClientFX system.

>
```
void SpecialFX::TurnON()
{
...
    CAutoMessage cMsg;
    cMsg.Writeuint8( SFX_CLIENTFXGROUP );
    cMsg.WriteString( m_sFxName );
    cMsg.Writeuint32( m_dwFxFlags );
    if( m_hTargetObj )
    {
        cMsg.Writeuint8( true );
        cMsg.WriteObject( m_hTargetObj );
        LTVector vPos;
        g_pLTServer->GetObjectPos( m_hTargetObj, &vPos );
        cMsg.WriteCompPos( vPos );
    }
    else
    {
       cMsg.Writeuint8( false );
    }
	
    g_pLTServer->SetObjectSFXMessage( m_hObject, cMsg.Read() );
    // Set flags so the client knows we are on...
    g_pLTSCommon->SetObjectFlags( m_hObject, OFT_Flags, FLAG_FORCECLIENTUPDATE, FLAG_FORCECLIENTUPDATE );
	
    g_pLTSCommon->SetObjectFlags( m_hObject, OFT_User, USRFLG_SFX_ON, USRFLG_SFX_ON );
...
}

```

#### ServerUtilites.cpp
There is one more useful function to mention is PlayClientFX() located in serverutilities.cpp. This function can be used to spawn instant effects.

>
```
void PlayClientFX(char* szFXName,
                  HOBJECT hParent,
                  HOBJECT hTarget,
                  LTVector* pvPos,
                  LTRotation *prRot,
                  LTVector *pvTargetPos,
                  uint32 dwFlags)

```

>

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\ClientFX\FXSamp.md)2006, All Rights Reserved.
