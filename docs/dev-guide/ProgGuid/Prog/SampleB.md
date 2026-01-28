| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Sample Base

The samplebase sample is an introductory programming sample that provides a basic framework for Jupiter game code. Licensees can use it to begin building their projects and it is the base project for other samples and tutorials. This documentation presents an overview of the functions and classes in samplebase and the basic flow of a Jupiter game.

The samplebase sample is located in your Jupiter installation directory under \samples\base\samplebase\ . The run.bat file in that directory can be used to launch samplebase. The Escape key can be used to exit the sample.

This section contains the following topics related to the program flow of the sample base:

-
[The ClientShell ](#TheClientShell)
-
[The ServerShell ](#TheServerShell)

---

## The Client Shell

A LithTech game begins its life in the game implementation of the **IClientShell **class. The interface for the **IClientShell **class is defined in \engine\src\inc\iclientshell.h . The game code must define its own class that inherits from IClientShell and implements certain methods. In samplebase, this class is called **CLTClientShell **and is defined and implemented in \samples\base\samplebase\cshell\src\ltclientshell.h and ltclientshell.cpp . For more information about the Client Shell, see [Client Shell ](../CltShell/mCShell.md).

### OnEngineInitialized

The engine first transfers control to your game code in the **IClientShell::OnEngineInitialized **method. This function verifies that various systems are functioning properly, and then attempts to initialize the renderer and sound subsystems of the engine. It also makes a call to **CLTClientShell::InitGame **that will initialize our small game. Lastly, it creates a few classes that we will use on the client.

At this point in the execution of the game, only the cshell.dll and cres.dll files have been loaded. Usually, the game would display a menu at this point and ask the user to choose what she would like to do next. For this sample, we will be loading a world and jumping into it directly. This is only to reduce the amount of code in the sample.

### InitGame

The **CLTClientShell::InitGame **method asks the engine to begin a game. Due to the client/server architecture of the LithTech engine, a game must have a client and a server part, each of which control certain behaviors. Creating or connecting to a server is necessary in order to load a world.

Once connected to the server, the client will load and unload worlds based on what the server instructs it to do. To do this, we create a **StartGameRequest **structure and fill in its members appropriately. We are asking for a **NORMAL **game which means the entire game will run on this computer, and it will not accept outside connections. This will create a standard, single-player game. Also, we tell the server which world to load. The call to **ILTClient::StartGame **initiates all of this. If this method returns failure, then there was a problem creating the server.

At this point, the object.lto is loaded, and several methods in the **IServerShell **class are called to initiate program flow on the server. These will be covered in a separate section below.

### OnEnterWorld

Once the server is initialized, then the **IClientShell::OnEnterWorld **method is called. This method is a good place to initialize any variables or classes on the client that should be reset when you enter a new world. Similarly, when you exit a world, the **IClientShell::OnExitWorld **method is called.

### Updates

After your client has entered a world, a regular update loop will be called every frame until the world is exited. This loop consists of one call each to **IClientShell::PreUpdate **, **IClientShell::Update **and **IClientShell::PostUpdate **, in that order. These calls have no major functional difference, but you can place different code in each one to logically structure your program. In samplebase all of the code is in the **Update **method for simplicity.

The **CLTClientShell::Update **method is the main loop for the client game code. It first checks to see if this is the first update it has received. If it is, then it sets up some state on the player object regarding dimensions and mass. The **ILTClient::GetClientObject **method returns the single object which acts as a link between the client and the server. Usually, this object represents your player character or your avatar on the server.

The **Update **method next synchronizes the position of the camera to the player object. The camera is created as a separate object so that it is easier to implement a 3rd person camera or a cinematic cut scene. The call to **PollInput **uses the input system in LithTech to determine if certain input commands are being activated and adjusts the player’s position and rotation accordingly. For more information on how to setup input in your game, see [Collecting Input ](../CltShell/input.md). Finally, this method calls **Render **and gives the **CWorldPropsClnt **class a chance to update itself.

### Render

The **CLTClientShell::Render **method handles the rendering of the scene each frame. This follows a basic pattern but can be modified to suite your needs. In this simple case, there is only one allowed camera and no heads up display or menu support. In addition to the **ILTClient **methods called here, you can use the CUI windowing toolkit and **DrawPrim **to render any custom effects that you have. For additional information about rendering, see [Rendering ](../Render/mRender.md).

### Other Functions

Several other methods in **CLTClientShell **perform useful functions.

The **OnMessage **method accepts messages sent from the server and reads the appropriate data or routes them to other places. Currently, this is used to get the appropriate starting position and rotation, and to set the world properties information.

The **OnCommandOn **method is called whenever a user presses a key or button that is mapped to a command using the input system. Most of our input processing is in **PollInput **, but you should handle commands that should be triggered on keypress in this method.

There are several methods that send the player’s current position and rotation to the server. These are **SendPosition **, **SendRotation **and **SendPosAndRot **.

Another useful method is the **SpecialEffectNotify **method. Despite its name, this method is very useful for objects that are not special effects. The engine calls this method when the client receives a message from the server to create an object that has a Special Effects message attached to it.

Any object on the server can have its own message attached to it. This is especially useful for objects that will be largely controlled on the client. Objects that will be controlled on the server typically do not need extra data sent to the client. However, a particle system should be animated on the client and not the server, to save bandwidth, so the server attaches a Special Effects message to the object with the appropriate object data. The client will then read this message and use the data to properly animate the particle system.

There are many other useful functions in the clientshell that are called in response to certain events. For further information, see the [IClientShell ](../CltShell/ICShell.md)class in the [ClientShell ](../CltShell/mCShell.md)section.

[Top ](#top)

---

## The Server Shell

The server-side part of a LithTech game begins its life in the game implementation of the **IServerShell **class. The interface for the **IServerShell **class is defined in \engine\src\inc\iservershell.h . The game code must define its own class that inherits from **IServerShell **and implements certain methods. In **samplebase **, this class is called **CLTServerShell **and is defined and implemented in \samples\base\samplebase\sshell\src\ltservershell.h and ltservershell.cpp . For more information about the Server Shell, see [Server Shell ](../SrvShell/mSShell.md).

### OnServerInitialized

Remember from the clientshell section that the servershell is created in response to the call to **ILTClient::StartGame **. Similar to the clientshell, the life of the servershell begins in **IServerShell::OnServerInitialized **. The **CLTServerShell **only verifies that the necessary interfaces to the LithTech engine exist and then returns success. Since we told the engine to load a world in our **StartGameRequest **, we do not have to repeat that on the server. However, you can control the level through the **ILTServer::LoadWorld **and **ILTServer::RunWorld **methods.

### Connect to the Server

The engine will then call the **IServerShell::OnAddClient **and **IServerShell::OnClientEnterWorld **methods because our local client is attempting to connect to our server. The **OnClientEnterWorld **method must create an object that represents the client. This special object is called the client object. It may be a model or any other type of object, even an invisible one. This method creates the object and attaches a Special Effects message to it. This message is currently empty, but allows the client shell to receive the callback and process the new object. The method also sends the starting position and rotation to the client.

### OnMessage

The other important method in the servershell is the **IServerShell::OnMessage **method. This method will receive messages from the attached clients. This is critical for updating the client's positions, for processing their commands and for having an interactive game. The **CLTServerShell::OnMessage **method only handles the movement of the client object in this case.

### Objects

The server shell also includes the code for all of the objects in your game. By placing the code for an object on the server, the engine will automatically update all connected clients about that object’s built-in behaviors and properties. You can also send any custom data for each object using the engine’s messaging system. These objects also allow level designers to place them in the world and to customize each object's properties. For more information on creating objects, see [Simulation Objects ](../SimObj/mSimObj.md).

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Prog\SampleB.md)2006, All Rights Reserved.
