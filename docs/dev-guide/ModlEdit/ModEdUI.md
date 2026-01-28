| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Understanding ModelEdit

The ModelEdit UI is made up of several windows, which are covered briefly in this section, and then described with greater detail in successive topics. The image below shows the ModelEdit UI. Move your mouse over the image to identify the individual ModelEdit interface components.

>

![](../Images/image052.gif)

- The **Model View **window previews the model mesh and its animations.
- The **Player **buttons control playback to preview the animation.
- The **Nodes **list selects the model's skeletal nodes.
- The **Pieces **tree lists all of the individual meshes that make up the current model.
- The **Animations **panel lists all animations that are loaded in the current model.
- The **Child Models **list contains all model files whose animations are referenced by this model. An animation that comes from a child model cannot be edited and will have an empty box next to its name in the Animations list. An animation that is part of the file you are currently editing will have a check mark beside its name and can be modified.
- The **Sockets list **contains the model's sockets.

Each of the ModelEdit controls and windows are discussed further in successive sections. The following lists some general uses of the controls:

- Use the **Level of Detai **l (LOD) slider to select the level of detail you want to view.
- Use the **Animation Edit **control to manipulate animation data.
- Use the **Transform Edit **control to manipulate socket or node positions.
- At the bottom of the display is the animation **Time **bar. The **Time **bar has a red mark for every key frame in the currently selected animation. Each animation or model has at least one key frame, but only those with more than one will show red marks. Frame Strings are a way to tie an event or message to a key frame so that it will be available to the game programmer. For example, if your character has a swiping claw attack, you can add a message to its swipe animation so that each time it swipes at something, a "whipping claw" sound plays.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ModlEdit\ModEdUI.md)2006, All Rights Reserved.
