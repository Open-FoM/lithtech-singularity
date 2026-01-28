| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# World Rendering

Before rendering can begin, the game code must put LithTech into the rendering state and select which camera to render from. This is usually done from the client shell’s main update loop.

Assuming that you have **hCamera **, an **HCAMERA **variable representing which camera you want to render from:

>
```
g_pLTClient->Start3D();
```

```
       g_pLTClient->ClearScreen(NULL,CLEARSCREEN_RENDER);
```

```
       g_pLTClient->RenderCamera(m_hCamera);
```

```
g_pLTClient->End3D(END3D_CANDRAWCONSOLE);
```

```
g_pLTClient->FlipScreen(NULL);

```

After you have rendered, you need to call FlipScreen with any appropriate parameters, or the new rendering will not be visible.

>
```
GetClientLT()->ClearScreen(NULL,CLEARSCREEN_RENDER);
```

```
GetClientLT()->Start3D();
```

```
GetClientLT()->RenderCamera(hCamera);
```

```
GetClientLT()->StartOptimized2D();
```

```
DoMyHUDDisplay();
```

```
GetClientLT()->EndOptimized2D();
```

```
GetClientLT()->End3D();
```

```
GetClientLT()->FlipScreen()
```

---

## World Polygons

When world polygons are rendered they are queued into one of the following buckets:

**Gouraud: **One texture, non-lightmapped polygons (lighting comes from vertex colors).

**LightMapped: **Two textured polygons (base texture + lightmap).

**Detail Textured: **Three textures (base, lightmap, detail texture).

**Env Mapped: **Three textures (base, lightmap, world env map).

Depending on the graphics card and **force1pass **and **force2pass **console variable settings, world polygons will be rendered in one or two passes, respectively.

In a **force1pass **, detail and environment mapped polygons are not supported. Such polygons will be rendered with only a lightmap.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Render\World.md)2006, All Rights Reserved.
