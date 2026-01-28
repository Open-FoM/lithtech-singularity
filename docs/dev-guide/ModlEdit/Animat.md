| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Adding Animations

Animations are part of the model's database. Animations can be added directly to a model’s own data, or they can be added by reference.

| **Note: ** | All animations that you import or add as child models must have the same hierarchy as the model being worked on. ModelEdit will warn you when there is a hierarchy mismatch. |
| --- | --- |

## Adding an Animation Directly

#### To add an animation directly

1. In the File menu and select Import. A file dialog appears with check boxes at the bottom listing the import options.
2. Make sure the Animations box is checked.
3. Browse to the animation file you wish to add to the model and select it.
4. Click Open.

The new animation’s name appears in the Animations list box when loading is done.

Adding animations directly to a model is useful if you know that no other models will need the animation. For example, an octopus's animations are probably only needed by the octopus model.

## Adding an Animation by Reference

A referenced animation's data is not included in the model file. Instead, the model file refers to another file that contains the actual animation data. This is useful when several models are going to share the same set of animations, because once the referenced model, the child model, is loaded into memory, all models that share animations from that model refer to the copy that’s already loaded. This saves significant memory in your game when it’s running.

#### To add an animation by reference

1. In the Child Model menu select Add Child Model. A browsing dialog appears.
2. Browse for and select the model or models you would like to load as the animation.
3. Click OK.
4. ModelEdit checks the .LTA file to see if the two models' hierarchies are the same. If they are, the animations will now be available to your current model.

When the model is saved, a reference to the child file is stored in the model data.

## About Child Models

The term "Child Model" is used to describe animations that are shared across multiple models. A child model is a reference to another data file that contains animations to be used. The only constraint to this construct is that the child model's skeleton has to be organized exactly like the skeleton of the currently loaded model. ModelEdit will signify that an error has occurred if the child model skeleton is not the same as the base model.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ModlEdit\Animat.md)2006, All Rights Reserved.
