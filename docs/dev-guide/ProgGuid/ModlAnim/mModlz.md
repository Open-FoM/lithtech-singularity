| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Models and Animation

Models are a collection of geometries and animations used to create complex objects such as characters and other props, such as consoles, weapons, monsters, and vehicles. Artists create models in a third-party editor such as Maya or 3D Studio Max. Using LithTech plugins, the models are saved in the .LTA file format. The .LTA file contains the geometry and animation information used by LithTech to manage and render the model. Artists can use LithTech’s ModelEdit tool to modify .LTA files. Models can be highly detailed and can move. For more information on creating models and ModelEdit, see the ModelEdit chapter.

Models are represented in game code by custom BaseClass-derived simulation objects of type OT_MODEL. When a model object is instantiated, the location of the model’s .LTB platform-dependent binary file is provided. LithTech uses the referenced .LTB file to render the model. The .LTB file is created from an .LTA file by a packer. The .LTB files are inaccessible in game code. All model functionality is exposed through the ILTModel interface.

Intrinsic to LithTech models are animations. Animations are a collection of time-to-transformation associations that modify the transform hierarchy of the model. The transform hierarchy is used to determine the positions.

Models move using animations. An animation is a series of movements that string together to make the model animate. An animation is created with the model in Max or Maya. When LithTech loads a model the default animation is automatically started. Animations can be looped. Animation trackers can be used to change a model’s animation.

Other model, sprite, and light objects can be connected to a model as attachments. Attachments are connected to models with sockets. Attachments can be used for such accessories as guns, knives, swords, bows, and even other less violent objects (such as clipboards and coffee cups).

This Section contains the following models and animations topics:

| #### Client Shell Topic | #### Description |
| --- | --- |
| [Instantiating a Model ](Instant.md) | Models are instantiated in game code using the **OT_MODEL **type in the **ObjectCreateStruct **structure passed in to the **ILTServer::CreateObject **function. |
| [Animations ](Animate.md) | Animations are a specific set of motions for a model. Each model can have any number of animations. |
| [Attachments and Sockets ](Attach.md) | Models, sprites, and lights can be attached to a model. Sockets define the connecting point between a model and an attachment. |
| [Transformations ](Transfrm.md) | Transformations define a position and rotation for attachments, sockets, and other items. |
| [Animation Code Example ](Example.md) | Describes how to set up a multi-tracked animation on an object. |



[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\ModlAnim\mModlz.md)2006, All Rights Reserved.
