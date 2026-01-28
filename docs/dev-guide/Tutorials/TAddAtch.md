| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# —TUTORIAL—
Adding an Attachment to Your Model

This tutorial provides step-by-step instructions on how to add an attachment to your model in Jupiter. Attachments are models used to add detail or dynamic parts to another model that would otherwise have to be built into the other model permanently. For example, you can use these steps to add a gun to a thug model, or to put a rider on the back of a horse. Attachments can be switched, removed or moved in your game code, so you could also attach things like helmets, shoes or tools to a character using attachments.

The following general steps (described in detail below) explain how to add an attachment to your model:

1. [Create the Attachment Model ](#StepOne)
2. [Set the Socket for the Attachment ](#StepTwo)
3. [Update Attachments.txt ](#StepThree)
4. [Assign and View the Attachment in DEdit ](#StepFour)

---

## Step One—Create the Attachment Model

In your 3D package (such as Maya or Max), perform the following steps:

1. Create and texture your mesh.
2. Create a skeleton. This skeleton must contain at least one bone to which you will bind your mesh.
3. Skin your model mesh onto its skeleton as explained under Exporters.
4. If you plan to give the attached model animations that are independent of the base model (moving sub-parts, and so on), create those animations.
5. Export your model using the Jupiter Model exporter. For convenience, you should export the .LTA to the location in your project where you plan to keep your game's attachment models.

In ModelEdit, perform the following steps:

1. Open the exported model, which must be in .LTA format.
2. Check to make sure that the model appears correctly.
3. Perform any steps you would normally do for a model in ModelEdit: generating LODs, adding childmodels, setting internal dimensions, and so on.
4. Save the model.
5. Pack the model to .LTB format.

---

## Step Two—Set the Socket for the Attachment

In ModelEdit

1. In your game project, open the model \Chars\Models\player_action.ltc. Use this model as a base to attach your new attachment model.
2. Look at the Sockets window.
3. If there is already a socket at the location on the model where you want your attachment to appear, select it from the list. If not, select the node on the model that your socket should be connected to in the Nodes list. Click Add Socket from the Socket menu and give a name to your new socket, then click OK.
4. Double-click the name of your socket in the Sockets list to open the Socket Properties dialog.
5. In the Attachment box, enter the name of your new attachment model or click the Browse button to locate it. This does NOT cause the model to appear in the game, but allows you to preview the model in ModelEdit.
6. Click OK.
7. Click on Show Attachments from the Options menu. Your model should now appear in ModelEdit, floating near or on the socket you created.
8. If your attachment is correctly set up, save and re-pack the player_action model.
9. If your attachment is not in the right location or needs to be turned to face the right way, click Show Sockets in the Options menu. The model's socket locations will become visible.
10. Select the name of your socket from the Sockets list again.
11. You can change the relation of the socket and attachment to the main model two ways: Using the Socket Properties dialog or the Transform Edit controls.
12. To use the Socket Properties dialog, double-click the socket and change the values in the Orientation, Position and Scale boxes.
13. To use the Transform Edit controls, click on the colored box for the property and axis that you want to change and drag the mouse left or right. The Rotation boxes change the rotation of the socket. Translation boxes move it, and Scale boxes scale it. These changes also cause the attached model to rotate, move or scale, allowing you to move it where you want it.
14. Once you have the attachment where you want it to be, save and re-pack the model.

---

## Step Three—Update Attachments.txt

In your project directory:

1. Go to \Game\Attributes and open attachments.txt.
2. Go to the last marked attachment listing. In TO2, this should be marked with the header [Attachment33].
3. Copy the header and the block of properties directly below it.
4. Paste the new section directly after the end of the section you just copied. Be sure to add blank lines if the sections run into each other so that you can read the file easily.
5. Change the Name, Model, and Skin0 properties to fit your new attachment.
6. If your model is using environment mapping to appear shiny, then add the name of the texture you want to use as the mask for the environment mapping in Skin1. Otherwise, delete Skin1 entirely and change RenderStyle0 to point to RS\default.ltb.
7. If you have your own custom RenderStyle, change RenderStyle0 to point to its location, and add or change any of the Skin properties to point to the textures required by your RenderStyle.
8. Save the file.

---

## Step Four—Assign and View the Attachment in DEdit

In DEdit:

1. Open your project.
2. Open a sample level or create a box level with room to walk around in it.
3. Using the crosshairs, add a CAIHuman game object to the level.
4. Go to the Properties tab.
5. Set the ModelTemplate property to Player.
6. Open the Attachments property.
7. Find the attachment location you want your new model to attach to in the list.
8. Drop that location's dropdown and find the name of your new attachment.
9. Click OK on the Attachments property dialog.
10. Process the level and run it.

A character appears in the level with your new attachment.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Tutorials\TAddAtch.md)2006, All Rights Reserved.
