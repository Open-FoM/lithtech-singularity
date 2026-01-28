Projected Shadows:



Below is a list of console commands for adjusting and fine-tuning projected model shadows.



| **Variable ** | **Type ** | **Default ** | **Description ** |
| --- | --- | --- | --- |
| DrawAllModelShadows | Bool | Off | Setting this flag causes all non fastlight lights to be factored into the cast shadow, otherwise only non fastlights with the cast shadow flag will be used |
| ModelShadow_Proj_MaxShadows | Int | 1 | The maximum number of shadows to be cast by a model. |
| ModelShadow_Proj_TextureRes | Int | 256 | The size of the texture that will be used for the projected shadow. This texture is square and can have the dimensions range from 16 to 512 in powers of two. |
| ModelShadow_Proj_LOD | Int | 1 | The LOD offset to use when rendering a model to a texture. If it is 0, it will use the same LOD that the model is being rendered at, if it is one, it will use the next lowest LOD level and so on. |
| ModelShadow_Proj_Tween | Bool | On | When this value is enabled, it will find the most prominent lights like normal, but it will reserve the last shadow for lighting by a fake light created by blending together all less prominent lights appropriately. This should be used to prevent popping of shadows when a model moves from one light to another. |
| ModelShadow_Proj_Perspective | Bool | Off | When enabled this will draw the shadow using perspective, so that it gets larger as it goes away from the model. Otherwise it will do an orthographic projection of the shadow onto the world. |
| ModelShadow_Proj_DrawLights | Bool | Off | When enabled, this will draw a line from the model being lit to the lights that are being used for the generation of the shadow. This is useful for debugging light placements in levels. |
| ModelShadow_Proj_DrawShadowTex | Bool | Off | When enabled this will draw a small version of the shadow texture in the upper right hand corner of the screen. |
| ModelShadow_Proj_DrawProjPlane | Bool | Off | When enabled this will draw the frustum from the light to the model that is being used to render the model. This is a good debugging tool for determining where the tweened lights are being placed as well as debugging light placement. |
| ModelShadow_Proj_NumAntiAliasPasses | Int | 0 | For each antialias pass it will do an additional render of the model into the shadow texture at a slight displacement, allowing the edges to be blurred slightly and reducing jagged edges. |
| ModelShadow_Proj_AntiAliasSpacing | Real | 2.0 | For each antialias pass, this is the number of pixels it should expand out to provide antialiasing. So if this was two, the first antialias pass would extend out about two pixels on each side, then the second pass would extend four, and so on. |
| ModelShadow_Proj_AntiAliasFalloff | Real | 0.2 | This is the amount of alpha that is subtracted from the model being rendered on any given antialias pass. So on the first pass if it was 0.2, it would only have eighty percent of its original alpha making it lighter. |
| ModelShadow_Proj_MinColorComponent | Real | 40.0 | This is critical for efficient performance of model shadows. After the color of the shadow is calculated, the largest color component is looked at, and if it is less than this value it will not be rendered on the world. Setting this to 0 will cause all shadows to be drawn all the time, and setting it to 255 will cause no shadows to be rendered. |
| ModelShadow_Proj_MaxProjDist | Real | 200.0 | This value indicates how far past a models midpoint a shadow can extend. It will fall off near the end of this region to prevent an abrupt stop. Extending this will allow the shadows to project further, but will render slower and cause shadows to extend through walls further possibly causing rendering glitches. |
| ModelShadow_Proj_Alpha | Real | 1.0 | This value scales the alpha component of shadows to allow them to be much softer or much darker. If it is at 0.5, shadows will only be half as dark, if it as 2.0 shadows will be twice as dark. This is useful for simulating more diffuse lighting or harsh high contrast lighting. |
| ModelShadow_Proj_ProjAreaRadiusScale | Real | 1.1 | This allows for some padding around the model so that when rendering the shadow parts of the model won’t get cut off. Increasing this value causes the shadow to look more pixilated, but shrinking it allows parts to get cut off in certain cases. |
