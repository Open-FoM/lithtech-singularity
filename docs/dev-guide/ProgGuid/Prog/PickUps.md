| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Creating Pickups Tutorial

The Pickups tutorial is an extension of the foundation created by SimplePhys. In this tutorial we add a common game object known as a pickup. This object is generally used for powerup items such as health or armor upgrades. The code for the Pickups tutorial is located in the following directory:

> \Samples\tutorials\pickups.

Most games have interactive game objects with which when touched give you a bigger and better weapon, or increae the amount of armor your character is currently wearing. This is refered to as a pickup or powerup. The normal actions for these objects is to disappear when touched. Pickups can "re-spawn" or "re-appear" after a certain amount of time, and they can play sounds when the pickup is touched to let players know they picked something up.

This section contains the following topics related to creating components to implement pickups:

- [Starting the Tutorial ](#StartingtheTutorial)
- [Touch Notify Messages ](#TouchNotifyMessages)
- [Simple Sound Effects ](#SimpleSoundEffects)
- [Object Messages ](#ObjectMessages)
- [The PF_STATICLIST Property ](#ThePF_STATICLISTProperty)
- [Prehook Plugins ](#PrehookPlugins)

---

## Starting the Tutorial

As with SimplePhys, when you startup Pickups, the only thing visible is text from the console. You need to specify the game type that you want to start. Select from one of the following game types:

- **Normal **—normal single player game. type: normal
- **Server **—start this as a multiplayer server. This is NOT a standalone server. type: server
- **Join **—connect to a server. type: join <ip.ip.ip.ip> or join <computername>

[Top ](#top)

---

## Touch Notify Messages

Touch Notify messages are sent to an object when a collsion between itself and another object has occured. The engine does not pass this message to the object by default.

You will notice in the **PreCreate **method of pickup.cpp that the **FLAG_TOUCH_NOTIFY **flag is specified.

>
```
pStruct->m_Flags = FLAG_VISIBLE | FLAG_TOUCH_NOTIFY;
```

If **m_Flags **is set, then the engine sends then **MID_TOUCHNOTIFY **messages if the object is touched. The following BaseClass function, **EngineMessageFn **, then gets called.

>
```

uint32 PickUp::EngineMessageFn(uint32 messageID, void *pData, float fData)
{
    switch (messageID)
    {
    case MID_PRECREATE:
        return PreCreate(pData, fData);
    case MID_TOUCHNOTIFY:
        return TouchNotify(pData, fData);
    case MID_UPDATE:
        return Update();

    default:
        break;
    }

    // Pass the message along to parent class.
    return BaseClass::EngineMessageFn(messageID, pData, fData);
}

```

[Top ](#top)

---

## Simple Sound Effects

This topic explains how to play a simple sound associated with grabbing a pickup.

The first step in playing a sound is includeing the header file for the sound manager interface.

>
```
#include <iltsoundmgr.h>
```

Next create a **PlaySoundInfo **structure and fill it with the proper information.

>
```

void PickUp::PlaySound()
{
    PlaySoundInfo playSoundInfo;
    PLAYSOUNDINFO_INIT(playSoundInfo);
    HLTSOUND hSnd = LTNULL;

    playSoundInfo.m_dwFlags = PLAYSOUND_ATTACHED | PLAYSOUND_3D;
    playSoundInfo.m_hObject = m_hObject;
    playSoundInfo.m_fInnerRadius = 10.0f;
    playSoundInfo.m_fOuterRadius = 250.0f;
    playSoundInfo.m_nVolume = 100;

    SAFE_STRCPY(playSoundInfo.m_szSoundName, "Snd/pickup.wav");

    LTRESULT hResult = g_pLTServer->SoundMgr()->PlaySound(&playSoundInfo, hSnd);

    if(hResult != LT_OK)
    {
        g_pLTServer->CPrint("(PickUp) Error playing sound");
    }
}

```

In the previous syntax, we are telling the sound that we want it played in 3D space and attached to an object. If the object were moving, the sound would follow the object. We also specify the volume and distance for the sound.

[Top ](#top)

---

## Object Messages

There are many situations within a game, that you might want the objects to pass information to other objects. One option is to get a handle to the object and get the instance of the class to which it is a member. But there are times where you want to pass information to a generic object without wanting to have direct access to the object's members. To do this, you can pass messages to other objects using the following method:

>
```

    // Send this message to the player object
    ILTMessage_Write *pMsg;
    g_pLTSCommon->CreateMessage(pMsg);
    pMsg->Writeuint32(OBJ_MID_PICKUP);
    pMsg->Writeuint8(m_nPickupType);
    pMsg->Writefloat(m_fPickupValue);
    g_pLTServer->SendToObject(pMsg->Read(), m_hObject, hObj, 0);

```

As you may notice, you need to create a message in the same manner that you create a message for any other purpose. You also need an **HOBJECT **handle to the object which is sending the message, and one to the object to which you wish to send the message.

You will also notice above that the **OBJ_MID_PICKUP **is getting pushed onto the message.

>
```

    (in shared/msgids.h)
    enum EObjMessageID
    {
        OBJ_MID_PICKUP = 0
    };

```

This is a value that we have defined in game code which gets referenced by the target object to determine if it has a way to deal with the incomming message. This is done below.

>
```

uint32 CPlayerSrvr::ObjectMessageFn(HOBJECT hSender, ILTMessage_Read *pMsg)
{
	pMsg->SeekTo(0);
	uint32 messageID = pMsg->Readuint32();
	switch(messageID)
	{
            case OBJ_MID_PICKUP:
            {
                uint8 nPickupType   	= pMsg->Readuint8();
                float fPickupValue        = pMsg->Readfloat();
                g_pLTServer->CPrint("PlayerSrvr -> OBJ_MID_PICKUP  %d  %f", nPickupType, fPickupValue);
            }
            break;
        default:
            break;
    }
    return BaseClass::ObjectMessageFn(hSender, pMsg);
}

```

[Top ](#top)

---

## The PF_STATICLIST Property

The **PF_STATICLIST **property is a flag which tells DEdit to create a "dropdown list" component for this object property.

>
```

ADD_STRINGPROP_FLAG(PickupType , "Health", PF_STATICLIST)

```

In the case of our pickup, you may want to support several types such as Health and Armor pickups. Using a static list will allow the levle designers to easily change between our predefined pickup types.

[Top ](#top)

---

## Prehook Plugins

DEdit has sevral very helpful features, among them is the ability to dynamically change lists at DEdit's runtime. You may want to do this if you are planning to store the pickup types in text file instead of hard coding them. This gives you a "hook" to read these values and fill the proper information.

The first part to this is setting up the macros that DEdit uses to find information about the objects. You will notice the **_PLUGIN **suffix to the standard end class macro. This allows you to specify a plugin class for DEdit to use. In the following example this is **CPickupPlugin **.

>
```

END_CLASS_DEFAULT_FLAGS_PLUGIN(PickUp, BaseClass, LTNULL, LTNULL, CF_ALWAYSLOAD, CPickupPlugin)

```

The plugin is derived from the **IObjectPlugin **class. To use **IObjectPlugin **class, you must first include the definition by including iobjectplugin.h. In the class **CPickupPlugin **, overwrite **PreHook_EditStringList **. **PreHook_EditStringList **gets called by DEdit when the object is read in from the world file. This gives you the chance to modify the contents of an object property.

>
```

#include <iobjectplugin.h>
class CPickupPlugin : public IObjectPlugin
{
  public:
	virtual LTRESULT PreHook_EditStringList(
				const char* szRezPath,
				const char* szPropName,
				char** aszStrings,
				uint32* pcStrings,
				const uint32 cMaxStrings,
				const uint32 cMaxStringLength);
};

```

The following implementation looks for the **PickupType **property. If it finds an incomming property with that name, then it adds our types to the list. We have setup **g_aszPickupTypes **in pickup.h with the desired types.

>
```

LTRESULT CPickupPlugin::PreHook_EditStringList(
	const char* szRezPath,
	const char* szPropName,
	char** aszStrings,
	uint32* pcStrings,
	const uint32 cMaxStrings,
	const uint32 cMaxStringLength)
{
	// If prop name is "PickupType", fill in the dropdown with g_aszPickupTypes[]
	if (strcmp("PickupType", szPropName) == 0)
	{
		for (int i=0; i < iPT_NUM_TYPES; i++)
		{
			if (cMaxStrings > (*pcStrings) + 1)	// sanity check
			{
				if (g_aszPickupTypes[i][0])		// skip null strings
					strcpy(aszStrings[(*pcStrings)++], g_aszPickupTypes[i]);
			}
		}
		return LT_OK;
	}
	return LT_UNSUPPORTED;
}
```

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Prog\PickUps.md)2006, All Rights Reserved.
