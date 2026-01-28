**Trans. A.M. **





**Overview **

This document details the new Transitional Area Mapping level-loading scheme, referred to as the “Trans. A.M.” Since the two are closely related, the new system and structure of the revised saving and loading scheme is also discussed. This paper is intended for Level Designers who will be creating transition areas between levels and for QA who will be testing the two systems. However all are welcome to read and comment on it’s contents and to get a better understanding of how the systems work.



The whole idea behind the “Trans. A.M.” is to create a sense of non-linear, larger levels. Two or more levels can be linked together using the “Trans. A.M.” offering the player an opportunity to complete a mission in multiple fashions giving the effect that each mission is actually one huge level. Once a player leaves a level through a transition area they are able to go back to that level and have it look exactly as they left it, if they left a door open when they left a level will be open when they return.



**Creation **

To create a transition between levels you must first place a TransitionArea object in each level. The two TransintionAreas MUST have the same name and MUST have the same dimensions. If the TransitionArea in the first level is named TransAM01 the TransitionArea in the second level must also be named TransAM01. You can have multiple TransitionAreas in each level but for every TransitionArea in one level you must have a TransitionArea in the level you wish to transition to with the same name and same dimensions. Binding a TransitionArea object to a brush creates a TransitionArea. The volume contained by the TransitionArea holds the objects that will be transitioned between levels. If an ammo box is in the TransitionArea as you leave the first level it will be in the same position and rotation within the TransitionArea of the second level. The player’s position and rotation within the TransitionArea is also kept. To specify which level a TransitionArea is supposed to transition you to, enter the level number in the ‘TransitionToLevel’ property. This number corresponds to a LevelNumber in missions.txt. You can only create transitions between levels within the same mission, so if the ‘TransitionToLevel’ property is 1 then the level that this TransitionArea will load will be the level specified in the missions ‘Level1’ property. To trigger a transition from a TransitionArea simply send the TransitionArea a ‘TRANSITION’ message. You can send this message from a switch or even a trigger from within the TransitionArea. The player must be inside the TransitionArea in order for the transition to occur.



To better create the effect that the player never really left a level, the geometry and other objects surrounding the TransitionAreas in each level should match exactly. When creating a TransitionArea, keep it small and simple. Use areas such as a small room, hallway, stairwell or elevator as places to contain the TransitionArea. Try not to over populate the TransitionArea with a lot of objects. Certain objects will not transition even if they are inside the TransitionArea. All WorldModels and Triggers will not transition but they can be inside a TransitionArea during a transition.



**Save and Load **

To accomplish the “Trans. A.M.” effect of having a single level and having levels look exactly the way you left them upon return we needed to rework how the game handles it’s saved games. With the old method each profile name had it’s own directory under the games’ Save directory. Inside each profile save directory were the save files for quick save, reloading the level, and each slot that the game can be saved to. The new method still has directories for each profile name under the main games’ Save directory. However each profile save directory now has a Working directory, a QuickSave directory, a Reload directory and a directory for each slot the game can be saved to. Under each directory, with exception to the working directory, is the actual save file and a working directory. As a player transitions between levels each level is saved in a separate file under the main working directory. When the player saves a game the contents of the working directory are copied over to that particular save directory’s working folder. So if the player quick saved the game every level’s save file from the main working directory is copied to the QuickSave working directory. This way a record of every level the player has transitioned from is saved with that save file so when the player loads a saved game all levels he has visited will be in the state he left them. When a game is loaded the main working directory is emptied and the loaded games working directory is copied over. The structure of the new save directory is detailed below:



Save \ Player_0 \

Working \ …

QuickSave \ Quick.sav

\ Working \ …

Reload \ Reload.sav

\ Working \ …

Slot01 \ Slot01.sav

\ Working \ …

Slot02 \ Slot02.sav

\ Working \ …



************
