| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Animation Blending and Weighting

This section discusses some specific details about animation, and provides instructions on blending, weighting, and interpolating your animations.

## Animation Blending

The Jupiter animation system supports two kinds of animation blending operations, consecutive and concurrent. The engine automatically blends between all consecutive animations. Additionally, you can create a custom blend between two concurrent animations using Animation Blending Weights.

>

**Consecutive Animation Blending **—Jupiter blends consecutive animations together by default. At the end of each animation the engine interpolates to the next animation in 200 milliseconds. This value can be changed per animation in ModelEdit.

**Concurrent Weighted Animation Blending **—Use ModelEdit to blend from two to four animations concurrently by specifying Weighted Animation Blending values for nodes in the animations. When the game programmer sets up concurrent animations for the models in the game code (using ILTModel::AddTracker), the Weighted Animation Blending can be swapped dynamically to produce new custom animations from existing animations. Be sure there is an Anim Blending Weight set associated with the second animation added in this fashion.

---

## Animation Weight Sets

All nodes belong to the weight sets, and for each weight set the nodes can be weighted differently. For example, an upperbody weight set might have all the nodes belonging to the hips and above set to have a weight of one, while the nodes in a lowerbody weight set would have all the node's weights below the hips set to one and all the nodes above the hip node set to zero.

#### To Create a Weight

1. In the Model menu, select the Edit Weights option.
2. In the Weight Set dialog box, click Add Set.
3. In the New Weight Set dialog box, type a name for the new weight set you are creating and then click OK.
4. In the Weight Set dialog box, select the new weight set you created.
5. In the Nodes list, select one or more nodes and then enter a new value in the Current Node Weight text box.
6. When you finish assigning weights to the nodes for the weight set, click Close.

#### To Create a Weighted Animation Blending

1. In the Animations list, click to select the first animation. Notice that the selected animation appears in the Animation Keyframes area.
2. Press and hold CTRL while you select a second animation. The second animation appears in the Animation Keyframes area, below the first animation.
3. In the Animation Keyframes area, click the button that displays the name of the first animation.
4. In the Select Animation Blending Weight Set dialog box, click the name of the weight set you created and then click OK.
5. Repeat steps 3 and 4 for each animation.
6. Click the Play button to see the results of your Weighted Animation Blending.

[Top ](#top)

---

## Weight Values

The value of weight can be any numerical value. Typically, you will use values between 0 and 1 since 0 means no change and 1 gives full value to the second node.

If the weight set value is 2, the algorithm will simply add the two animations together. This can also be used to great effect. Generally the added animation should be designed intentionally to modify the first. A walk and a run added together would result in a very strange animation.

[Top ](#top)

---

## Linear Interpolation

The order in which the animations are selected makes a difference. The animations linearly interpolate (LERP) per frame from the first to the next using the node's weight value. The formula of the resulting animation equals the first animation plus the difference between the first and the second animation times the second animation's weight value.

>
```
RES = FIRST + ((SECOND - FIRST) * SECOND's WEIGHT)
```

Be aware that when you blend more than two animations, this algorithm is recursive: the result from the previous evaluation becomes the FIRST value, and the SECOND parameter is the next animation in the list that has yet to be evaluated. For example,

>
```
LERP (LERP (first, second, second-weight), third, third-weight)
```

Because of the nature of the algorithm, getting things to look the way you want could be tricky, so as you experiment with various animations, try to make concurrent animations as similar to each other as possible. For example if you want to accentuate a walk, make sure the foot plants occur on the same key frame. If you are doing upper-lower body animations, have the weights at the fusion points ramp from one animation weight set to the other. With enough experimentation, you will develop good intuition as to what works.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ModlEdit\AniBlend.md)2006, All Rights Reserved.
