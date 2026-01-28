| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Implementing Your Game Using Jupiter

The Jupiter System arms your developers with the tools and code base to enable your entire game team to focus on the creation of compelling game content. Instead of spending their time designing and coding a renderer or physics system, for example, they can concentrate on supporting level designers and artists.

This section provides some suggestions for implementing a game using Jupiter and exploiting the benefits of having developers with (relatively) more time on their hands. This document assumes that you have already completed a game specification and have a design document very near completion. Armed with these imperatives, your team can roll up their sleeves and start working on the game.

>

**Note: **While you can use the No One Lives Forever 2 objects and code provided with Jupiter, the art resources are included as samples only and cannot be included in any released games. TO2 textures, models, levels, and animations are the property of Monolith Productions, and may not be included in any released product.

This section contains the following suggestions to aid you in getting started implementing your game using the Jupiter system:

- [Learn the Jupiter System ](#UnderstandJupiter)
- [Review your Design Documentation ](#ReviewDesignDoc)
- [Identify your Development Tasks ](#IdentifyDevelopmentTasks)
- [Modify your Object Properties ](#ModifyObjectProperties)
- [Create your New Levels ](#CreateNewLevels)
- [Develop your Artwork ](#DevelopArtwork)

---

## Learn the Jupiter System

The first thing that your entire game team must do is sufficiently acquaint themselves with Jupiter.

For developers, this means, at the very least, learning how the game objects work so that they can modify and/or duplicate them. If you are adding significant functionality to the Jupiter code base, then they will also need to familiarize themselves with the sections of code to be modified. Examining the TO2 game code is a perfect way to learn how Jupiter works. The engine code includes comments for specific classes, methods, and so on. The Programming section of this document provides many overviews for the various Jupiter subsystems.

Level designers must acquaint themselves with the tools and many game objects and level design issues unique to Jupiter. The DEdit and ModelEdit tools give your artists significant powers in designing levels and models. Reviewing the TO2 levels provided with Jupiter is ideal for learning about Jupiter objects. Level designers can also peruse the objects and properties listings in DEdit.

[Top ](#top)

---

## Review Your Design Documentation

After your developers and level designers have familiarized themselves with what Jupiter has to offer, they should spend some time comparing the resources of Jupiter to the demands of your design document. That is, does Jupiter provide all the objects and functionality required by your game? You should identify all such differences and prioritize them. After completing this task, you should have a good idea of the work load for your developers.

[Top ](#top)

---

## Identify Your Development Tasks

Your developers, freed from the burden of coding the game engine, are going to have a lot of time to support the needs of your level designers. The following list describes some of the common tasks your developers need to complete:

- **Reviewing Code and Objects **

>

As stated previously, it is imperative that your developers learn the Jupiter code so that they can efficiently modify or add to it. For more information about the Jupiter code, see the Jupiter API Reference.

- **Setting Up the Game Project **

>

You will need a project in which the level designers and artists can test their content. The easiest way to set up your game project is to use the TO2 game as a base by renaming TO2.dsw, and then modifying the project to suit your own requirements.

- **Altering Existing Objects **

>

Obviously, your game will likely require objects uniquely suited to your own game, and radically different than those provided by the TO2 objects. It is often possible to modify the existing TO2 objects to suit your needs rather than putting forth the effort to create entirely new objects.

- **Creating New Objects **

>

Your game may demand such a unique object that none of the existing TO2 objects can be easily altered to fulfill the required functionality. In this case, your developers will need to create a new object. The existing objects, however, can still provide very good examples for this endeavor.

- **Adding or Altering Code **

>

If your game has extreme or unique game play elements your developers may need to enhance the Jupiter code base.

- **Purging Unneeded Code **

>

Near the end of the development cycle, your developers should sweep the game project for unneeded code. If you used the TO2 workspace as a starting point, there may be extraneous code that your game does not utilize. To streamline your game code, some effort should be expended in removing this code.

Be sure to examine the code carefully before deleting it to make sure it has no dependencies and will not result in a bug.

- **Designing Levels **

>

Your level designers will have more access to receptive developers thanks to the game engine Jupiter provides. This should translate into quicker turnaround on functionality requests and improved content.

Level designers will continue to design levels as usual, but they should pay attention to these Jupiter-specific issues:

- **Learning the Tools **

>

The DEdit and ModelEdit tools provided with Jupiter are used to create levels and prepare models for inclusion in those levels, respectively. Unless they know these tools, designers will find their efforts hampered.

- **Learning the Objects **

>

Jupiter includes many TO2 objects that you can leverage in your game. At the beginning of your game development cycle, designers should determine which of these objects your game will use. They should also help identify which of your game design requirements are not fulfilled by Jupiter and inform the developers of them.

- **Reviewing the Levels **

>

A good learning experience for your level designers is to traverse the TO2 levels included with Jupiter. They should keep an eye out for issues unique to your game design and critically analyze your games levels to determine how to accomplish your needs using Jupiter. It would be a good idea to have a developer present during this review process so they can provide real-time input on the feasibility of various requests. After you start the game, click Single Player and then Custom Level to choose from the levels included with Jupiter.

[Top ](#top)

---

## Modify Your Object Properties

Many of the TO2 objects will closely fulfill the requirements of your game. Often, all you must do is alter the properties of an object to make it comply with the demands of your game. Of course, you will also have to reset the textures and models for your objects to utilize your art assets instead of the proprietary TO2 assets.

[Top ](#top)

---

## Create Your New Levels

Using DEdit and the modified objects, your level designers must create new levels for your game.

##

[Top ](#top)

---

## Develop Your Artwork

Artists must merely comply with a few Jupiter requirements when creating their textures and models. They must also use the ModelEdit tool to prepare the model for inclusion in the Jupiter game.

When creating a model in 3D Studio MAX or Maya, follow these high-level steps:

1. Create a sphere.
2. Create a joint.
3. Bind the skin.
4. Export.


**Note: **Use 32 bit .TGA files for textures in the Jupiter system.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ImpGame\ImpGame.md)2006, All Rights Reserved.
