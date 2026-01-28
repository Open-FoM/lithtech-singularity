| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Creating Projectiles Tutorial

The Creating Projectiles Tutorial is an extension of the foundation created by the [Creating Pickups Tutorial ](PickUps.md). In most action oriented games players are able to throw or shoot objects. These are commonly refered to as, projectiles. In this tutorial we walk through, how to add this functionallity to the game. This code for the Creating Projectiles tutorial is located in the following directory:

> \Samples\tutorials\projectiles.

It is very common for action game developers to want players to be able to throw or shoot objects from the origin of other objects. These are usually called projectiles and typically cause damage to other objects and players (though this may not always be the case). This section explains one possible way to implement a Projectile game object. This requires you to make changes to input handling on the client as well as add a brand new game object class to the server.

This section contains the following projectile creation topics:

- [Starting the Tutorial ](#StartingtheTutorial)
- [Client-side Changes ](#ClientsideChanges)
- [Server-side Changes ](#ServersideChanges)

  - [Implementing the Projectile Class ](#ImplementingtheProjectileClass)

---

## Starting the Tutorial

As with Pickups, when you startup Projectiles, the only thing visible is text from the console. You need to specify the game type that you want to start. The options are listed below:

- **Normal— **normal single player game. type: normal
- **Server— **start this as a multiplayer server. This is NOT a standalone server. type: server
- **Join— **connect to a server. type: join <ip.ip.ip.ip> or join <computername>

---

## Client-side Changes

This section lists changes to the client side code.

#### Autoexec.cfg

To create a projectile, you must modify the autoexec.cfg file to add a new key binding for "shoot". Add the following entries to the autoexec.cfg file:

>
```
 AddAction Shoot 10
     rangebind "##mouse" "Button 0" 0.000000 0.000000 "Shoot"

```

Pay close attention to the location of these entries in the file. The "Action" must be added before you attempt to bind a key/button to that action.

#### Commandids.h
The command IDs file (commands.h) contains a list that corresponds to the AddAction list in the autoexec.cfg.

>
```

enum ECommands
{
    ...
    COMMAND_SHOOT       = 10,
    ...
}
```

Notice that both **COMMAND_SHOOT = 10 **, and "AddAction Shoot" in the autoexec.cfg file are set to 10. This is because the **COMMAND_SHOOT **property gets used when polling input.

#### ltclientshell.cpp
Listen for a command to execute a "shoot" operation in: **CLTClientShell::OnCommandOn **. Use the **OnCommandOn **method for commands that you only want to get each time the key/button is pressed, but not every frame in which the key is down, as is the case with **IsCommandOn **.

>
```

void CLTClientShell::OnCommandOn(int command)
{
	switch (command)
	{
          ...
          case COMMAND_SHOOT:
          {
              g_pLTClient->CPrint("CLIENT SHOOT!");
              ILTMessage_Write *pMessage;
              LTRESULT nResult = g_pLTCCommon->CreateMessage(pMessage);

              if( LT_OK == nResult)
              {
                  pMessage->IncRef();
                  pMessage->Writeuint8(MSG_CS_SHOOT);
                  g_pLTClient->SendToServer(pMessage->Read(), 0);
                  pMessage->DecRef();
              }
          }
          break;
        ...
        }
}

```

You may notice that this code sends a message to the server instead of creating the projectile on the client. This enables all players in the game to see and react to the projectile.

#### msgids.h
In the **OnCommandOn **example above you are passing **MSG_CS_SHOOT **into the message that you send to the server. You must define **MSG_CS_SHOOT **in the msgids.h shared file. Both the client and server use the **MSG_CS_SHOOT **identifier.

>
```

enum EMessageID
{
...
        MSG_CS_SHOOT,     // client->server
...		
};

```

This concludes the changes and additions to the client-side. The following section explains what must take place on the server.

[Top ](#top)

---

## Server-side Changes

The sections lists changes to the server side code.

#### ltservershell.cpp
First you must catch the messages sent to the server. This is done using the **CLTServerShell::OnMessage **method.

>
```

void CLTServerShell::OnMessage(HCLIENT hSender, ILTMessage_Read *pMessage)
{	
	BaseClass *pBaseClass = (BaseClass *)g_pLTServer->GetClientUserData(hSender);
	HOBJECT hPlayer = g_pLTServer->ObjectToHandle(pBaseClass);
	
	pMessage->SeekTo(0);
	uint8 messageID = pMessage->Readuint8();
	
	switch(messageID)
	{
          ...
          case MSG_CS_SHOOT:
            {
                LTVector vPos;
                LTRotation rRot;

                g_pLTServer->GetObjectPos(hPlayer, &vPos);
                g_pLTServer->GetObjectRotation(hPlayer, &rRot);

                // Create Projectile
                HCLASS hClass = g_pLTServer->GetClass("Projectile");

                ObjectCreateStruct ocs;
                ocs.m_ObjectType = OT_MODEL;
                ocs.m_Flags      = FLAG_VISIBLE;
                ocs.m_Rotation   = rRot;
                vPos += rRot.Forward() * 25.f;
                ocs.m_Pos        = vPos;

                strncpy(ocs.m_Filename, "Models/GenCan.ltb", MAX_CS_FILENAME_LEN -1);
                strncpy(ocs.m_SkinName, "ModelTextures/GenCan1.dtx", MAX_CS_FILENAME_LEN -1);
                Projectile* pObject = (Projectile*)g_pLTServer->CreateObject(hClass, &ocs);

                if(pObject)
                {
                    g_pLTServer->CPrint("Shoot!!!");
                    pObject->SetOwner(hPlayer);
                }
            }
            break;	
         ...
	}
	
}

```

Notice that **MSG_CS_SHOOT **is being handled.

[Top ](#top)

---

### Implementing the Projectile Class

In the previous section you created an instance of the **Projectile **class, and placed it in your game world. The following topics explain how to implement the **Projectile **class, orient the projectile with the player, and place the projectile a few units in front of the player.

#### projectile.cpp
You must create the **Projectile **class in projectile.cpp/.h. The Projectile class is a new game code class created for this tutorial.

>
```

BEGIN_CLASS(Projectile)
END_CLASS_DEFAULT_FLAGS(Projectile, BaseClass, LTNULL, LTNULL, CF_ALWAYSLOAD | CF_HIDDEN)

```

You must use the **CF_HIDDEN **class flag on Projectile. **CF_HIDDEN **prevents the projectile object from appearing in DEdit as a placable object. This prevents Level Designers from placing Projectile objects in the level by mistake.

>
```

uint32 Projectile::InitialUpdate(void *pData, float fData)
{
...
    // Don't ignore small collisions
    g_pLTSPhysics->SetForceIgnoreLimit( m_hObject , 0.0f );
...
}

```

One important thing to note is the use of the **SetForceIgnoreLimit **method. By default, objects are setup to ignore very small collisions. So, when the Projectile hits a wall instead of detecting a collision, it slides against the wall. To prevent projectiles from sliding against polygons you want to remove the projectile from the world. In order to remove a projectile, you must tell the physics interface not to ingore any collisions.

>
```

uint32 Projectile::Update(void *pData, float fData)
{
    LTVector vVel;
    LTRotation rRot;

    g_pLTServer->GetObjectRotation(m_hObject, &rRot);
    vVel = rRot.Forward() * 300.0f;
    g_pLTSPhysics->SetVelocity(m_hObject, &vVel);
    g_pLTServer->SetNextUpdate(m_hObject, 0.1f);
    return 1;
}

```

Now that you have placed the Projectile in the world, you must move the projectile. To move the projectile you must set its velocity. You could simply call the **MoveObject **, or the **SetObjectPos **method to move the projectile, but the **SetVelocity **method allows the clients to perform client-side prediction for this object. Because of this, you should use the **SetVelocity **method for moving objects whenever possible.

Since your object moves in a straight line, you only need to set the velocity once. The following example sets the velocity 10 times per second. If your Projectile has a more complicated flight path, you will want to update the velocity frequently.

>
```

uint32 Projectile::TouchNotify(void *pData, float fData)
{
    HOBJECT hObj = (HOBJECT)pData;

    CollisionInfo Info;
    memset(&Info, 0, sizeof(Info));
    g_pLTServer->GetLastCollision(&Info);

    LTRESULT result = g_pLTSPhysics->IsWorldObject( hObj );

    if (result == LT_YES)
    {
        g_pLTServer->CPrint( "Projectile::I hit the world at poly %d", Info.m_hPoly );
    }
    else
    {
        if(hObj != m_hOwner)
        g_pLTServer->CPrint( "Projectile::I hit an object %p", hObj );
    }
    if(hObj != m_hOwner)
    g_pLTServer->RemoveObject(m_hObject);
    return 1;
}

```

The last piece of important information in about your Projectile class is the handling of Touch Notify events. When an object gets a **MID_TOUCHTOTIFY **engine message it passes a void pointer "pData," which contains the **HOBJECT **of the object the projectile collided with. Typically you will want to find out whether the target object your projectile connected with is another game object or a polygon on the world, (such a a wall or the floor).

To determine if the object is a world polygon, call **LTRESULT ILTPhysics::IsWorldObject(HOBJECT) **. It returns **LT_YES **if the object is a world polygon or **LT_NO **if otherwise. You can then make logical decisions based on the result.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Prog\Bullets.md)2006, All Rights Reserved.
