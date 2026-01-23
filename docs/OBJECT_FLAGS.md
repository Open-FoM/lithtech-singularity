# Object Flags (FLAG_ / FLAG2_)

This document summarizes the object flag sets defined in `engine/sdk/inc/ltbasedefs.h`. These flags apply to **all engine objects** (including lights) via `m_Flags` and `m_Flags2` on the base object type. `FLAG2_` is an extension set; only the **lower 16 bits** of `FLAG2_` are replicated to the client.

## FLAG_ (m_Flags)
Defined in the `FLAG_` enum section in `ltbasedefs.h`.

- `FLAG_VISIBLE` — See `ltbasedefs.h` for details.
- `FLAG_SHADOW` — Does this model cast shadows?
- `FLAG_SPRITEBIAS` — Biases the Z towards the view so a sprite doesn't clip as much.
- `FLAG_CASTSHADOWS` — Should this light cast shadows (slower)?
- `FLAG_ROTATABLESPRITE` — Sprites only.
- `FLAG_UPDATEUNSEEN` — See `ltbasedefs.h` for details.
- `FLAG_WASDRAWN` — See `ltbasedefs.h` for details.
- `FLAG_SPRITE_NOZ` — Disable Z read/write on sprite (good for lens flares).
- `FLAG_GLOWSPRITE` — Shrinks the sprite as the viewer gets nearer.
- `FLAG_ONLYLIGHTWORLD` — Lights only - tells it to only light the world.
- `FLAG_ENVIRONMENTMAPONLY` — For PolyGrids - says to only use the environment map (ignore main texture).
- `FLAG_DONTLIGHTBACKFACING` — Lights only - don't light backfacing polies.
- `FLAG_REALLYCLOSE` — See `ltbasedefs.h` for details.
- `FLAG_FOGDISABLE` — Disables fog on WorldModels, Sprites, Particle Systems and Canvases only.
- `FLAG_ONLYLIGHTOBJECTS` — Lights only - tells it to only light objects (and not the world).
- `FLAG_FULLPOSITIONRES` — See `ltbasedefs.h` for details.
- `FLAG_NOLIGHT` — See `ltbasedefs.h` for details.
- `FLAG_PAUSED` — Determines whether or not this object is paused and should not animate or update in a visible manner
- `FLAG_YROTATION` — See `ltbasedefs.h` for details.
- `FLAG_RAYHIT` — Object is hit by raycasts.
- `FLAG_SOLID` — Object can't go thru other solid objects.
- `FLAG_BOXPHYSICS` — Use simple box physics on this object (used for WorldModels and containers).
- `FLAG_CLIENTNONSOLID` — This object is solid on the server and nonsolid on the client.

## Server-only FLAG_ (m_Flags)
Defined as `#define` entries under the "Server only flags" section in `ltbasedefs.h`. These are not part of the main `FLAG_` enum but still occupy bits in `m_Flags`.

- `FLAG_TOUCH_NOTIFY` — Server-only object behavior flag.
- `FLAG_GRAVITY` — Server-only object behavior flag.
- `FLAG_STAIRSTEP` — Server-only object behavior flag.
- `FLAG_GOTHRUWORLD` — Server-only object behavior flag.
- `FLAG_DONTFOLLOWSTANDING` — Server-only object behavior flag.
- `FLAG_NOSLIDING` — Server-only object behavior flag.
- `FLAG_POINTCOLLIDE` — Server-only object behavior flag.
- `FLAG_MODELKEYS` — Server-only object behavior flag.
- `FLAG_TOUCHABLE` — Server-only object behavior flag.
- `FLAG_FORCECLIENTUPDATE` — Server-only object behavior flag.
- `FLAG_REMOVEIFOUTSIDE` — Server-only object behavior flag.
- `FLAG_FORCEOPTIMIZEOBJECT` — Server-only object behavior flag.
- `FLAG_CONTAINER` — Server-only object behavior flag.

## FLAG2_ (m_Flags2)
Defined in the `FLAG2_` enum section in `ltbasedefs.h`.

- `FLAG2_FORCEDYNAMICLIGHTWORLD` — If this is a dynamic light, it will always light the world regardless of what the console variable settings say
- `FLAG2_ADDITIVE` — For sprites - do additive blending.
- `FLAG2_MULTIPLY` — For sprites. Multiplied color blending.
- `FLAG2_PLAYERCOLLIDE` — Use a y-axis aligned cylinder to collide with the BSP.
- `FLAG2_DYNAMICDIRLIGHT` — See `ltbasedefs.h` for details.
- `FLAG2_SKYOBJECT` — See `ltbasedefs.h` for details.
- `FLAG2_FORCETRANSLUCENT` — See `ltbasedefs.h` for details.
- `FLAG2_DISABLEPREDICTION` — See `ltbasedefs.h` for details.
- `FLAG2_SERVERDIMS` — See `ltbasedefs.h` for details.
- `FLAG2_SPECIALNONSOLID` — See `ltbasedefs.h` for details.
- `FLAG2_USEMODELOBBS` — See `ltbasedefs.h` for details.

## Related Masks / Internal Flags

- `CLIENT_FLAGMASK` — Flags the client knows about.
- `FLAGMASK_ALL` — All flags (useful for `SetObjectFlags`).
- `FLAG_OPTIMIZEMASK` — Flags relevant to optimization (clearing these can skip object iteration).
- `FLAG_INTERNAL1`, `FLAG_INTERNAL2` — Internal engine flags (not for general use).

## Object-type Overview

### Lights
- `FLAG_CASTSHADOWS`
- `FLAG_ONLYLIGHTWORLD`
- `FLAG_DONTLIGHTBACKFACING`
- `FLAG_ONLYLIGHTOBJECTS`
- `FLAG2_FORCEDYNAMICLIGHTWORLD`

### Sprites
- `FLAG_SPRITEBIAS`
- `FLAG_ROTATABLESPRITE`
- `FLAG_SPRITE_NOZ`
- `FLAG_GLOWSPRITE`
- `FLAG2_ADDITIVE`
- `FLAG2_MULTIPLY`
- `FLAG2_FORCETRANSLUCENT`

### Models
- `FLAG_REALLYCLOSE`
- `FLAG_FULLPOSITIONRES`
- `FLAG_YROTATION`
- `FLAG2_DYNAMICDIRLIGHT`
- `FLAG2_USEMODELOBBS`

### Particle Systems
- `FLAG_UPDATEUNSEEN`
- `FLAG_WASDRAWN`

### PolyGrids
- `FLAG_ENVIRONMENTMAPONLY`
- `FLAG_WASDRAWN`

### Physics/Collision
- `FLAG_SOLID`
- `FLAG_BOXPHYSICS`
- `FLAG_RAYHIT`
- `FLAG_POINTCOLLIDE`
- `FLAG_TOUCH_NOTIFY`
- `FLAG_TOUCHABLE`
- `FLAG2_PLAYERCOLLIDE`
- `FLAG2_SPECIALNONSOLID`

### Networking/Prediction
- `FLAG_FULLPOSITIONRES`
- `FLAG_YROTATION`
- `FLAG_FORCECLIENTUPDATE`
- `FLAG2_DISABLEPREDICTION`
- `FLAG2_SERVERDIMS`
- `FLAG_CLIENTNONSOLID`

## Usage References

Below are up to three references per flag in engine or tool code (excluding the defining header). Paths include line numbers from `rg -n`.

### `FLAG_VISIBLE`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/fallingstufffx.cpp:354:			ocs.m_Flags				= FLAG_VISIBLE | FLAG_NOLIGHT | FLAG_ROTATABLESPRITE;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/bouncychunkfx.cpp:189:	ocs.m_Flags				= FLAG_NOLIGHT | FLAG_VISIBLE;`
- `/Users/paxful/projects/lithtech-singularity/tools/DEdit2/editor_model_mgr.cpp:279:	inst->m_Flags = FLAG_VISIBLE;`

### `FLAG_SHADOW`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/ltengineobjects.cpp:123:					pStruct->m_Flags |= FLAG_SHADOW;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/ltbmodelfx.cpp:278:	ocs.m_Flags				|= pBaseData->m_dwObjectFlags |	(GetProps()->m_bShadow ? FLAG_SHADOW : 0 );`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_shadow_draw.cpp:311:		if ((hook_data.m_ObjectFlags & FLAG_SHADOW) == 0)`

### `FLAG_SPRITEBIAS`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/spritefx.cpp:134:				m_dwObjectFlags |= FLAG_SPRITEBIAS;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/flarespritefx.cpp:270:			m_pLTClient->Common()->SetObjectFlags( m_hObject, OFT_Flags, FLAG_SPRITEBIAS | FLAG_VISIBLE, FLAG_SPRITE_NOZ | FLAG_SPRITEBIAS | FLAG_VISIBLE);`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_object_draw.cpp:570:	if ((instance->m_Flags & FLAG_SPRITEBIAS) && !really_close)`

### `FLAG_CASTSHADOWS`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/world/src/world_shared_bsp.cpp:669:						light.m_Flags				= (light_castshadow ? FLAG_CASTSHADOWS : 0);`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_shadow_draw.cpp:356:		if (!(light->m_Flags & FLAG_CASTSHADOWS) && !g_CV_DrawAllModelShadows.m_Val)`

### `FLAG_ROTATABLESPRITE`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/ltengineobjects.cpp:131:					pStruct->m_Flags |= FLAG_ROTATABLESPRITE;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/fallingstufffx.cpp:354:			ocs.m_Flags				= FLAG_VISIBLE | FLAG_NOLIGHT | FLAG_ROTATABLESPRITE;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/spritefx.cpp:232:	ocs.m_Flags				|= GetProps()->m_dwObjectFlags | pBaseData->m_dwObjectFlags | (GetProps()->m_nFacing || GetProps()->m_bRotate ? FLAG_ROTATABLESPRITE : 0);`

### `FLAG_UPDATEUNSEEN`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/particlesystemfx.cpp:314:	ocs.m_Flags			|= FLAG_VISIBLE | FLAG_UPDATEUNSEEN | FLAG_FOGDISABLE | pData->m_dwObjectFlags;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/client/src/clientmgr.cpp:2335:            if (!(flags & FLAG_UPDATEUNSEEN) && !(flags & FLAG_INTERNAL1) && (pSystem->m_nChangedParticles == 0))`

### `FLAG_WASDRAWN`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/particlesystemfx.cpp:524:	if( m_bRendered && !(dwFlags & FLAG_WASDRAWN ) )`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/client/src/clientmgr.cpp:2332:            // Set its FLAG_WASDRAWN appropriately and avoid updating if possible.`

### `FLAG_SPRITE_NOZ`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/spritefx.cpp:118:			m_dwObjectFlags |= ( bNoZ ? FLAG_SPRITE_NOZ : 0 );`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/flarespritefx.cpp:270:			m_pLTClient->Common()->SetObjectFlags( m_hObject, OFT_Flags, FLAG_SPRITEBIAS | FLAG_VISIBLE, FLAG_SPRITE_NOZ | FLAG_SPRITEBIAS | FLAG_VISIBLE);`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_object_draw.cpp:1881:	if (object->m_Flags & FLAG_SPRITE_NOZ)`

### `FLAG_GLOWSPRITE`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_object_draw.cpp:535:		if (instance->m_Flags & FLAG_GLOWSPRITE)`

### `FLAG_ONLYLIGHTWORLD`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_object_draw.cpp:1291:					if (!dynamic || (dynamic->m_Flags & FLAG_ONLYLIGHTWORLD))`

### `FLAG_ENVIRONMENTMAPONLY`
- _No direct references found outside `ltbasedefs.h`._

### `FLAG_DONTLIGHTBACKFACING`
- _No direct references found outside `ltbasedefs.h`._

### `FLAG_REALLYCLOSE`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/polytubefx.cpp:182:	m_bReallyClose = !!(pBaseData->m_dwObjectFlags & FLAG_REALLYCLOSE);`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/dynalightfx.cpp:125:	ocs.m_Flags	&= ~FLAG_REALLYCLOSE;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/shared/fxflags.h:16:	#define	FXFLAG_REALLYCLOSE				(1<<2)`

### `FLAG_FOGDISABLE`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/particlesystemfx.cpp:314:	ocs.m_Flags			|= FLAG_VISIBLE | FLAG_UPDATEUNSEEN | FLAG_FOGDISABLE | pData->m_dwObjectFlags;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_object_draw.cpp:644:	const bool fog_enabled = (g_CV_FogEnable.m_Val != 0) && ((instance->m_Flags & FLAG_FOGDISABLE) == 0);`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_world_draw.cpp:1902:		const bool fog_enabled = fog_enabled_base && ((instance->m_Flags & FLAG_FOGDISABLE) == 0);`

### `FLAG_ONLYLIGHTOBJECTS`
- _No direct references found outside `ltbasedefs.h`._

### `FLAG_FULLPOSITIONRES`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/client/src/shellnet.cpp:499:        if (pObject->m_Flags & FLAG_FULLPOSITIONRES) `
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/s_net.cpp:709:        if (pObj->m_Flags & FLAG_FULLPOSITIONRES)`

### `FLAG_NOLIGHT`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/fallingstufffx.cpp:354:			ocs.m_Flags				= FLAG_VISIBLE | FLAG_NOLIGHT | FLAG_ROTATABLESPRITE;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/bouncychunkfx.cpp:189:	ocs.m_Flags				= FLAG_NOLIGHT | FLAG_VISIBLE;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/playsoundfx.cpp:151:	ocs.m_Flags				= FLAG_NOLIGHT;`

### `FLAG_PAUSED`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/basefx.cpp:553:		m_pLTClient->Common()->SetObjectFlags(m_hObject, OFT_Flags, (bPause) ? FLAG_PAUSED : 0, FLAG_PAUSED);`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/world/src/de_objects.h:205:		return !!(m_Flags & FLAG_PAUSED);`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/sound/src/soundbuffer.cpp:551:    if (soundInstance.GetSoundInstanceFlags() & SOUNDINSTANCEFLAG_PAUSED)`

### `FLAG_YROTATION`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/s_client.cpp:833:		if (pObject->m_Flags & FLAG_YROTATION) `

### `FLAG_RAYHIT`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/ltengineobjects.cpp:170:					pStruct->m_Flags |= FLAG_RAYHIT;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/servermgr.cpp:1903:		ocs.m_Flags = FLAG_SOLID | FLAG_RAYHIT;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/s_client.cpp:945:	static const uint32 k_nInteractionFlags = FLAG_VISIBLE | FLAG_SOLID | FLAG_RAYHIT;`

### `FLAG_SOLID`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/ltengineobjects.cpp:148:					pStruct->m_Flags |= FLAG_SOLID;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/world/src/intersectsweptsphere.cpp:398:	if((pObj->m_ObjectType == OT_WORLDMODEL) && (pObj->m_Flags & FLAG_SOLID))`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/world/src/fullintersectline.cpp:621:    if (pObject->m_Flags & (FLAG_RAYHIT|FLAG_SOLID) || g_bProcessNonSolid) `

### `FLAG_BOXPHYSICS`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:22:	w_TransformWorldModel(pWorldModel, &pWorldModel->m_Transform, !!(pWorldModel->m_Flags & FLAG_BOXPHYSICS));`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/server_iltcommon.cpp:134:            if (HasWorldModel(hObj) && (nChangingFlags & FLAG_BOXPHYSICS) && !(nFlags & FLAG_BOXPHYSICS))`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveplayer.cpp:1192:	if (pObject->HasWorldModel() && ((pObject->m_Flags & FLAG_BOXPHYSICS) == 0))`

### `FLAG_CLIENTNONSOLID`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:87:	return !!((dwFlags & FLAG_SOLID) && !(dwFlags & FLAG_CLIENTNONSOLID));`

### `FLAG_TOUCH_NOTIFY`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/iclientshell.h:149:Called when an object (with FLAG_TOUCH_NOTIFY) moved on the client`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/ltengineobjects.cpp:162:					pStruct->m_Flags |= FLAG_TOUCH_NOTIFY;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/collision.cpp:2513:		if(request.m_pObject->m_Flags & FLAG_TOUCH_NOTIFY)`

### `FLAG_GRAVITY`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/ltengineobjects.cpp:155:					pStruct->m_Flags |= FLAG_GRAVITY;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/client/src/predict.cpp:241:			(pObj->m_Flags & FLAG_GRAVITY) != 0,`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/client/src/client_iltphysics.cpp:118:                (pMoveInfo->m_hObject->m_Flags & FLAG_GRAVITY) != 0,`

### `FLAG_STAIRSTEP`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/collision.cpp:2310:	if( request.m_pObject->m_Flags & FLAG_STAIRSTEP )`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveplayer.cpp:1777:			if ((m_nPlayerFlags & FLAG_STAIRSTEP) && ((vOrigin.x != vEnd.x) || (vOrigin.z != vEnd.z)))`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/collision.h:125:	// If FLAG_STAIRSTEP is specified, this will set m_pStandingOn to`

### `FLAG_GOTHRUWORLD`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:167:	if(pObj->m_Flags & FLAG_GOTHRUWORLD)`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveplayer.cpp:42:		m_nPlayerFlags |= FLAG_GOTHRUWORLD;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/world/src/de_world.h:29:                                        // 2. FLAG_GOTHRUWORLD is checked for world models`

### `FLAG_DONTFOLLOWSTANDING`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:1502:			if( pObjOn->m_Flags & FLAG_DONTFOLLOWSTANDING )`

### `FLAG_NOSLIDING`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:516:								!(pState->m_pObj->m_Flags & FLAG_NOSLIDING) ) )`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveplayer.cpp:40:		m_nPlayerFlags |= FLAG_NOSLIDING;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.h:21:#define MO_NOSLIDING		(1<<5) // Don't slide (override for FLAG_NOSLIDING)`

### `FLAG_POINTCOLLIDE`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/collision.cpp:2277:	if( request.m_pObject->m_Flags & FLAG_POINTCOLLIDE )`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:413:			if ((pObj1->m_Flags & FLAG_POINTCOLLIDE) != 0)`

### `FLAG_MODELKEYS`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/iltbaseclass.h:555:(You only get it if your \b FLAG_MODELKEYS flag is set`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/s_object.cpp:180:        if (pObj->m_Flags & FLAG_MODELKEYS)`

### `FLAG_TOUCHABLE`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:1138:		if(!IsPhysical(pObject->m_Flags, pArray->m_pState->m_bServer) && !( pObject->m_Flags & FLAG_TOUCHABLE ))`

### `FLAG_FORCECLIENTUPDATE`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/s_client.cpp:947:	if (!(pObject->m_Flags & FLAG_FORCECLIENTUPDATE)) `

### `FLAG_REMOVEIFOUTSIDE`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/s_object.cpp:142:        if (((pObj->m_Flags & FLAG_REMOVEIFOUTSIDE) != 0) &&`

### `FLAG_FORCEOPTIMIZEOBJECT`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/ltengineobjects.cpp:919:			g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, FLAG_FORCEOPTIMIZEOBJECT, FLAG_FORCEOPTIMIZEOBJECT);`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/s_object.cpp:728:    return (pObj->m_Flags & FLAG_FORCEOPTIMIZEOBJECT) || `

### `FLAG_CONTAINER`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/ltengineobjects.cpp:239:	ADD_LONGINTPROP(Flags, (FLAG_VISIBLE | FLAG_CONTAINER))`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:467:	if(pObj1->m_Flags & FLAG_CONTAINER)`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.h:121:	return (IsSolid(flags, bServer) || flags & (FLAG_TOUCH_NOTIFY|FLAG_CONTAINER));`

### `FLAG2_FORCEDYNAMICLIGHTWORLD`
- `/Users/paxful/projects/lithtech-singularity/tools/DEdit2/app_main.cpp:568:		light.m_Flags2 |= FLAG2_FORCEDYNAMICLIGHTWORLD;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/dynalightfx.cpp:120:		ocs.m_Flags2 |= FLAG2_FORCEDYNAMICLIGHTWORLD;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_scene_collect.cpp:210:	if (!g_CV_DynamicLightWorld.m_Val && ((light->m_Flags2 & FLAG2_FORCEDYNAMICLIGHTWORLD) == 0))`

### `FLAG2_ADDITIVE`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/particlesystemfx.cpp:197:				m_dwBlendMode = FLAG2_ADDITIVE;`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_world_draw.cpp:1906:				: ((instance->m_Flags2 & FLAG2_ADDITIVE) != 0 ? kWorldBlendAdditive : kWorldBlendAlpha);`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_object_draw.cpp:132:	if (object->m_Flags2 & FLAG2_ADDITIVE)`

### `FLAG2_MULTIPLY`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_object_draw.cpp:137:	if (object->m_Flags2 & FLAG2_MULTIPLY)`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/spritefx.cpp:111:				m_dwObjectFlags2 |= FLAG2_MULTIPLY;`
- `/Users/paxful/projects/lithtech-singularity/engine/clientfx/particlesystemfx.cpp:201:				m_dwBlendMode = FLAG2_MULTIPLY;`

### `FLAG2_PLAYERCOLLIDE`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/collision.cpp:1836:	if (request.m_pObject->m_Flags2 & FLAG2_PLAYERCOLLIDE)`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:1056:	if(bPushAway && ((pState->m_pObj->m_Flags2 & FLAG2_PLAYERCOLLIDE) != 0) )`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveplayer.h:21:				(pObj->m_Flags2 & FLAG2_PLAYERCOLLIDE) &&`

### `FLAG2_DYNAMICDIRLIGHT`
- _No direct references found outside `ltbasedefs.h`._

### `FLAG2_SKYOBJECT`
- `/Users/paxful/projects/lithtech-singularity/engine/sdk/inc/ltengineobjects.cpp:920:			g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags2, FLAG2_SKYOBJECT, FLAG2_SKYOBJECT);`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_object_draw.cpp:1853:	if (object->m_Flags2 & FLAG2_SKYOBJECT)`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_scene_collect.cpp:154:	if (object->m_Flags2 & FLAG2_SKYOBJECT)`

### `FLAG2_FORCETRANSLUCENT`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/world/src/de_objects.h:199:				(m_Flags2 & (FLAG2_ADDITIVE | FLAG2_FORCETRANSLUCENT));	// or they have a flag set forcing translucency`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/render_a/src/sys/diligent/diligent_object_draw.cpp:2032:	if (object->m_Flags2 & FLAG2_FORCETRANSLUCENT)`

### `FLAG2_DISABLEPREDICTION`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/client/src/predict.cpp:119:	if(g_bPrediction && ((pObject->m_Flags2 & FLAG2_DISABLEPREDICTION) == 0))`

### `FLAG2_SERVERDIMS`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/server/src/s_net.cpp:561:	if ((changeFlags & CF_DIMS) && ((pObj->m_Flags2 & FLAG2_SERVERDIMS) == 0))`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/client/src/cutil.cpp:715:    if ((pInstance->cd.m_ClientFlags & CF_DONTSETDIMS) || (pInstance->m_Flags2 & FLAG2_SERVERDIMS))`

### `FLAG2_SPECIALNONSOLID`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveobject.cpp:1042:	// Honor FLAG2_SPECIALNONSOLID`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/shared/src/moveplayer.cpp:79:	if (((pObj->m_Flags2 & FLAG2_SPECIALNONSOLID) != 0) &&`

### `FLAG2_USEMODELOBBS`
- `/Users/paxful/projects/lithtech-singularity/engine/runtime/world/src/fullintersectline.cpp:535:                                    if(pServerObj->m_Flags2 & FLAG2_USEMODELOBBS)`
