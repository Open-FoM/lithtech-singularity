| ### Content Guide | [ ![](../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Organizing World Components

This section contains information about organizing the components you create for your world in DEdit. This section contains the following organization topics:

- [About Organizing Components ](#AboutOrganizingComponents)
- [Naming and Renaming Objects ](#NamingandRenamingObjects)
- [Names for Groups of Objects ](#NamesforGroupsofObjects)
- [Using Containers to Organize a Level ](#UsingContainerstoOrganizeaLevel)
- [Context Menu Commands ](#ContextMenuCommands)

---

## About Organizing Components

As your levels grow larger and more complex, it becomes increasingly important to have ways of grouping areas of your map into subsections for easier selection and management. The Nodes tab is designed to help you do this, as well as to manage doors, windows, and other objects that are composed of a brush bound to a simulation object, rather than a simulation object or brush on its own.

| **Note: ** | Binding brushes to objects is discussed in more detail in the [Adding Simulation Objects ](SimObj.md#BindingSimulationObjectstoBrushes)section. |
| --- | --- |

Your level may also have groups of simulation objects like AI paths or scripting paths that would benefit from being in groups so that you can easily find them.

The Nodes tab is a tool for organizing your level, for viewing its structure and for quickly locating objects in it. As mentioned previously in our section about selection, the nodes view reflects objects you select in the other views and vice versa. If you select a brush or object in one of the viewports, it becomes selected in the Nodes view. If you select a node, it will be highlighted in the viewports as well.

Go to the Nodes tab and look around at your level. What you’ll see is that the tab lays out your level not as it’s visually organized, but as a tree, showing the connections and relationships between objects in the level. At the very top of the tree is the root node. This object has no properties, and selecting it selects all of the child nodes below it, but doesn’t actually select the root. Selecting a node that contains other nodes always selects the contained nodes, but usually it also selects the parent node as well. In this case, the root node is special. You can’t delete, change, move or rename the root node, so there is no reason to select it other than as a quick way to select all objects in the level.

You’ll also see that there are multiple levels in the tree of nodes. Some nodes are hanging off of nodes other than the root node. Most of the objects that contain other nodes probably have the name Container. Container objects are special. When an object is contained as a child node below another object, it inherits the parent’s properties. This is called binding and will be talked about in the section on objects. The reason that containers don’t affect the objects that they hold is because Containers don’t have any properties. They only exist to hold and organize the other objects in your level.

As mentioned before, the Nodes tab also shows the relationship between the objects in the level, which can be very important in the case of objects.

[Top ](#top)

---

## Naming and Renaming Objects

Many nodes have somewhat cryptic names like Container or Brush1. That’s easily fixed, though. By default, DEdit creates object names based on the class of the object. When a new brush is added to DEdit, its name is automatically set to Brush#, where # is a unique number. DEdit tries to give each object a unique name to make each one easier to find.

If you don’t like the names DEdit gives to objects or want to change them for other reasons, you can change the name of a node just like you’d change it in Windows Explorer.

1. Click on one of the node names.
2. Press F2 or single-click once again on the node’s name. You’ll see the name become highlighted, just like a file or folder in Windows.
3. You can now type a new name.
4. When you press ENTER the new name is applied.

[Top ](#top)

---

## Names for Groups of Objects

Since the regular viewports are linked to the Nodes view, you can also rename objects in their Properties dialog, which is useful when you want a group of objects to have the same name.

1. Open your Simple1.LTA file.
2. Using the Perspective view, select all of the brushes you used to seal your level, the six brushes that make up the level’s outside walls.
3. Select the Properties tab and look at the Name field. Though you only see a single entry there, if you change the Name field here, the names of all selected objects will change.
4. Type in Outer Wall in the Name field and then switch to the Nodes tab.

You should see six nodes, all of which have the same name: Outer Wall. You can also see how the Nodes tab indicates that an object is selected. The little box next to the object’s name is checked. If you look at the other objects that are visible but not selected, you’ll see that their checkboxes are empty. If you click on a checkbox, it becomes checked, indicating that you’ve just selected the object. Likewise, you can unselect an object by clearing its checkbox.

You’ll also probably notice that our six outer wall brushes are inside a Container node. Since you didn’t know where the brushes were in the Nodes view until you selected them in the Viewport, you couldn’t take advantage of that container to quickly select the brushes. However, now that you do know where it is, click on the Container node and rename it to Outer Walls to make it easier to find.

The reason that these brushes are inside a container is because you used the Hollow command to create them. Certain operations like hollowing, carving and splitting that take a single brush and turn it into multiple brushes automatically create a container and put the resulting brushes into it to keep them organized.

[Top ](#top)

---

## Using Containers to Organize a Level

Using containers to organize your level is a very good idea. Imagine a level with 200 brushes, 50 lights and 75 enemies and you can imagine why. Using containers to group brushes by room, lights by area and enemies by squad turns your level from a massive jumble into a logical order.

| **Note: ** | Now that you know about container nodes and renaming things, you may be tempted to carefully name each brush and object, or to build six layers of containers for all of your items. Don’t spend more time than necessary on organizing things. It’s wise to sort out your level in a general way to make things easy to find and to help others who may need to work on your level. However, it’s easy to go overboard as well. Spending hours laying out your level in the Nodes tab is as much of a waste as the hours you may spend searching through an unorganized level to find a brush or simulation object. Be careful to find a balance. |
| --- | --- |

You add a container of your own from the Context menu in the nodes view.

1. Right-click on the Root node.
2. The top item on the context menu is Add Container Node. Select it and you should see a new container node appear in the tree.

You can have as many container nodes as you want in your level. Since containers are removed from the level when it’s processed, they have no effect on the actual operation of the level in the game. They’re present exclusively to help you structure your level within the editor.

[Top ](#top)

---

## Context Menu Commands

The Context menu has many useful commands on it. However, there are several that deserve to be highlighted.

### Set Active Parent

>

The active parent node is the node where new and pasted objects are added. By default when the level is opened, the active parent node is always the root node. Thus, each brush, simulation object and container you add is added directly below the root node.

This command allows you to make another container node into the active parent. Thus, if you’re creating a new room you can create a new container node and designate it as the active parent to have all new brushes and objects appear below that node. This helps a great deal when you’re trying to make a level neatly without wasting a great deal of time. Instead of organizing beforehand, you can organize as you go.

### Hide Commands

>

The various hide commands hide the targeted nodes in the Viewports, but you can still see them in the node view. They’re most useful for getting brushes out of the way if you want to do tough geometry work or if you’re having trouble seeing how parts fit together.

### Move Tagged Nodes Here

>

This command moves all the objects that are currently selected under whichever node you right-clicked on. This is useful for organizing things into containers quickly, since otherwise you must drag brushes under a container node individually.

### Group Selection

>

You can also use the Group Selection command to group objects under a node, but that creates a new container for the selected brush, which isn’t always what you want.

### Go To Next Tagged Node

>

This is a command you can use to easily jump between selected objects in the node list. Each time you select this item from the context menu or press F4 (the hotkey for this action), the Nodes view will scroll and highlight the next selected node. If you’ve used the Selection\Advanced menu to do an advanced selection in the level and therefore don’t know exactly what is selected, this is a quick way to jump to the objects you want. It’s also useful in cases where you can find something in the viewports but not in the nodes view. Last, if you’re just not certain what’s currently selected (or if anything actually is), you can use this command to quickly find out.

### Nodes Toggles

>

One last useful tool that the Nodes tab offers is the set of radio buttons at the top of the node list, right below the tab name. You can use these commands to toggle between viewing the names of the level’s objects and their class. This is often helpful when you need to identify nodes or find objects.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Dedit\NodeView.md)2006, All Rights Reserved.
