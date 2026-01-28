**Player Messages **

The “player” object handles a number of different trigger messages. This document will list the syntax and results of these various messages. In the syntax definitions below, parameters that appear in angle brackets, i.e. < > are required parameters. Parameters that appear in square brackets, i.e. [ ] are optional.

***Music ***

Most of the player messages to affect the game music take an optional timing parameter. This parameter may be any of the following:

Default - Will use the default value that is defined in the Control File or by DirectMusic

Immediately - Will happen immediately

Beat - Will happen on the next beat

Measure - Will happen on the next measure

Grid - Will happen on the next grid

Segment - Will happen on the next segment transition

**Intensity **

This command changes the intensity of the music.



Music i <intensity> [timing]

Music intensity <intensity> [timing]

**Secondary Segment **

This command will play a secondary segment.



Music secondary <segment name> [timing]

Music ps <segment name> [timing]

**Motif **

This command will play a motif.



Music motif <motif style> <motif name> [timing]

Music pm <motif style> <motif name> [timing]



**Volume **

This command will adjust the volume of the music.



Music volume <volume adjustment in db> Music v <volume adjustment in db>





**Stop Secondary Segment **

This command will stop a secondary segment.



Music stopsecondary <segment name> [timing]

Music ss <segment name> [timing]

**Stop Motif **

This command will stop a motif.



Music stopmotif <motif name> [timing]

Music sm <motif name> [timing]



**Play **

This command will start the music playing.



Music play

Music p

**Stop **

This command will stop playing the music.



Music stop [timing]

Music s [timing]

**Lock Mood **

This command will lock the music in the current mood.



LockMood

LockMusic

**Unlock Mood **

This command will unlock the music from the currently locked mood.



LockMood

LockMusic



**Lock Event **

This command will lock the music in the current motif.



LockMotif

LockEvent





**Unlock Event **

This command will unlock the music from the currently locked motif.



LockMotif

LockEvent





***Objectives, Optional Objectives, and Mission Parameters ***

The player’s objectives are divided into three classes. Objectives are goals which must be accomplished to complete the mission. Optional Objectives are goals that may be accomplished, but are not required. Mission Parameters are rules for how your objectives may be accomplished. The objective IDs referred to in these commands are the ids of strings in the CRes.dll string table.

****

**Add Objective **

This command will add a new required objective.



Objective add <objective id>

**Add Optional Objective **

This command will add a new optional objective.



Option add <objective id>



**Add Mission Parameter **

This command will add a new mission parameter.



Parameter add <objective id>



**Remove Objective **

This command will remove an objective, optional objective, or mission parameter.



Objective remove <objective id>

**Remove All Objectives **

This command will remove all objectives, optional objectives, or mission parameters.



Objective removeall

**Complete Objective **

This command will mark an objective or optional objective as completed.



Objective completed <objective id>



***Keys ***

The key IDs referred to in these commands are the ids of entries in the KeyItems.txt attributes file.

**Add Key **

This command will add a new key item.



Key add <key id>

**Remove Key **

This command will remove a key item.



Key remove <key id>

**Remove All Keys **

This command will remove all key items.



Key removeall



***Transmission ***

Transmissions are text messages that are sent to the player. The transmission ID referred to in this command is the id of a string in the CRes.dll string table.



Transmission <transmission id>



***Credits ***

The credits message starts or stops the on screen credits.

**Start credits **

This message will start the display of credits.



Credits on



**Stop credits **

This message will stop the display of credits.



Credits off



**Intro credits **

This message will start the display of the intro credits.



Credits intro



***Intel ***

This message will give the player an intelligence item.

The <text id> parameter is the ID of a string in the CRes.dll string table.

The <popup id> parameter is the ID of an entry in the PopupItems.txt attributes file which will define how the item will be displayed in popups and in the menus.

The [intel] parameter should be 1 if the item should be considered as valuable intelligence data and 0 if it should be considered general information. This will be 0 by default.

The [show] parameter should be 1 if the popup up should be displayed and 0 if it should not. This will be 0 by default.



MissionText <text id> <popup id> [intel] [show]





***Fade In, and Fade Out ***

These messages cause the screen to fade in or fade out over the given number of seconds.



FadeIn <time>

FadeOut <time>



***Mission Text ***

**This message is not yet implemented in TO2 **

This message will cause text to appear teletype style on the screen. The text id is the id of a string in the CRes.dll string table.



MissionText <text id>



***Mission Failed ***

**This message is not yet implemented in TO2 **

This message will the Mission Failure screen to appear and display the specified text. The text id is the id of a string in the CRes.dll string table.



MissionFailed <text id>





***Remove Bodies ***

This message will remove all body objects that are currently in the level.



RemoveBodies



***Rewards ***

Rewards give the player experience points. The <reward name> parameter specifies the name of the reward given. The amount of experience received is set in missions.txt and is indexed by <reward name>



Reward <reward name>
