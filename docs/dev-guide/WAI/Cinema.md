| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Creating Cinematic AIs

The Talk Goal allows AI to coordinate meeting up, having a conversation, and going on to do other things. The Talk Goal also handles responding to interruptions, and possibly returning to have the conversation later. There are basically two options for setting up AI to have a conversation: statically positioned, and dynamically positioned.

### Statically Positioned Conversation Participants

- AI are placed at AINodeTalk nodes and given initial commands of: addgoal talk node=AINodeTalk04. AI will walk to Talk nodes and wait for a dialogue object to be triggered.
- If both AI are within the radii of their Talk nodes when the corresponding dialogue object is triggered, they will have their conversation.
- When the conversation ends, the dialogue object will run its finished and cleanup commands.
- An AI with a Talk Goal and Talk node will ignore his Guard and Patrol Goals. The Talk Goal overrides other relaxed goals, so the AI may still have a full GoalSet, such as Patrol or Guard.
- If an AI is at a Talk node, and notices a disturbance, he will investigate. Guard node radii will be ignored.
- If the dialogue object is triggered while one or both of the dialogue participants are not relaxed, the dialogue will not play, and the dialogue object’s cleanup command will be run.
- If the dialogue object was not triggered while the AI was investigating, when the AI returns to a relaxed state of awareness, he returns to his Talk node and continues waiting for the dialogue to be triggered.
- The Talk Goal parameter disposable=1 tells the AI not to return to his Talk node if he was disturbed while waiting for the conversation. In this case, the AI will simply return to his normal relaxed behaviors and forget about having the dialogue.

### Dynamically Positioned Conversation Participants

- AI may be anywhere in the level, running various Goals. They do not need to be near their Talk nodes.
- When a dialogue object is triggered, commands are sent to the dialogue participants: addgoal talk node=AINodeTalk09 retrigger=1. If either AI are outside of their Talk node radii, the dialogue will not start. The parameter retrigger=1 tells the AI to retrigger the dialogue object once the AI arrive at their Talk nodes.
- Once both AI are in position, the conversation starts.
- When the conversation ends, the dialogue object will run its finished and cleanup commands.
- The above steps may be used to get AI to walk towards each other and start talking, or to make an AI stop working and start talking to someone.

### General Notes about the Talk Goal

- While the Talk Goal is active, AI face each other and talk while playing their basic idle animation (usually 3StGd).
- AI may be instructed to play gesture animations at key points in the conversation. From the dialogue object, send a command to the AI: goal_talk gesture=NameOfAnim.
- If a dialogue object is triggered, and one or more of the dialogue participants does not exist (probably because he is dead), the dialogue will not play and the dialogue object’s cleanup command will run.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: WAI\Cinema.md)2006, All Rights Reserved.
