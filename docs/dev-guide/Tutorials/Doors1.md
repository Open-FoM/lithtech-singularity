| ### Programming Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# —Tutorial—
Creating Doors

This section shows how to create a simple door that opens when touched by the player or a projectile. It also allows placement of the door through the DEdit application. The Doors sample can be found in the following directory:

> \Samples\tutorials\doors.

This section contains the following topics related to the creation of a simple door:

- [Starting the Tutorial ](#StartingtheTutorial)
- [Server-side Changes ](#ServersideChanges)

---

## Starting the Tutorial

As with SpecialEffects1, when you startup the sample file for the Doors Tutorial, the only thing visible is text from the console. You need to specify the game type that you want to start. The options are listed below:

- **Normal— **normal single player game. type: normal
- **Server— **start this as a multiplayer server. This is NOT a standalone server. type: server
- **Join— **connect to a server. type: join <ip.ip.ip.ip> or join <computername>

---

## Server-side Changes
When creating a door using the techniques described in this tutorial, the only required change to your server-side game code is the addition of a "door" class. This object must be setup to support placement using DEdit. Your door opens when touched by the player, or a projectile.

#### Door.cpp

Just as with any DEdit-placable game object, you must setup the macro block with the proper information. You should notice the **CF_WORLDMODEL **flag in the closing macro. This lets DEdit and Processor know that this object should be treated as a worldmodel, and hence has geometry associated with it.

>
```

BEGIN_CLASS(Door)
ADD_REALPROP(MaxOpenAngle, 90.0f) // The angle offset when the door is fully open
ADD_VECTORPROP(HingeOffset) // Offset from the center of the door, to the rotation point of the door
ADD_REALPROP(UpdateRate, 0.0333f) // Time delay, in milliseconds between updates
ADD_REALPROP(DegreesPerUpdate, 2.5f) // How many degrees to rotate each update
END_CLASS_DEFAULT_FLAGS(Door, BaseClass, LTNULL, LTNULL, CF_ALWAYSLOAD | CF_WORLDMODEL)

```

The **PreCreate **function of your Door contains some important setup information. Most importantly, you must inform the engine that this object is a WorldModel by specifying the **OT_WORLDMODEL **object type flag. You must also set up the following special flags on your door object:

- Use **FLAG_GOTHRUWORLD **because, as your door rotates, it is very likely that it will clip the world slightly. So you want to disable door to world collisions.
- Use **FLAG_TOUCH_NOTIFY **to allow the player to open the door by simply touching it.
- Use **FLAG_FULLPOSITION **to ensure that the position information sent to the client is as accurate as possible.
- Use **FLAG2_DISABLEPREDICTION **to force the client to not assume the position of the door since you are performing special movement patterns.

The following code shows the implementation of the **PreCreate **method.

>
```

uint32 Door::PreCreate(void *pData, float fData)
{
    // Let parent class handle it first
    BaseClass::EngineMessageFn(MID_PRECREATE, pData, fData);

    // Cast pData to a ObjectCreateStruct* for convenience
    ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

    // Set the object type to OT_NORMAL
    pStruct->m_ObjectType = OT_WORLDMODEL;

    pStruct->m_Flags |= (FLAG_SOLID
			| FLAG_VISIBLE
			| FLAG_GOTHRUWORLD
			| FLAG_TOUCH_NOTIFY
			| FLAG_FULLPOSITIONRES);
    pStruct->m_Flags2 |= FLAG2_DISABLEPREDICTION;
...
}

```

The movement of your door is managed by the **Door::Update **method. Your door object has four states, Open, Closed, Opening, Closing. The following code shows the implementation of the **Update **method.

>
```

uint32 Door::Update()
{
    switch(m_nMode)
    {
    case DOOR_OPENING:
        {
            m_fOpenRot -= m_fDegreesPerUpdate;
            if((m_fOpenRot - m_fDegreesPerUpdate) < 0.0f)
            {
		// Place door in fully opened position
		OnMoveDone(DOOR_OPEN, 0.0f, m_rOpenRot);
                break;
            }

	// Continue opening door. If the door is touched during this,
	// then close the door
	OnMove(m_fDegreesPerUpdate, DOOR_CLOSING);
        }
        break;
    case DOOR_CLOSING:
        {
            m_fOpenRot += m_fDegreesPerUpdate;
            if((m_fOpenRot + m_fDegreesPerUpdate) > m_fMaxOpenAngle)
            {
		// Place door in fully closed position
		OnMoveDone(DOOR_CLOSED, m_fMaxOpenAngle, m_rClosedRot);
                break;
            }

	// Continue closing door. If the door is touched during this,
	// then open the door
	OnMove(-m_fDegreesPerUpdate, DOOR_OPENING);
        }
        break;
    case DOOR_OPEN:
        {
            m_nMode = DOOR_CLOSING;
            g_pLTServer->SetNextUpdate(m_hObject, m_fUpdateRate);
        }
        break;
    default:
        g_pLTServer->SetNextUpdate(m_hObject, 0.0f);
        break;
    }

    return 1;
}

```

When your door finishes a movement cycle (of either opening or closing completely) then call **Door::OnMoveDone **to set the final resting position of the door. The following code shows the implementation of the **OnMoveDone **method.

>
```

void Door::OnMoveDone(int nState, float fAngle, LTRotation &rMode)
{
    m_nMode = nState;
    m_fOpenRot = fAngle;
    LTRotation rRot;
    LTVector vPos;
    g_pLTServer->GetObjectRotation(m_hObject, &rRot);

    // Calculate our final position
    LTMatrix mRotation;	
    rRot = m_hackInitialRot * rMode * ~m_hackInitialRot;
    rRot.ConvertToMatrix( mRotation );
    vPos = (mRotation * m_vHingeOffset) + (m_vClosedPos - m_vHingeOffset);

    // Set our position and rotation
    g_pLTServer->SetObjectPos(m_hObject, &vPos);
    g_pLTServer->SetObjectRotation(m_hObject, &rMode);
    g_pLTServer->SetNextUpdate(m_hObject, 1.0f);
}

```

While the door is in motion, whether opening or closing, call your **Door::OnMove **method. The following code shows the implementation of the **OnMove **method.

>
```

void Door::OnMove(float fDegrees, int nOnInterrupt)
{
    // Get the current rotation
    LTRotation rRot;
    g_pLTServer->GetObjectRotation(m_hObject, &rRot);

    // Set our new rotation
    rRot.Rotate(rRot.Up(), DegreesToRadians(fDegrees));
    g_pLTServer->RotateObject(m_hObject, &rRot);

    // Get our current possition
    LTVector vPos;
    g_pLTServer->GetObjectPos(m_hObject, &vPos);
    // Save our current position.
    LTVector vOldPos;	
    vOldPos = vPos;
    // Calculate the position to move to.
    LTMatrix	mRotation;	
    rRot = m_hackInitialRot * rRot * ~m_hackInitialRot;
    rRot.ConvertToMatrix( mRotation );
    vPos = (mRotation * m_vHingeOffset) + (m_vClosedPos - m_vHingeOffset);

    // Try to move the object
    g_pLTServer->MoveObject(m_hObject, &vPos);

    // Check to see if anything is in our way.
    // If it is, set the mode opposite of the current mode,
    // and reset the position.
    LTVector	vCurPos;
    g_pLTServer->GetObjectPos( m_hObject, &vCurPos );
    if( !vCurPos.NearlyEquals( vPos, MATH_EPSILON ) )
    {
        m_nMode = nOnInterrupt;
        g_pLTServer->SetObjectPos(m_hObject, &vOldPos);
    }
    g_pLTServer->SetNextUpdate(m_hObject, m_fUpdateRate);
}

```

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Tutorials\Doors1.md)2006, All Rights Reserved.
