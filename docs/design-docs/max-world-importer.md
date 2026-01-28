**3D Studio MAX World Importer **



EMBED Photoshop.Image.5 \s



**Scale **– Imported brushes and objects will be scaled by this amount. Normally one DEdit unit is one MAX unit.



**Import Objects **– When checked, objects in the .lta file will be imported as Dummy objects in MAX. Only their position is correct, and no object information will be exported by the exporter. These dummy objects should only be used for reference.



**Use LithTech Texture Information **– When checked, texture assignments from the .lta file will be used to create Standard materials (and multi-subobject materials) within MAX. These materials are automatically assigned to faces as needed. Since texture paths are stored relative to the game project directory in .lta files, the user needs to specify a texture path that will be added to the beginning of the relative paths in order to get an absolute texture path. .dtx extensions are automatically converted to .tgas.



Hierarchy in the DEdit scene will be recreated within MAX. Containers will become groups.
