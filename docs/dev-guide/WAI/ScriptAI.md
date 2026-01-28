| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Scripting AI Goals

Scripting was avoided as much as possible in NOLF2, but there were situations where some amount of scripting was necessary to drive the story or vary the gameplay for specific levels. The following list describes a few methods for accomplishing scripted behaviors without breaking the autonomy of the AI.

- **Enable/Disable Node Commands— **The majority of scripting in NOLF2 involved enabling and disabling nodes, or assigning new nodes with command, such as: goal_guard node=AINodeGuard45. This gives control over where AI are idling while still allowing some randomness and allows them to react normally to whatever situation they find themselves in.
- **GoalScripts— **GoalScripts supply an AI with a sequence of Goals to complete. When one Goal is satisfied (e.g. the AI has arrived at the destination node of a Goto Goal), the next Goal activates. GoalScripts may be interrupted if the AI notices a disturbance, and the GoalScript will continue once the AI returns to a relaxed state of awareness. GoalScripts are useful for making an AI go fro place-to-place and animate. You can find examples of existing GoalScripts in the Attributes\aigoals.txt file.
- **Search— **Normally AI search after activating other Goals (e.g. after investigating or chasing and losing the target). In cases where AI are spawned with the intention of immediately searching, they may be given the Search Goal, which is a scripted Search. AI with the Search Goal can be given a list of comma-separated regions to search: addgoal search region=airegion01,airegion02,airegion03. AI who search through normal means will only search their current region.
- **Patrol— **Another way to script searching behavior is to spawn AI and assign them patrol routes. The Patrol Goal may be instructed to play investigative walking animations: addgoal patrol awareness=investigate. In some nolf2 levels, we periodically spawn a patrolling AI to ensure the level will never be completely empty (and boring to the player), even when all AI have been killed leaving no one to sound alarms. An AI may be spawned, instructed to patrol, and then removed through a command on a destination patrol node.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: WAI\ScriptAI.md)2006, All Rights Reserved.
