| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Samples

The samples are organized into the following directories:

- [Sample Base ](#Base)
- [Art Content Samples ](#Content)
- [Audio Samples ](#Audio)
- [Graphics Samples ](#Graphics)
- [Networking Samples ](#Networking)
- [Samples Using Models ](#Models)
- [Samples Using Objects ](#Objects)

### Base

>

The \Samples\base\mpflycam folder contains the **MPFlyCam **sample. This sample is the same as SampleBase but has been expanded to include the ability to play in multi-player server mode.

The \Samples\base\samplebase folder contains the **SampleBase **sample. This simple example helps you get started with the Jupiter System. This sample also shows how to use the networking system.

The \Samples\base\simplephys folder contains the **SimplePhys **sample. This sample is the same as the MpFlyCam sample, but has been expanded to include physics such as gravity and stair-stepping.

### Content

>

The \Samples\content\maya\ folder contains sample content from No One Lives Forever™ 2 that can aid content creators in creating artwork that is easily usable with the Jupiter System. The No One Lives Forever™ 2 team uses Maya for character models. Included are samples of character and player view models that are properly rigged to work with the Jupiter System. These models should be exported to ModelEdit using the Maya 4.0 model exporter.

The \Samples\content\maya\SealHunter folder contains some of the artwork used to create the Seal Hunter sample mini game. These models were created using Maya 4.0.

### Audio

>

The \Samples\audio\music folder contains the **Music **sample. This sample demonstrates Jupiter's DirectMusic interface using music files from No One Lives Forever™ 2.

>

The \Samples\audio\sounds folder contains the **Sounds **sample. This sample allows developers to experiment with different sound settings for multiple sounds.

### Graphics

>

The \Samples\graphics\clientfx folder contains the **ClientFX **sample. This sample allows developers to browse and view effects created by artists. This sample reads through all of the effects in the current .fxf file and creates a graphical selection menu allowing you to scroll through and play any effects in that list.

The \Samples\graphics\drawprim\ folder contains the **DrawPrim **sample. This sample demonstrates the use of the DrawPrim interface as a means for creating procedural terrain. The height map data in this sample was created from USGS data.

The \Samples\graphics\fonts\ folder contains the **Fonts **sample. This sample demonstrates the creation and display of vector (*.ttf) and bitmap (*.dtx) fonts.

The \Samples\graphics\renderdemo\ folder contains the **RenderDemo **sample. This sample demonstrates basic polygon throughput. Models can be added in rows and columns while the polygon count and frame rate are displayed.

The \Samples\graphics\shaders\ folder contains the **Shaders **sample. This sample demonstrates the use of vertex and pixels shaders on models using RenderStyles.

The \Samples\graphics\specialeffects1 folder contains the **SpecialEffects1 **sample. This sample combines the effects of the Animations2 tutorial and the ClientFX sample.

The \Samples\graphics\video folder contains the **Video **sample. This sample demonstrates the video playing functionality of Jupiter.

### Networking

>

The \Samples\networking\nettest folder contains the **NetTest **sample. This sample demonstrates a simple way to create an in-game LAN game browser.

The \Samples\networking\sealhunter folder contains the **Seal Hunter **sample. This light humored sample implements a single player and networked multiplayer mini game with the following features:

- Single player or multiplayer game (supports up to 12 players in multiplayer)
- Windows and Linux stand-alone server for hosting games
- Scoring system, current score HUD, and ranking screen
- Multiplayer chat
- The world features environment mapping, translucency, dynamic lighting, shadows, and skybox
- ClientFX particle systems, sprites, and polytrails are used for the effects
- The character can attach 3 different weapons in multiplayer
- Audio clips played randomly for player smack talk
- Simple AI and collision for seals

### Models

>

The \Samples\models\animations folder contains the **Animations **sample. This sample is the same as the Projectiles sample, but has been expanded to include code implementing simple model animations in a multiplayer environment.

The \Samples\models\animations2 folder contains the **Animations2 **sample. This sample is the same as the Animations sample, but has been expanded to include code implementing more advanced model animations in a multiplayer environment.

The \Samples\models\attachments folder contains the **Attachments **sample. This sample builds on the SampleBase sample and shows how to make an attachment, break an attachment, and attach different objects to one socket.

### Objects

>

The \Samples\objects\doors folder contains the **Doors **sample. This sample shows how to create a simple door that opens when touched by the player or a projectile. It also allows placement of the door through the DEdit application.

The \Samples\objects\pickups folder contains the **Pickups **sample. This sample is the same as the SimplePhys sample, but has been expanded to include a box pickup. This tutorial shows how to create and implement code for pickups and powerups.

The \Samples\objects\projectiles folder contains the **Projectiles **sample. This sample is the same as the Pickups sample, but has been expanded to include the ability to throw an object. This tutorial shows how to create and implement code for projectiles.

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Intro\samples.md)2006, All Rights Reserved.
