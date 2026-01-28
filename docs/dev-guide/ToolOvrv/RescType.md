| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# LithTech Resource Types

Jupiter can access and use a wide variety of file types and other resources. The list below is arranged by resource type, and each resource type includes a list of the sorts of files Jupiter can use in that type, as well as a description of those files.

**Directories **—The subdirectories that contain your other game resources are a type of resource in themselves. The structure of these directories defines the tree views you see in many of the tabs in DEdit’s Project window. If you find a game’s .DEP file and look at the subdirectories in the same folder with it, you’ll see names such as TEXTURES, WORLDS, SOUNDS, and MODELS. These folders contain whatever resource they’re named for, so if you want to add new textures to a game or copy a world from one game to another, you should look in the directories named Textures and Worlds, respectively.

>

**Note: **Inside each of these directories is a tag file with a name that tells DEdit what resource it contains. A file called DIRTYPETEXTURES tells DEdit that the directory contains textures, DIRTYPEWORLDS says the directory contains worlds, and so on. If you want to add new subdirectories or reorganize your resources, just copy the proper tag file into any new directories that don’t have one, and they will appear on the proper tabs in DEdit.

**Models **—Models in Jupiter are in a custom format, the .LTA file. This contains the mesh/geometry for the model and other information used by the game engine. You create these files in an external editor such as Maya or 3D Studio Max, then save them in the .LTA file format using a plug-in. Use ModelEdit to modify files in the .LTA format once they’ve been created.

**Prefabs **—A prefab is a collection of objects, both brushes and code objects that are stored in their own .LTA files (just like a regular game world) in a separate set of directories. Anything you make in a level can be exported as a prefab to make it easier to re-use later. Examples are benches, camera systems, hallways, doorways, and statues. Prefabs can be useful if your group needs a way to distribute standard-looking objects to its members.

**Project **—As mentioned before, the project is the total of your whole game and all its resources. A .DEP (DEdit Project) file at the top of your tree of game resource directories serves as a guide to DEdit. The .DEP file is a pointer to the rest of the resources for DEdit and includes some of the information about your game resources. The rest of your game’s files exist in subdirectories of the directory where your .DEP file resides.

**Simulation Objects **—These are objects that programmers construct using code. Some of this code exists in the engine and doesn’t appear in the project directories. However, many objects are created in code written for the specific game. These objects are called game objects (to distinguish them from the engine objects inside the engine code) and are stored in a file called OBJECT.LTO in the same directory as the project’s .DEP file.

**Sounds **—Jupiter supports standard .WAV files directly with no custom modifications. When you double-click a .WAV inside of DEdit’s Project Window, it will open in your system’s default sound editor so that you can preview it.

**Sprites **—Sprites (.SPR files) consist of a series of .DTX files linked together as an animation with a set frame rate. They’re commonly used for animations and special effects such as smoke, bullet holes, and liquid droplets.

**Textures **—Jupiter uses its own form of texture, the .DTX file. Textures can be up to 32-bit images, although you can obviously choose to make lower-color textures. The .DTX files also contain flags for additional information used by the engine and the game. You can convert .PCX and .TGA files into .DTX files by importing them inside of DEdit.

| **Note: ** | Paint Shop Pro™ and some other programs have the capability of writing compressed run-length encoded Targa (.TGA) files. Though this compression makes the files smaller to store, it renders them unreadable by DEdit when you try to create a .DTX texture. You can choose the Uncompressed option in Paint Shop Pro when you save a .TGA file to make that file readable by DEdit. |
| --- | --- |

**Worlds **—Worlds come in two forms: .LTA files and .DAT files. If you think of worlds as programs, the .LTA file is the source code: Editable and understandable by the user and DEdit, but not executable. The .DAT file is like an actual program: it can be run in the game, but since it’s been “compiled” (processed, in this case), the user can no longer modify it. The .LTA files are what you change and save changes to inside of DEdit, and the .DAT file is the output of Processor.exe, which prepares your world to run in the game and optimizes its performance.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ToolOvrv\RescType.md)2006, All Rights Reserved.
