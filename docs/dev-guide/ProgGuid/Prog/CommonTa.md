| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Common Tasks

When you develop an application there are certain common tasks that need to be conducted. This section discusses the following common tasks:

- [Camera Creation ](#cameracreation)
- [Player Creation ](#playercreation)
- [World Loading ](#WorldLoading)

---

## Camera Creation

If you want to see anything other than flat menus in Jupiter, you must define a camera class with which you can instantiate a camera object. Camera objects provide views into your worlds and generally reside on the client.

| **Note: ** | Jupiter game code defines a camera class in the \Game\ClientShellDLL\PlayerCamera.h header file. You can use and modify this class as a starting point for your own camera. |
| --- | --- |



Usually, you will attach the camera object to a player object. There probably is no reason the server needs to know about the camera object. Instead, the server will monitor and be aware of the player object. Therefore, you do not need to derive the camera class from the **BaseClass **class, which is the common practice for creating simulation objects that are monitored by the server. This means you need to create a client-side object for the camera object.

Your client-side camera object still needs an associated engine object to keep track of position and rotation and other information. You can create a client-side camera object using the **ObjectCreateStruct **structure and the **ILTClient::CreateObject **function to retrieve a handle to an associated engine object.

When defining camera classes or using the camera class provided with Jupiter, you should consider the following:

- Since you are only creating a client-side object, you need only define the camera object in the Client Shell project.
- You must create an **ObjectCreateStruct **and set the **m_ObjectType **member variable to **OT_CAMERA **.
- You must call **ILTClient::CreateObject **and pass in the address of your **ObjectCreateStruct **structure. This function returns a handle to the associated engine object.
- Your camera class should include a member variable to store the handle ( **HLOCALOBJ **) to the associated engine object. You will use this handle to conduct various engine operations on the camera, such as setting rotation, position, and field of view.
- A good place to instantiate the camera object is in constructor of your implementation of the **IClientShell **interface.
- A good place to initialize your camera object with starting position and rotation data is in the **IClientShell::OnEngineInitialized **function.
- You may want to include a camera state member variable. An enumerated type with elements such as **CAMERASTATE_FIRST_PERSON **and **CAMERASTATE_THIRD_PERSON **could prove useful in keeping track of the camera view.
- Your camera object may need to know about various events. Therefore, you should consider implementing some event notification functions, such as **Update **, **OnCommandOn **, and **OnCommandOff **.
- The camera class’s **Update **function is a good place to reposition the camera for player movement, if the camera is attached to a player object. You should retrieve a pointer to the player object and convert that to a handle to the associated engine object using the **ILTClient::ObjectToHandle **function. You can then use the **ILTPhysics::GetPosition **parameter to retrieve the player’s position and then reposition the camera object using the **ILTPhysics::SetPosition **function. You can repeat this process with **ILTPhysics::GetRotation **and **SetRotation **.
- You can use the **OnCommandOn **event to reset or center the camera view, or enable other view options such as zoom.

[Top ](#top)

---

## Player Creation

A player is the focal point of the game. A player object represents the application user in the world. Input from the user is often applied to the player object. A camera is often attached to the player object to give the user a view of the world.

A player object must obviously exist on the Client Shell, so the client can move it around and be aware of its existence. A player object usually must also exist on the Server Shell, so that the server can monitor it and distribute movement and other information to all clients. You must define both the client player object and the server player object.

#### To create player objects for a single-player game

1. Game code sends a **StartGameRequest **instance to Jupiter using the **ILTClient::StartGame **function.
2. Jupiter calls your implementation of the **IServerShell::OnClientEnterWorld **function.
3. Inside the implementation of the **IServerShell::OnClientEnterWorld **function, a server player object is instantiated.
4. When the server player object is instantiated by the game code, Jupiter also instantiates an associated client player object.
5. The client player object notifies the Client Shell that it exists.

>

| **Note: ** | For examples and starting points for player objects see the \Game\ObjectDLL\PlayerObj.h header file. |
| --- | --- |

When defining player classes or using the player classes provided with Jupiter, you should consider the following issues concerning the Client Shell and Server Shell player classes.

### Server Shell Player Class

- The Server Shell player class must derive from **BaseClass **. This class is defined in the iltengineobjects.h header file.
- You must define a **BaseClass::EngineMessageFn **function for the player class. This engine sends messages to the game object using this function. Your implementation of **EngineMessageFn **should handle the **MID_ **messages and pass them along to the parent **BaseClass::EngineMessageFn **function. Player objects should support at least **MID_PRECREATE **, **MID_OBJECTCREATED **, and **MID_UPDATE **.
- You must define a **BaseClass::ObjectMessageFn **function for the player class. This function receives messages from other objects. You should pass any messages along to the parent **BaseClass::ObjectMessageFn **function.
- You must register the player object class with the distributed object system using the **distr_class **macro in the class definition.
- The Server Shell player object must be able to identify the client connection. Therefore, your Server Shell player class should include a member variable containing an **HCLIENT **handle to the client.
- You should instantiate the Server Shell player object in the **OnClientEnterWorld **function of your implementation of the **IServerShell **interface.

### Client Shell Player Class

- The Client Shell player class must derive from the **BaseClassClient **class. Among other things, this parent class provides your player object class with distributed object system functionality. This class is defined in the ltengineobjects.h header file.
- Deriving your client-side player class from **BaseClassClient **also gives your player class a member variable, **m_hObject **. This is an **HOBJECT **handle to the associated engine object which you can use to access and modify position, rotation, and other aspects of the player object.
- You must register the player object class with the distributed object system using the **distr_class **macro in the class definition.
- Your player class should include a member variable to store a pointer to the camera object attached to the player object. You can use this pointer to adjust the camera as needed.
- When Jupiter instantiates the client player object, it does not automatically inform your **IClientShell **implementation of the existence of the new object. Therefore, in your client player class’s constructor, you should give a pointer to the new player object to your **IClientShell **instance.
- You may want to include a player state member variable. An enumerated type with elements such as **PLAYERSTATE_WALK **, **PLAYERSTATE_RUN **, **PLAYERSTATE_HOOSIER **, and **PLAYERSTATE_DEAD **could prove useful in keeping track of the player object.

Your player object may need to know about various events. Therefore, you should consider implementing some event notification functions, such as **Update **, **OnCommandOn **, and **OnCommandOff **.

### Projected Shadows

To enable projected shadows for a model requires the following tasks be completed by the programmer and the model artist:

- In ModelEdit, set the **User Dim **for the model animations.
- In ModelEdit, set **ShadowEnable **in the **Model Command String **. To override this, set **DrawAllModelShadows **= 1.
- In ModelEdit, the animation dims on the object must be set to a value that surrounds the object.
- In DEdit, the lights must be enabled to cast shadows.
- In the game code, set the console variable **ModelShadow_Proj_Enable **= 1.
- The **FLAG_SHADOW **flag must be set in the model’s **m_Flag **member variable.

[Top ](#top)

---

## World Loading

Jupiter provides world loading code in the \Game\ClientShellDLL\GameClientShell.cpp file. Worlds are loaded in the following manner:

1. Create a **StartGameRequest **class instance.
2. Populate the **StartGameRequest **members.
3. Call the **ILTClient::StartGame **function.

### Create a StartGameRequest Class Instance

The **StartGameRequest **class is defined in the \Engine\sdk\inc\LTBaseDefs.h header file. You must create an instance of this class for use with the **ILTClient::StartGame **function.

### Populate the StartGameRequest Member Variables

The **StartGameRequest **class contains several member variables that describe the type of game to be started. Only the **m_Type **and **m_WorldName **member variables need to be set for simple applications that do not involve hosting or networking operations.

**m_Type **—The type of game to start. Set this variable to **STARTGAME_NORMAL **to start a game without networking capability.

**m_WorldName **—A string identifying the relative path (from the application’s resource directory) to the file containing the world.

### Call ILTClient::StartGame

To start the game and load the world, call the **ILTClient::StartGame **function and pass in the address of the **StartGameRequest **instance you just created and populated. The **StartGame **function is defined in the ILTClient.h header file.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\Prog\CommonTa.md)2006, All Rights Reserved.
