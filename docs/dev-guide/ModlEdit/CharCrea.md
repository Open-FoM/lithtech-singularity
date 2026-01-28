| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Creating a Character

The sections below explain the steps required to create a character in Jupiter. All the characters in TO2 were created in Maya, so steps will vary from package to package. Also, be aware that most of the naming conventions and skeletal layout requirements can be changed by modifying the TO2 game code.

#### Skeletons

All the human characters in TO2 have the same skeletal hierarchy. This allows animations to be universally shared.

To share animations across models, keep their height within a range of about 15%. Having extra tall or short characters will cause strange floating when using a shared animation. Exported skeletons will still need to match the existing TO2 skeleton in order to share animations with TO2 models. This may require some experimentation. If you'd rather use a different skeleton (for example, a ponytail) then you will need to redo all of the animations used by that character or use attachments to add extra objects.

TO2 uses a node called Eyelid to control vertices that allow blinking. It is important to name this bone correctly since the TO2 game code uses this node to tell what a character is looking at.

#### Polygon Counts

All characters used in TO2 were modeled using 1000-3000 triangles.

#### Textures

All texture mapping for TO2 was done in Maya. Each character uses two base texture maps: one for the body and one for the head. These should be saved as 32 bit .TGA files. They are converted in DEdit to .DTX files by going to the Textures tab and importing the .TGA files to the \Development\TO2\Game\ Chars\Skins folder.

#### Naming

Any model that is going to use the Jupiter AI code needs to be named in the same form as the characters for TO2, such as ninja.LTA. There is no comprehensive list of appropriate character names, but looking at the modelbutes.txt file (in the \Development\TO2\Game\attributes\ folder) should point you in the right direction.

Textures should be named in the same form as the models. You can use the modelbutes.txt file to define what textures are used where.

#### Export Location

Characters should be exported to the \Development\TO2\game\chars\models\ folder.

#### ModelEdit Attributes

After exporting a character, you use ModelEdit to add several attributes including sockets, weight sets, LODs, user dims (used for physics), and internal dims (used for in-game clipping). Most of these can be imported from a similar existing model if there is one. Just select File, then Import and select a similar character file, and then check the items you want to import.

#### Weight Sets

Jupiter characters can use weight sets to allow morphing animations for up and down aims with guns and also for blending upper and lower body animations. These can be created individually, but it's a lot easier to just import them from an existing character if you have one. If your new model has a different skeleton, weight sets will not import properly and will need to be created from scratch.

#### Levels of Detail

Most TO2 characters have three levels of detail (LODs). These can be imported from existing characters, but only from a character that has the same number of pieces. If your new character has more or less pieces than any of the existing TO2 characters, you can still use the LOD values of the existing characters by manually copying the values onto the new character.

#### Child Model Translation Information

Another thing that must usually be done is to turn off the translation information from child models on most of the bones.

In the case of TO2 characters, study an existing character and check the box next to each node name in the Nodes list that is checked off in your own character. Not doing this may result in strangely stretched or squished characters. If you create your own skeletons, you should decide for yourself which nodes are used for translation.

#### Multiplayer Animations

TO2 Multiplayer characters use different animations than the single player ones. Therefore, duplicates must be created at this point.

Place the copy in the \Development\TO2\Game\Chars\Models\Multi\ folder. The child model animations are in this directory already.

#### Internal Radius

After adding these animations, set the model’s internal radius. This is most easily done by going selecting Calc Internal Radius from the Model menu, but you can also set this value by hand on using Set Internal Radius on the Model menu. This value determines when the model appears and disappears from view. If you do not set an internal radius, your model will disappear as soon as its origin is out of view of the active camera. The internal radius should be exactly large enough to contain the model in any of its most extreme movements.

[Top ](#top)

---

## Attachments

Any model can be used as an attachment, but won’t animate without specifically being called by the game code.

#### To use an attachment in the game

1. Update the attachments.txt file in the \Development\TO2\Game\attributes folder. The attachments.txt file contains comments that will help you to understand how it works.
2. Copy and paste any attachment section to the end of the attachment list.
3. Change the number and name to be appropriate for your new attachment.
4. Also change the file paths to point to your attachment file and texture. These can be named however you want, and it's best to keep them in the \Game\Models\Attachments and skins\ folder.
5. The attachment must now be added to the character in DEdit.

#### Frame Strings

Frame strings are added to TO2 models to provide animation based cues to the game code. For characters, the ones used are **bute_sounds **for death sounds and **FOOTSTEP_KEY **for footsteps.

These are entered by going to the frame of animation where the event occurs and typing the appropriate string in the Frame String box. These can be placed on the child models when possible so many characters can use them.

#### Animation Editing Features

The simple animation editing features of ModelEdit, found in the Animation menu, can only be used on animations which are native to a model (not from a child model).

Use Set Animation Length to set the time the selected animation takes to play back to speed up or slow down an animation.

Use Set Animation Rate to get rid of frames in a really dense animation. This is mostly useful when dealing with motion capture which is at 30 frames/second. Most animations look fine at 8-10 frames/second using the linear interpolation of the engine.

Make Continuous is an easy way to make a simple cycling animation. Select an animation and delete the last couple of frames making a note of how much time is being cut by doing this. Then go to Make Continuous and enter the time that was noted above. This may need to be tweaked a little, but on animations that don't have a lot of action, usually this will give a good cycle.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ModlEdit\CharCrea.md)2006, All Rights Reserved.
