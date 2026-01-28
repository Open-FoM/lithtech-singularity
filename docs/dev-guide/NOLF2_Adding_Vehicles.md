| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Changing and Adding Vehicles

### Overview

This document will detail the steps needed to change the current vehicles or add a new vehicle into the game No One Lives Forever 2 (NOLF2). This document will go into changing the source code so proficiency in C++ and Microsoft Visual C++ 6.0 will be needed for most of the changes and additions. In NOLF2 there were two types of vehicles, the snowmobile and the PlayerLure that was used for the bicycle chase. For this document we will only get into adding vehicles like the snowmobile. Within the document, any place ‘new vehicle’ or ‘NewVehicle’ is mentioned you should replace with the name of your new vehicle such as, Truck, ATV, or Tricycle. When asked to add code you will only need to add the code that is in bold type. The other code is there as reference so it is easier to see where to add the new code.

### PlayerVehicle

If you want to add a new vehicle to NOLF2 you will first need to add the ability to place the new vehicle in the levels. The object that level designers use to place vehicles in the levels in PlayerVehicle. The source for PlayerVehicle is in the files \Game\ObjectDLL\ObjectShared\PlaeryVehicle.cpp/.h

In order to get your new vehicle to show up in the drop down list of the VehicleType property on the PlayerVehicle object you will first need to add a new type to the PlayerPhysicsModel enum and to the list of PlayerPhysicsModel types that are considered vehicles in the function IsVehicleModel() found in Game\Shared\SharedMovement.h. Add these two lines:

enum PlayerPhysicsModel

{

PPM_FIRST=0,

PPM_NORMAL=0,

PPM_SNOWMOBILE,

PPM_LURE,

PPM_LIGHTCYCLE,

**PPM_NEWVEHICLE, **

PPM_NUM_MODELS

};

inline LTBOOL IsVehicleModel(PlayerPhysicsModel eModel)

{

switch (eModel)

{

case PPM_SNOWMOBILE :

case PPM_LURE :

case PPM_LIGHTCYCLE :

**case PPM_NEWVEHICLE: **

return LTTRUE;

break;

default : break;

}

return LTFALSE;

}

Next, you will need to add the name of your new vehicle to the array of vehicle names found in Game\Shared\SharedMovement.cpp. Add the one line:

char* aPPMStrings[] =

{

"Normal",

"Snowmobile",

"Lure",

"LightCycle" **, **

**“NewVehicle” **

};

Now you need to specify the model filename, skins and renderstyles to use for your vehicle. These are set within the PlayerVehicle::PostPropRead() found in \Game\ObjectDLL\ObjectShared\PlaeryVehicle.cpp. The current filenames for the snowmobile are defined in the player section of serverbutes.txt. You will want to create new entries for your new vehicle in this section. Open the file \Game\ObjectDLL\ObjectShared\PlayerButes.h and add the following #defines:

#define PLAYER_BUTE_SNOWMOBILETRANSLUCENT "SnowmobileTranslucent"

**#define PLAYER_BUTE_NEWVEHICLEMODEL "NewVehicleModel" **

**#define PLAYER_BUTE_NEWVEHICLESKIN "NewVehicleSkin" **

**#define PLAYER_BUTE_NEWVEHICLERS "NewVehicleRS" **

**#define PLAYER_BUTE_NEWVEHICLETRANSLUCENT "NewVehicleTranslucent" **

#define PLAYER_BUTE_WALKVOLUME "WalkFootstepVolume"

You will now need to add these entries to the player section in serverbutes.txt. You will probably want to create a new model for your vehicle but for now, we will just use the tricycle:

SnowmobileTranslucent = 1

**NewVehicleModel = "Props\Models\InTricycle.ltb" **

**NewVehicleSkin = "Props\Skins\InTricycle.dtx" **

**NewVehicleRS = "RS\default.ltb" **

**NewVehicleRS2 = "RS\transparent.ltb" **

**NewVehicleTranslucent = 1 **

WalkFootstepVolume = 0.0

Next, you need to setup the PlayerVehicle object to use your new filenames for your new vehicle. All of the file names are set in PlayerVehicle::PostReadProp(). Add the new PlayerPhysicsModel you created to the switch statement there and set the appropriate char pointers for the serverbute entries you made:

switch (m_ePPhysicsModel)

{

case PPM_SNOWMOBILE :

{

pModelAttribute = PLAYER_BUTE_SNOWMOBILEMODEL;

pSkin1Attribute = PLAYER_BUTE_SNOWMOBILESKIN;

pSkin2Attribute = PLAYER_BUTE_SNOWMOBILESKIN2;

pSkin3Attribute = PLAYER_BUTE_SNOWMOBILESKIN3;

pRenderStyle1Bute = PLAYER_BUTE_SNOWMOBILERS;

pRenderStyle2Bute = PLAYER_BUTE_SNOWMOBILERS2;

pRenderStyle3Bute = PLAYER_BUTE_SNOWMOBILERS3;

pModelTranslucent = PLAYER_BUTE_SNOWMOBILETRANSLUCENT;

}

break;

**case PPM_NEWVEHICLE : **

**{ **

**pModelAttribute = PLAYER_BUTE_NEWVEHICLEMODEL; **

**pSkin1Attribute = PLAYER_BUTE_NEWVEHICLESKIN; **

**pRenderStyle1Bute = PLAYER_BUTE_NEWVEHICLERS; **

**pRenderStyle2Bute = PLAYER_BUTE_NEWVEHICLERS2; **

**pModelTranslucent = PLAYER_BUTE_SNOWMOBILETRANSLUCENT; **

**} **

**break; **

default :

break;

}

You will now be able to place your new vehicle in your levels by adding a PlayerView object and selecting NewVehicle in the VehicleType dropdpwn list property. However if you try to activate your new vehicle you will notice that you are unable to actually ride it. This is because you have not yet added the physics for your new vehicle. Adding physics is discussed in the next section.

### Vehicle Physics

Once you have your vehicle in the level you will now need to create the player view model for the vehicle and setup the physics. Most of the necessary code you will need to add will be to the class CVehicleMgr located in the source files \Game\ClientShellDLL\ClientShellShared\VehicleMgr.cpp/h. CVehicleMgr is where you will need to add code for getting on your new vehicle, physics for your new vehicle, creating and updating the player view model, and dismounting from your vehicle and switching to normal player physics.

The first thing you will want to do is setup the player view model for your vehicle. The code for creating the model is in the function, CVehicleMgr::CreateVehicleModel(). You will probably want to create your own player view model that more closely resembles the vehicle you are adding. You will need to add the following code to CVehicleMgr::CreateVehicleModel():

**case PPM_NEWVEHICLE: **

**{ **

**createStruct.m_ObjectType = OT_MODEL; **

**createStruct.m_Flags = FLAG_VISIBLE | FLAG_REALLYCLOSE; **

**createStruct.m_Flags2 = FLAG2_FORCETRANSLUCENT | FLAG2_DYNAMICDIRLIGHT; **

****

**SAFE_STRCPY(createStruct.m_Filename, "Guns\\Models_PV\\ NewVehicle.ltb"); **

**SAFE_STRCPY(createStruct.m_SkinNames[1], "Guns\\Skins_PV\\ NewVehicle.dtx"); **

**SAFE_STRCPY(createStruct.m_RenderStyleNames[0], "RS\\default.ltb"); **

****

**CCharacterFX* pCharFX = g_pMoveMgr->GetCharacterFX(); **

****

**if (pCharFX ) **

**{ **

**SAFE_STRCPY(createStruct.m_SkinNames[0], g_pModelButeMgr->GetHandsSkinFilename(pCharFX->GetModelId())); **

**} **

**} **

**break; **

****

Once your player view model is created you will need to make sure that it will get updated properly. Updating of the vehicle model happens in the function CVehicleMgr::UpdateVehicleModel(). You will need to change the following line:

if (!m_hVehicleModel || (m_ePPhysicsModel != PPM_SNOWMOBILE) ) return;

To:

if (!m_hVehicleModel || **((m_ePPhysicsModel != PPM_SNOWMOBILE) && (m_ePPhysicsModel != PPM_NEWVEHICLE)) **) return;

That’s it for creating and updating your player view model. You will need to add a call to CreateVehicleModel() when you set your new vehicle physics model which is discussed a little later in this document.

The next thing you will need to do is create some console variables for your new vehicle that control a lot of the behavior. Console variables are used so you will be able to change their value while in the game, allowing you to tweak the variables until the vehicle feels the way you want it to behave.

In VehicleMgr.cpp there is already a long list of VarTrack variables, you will need to add the following lines to the list of VarTracks:

**VarTrack g_vtNewVehicleInfoTrack; **

**VarTrack g_vtNewVehicleVel; **

**VarTrack g_vtNewVehicleAccel; **

**VarTrack g_vtNewVehicleAccelTime; **

**VarTrack g_vtNewVehicleMaxDecel; **

**VarTrack g_vtNewVehicleDecel; **

**VarTrack g_vtNewVehicleDecelTime; **

**VarTrack g_vtNewVehicleMaxBrake; **

**VarTrack g_vtNewVehicleBrakeTime; **

**VarTrack g_vtNewVehicleOffsetX; **

**VarTrack g_vtNewVehicleOffsetY; **

**VarTrack g_vtNewVehicleOffsetZ; **

**VarTrack g_vtNewVehicleStoppedPercent; **

**VarTrack g_vtNewVehicleStoppedTurnPercent; **

**VarTrack g_vtNewVehicleMaxSpeedPercent; **

**VarTrack g_vtNewVehicleAccelGearChange; **

**VarTrack g_vtNewVehicleMinTurnAccel; **

You will need to initialize these variables so add the following lines to CVehicleMgr::Init():

**g_vtNewVehicleInfoTrack.Init( g_pLTClient, "NewVehicleInfo", NULL, 0.0f ); **

**g_vtNewVehicleVel.Init( g_pLTClient, "NewVehicleVel", NULL, 600.0f ); **

**g_vtNewVehicleAccel.Init( g_pLTClient, "NewVehicleAccel", NULL, 3000.0f ); **

**g_vtNewVehicleAccelTime.Init( g_pLTClient, "NewVehicleAccelTime", NULL, 3.0f ); **

**g_vtNewVehicleMaxDecel.Init( g_pLTClient, "NewVehicleDecelMax", LTNULL, 22.5f ); **

**g_vtNewVehicleDecel.Init( g_pLTClient, "NewVehicleDecel", LTNULL, 37.5f ); **

**g_vtNewVehicleDecelTime.Init( g_pLTClient, "NewVehicleDecelTime", LTNULL, 0.5f ); **

**g_vtNewVehicleMaxBrake.Init( g_pLTClient, "NewVehicleBrakeMax", LTNULL, 112.5f ); **

**g_vtNewVehicleBrakeTime.Init( g_pLTClient, "NewVehicleBrakeTime", LTNULL, 1.0f ); **

**g_vtNewVehicleMinTurnAccel.Init( g_pLTClient, "NewVehicleMinTurnAccel", LTNULL, 0.0f ); **

****

**g_vtNewVehicleOffsetX.Init( g_pLTClient, "NewVehicleOffsetX", LTNULL, 0.0f ); **

**g_vtNewVehicleOffsetY.Init( g_pLTClient, "NewVehicleOffsetY", LTNULL, -0.8f ); **

**g_vtNewVehicleOffsetZ.Init( g_pLTClient, "NewVehicleOffsetZ", LTNULL, 0.6f ); **

****

**g_vtNewVehicleStoppedPercent.Init( g_pLTClient, "NewVehicleStoppedPercent", LTNULL, 0.05f ); **

**g_vtNewVehicleMaxSpeedPercent.Init( g_pLTClient, "NewVehicleMaxSpeedPercent", LTNULL, 0.95f ); **

**g_vtNewVehicleAccelGearChange.Init( g_pLTClient, "NewVehicleAccelGearChange", LTNULL, 1.0f ); **

Once all of your console variables are set up you can add the code that will make sure your vehicle will use the proper values for things like velocity and acceleration. A lot of the functions that are looking for physics related values are set up as switch statements based on the PlayerPhysicsModel so adding your vehicle requires you to add your NewVehicle PhysicsModel, PPM_NEWVEHICLE, to these switch statements and that the proper values are set to the console variable values. To begin you should add your velocity console variable to the function CVehicleMgr::GetMaxVelMag(). Add these lines to that function:

case PPM_SNOWMOBILE :

{

fMaxVel = g_vtSnowmobileVel.GetFloat();

}

break;

**case PPM_NEWVEHICLE: **

**{ **

**fMaxVel = g_vtNewVehicleVel.GetFloat(); **

**} **

**break; **

****

Now do the same for CVehicleMgr::GetMaxAccelMag():

case PPM_SNOWMOBILE :

{

fMaxVel = g_vtSnowmobileAccel.GetFloat();

}

break;

**case PPM_NEWVEHICLE: **

**{ **

**fMaxVel = g_vtNewVehicleAccel.GetFloat(); **

**} **

**break; **

In the function CVehicleMgr::GetVehicleAccelTime() add:

case PPM_SNOWMOBILE :

fTime = g_vtSnowmobileAccelTime.GetFloat();

break;

**case PPM_NEWVEHICLE: **

**fTime = g_vtNewVehicleAccelTime.GetFloat(); **

**break; **

In the function CVehicleMgr::GetVehicleMaxDecel() add:

case PPM_SNOWMOBILE :

fValue = g_vtSnowmobileMaxDecel.GetFloat();

break;

**case PPM_NEWVEHICLE: **

**fValue = g_vtNewVehicleMaxDecel.GetFloat(); **

**break; **

In the function CVehicleMgr::GetVehicleDecelTime() add:

case PPM_SNOWMOBILE :

fValue = g_vtSnowmobileDecelTime.GetFloat();

break;

**case PPM_NEWVEHICLE: **

**fValue = g_vtNewVehicleDecelTime.GetFloat(); **

**break; **

In the function CVehicleMgr::GetVehicleMaxBrake() add:

case PPM_SNOWMOBILE :

fValue = g_vtSnowmobileMaxBrake.GetFloat();

break;

**case PPM_NEWVEHICLE: **

**fValue = g_vtNewVehicleMaxBrake.GetFloat(); **

**break; **

In the function CVehicleMgr::GetVehicleBrakeTime() add:

case PPM_SNOWMOBILE :

fValue = g_vtSnowmobileBrakeTime.GetFloat();

break;

**case PPM_NEWVEHICLE: **

**fValue = g_vtNewVehicleBrakeTime.GetFloat(); **

**break; **

In the function CVehicleMgr::GetVehicleMinTurnAccel() add:

case PPM_SNOWMOBILE :

fValue = g_vtSnowmobileMinTurnAccel.GetFloat();

break;

**case PPM_NEWVEHICLE: **

**fValue = g_vtNewVehicleMinTurnAccel.GetFloat(); **

**break; **

That’s it for adding code for getting the values specified in console variables. Now you should add code to the functions that are looking for specific sounds. You will need to add the following code.

In the function CVehicleMgr::GetIdleSnd() add:

case PPM_SNOWMOBILE :

return "Snd\\Vehicle\\Snowmobile\\idle.wav";

break;

**case PPM_NEWVEHICLE: **

**return "Snd\\Vehicle\\ NewVehicle \\idle.wav"; **

**break; **

In the function CVehicleMgr::GetAccelSnd() add:

case PPM_SNOWMOBILE :

return "Snd\\Vehicle\\Snowmobile\\accel.wav";

break;

**case PPM_NEWVEHICLE: **

**return "Snd\\Vehicle\\ NewVehicle \\accel.wav"; **

**break; **

In the function CVehicleMgr::GetDecelSnd() add:

case PPM_SNOWMOBILE :

return "Snd\\Vehicle\\Snowmobile\\decel.wav";

break;

**case PPM_NEWVEHICLE : **

**return "Snd\\Vehicle\\ NewVehicle \\decel.wav"; **

**break; **

In the function CVehicleMgr::GetBrakeSnd() add:

case PPM_SNOWMOBILE :

return "Snd\\Vehicle\\Snowmobile\\brake.wav";

break;

**case PPM_NEWVEHICLE : **

**return "Snd\\Vehicle\\NewVehicle\\brake.wav"; **

**break; **

In the function CVehicleMgr::GetImpactSnd() add:

case PPM_SNOWMOBILE :

{

if (fCurVelocityPercent > 0.1f && fCurVelocityPercent < 0.4f)

{

pSound = "Snd\\Vehicle\\Snowmobile\\slowimpact.wav";

}

else if (fCurVelocityPercent < 0.7f)

{

pSound = "Snd\\Vehicle\\Snowmobile\\medimpact.wav";

}

else

{

pSound = "Snd\\Vehicle\\Snowmobile\\fastimpact.wav";

}

}

break;

**case PPM_NEWVEHICLE : **

**{ **

**if (fCurVelocityPercent > 0.1f && fCurVelocityPercent < 0.4f) **

**{ **

**pSound = "Snd\\Vehicle\\NewVehicle\\slowimpact.wav"; **

**} **

**else if (fCurVelocityPercent < 0.7f) **

**{ **

**pSound = "Snd\\Vehicle\\NewVehicle\\medimpact.wav"; **

**} **

**else **

**{ **

**pSound = "Snd\\Vehicle\\NewVehicle\\fastimpact.wav"; **

**} **

**} **

**break; **

Now you should have all of the sound functions that are dependent on which vehicle the player is riding up to date. You will want to make sure that the sounds actually get used and the correct ones play at the correct time. To do this add the following line to CVehicleMgr::UpdateSound():

case PPM_SNOWMOBILE :

**case PPM_NEWVEHICLE : **

UpdateVehicleSounds();

break;

The physics for your new vehicle should be added next. You will first need to add code for switching to and from your PPM_NEWVEHICLE PlayerPhysicsModel, which is setting up what happens when the player gets on and off your vehicle. The function, CVehicleMgr::SetPhysicsModel() gets called when the player wants to switch their PlayerPhysicsModel, such as getting on or off a vehicle. CVehicleMgr::SetPhysicsModel() proceeds to call CVehicleMgr::PreSetPhysicsModel() first to do some preliminary setup for the vehicles. You should look at this function first and add the following code for when the player first gets on the vehicle:

// Check if we should disable the weapon.

switch (eModel)

{

case PPM_SNOWMOBILE :

EnableWeapon( false );

break;

**case PPM_NEWVEHICLE: **

**EnableWeapon( false ); **

**break; **

case PPM_LURE :

case PPM_NORMAL :

default :

break;

}

The above code will disable the player’s weapon when getting on your vehicle. Now you will also need to add the following block of code to the same function for when the player gets off your vehicle:

case PPM_SNOWMOBILE :

{

g_pClientSoundMgr->PlaySoundLocal("Snd\\Vehicle\\Snowmobile\\turnoff.wav", SOUNDPRIORITY_PLAYER_HIGH, PLAYSOUND_CLIENT);

PlayVehicleAni("Deselect");

// Can't move until deselect is done...

g_pMoveMgr->AllowMovement(LTFALSE);

// Wait to change modes...

bRet = LTFALSE;

}

break;

**case PPM_NEWVEHICLE : **

**{ **

**g_pClientSoundMgr->PlaySoundLocal("Snd\\Vehicle\\NewVehicle\\turnoff.wav", SOUNDPRIORITY_PLAYER_HIGH, PLAYSOUND_CLIENT); **

****

**PlayVehicleAni("Deselect"); **

****

**// Can't move until deselect is done... **

**g_pMoveMgr->AllowMovement(LTFALSE); **

****

**// Wait to change modes... **

**bRet = LTFALSE; **

**} **

**break; **

****

The above code will play a sound for turning off the engine of your vehicle and will play the deselect animation on the player view model showing the player they are getting off the vehicle. Getting back to CVehicleMgr::SetPhysicsModel() you will need to add code for actually setting the physics model for your new vehicle. Add the following code to CVehicleMgr::SetPhysicsModel():

case PPM_SNOWMOBILE :

{

SetSnowmobilePhysicsModel();

if (g_vtSnowmobileInfoTrack.GetFloat())

{

g_pLTClient->CPrint("Snowmobile Physics Mode: ON");

}

}

break;

**case PPM_NEWVEHICLE: **

**{ **

**SetNewVehiclePhysicsModel(); **

**} **

**break; **

****

Note that you are now making a call to a function that does not yet exist. You will need to write this function next. Open the header file for the class CVehicleMgr, \Game\ClientShellDLL\ClientShellShared\VehicleMgr.h and add the following line to the CVehicleMgr’s protected function definitions:

void SetSnowmobilePhysicsModel();

void SetPlayerLurePhysicsModel( );

void SetNormalPhysicsModel();

**void SetNewVehiclePhysicsModel(); **

Now you will need to go back to \Game\ClientShellDLL\ClientShellShared\VehicleMgr.cpp and write the actual function CVehicleMgr::SetNewVehicelPhysicsModel(). Add the following function:

**void CVehicleMgr::SetNewVehiclePhysicsModel() **

**{ **

**if (!m_hVehicleStartSnd) **

**{ **

**uint32 dwFlags = PLAYSOUND_GETHANDLE | PLAYSOUND_CLIENT; **

**m_hVehicleStartSnd = g_pClientSoundMgr->PlaySoundLocal("Snd\\Vehicle\\NewVehicle\\startup.wav", SOUNDPRIORITY_PLAYER_HIGH, dwFlags); **

**} **

****

**CreateVehicleModel(); **

****

**m_vVehicleOffset.x = g_vtNewVehicleOffsetX.GetFloat(); **

**m_vVehicleOffset.y = g_vtNewVehicleOffsetY.GetFloat(); **

**m_vVehicleOffset.z = g_vtNewVehicleOffsetZ.GetFloat(); **

****

**// Reset acceleration so we don't start off moving... **

****

**m_fVehicleBaseMoveAccel = 0.0f; **

****

**ShowVehicleModel(LTTRUE); **

****

**PlayVehicleAni("Select"); **

****

**// Can't move until select ani is done... **

****

**g_pMoveMgr->AllowMovement(LTFALSE); **

****

**m_bSetLastAngles = false; **

**m_vLastPlayerAngles.Init(); **

**m_vLastCameraAngles.Init(); **

**m_vLastVehiclePYR.Init(); **

**} **

The above function will play an initial sound for starting up your vehicle, it will then create the vehicle model that you setup before, play a select or getting on animation and finally resets some data members. You still won’t be able to ride your vehicle at this point but you are getting closer to getting everything setup. Next you will need to add a few more lines of code to functions that deal with the motion of the vehicle. Add the following line to CVehicleMgr::MoveLocalSolidObject( ):

// These guys don't handle movement.

case PPM_NORMAL:

case PPM_SNOWMOBILE:

**case PPM_NEWVEHICLE: **

return false;

break;

Add this line to CVehicleMgr::MoveVehicleObject():

// These guys don't handle movement.

case PPM_SNOWMOBILE:

**case PPM_NEWVEHICLE: **

You probably won’t need to do any camera displacement so add the following line to CVehicleMgr::CalculateVehicleCameraDisplacment():

case PPM_NORMAL:

case PPM_SNOWMOBILE:

**case PPM_NEWVEHICLE: **

case PPM_LIGHTCYCLE:

return;

break;

Now you will need to make sure your vehicle is able to accelerate, brake and turn when the player presses those commands. You will need to make sure the commands are properly read. Add the following line to CVehicleMgr::UpdateControlFlags():

**case PPM_NEWVEHICLE : **

case PPM_SNOWMOBILE :

UpdateSnowmobileControlFlags();

break;

Notice that your new vehicle is calling the function for updating the snowmobile control flags. Since the behavior you want is similar to the snowmobile and you will want to check the same control flags this is perfectly fine. If you decide you would want to have different behavior for your new vehicle then you would create a separate function to read in the control flags while riding your new vehicle.

Now you need to make sure the vehicle actually moves. Add the following line to CVehicleMgr::UpdateMotion():

case PPM_SNOWMOBILE :

**case PPM_NEWVEHICLE : **

UpdateVehicleMotion();

break;

Add the following line to CVehicleMgr::UpdateVehicleGear():

case PPM_SNOWMOBILE :

**case PPM_NEWVEHICLE : **

UpdateSnowmobileGear();

break;

And the following line to CVehicleMgr::UpdateFriction():

case PPM_SNOWMOBILE :

**case PPM_NEWVEHICLE : **

UpdateVehicleFriction(SNOWMOBILE_SLIDE_TO_STOP_TIME);

break;

That takes care of updating the movement for your new vehicle. Now you’ll want to make sure the rotation gets updated properly. Add the following code to CVehicleMgr::CalculateVehicleRotation():

case PPM_SNOWMOBILE:

CalculateBankingVehicleRotation( vPlayerPYR, vPYR, fYawDelta );

CalculateVehicleContourRotation( vPlayerPYR, vPYR );

break;

**case PPM_NEWVEHICLE: **

**CalculateBankingVehicleRotation( vPlayerPYR, vPYR, fYawDelta ); **

**CalculateVehicleContourRotation( vPlayerPYR, vPYR ); **

**break; **

That takes care of all the steps needed to get the physics working on your new vehicle. You may want to change some of the console variable values until you get the feel of the vehicle you want. You might also want to adjust how the vehicle banks, rotates and contours depending on what type of vehicle you are adding.

### Player

The final steps in adding a new vehicle involve how the player interacts with the vehicle. The CPlayerObj also needs to change some behavior depending on what PlayerPhysicsModel it is in. Open \Game\ObjectDLL\ObjectShared\PlaeryObj.cpp and change the following lines of code.

Just like CVehicleMgr needs to change the physics model so does CPlayerObj. Add the following line to CPlayerObj::SetPhysicsModel():

case PPM_SNOWMOBILE:

**case PPM_NEWVEHICLE **:

case PPM_LIGHTCYCLE:

SetVehiclePhysicsModel(eModel);

break;

This will let the player know they are actually riding a vehicle.

You will also need to make sure that other players, when in a multiplayer game, know that the player is riding a vehicle. Change the following line in CPlayerObj::SetPhysicsModel():

if (m_ePPhysicsModel == PPM_SNOWMOBILE)

To:

if **( **(m_ePPhysicsModel == PPM_SNOWMOBILE) || **(m_ePPhysicsModel == PPM_NEWVEHICLE) ) **

This will set the USRFLG_PLAYER_SNOWMOBILE user flag on any player that is riding your vehicle. This is fine because this flag is just used to see if the player is on a vehicle, not just the snowmobile. If you need to distinguish a difference between the snowmobile and your new vehicle, you will need to add a new user flag (none is currently available) or change how this is handled by going through the SpecialFX message.

CPlayerObj also needs to keep track of where the vehicle is and let the AI know its on a vehicle. Add the following line to CPlayerObj::UpdateVehicleMovement():

case PPM_SNOWMOBILE:

**case PPM_NEWVEHICLE: **

CPlayerObj controls what animation the player should be in while riding vehicles so you’ll have to make sure

### Conclusion

You have now just added a new vehicle to NOLF2. Now that you are familiar with the vehicle systems, you should be able to change some values and create the vehicle you want. You will want to create new sounds, models and skins for your vehicle so it looks the way you want. Adding more vehicles should now be very simple for you and you could even create a mod with several different vehicles to choose form. Have fun!

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: NOLF2_Adding_Vehicles.md)2006, All Rights Reserved.
