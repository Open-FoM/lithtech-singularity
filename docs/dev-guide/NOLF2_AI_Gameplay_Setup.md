| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

**NOLF2 AI Gameplay Setup **

This document describes how the nolf2 AI systems were used to setup basic gameplay. This is not an exhaustive description of every piece of the AI systems, but instead an overview of the most commonly used pieces, highlighting things that may not be obvious by looking at the code or Dedit properties.

## GoalSets

In general, AI in nolf2 are assigned either the Patrol, Guard, or Reinforcement GoalSet . There are a total of 46 GoalSets defined in aigoals.txt, but most are variations of Patrol or Guard for special characters (e.g. Ninjas, SuperSoldiers , Roots, etc). Please see aigoals.txt for details of what Goals are in each GoalSet .

Patrol, Guard, and Reinforcement share identical combat behavior, but vary in their relaxed and investigative behaviors. Below are descriptions of the expected behaviors with each of these GoalSets .

### Patrol GoalSet

· The Patrol GoalSet includes the DefaultRequired and DefaultBasic GoalSets to define the standard combat behavior.

· Patrol’s relaxed behaviors are defined by the Goals Patrol and Work.

· AI will walk along a patrol path defined by LevelDesign through AINodePatrols .

· A patrolling AI owns his path, and no other AI can use it until he dies or otherwise relinquishes it.

· A patrol path can be assigned by specifying a node on the path in the AI’s Initial Command: goal_patrol node=AINodePatrol9.

· If no node is specified, AI will find the nearest unowned AINodePatrol , and take ownership of that node’s path.

· A patrolling AI may not own an AINodeGuard . Patrol and Guard are mutually exclusive.

· If a patrolling AI walks near an unowned work node ( AINodeUseObject with SmartObject set to Work), he will stop patrolling to do some “work,” then continue patrolling. Patrolling AI will ignore any work nodes that are owned by an AINodeGuard .

· A patrolling AI may animate an investigative walk rather than the routine patrol walk through the command: goal_patrol awareness=investigate. This is useful for levels where AI are intended to be eternally suspicious regardless of stimuli.

· Patrolling AI will turn off lights as they leave an AINodePatrol , and turn on lights at a destination AINodePatrol .

### Guard

· The Guard GoalSet includes the DefaultRequired and DeafultBasic GoalSets to define the standard combat behavior.

· The Guard GoalSet includes the Sniper Goal.

· Guard’s relaxed behaviors are defined by the Goals Guard, Work, and Menace.

· A guarding AI will stay within the return radius of his guard node. If he is drawn outside of this radius, he will walk to return to it.

· A guarding AI owns his guard node, and no other AI can use it until he dies or otherwise relinquishes it.

· An AINodeGuard can be assigned by specifying a node in the AI’s Initial Command: goal_guard node=AINodeGuard22.

· If no node is specified, AI will find the nearest unowned AINodeGuard .

· A guarding AI may not own any AINodePatrols . Patrol and Guard are mutually exclusive.

· The guarding AI owns his guard node, and the guard node may own additional nodes. Owned nodes are AINodeUseObjects used for the Work, Menace, and Sniper Goals.

· An AI that owns a guard node will randomly select work nodes owned by his guard node, rather than always going to the next nearest work node. This AI may not use any unowned work nodes. The same applies to menace nodes.

· Menace nodes are used exactly like work nodes, but are used in levels where the AI is supposed to already be aware of the enemy’s presence, and should appear permanently agitated, even when the AI system thinks the AI is relaxed. In other words, when the AI is not in combat he will run between menace nodes ( AINodeUseObjects with SmartObject set to Menace), and play aggressive animations.

· When a guarding AI investigates a disturbance, he will stop at the edge of his guard radius and play an alert animation, rather than walking all the way to the disturbance.

· A Guarding AI will not go to search nodes outside of his guard radius.

· If the guarding AI has previously seen the player, heard an alarm, or been shot, he will ignore his guard radius and investigate anywhere. Once the AI returns to a relaxed state of awareness, he will start paying attention to his guard radius again.

· If the AI notices a disturbance outside of his guard radius, he will not speak. This prevents an AI from repeatedly saying “I hear you !, ” “What was that!,” “I know you’re there!,” but never coming to investigate.

· If an AI currently owns a talk node, he will ignore his guard node. This is covered in more detail later in the section about the Talk Goal and conversations.

### Reinforcement

· The Reinforcement GoalSet is used for AIs who spawned as reinforcements, usually from a command in an alarm.

· The Reinforcement GoalSet includes the DefaultRequired and DeafultBasic GoalSets to define the standard combat behavior.

· The Reinforcement GoalSet includes the Sniper Goal.

· Reinforcement’s relaxed behaviors are defined by the Goals Guard, Work, ExitLevel , and Menace. (See the Guard GoalSet above for more info on Menace).

· A reinforcement AI will enter the level and respond to whatever is currently going on. Once the situation dies down, and the AI returns to a relaxed state of awareness, he will look for unclaimed guard and patrol nodes. If one is found, he will claim a node and activate the Guard or Patrol goal, and behave like an ordinary AI that is guarding or patrolling.

· Nodes may be unclaimed because the AI that previously owned them have died. In this case the reinforcements are replenishing the level to maintain some number of AI, and keep the level at approximately the same difficulty.

· Alternatively , the level may intentionally contain a surplus of nodes, so that the level will become harder once reinforcements are brought in.

· If no unclaimed nodes are found, the AI will look for an AINodeExitLevel . If an ExitLevel node is found, the AI will walk to the node and get removed from the level. These are normally placed in areas the player cannot get to, so it is not obvious the AI is getting instantly removed from the game.

· The AI will not look for an ExitLevel node until he has been in the level for at least 5 seconds. This restriction prevents an AI from spawning near an ExitLevel node, and immediately removing himself before noticing an alarm sounding, or other stimuli.

## Scripting

Scripting was avoided as much as possible in nolf2, but there were situations where some amount of scripting was necessary to drive the story or vary the gameplay for specific levels. Here are the ways we accomplished scripted behaviors without breaking the autonomy of the AI.

· **Enable/Disable Node Commands **: The majority of scripting in nolf2 involved enabling and disabling nodes, or assigning new nodes with command, such as: goal_guard node=AINodeGuard45. This gives control over where AI are idling while still allowing some randomness and allows them to react normally to whatever situation they find themselves in.

· **GoalScripts **: GoalScripts supply an AI with a sequence of Goals to complete. When one Goal is satisfied (e.g. the AI has arrived at the destination node of a Goto Goal), the next Goal activates. GoalScripts may be interrupted if the AI notices a disturbance, and the GoalScript will continue once the AI returns to a relaxed state of awareness. GoalScripts are useful for making an AI go fro place-to-place and animate.

· **Search **: Normally AI search after activating other Goals (e.g. after investigating or chasing and losing the target). In cases where AI are spawned with the intention of immediately searching, they may be given the Search Goal, which is a scripted Search. AI with the Search Goal can be given a list of comma-separated regions to search: addgoal search region=airegion01 ,airegion02,airegion03 . AI who search through normal means will only search their current region.

· **Patrol **: Another way to script searching behavior is to spawn AI and assign them patrol routes. The Patrol Goal may be instructed to play investigative walking animations: addgoal patrol awareness=investigate. In some nolf2 levels, we periodically spawn a patrolling AI to ensure the level will never be completely empty (and boring to the player), even when all AI have been killed leaving no one to sound alarms. An AI may be spawned, instructed to patrol, and then removed through a command on a destination patrol node.

### Conversations

The Talk Goal allows AI to coordinate meeting up, having a conversation, and going on to do other things. The Talk Goal also handles responding to interruptions, and possibly returning to have the conversation later. There are basically two options for setting up AI to have a conversation:

**Statically Positioned Conversation Participants: **

· AI are placed at AINodeTalk nodes and given initial commands of: addgoal talk node=AINodeTalk04. AI will walk to Talk nodes and wait for a dialogue object to be triggered.

· If both AI are within the radii of their Talk nodes when the corresponding dialogue object is triggered, they will have their conversation.

· When the conversation ends, the dialogue object will run its finished and cleanup commands.

· An AI with a Talk Goal and Talk node will ignore his Guard and Patrol Goals. The Talk Goal overrides other relaxed goals, so the AI may still have a full GoalSet , such as Patrol or Guard.

· If an AI is at a Talk node, and notices a disturbance, he will investigate. Guard node radii will be ignored.

· If the dialogue object is triggered while one or both of the dialogue participants are not relaxed, the dialogue will not play, and the dialogue object’s cleanup command will be run.

· If the dialogue object was not triggered while the AI was investigating, when the AI returns to a relaxed state of awareness, he returns to his Talk node and continues waiting for the dialogue to be triggered.

· The Talk Goal parameter disposable=1 tells the AI not to return to his Talk node if he was disturbed while waiting for the conversation. In this case, the AI will simply return to his normal relaxed behaviors and forget about having the dialogue.

**Dynamically Positioned Conversation Participants: **

· AI may be anywhere in the level, running various Goals. They do not need to be near their Talk nodes.

· When a dialogue object is triggered, commands are sent to the dialogue participants: addgoal talk node=AINodeTalk09 retrigger=1. If either AI are outside of their Talk node radii, the dialogue will not start. The parameter retrigger=1 tells the AI to retrigger the dialogue object once the AI arrive at their Talk nodes.

· Once both AI are in position, the conversation starts.

· When the conversation ends, the dialogue object will run its finished and cleanup commands.

· The above steps may be used to get AI to walk towards each other and start talking, or to make an AI stop working and start talking to someone.

A couple more general notes about the Talk Goal:

· While the Talk Goal is active, AI will face each other and talk while playing their basic idle animation (usually 3StGd).

· AI may be instructed to play gesture animations at key points in the conversation. From the dialogue object, send a command to the AI: goal_talk gesture=NameOfAnim.

· If a dialogue object is triggered, and one or more of the dialogue participants does not exist (probably because he is dead), the dialogue will not play and the dialogue object’s cleanup command will run.

## Goal Notes

Below are points of interest about various Goals:

### Chase

· Once the Chase Goal is activated (by seeing the enemy) the AI will never lose interest in chasing, with the exceptions described below.

· An AI may lose interest in chasing if another goal activates that puts the AI in a state of awareness other than alert (so, relaxed or suspicious). Normally any Goals that activate in favor of Chase are other alert Goals, such as AttackRanged . The exceptions are Goals like SpecialDamage , which incapacitate the AI and then send him searching for his attacker. When an AI wakes up from SpecialDamage , he is investigative and loses interest in chasing.

· An AI may lose interest in chasing if he loses his target in a Junction volume, and gives up.

· Chase Goal Parameters:

o **SeekPlayer ****=1 **: ****This parameter instructs the AI to immediately start chasing a player, even if no stimuli have been sensed. This allows the AI to cheat in combat levels where the AI is intended to already know where the player is. The AI ignores Junction volumes while seeking the player. Once the player has been seen, the AI falls back to normal chasing behavior where he can get lost in Junction volumes.

o **NeverGiveUp ****=1 **: ****This parameter instructs the AI to cheat indefinitely. The AI will always know where the player is, and will never lose interest in chasing. This parameter was used for the nolf2 ninja trailer park level, where combat should never stop, but ninjas may get temporarily incapacitated when hit with SpecialDamage (e.g. poison).

### AttackFromCover

· Cover nodes have an optional parameter Object. This Object may be an object like a table that can be flipped on its side to use for cover. If an AINodeUseObject also points to this object, and the UseObject node has a SmartObject set to something for cover, the SmartObject can specify what animation the AI should use when activating the Object. For example, SmartObject FlipTable tells an AI how to animate when flipping a table to use as cover. After flipping the table, the AI runs to the Cover node, and runs the normal AttackFromCover behavior.

### AttackFromVantage

· AINodeVantage may specify an optional Object property. This object will be triggered On before the AI starts attacking from the Vantage node, and triggered Off after the AI attacks. This could be used to open and shut window shutters, for example. This Object was used in nolf2 to allow Pierre to open and shut steam hatches in the underwater base boss-fight.

· There are a couple Goals derived from AttackFromVantage that may be useful, or may be good templates for doing similar things.

o **AttackFromRandomVantage **works exactly like AttackFromVantage , but does not require any stimuli to activate the Goal, and chooses valid Vantage nodes at random instead of choosing the nearest valid node. This Goal is used in nolf2 for the Pierre boos-fight in the underwater base, where Pierre pops out of random steam hatches to throw knives at the player.

o **AttackFromRoofVantage **works exactly like AttackFromVantage , but uses AINodeVantageRoof attractor nodes instead of normal AINodeVantages . An AI may only activate AttackFromRoofVantage if no other AI already has the Goal active, and only if at least 3 AI are already attacking the same target. This Goal is used in nolf2 to control when ninja AI attack from rooftops.

### AttackFromView

· An AI will attack from an AINodeView if the AI has sensed stimuli of the enemy’s presence, and cannot find a path to the enemy, and the enemy is in an AIVolumePlayerInfo that points to a View node that the AI can find a path to.

· **SeekPlayer ****=1 **: This parameter instructs an AI to attack from a View node without first sensing any stimuli.

### Work

· AINodeUseObjects set to SmartObjects for work are used with the Work Goal. These nodes control most of nolf2’s relaxed behaviors (smoking, working at desks, dancing, leaning, etc).

· There are various Goals derived from Work that are nearly identical Goals, but are attracted to different types of Smart Objects:

o **Sleep **: AI sleeps in a bed or chair, or while standing. Visual senses are disabled while sleeping.

o **PlacePoster **and **EnjoyPoster **: Used in nolf2’s India levels for police placing posters and civilians looking at posters.

o **Destroy **: Used for nolf2’s SuperSoldiers to destroy things by smashing or shooting.

o **Ride **: Used for nolf2’s underwater base boss-fight to levitate Pierre on steam bursts.

· If a Work node has a ReactivationTime of zero, an AI may not use this node again until first using another Work node. Any ReactivationTime greater than zero will allow an AI to re-use the same node once the node is active again.

· An AI can be instructed to immediately stop working at a node by sending a Disable command to the node.

· If an AI is interrupted in the middle of working, he may optionally play an interrupt animation. For example, an AI that is smoking may abruptly throw down his cigarette instead of calmly putting it out. When the Work Goal deactivates, it will play an interrupt animation if it exists in animationsCharacterName.txt, otherwise the AI will play its normal out transition if it exists.

· If an AI is hit with SpecialDamage while working, he may optionally play a specific SpecialDamage animation rather than transition out of Work normally. For example, and AI may slump over and pass out at his desk, instead of standing up, pushing in his chair and then passing out. This animation actually plays in the SpecialDamage Goal.

· AI may play a specific death animation corresponding to his work. For example, slumping over a desk, or falling out of a chair.

### SpecialDamage

· The SpecialDamage Goal handles damage types that are specific to nolf2, but may act as a template for similar functionality in other titles. It handles both instant and progressive damage, and includes the following damage types:

o Sleeping

o Stun

o Bear_Trap

o Glue

o Laughing

o Slippery

o Bleeding

o Burn

o Choke

o ASSS (Anti SuperSoldier Serum)

o Poison

o Electrocute

· Incapacitating animations and transition animations are defined through SmartObjects . Note that this goal uses SmartObjects to drive animation without the presence of an AINodeUseObject .

· Instead of playing the normal in-transition animation, the Goal may play an animation specific to the AI’s current animation, as described previously in the Work Goal. This allows an AI to slump over on his desk when knocked-out. This also allows an AI to get knocked out while sleeping or prone without getting up first.

· AI may play a specific death animation corresponding to the damage. For example, dying instantly without moving while knocked out.

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: NOLF2_AI_Gameplay_Setup.md)2006, All Rights Reserved.
