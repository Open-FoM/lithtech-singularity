**Spawner object update (03/13/02) **



The spawner object has been expanded to operate using “object templates.” These look just like any other object in DEdit, except for the fact that their “template” parameter has been set to “True”. In order to spawn in the desired object template, set the spawner’s “Target” property to the name of the object template, and send the message a “Spawn” or “SpawnFrom” message. These new messages are described below.



**Spawn [Name] **



Spawn an object based on the object template specified in the “Target” parameter. If the optional **Name **parameter is provided, the object’s name will be set to that value. Otherwise a unique name will be generated.



**SpawnFrom Object [Name] ******



Spawn an object based on the object template specified in the “Target” parameter, located at the position and rotation of the object named **Object **. If the optional **Name **parameter is provided, the object’s name will be set to that value. Otherwise a unique name will be generated.
