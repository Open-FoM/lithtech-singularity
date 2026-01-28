**Lite Objects ******





**The Lite Object system improves performance: **

****

Every frame the engine iterates (many times) over all the objects in the world. The fewer number of objects in this list, the faster the game runs. The Lite Object system allows less objects to be in the main object list which will, in turn, provide better performance.



**The Lite Object system is for non-physical / non-graphical objects only: **



The idea behind the system is that the engine should not spend time thinking about objects that are not rendered (or interacted with). Many game objects are based on engine objects simply so they show up in DEdit so level designers can place them and/or set their properties. Often these objects are simply tied to game logic and don't correspond to a physical object in the world (e.g., AIVolume, Controller, CommandObject, etc.). These are the types of objects that slow down the game just by being in the list of engine objects. These are the types of objects that should be converted to Lite Objects.



**Lite Objects take up less memory: **

****

Since Lite Objects don't have an LTObject, they take up much less memory. Depending on the number of objects in a level this can add up to a lot of memory. For example, by changing the objects listed below over to Lite Objects, We found that there was a savings of about 1MB in some levels. Just by changing 4 objects over to use this system!



**Objects that can NOT use the Lite Object system: **



Models and WorldModels

Objects that have SpecialFX messages (i.e., any client-side component)

Objects that require TOUCH_NOTIFY (e.g., Trigger)





**Details: **



As some of you are already aware, there's a new object system on the game side of things, and they're called "lite objects". The summary is that they're game objects (ILTBaseClass*) without an engine object. This is a very good thing for objects like AIVolumes, Controllers, and CommandObjects, since they don't really want any interaction with the engine anyway. (Note that those classes and Group have already been converted over to the new system...)



Here are the main differences between a lite object and a normal object:



They're derived from GameBaseLite

m_hObject is null (and private)

They don't get calls from EngineMessageFn or any of the other normal points of entry from the engine. (See GameBaseLite.h for the full list.)

Initial update behavior is *slightly *different

They don't have any way of periodically updating. (They're either active or inactive.)

GameBaseLite has the following new member functions:

- Save - Called when saving the object

- Load - Called when loading the object

- InitialUpdate - Called after all objects have been loaded

- Update - Called for active objects once per frame

- IsActive - Returns true if the object is active

- Activate - Activates the object

- Deactivate - De-activates the object

- Get/SetClass - Access to the HCLASS of the object. (Note : SetClass is generally only used internally and by the LiteObjectMgr.)

- Get/SetName - Access to the name of the object. (Note : SetName is generally only used internally and by the LiteObjectMgr.)

- Get/SetSerializeID - There's really no reason to be calling this function

- OnLinkBroken - For ILTObjRefReceiver support.

- ReadProp - Called by GameBaseLite as part of the creation process.

- OnTrigger - Similar to OnTrigger in GameBase

Having more than one object with the same name is very bad mojo. No memory will leak, and the object will still be created, but CLiteObjectMgr::FindObjectByName may not return what you expect. (This includes the possibility of returning null, if two objects were created with the same name, and the one the name map is pointing to gets deleted.)

You can query them by class from the lite object mgr by calling CLiteObjectMgr::GetObjectsOfClass.

They must be removed via CLiteObjectMgr::RemoveObject, instead of ILTCSBase::RemoveObject

Since there's no deletion notification handling, LTObjRef (and its associated stuff) is not useable with these objects. It is not generally adviseable to change objects which are going to be destroyed dynamically over to lite objects. If any other objects have pointers to the object, they will have to be notified of deletion through some other mechanism.

Activation and deactivation are no longer constant-time operations. (Currently.) They're still not hideously expensive or anything, but they do take longer the more lite objects are in the level.



Here's the normal life cycle of a lite object, when loading a level, or when the object is created manually:



Constructor gets called. Note that the constructor for GameBaseLite takes a boolean value indicating whether or not the object should be active by default. The default value on the constructor is to be active.

ReadProp gets called. This returns a boolean value. If the return value is false, the object will not be created. **Make sure to call the parent class's ReadProp, and jump out if it returns false. **(BaseClass reads the object's name, and GameBaseLite adds the object to the LiteObjectMgr.)

InitialUpdate gets called. At this point, all objects in the game have been created. The confusing bit of fine print here is that on normal objects, the MID_INITIALUPDATE message is called immediately after the object was added to the world, and MID_ALLOBJECTSCREATED was often used by the objects to call their InitialUpdate function. In this case, the behavior is effectively the same, because the object never gets added to the world, and it gets called before any objects have gotten their updates. (Unless, of course, this object was created through some means other than the world loading.) Also, note that this function is also called during the serialization process, as mentioned below. So if it’s got behavior that’s only supposed to happen during creation, and not serialization, make sure to check the parameter. **Also, make sure to call the InitialUpdate function of the parent class. ******

While the object is active, Update gets called once per frame.

The destructor gets called, either as a result of the LiteObjectMgr's RemoveObject function getting called, or the world ending.





In the case of serialization, here's what happens:



Save gets called when the object is geting saved. **Make sure to call the Save function of the parent class. ******

Later, when the object is being loaded, the constructor will be called.

Load will be called. (Note that all objects have been created at this point, but it's probably a good idea to stay within the bounds of serialization at this point...) **Make sure to call the Load function of the parent class. ******

InitialUpdate will be called with INITIALUPDATE_SAVEGAME as the parameter. (All objects will have been loaded before it gets to this function.) **Make sure to call the InitialUpdate function of the parent class. ******

Same as normal, Update, destructor, etc.



When changing an object over to a lite object, do the following things:



Change the object to be derived from GameBaseLite

Add the CF_CLASSONLY class flag to the object

Make sure it's not accessing m_hObject anywhere. (This will show up as a compile error.) Most applicable functions in the game have already been changed to take ILTBaseClass*'s instead of HOBJECT's. However, it may be possible that there's no good way to make this conversion, which may mean that you can't convert this object.

Make sure the function declarations are the same as GameBaseLite. (ReadProp/InitialUpdate/etc.)

Change calls to SetNextUpdate to Activate/Deactivate

Add in the parent class function calls. (See bold above.)

The bad news is that at this point, there may still be other objects relying on the object having an engine object. Examples are calls to GetNextObject which are looking for objects of this class, and calls to g_pLTServer->FindNamedObjects instead of ::FindNamedObject. Trigger message handling has already been converted over, and almost all of the code that does those types of things are dealing with objects that will most definitely have an HOBJECT. (Doors, players, that sort of thing...)

TEST, TEST, TEST.



When changing other objects, from here on out, keep the following in mind:



Deal with objects as ILTBaseClass*'s instead of HOBJECT's as much as possible. (Note the caveat above about lack of LTObjRef support. If the object has even the remotest possibility of being deleted while you're processing something, it should probably be referred to using an LTObjRef and not be a lite object.)

::FindNamedObject is your friend.

If you have an ILTBaseClass* (parameter to a function, or whatever), don't assume that m_hObject is valid. If your pointer is valid, and m_hObject is null, it's a lite object.

The CLiteObjectMgr interface is accessible through g_pGameServerShell->GetLiteObjectMgr()
