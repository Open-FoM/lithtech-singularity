| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Render Styles

LithTech Render Styles provide a consistent mechanism to customize how models are rendered. The customization encompasses parts of both lighting and rasterization from the basic render states and texture passes to dynamic UV generation and lighting material properties. Render Styles also supports platform-specific rendering methods (such as Direct3D vertex shaders). Render Styles are representing in code by the **CRenderStyle **class.

This section contains the following render styles topics:

- [About Render Styles ](#AboutRenderStyles)
- [Using ILTRenderStyles ](#UsingILTRenderStyles)
- [Using CreateRenderStyle ](#UsingCreateRenderStyle)
- [Setting Values in CRenderStyle ](#SettingValuesinCRenderStyle)
- [Applying a Render Style to a Model ](#ApplyingaRenderStyletoaModel)
- [Changing Render Styles Dynamically ](#ChangingtoRenderStylesDynamically)
- [Using Pre-Made Render Styles ](#UsingPreMadeRenderStyles)
- [Default Render Styles ](#DefaultRenderStyles)
- [Additional Render Styles ](#AdditionalRenderStyles)

---

## About Render Styles

The high-level process for applying Render Styles proceeds as follows:

1. Artists create a model in Maya or Max.
2. The model is exported to ModelEdit and customized therein.
3. Engineers create a Render Style to represent a given effect. This can be accomplished using the Render Styles Editor or by appropriate coding.
4. Engineers assign a Render Style in the game code.
5. The artist or engineer sets the Render Style index in ModelEdit just like for Textures.

LithTech includes several pre-generated Render Styles for use in your application. These cover most of the basic effects that are used in games. However, you can create your own Render Styles for customized effects using the Render Styles Editor or programmatically using the **ILTRenderStyles **interface and **CRenderStyle **class.

| **Note: ** | Creating a new Render Style requires a level of graphics and rendering knowledge. The Render Styles Editor automates some of the Render Styles creation processes. This tool provides real-time feedback and convenient features, but is still fairly complex in nature. It is not our expectation that all members of a game design team will be able to build their own render styles. |
| --- | --- |

When an object is created, you can set the Render Style using the **ObjectCreateStruct’s m_RenderStyleName **member variable.

#### To programmatically change a Render Style on an existing model in your game code

1. Acquire a pointer to LithTech’s **ILTRenderStyles **class instance using the Interface Manager.
2. Call the **ILTRenderStyles::CreateRenderStyle **function (or the **ILTModel::GetRenderStyle **function) to retrieve a pointer to a **CRenderStyle **instance.
3. Set the values in the **CRenderStyle **instance as desired.
4. Apply the **CRenderStyle **instance to the model using the **ILTModel::SetRenderStyle **function.

The RenderStyles sample located in the \samples\minimal\renderstyles\ directory of your LithTech installation provides examples of code implementing Render Styles.

[Top ](#top)

---

## Using ILTRenderStyles

The **ILTRenderStyles **interface provides methods to create, load, duplicate and free Render Styles ( **CRenderStyle **instances). LithTech exposes an **ILTRenderStyles **instance to the game code. You should not instantiate your own **ILTRenderStyles **instance. Use the **define_holder **macro of the Interface Manager to acquire a pointer to it:

>
```
static ILTRenderStyles *g_pLTRenderStyles;
```

```
define_holder(ILTRenderStyles, g_pLTRenderStyles);
```

After you have a pointer to the **ILTRenderStyles **instance, you can invoke several render style functions ( **CreateRenderStyle **, **DuplicateRenderStyle **, **FreeRenderStyle **, and **LoadRenderStyle **).

[Top ](#top)

---

## Using CreateRenderStyle

The **ILTRenderStyles::CreateRenderStyle **function returns a pointer to a **CRenderStyle **instance created by LithTech. This is the only way that a **CRenderStyle **instance should be instantiated.

>
```
CRenderStyle* CreateRenderStyle(bool bSetToDefault = true);
```

The function’s **bSetToDefault **parameter determines whether or not the returned **CRenderStyle **instance will have its member variables set to default values. The return value is a pointer to a newly instantiated **CRenderStyle **instance. You can access this instance’s member variables and functions to set values as desired.

[Top ](#top)

---

## Setting Values in CRenderStyle

The CRenderStyle class defines a Render Style. CRenderStyle instances should only be created using one of the ILTRenderStyles functions (CreateRenderStyle, DuplicateRenderStyle, or LoadRenderStyle). You should not manually instantiate a CRenderStyle instance in your game code.

After you have a pointer to a CRenderStyle instance you can use its many Set and Get functions to manipulate the following render options:

- Clip mode (no clipping, fast clipping, full clipping)
- Lighting material (ambient, diffuse, emissive, specular, specular power)

In addition to the options available above, you must also set one or more render passes to be applied to the model using the various **CRenderStyles **render pass functions:

- **AddRenderPass **
- **RemoveRenderPass **
- **SetRenderPass **
- **GetRenderPass **
- **GetRenderPassCount **

Each render pass is defined by a **RenderPassOp **structure. You can set the following render pass options using the **RenderPassOp **member variables:

Alpha test mode (none, less than, less than or equal to, greater than, greater than or equal to, equal, not equal)

- Fill mode (wire, fill)
- Cull mode (no culling, clockwise culling, counter-clockwise culling)
- Dynamic lighting (enable/disable)
- Texture stages
- Alpha blending mode
- Z-buffer mode
- Texture Factor
- Direct3D Bump environment mapping (enable/disable)

[Top ](#top)

---

## Applying a Render Style to a Model

After you have set the **CRenderStyle **variables you can apply the render style to a piece of a model using the **ILTModel::SetRenderStyle **function:

>
```
LTRESULT SetRenderStyle(
```

```
       HOBJECT pObj,
```

```
       HMODELPIECE hPiece,
```

```
       uint32 iLOD,
```

```
       CRenderStyle* pRenderStyle);
```

The **pObj **parameter identifies the engine object on which the model piece is located. The **hPiece **is a handle to the specific model piece on which the render style will be applied. The **iLOD **parameter identifies the level of detail. The **pRenderStyle **parameter points to the **CRenderStyle **to apply to the model piece.

[Top ](#top)

---

## Changing Render Styles Dynamically

Most Render Styles can be changed dynamically with little impact on performance. However, changing some of the parameters within the object is programmatically costly. A certain amount of pre-computation is done at pack time and at world load time. A render style is assigned to a model at model pack time. Some portions of the render style effect how it is packed and therefore cannot be changed at runtime.

[Top ](#top)

---

## Using Pre-Made Render Styles

LithTech provides several pre-made Render Styles that you can use to do basic effects.

Use the **ILTRenderStyles::LoadRenderStyle **function to create an object with one of these styles, as shown in the following example:

```
     m_CRenderStyle =
     g_pLTCRenderStyles->LoadRenderStyle("RenderStyles/rs_halo_vs.ltb");
```

[Top ](#top)

---

## Default Render Styles

These standard Render Styles are compiled and placed in the engine.rez file which is loaded by default by LithTech. They are all prefaced with the RenderStyles\ directory.

>

**Alphablend (with texalpha), rs_alphablend_texalpha.ltb **—Alpha blend with alpha factor from the textures alpha.

**Alphablend (with tfactoralpha), rs_alphablend_tfactoralpha_1tex.ltb **—Alpha blend using the TextureFactor alpha as alpha.

**Alphatest (greater), rs_alphatest_greater.ltb **—Alpha Test (also known as ChromaKey).

**Sphereenvmap (Add), rs_sphereenvmap_add.ltb **—Sphere environment map (add the environment map pass).

**Sphereenvmap (modtexalpha), rs_sphereenvmap_modtexalpha.ltb **—Sphere environment map (using the texture alpha to modulate on the env map).

**Two Texture, rs_twotexture.ltb **—Use two textures (both with same UV set).

**Vertex Specular, rs_vertexspecular.ltb **—Use vertex specular.

**dot3bump bump mapping, rs_dot3bump_dir_vs.ltb. **

**Halo, rs_halo_vs.ltbI **—Draw a white halo around the model on a second pass.

**Cartoon render, rs_toon_vs.ltb **—The first texture is the shade texture.

**Fullbright, rs_fullbright.ltb **—No dynamic lighting. Textures decal onto the models directly.

**Fullbright (with texture masking), rs_fullbright_basetexmask.ltb **—The model's base texture's alpha specifies where the model is fullbright and where it uses the regular dynamic lighting.

[Top ](#top)

---

## Additional Render Styles

The following Render Styles are not compiled in the engine.rez file by default. To use these Render Styles you must explicitly add them to your game resources.

>

**Anisotropic lighting, rs_aniso_3light_vs.lta **—Simulates anisotropic lighting. Uses a second texture with an alpha mask as a lighting highlight with its UVs generated from x = l dot n, y = h dot n. Where l is the light vector, n is the normal, and h is the half angle vector.

**Anisotropic lighting (with texture masking), rs_aniso_basetexmask_3light_vs.lta **—Similar to rs_aniso_3light_vs.lta, except it uses the base texture's alpha mask to attenuate the anisotropic highlighting.

**Blurry, rs_blurry_vs.lta **—Simulate a blur (from field of view or other). Renders the model three times. The second and third renderings are offset in screen space by a small amount to give the effect of a blurry object.

**Eye dot Normal Highlighting, rs_eyedotnormal_3light_vs.lta **—Uses a second texture pass to highlight the model with a texture map with its UVs generated from the eye vector dot the normal vector. The highlight texture can be used to emphasize the center or edges of the model (screen space center/edges).

**Eye dot Normal Highlighting (with texture masking), rs_eyedotnormal_basetexmask_3light_vs.lta **—Like rs_eyedotnormal_3light_vs.lta, but uses the base texture's alpha mask to attenuate the hightlight texture.

**Eye dot Normal Hightlighting (with highlight texture alpha attenuation), rs_eyedotnormal_highlight_3light_vs.lta **—Like rs_eyedotnormal_3light_vs.lta, but uses the alpha from the highlight texture in the effect.

**Motion blur, rs_motionblur_vs.lta **—Single pass motion blur. Transforms each vertex with the objects current and previous transform, takes the difference vector and dots it with the normal. Uses the result to choose the actual position of the vertex and its alpha. Note that this only works on rigid objects.

**Motion fade, rs_motionfade_vs.lta **—Renders two more copies of the objects at previous locations fading the object as it goes. Note that this only works on rigid objects.

**Power shield 1, rs_powershield1_vs.lta **—A power shield example. Expands the model by its normals, and uses the eye dot normal to set the transparency.

**Power shield 2, rs_powershield2_vs.lta **—A power shield example. Expands the model by its normals, uses the the eye dot normal for transparency, and generates a psuedo-random-rotation UV for a second texture with an alpha mask which is used for highlighting the shield.

**Power shield 3, rs_powershield2_vs.lta **—A power shield example. Very much like rs_powershield2_vs.lta, except that it inverts the e dot n for the UVs, expands the model even more. Creates a fogy looking shield with the highlight texture color and alpha coming through in the effect.

**Cartoon render with base texture, rs_toon_basetex_2pass_2texstage_vs.lta **—Very much like the original cartoon renderstyle, expect that it adds a base texture.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Render\Styles.md)2006, All Rights Reserved.
