**Untargeted commands **



Whenever a msg command gets processed, it can either have the name of an object (e.g. "msg Ninja01 (addgoal chase seekplayer=1)"), or the class of the object in question (e.g. "msg <CAIHuman> (addgoal chase seekplayer=1)"). This allows messages to be verified by the command mgr even if they're going to be created at runtime. It can also have both, as in "msg <CAIHuman>Ninja01 (addgoal chase seekplayer=1)", in case you know what the name of the object is going to be at runtime.



When the object isn't explicitly listed in the command, it uses the "default" object target. Usually, this is the object sending the message. Here is the list of default object targets that are not defaulting to the sender, as near as I can deduce by looking through the code:



AINodeUseObject - PreActivateCommand & PostActivateCommand - The targetted use object

All WorldModel objects - All commands - The object activating the worldmodel.

KeyItem - PickedUpCommand - The object activating the key item

PlayerVehicle - LockedCommand - The object activating the vehicle

Trigger - CommandTouch - The object touching the trigger

Spawner - InitialCommand - The object being created

AINodeBackup - Command - The AI requesting backup

Controller - Flicker messages - The object being flickered

Anything Destructible - The damager and killer messages - The damager/killer

Console trigger messages - The object to which the message is being sent
