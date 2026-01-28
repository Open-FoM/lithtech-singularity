**3D Studio MAX World Exporter **



EMBED Photoshop.Image.5 \s



**Pivot Point to Origin **– When checked, objects will be placed at the origin when loaded into DEdit. Normally objects will export with the same world-space location they had in MAX.



**Use MAX Texture Information **– When checked, textures assignments within MAX will be retained in DEdit. Texture paths in .lta files are relative, so the user needs to specify the part of the MAX texture files which needs to be stripped off from the texture path in order to make it relative. The exporter automatically handles changing the file extension to .dtx.



**Export Objects As Brushes **– When this option isn’t checked, objects are exported as terrain, along with the associated ‘Terrain’ object. Be sure to have only one ‘Terrain’ object in the DEdit world if you are importing an exported .lta into an existing world. If this option is checked, the MAX objects are exported as brushes, and the following options apply:



**Triangulate Brushes **– Each MAX object will still be one DEdit brush, but the brush will be made up of triangles rather than n-gons.



**Export Animated World Models **– When checked, animated MAX objects will be exported as animated WorldModels. KeyFramer and Key objects will be placed as needed in the .lta file. Any object with set keys in MAX will become it’s own WorldModel, along with it’s children. If any of the children are also animated, they will become separate WorldModels, and so on. Time settings in MAX will be used for determining the time values for Key objects in the .lta file.



**Force Keys Every N Frames **– When this isn’t checked, only keyframes set in MAX will have Key objects created. This often will result in interpolation errors in the game. To get around this, the user can force a Key object to be created at specified intervals.



**Include Defined Keys **– If the user has specified that keys should be forced at specific intervals, user-defined keys in MAX won’t be exported unless this option is checked. This should be checked if there are any critical keys that must be exported in order for the animation to look correctly.
