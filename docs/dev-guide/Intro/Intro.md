| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Introduction to the Jupiter System

Powering No One Lives Forever™ 2, TRON® 2.0 and numerous other games produced by developers in North America, Asia, and Europe, Jupiter is a comprehensive set of tools and technologies that gives developers wanting to push the envelope the power to create high performance, immersive games for the PC and Xbox.

Jupiter handles model animation and physics, as well as rendering, special effects, networking, input management, audio, music, and so on. Jupiter is unique in that the game code and engine code are completely separate entities. This is what gives Jupiter much of its flexibility.

Jupiter includes the following features and advantages:

- Complete game code for No One Lives Forever™ 2.
- A new world renderer, which draws 3D graphics in larger chunks, enabling the system to work faster and more efficiently. With the new renderer, developers using Jupiter may create detailed, high poly environments.
- Enables developers to create large outdoor environments. It also features gouraud shading, which gives developers the ability to create shadows instead of using lightmaps, which will render worlds faster and more efficiently.

## Development Tasks

When developing a 3D game application there are many tasks that must be completed. Planning, scheduling, and designing are well beyond the scope of this document. For games using Jupiter, the tasks can be assembled into the following groups:

- Design levels : Use the DEdit tool to design levels.
- Create models : Use Max or Maya along with the ModelEdit tool to create models.
- Create sound and music : Use a sound creation tool of your choice and the DirectMusic tools to create sound and music files.
- Create textures : Use a tool that exports TGA files, and convert the textures to .DTX files in DEdit.
- Write engine code : Jupiter provides full engine functionality.
- Write and/or modify game code : The task for your engineers.

The first four tasks are the responsibility of artists and level designers. The tools associated with them are described in the *Using the Tools *section of this document.

The fifth task, writing engine code, has already been done. Jupiter saves the game engineer the time and effort required to write the engine code upon which your game is based. (However, you may modify the engine code if desired.)

The last task, writing game code, is what remains for game programmers. Using Jupiter game code to provide basic functionality, you must write game code that handles the specific operations required by your game. See the *Programming *section of this document.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Intro\Intro.md)2006, All Rights Reserved.
