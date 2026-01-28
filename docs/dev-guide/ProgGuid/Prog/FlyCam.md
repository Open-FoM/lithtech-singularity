| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Multiplayer Flying Camera Tutorial

The Multiplayer Flying Camera Tutorial adds multiplayer capabilities to the [samplebase ](SampleB.md)sample. This tutorial
describes how to add multiplayer functionaity to the the sample base, and serves as an example of how you an add multiplayer functionality to your own game.

You can find the flying camera sample code in your Jupiter installation under:

>

\samples\tutorials\mpflycam

The run.bat file in that directory can be opens and runs the sample.

When the sample begins, you need to bring down the console by pressing the ~ (tilde) key. Then you should type
normal, serve, or join and hit enter.

>

Typing normal creates a regular, single-player game.

Typing server creates a multiplayer server that other clients can attach to.

Type join < IP address > (where < IP address > is the IP of a computer that is running the sample in server mode) to
join a server game. The IP should be in the form 127.0.0.1.

## Client Shell

We will begin this tutorial looking at the changes to the clientshell. There are several changes in \cshell\src\ltclientshell.h
and .cpp. The two major changes are in the way the game is created and in the data that is sent to the server.

The **InitGame **method now takes a few parameters to specify the type of game you are creating, and an optional IP
address to connect to.

There are also several functions near the top of ltclientshell.cpp. These functions are console programs. They are
registered with the engine as console programs in **OnEngineInitialized **. Once they are registered, they will be called
whenever the user brings down the console and types in the program name. This is how the game type is set for this
sample. In a full game, you should build a UI that will take care of these choices.

The second major change to the client is that we are now using velocity to move the player instead of simply setting
a new position. This creates a much smoother networked game because the engine can use the velocity to predict the
future location of an object.

Some other changes include the setting of the client object to non-visible and a new method called **SetService **. This
method selects the TCP/IP service as the intended service for networking.

## Server Shell

There are only a few changes to the Server Shell.

First, in the **OnClientEnterWorld **method, the object created is a model. This model represents the player and is
visible on all clients (except the controlling client has overridden this). The model’s filename and skinname (texture) are
set appropriately. Also, after creation the model is set to play its flying animation.

The other change to the Server Shell is to read the data from the client as a velocity update instead of a position
update. This is a simple change that only affects the **OnMessage **method.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Prog\FlyCam.md)2006, All Rights Reserved.
