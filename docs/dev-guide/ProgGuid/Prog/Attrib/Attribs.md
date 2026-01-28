| ### Content Guide | [ ![](../../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Working With Attributes

TO2 Game code uses attributes files to control a wide variety of functions. Attributes files are located in the following directory:

>

LithTech\LT_Jupiter_Src\Development\TO2\Game\Attributes

If you are using the TO2 game code as a base, or as a reference when creating your own game, then the attributes files are particularly useful in setting the basic functionality of components in the game. Attributes are applied to many components of the game from animations to surface textures. By editing the attributes files you can redefine attributes or add entirely new attributes to your game.

To redefine an attribute, or to add a new attribute, simply open the appropriate attributes file in a text editor, edit the file following the commented instructions, save the file, process your game, and test the results.

The following table describes each of the files contained in the Attributes directory:

| #### File Name | #### Description |
| --- | --- |
| ActivateTypes.txt | Defines activation types such as OpenClose, TipOver, and SoundAlarm. Associates string IDs from Cres.DLL to display when an object is in an On or Off state. |
| aibutes.txt | Defines the "brains" of the AIs determining how they act and react in the game. |
| aigoals.txt | Defines goals for AIs used and their requirements. |
| animationshuman.txt | Defines animation sets used by human models. |
| animationsmime.txt | Defines animation sets for mimes. |
| animationsninja.txt | Defines animation sets for ninjas. |
| animationspierre.txt | Defines animation sets for Pierre. |
| animationsrat.txt | Defines animation sets for rats. |
| animationsrobot.txt | Defines animation sets for robots. |
| animationssupersoldier.txt | Defines animation sets for super soldiers. |
| animationsunicycle.txt | Defines animation sets for unicycles. |
| attachments.txt | Defines game objects that can attach to models. |
| clientbutes.txt | Defines client effects attributes and debuggin information for AIs. |
| clientsnd.txt | Defines sound path attributes. |
| commands.txt | Defines groups of commands and messages that can be called by game objects. |
| coopmissions.txt | Defines multi-player missions that specify the levels in a game. Sets mission goals, sets default weapons, and associates level files with missions. |
| damagetx.txt | Defines all of the attributes associated with a particular DamageFX |
| damagetypes.txt | Defines the attributes of the various damage types in the game. |
| debris.txt | Defines debris used in the game. |
| fx.txt | Defines the special effects used in the game. The majority of effects are weapons related, but a small minority of effects used for other purposes are also included in this file. |
| gadgettargets.txt | Defines all of the attributes associated with a particular Gadget Target |
| intelitems.txt | Defines all of the attributes associated with a particular intelligence item. |
| inventory.txt | Defines inventory items. Not currently used. |
| KeyItems.txt | Defines all of the attributes associated with a particular key item type. Key items are required to unlock or perform certain actions in the game. |
| layout.txt | Specifies positions and sizes for common screen components. |
| missions.txt | Defines single-player missions that specify the levels in a game. Sets mission goals, sets default weapons, and associates level files with missions. |
| modelbutes.txt | Defines attributes of model types used in the game. |
| mp_weapons.txt | Defines WeaponOrder, Ammo, Weapon, Mod, Gear, and Weapon animations for weapons used in the game. |
| music.txt | Controls the dynamic music. there are two elements of the dynamic music which are controllable: the mood of the music, and the events. This concept maps into direct music's intensities and motifs, respectively. |
| PopupItems.txt | Defines popup sprites and textures used to display items to the character when they are looking closely at an object. |
| proptypes.txt | Defines the attributes of props used in the game. |
| RadarTypes.txt | Defines radar points that appear on the characters radar. |
| RelationData.txt | Defines relationships between the characters in the game. Relationships are used by the AI to help determine actions. |
| searchitems.txt | Defines all the attributes associated with a particular item that is found while searching. |
| serverbutes.txt | Defines attributes for the player and for security cameras. Use the file to adjust how the player can interact with elements of the game. |
| serveroptions.txt | Defines server options such as control types and game types. |
| serversnd.txt | Defines server sound attributes such as sound paths, body sounds, and AI sounds. |
| skills.txt | Defines skills that the player can acquire. |
| soundbutes.txt | Defines all of the attributes associated with a particular Sound Bute. Defining a SoundBute allows you to make global changes to all the Sounds. |
| sountfilters.txt | Defines sound filters used in the game. |
| surface.txt | Defines properties of surface textures. |
| TriggerTypes.txt | Defines trigger types used in the game. |
| weapons.txt | Defines weapons used in the game. |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Prog\Attrib\Attribs.md)2006, All Rights Reserved.
