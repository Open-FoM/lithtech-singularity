| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Working With AI

AI (Artificial Intelligence) refers specifically to any characters in your game that are not controlled by a human player. All characters not controlled by the player are controlled by the computer, and by design they must know how to interact with players and perform actions of their own. These actions include, but are not limited to: standing still, attacking, patrolling, talking, searching, eating, responding to alarms, smoking, and working. AIs must know when to perform these actions, how to perform these actions, and when they cannot peform these actions.

While the Jupiter engine code itself does contain any AI specific code, the NOLF2 game code supplied with the Jupiter engine contains a large number of classes and objects specifically for AI. This section focuses on how AI works using NOLF2 game code. Specifically this section explains how Level Developers can implement NOLF 2 AI in their own levels using the NOLF2 game code. The purpose of this section is to serve as a reference. Developers of unique games should create their own AI classes and objects, and use the NOLF2 AI game code only as a reference.

Regardless of how you choose to implement your own AI, making the tools necessary for level designers to implement and create AI interaction through DEdit creates a quick and easy way for your Level Designers to create levels (worlds) that are interactive and interesting to the player. If you choose to implement AI strictly through a programming interface (API), then it becomes far more difficult for your level designers to see and test how their AI characters operate and interact with the players in the game. Because of this, you are advised to make your AI classes and objects available to Level Designers through the DEdit application.

This section contains the following topics specific to implementing NOLF 2 AI through DEdit:

| [About the NOLF2 AI ](#AboutAI) | Describes the fundamentals of how the AI system in NOLF2 operates, the base classes used, and the types of objects applied through DEdit. |
| --- | --- |
| [Using Regions and Volumes ](Pathing.md) | Describes how to specify where an AI can see and travel. |
| [Using AI Brains and Goals ](Goals.md) | Describes how to specify actions for an AI under different conditions. |
| [Using AI Senses ](Senses.md) | Describes how to use and modify the senses AIs use to precieve their environment. |
| [Using AI Attributes ](Attribs.md) | Describes how to use and modify the general attributes of AIs such as hit points, armor, jump speed, and others. |
| [Scripting AI Goals ](ScriptAI.md) | Describes how to create goal scripts to specify multiple actions for an AI. |
| [Using AI Commands ](Comnds.md) | Describes the commands available to send as player messages. |
| [Debugging AIs ](Debugin.md) | Provides instuctions for debugging AIs. |
| [Creating Cinematic AIs ](Cinema.md) | Describes general rules for creating AIs that talk in cut-scenes. |
| [AI Tutorial ](AItutor.md) | Provides a walk-through set of steps describing how to create a simple AI interaction in a level. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: WAI\mAI.md)2006, All Rights Reserved.
