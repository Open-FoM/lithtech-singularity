| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Editing Animations

Below the Model View panel, there are CD player-style buttons that control animation playback.

Below that is the animation bar. Red vertical lines are key frames.

The four buttons in the Animation Edit area manipulate the ordering of the animations in the Animations list box. Clicking the button with a red X on it deletes the current animation from the model. The # button renumbers the currently selected animation. The arrows move the animation up and down in the list box.

The radio buttons labelled User Dimensions and Translation control the six thumbwheel buttons below them. Depending which is selected, you can adjust either the size of the bounding box of the animation or the base position of the animation in space.

| **Note: ** | To see the User Dimensions, in the Options menu select Show User Dimensions. In Jupiter, you cannot change the User Dimensions for child model animations without editing them directly in the child model. |
| --- | --- |

>


![](../Images/image060.jpg)

In the Frame String text box, you can associate a text string with each key frame in an animation. This association is used as a message to indicate to the game code that a certain event has happened. This is most often used to associate a sound with an event. For example, you could associate footsteps with a "foot down" frame, gunshots with a model's "fire" frame, or a body hitting the floor with the last frames of a death animation.

#### To add a Frame String

1. Select a key frame by clicking on it
2. Type a word in the Frame String text field.

When the animation runs, the frame string is sent to the game code as the tagged key frame passes.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ModlEdit\EditAnim.md)2006, All Rights Reserved.
