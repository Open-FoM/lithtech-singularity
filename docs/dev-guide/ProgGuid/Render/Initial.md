| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Initialization

Rendering initialization is a two-step process:

1. Define console variables.
2. Set the rendering mode with ILTClient::SetRenderMode.

## Define Console Variables

Console variables are defined using console commands. The console variables used for rendering include:

- BitDepth
- ScreenHeight
- ScreenWidth
- Windowed

## Set the rendering mode with ILTClient::SetRenderMode

To set the rendering mode (including display drivers, devices, and display mode) use the **ILTClient::SetRenderMode **function:

>
```
LTRESULT( * ILTClient :: SetRenderMode)(RMode *pMode)
```

The **pMode **parameter is a **RMode **structure containing the relevant video card information:

>
```
struct RMode
```

```
{
```

```
       LTBOOL m_bHardware;
```

```
       char m_InternalName[128];
```

```
       char m_Description[128];
```

```
       uint32 m_Width, m_Height, m_BitDepth;
```

```
       RMode *m_pNext;
```

```
);
```

The **m_bHardware **element is a Boolean value indicating whether or not this **RMode **structure supports hardware rasterization. The **m_Description **element is a friendly string describing the video card. The **m_Width **, **m_Height **, and **m_BitDepth **elements define the display mode. The **m_pNext **element is a pointer to the next **RMode **structure (this can be used to cycle through the list of **RMode **structures).

The **ILTClient::GetRenderModes **function returns a list of valid render modes. After reviewing the render modes with this function, call the **ILTClient::RelinquishRenderModes **function to release the **RMode **structure list.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Render\Initial.md)2006, All Rights Reserved.
