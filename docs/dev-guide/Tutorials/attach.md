| ### Programming Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Attachments Sample

The attachments sample builds upon the foundation of samplebase, and is located in the following directory:

>
\Samples\base\attachments.

The Attachments sample demonstrates the use of model attachments.

## Server-side Changes

### class PropModel
The attachments sample adds only one class, the PropModel class, to the samplebase foundation. The PropModel class contains a fully working example of how to make an attachment, break an attachment, and attach different objects to one socket.

Three "helper" functions exist to allow easy reuse of resources within PropModel. This section describes and shows the source code for each of the helper functions.

#### The CreateAttachmentObject Function

The first of helper function is the CreateAttachmentObject function. This function simply instantiates a model from the specified input parameters. The following source code shows the implementation of the CreateAttachmentObject function.

>
```

void Propmodel::CreateAttachmentObject(HOBJECT &hObj, const char* sFilename, const char* sTexture)
{
    ObjectCreateStruct objCreateStruct;
    HCLASS hClass = g_pLTServer->GetClass("BaseClass");

    if (!hClass)
    {
        assert(!"ERROR: Couldn't get BaseClass!!");
        return;
    }

    objCreateStruct.Clear();
    objCreateStruct.m_ObjectType = OT_MODEL;	
    objCreateStruct.m_Flags = FLAG_VISIBLE;

    strncpy(objCreateStruct.m_Filename, sFilename, 127);
    strncpy(objCreateStruct.m_SkinNames[0], sTexture, 127);
    BaseClass* m_pObj = (BaseClass*)g_pLTServer->CreateObject(hClass, &objCreateStruct);

    if (m_pObj)
    {
        hObj = m_pObj->m_hObject;
        g_pLTServer->CPrint("Attachment Object Created!");
    }
}

```

#### The CreateAttachment Function

The next helper function is the CreateAttachment function. This function is responsible for making the actual attachment between the two input models. The following source code shows the implementation of the CreateAttachment function.

>
```

void Propmodel::CreateAttachment(HATTACHMENT &hAttachment,
                                 HOBJECT hChildObject,
                                 const char* sSocket,
                                 LTVector &vRotOffset,
                                 LTVector &vPosOffset)
{
    LTVector vOffset;
    LTRotation rOffset;

    vOffset.Init();
    rOffset.Init();
    hAttachment = NULL;

    HMODELSOCKET hSocket;
    LTRESULT SocketResult = g_pLTSModel->GetSocket(m_hObject, sSocket, hSocket);
    if(LT_OK == SocketResult)
    {
        g_pLTServer->CPrint("Got the socket");
    }else
    {
        g_pLTServer->CPrint("GetSocket failed: %s", LTRESULT_TO_STRING(SocketResult));
    }

    LTransform tSocketTransform;
    LTRESULT SocketTrResult = g_pLTSModel->GetSocketTransform(m_hObject, hSocket, tSocketTransform, LTTRUE);
    if(LT_OK == SocketTrResult)
    {
        g_pLTServer->CPrint("Got the transform");
    }else
    {
        g_pLTServer->CPrint("GetTrasform failed: %s", LTRESULT_TO_STRING(SocketTrResult));
    }
    // We need to adjust the rotation of our weapon before we make the attachment
    rOffset.Rotate(rOffset.Right(), MATH_DEGREES_TO_RADIANS(vRotOffset.x));
    rOffset.Rotate(rOffset.Up(), MATH_DEGREES_TO_RADIANS(vRotOffset.y));
    rOffset.Rotate(rOffset.Forward(), MATH_DEGREES_TO_RADIANS(vRotOffset.z));
    LTRESULT resultAttachment = g_pLTServer->CreateAttachment(m_hObject,
        hChildObject,
        sSocket,
        &vOffset,
        &rOffset,
        &hAttachment);

    if(LT_OK != resultAttachment)
    {
        g_pLTServer->CPrint("Error creating attachment!!");
    }
}

```

The actual attachment gets created with the following engine call.

>
```

    LTRESULT resultAttachment = g_pLTServer->CreateAttachment(m_hObject,
        hChildObject,
        sSocket,
        &vOffset,
        &rOffset,
        &hAttachment);

```

To remove an attachment, the following call is made.

>
```

    g_pLTServer->RemoveAttachment(m_hAttachment);

```

#### The Update Function

The PropModel::Update() function is responsible for controlling the playing of animations and which attachments are made. The following source code shows the implementation of the Update function.

>
```

uint32 Propmodel::Update()
{
    // Animation sequence
    switch(m_nAnimStep)
    {
    case 0:
        PlayAnimation("LStR");
        ++m_nAnimStep;
        break;

    case 1:
        PlayAnimation("LStL");
        ++m_nAnimStep;
        break;
    case 2:
        PlayAnimation("LRF");
        ++m_nAnimStep;
        // Let's run for 3 seconds instead of 2
        g_pLTServer->SetNextUpdate(m_hObject, 3.0f);
        return 1;
        break;
    case 3:
        PlayAnimation("LSt");
        ++m_nAnimStep;
        break;
		
    case 4:
	{
		PlayAnimation("LSt");
		g_pLTServer->RemoveAttachment(m_hAttachment);
		g_pLTServer->RemoveObject(m_hCrossbow);
		CreateAttachmentObject(m_hBillyclub, "Models\\billyclub.ltb", "ModelTextures\\club.dtx");
		LTVector vRotOffset(25.0f, 25.0f, 0.0f);
		LTVector vPosOffset(0.0f, 0.0f, 0.0f);
		CreateAttachment(m_hAttachment, m_hBillyclub, "RightHand", vRotOffset, vPosOffset);
		++m_nAnimStep;
	}
        break;
		
    case 5:
	{	
		PlayAnimation("LC");
		g_pLTServer->SetNextUpdate(m_hObject, 2.0f);
		++m_nAnimStep;
		return 1;
	}
       break;
    default:
	{
		m_nAnimStep = 0;
		PlayAnimation("LSt");
		g_pLTServer->RemoveAttachment(m_hAttachment);
		g_pLTServer->RemoveObject(m_hBillyclub);
		CreateAttachmentObject(m_hCrossbow, "Models\\crossbow.ltb", "ModelTextures\\crossbow.dtx");
		LTVector vRotOffset(25.0f, 25.0f, 0.0f);
		LTVector vPosOffset(0.0f, 0.0f, 0.0f);
		CreateAttachment(m_hAttachment, m_hCrossbow, "RightHand", vRotOffset, vPosOffset);
	}
        break;
    }
    g_pLTServer->SetNextUpdate(m_hObject, 2.0f);
    return 1;
}

```

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Tutorials\attach.md)2006, All Rights Reserved.
