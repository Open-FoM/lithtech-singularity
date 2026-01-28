| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# ModelEdit UI Reference

This section describes the following menu commands available on the ModelEdit interface:

- [File Menu ](#FileMenu)
- [Model Menu ](#ModelMenu)
- [Animation Menu ](#AnimationMenu)
- [Child Model Menu ](#ChildModelMenu)
- [Sockets Menu ](#SocketsMenu)
- [Piece Menu ](#PieceMenu)
- [Options Menu ](#OptionsMenu)
- [Model View Window ](#ModelViewMenu)
- [The Animation Bar ](#TheAnimationBar)
- [Animation Edit ](#AnimationEdit)
- [Transform Edit ](#TransformEdit)
- [Nodes List ](#NodesList)
- [Animations List ](#AnimationsList)
- [Command String ](#CommandString)

---

## File Menu

**Open **—Opens a model (.LTA).

**Import **—Reads data from a model (.LTA, .ltc) into the current open model, such as new animations.

**Save **—Saves the model.

**Save **As—Saves the model under a different filename.

**Exit **—Quits ModelEdit.

[Top ](#top)

---

## Model Menu

**Model Info **—Shows information about the model such as the amount of memory it uses and the number of triangles it contains.

**Command String **—Allows control over attributes of the model; See below for details.

**Build LOD **—Generates LOD information .

**Rename Node **—Renames a node.

**Generate Vertex Normals **—Recalculates the vertex normals based on neighboring polygons.

**Remove Node **—Deletes the selected node.

**Set Texture **—Changes the texture to be displayed on the model in the display window in ModelEdit. To display multiple textures, select a piece with the lowest texture index and click Set Texture to select its texture. Then select a piece with the next index and click Set texture to set its texture.

**Set Internal Radius **—Sets the radius of the visibility-culling sphere.

**Calc Internal Radius **—Automatically sets internal radius based on all the animations used by the model. Using this command instead of 'Set Internal Radius usually gives greater accuracy and is quicker.

**Edit Weights **—Dialog where you create weight sets on the model’s nodes for animation blending.

[Top ](#top)

---

## Animation Menu

**Rename **—Renames the selected animation.

**Duplicate **—Makes a copy of the selected animation.

**Dimensions **—Changes the user dimensions of the selected animation to determine its collision information.

**Reverse **—Reverses the time direction of the selected animation.

**Make Continuous **—Causes the selected animation to loop. This adds a length of time at the end of an animation so that it can interpolate back to the first keyframe.

**Set Animation Rate **—Changes the overall frame rate of the selected animation (or all animations) This does not directly set the animation's frame rate, but instead sets the number of stored keyframes per second. The engine then linearly interpolates to fill in the gaps.

**Set Animation Length **—Scales the length of the entire animation, making it play back faster or slower.

**Set Animation Interpolation **—Creates a dialog to set the interpolation time (in micro seconds) between two animations.

**Create Single Frame **—Creates a single-frame animation containing the current frame.

**Translation **—Adds a spatial translation from the model’s origin to the selected animation.

**Rotate **—Rotates the selected animation.

**Remove Extra Frames **—Opens a dialog that attempts to clean out any redundant frames in the current animation. Best used to reduce the amount of unneeded data in a motion-capture file, but can also find extra keyframes you may have added by mistake.

[Top ](#top)

---

## Child Model Menu

**Add Child Model **—Adds a new child model. Any .LTA file with an identical hierarchy to the current model can be used as its child model.

**Remove Child Model **—Removes the selected child model.

**Rebuild Tree **—Attempts to reconstruct and verify the current model’s childmodel references.

[Top ](#top)

---

## Sockets Menu

Sockets are transforms for attachments.

**Add Socket **—Adds a new socket.

**Remove Socket **—Removes the selected socket.

**Rename Socket **—Renames the selected socket.

[Top ](#top)

---

## Piece Menu

**Piece Info **—Changes material attributes of the piece. The texture index can be changed here, as well as the specular attributes of the piece. You can use the texture index here to modify the indices you set when you exported the model, or use this value instead of setting it when exported. The specular power represents the size of any reflected light on the surface of the model. The larger the power the larger the light reflection will be. The scale attributes represent how matte the surface is. With a value close 0.5 the surface will not reflect very well, but with a value of 1.0 the surface will look very smooth and shiny. ModelEdit will not show the specular highlight. The engine uses the value when running.

**Merge Pieces **—Select a group of pieces in the piece window and use this to combine them into one piece, based on texture map. If you select all the pieces, the result will be 1 piece for each texture map the model uses. Do not use this on models with LODs, since the LOD pieces will combine incorrectly. This is mostly very useful for prop objects which contain many pieces.

**Set Preview Texture **—Select a piece in the piece window and use this menu command to apply a texture to it. When you save the model, the preview texture information will be saved as well. Note that this texture information is used only by ModelEdit. You will still need to set the character’s/item’s texture as usual in its attribute file.

[Top ](#top)

---

## Options Menu

**Lights Follow Camera **—When toggled, the red/blue preview in ModelEdit lights move along with the camera.

**Wireframe **—Toggles wireframe rendering mode.

**Show User Dimensions **—Draws a box that shows the size of the selected animation's dimensions graphically. User dimensions define the bounding shape around models for physics.

**Show Internal Dimensions **—Draws a box that shows the size of the model's internal dimensions, which are used to determine the visibility clipping of a model. The internal dimensions should always *contain *all of the model geometry in any of its animations as *efficiently *as possible.

**Show Skeleton **—Draws the location of the nodes in the model.

**Show Sockets **—Draws the position and orientation of the sockets.

**Show Attachments **—Draws any attached models in relation to their socket (See Command String/SocketModel).

**Solid Attachments **—Toggles Solid/wireframe mode for any attached models.

**Show Normals **—Draws the vertex normals.

**Show NormalRef **—If supported by the model, displays the vertex normal frame of reference.

**Profile **—Shows performance information in the draw window. Note: this information doesn’t determine the final performance of the model in your game.

**New Background Color **—Change the color of the model view window.

**Change Field of View **—Change the camera angle in the model view window.

[Top ](#top)

---

## Model View Window

Mouse motion changes the camera position.

Left mouse rotates the camera around the model.

Right mouse dollies the camera in and out.

Right and left mouse combined pans the camera.

Shift left mouse restores the view.

[Top ](#top)

---

## The Animation Bar

Click to move to the nearest key frame, represented by red lines. Double-click to tag a key frame. Double-click on the frame again to un-tag it. Right-click on a frame to see a context menu for the frame.

**Frame Time **—Allows skipping to a given time in the animation.

**Frame String **—Assigns a control string to the specified time in the animation.

**LOD slider **—Changes the displayed LOD of the model.

[Top ](#top)

---

## Animation Edit

**User Dimensions radio button **—When this button is selected, the +/- buttons below it alter the size of the current animation's dimensions.

**Translation radio button **—When selected, the +/- buttons change the translation of the current animation

**X **—Deletes the current animation.

**# **—Moves the animation to the specified location in the model’s list of animations. Game code often refers to model animations by index, so their order may be important.

**Up arrow **—Moves the animation up in the model’s animation list.

**Down arrow **—Moves the animation down in the list.

[Top ](#top)

---

## Transform Edit

**Global **—Toggles whether adjustments are node-relative or global.

**Socket **—Specifies that the Rotation and Translation color boxes will affect the model’s sockets.

**Relation **—Specifies that nodes will be adjusted.

**Rotation **—These buttons each represent an axis of the currently selected socket or node (Red = X, Green = Y, Blue = Z). Click on the color of the axis you want to change and drag left/right to rotate around that axis.

**Translation **—Click on the color and drag left/right to move along that axis.

[Top ](#top)

---

## Nodes List

If a node name has a check in its check box, the node will be placed relative to its parent and only rotation information from the animation data will be used. If you don’t check off a node here, its bone length will be transferred from the child model to the parent along with the rotational data, causing potentially strange-looking scaling of a character.

[Top ](#top)

---

## Animations List

A checkbox here indicates that an animation is from the current model and may therefore be changed/moved/etc.

[Top ](#top)

---

## Command String

The command string is a semicolon-delimited string used to define extra control settings for a model in ModelEdit, the engine, and in the final game. Most commands are game specific, but some Command String commands are used by ModelEdit.

A model containing SocketModel commands will store its attachment model filenames in your machine’s Windows registry so that you no longer need to re-specify an attachment each time you load the file in ModelEdit to preview it.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ModlEdit\UIRef.md)2006, All Rights Reserved.
