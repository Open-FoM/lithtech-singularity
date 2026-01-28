| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Animations

An animation is a specific set of motions for a model. A model can have any number of animations associated with it. Artists create animations in a third-party editor such as Maya or 3D Studio Max at the same time that they create the associated model. Using LithTech plugins, the model and accompanying animations are saved together in the same .LTA file format. Artists can use LithTech’s ModelEdit tool to modify .LTA files.

LithTech handles memory management of animation trackers. Therefore, game code does not need to access the animation tracker instances. Instead, animation trackers are referenced and manipulated using the **ANIMTRACKERID **type definition.

Animations for models can be stopped, played, looped, changed or blended. To stop or play an animation, use the **ILTModel::SetPlaying **function. To loop, or unloop, an animation, use the **ILTModel::SetLooping **function. To change a model’s animation, use the **ILTModel::RemoveTracker **, **ILTModel::AddTracker **and **ILTModel::SetCurAnim **functions. To blend two or more animations on the same model at the same time, use the **AddTracker **and **SetCurAnim **functions, as well as the **ILTModel::FindWeightSet **, **GetWeightSet **, and **SetWeightSet **functions. See the Animation Trackers section below for more information.

An animation causes the model to engage in some motion without changing the model’s position. For instance, a model’s walk animation causes the legs of the model to move as if walking. However, unless the model object’s position is changed at the same time the animation results in a moonwalk effect.

This section contains the following topics explaining animation code:

- [About the Default Animation ](#DefaultAnimation)
- [Using Animation Trackers ](#AnimationTrackers)
- [Adding Animation Tracker After Object Creation ](#AddingAnimationTrackerAfterObjectCreation)
- [Removing Animation Tracker ](#RemovingAnimationTracker)
- [Associating Animation With Animation Tracker ](#AssociatingAnimationWithAnimationTracker)
- [Blending Animations ](#BlendingAnimations)

---

## About the Default Animation

The default animation is always the first animation. The first animation can be seen in ModelEdit as the first animation in the animation list box. The default animation is automatically started when LithTech loads the model. Access to the default animation can be acquired using the **ILTModel::GetTracker **function.

To change the animation from the default, use the **ILTModel::SetCurAnim **function.

[Top ](#top)

---

## Using Animation Trackers

An animation tracker identifies specific animations for a model. The animations for a model can be accessed using animation trackers. Models by default start with one animation tracker. This tracker points to the first animation found in the model database. The animation tracker keeps track of the current time, and the animation rate for the animation it holds. LithTech manages the creation, network distribution, and deletion of all animation trackers. Game code uses **ANIMTRACKERID **typedefs to identify animation. A model must have at least one tracker.

A model can have any number of animation trackers. Each animation tracker represents a specific animation, and each can be played consecutively or concurrently on the model. Concurrent animation is called blending. Each animation tracker added to the model blends with the base animation tracker. To blend animations together properly an animation blend weight set has to be associated with each animation tracker added after the first.

To change which animation a tracker is pointing to use **ILTModel::SetCurAnim **. This will effectively change the current animation a model is playing.

Enabling animation trackers after object creation is a two-step process. First, an **ANIMTRACKERID **is associated with a model using the **ILTModel::AddTracker **function. Then, the **ANIMTRACKERID **is associated with a specific animation using the **ILTModel::SetCurAnim **function.

[Top ](#top)

---

## Adding Animation Tracker After Object Creation

To add animation trackers after the model object has been created, use the **ILTModel::AddTracker **function. This function associates an aninmation tracker with a specific model.

This function creates an animation tracker for the **OT_MODEL **. The **ANIMTRACKERID **is the reference index to the tracker for that model. Different models can have the same tracker identifier or reference index.

>
```
LTRESULT AddTracker(
```

```
       HOBJECT hModel,
```

```
       ANIMTRACKERID TrackerID);
```

Once a tracker is created an animation can be assigned to it with the **ILTModel::SetCurAnim **function. The **AddTracker **function does not actually associate an animation with a model. It merely provides the model with an identifier to an animation tracker that can be associated with an animation using the **ILTModel::SetCurAnim **.

If the animation for a given animation tracker is changed using the **SetCurAnim **function, the previous animation will continue playing until finished before the new animation begins.

Animation tracker identifiers are composed of a value for a given model. Different models may use the same value for the animation tracker identifier without conflict. The animations for these trackers need not be identical.

**ANIMTRACKERID0xff **(the maximum value of an 8-bit unsigned integer) is reserved for the main tracker, which is always created for all models. You can refer to this resolved value with the name **MAIN_TRACKER **.

[Top ](#top)

---

## Removing Animation Tracker

You can remove an animation tracker from a model using the **ILTModel::RemoveTracker **function.

>
```
LTRESULT RemoveTracker(
```

```
       HOBJECT hModel,
```

```
       ANIMTRACKERID TrackerID);
```

The **TrackerID **parameter identifies the animation tracker to be removed from the model identified by the **hModel **parameter.

[Top ](#top)

---

## Associating Animation With Animation Tracker

After an animation tracker has been associated with a model, the animation tracker still needs to be associated with an animation using the **ILTModel::SetCurAnim **function.

>
```
LTRESULT SetCurAnim(
```

```
       HOBJECT hModel,
```

```
       ANIMTRACKERID TrackerID,
```

```
       HMODELANIM hAnim);
```

The **SetCurAnim **function associates the animation trackers identified by the **TrackerID **parameter with the animation identified by the **hAnim **parameter for the model identified by the **hModel **parameter.

The **hModel **parameter is required because the animation tracker identifier exists in the model namespace. That is, any given value for an animation tracker identifier can exist for multiple models but identify different animations.

The play state of the new animation is set to the play state of the previous animation. If there was not previous animation, the **play **state is set to **play **.

[Top ](#top)

---

## Blending Animations

Multiple animations can be played on the same model at the same time. This is useful, for instance, to have a walking character raise a gun and shoot it. In this case, both a walk animation and a shoot animation would play at the same time.

A weightset is a collection of values used to interpolate from one animation to another. Each weightset contains a number of (possibly) disparate values that apply to each node in a model. Weightsets are created in ModelEdit and are represented in code by the **HMODELWEIGHTSET **type definition.

To set the weightset for a given animation, use the **ILTModel::SetWeightSet **function.

>
```
LTRESULT SetWeightSet(
```

```
       HOBJECT hModel,
```

```
       ANIMTRACKERID TrackerID,
```

```
       HMODELWEIGHTSET hSet);
```

The **hSet **parameter defines the weightset for the animation identified by the **TrackerID **parameter for the model identified by the **hModel **parameter. If the **hSet **parameter is invalid, the model disappears in-game.

Game code should keep track of the various blending possibilities and appropriately reset the animation blending weightsets for each.

The order in which animations are blended on a model is significant. If three animations are applied to a model, the first two animations are blended using the second animation’s weightset, and the result is blended with the third animation using its weightset. This process extrapolates for any number of weightsets.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\ModlAnim\Animate.md)2006, All Rights Reserved.
