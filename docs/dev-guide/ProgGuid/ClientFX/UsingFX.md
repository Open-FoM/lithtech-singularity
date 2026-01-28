| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Using ClientFX

The ClientFX System provides a simplified interface with which to create interesting and elaborate special effects. The ClientFX DLL code base sits on the game code layer. It interfaces with the LithTech engine to create blazing fast particle systems, sprite systems, lighting effects, poly trails, and many more “effect” types. Any or all of the effect types (keys) can be added together to make a single playable effect. The FxED application is how the artists interface with the ClientFX System. It allows artists to create effects without the need for any additional engineering. In short, it puts the power in the hands of the artists.

This section contains the following topics specific to implementing ClientFX:

- [Core Components ](#CoreComponents)
- [Implementation Details ](#ImplementationDetails)
- [ClientShell Specifics ](#ClientShellSpecifics)

---

## Core Components

>

### ClientFX DLL (Clientfx.fxd)

The ClientFX DLL contains the “keys”, that can be used in FxED when creating your own custom effects, and all of the “behind the scenes” source code associated with controlling those effects. The source code for this project can be found in the following directory:

>

\LT_Jupiter_Xxx\Engine\clientfx .
(replace Xxx with Bin or Src depending on your version)

### FxED application

This is the application used by artists to create effects that utilize the ClientFX system. The details of it’s use are documented in the JupiterHelp.chm document found in the Docs folder.

### Game code objects:

>

#### Client-side

>

**ClientFXDB **
This object is the database of effects containing all the cached properties, and the constant, non-instanced ClientFX data. It is also responsible for loading and unloading the ClientFX DLL(ClientFx.fxd).

**ClientFXMgr **
This object controls input to the ClientFX system. The ClientShell interfaces with this object to inform the ClientFX system to update, when to render, and what effects to play.

**ClientShell code **
The ClientFXDB and ClientFXMgr objects are contained within the ClientShell. Several messages, such as rendering, updating of the effects are passed to the ClientFXMgr here.

#### Server-side

>

**ServerSpecialFX **
This object is placed in a level using DEdit, and sends a message to the ClientShell informing ClientFX which effect it should play. This object reads in the ClientFX.fcf file in order to generate a list that allows the level designer to pick the FX by name out of a drop down menu in DEdit. It expects the .fcf file to be located in \rez\Clientfx\ of your project. If you plan to change this, you will need to make the appropriate changes to this object.

[Top ](#top)

---

## Implementation Details

### ClientShell project

>

**Preprocessor Defines: **
_CLIENTBUILD must be added to the preprocessor define section in Visual Studio, for debug and release projects.

**Includes: **
The proper path to \Engine\clientfx\Shared must be added to the includes section for both debug and release projects. This path contains several files required by both ClientFXDB and ClientFXMgr.

**Source files: **

The following files should be moved/copied to your actual project source directory.

- ClientFXDB.cpp
- ClientFXDB.h

The following files should be moved/copied to your actual project source directory.

- ClientFXMgr.cpp
- ClientFXMgr.h

The following files are referenced from the \Engine\clientfx\Shared directory. They are required for proper linking.

- CommonUtilities.cpp
- ParsedMsg.cpp
- WinUtil.cpp

### ServerShell project

>

**Includes: **
The proper path to \Engine\clientfx\Shared must be added to the includes section for both debug and release projects. This path contains several files required by the ServerSpecialFX object.

**Source files: **
The following files need to be placed in your project source directory.

- ServerSpecialFX.cpp
- ServerSpecialFX.h

**Linking Files: **
The following file is referenced from the \Engine\clientfx\Shared directory. It is needed for proper linking.

- ParsedMsg.cpp

[Top ](#top)

---

## ClientShell Specifics

Several pieces game code need to be added to your ClientShell source which will initialize, and pass control parameters to the ClientFXMgr and ClientFXDB objects. As noted in the summary, this is done in the clientfx sample project. Developers should review the sample project for a working example of these methods, in-use.

### CClientFXMgr

bool CClientFXMgr::Init( ILTClient *pLTClient )
This method is used to grant access to the current instance of the ILTClient interface.

void CClientFXMgr::SetCamera( HOBJECT hCamera )

In order for ClientFX to render, we need to set a camera object for the ClientFX instance.

void CClientFXMgr::ShutdownAllFX()

This will stop all FX from playing and remove their instances from memory.

bool CClientFXMgr::UpdateAllActiveFX( bool bRender )

This is used to allow the FX to update.

bool CClientFXMgr::RenderAllActiveFX( bool bRender )

This method is used to tell ClientFX to render it’s FX now.

void CClientFXMgr::OnRendererShutdown()

This method is used to tell ClientFX that the renderer is no longer available.
For example, if the application window loses “focus”.

void CClientFXMgr::OnSpecialEffectNotify( HOBJECT hObj, ILTMessage_Read *pMessage )

The ServerSpecialFX object sets a special effect notify message for each FX placed in DEdit.

This is how the client is informed of server side effects. That message is handled here.

bool CClientFXMgr::OnObjectRemove( HOBJECT hObj )

When an object is removed from the game world, ClientFX needs to know that the

object associated with an effect is no longer available. This allows the system to shutdown

and remove the effect.

### Additional Methods of Interest

void CClientFXMgr::SetDetailLevel( int nLOD )

This method allows developers to setup performance options within the ClientFX system

void CClientFXMgr::SetGoreEnabled(bool bEnabled)

This method allows developers to turn effects marked as “gore” off.

void CClientFXMgr::Pause(bool bPause)

This method is used to pause and un-pause the ClientFX system.

bool CClientFXMgr::IsPaused()

This method returns the current state of pause on the ClientFX system.

### CClientFXDB

The only action required for dealing with the ClientFXDB is the initialization of that object.

CClientFXDB::GetSingleton().Init( ILTClient *pLTClient )

This method sets the current instance of the ILTClient interface for the ClientFXDB object.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\ClientFX\UsingFX.md)2006, All Rights Reserved.
