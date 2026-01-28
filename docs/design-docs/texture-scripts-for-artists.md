**Texture Scripts For Artists: **



**What is a Texture Script? **



A texture script is a small program that can be written by engineers in order to create a specific type of effect on textures. In the old system there were effect such as pan and warble. The scripts act in a very similar manner, but instead of using the pan effect, you would use a script, which tells the engine how to pan the texture. Each texture script has a list of parameters that it allows artists to set in order to customize behavior for a desired effect, which is again very similar to the old pan effect where a velocity would specified to control how fast it should actually pan. Texture scripts are essentially different types of effects that the engine can use to dynamically update textures and can be controlled through a set of parameters.



**What is a Texture Effect Group? **



The engine itself does not use the scripts directly. Instead scripts are contained within an object called a Texture Effect Group. The texture effect group dictates what scripts will be used, what parameters will be provided to them, and to which texture the script applies. Each effect group has a unique name as well as a series of stages. Each stage represents a script and to what it applies. To clarify here is an example: when rendering a polygon with a detail texture, there are two textures involved, the base texture and the detail texture. Each one of these textures can have a script with its own parameters applied, so the base texture could for example pan, while the detail texture warbled. The settings of the texture type, script, and parameters are called a stage. This will be clarified further in the section Creating and Editing Texture Groups. In addition, if you want one stage to act exactly like a different stage, so that it has the exact same script and parameters, you can specify that the stage is overridden, and specify which stage it should copy all the information from. When a stage is overridden, it does not have any parameters of its own, so changes to the parameters must be done to the overriding stage.



**Assigning Texture Groups in DEdit **



Texture groups can be assigned to brushes in DEdit through the TextureEffect property. Pressing the browse button on the property causes the Texture Group dialog to appear as shown below:



EMBED PBrush



From the dropdown you can select any existing effect groups, and through the buttons new effect groups can be created or existing ones edited. Pressing OK will assign the currently selected effect group to all selected brushes.



**Creating and Editing Texture Groups **



When a new effect group is created, or an existing one edited, it brings up the Edit Effect Group dialog shown below:







As can be seen, the effect group has a name, which can be edited when creating a new effect. In addition it has two stages for setting up how the effect will actually work. First stage one will be examined, which in the diagram is set up to have a warble effect played on the base texture. The channel field indicates which texture will have the script applied to it, in this case it is the Base Texture, but it can also be the Detail Texture, Environment Map, Light Map, or can also be set to Disabled if this entire stage should be ignored and nothing effected. The next drop down specifies what stage overrides this one, and in this case there is nothing overriding it, indicating that it provides its own script and parameters for controlling the effect. The script is the actual effect itself, which in the example shows a warble script selected, which will cause the texture to move around in an odd fashion based upon the parameters provided. The script name cannot be edited directly, but by pressing the button to the right it allows the user to select a script through a file dialog. Once a script is selected, it will be looked at to determine what parameters it supports. Upon finding those it will display them below the script allowing the user to enter custom values in order to customize the script for the particular instance. These parameters are dependant upon the script itself so for further information you should consult the texture script documentation.

Stage two shown below stage one is an example of modifying a different texture channel so that both can be animating. However, for this one it has been overridden to copy stage one. This means that it will use the exact same script and parameters as the first script, and if any of those parameters were to change, it would change for stage two as well. Essentially overriding allows one stage to piggyback on a different stage but apply it to a different texture channel. If this stage was not overridden though, it could provide its own effect and parameters allowing the detail texture to animate independently of its base texture.



**Adding Support for Triggers **



Texture scripts can have their parameters changed during runtime through triggers, allowing such effects as a player walking through a doorway and causing conveyer belt textures to begin panning or other such dynamic effects. In order to do this, a TextureFX object must be placed in the level. The TextureFX has a single property, which should be filled out with the name of the group that it is corresponding to. This object can then receive messages of the form:



*SETVAR (<stage> <variable> <value>) *



This will change the effect group’s parameter to the specified value. Stage represents the stage that is having the parameter change and can be either 1 or 2. Note that it cannot be a stage that is disabled or a stage that is overridden. Variable represents the index of the variable that is to be changed and corresponds to the order in which they are listed in DEdit going from left to right, top to bottom and must be from one to six. The value is the value that is to be stored at the specified location. So for example, if we had a panning script whose parameters were: DirX, DirY, DirZ, Velocity, Offset, and that was part of the effect group ConveyorBelt.tfg, we would first need to create a TextureFX object named ConveyorBelt whose parameter TextureEffect was assigned to ConveyorBelt.tfg so that it would know to modify the conveyor belt effect group. We could then set up a trigger in a doorway that would start the conveyor belt up by changing its velocity from 0 to 5. The conveyor belt uses stage one, and since velocity is the fourth parameter, our message would look like this:



*Msg ConveyorBelt SETVAR (1 4 5.0) *



**Complete Overview **

Below is a diagram that illustrates the process of creating texture effects:





EMBED Visio.Drawing.6
