| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# About the NOLF2 AI

The NOLF2 AI is a complex system of components integrated thoroughly with the game code. Because the AI system is not entirely modular, you may find it difficult to extract and implement the system in your own games. A better approach for Developers attempting to implement their own AI is to use the NOLF2 AI game code as a reference. This section is intended for both Developers and Level Designers, and provides a general overview of the components used by the NOLF2 AI, and the objects used in DEdit to implement AI in a level.

Read the following sub-sections to get a general understanding of how the NOLF2 AI operates:

- [About the Operation of NOLF2 AI ](#AbouttheOperationofNOLF2AI)
- [About the NOLF2 AI Components ](#AbouttheNOLF2AIComponents)
- [About DEdit AI Objects ](#AboutDEditAIObjects)

---

## About the Operation of NOLF2 AI

NOLF2 does not differentiate between NPC and combat AI. There are a handful of generic systems that work in combat and non-combat si tuations, for both friendly NPC and enemy behavior. The major systems are Pathfinding, Goals, SmartObjects, Stimulus and Senses.

AIs are supplied with a set of Goals that determine their actions. Actions are selected based on an AI's proximity to SmartObjects and on sensed stimuli. The NOLF2 pathfinding system is unique in that it not only finds a path, but also instructs an AI what type of movement is required. For example, a ninja may run from point A to point B, or may leap over cars, fences or trailers to get there.

### Collaboration

Each of the AI systems is fairly simple on its own. The challenge comes in anticipating what will happen when all of the systems work together. What happens when an AI is working at his desk and he sees an ally run by on fire? What happens when a ninja spots the player and jumps from rooftop to rooftop to try to get to an alarm, or when she is suddenly electrocuted while in mid-air? These are the types of situations that arise in NOLF 2.

Many action games subscribe strictly to the "monster closet" game design philosophy, where AIs stand still waiting for the player to trigger them to rush out and attack. Most of the levels in NOLF 2 are populated with AIs who are going about their business. The player is free to explore the level, and the AIs will respond to their observations of the world around them.

Relaxed AIs wander between SmartObjects where they take naps, use restrooms, do deskwork, or turn on a radio and dance a jig. If an AI is at his desk and notices someone turned off a light, he will get up and investigate, turn the light back on and go back to his desk. In a combat scenario, an AI may knock the desk over and take cover behind it, while blindly firing over the top at an enemy who barged into his office.

Alerted AI will chase the player, but there are always opportunities to elude the enemy. For example, the bear trap can stop the pursuers in their tracks, buying the player time to find a hiding spot. When the enemies finally free themselves, they will activate goals to alert allies and sound alarms to bring in reinforcements. As new AIs join the hunt, they will spread out to search the area, but if the player is not found, the aggressive goals will deactivate as the AIs concede defeat and return to their relaxed behaviors. The goal system provides AIs with lower priority behaviors to fall back to, giving the player the opportunity to recover from a dangerous situation.

Monolith Level Designers have filled the levels with SmartObjects that provide opportunities for the AI to showcase their behaviors. Many levels are packed with desks, drinking fountains, alarms, radios, microscopes, urinals, roofs to jump on, cars to jump over, beds, light switches and lots more. The AI dynamically decides to activate goals that are appropriate for their current situation, making gameplay unpredictable and replayable, adapting to however the player approaches the level.

### The Goal-Based Approach

NOLF 2 uses a goal-based approach to AI design because it produces a living world with AIs that behave believably and have some purpose other than killing the player. This approach also creates a system that lets players use stealth. If players are careful, they can sneak around observing the enemy unnoticed. If players are discovered, they can get away.

The advantage of a goal-based system is that it gives the AIs' behavior consistency. Each individual AI does not need to be scripted to respond to different situations. Goals take care of responding appropriately to whatever situation an AI finds himself in. This gives depth to the gameplay, as the player has freedom to do whatever they want. For example, in one of the earlier missions, Cate is assisted by a Soviet pilot. If the player wants to tranquilize the pilot, they have the option to perform this action.

Other benefits of goal-based AI are that it's easier to code, and easier to globally modify behavior. Each goal is its own module, so each behavior can be tested individually. Once it works on its own, the goal can be added to an existing goal set, and instantly, every AI in the game that has this goal set has a new behavior. This is a lifesaver during bug-fixing beta periods, as bug fixes to AI behavior can usually be applied to the entire game without requiring any work from the over-loaded Level Designers.

### Development and Testing

Once an AI is supplied with a full palette of goals (our average AI has 26 goals in its goal set), the AI may behave in unexpected ways. This unpredictability is great for gameplay, but can cause trouble during development and testing.

he flexibility of the AI goal system allows for a shared base of low-level functionality upon which we can add specific behavior and tendencies unique to each specific AI type. This works well to help keep things fresh as the game progresses. Instead of a new enemy AI being the same as the last but with a different model skin, facing a new AI type becomes a new obstacle for the player to overcome, requiring adaptive strategies and a degree of problem solving.

The most notable aspect of the NOLF 2 AI compared with many others in the genre is the AI's unscripted awareness of the world around them. They really k now what is going on, and they interact with the environment and incorporate feedback into both their idle activities and combat strategies. This allows the AI to act and react as you would expect a real opponent to.

The major limiting factor in the level of complexity of the AI is development time. Every new behavior takes time to tweak and to perfect its interaction with other existing behaviors. Sometimes, problems do not surface immediately because different levels provide different opportunities for the AIs to showcase their behaviors.

The engine's performance also factors into the number of AIs that can be active at once because AIs need to share the processing resources for physics and visibility calculations. Each AI needs to do expensive physics calculations to find the floor while they are moving. Expensive line of sight calculations are also required to allow the AIs to determine what is visible to them.

Monolith has put a lot of effort into making its AIs believable. AIs can split up to encircle a target, alert their comrades to threats, even shout at each other to get out of the way if they can't get a clear shot at the player. They work at desks and drink coffee and go to the restroom. They even sometimes trip and fall on their faces when vaulting over a railing to chase the player. These behaviors help make enemies seem believable, but if it is a choice between what's realistic and what's fun, fun always wins. For example, early on Monolith had enemies carry out organized, efficient searches when they were alerted to the player's presence. The problem was that they always found the player, just like they would in real life. So Monolith made them slightly less competent. The AIs still search, which feels authentic, but instead of looking in the really likely hiding spots, they search NEAR the likely hiding spots, which makes it easy for you to observe them without getting caught. Monolith refers to this as "movie realism."

### Bug Problems

All of the NOLF 2 human AIs respond to tranquilizers and tazers by passing out. This leads to some interesting problems in UNITY headquarters, where the player can tazer a friendly scientist, then activate him while he laid on the ground sleeping. He would turn his head and say "Hi Cate, Mildred said to say thank you for the flowers."

The addition of ambient life, such as rats and rabbits, sounded simple enough. Ambient creatures are simply AI with a very limited goal set. They wander around, and run in fear from humans. Even the simplest addition can cause unpredicted results. The first problem was that our interactive music system is tied to the collective AI state of awareness. When the player scared a rabbit, the music intensity would go up to its aggressive themes. This made scaring a rabbit a very dramatic event.

The NOLF 2 AIs are also aware of each other, and notice if someone else is alarmed. This gives some desirable behavior for free. If the player agitates a rabbit and it runs by the enemy, it causes a disturbance that may alert the enemy to the player's presence. The problem was that rabbits were getting alarmed by one AI, and running by another AI. This meant that even if the player had never even moved yet, the AI were already suspicious of an intruder.

---

## About the NOLF2 AI Components

The NOLF2 AI aggregates many sub-components to implement its logic. The list below describes some of the sub-components used to implement AI logic:

**CAITarget— **This class manages tracking of the AIs current target. It updates the target's visibility, position, threat status, and other attributes.

**CAIGoalMgr— **This components is responsible for creating autonomous behavior in AIs. It maps sensory events into state transitions, and implements a higher level logic than what exists at the machine level.

**CAISenseMgr— **This component is responsible for managing all of the AIs senses. Each individual sense is implemented as a derivative of CAISense. The senses track the state of their stimulation and reaction times against a stimulus.

**CAISense— **This is the parent class for the CAISenseMgr.

**CAISenseRecorder— **This component handles remembering what sensory events the AI has already experienced.

**CAnimationMgr— **This is the parent class that handles animations of all AIs.

**CAnimationContext— **Part of the CAnimationMgr system specific to human AIs.

**CAnimator— **The parent class for all non-human animations.

**CAIState— **The parent class for all CAI states. The AI state is the most important logical aspect of the AI, and performs the bulk of decision making for the AI. Some examples of state are: attack, chase, check body, talk, and search. The state shares a lot of the similar Update and Handle methods as the AI, because the AI commonly defers these methods down to the state. For example, the state has a Pre/Post/Update, a call to update senses, a call to update animation, a call to update the dynamic music, etc. The state also handles similar events such as AI damage, trigger commands, model strings, and senses. AIs switch states in one of three ways: by being send a command by a level designer, by the goal subsystem, or by a message from another state. All state transitions occur by the AI receiving a text string with the name of the state and its parameters. State text string messages are handled by CAI::HandleCommand and CAIState::HandleNameValuePair.

**CAIHumanStrategy— **Most states share some common sub-behaviors. Sub-behaviors are encapsulated in strategies. The CAIHumanStrategy base class contains sub-states that present a common interface to the state. For example, the CAIHumanStrategyCover base class is used in CAIHumanStateAttackFromCover. The concrete base classes that actually get instantiated are CAIHumanStrategyCover1WayCorner, CAIHumanStrategyCoverBlind, and all of the other cover type base classes. The CAIState uses the same common interface to the base strategy. The term "Strategy" comes from design patterns.

[Top ](#top)

---

## About DEdit AI Objects

The objects discussed in this topic are the components available through DEdit that allow level designers to implement the NOLF2 AI in their levels. Like all DEdit objects, each AI object has properties that allow you to set the specifics of the AI actions and reactions.

### CAI obects

The CAIHuman class is the basis for all AIs in the NOLF2 game code. When you insert a CAI object into your level, you must specify a model, brain, and basic goalset for the AI. For additional information, see [Using AI Brains and Goals ](Goals.md).

### AI volume objects

AI volumes allow AIs to pathfind, controlling where they can move. AIVolumes are used strictly for navigation, and do not control raycasting or what the AI can see. AIs never move into areas where AIVolumes do not exist unless they are killed, blasted, or otherwise moved against their will. For specific information about the different types of AIVolumes, see [Specifying an AIVolume ](Pathing.md#SpecifyinganAIVolume).

### AI node objects

AI nodes are locations and areas within an AI volume that specify particular actions (and animations) an AI can perform based on their current goals. Some examples include: AINodeGuard, AINodePatrol, AINodeUseObject, and AINodeCover. For additional information, see [Using AINodes and Goals ](Goals.md#AINodesandGoals).

### AI regions

AI regions are a collection of AIVolume objects bound to a single AIRegion object. Each AIVolume must belong to one, and only one, AIRegion. The primary purpose for AIRegions is to for searching. AIRegions also control AI response to alarms, and allow you to set which AIRegions respond or become alert when an alarm gets sounded. Additionally, you can specify regions that responding AI search after they have responded to an alarm and visited all of the AINodeSearch nodes in the region the alarm exists in. For additional information, see [Specifying an AI Region ](Pathing.md#SpecifyinganAIRegion).

### AI goals

Each character AI has a default goal set of "None" when they are initially placed in a level. You can change this goal, giving the character an initial goal equivalent to the action you want them to perform. You can set initial goals in the Commands parameter of the CAI class. During gameplay you can add or remove goals from AI by using the addgoal and removegoal commands. For additional information, see [Using AI Brains and Goals ](Goals.md).

### AI commands

CAI objects respond to message commands just like any other object in the NOLF2 game code. The [Using AI Commands ](Comnds.md)section provides a detailed list of each of the commands an AI can receive. During gameplay you can send commands to AIs to perform a variety of operations, including: modifying attributes, changing alignment, moving to a location, activating, facing a specific direction, and many others. You can also use commands in AI goal scripting to create cinematic sequences and activate specific animations.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: WAI\AboutAI.md)2006, All Rights Reserved.
