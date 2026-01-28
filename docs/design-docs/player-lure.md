**Player Lure **



This document describes how to use the PlayerLure object to keyframe the player’s movement. The movement is controlled by keyframing a server side object called a PlayerLure. You trigger the player to follow that playerlure using “followlure”, and then move the playerlure object around. The player will keep following the lure until you trigger the player to cancel following the lure using “cancellure”. The PlayerLure object has 3 options on how to restrict the camera rotation during the lure: none, limited and unlimited. The PlayerLure can also disable the weapon.



**PlayerLure Game Object **

This object can be moved on the server. The player follows the object after receiving a “followlure” trigger command.

| **Property ** | **Value ** | **Comment ** |
| --- | --- | --- |
| PlayerLureCameraFreedom | None Limited Unlimited | Specifies the amount of restriction to place on the camera rotation. |
| LimitedYawLeftRight | “ *LeftDegree **RightDegree *” | Specifies how much you can rotate left and how much you can rotate right. If LeftDegree < -180 and RightDegree > 180, you will have unlimited rotation in Yaw. |
| LimitedPitchDownUp | “ *DownDegree **UpDegree *” | Specifies how much you can rotate down and how much you can rotate up. Unlimited rotation in pitch is not possible. |
| AllowWeapon | True False | Allow/disallow the weapon during lure. Default: False. |
| RetainOffsets | True False | Player retains position and rotation offset from PlayerLure. Default: False. |
| Bicycle | True False | Camera simulates motion of riding a bicycle. Default: False |



**Player Trigger Commands **

The trigger commands to control the playerlure are specified here.

| **Command ** | **Parameters ** | **Comment ** |
| --- | --- | --- |
| Followlure *playerlurename *** | *playerlurename *- name of the lure to follow | Tells the player to start following the playerlure named *playerlurename *. |
| Cancellure | None | Tells the player to stop following the playerlure and go back to normal movement. |

**Bicycle Console Variables **

There are a few console variables that let you control the camera motion in the Bicycle = true case.

| **Console Variable ** | **Value ** | **Comment ** |
| --- | --- | --- |
| BicycleInterBumpTimeRange | “ *mintime **maxtime *” in seconds | Time between bumps in the road. Random number between mintime and maxtime. Default: "0.75 1.25" |
| BicycleIntraBumpTimeRange | “ *mintime maxtime *” in seconds | Time taken to go over bump in the road. Random number between mintime and maxtime. Default: "0.05 0.15" |
| BicycleBumpHeightRange | “ *minheight maxheight *” in game units | Height of bump. Random number between minheight and maxheight. Default: "0 0" |
| BicycleBobRate | 0 to ?? Hz | Number of bobs per second. Default: 2.5 |
| BicycleBobSwayAmplitude | 0 to ?? game units | How much to sway left and right during each bob. Default: 2.0 |
| BicycleBobRollAmplitude | -180 to 180 degrees | How much to roll the camera during each bob in degrees. Positive numbers will roll counter clockwise while sway goes right. Default: -0.25 |
| BicycleModelOffset | “ *x y z *” in game units | Model position offset from center of lure. Default: "0.0 -25.0 15.0" |

**Bicycle Resources **

The bicycle model uses the following resources:

chars\Models\armstrong.ltb

chars\skins\armstrong.dtx

chars\skins\armstronghead.dtx

RS\default.ltb



The bicycle sound uses the following resources:

Snd\vehicle\Bicycle\accel.wav

*The Operative 2 Design Specification. ©2001 Monolith Productions, Inc. Proprietary & Confidential. Page PAGE 1. *

.
