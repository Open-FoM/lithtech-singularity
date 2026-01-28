| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Custom Rendering

LithTech handles rendering of the world models and imported models. However, LithTech does expose some lower-level interfaces that provide custom rendering of specific, effects. This section discusses the following lower-level custom rendering interfaces:

- [ILTDrawPrim ](#ILTDrawPrim): Renders arbitrary primitives, including triangles, quads, lines, points, strips and fans.
- [ILTTexInterface ](#ILTTexInterface): Creates and modifies textures.
- [Canvases ](#Canvases): Used to get callbacks at object render time (for sorted alpha models).

---

## ILTDrawPrim

The **ILTDrawPrim **interface exposes low-level primitive rendering to the game code. **ILTDrawPrim **functions can be used to draw arbitrary polygons, including triangles, quads, strips, lines, points, and fans. These primitives can be flat-shaded or Gouraud-shaded, and also textured or untextured. Alpha blending is supported in both textures and vertex colors.

**ILTDrawPrim **is not only useful for rendering special polygons that will appear in the final game, but it can also be used for debugging purposes. Various types of debugging geometry can be used to test an object’s size, collisions, or just to test the appearance of a texture.

**ILTDrawPrim **uses structures to contain relevant vertex and polygon information. These structures are defined in the iltdrawprim.h header file.

### Vertex Structures

The first set of structures defined contain vertex information. These various vertex structures provide a base upon which the polygon structures depend.

For all polygon structures, the vertex structures contain the coordinate information and the Gouraud-shading color information. For textured polygons, the vertex structures contain the u and v information.

Flat-shaded vertex structures do not contain color data and only represent the location of the vertex. The color of a flat-shaded polygon is set in the polygon structure.

The vertex structures include **LT_VERTRGBA **, **LT_VERTF **, **LT_VERTFT **, **LT_VERTG **, and **LT_VERTGT **.

### Line Structures

Structures defining primitive lines include **LT_LINEF **, **LT_LINEFT **, **LT_LINEG **, and **LT_LINEGT **.

### Triangle Structures

Structures defining primitive triangles include **LT_POLYF3 **, **LT_POLYFT3 **, **LT_POLYG3 **, and **LT_POLYGT3 **.

### Quadrilateral Structures

Structures defining primitive quadrilaterals include **LT_POLYF4 **, **LT_POLYFT4 **, **LT_POLYG4 **, and **LT_POLYGT4 **.

### Accessing the ILTDrawPrim Interface

The polygon structures listed above are available for creation when the LithTech header files are included in a project. Before calling the **ILTDrawPrim **functions, however, the game code must acquire a handle to LithTech’s instantiation of the **ILTDrawPrim **interface. The game code should not instantiate **ILTDrawPrim **.

The Interface Manager exposes one **ILTDrawPrim **instance for the game code to access. The game code can acquire a handle to this instance by associating it with an **ILTDrawPrim **pointer using the **define_holder **macro (defined in ltmodule.h).

The code snippet below first creates an **ILTDrawPrim **pointer ( **g_pILTDrawPrim **). The define_holder macro is then used to set the address of the **g_pILTDrawPrim **pointer to the location for the **ILTDrawPrim **instance.

>
```
#include “iltdrawprim.h”
```

```
static ILTDrawPrim *g_pILTDrawPrim;
```

```
define_holder(ILTDrawPrim, g_pILTDrawPrim);
```

After setting the correct address for the **g_pILTDrawPrim **pointer as shown above, the **ILTDrawPrim **functions can be called.

| **Note: ** | LithTech uses another unexposed instantiation of the **ILTDrawPrim **interface to provide internal 2D rendering, text and user interface, and console functionality |
| --- | --- |

### Primitive Structure Helper Functions

The **ILTDrawPrim **interface provides several helper functions to quickly set values for primitive quadrilateral structures. These helper functions include **SetALPHA **, **SetALPHA4 **, **SetRGB **, **SetRGBA **, **SetRGB4 **, **SetRGBA4 **, **SetUV4 **, **SetUVWH **, **SetXY4 **, and **SetXYWH **.

These functions are overloaded for two or more quadrilateral structures.

### State-setting Functions

The **ILTDrawPrim **interface includes a series of functions to set various states for drawing primitives. The game code only references one **ILTDrawPrim **instance. Therefore, each **DrawPrim* **call implements the same set of states. Whereas these states may be modified by any number of areas in the game code, the current state settings cannot be guaranteed. Therefore, states should be set (or reset) before each block of **DrawPrim **calls to insure that the desired states are actually used.

The state-setting functions include **SetAlphaBlendMode **, **SetAlphaTestMode **, **SetCamera **, **SetClipMode **, **SetColorOp **, **SetCullMode **, **SetFillMode **, **SetTexture **, **SetTransformType **, and **SetZBufferMode **.

### Draw Functions

The **ILTDrawPrim **interface exposes several overloaded drawing functions, including **DrawPrim **, **DrawPrimPoint **, **DrawPrimFan **, and **DrawPrimStrip **. When one of these functions is called, the primitive is immediately rendered to the screen.

The current state settings (described above) identify the characteristics of any primitive drawn with one of the **DrawPrim* **functions. The states are not protected from modification from other areas of the code. If you call other functions, they may mofidy the current states in **DrawPrim **. Therefore, all of the states should be set before each block of **DrawPrim **, **DrawPrimPoint **, **DrawPrimFan **, or **DrawPrimStrip **function calls.

### Checkerboard Sample Code

The following code sample implements various functions of the ILTDrawPrim interface to render a checkerboard pattern onto the screen.

>
```
#include "iltdrawprim.h"
```

```
 
```

```
// acquire the interface from the Interface Manager
```

```
static ILTDrawPrim *g_pLTDrawPrim;
```

```
define_holder(ILTDrawPrim, g_pLTDrawPrim);
```

```
 
```

```
void DrawCheckerboard()
```

```
{
```

```
       // initialize
```

```
       uint32 idx;
```

```
       uint32 x, y;
```

```
       uint32 tileWidth, tileWeight;
```

```
       uint32 screenWidth, screenHeight;
```

```
       LT_POLYF4 pPolyArray[25]; // a 5 x 5 grid of 4-sided    polygons
```

```
 
```

```
       g_pLTClient->GetSurfaceDims(g_pLTClient->GetScreenSurface(),
```

```
              &screenWidth, &screenHeight);
```

```
 
```

```
       tileWidth = screenWidth / 5;
```

```
       tileWeight = screenHeight / 5;
```

```
 
```

```
       for (y=0; y<5; y++)
```

```
       {
```

```
              for (x=0; x<5; x++)
```

```
              {
```

```
                     idx = y * 5 + x;
```

```
 
```

```
                     // set white or black color
```

```
                     pPolyArray[idx].rgba.r = 255 * (idx % 2);
```

```
                     pPolyArray[idx].rgba.g = 255 * (idx % 2);
```

```
                     pPolyArray[idx].rgba.b= 255 * (idx % 2);
```

```
                     pPolyArray[idx].rgba.a= 255;
```

```
 
```

```
                     // vertices are clockwise from upper left
```

```
                     pPolyArray[idx].verts[0].x = float(x * tileWidth);
```

```
                     pPolyArray[idx].verts[0].y = float(y * tileWeight);
```

```
                     pPolyArray[idx].verts[0].z = 0;
```

```
 
```

```
                     pPolyArray[idx].verts[1].x = float(x * tileWidth + tileWidth);
```

```
                     pPolyArray[idx].verts[1].y = float(y * tileWeight);
```

```
                     pPolyArray[idx].verts[1].z = 0;
```

```
 
```

```
                     pPolyArray[idx].verts[2].x = float(x * tileWidth + tileWidth);
```

```
                     pPolyArray[idx].verts[2].y = float(y * tileWeight + tileWeight);
```

```
                     pPolyArray[idx].verts[2].z = 0;
```

```
 
```

```
                     pPolyArray[idx].verts[3].x = float(x * tileWidth);
```

```
                     pPolyArray[idx].verts[3].y = float(y * tileWeight + tileWeight);
```

```
                     pPolyArray[idx].verts[3].z = 0;
```

```
              }
```

```
       }
```

```
 
```

```
       // render (each frame)
```

```
 
```

```
       // set up the render state
```

```
       g_pLTDrawPrim->SetTransformType(DRAWPRIM_TRANSFORM_SCREEN);
```

```
       g_pLTDrawPrim->SetColorOp(DRAWPRIM_NOCOLOROP);
```

```
       g_pLTDrawPrim->SetAlphaBlendMode(DRAWPRIM_NOBLEND);
```

```
       g_pLTDrawPrim->SetZBufferMode(DRAWPRIM_NOZ);
```

```
       g_pLTDrawPrim->SetAlphaTestMode(DRAWPRIM_NOALPHATEST);
```

```
       g_pLTDrawPrim->SetClipMode(DRAWPRIM_FASTCLIP);
```

```
       g_pLTDrawPrim->SetFillMode(DRAWPRIM_FILL);
```

```
                    
```

```
       // draw our checkerboard array
```

```
       g_pLTDrawPrim->DrawPrim(pPolyArray, 25);
```

```
}
```

[Top ](#top)

---

## ILTTexInterface

The **ILTTexInterface **interface exposes texture manipulation functionality to the game code. It provides direct access to texture data. This direct access means that the texture won’t be converted to a new format, and will therefore result in faster texture modification.

The **ILTTexInterface **and associate types are defined in the ilttexinterface.h header file.

### Create a Texture From a DTX File

The ILTTexInterface:: **CreateTextureFromName **function is used to create a texture from a .DTX file. A .DTX file is created using DEdit. This function will always create a new internal texture from the given filename. However, it may read the file data from its internal resource cache. Often you will want to call FindTextureFromName to find out if a texture is currently loaded, and then call **CreateTextureFromName **to load it if it is not.

>
```
LTRESULT CreateTextureFromName
```

```
(
```

```
       HTEXTURE &hTexture,
```

```
       const char *pFilename
```

```
);
```

The **pFilename **parameter points to a string containing the path to the resource directory in which the .DTX file is located.

The address of the created texture is returned in the **hTexture **out-parameter.

If the .DTX file was created with the **DTX_32BITSYSCOPY **bit set, the texture will not be converted by LithTech to a platform compatible format.

To determine the format type of the new texture, use the [ILTTexInterface::GetTextureType ](#GetTextureType).

### Create a Texture From Data

The first version of the overloaded **ILTTexInterface::CreateTextureFromData **function will create a texture from information passed in the pData parameter.

>
```
LTRESULT CreateTextureFromData
```

```
(
```

```
       HTEXTURE &hTexture,
```

```
       ETextureType eTextureType,
```

```
       uint32 TextureFlags,
```

```
       uint8 *pData,
```

```
       uint32 nWidth,
```

```
       uint32 nHeight,
```

```
       uint32 nAutoGenMipMaps = 1
```

```
);
```

The texture data from which to create the texture is passed into the function in the **pData **pointer.

The width and height are passed in the **nWidth **and **nHeight **parameters, respectively.

The **nAutoGenMipMaps **parameter sets the number of mip maps to create and populate for this texture. If it is set to 0, mip maps all the way down to 1x1 will be created and populated.

The created texture is returned to the **hTexture **out-parameter.

The **eTextureType **parameter specifies the bitdepth and format of the texture to create.

The **TextureFlags **parameter is a combination of flags that instruct the function to create the texture in a specific manner. It may be a combination of one or more of the following values:

- **DTX_32BITSYSCOPY **: If there is a system copy of the texture don't convert it to a device specific format (keep it 32 bit).
- **DTX_FULLBRITE **: Use fullbright colors.
- **DTX_MIPSALLOCED **: TextureMipData texture data is allocated.
- **DTX_NOSYSCACHE **: Do not save the texture in the texture cache list.
- **DTX_PREFER16BIT **: Use 16-bit, even if in 32-bit mode.
- **DTX_PREFER4444 **: If in 16-bit mode, use a 4444 texture.
- **DTX_PREFER5551 **: If in 16-bit mode, use 5551 texture.
- **DTX_SECTIONSFIXED **: Set for fixed sections counts.

### Create a Palettized Texture from Data

The second version of the overloaded **ILTTexInterface::CreateTextureFromData **function creates a palettized texture from information passed in the **pData **parameter.

>
```
LTRESULT CreateTextureFromData
```

```
(
```

```
       HTEXTURE &hTexture,
```

```
       ETextureType eTextureType,
```

```
       uint32 TextureFlags,
```

```
       uint8 *pData,
```

```
       uint8 *pPalette,
```

```
       uint32 nWidth,
```

```
       uint32 nHeight,
```

```
       uint32 nAutoGenMipMaps = 1
```

```
);
```

All of the parameters for this version of the function perform as for the first version of the function (as explained above). The new in-parameter: the **pPalette **pointer, points to the palette to use for the texture.

### Create an Empty Texture

Empty textures can be created using the ILTTexInterface::CreateEmptyTexture.

>
```
LTRESULT CreateEmptyTexture
```

```
(
```

```
       HTEXTURE &hTexture,
```

```
       ETextureType eTextureType,
```

```
       uint32 TextureFlags,
```

```
       uint32 nWidth,
```

```
       uint32 nHeight,
```

```
       uint32 nAutoGenMipMaps = 1
```

```
);
```

There are two differences between this function and the **ILTTexInterface::CreateTextureFromName **function:

- This function lacks the **pData **parameter.
- The mip maps for an empty texture will not be populated.

Texture data can be applied to an empty texture by using the **ILTTexInterface:GetTextureData **function to get a pointer to the empty data location. Texture data can then be assigned to it.

To populate the mip maps, use the **ILTTexInterface::AutoGenMipMaps **function.

### Find a Texture

The **ILTTexInterface::FindTextureFromName **function retrieves a handle to an existing texture given a filename. This function enables you to modify an existing texture and all polygons that use this texture will reflect the changes you make.

>
```
LTRESULT FindTextureFromName
```

```
(
```

```
       HTEXTURE &hTexture,
```

```
       const char *pFilename
```

```
);
```

The **pFilename **parameter points to a string containing the relative path to the resource directory in which the .DTX file is located.

The address of the texture is returned in the **hTexture **out-parameter.

### Modify Texture Data

Texture data can be modified using the first version of the overloaded **ILTTexInterfaces::GetTextureData **function. This function is useful for assigning texture data to a texture that was created with the **ILTTexInterface::CreateEmptyTexture **function, or modifying an existing texture.

>
```
LTRESULT GetTextureData
```

```
(
```

```
       const HTEXTURE hTexture,
```

```
       bool bReadOnly,
```

```
       uint8* &pData,
```

```
       uint32 &nPitch,
```

```
       uint32 nMipMap = 0
```

```
);
```

The texture to be modified is passed into the function using the **hTexture **parameter.

The **bReadOnly **in-parameter indicates whether the data is read only or read/write. To modify the texture data, **bReadOnly **must be set to false.

The **nMipMap **in-parameter identifies the texture or mip map to retrieve. If this is set to 0, the **pData **parameter will point to the base texture. If this is set to 1 or more, the **pData **parameter will point to the appropriate mip map.

The **pData **out-parameter is a pointer to the texture data location. The data can be modified or overwritten if the **bReadOnly **parameter is set to false.

The **nPitch **out-parameter is the size in bytes of a row of data in the texture.

The game code must inform LithTech when it is finished modifying the texture data. This is done by using the **ILTTexInterface::FlushTextureData **function described in the *Finish Modifying Texture Data *section.

### Modify Palettized Texture Data

Palette data for a texture can be modified using the second version of the overloaded **ILTTexInterfaces::GetTextureData **function.

LTRESULT GetTextureData

(

const HTEXTURE hTexture,

bool bReadOnly,

uint8* &pData,

uint8* &pPalette,

uint32 &nPitch,

uint32 nMipMap = 0

);

This function is the same as the first version except that it adds the **pPalette **out-parameter. This parameter is the address of the palette being used by the texture. This data can be modified or overwritten if the **bReadOnly **parameter is set to false.

The game code must inform LithTech when it is finished modifying the texture data. This is done by using the ILTTexInterface:: **FlushTextureData **function. For more information about the FlushTextureData function, see [Finish Modifying Texture Data ](#FinishModifyingTextureData).

### Add Detail to a Texture

Detail textures can be added to a texture using the **ILTTexInterface::SetDetailTexture **function.

>
```
LTRESULT SetDetailTexture
```

```
(
```

```
       HTEXTURE &hTexture,
```

```
       const HTEXTURE hRefTex,
```

```
       ETexRefType eDetailTexType
```

```
);
```

The **hTexture **parameter sets the texture to which the detail will be added.

The **hRefTex **parameter is the detail texture to add to the base texture.

The **eDetailTexType **is an **ETexRefType **value setting the detail texture’s type.

Use the **ILTTexInterface::GetDetailType **function to retrieve a texture’s detail texture.

### Finish Modifying Texture Data

While the game code is modifying the texture data (using either version of the overloaded **ILTTexInterface::GetTextureData **function), LithTech will not use that texture. Use the **ILTTexInterface::FlushTextureData **function to inform LithTech that the texture is no longer being modified.

>
```
virtual LTRESULT FlushTextureData
```

```
(
```

```
       const HTEXTURE hTexture,
```

```
       ETextureMod eChanged = TEXTURE_DATAANDPALETTECHANGED,
```

```
       uint32 nMipMap = 0
```

```
)=0;
```

The modified texture is identified with the **hTexture **in-parameter.

The **eChanged **in-parameter is a **ETextureMod **enumerated type. It may be one of the following values:

- **TEXTURE_DATACHANGED **—Only the texture data (the **pData **parameter) was changed.
- **TEXTURE_PALETTECHANGED **—Only the texture’s palette (the **pPalette **parameter) was changed.
- **TEXTURE_DATAANDPALETTECHANGED **—Both the texture data and palette data were changed.

The **nMipMap **parameter indicates individual mip map that is no longer being modified. A value of 0 indicates the base texture is no long being modified. This must match the **nMipMap **parameter value of the **GetTextureData **call used to modify the texture.

### Automatically Populate Mip Maps

Mip maps can be populated with texture data using the **ILTTexInterface::AutoGenMipMaps **function. Storage sapce for the mip maps must already have been allocated using the **ILTTexInterface::CreateTextureFromName **, **CreateTextureFromData **, or **CreateEmptyTexture **functions.

>
```
LTRESULT AutoGenMipMaps
```

```
(
```

```
       const HTEXTURE hTexture,
```

```
       uint32 nAutoGenMipMaps = 1
```

```
);
```

The **hTexture **parameter identifies the texture for which the mip maps will be automatically populated.

The **nAutoGenMipMaps **parameter sets how many mip maps will be populated, starting with the mip map closest to the base texture. This number must be equal to or less than the number of mip maps that exist for the texture. If this parameter is set to 0, all existing mip maps for the texture will be populated.

Existing mip map data will be overwritten by this function.

### Set Mip Map Range

At times, it may be desirable to confine the usage of a texture’s mip maps to a subset of all mip maps available. This can be accomplished using the **ILTTexInterface::SetTextureMipMapRange **function.

>
```
LTRESULT SetTextureMipMapRange
```

```
(
```

```
       const HTEXTURE hTexture,
```

```
       uint32 nMaxMipMaps,
```

```
       uint32 nMinMipMaps
```

```
);
```

The **hTexture **parameter identifies the texture for which the mip map range is set.

The **nMinMipMaps **parameter sets the smallest mip map to use for the texture. The value is actually a number of mip maps from the base texture (which has a value of 0).For example, the third mip map for a texture has a value of 3.

The **nMaxMipMaps **parameter sets the largest mip map to use for the texture. The value is actually a number of mip maps from the base texture.

Only mip maps of sizes between (inclusive) the **nMipMaps **and **nMaxMipMaps **values will be used in the texture.

### Release Texture

When the game code is done with a texture, the **HTEXTURE **should be released to free up system resources. Call the **ILTTexInterface::ReleaseTextureHandle **to free the **HTEXTURE **identified by the **hTexture **parameter.

>
```
bool ReleaseTextureHandle
```

```
(
```

```
       const HTEXTURE hTexture
```

```
);
```

If the **HTEXTURE **is still being used by LithTech, it will not be immediately freed. After LithTech is through with the **HTEXTURE **, it will be freed.

### ILTTexInterface Review Functions

The **ILTTexInterface **class exposes several functions that can be used to review current information concerning a particular texture. Each of these functions takes an **HTEXTURE **parameter identifying the texture for which information is desired. The desired data is returned in one or more out-parameters.

**GetDetailTexture **—Retrieves a HTEXTURE (the hRefTex out-parameter) to the detail texture of the texture. The type (an ETexRefType enumerated type) of detail texture is also returned in the eDetailTexType out-parameter. The detail texture can be set using the ILTTexInterface:SetDetailTexture function.

**GetTextureMipMapRange **—Retrieves the minimum number of mip maps (the nMinMipMaps out-parameter) and the maximum number of mip maps (the nMaxMipMaps out-parameter) for the texture.

**GetTextureNumMipMaps **—Retrieves the number of mip maps (the nMipMaps out-parameter) for the texture.

**GetTextureDims **—Retrieves the current pixel width and pixel height (the nWidth and nHeight out-parameters) of the texture.

**GetTextureFlags **—Retrieves the flags (the TextureFlags out-parameter) set for the texture. The TextureFlags parameter can be a combination of one or more of the following values:

**DTX_32BITSYSCOPY **—If there is a system copy of the texture don't convert it to a device specific format (keep it 32 bit).

**DTX_FULLBRITE **—Use fullbrite colors.

**DTX_MIPSALLOCED **—TextureMipData texture data is allocated.

**DTX_NOSYSCACHE **—Do not save the texture in the texture cache list.

**DTX_PREFER16BIT **—Use 16-bit, even if in 32-bit mode.

**DTX_PREFER4444 **—If in 16-bit mode, use a 4444 texture.

**DTX_PREFER5551 **—If in 16-bit mode, use 5551 texture.

**DTX_SECTIONSFIXED **—Set for fixed sections counts.

**GetTextureType **—This function retrieves the texture format type (the eTextureType out-parameter) of a texture. The possible types are defined by the ETextureType enumerated type and include:

**TEXTURETYPE_INVALID **—The HTEXTURE does not have an accepted type.

**TEXTURETYPE_ARGB8888 **—Compatible with the PC platform only.

**TEXTURETYPE_ABGR8888 **—Compatible with the PlayStation 2 platform only.

**TEXTURETYPE_ARGB8888_P8 **—Compatible with the PC platform only.

**TEXTURETYPE_ABGR8888_P8 **—Compatible with the PlayStation 2 platform only.

**TEXTURETYPE_ARGB4444 **—Compatible with the PC platform only.

**TEXTURETYPE_ARGB1555 **—Compatible with the PC platform only.

**TEXTURETYPE_RGB565 **—Compatible with the PC platform only.

**TEXTURETYPE_DXT1 **—Compatible with the PC platform only. DirectX texture compression format.

**TEXTURETYPE_DXT3 **—Compatible with the PC platform only. DirectX texture compression format.

**TEXTURETYPE_DXT5 **—Compatible with the PC platform only. DirectX texture compression format.

**WasTextureDrawnLastFrame **—Returns a Boolean value indicating whether or not the texture was rendered in the last frame. If it returns false, the texture is not in view and need not be updated.

For more information on these functions, see the API Reference.

[Top ](#top)

---

## Canvases

The **ILTClient::SetCanvasFn **and **ILTClient::GetCanvasFn **functions are used to set and get callbacks at object render time (for sorted alpha rendering).

### Rendering Objects By Hand

There are also situations where you might not be able to use the normal rendering methods, which require a world to be loaded. A good example might be if you wished to display one of the player models during a ‘character selection screen’ before allowing the person to start up the game. This is done by creating an array of **HLOCALOBJ **handles for the objects you want to render and placing them relative to a camera you create as a local client object. Then you render through them (again, in a **Start3D/End3D **block) like so:

>
```
GetClientLT()->ClearScreen(NULL,CLEARSCREEN_SCREEN);
```

```
GetClientLT()->Start3D();
```

```
GetClientLT()->RenderObjects(hCamera, pObjectList, 2);
```

```
GetClientLT()->End3D();
```

```
GetClientLT()->FlipScreen();
```

In this case, let us assume that the list of objects has two entries, one being a model (the player model) we are rendering for the character selection and the other being a light to give a nice dramatic lighting effect. Just as though we were rendering the world through a camera, model hooks can be called on manually rendered objects.

### Rendering Hooks

While LithTech itself handles the majority of rendering, the game code has an opportunity to alter certain rendering functions, such as model rendering. You can set a ‘model hook function’ that can be used to alter lighting on a model in a way specific to that model, for example. This would be useful for creating a “pulsing glow” effect on a power-up in a first-person shooter.

A Model Hook Function is given a chance to alter the rendering options of a particular model each frame. This function will be called with two parameters: a **ModelHookData **structure that contains information on what model is being rendered and allows changes to be made, and a void pointer defined by the user.

A good example might be to cause a “pulsing glow” as described above:

>
```
void DefaultModelHook (struct ModelHookData_t *pData, void      *pUser)
```

```
{
```

```
       LTClientShell* pShell = (LTClientShell*) pUser;
```

```
       if (!pShell) return;
```

```
       unit32 nUserFlags = 0;
```

```
       g_pClientLT->GetObjectUserFlags (pData->m_hObject,      &nUserFlags);
```

```
       if (nUserFlags & USRFLG_GLOW)
```

```
       {
```

```
              if (pData->m_LightAdd)
```

```
              {
```

```
                     *pData->m_LightAdd = pShell->GetModelGlow();
```

```
              }
```

```
       }
```

```
}
```

The function to set the model rendering hook is

>
```
LTRESULT SetModelHook(ModelHookFn fn, void* pUser);
```

As the **pUser **variable is a pointer to whatever is defined by the user at the time they set the model hook, this particular example assumes that you passed a pointer to the ClientShell, which has a **GetModelGlow **function which will return the amount of light to add to the model as a **LTVector **. This would cause the model itself to be rendered as if there were an additional lightsource that affected only it.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Render\Custom.md)2006, All Rights Reserved.
