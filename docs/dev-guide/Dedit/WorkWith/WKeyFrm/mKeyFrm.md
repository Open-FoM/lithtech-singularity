| ### Content Guide | [ ![](../../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Working With KeyFrames

When considering how to animate WorldModels in DEdit, many Level Designers choose to use KeyFrames to create paths for their object. KeyFrames allow you to lock world geometry bound to a WorldModel onto a path. When activated, the WorldModel and all its bound objects move at a set speed from key to key along the path. You can send commands triggered by the arrival of the object from each key point along the path, and you have control over many properties of the animated object. KeyFrames can control more than just WorldModels. Cameras, explosion debris, special effects, and living props are all examples of objects that often get KeyFramed in a level.

This section contains the following topics explaining how to insert and use KeyFrame objects:

- [General Steps to KeyFraming ](#GeneralStepstoKeyFraming)
- [KeyFraming a WorldModel ](#KeyFramingaWorldModel)
- [Modifying a Key Path ](#ModifyingaKeyPath)
- [Orienting a WorldModel ](#OrientingaWorldModel)
- [Using KeyFramer Commands ](#UsingKeyFramerCommands)
- [Troubleshooting KeyFrame Animations ](#TroubleshootingKeyFrames)

---

## General Steps to KeyFraming

This topic lists the general steps required to KeyFrame an object. The following steps are covered in more detail in successive topics. If you are familiar with KeyFraming, then you can use this list to quickly start keyframing an object.

You must perform the following steps to animate an object using KeyFrames:

- Bind the Geometry to a WorldModel object.
- Insert a KeyFramer object.
- Set the KeyFramer ObjectName property to the name of the WorldModel object to animate.
- Insert the Key objects that define the path the WorldModel moves along during the animation.
- Ensure all Key objects are numbered sequential order along the KeyFrame path. (Key0, Key01, Key02, etc).
- Set the KeyFramer BaseKeyName property to the name of the first Key, truncating the number. (For example, if the first key is Key00, then you must use Key, without the number, as the BaseKeyName).
- Bind all Key objects to the KeyFramer object, and set the KeyFramer object to Path.
- Adjust the Bezier curve of each Key, and set the TimeStamp for each key.

For more specific information on each of these steps, see the topics below.

[Top ](#top)

---

## KeyFraming a WorldModel

This topic provides procedures for KeyFraming a simple WorldModel object. This topic assumes that you have already created the world geometry you want to animate.

KeyFraming involves three primary object types. A WorldModel (or other game object) to bind the geometry to. A KeyFramer object to control the animation. And a series of Key objects to move the animation between.

#### To KeyFrame a WorldModel

1. Bind the world geometry to a WorldModel object.

  1. Select the world geometry.
  2. Right-click the world geometry, move to Selection, and then click Bind to Object.
  3. Select WorldModel, and then click OK.
  4. In the Properties tab for the WorldModel object, set the IsKeyFramed property to TRUE.

2. Insert the KeyFramer and Key objects.

  1. Use the X key to position the marker near the WorldModel.
  2. Press CTRL + K, select KeyFramer, and then click OK. The KeyFramer object appears.
  3. Use the X key to position the marker where you want the animation occur.
  4. Press CTRL + K, select Key, and then click OK. A single Key object appears.

>

**Note: **In the Properties tab for the first Key object you insert, renumber the object's name to zero. For example, if the Key object's name is Key1, name the object Key0. If the Key object's name is MyKey1, rename the object MyKey0. The first Key in a KeyFramed animation must start with zero, and all successive keys must follow the numbering linearly. For example: Key0, Key01, Key02, etc. The KeyFramed animation moves between each of the keys in numeric order.

  5. Each Key object represents a point in the animation path. Repeat steps C and D until you have placed all of the Key objects you want for your animation.

3. Set the KeyFramer properties.

  1. Select the KeyFramer object, and then click the Properties tab.
  2. In the ObjectName property, type the name of the WorldModel from step one.
  3. In the BaseKeyName property, type the base name of the Key objects.
For example, if Key0 is the first Key object, then Key is the base name.
  4. Set the StartActive and Looping properties to TRUE.
  5. In the Properties tab for each Key object, set the TimeStamp to specify the number of seconds it takes to get from the previous node to the selected node.

1. Bind the Key objects and set them to create a path.

  1. Select all of the Key objects used in the animation.
  2. In Nodes view, right-click the KeyFramer object, and then click Move Tagged Nodes Here.
  3. In Nodes view, right-click the KeyFramer object, and then click Set Path.

This concludes the creation of your KeyFramed animation. Save and process your level. You should see your WorldModel moving between the Key objects. To modify the Bezier curves between the Keys, making them curved, read the next topic in this section.

[Top ](#top)

---

## Modifying a Key Path

Each Key object has two possible Bezier curve points. One point controls the curve before running into the Key object, and one point controls the curve running away from the Key object. Each point is represented by an arrow. The first and last Key objects in the path only have one point. Use the following procedure to alter the Bezier curves between each of the Key objects in your path.

#### To modify a Key path

1. Select a Key object.
2. Hold the P key, and then click and drag the arrow from the center of the Key object.
3. Repeat step two to move the other arrow (as needed) away from the Key object.
4. Continue holding the P key, and drag the arrows until the curve appears in the correct shape.
5. Repeat this procedure for each of the Key objects in the animation.

[Top ](#top)

---

## Orienting a WorldModel

There are three primary ways of orienting the direction a KeyFramed WorldModel faces.

1. Target an object to face using the TargetName property. The forward facing vector of the WorldModel always attempts to point at the specified target.
2. Offset the forward vector of the WorldModel targeting an object by setting the TargetOffset property. The forward facing vector of the WorldModel always attempts to point at the specified target but is offset from the target by the specified vector.
3. Face the forward vector of the path by setting AlignToPath to TRUE. The forward facing vector of the WorldModel always attempts to align itself (become parallel) to the Bezier curve between each Key object.

[Top ](#top)

---

## Using KeyFramer Commands

You can send the following command messages to a KeyFramer object from any object capable of sending commands.

**ON— **Activates the KeyFramer and starts moving the WorldModel.

**OFF— **Stops the WorldModel at its current position.

**PAUSE— **Causes the WorldModel to pause at its current position.

**RESUME— **Causes the WorldModel to resume movement after pausing or stopping.

**FORWARD— **Causes the WorldModel to move forward.

**BACKWARD— **Causes the WorldModel to move in the reverse direction.

**REVERSE— **Causes the WorldModel to move in the reverse direction.

**TOGGLEDIR— **Reverses the current direction of the WorldModel.

**GOTO <key>— **Specifies a Key object in the KeyFrame to jump the WorldModel to. The WorldModel jumps immediately to the specified Key object and stops.

**MOVETO <key> [destination cmd]— **Specifies a Key object in the KeyFrame to move the WorldModel to. Also allows you to specify a command to send the KeyFramer when the WorldModel reaches the Key object. The WorldModel moves from its current position to the specified key, and then stops or follows the optional command. For example: msg KeyFramer0 (MOVETO Key01 (msg KeyFramer0 RESUME)) causes the WorldModel to move to Key01 and then resume its normal course.

**TARGET <object>— **Specifies a new TargetName object.

**CLEARTARGET— **Removes the TargetName object from the TargetName property.

**TARGETOFFSET <x> <y> <z>— **Sets the offset when using TargetName to control the forward facing vector of the KeyFramer.

**VISIBLE <bool>— **Sets the visibility of the WorldModel.

**SOLID <bool>— **Sets the solid/intangible state of the WorldModel.

**HIDDEN <bool>— **Sets the visibility of the WorldModel.

**REMOVE— **Removes the KeyFrame object from the level.

[Top ](#top)

---

## Troubleshooting KeyFrame Animations

The following information contains common questions and answers regarding the use of the KeyFramer object. Use these questions to help troubleshoot your animation if it is not working correctly.

#### The WorldModel is not moving.

- Ensure the ObjectNam e property in the KeyFramer object specifies the correct WorldModel object.
- Ensure the KeyFramer object's BaseKeyName property is correct and does not contain a number.
- Ensure the TimeStamp in each Key object has a value.
- Ensure all Key objects contain the same base name, and that the numbers proceed linearly. For example, Key0, Key01, Key02, etc. The first Key object MUST start with zero. All successive Key objects must be numbered 0X where X is the next number in a linear numeric set.
- Ensure the IsKeyFrame d property in the WorldModel object is set to TRUE.

#### The Animation is too fast or too slow between some Key objects.

- Adjust the TimeStamp property of the Key objects. The TimeStamp property specifies the amount of time in seconds the WorldModel takes to move from the previous Key object to the currently selected Key object.

#### The WorldModel jumps when the animation starts

- Try moving your WorldModel object closer to the first Key object, or move the first Key object closer to the WorldModel.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Dedit\WorkWith\WKeyFrm\mKeyFrm.md)2006, All Rights Reserved.
