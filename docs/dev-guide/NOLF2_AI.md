| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

AI Placement in Dedit

All AI placed through Dedit are CAIHuman. (Yes, including robots, rats, and rabbits). In addition to placing CAIHumans, other AI objects are placed in Dedit that allow AI to understand the world. AI navigate the world by planning paths through connected AIVolumes. AI determine what objects or places of interest are nearby by searching for AINodes. AIVolumes and AINodes are invisible to the player.

Properties on the CAIHuman object determine the AI’s look and behavior:

**CAIHuman Parameters: **

| **ModelTemplate ** | The model template from modelbutes.txt. The model template deteremines the ltb model file, skeleton, animation data file, skin textures, and renderstyles. The model template references an AI template in aibutes.txt. The AI template determines the AI’s alignment, and sensory parameters. |
| --- | --- |
| **Brain ** | The brain template from aibutes.txt. The brain determines a number of parameters that affect the AI’s behavior. |
| **GoalSet ** | The goalset defined in aigoals.txt. The goalset lists the goals that will control the AI’s behavior. |

---

## AIButes.txt

AIButes.txt contains various sections defining parameters that affect AI behavior. The major sections are AIBrains, and AI Attribute Templates. Sections not described are insignificant, self-explanatory, or beyond the scope of this document (AIStimulus – tweak at your own risk!).

### AIBrain

AIBrains specify a number of parameters that define an AI’s behavior. Many AI may share the same brain. There are a number of required parameters, and some optional parameters. Some parameters found in AIbutes.txt were not used for NOLF2.

**Required Parameters: **

| **DodgeVectorCheckTime ** | How frequently we check to see if we need to dodge a vector |
| --- | --- |
| **DodgeVectorCheckChance ** | The % chance we will check to see if we need to dodge a vector once this time is up |
| **DodgeVectorDelayTime ** | After dodging a vector, the time we must wait before we can check again |
| **DodgeVectorShuffleDist ** | How much space is needed to shuffle. |
| **DodgeVectorRollDist ** | How much space is needed to roll. |
| **DodgeVectorShuffleChance ** | The % chance the dodge will be a shuffle. |
| **DodgeVectorRollChance ** | The % chance the dodge will be a roll. |
| **DodgeVectorBlockChance ** | The % chance the dodge will be a block. |
| **DodgeProjectileCheckTime ** | Not used for NOLF2. |
| **DodgeProjectileCheckChance ** | Not used for NOLF2. |
| **DodgeProjectileDelayTime ** | Not used for NOLF2. |
| **AttackRangedRange ** | The min/max distance we must be at to attack with a ranged weapon (e.g. rifle) |
| **AttackThrownRange ** | The min/max distance we must be at to attack with a thrown weapon (e.g. grenade) |
| **AttackMeleeRange ** | The min/max distance we must be at to attack with a melee weapon (e.g. sword) |
| **AttackChaseTime ** | After we lose sight of our target, how long do we wait before chasing |
| **AttackChaseTimeRandom ** | An additional random amount of time to the above time |
| **AttackPoseCrouchChance ** | When we enter the attack state, chance we will crouch and fire |
| **AttackPoseCrouchTime ** | How long we stay crouched |
| **AttackGrenadeThrowTime ** | How often we'll consider throwing a grenade |
| **AttackGrenadeThrowChance ** | Chance we'll attempt to throw the grenade after this time expires |
| **AttackCoverCheckTime ** | How often we check for the presence of cover |
| **AttackCoverCheckChance ** | Chance we'll check once this time expires |
| **AttackSoundTimeMin ** | Min amount of time since any AI played combat dialog before this AI can play combat dialog |
| **AttackWhileMoving ** | Can the AI attack while moving? |
| **AttackFromCoverCoverTime ** | How long we will stay covered for before uncovering |
| **AttackFromCoverCoverTimeRandom ** | An additional random amount of time to the above time |
| **AttackFromCoverUncoverTime ** | How long we will stay uncovered for before covering |
| **AttackFromCoverUncoverTimeRandom ** | An additional random amount of time to the above time |
| **AttackFromCoverReactionTime ** | If uncovered and shot at, the delay before we will go back to cover |
| **AttackFromCoverMaxRetries ** | How many times we will attempt to go for new cover when cover becomes invalidated. |
| **AttackFromVantageAttackTime ** | Time we will fire from a vantage node before moving on to the next one |
| **AttackFromVantageAttackTimeRandom ** | An additional random amount of time to the above time |
| **AttackFromViewChaseTime ** | How long we will we wait to chase at an attack from view node |
| **ChaseExtraTime ** | Not used for NOLF2. |
| **ChaseExtraTimeRandom ** | Not used for NOLF2. |
| **ChaseSearchTime ** | Not used for NOLF2. |
| **PatrolSoundTime ** | Time between chances of playing a patrolling sound |
| **PatrolSoundTimeRandom ** | An additional random amount of time to the above time |
| **PatrolSoundChance ** | The chance we'll play a patrol sound after the patrolsoundtime |
| **DistressFacingThreshhold ** | Dot product (-1=180' to 1=0') at which target must be facing towards us to be considered threatening |
| **DistressIncreaseRate ** | Rate at which distress increases when being threatened |
| **DistressDecreaseRate ** | Rate at which distress decreases when not being threatened |
| **DistressLevels ** | How many levels of distress to go through before panic |
| **MajorAlarmThreshold ** | How high alarm level must be to start searching (threat assumed) |
| **ImmediateAlarmThreshold ** | How high alarm level must be to know threat is immediate |
| **TauntDelay ** | How long between checks to taunt |
| **TauntChance ** | Chance the AI should taunt when it is able to |
| **CrawlThreshhold ** | Not used for NOLF2. |
| **CrawlChance ** | Not used for NOLF2. |
| **CrawlLifetime ** | Not used for NOLF2. |
| **CanLipSync ** | Can this AI lip sync? |
| **DamageMask ** | Mask of accepted types of damage. Other types are ignored. "None" means affected by all damage types. |

**Optional Parameters: **

Parameters that are not required for every AI are specified in the AIBrain with name = value pairs, in the form AIDataN = “SomeLabel=SomeValue”. For example:

AIData0 = "AccidentChance=0.1"

AIData1 = "DivergePaths=1.0"

AIData2 = "OneAnimCover=1.0"

|  | **AccidentChance ** | Percent chance of AI having an accident. Used for AIs falling on their faces when jumping over railings. |  |  |
| --- | --- | --- | --- | --- |
|  | **BreaksDoors ** | AI destroys doors when going through them. Used for SuperSoldiers. |  |  |
|  | **CannotPathThruDoors ** | AI may not go through doors. Used to contain ambient life. |  |  |
|  | **ChaseEndurance ** | Number of seconds running before AI needs to catch his breath. Used for police. |  |  |
|  | **DeflectChance ** | Percent chance of deflecting a bullet. Used for ninjas with katanas. |  |  |
|  | **DisappearDistMin ** | Minimum distance away a threat must be to trigger AI to disappear. Used for ninjas. |  |  |
|  | **DisappearDistMax ** | Minimum distance away a threat can be to trigger AI to disappear. Used for ninjas. |  |  |
|  | **DisappearFadeTime ** | Amount of time to fade translucency when disappearing. Used for ninjas. |  |  |
|  | **DisposeLantern ** | AI must detach object from light socket when leaving an investigative state. Used for ninjas with search lanterns. |  |  |
|  | **DivergePaths ** | AIs try to split up taking separate paths to the same destination. |  |  |
|  | **DoNotCloseDoors ** | AI does not close doors after going through them. |  |  |
|  | **DoNotDamageSelf ** | AI does not damage himself with his own weapons. Used to keep Volkov from injuring himself with rocket splash damage. |  |  |
|  | **DoNotExposeHiding ** | AI does not expose a hidden player, in a hiding spot. Used for ambient life. |  |  |
|  | **DoNotToggleLights ** | AI does not attempt to turn on/off lights. |  |  |
|  | **DynMoveTimeMin ** | Minimum time AI stands still while attacking with a ranged weapon. |  |  |
|  | **DynMoveTimeMax ** | Maximum time AI stands still while attacking with a ranged weapon. |  |  |
|  | **DynMoveBackupDist ** | Distance an AI backs up while attacking with a ranged weapon. |  |  |
|  | **DynMoveFlankPassDist ** | Distance past a target AI will travel when flanking. |  |  |
|  | **DynMoveFlankWidthDist ** | Distance to the side of a target AI will travel when flanking. |  |  |
|  | **IgnoreJunctions ** | AI cannot get lost in junctions. |  |  |
|  | **LethalSpecialDamage ** | SpecialDamage will instantly kill AI. Used for ambient life, when hit with sleeping damage, electrical damage, etc. |  |  |
|  | **LongJumpHeightMin ** | Minimum height of parabola when jumping long distances. Used for ninja lunge. |  |  |
|  | **LongJumpHeightMax ** | Maximum height of parabola when jumping long distances. Used for ninja lunge. |  |  |
|  | **LungeDistMin ** | Minimum distance away a target can be to trigger a lunge. Used for ninja. |  |  |
|  | **LungeDistMax ** | Maximum distance away a target can be to trigger a lunge. Used for ninja. |  |  |
|  | **LungeSpeed ** | Speed AI travels while lunging. Used for ninja. |  |  |
|  | **LungeStopDistance ** | Distance in front of a target to end a lunge. Used for ninja. |  |  |
|  | **GoalProximityDist ** | Distance to trigger the ProximityCommand goal. |  |  |
|  | **MeleeKnockBackDist ** | Distance to push back a target with a melee attack. |  |  |
| **MinPathWeight ** | Minimum weight of an AIVolume that AI will use when pathfinding. Used to restrict some AI to preferred volumes. |  |  |  |
|  | **MusicMoodMin ** | Minimum music mood AI will set. Used to keep ambient life from affecting music. |  |  |
|  | **MusicMoodMax ** | Maximum music mood AI will set. Used to keep ambient life from affecting music. |  |  |
|  | **NoDynamicPathfinding ** | AI will not try to avoid dynamic obstacles. |  |  |
| **OneAnimCover ** | AI uses one movement-encoded animation to attack from cover, rather than a series of animations. |  |  |  |
|  | **PrimaryWeaponOnLeft ** | Primary weapon is on AI’s left side, instead of the default right side. |  |  |
|  | **ProneDistMin ** | Minimum distance from target AI must be to go prone. |  |  |
|  | **ProneSlideDist ** | Distance AI slides forward when diving to the ground to go prone. |  |  |
|  | **ProneTime ** | Amount of time AI stays prone. |  |  |
|  | **ReappearDist ** | Distance away AI reappears after disappearing. Used for ninja. |  |  |
|  | **RetreatTriggerDist ** | Distance from target to AI required to trigger AI to retreat. Used for ninja. |  |  |
|  | **RetreatJumpDist ** | Distance AI jumps back when retreating. Used for ninja. |  |  |
|  | **RetreatSpeed ** | Speed AI travels while jumping back when retreating. Used for ninja. |  |  |
|  | **SleepTimeMin ** | Minimum time AI sleeps. Used for powered-down SuperSoldiers. |  |  |
|  | **SleepTimeMax ** | Maximum time AI sleeps. Used for powered-down SuperSoldiers. |  |  |
|  |  |  |  |  |

### AI Attribute Templates

AI attribute templates define parameters for AI behavior associated with a model. Multiple models may share the same template.

**Required Parameters: **

| **Name ** | Name of template, referenced in modelbutes.txt as AIName. |
| --- | --- |
| **RunSpeed ** | Speed AI moves when running with an animation that uses constant velocity (rather than movement encoding). |
| **JumpSpeed ** | Speed AI moves while playing a jumpUp animation. |
| **JumpOverSpeed ** | Speed AI moves while playing a jumpOver animation. |
| **FallSpeed ** | Speed AI moves while falling. |
| **CrawlSpeed ** | Not used in NOLF2. |
| **WalkSpeed ** | Speed AI moves when walking with an animation that uses constant velocity (rather than movement encoding). |
| **SwimSpeed ** | Not used in NOLF2. |
| **FlySpeed ** | Not used in NOLF2. |
| **Marksmanship ** | Not used in NOLF2. |
| **Bravado ** | Not used in NOLF2. |
| **SoundRadius ** | Sound radius used when AI plays dialogue sounds. |
| **HitPoints ** | Amount of hit points AI starts with. Modified by difficulty. |
| **Armor ** | Always 0 for NOLF2. |
| **Accuracy ** | Percent chance a shot will hit AI’s target. |
| **AccuracyIncreaseRate ** | Rate AI regains accuracy after a change due to attacking from cover. |
| **AccuracyDecreaseRate ** | Not used in NOLF2. |
| **FullAccuracyRadius ** | Radius from AI where AI has perfect accuracy. |
| **AccuracyMissPerturb ** | Amount of perturb on bullets that miss their target. |
| **MaxMovementAccuracyPerturb ** | Maximum amount of perturb caused by aiming at a moving target. |
| **MovementAccuracyPerturbTime ** | Amount of time to catch up when aiming at a moving target. |
| **CanUseVolumeXXX ** | AI may pathfind through AIVolumes of type XXX (e.g. CanUseVolumeJumpUp). By default, AI may pathfind through all AIVolume types. Types of AIVolumes are described in the AIVolume section of this document. |
| **CanXXX ** | TRUE if sense is turned on for AI, where XXX is a sense. (e.g. CanSeeEnemy). |
| **XXXDistance ** | Distance that AI can sense XXX. (e.g. SeeEnemyDistance). |

**Senses referenced by XXX above include: **

| **SeeEnemy ** | AI sees an enemy. |
| --- | --- |
| **SeeEnemyLean ** | AI sees a leaning enemy. |
| **SeeEnemyWeaponImpact ** | AI sees someone get shot by an enemy. |
| **SeeEnemyDanger ** | AI sees something dangerous. Used for AIGoalLoveKitty. |
| **SeeEnemyDisturbance ** | AI sees something disturbed by an enemy. |
| **SeeEnemyLightDisturbance ** | AI sees a light disturbed by an enemy (turned on or off). |
| **SeeEnemyFootprint ** | AI sees an enemy’s footprint. |
| **SeeAllyDeath ** | AI sees an ally’s dead, or knocked out body. |
| **SeeAllyDisturbance ** | AI sees an ally get disturbed. |
| **HearEnemyWeaponFire ** | AI hears an enemy fire a weapon. |
| **HearEnemyWeaponImpact ** | AI hears an enemy’s weapon impact. |
| **HearEnemyFootstep ** | AI hears an enemy’s footsteps. |
| **HearEnemyAlarm ** | AI hears an alarm sound, due to an enemy’s presence. |
| **HearEnemyDisturbance ** | AI hears an enemy make a disturbance. |
| **HearAllyDeath ** | AI hears an ally die. |
| **HearAllyPain ** | AI hears an ally shout in pain. |
| **HearAllyWeaponFire ** | AI hears an ally fire a weapon. |
| **HearAllyDisturbance ** | AI hears an ally get disturbed. |
| **SeeAllyDistress ** | AI sees an ally in distress, going for backup. |
| **SeeAllySpecialDamage ** | AI sees an ally affected by special damage (glued, in bear trap, on fire, etc) |

---

## AIVolumes

AIVolumes denote open space to an AI, and must be placed everywhere an AI may travel. AIVolumes are invisible to the player.

The restrictions and requirements for placing AIVolumes in general are:

· AIVolumes do not contain any physical obstacles (e.g. desks, walls, columns, etc).

· AIVolumes align exactly with adjacent AIVolumes.

· AIVolumes containing doors must be narrow volumes just containing the dimensions of the closed door. Only one door may be in an AIVolume.

· AIVolumes have minimum dimensions in XZ of 48x48.

· AIVolumes are generally 16 units tall, and 8 units off the ground. This is not a requirement.

· AIVolumes touching other AIVolumes must have a connection of at least 48 units.

There are several varieties of AIVolumes, used for different purposes. Below are descriptions of the purposes and parameters of the different types of AIVolumes. AIVolumes not listed below were not used on NOLF2.

### AIVolume

AIVolume is the most basic volume type. All other AIVolume objects are derived from AIVolume. All AIVolumes have the parameters below at a minimum.

| **Lit ** | FALSE if volume is in the dark. Tells AI whether they need to turn on a light. Can be toggled through the commands “lit” and “unlit”. Example: msg aivolume01 unlit |
| --- | --- |
| **Region ** | AIRegion this AIVolume is part of. Regions are used for alarms and for searching. |
| **LightSwitchNode ** | LightSwitch AINodeUseObject used to toggle the light state of this volume (lit/unlit). |
| **PreferredPath ** | TRUE if this volume should be preferred over other for pathfinding. In NOLF2 this was used a couple of different ways. Only SuperSoliders and Robots can travel through AIVolume with PreferredPath set to true so they don’t walk through spaces that are too small for them to fit. The AIBrain AIData MinPathWeight=1 takes care of this. We also used this to make the AI path finding look better in some situations. For instance in Japan, when a ninja was patrolling a section of street sometimes she would walk through a flower bed to get to her next patrol point. Setting the PreferredPath to True on the AIVolumes that were on the streets helped make the AI paths look better by making them walk on the streets when in a relaxed state.. |

### AIVolumeAmbientLife

There is nothing special about AIVolumeAmbientLife. The volume types simply exists so that ambient life AI can be use a set of volumes that other AI do not use (e.g. so that a rat can run under a table, but a soldier cannot). AI are restricted from pathfinding through AIVolumeAmbientLife by adding this line to their AI Attribute Template in AIButes.txt:

CanUseVolumeAmbientLife = FALSE

### AIVolumeJumpOver

AI use AIVolumeJumpOvers to pathfind and jump over obstacles (e.g. fences). This volume should connect to AIVolumes horizontally. AI jump from one edge of the volume to the opposite edge. The height of the parabola of the jump is determined by the height of the top of the volume.

### AIVolumeJumpUp

AI use AIVolumeJumpUps to pathfind and jump up and down from things (e.g. rooftops). This volume should connect to AIVolumes above and below the jump destination. AIVolumeJumpUp has several additional parameters.

| **OnlyJumpDown ** | This volume may only be used to jump down from something (and not jump up onto something). |
| --- | --- |
| **OnlyJumpWhenAlert ** | AI may only jump through this volume when alerted to a threat. |
| **HasRailing ** | AI must play an animation to jump over a railing when jumping through this volume. |

### AIVolumeJunction

AI get lost when chasing a target, and running through an AIVolumeJunction while the target is out of sight. A junction connects up to four AIVolumes. AIVolumeJunctions have extra parameters determining the percent chance of different actions that can be taken once the junction is reached. The extra parameters are duplicated for each of the connecting volumes in four possible directions, North, South, East, and West.

**Junction Actions: **

| **Nothing ** | Run through junction and get lost. |
| --- | --- |
| **Continue ** | Run through junction and get lost. |
| **Peek ** | Peek into a connecting volume, and then run a different direction. |
| **Search ** | Run into a volume and search its AIRegion. |

### AIVolumeLadder

AI use AIVolumeLadders to pathfind and climb on ladders. This volume should be centered on the ladder, and connect to AIVolumes above and below the ladder. AI play a special death animation when killed inside an AIVolumeLadder.

### AIVolumeLedge

AI play a special death animation when killed inside an AIVolumeLedge. These volumes are typically used on balconies, rooftops, and cliffs. The angle of the Yaw determines which direction the AI will fall off the ledge.

### AIVolumeStairs

AI use AIVolumeStairs to pathfind and walk up stairs. This volume should enclose the staircase, and connect to AIVolumes above and below the stairs. AI play a special death animation when killed inside an AIVolumeStairs . The angle of the Yaw determines which direction the AI will roll down the stairs when killed.

### AIVolumePlayerInfo


AIVolumePlayerInfo is totally unrelated to the other AIVolumes. AIVolumePlayerInfo is not used for AI pathfinding. It is used to determine if the player is hidden, or attackable from an associated AINodeView. AIVolumePlayerInfo has a number of unique parameters.

| **On ** | Volume is usable for hiding, and sense mask is enabled when On is TRUE. This volume can be messaged “On” and “Off” with commands. |
| --- | --- |
| **SenseMask ** | SenseMask applied to the volume, that blocks out specified senses while AI are relaxed. Used in the HARM Villa to make enemies ignore Cate when she was in unrestricted areas. |
| **Hiding ** | Player can use this volume to hide. |
| **HidingCrouch ** | Player can use this volume to hide, if she is crouched. |
| **ViewNodes ** | List of AINodeViews that are associated with this volume. If an AI cannot find a path to the player, and wants to attack, he may go to an AINodeView and attack from there. |

---

## AINodes

AINodes are objects that provide information to AI about opportunities at particular locations. AI must be able to find a path to an AINode, so all AINodes must be inside of AIVolumes. AINodes are invisible to the player.

There are several varieties of AINodes, used for different purposes. Below are descriptions of the purposes and parameters of the different types of AINodes. AINodes not listed below were not used on NOLF2.

### AINode

AINode is the most basic node type. All other AINode objects are derived from AINode. All AINodes have the parameters below at a minimum.

| **Face ** | AI turns to face the direction of this node when arriving at this node. |
| --- | --- |
| **Alignment ** | Only AI with the specified alignment may use this node. By default, Alignment is set to “None,” which means all AI may use the node. An AI’s alignment is specified in the AI Attribute Template in AIButes.txt. |
| **StartDisabled ** | This node starts disabled. AI ignores disabled nodes . Nodes may be enabled / disabled through the commands “enable” and “disable”. |

### AINodeBackup

AI runs to an AINodeBackup when he sees a threat and needs backup. The goal GetBackup sends an AI to an AINodeBackup. Other AI that are within the AINodeBackup’s stimulus radius will respond to the AI when he arrives at the node, and follow him to the threat if they have the goal RespondToBackup.

| **AttractRadius ** | AI must be within this radius for the node to be valid to go to for backup. |
| --- | --- |
| **ResetTime ** | Amount of time before any AI may use this node again. |
| **StimulusRadius ** | AI within the stimulus radius may respond to the call for backup. |
| **Movement ** | Movement animation AI uses to get to the node. |
| **CryWolfCount ** | Number of times AI may call for backup at this node before allies think he is crying wolf. Count resets when allies see a threat |
| **Command ** | Command to run on arrival to the node. |

### AINodeCover

AI run to an AINodeCover when a threat is present, and the AI activates goal Cover or AttackFromCover.

| **Duck ** | AI ducks at node and pops up to attack. |
| --- | --- |
| **BlindFire ** | AI ducks at node, and fires blindly over an object. |
| **1WayRoll ** | Not used in NOLF2. |
| **2WayRoll ** | Not used in NOLF2. |
| **1WayStep ** | AI stands at node, and steps out from behind something to attack. Node rotation determines the direction of the step. |
| **2WayStep ** | AI stands at node, and steps out from behind something to attack. AI may step out from 2 directions. Node rotation determines the directions of the step. |
| **IgnoreDir ** | Ignore the direction of a threat when determining if node is valid for cover. |
| **Fov ** | Field of view that threat must be in to validate the node for cover. |
| **Radius ** | AI must be within this radius for the node to be valid to go to for cover. |
| **ThreatRadius ** | If threat enters the ThreatRadius, node is invalidated for cover. |
| **Object ** | Object or AINodeUSeObject to activate on arrival at the node. (e.g. to flip a table to take cover behind). |
| **Timeout ** | Amount of time AI can stay at the node. |
| **HitpointsBoost ** | Amount to boost AI’s hitpoints while at the node. |

****

### AINodeDeath

AI pathfind to an AINodeDeath before dying with a special animation from the DramaDeath goal.

### AINodeExitLevel

AI walk to an AINodeExitLevel if they have the ExitLevel goal, and no other goals are active. This node is mainly used for reinforcements leaving the level if there are no open patrol routes of guard posts.

### AINodeGoto

AI pathfind to an AINodeGoto when given a Goto goal with an AINodeGoto as the destination.

| **Command ** | Command AI runs when arriving at the AINodeGoto. |
| --- | --- |

****

### AINodeGuard

AI stays within the ReturnRadius of an AINodeGuard when he has the Guard goal, and is unaware of a threat.

| **ReturnRadius ** | AI stays within this radius when unaware of a threat. AI returns to this radius when giving up on a chase or search. |
| --- | --- |
| **GuardedNodes ** | List of AINodeUseObject nodes AI may use while assigned to this AINodeGuard. Only the AI assigned to the AINodeGuard may use these nodes. |

### AINodePanic

AI run to AINodePanic nodes when they are in distress. AI plays a panic animation at the node.

| **Next ** | Next AINodePatrol on the patrol route. |
| --- | --- |
| **Action ** | Animation to play at the node. |
| **Command ** | Command to run on arrival to the node. |

### AINodePatrol

AI with the Patrol goal patrol along a route set by chaining AINodePatrol nodes.

| **AttractRadius ** | AI must be within this radius for the node to be valid to go to for backup. |
| --- | --- |
| **ResetTime ** | Amount of time before any AI may use this node again. |
| **StimulusRadius ** | AI within the stimulus radius may respond to the call for backup. |
| **Movement ** | Movement animation AI uses to get to the node. |
| **CryWolfCount ** | Number of times AI may call for backup at this node before allies think he is crying wolf. Count resets when allies see a threat |
| **Command ** | Command to run on arrival to the node. |

### AINodeSearch

AI pathfind between AINodeSearch nodes when searching for a known threat. At each node, AI plays a searching animation.

| **ResetTime ** | Amount of time before any AI may visit this node again. |
| --- | --- |
| **ShineFlashlight ** | Play a flashlight animation. |
| **LookUnder ** | Play a look under animation. |
| **LookOver ** | Play a look over animation. |
| **LookUp ** | Play a look up animation. |
| **LookLeft ** | Play a look left animation. |
| **LookRight ** | Play a look right animation. |
| **KnockOnDoor ** | Play a knock animation. |
| **Alert ** | Play an alert animation. |
| **SearchType ** | Invalidation test if any: Default, OneWay, or Corner. OneWay nodes are only valid if AI approaches facing the same direction the node points. Corner nodes are only valid if AI approaches facing the opposite direction from the direction the node points . If set to Corner then the Yaw should be pointed at the corner the AI is going to peak around when searching. |

### AINodeTalk

AINodeTalk is exactly the same as AINodeGuard, but AI stand at AINodeTalk nodes when having a conversation with another AI.

****

### AINodeUseObject

AINodeUseObject nodes are special general-purpose nodes. When an AI arrives at an AINodeUseObject, he animates using a set of animations defined by the SmartObject specified on the node. The set of animations usually includes a transition in and out, some looping idles, and some intermittent fidgets.

| **Object ** | Optional object to activate on arrival to the node. |
| --- | --- |
| **Radius ** | AI must be within this radius for the node to be a valid destination. |
| **Movement ** | Movement animation used to travel to the node (e.g. Run or Walk). |
| **SmartObject ** | SmartObject defining the purpose of the node. SmartObjects are defined in AIGoals.txt. |
| **PreActivateCommand ** | Command to run on arrival to the node. |
| **PostActivateCommand ** | Command to run when leaving the node. |
| **ReactivationTime ** | Amount of time before any AI may use this node again. |
| **FirstSound ** | Sound to play on arrival to the node. |
| **FidgetSound ** | Sound to play while playing fidget animations. |
| **OneWay ** | This node may only be used when approaching facing the direction matching the direction of the node. This was used to prevent SuperSoldiers from spinning around 180 degrees to destroy something on the wrong side. |

### AINodeVantage

AI run to an AINodeVantage to attack an enemy from a specific position. When an enemy’s position validates an AINodeVantage, the goal AttackFromVantage sends the AI to the AINodeVantage to attack.

| **IgnoreDir ** | Ignore the direction of a threat when determining if node is valid. |
| --- | --- |
| **Fov ** | Field of view that threat must be in to validate the node. |
| **Radius ** | AI must be within this radius for the node to be valid. |
| **ThreatRadius ** | If threat enters the ThreatRadius, node is invalidated. |
| **Object ** | Object or AINodeUseObject to activate on arrival at the node. (e.g. open a hatch or window shutters). |

### AINodeVantageRoof

AINodeVantageRoof is exactly the same as AINodeVantage. Having a separate node type allows goals to use specific nodes. Ninjas use special logic in AttackFromVantageRoof to determine when to jump onto a roof to attack from an AINodeVantageRoof.

****

### AINodeView

AI run to an AINodeView to attack an enemy that they cannot find a path to. If an AI cannot find a path to a threat, and the threat is in an AIVolumePlayerInfo, the goal AttackFromView sends the AI to an AINodeView associated with the AIVolumePlayerInfo.

| **Radius ** | AI must be within this radius for the node to be valid to go to it. |
| --- | --- |

---

## AIGoals

Goals control an AI’s behavior. AI are given a goalset in Dedit. Goalsets are defined in AIGoals.txt. Goals may also be added and removed dynamically through commands. Goals compete for activation, and only one goal is active at a time. Each goal has an Importance value, which determines its priority over other goals. Goals may optionally have required SenseTriggers and/or Attractors. Attractors are types of nodes, or types of SmartObjects. The Importance of a goal is zero when SenseTrigger and/or Attractor requirements are not satisfied. Goals may optionally have parameters that can be set through name=value pairs in commands. Goals not listed below were not used on NOLF2.

### Alarm

AI runs to activate an alarm when an enemy is seen, then runs back to position where enemy was seen and searches the AIRegion.

| **Importance ** | 32 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | AINodeUseObject with SmartObject Alarm |

### Animate

AI plays an animation on command. This goal is used to script animations.

| **Importance ** | 100 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | None |

**Parameters: **

| **ANIM=animname ** | Name of animation to play, from ltb file. |
| --- | --- |
| **LOOP=1 or 0 ** | Infinitely loop the animation if TRUE. |

### AttackFromCover

AI runs to the nearest valid AINodeCover and attacks from there.

| **Importance ** | 27 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | AINodeCover |

### AttackFromVantage

AI runs to the nearest valid AINodeVantage and attacks from there.

| **Importance ** | 27 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | AINodeVantage |

### AttackFromRandomVantage

AI runs to a random valid AINodeVantage and attacks from there.

| **Importance ** | 27 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | AINodeVantage |

### AttackFromRoofVantage

AI runs to the nearest valid AINodeVantageRoof and attacks from there. AI will only activate this goal if no other AI are at AINodeVantageRoof nodes, and at least 3 AI are currently attacking the same target. Used for ninja.

| **Importance ** | 28 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | AINodeVantageRoof |

### AttackFromView

AI runs to the nearest valid AINodeView and attacks from there. AI will only activate this goal if he cannot find a path to his target, and his target is an AIVolumePlayerInfo that has associated AINodeViews.

| **Importance ** | 27 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy SeeEnemyLean HearEnemyWeaponFire HearEnemyWeaponImpact SeeEnemyWeaponImpact HearAllyDeath HearAllyPain HearAllyWeaponFire HearEnemyDisturbance SeeEnemyDisturbance |
| **Attractors ** | AINodeView |

**Parameters: **

| **SEEKPLAYER=1 ** | AI will activate goal without first sensing anything if it receives the parameter SEEKPLAYER=1. |
| --- | --- |

### AttackMelee

AI attacks with a melee weapon, if he has one and target is within range set in AIBrain.

| **Importance ** | 24 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | None |

### AttackProne

AI drops to lie on stomach and attack with a ranged weapon. AI first checks for clear space in front of him. Only one AI may AttackProne at a time, and timing is coordinated between all AI to control frequency.

| **Importance ** | 26 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | None |

### AttackRanged

AI attacks with a ranged weapon, if he has one and target is within range set in AIBrain.

| **Importance ** | 23 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | None |

### AttackRangedDynamic

AI attacks with a ranged weapon, if he has one and target is within range set in AIBrain. Ai constantly moves between shots. Moves are randomly selected. Moves include strafing, flanking, and backing up.

| **Importance ** | 23 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | None |

### Chase

AI chases a target, and fires a weapon if allowed by the AIBrain. AI gets lost if chase path goes through an AIVolumeJunction, and target is not in sight. If target is lost, AI searches the AIRegion.

| **Importance ** | 20 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy (other senses are used to get AI to lose interest in searching). |
| **Attractors ** | None |

**Parameters: **

| **SEEKPLAYER=1 ** | AI will activate goal without first sensing anything if it receives the parameter SEEKPLAYER=1. |
| --- | --- |
| **NEVERGIVEUP=1 ** | AI will never lose interest in chasing if it receives the parameter NEVERGIVEUP=1. Used for ninjas in the trailer park, arena combat level. |
| **KEEPDISTANCE=1 ** | AI will only come close enough to get target in firing range if it receives the parameter KEEPDISTANCE=1. Used for HARM Villa, where guards apprehend player rather than attacking. |

### CheckBody

AI checks the body of a dead or knocked out ally. Ally is woken if knocked out. If ally is dead, AI disposed of the body. AI searches the AIRegion after checking the body.

| **Importance ** | 15 |
| --- | --- |
| **SenseTriggers ** | SeeAllyDeath (other senses are used to get AI to lose interest in searching). |
| **Attractors ** | None |

### Cover

AI runs to the nearest valid AINodeCover and takes cover. This goal only activates if AI has a ranged weapon. AI takes cover if he was not previously aware of a threat, and hears a bullet hit nearby.

| **Importance ** | 18 |
| --- | --- |
| **SenseTriggers ** | HearEnemyWeaponImpact |
| **Attractors ** | AINodeCover |

### Destroy

AI walks to AINodeUseObject and animates destroying something. Animation may cause AI to fire a weapon, or attack with bare hands. Used for SuperSoldiers destroying environment.

| **Importance ** | 4 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeUseObject with SmartObject Attackable or Smashable |

### DisappearReappearEvasive

AI disappears in a cloud of smoke, and reappears elsewhere. AI only disappears if an enemy is aiming a weapon at her. Frequency of any AI disappearing is coordinated between all AI. Used for ninja.

| **Importance ** | 31 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | None |

**Parameters: **

| **NOW=1 ** | AI will disappear on command if it receives the parameter NOW=1. |
| --- | --- |
| **OVERRIDEDIST=dist ** | Distance used to override ReappearDist set in AIBrain. |
| **DELAY=secs ** | Amount of time before AI reappears. |

### Distress

AI puts up hands in distress. This goal only activates if the AI is unarmed and a weapon is aimed at him. Distressing may lead to panicking, running to an AINodePanic.

| **Importance ** | 17 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | None |

### DramaDeath

AI pathfinds to an AINodeDeath, and animates a dramatic death. This goal only activates when the AI dies. Used for deaths of Pierre and Volkov.

| **Importance ** | 1000 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeDeath |

****

**Parameters: **

| **LAUNCH=1 ** | Launch AI in a parabola to the AINodeDeath if the parameter LAUNCH=1 is set, rather than pathfinding. |
| --- | --- |

### DrawWeapon

AI draws a weapon (or weapons) when a target is sensed, if he has any weapons.

| **Importance ** | 50 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy SeeEnemyLean SeeEnemyFootprint HearEnemyWeaponFire HearEnemyWeaponImpact SeeEnemyWeaponImpact HearEnemyFootstep HearEnemyAlarm HearEnemyDisturbance SeeEnemyDisturbance SeeEnemyLightDisturbance SeeAllyDeath HearAllyDeath HearAllyPain HearAllyWeaponFire SeeAllyDisturbance HearAllyDisturbance SeeAllySpecialDamage |
| **Attractors ** | None |

### EnjoyPoster

AI examines something on a wall. This was only used in the India Street level where the Police were hanging wanted posters.

| **Importance ** | 4 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeUseObject with SmartObject Examinable |

### ExitLevel

AI walks to an AINodeExitLevel, and deletes himself from the game. This is used to remove reinforcements who entered the level as the result of an alarm sounding. If the reinforcements cannot find any open guard nodes or patrol routes, they exit the level.

| **Importance ** | 7 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeExitLevel |

### Flee

AI runs to an AINodePanic and panics. This goal only activates if the AI is unarmed. If no panic node is found, AI runs away randomly.

| **Importance ** | 16 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy HearEnemyWeaponFire HearEnemyWeaponImpact SeeEnemyWeaponImpact SeeAllyDeath HearAllyDeath HearAllyPain HearAllyWeaponFire SeeAllyDisturbance |
| **Attractors ** | AINodePanic |

### FollowFootprint

AI follows an enemy’s footprints, then searches the AIRegion.

| **Importance ** | 14 |
| --- | --- |
| **SenseTriggers ** | SeeEnemyFootprint |
| **Attractors ** | None |

### GetBackup

AI runs to an AINodeBackup to call for help. If allies are within the stimulus radius of the AINodeBackup, and have the RespondToBackup goal, they will follow the AI back to where he last saw the enemy. If AI loses track of his target, he searches the AIRegion.

| **Importance ** | 30 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy SeeEnemyWeaponImpact |
| **Attractors ** | AINodeBackup |

### Goto

AI pathfinds to a specified node on command.

| **Importance ** | 8 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | None |

****

**Parameters: **

| **NODE=nodename ** | Name of node to pathfind to. |
| --- | --- |
| **MOVEMENT=movement ** | Type of movement animation (e.g. Walk or Run). |
| **AWARENESS=awareness ** | Awareness type of animation (e.g. Investigate). |

### Guard

AI stays within the radius of an AINodeGuard. The AINodeGuard may be assigned through a command, or the AI can find the nearest AINodeGuard. AI may wander within the radius, and activate goals associated with AINodeUseObject nodes that are guarded by the AINodeGuard. If AI leaves the guard radius to chase an enemy, he will return to the radius when he relaxes.

| **Importance ** | 5 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeGuard |

****

**Parameters: **

| **NODE=nodename ** | Name of node to assign as the AI’s AINodeGuard. |
| --- | --- |

### HolsterWeapon

AI holsters weapon. This goal only activates if AI has a weapon in his left and/or right hand.

| **Importance ** | 10 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | None |

### Investigate

AI investigates a disturbance. AI pathfinds to a disturbance and looks around, then searches the AIRegion. The AI’s movement (walking or running) depends how alarmed the AI is. Disturbances have different alarm levels, and multiple minor disturbances accumulate to equal a larger disturbance. If the disturbed object has an AINodeUseObject pointing at it, the AI may activate the object (e.g. close a drawer). If it is the first minor disturbance, or the AI cannot pathfind to the disturbance, he will just look in the direction of the disturbance.

| **Importance ** | 13 |
| --- | --- |
| **SenseTriggers ** | HearEnemyFootstep HearEnemyWeaponFire HearEnemyWeaponImpact SeeEnemyWeaponImpact HearAllyDeath HearAllyPain HearAllyWeaponFire HearEnemyDisturbance SeeEnemyDisturbance SeeEnemyLightDisturbance SeeAllyDisturbance HearAllyDisturbance SeeAllySpecialDamage SeeEnemyLean SeeEnemy |
| **Attractors ** | None |

### LoveKitty

AI walks to an AngryKitty proximity mine to pet it. If AI has been blown up before, he says something like “bad kitty” and does not walk over.

| **Importance ** | 9 |
| --- | --- |
| **SenseTriggers ** | SeeEnemyDanger |
| **Attractors ** | None |

### Lunge

AI lunges at a target and causes damage. Only one AI may lunge at a time. AI must have a path clear of static obstacles in front of her. Used for ninja.

| **Importance ** | 26 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | None |

### Menace

AI walks to an AINodeUseObject and places a menace animation. This goal is used to make AI appear agitated when they are actually unaware of a threat. Used in levels where the AI is intended to already know that the player is around.

| **Importance ** | 4 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeUseObject with SmartObject MenacePlace |

### MountedFlashlight

AI turns on/off flashlight mounted on rifle when entering/exiting a dark AIVolume.

| **Importance ** | 0 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | None |

### Patrol

AI patrols on route set between AINodePatrols.

| **Importance ** | 2 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | None |

### PlacePoster

AI places a poster at the location of an AINodeUseObject. Used for police.

| **Importance ** | 4 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeUseObject with SmartObject PostingPlace |

### ProximityCommand

AI runs a command when he comes within proximity of an enemy. Command is set on CAIHuman object in Dedit. Used for HARM guards in HARM villa, who apprehend the player rather than attacking.

| **Importance ** | 1000 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | None |

****

**Parameters: **

| **RESET=1 ** | Reset the command to run again if AI receives the parameter RESET=1. |
| --- | --- |

### PsychoChase

AI chases an enemy relentlessly. No stimulus is required to activate the goal. AI ignores junctions, and never gives up.

| **Importance ** | 21 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | None |

### RespondToAlarm

AI responds to an alarm. Alarms specify AIRegions where AI respond or go alert. If AI is in an alert AIRegion, he looks around. If he is in a respond AIRegion, he runs to the alarm. The alarm specifies a list of AIRegions to search after reaching the location of the alarm.

| **Importance ** | 19 |
| --- | --- |
| **SenseTriggers ** | HearEnemyAlarm |
| **Attractors ** | None |

### RespondToBackup

AI responds to an ally’s call for backup. AI must be standing within the stimulus radius of an AINodeBackup that an AI with the goal GetBackup is running to. Responding AI follow the AI that called for backup back to the last place the enemy was seen, and search the region. If AI has called for backup too many times, without finding anything, allies believe he is crying wolf, and ignore his call for help.

| **Importance ** | 19 |
| --- | --- |
| **SenseTriggers ** | SeeAllyDistress. AI loses interest in RespondToBackup if he senses SeeEnemy. |
| **Attractors ** | None |

### Retreat

AI jumps back to retreat from an enemy. AI only retreats from enemies wielding melee weapons. Used for ninja.

| **Importance ** | 25 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | None |

### Ride

AI plays a riding animation when at an AINodeUseObject. Used for Pierre getting shot up on steam for the Underwater Base boss fight.

| **Importance ** | 500 |
| --- | --- |
| **SenseTriggers ** | SeeEnemy |
| **Attractors ** | AINodeUseObject with SmartObject Ride |

### Search

AI searches a list of AIRegions on command. Used for scripting AI to search, without first sensing a disturbance.

| **Importance ** | 12 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | None |

****

**Parameters: **

| **REGION=list of AIRegions ** | Name of node to pathfind to. |
| --- | --- |
| **MOVEMENT=movement ** | Type of movement animation (e.g. Walk or Run). |

### SerumDeath

AI dies instantly if hit with AntiSuperSoldierSerum while powered down. Any other damage type reduces hitpoints but does not kill AI. When AI reaches zero hitpoints, he powers down and regenerates full hitpoints, then powers up. Used for SuperSoldiers.

| **Importance ** | 1000 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | None |

### Sleep

AI sleeps at an AINodeUseObject. Visual senses are turned off while sleeping.

| **Importance ** | 4 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeUseObject with SmartObject Bed |

### Sniper

AI stands alert and fires at any enemy in view. If a node is not specified, AI will find the nearest node owned by his AINodeGuard. If AI is shot, he will choose a new node. Sniper nodes may be listed as view nodes of AIVolumePlayerInfos, so that AI chooses the best node to attack an enemy in specified areas.

| **Importance ** | 29 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeUseObject with SmartObject Sniper |

****

**Parameters: **

| **NODE=nodename ** | Name of node to go to, to fire from. |
| --- | --- |

### SpecialDamage

AI reacts to special damage types. Special damage includes sleeping, stun, bear trap, glue, laughing, slippery, bleeding, burn, choke, ASSS, poison, and electrocute damage types. AI plays a looping, incapacitating damage animation with a transition in and out. Some damage types knock the AI out, at which point weapons may be stolen. If the AI wakes up unarmed, he panics. Otherwise, he searches the AIRegion.

| **Importance ** | 1000 |
| --- | --- |
| **SenseTriggers ** | Anything sensed interrupts the AI’s search. |
| **Attractors ** | None |

****

**Parameters: **

| **PAUSE=1 or 0 ** | AI stops decrementing the damage timer when PAUSE=1, and continues when PAUSE=0. This command parameter is used when a player picks up a knocked out AI. |
| --- | --- |
| **SLEEPFOREVER=1 ** | AI is put to sleep and never wakes up if it receives the parameter SLEEPFOREVER=1. This was used to knock out Cate in Japan co-op. |

### Talk

AI walks to an AINodeTalk to have a conversation with another AI. AI waits until all dialogue participants are in place. If AI is waiting at the node and notices a disturbance, he may investigate, and then return to the AINodeTalk.

| **Importance ** | 6 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | None |

****

**Parameters: **

| **RETRIGGER=1 ** | AI retriggers the dialogue object on arrival to the AINodeTalk if it receives the parameter RETRIGGER=1. |
| --- | --- |
| **DISPOSABLE=1 ** | If AI receives the parameter DISPOSABLE=1, the conversation will never occur if the AI is drawn away to investigate a disturbance. |
| **GESTURE=animname ** | Name of animation to play while having a conversation. Used for scripting gesture animations from dialogue objects. |
| **MOVEMENT=movement ** | Type of movement animation (e.g. Walk or Run). |

### Work

AI walks to an AINodeUseObject and does some work. Work may include anything a SmartObject is set up to do. Work includes desk work, smoking, urinating, stretching, etc.

| **Importance ** | 4 |
| --- | --- |
| **SenseTriggers ** | None |
| **Attractors ** | AINodeUseObject with SmartObject Work |

---

## SmartObjects

****

SmartObjects are defined in AIGoals.txt. A SmartObject is a template that announces an AINode’s purpose to an AI, and describes the animation set to play while using the SmartObject. In general, the AI will play a single Action animation, or a looping Idle animation with optional intermittent Fidget animations.

NOLF2 had 87 SmartObjects covering a wide variety of objects and activities, such as desks, microscopes, urinals, places to hammer, smoke, and lean. This document will not enumerate each of the NOLF2 SmartObjects, but will describe the properties and commands that define a SmartObject.

Here are a couple examples of SmartObjects:

// A desk that AI can sit at and do work.

[SmartObject1]

Name = "Desk"

Flag0 = "WorkItem"

Cmd0 = "HumanUseObject Activity=DeskWork Pose=Sit LoopTime=[30.0,60.0] FidgetFreq=[8.0,10.0] LockNode=FALSE"

AddAnimsLTB0 = "\chars\models\anims\desk.ltb"

// A file cabinet that serves 2 purposes to AI:

// 1) AI may open and close the cabinet to do work in

// its default state.

// 2) AI may notice that a player has opened the cabinet,

// and put it into a disturbed state.

[SmartObject5]

Name = "FileCabinet"

Flag0 = "WorkItem"

Cmd0 = "HumanUseObject Activity=FilingHigh LoopTime=[15.0,25.0] LockNode=FALSE"

Flag1 = "Disturbance"

Cmd1 = "HumanUseObject Action=CloseDrawer LockNode=FALSE"

StateDefault0 = "WorkItem"

StateDisturbed0 = "Disturbance"

AddAnimsLTB0 = "\chars\models\anims\filecabinet.ltb"

**SmartObject Properties: **

| **Name ** | Name of SmartObject. |
| --- | --- |
| **Flag0-N ** | Type of SmartObject. A SmartObject may have any number of types. Types are listed in SmartObject Types table below. |
| **Cmd0-N ** | Command for SmartObject type flag with matching index. Parameters for SmartObject commands are described in the table below. |
| **StateDefault0-N ** | SmartObject types active in the Default state. |
| **StateDisturbed0-N ** | SmartObject types active in the Disturbed state. |
| **AddAnimsLTB0-N ** | Child ltb files containing extra animations that need to be loaded for the AI to interact with this SmartObject. |

**SmartObject Types: **

| **Alarm ** | AI can sound an alarm at the node. |
| --- | --- |
| **Attackable ** | AI can attack from the node. |
| **Bed ** | AI can sleep at the node. |
| **Coverable ** | AI can take cover at the node. |
| **DamageType ** | SmartObject used to define animation resulting from taking special damage (glue, bear traps, sleeping, stun, slippery, poison, ASSS, bleeding, burn, choke, electrocute). |
| **Disturbance ** | AI can investigate a disturbance at the node. |
| **Examinable ** | AI can look at something on a wall. |
| **LightSwitch ** | AI can turn on/off a light at the node. |
| **MenacePlace ** | AI can play a menace at the node. |
| **PostingPlace ** | AI can post something at the node. Used for police. |
| **Ride ** | AI can ride something. Used for Pierre getting shot up by steam. |
| **Smashable ** | AI can destroy something. Used for SuperSoldiers. |
| **Sniper ** | AI can shoot someone from the node. |
| **WorkItem ** | AI can do some work at the node. (e.g. sit at a desk) |

****

**SmartObject Command Parameters: **

| **HumanUseObject ** | All SmartObject commands start with HumanUseObject, because they are setting parameters in the AIHumanStateUseObject. |
| --- | --- |
| **Action ** | Action animation property from AnimationsHuman.txt. |
| **Activity ** | Awareness animation property from AnimationsHuman.txt. |
| **Mood ** | Mood animation property from AnimationsHuman.txt. |
| **Pose ** | Posture animation property from AnimationsHuman.txt. |
| **WeaponAction ** | WeaponAction animation property from AnimationsHuman.txt. |
| **LoopTime ** | Minimum and maximum for random range of seconds to animate. [0, 0] means loop infinitely. |
| **FidgetFreq ** | Minimum and maximum for random range of seconds between fidget animations. |
| **LockNode ** | FALSE if AINode is re-usable after an AI has used this SmartObject. |

****

---

## AI Commands

Level Designers can modify AI behavior by sending commands to AINodes, AIVolumes, and AIs themselves.

**Commands to AINodes: **

| **ENABLE ** | Enable the AINode, so that AI may use it. |
| --- | --- |
| **DISABLE ** | Disable the AINode, so that AI will ignore that it exists. If AI was currently at the node, AI will lose interest in whatever he was doing. |

**Commands to AIVolumes: **

| **ENABLE ** | Enable the AIVolume, so that AI may use it for pathfinding. |
| --- | --- |
| **DISABLE ** | Disable the AIVolume, so that AI will ignore that it exists, and may not pathfind through it. |
| **LIT ** | Make the AIVolume appear lit to the AI. |
| **BRIGHT ** | Make the AIVolume appear lit to the AI. |
| **UNLIT ** | Make the AIVolume appear unlit to the AI. |
| **DARK ** | Make the AIVolume appear unlit to the AI. |

**Commands to AIVolumePlayerInfos: **

| **ON ** | Enable the AIVolumePlayerInfo, so that player may use its hiding properties, and SenseMask will be enabled. |
| --- | --- |
| **OFF ** | Disable the AIVolumePlayerInfo, so that player may not use its hiding properties, and SenseMask will be disabled. |

**Commands to AI: **

| **HIDDEN 1 or 0 ** | AI is hidden if HIDDEN 1, and exposed if HIDDEN 0. |
| --- | --- |
| **CINERACT animname ** | AI plays a specified animation once. |
| **CINERACTLOOP animname ** | AI loops a specified animation. |
| **TRACKLOOKAT ** | AI tracks player with his head. |
| **TRACKAIMAT ** | AI tracks player with his head and torso. |
| **TRACKNONE ** | AI disables head and torso tracking. |
| **ALIGNMENT alignment ** | Set an AI’s alignment. **Alignment is Case Sensitive!! Valid alignments are: GoodCharacter BadCharacter NeutralCharacter |
| **DAMAGEMASK mask ** | Set an AI’s damage mask to a mask listed in AIButes.txt, or to None. |
| **GOAL_xxx name1=value1 name2=value2 ... ** | Set parameters on a goal that the AI already has, where xxx is the name of a goal. For example: GOAL_chase SEEKPLAYER=1 KEEPDISTANCE=1 |
| **GOALSET goalset ** | Change an AIs goalset. Goalset may be any goalset listed in AIGoals.txt. |
| **ADDGOAL goal name1=value1 name2=value2 ... ** | Add a goal to an AI, and optionally set parameters on the new goals. For example: ADDGOAL goto NODE=AINodeGoto1 MOVEMENT=run |
| **REMOVEGOAL goal ** | Remove a goal from an AI. |
| **GOALSCRIPT goal1 name1=value1 name2=value2 ... goal2 name1=value1 name2=value2 ... ** | Add a sequence of goals to an AI. This is used to script an AI to do a series of actions. There can be any number of goals, and each goal can set any number of parameters. For example: GOALSCRIPT goto NODE=AINodeGoto1 MOVEMENT=run animate ANIM=dance LOOP =1 |

---

## AI Debug Keys

If you build debug or release configurations, some helpful debugging features will be enabled while running the game. See the documentation on building the Nolf2 source code for more information on this topic.

****

| **F2 ** | Spectator Mode: Player can fly the camera around the level freely without physics. |
| --- | --- |
| **F5 ** | AI Info: AI’s names will float above their heads, and a panel of information will be displayed to the AI that the camera aims at. Information includes their current hitpoints, goal, and state. Hitting F5 multiple times displays different types of information. |
| **F7 ** | GodMode: Runs the cheats mpgod, mpkfa, mpmods |
| **F11 ** | Show AIVolumes and Paths: Render wireframes and names for AIVolumes, and draw AI paths. Hitting F11 multiple times cycles through 4 modes: Show AIVolumes Show AI Paths Show AIVolumes and Paths Show AIVolumePlayerInfos |
| **SHIFT + F11 ** | Show AINodes: Render wireframes and names for AINodes. Hitting SHIFT + F11 multiple times cycles through the different node types. |

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: NOLF2_AI.md)2006, All Rights Reserved.
