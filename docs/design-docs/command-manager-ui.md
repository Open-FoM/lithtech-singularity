**Command Manager User Interface **



The command manager user interface provides a means for creating, editing, and viewing a series of messages or commands grouped together in a chronological order inside of a single command object. These commands are messages that are sent to the specified objects, and for more information on the content of these messages, the appropriate game object documentation should be referred to. The commands are grouped together not only by object, but also by track. Each object has a certain number of tracks, usually six, of which can hold a specified number of commands. These tracks have no effect on the order or manner in which the commands are processed, and exist only for a means of aiding content creators to better organize objects for themselves and others.



Below is shown a screenshot of the user interface for editing commands:



EMBED PBrush

****

**1: **Track Names



Along this side of the user interface is a list of the track names. Each track can have up to 16 characters indicating the name of the track. This has no effect on the final object and is used only for organizational purposes.



**2: **Command View



This window provides a view of the current layout of the commands, with time increasing as it moves along the X-axis, and the track changing along the Y-axis. Each command is represented as a circle, which is blue if unselected, and green if selected. For each track, there is a black bar that extends from the very first command on the track to the very last command on the track so that it is easier to visualize what tracks are currently active at any given point in time. In addition there is an active track, indicated by a red background that all operations will occur on. This active track automatically follows the mouse pointer and serves primarily as a visual aid to indicate to the user which track will have actions performed upon it.



**3: **List View



Pressing this button will bring up the list view dialog which is described later. This provides a means of quickly viewing and editing all commands.



**4: **View Ruler



This ruler located under the view serves as a visual indicator for quickly assessing where commands are located along the timeline.



**5: **Time Position



This scrollbar located beneath the view is used to control the section of the timeline that is currently visible. Scrolling it to the right increases the starting visible time, and scrolling it left decreases the starting visible time. The scrollbar can go from time 0 up until the specified length of the command object.



**6: **Statistics



Located in the bottom left statistics about the command object being edited can be found. The top statistic tells what time is located directly beneath the mouse cursor. Below that is an indicator specifying how many commands are already place upon the command object. The last statistic tells both how many objects are placed on the currently active track, along with how many objects can be placed on that track.



**7: **Message



If a command is selected, its message can be entered in this edit field. This edit field will change with the selection to fit the text of the selected commands. If a single command is selected, this edit field will display the associated text. If multiple commands are selected, it will determine if they are all set to the same value, and if so will display that value inside the edit box, and otherwise leave it empty. Changing the text inside of this edit field is immediately applied to all currently selected objects.



**8: **Immediate Message



This edit field displays the text of the topmost command that lies immediately beneath the mouse cursor. This is used to quickly see what the text of a command is without having to select it.



**9: **Scale



The value within this edit field indicates in seconds how much is visible within the view. This value can range from one second on up to two minutes. It will not be able to go beyond the total length of the command object however, so if it is less than two minutes, it will use that as the maximum scale possible. Adjusting this value allows for the user to quickly view an entire command object, or zoom in close for precision placement of commands.



**10: **Length



The value located within this field indicates the total length of the command object. Commands cannot be placed outside of this range, and this cannot be reduced less than any currently placed objects. The maximum time span for a single command object is fifty minutes and the minimum is one second.





**Command Manipulation **



There are a variety of means to modify the commands of which are described below. They must all be performed within the main view, and most commands require either a selection or an active track, and cannot be performed without them. There are three means of accessing the manipulations, one is through keyboard commands, one is through mouse commands, and the final means is through the context menu shown below:



EMBED PBrush



**Selection **

Accessible through: Left Mouse

Requires: -



Simply left clicking on a node will select that node. It will also deselect any previously selected nodes. Users can click on a node while holding the shift key down causing the selected state to be toggled, allowing multiple nodes to be selected.



**Moving **

Accessible through: Left Mouse Drag

Requires: -



By pressing and holding the left mouse button over a command, it will select the command and allow the user to drag it to a new location. This command can be moved in the X direction to change its time, or in the Y direction to change the tack that it lies on. It is important to note though that it will not allow the command to be dragged onto a track that already contains the maximum number of commands.



**Nudge **

Accessible through: Left and Right arrow keys

Requires: Selection



Nudge allows for fine adjustments of the currently selected nodes along the X axis.



**Add Event **

Accessible through: Insert key or context menu

Requires: Active Track



If the currently active track is not full, it will place a new command at the current mouse location with an empty associated message.



**Delete Event **

Accessible through: Delete key or context menu

Requires: Selection



If commands are currently selected, they will be removed from the command object.



**Delete All **

Accessible through: Context menu

Requires: Existing commands



This will remove all commands from the command object, giving a fresh slate to work from.



**Delete Track **

Accessible through: Context menu

Requires: Active Track



This will remove all commands from the currently active track



**Cut **

Accessible through: Context menu

Requires: Selection



This will remove the currently selected command and place it on the clipboard so that it can be later pasted. Note that this will only work for a single command and not multiple.



**Copy **

Accessible through: Context menu

Requires: Selection



This will place the currently selected command on the clipboard so that it can be later pasted. Note that this will only work for a single command and not multiple.



**Paste **

Accessible through: Context menu

Requires: Active Track



This will create a new command at the current mouse position on the currently active track that has an identical message to the previously copied or cut command. Note that this will not work if the currently active track is already full.





**List View **



The list view provides an alternate view for displaying all the commands in a command object. This view presents the message, time, and track number of every command in table. A screenshot is provided below:



EMBED PBrush



The commands can be sorted by clicking on any one of the headings, which can also be rearranged. The message text can be edited for any command by double clicking on the appropriate row. The goal of this list view is to provide a quick means for viewing the text of all the commands as well as being able to view them in a strictly chronological order to allow for easier and quicker debugging of objects.



**Command Feedback **



After pressing OK on the dialog, all results will be stored back into the object and processed for quick feedback on the validity of the passed in messages. If any of the messages entered are invalid, the DEdit message window will appear and the most recent entries will contain information about the offending messages. The same will occur on level load, so if upon opening a level the DEdit Message Window appears, the level contains a command object that has incorrect messages and should be fixed. An example error is shown below:



EMBED PBrush
