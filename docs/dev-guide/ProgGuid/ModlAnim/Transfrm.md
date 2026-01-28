| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Transformations

A transformation is a position and rotation. Artists create transformations in a third-party editor such as Maya or 3D Studio Max at the same time that they create a model.

In relation to models, transformations can apply to sockets, nodes, and attachments. A transformation is represented in code by the **LTTransform **class, defined in the ltbasetypes.h header file.

>
```
class LTransform
```

```
{
```

```
public:
```

```
       LTVector      m_Pos;
```

```
       LTRotation    m_Rot;
```

```
       LTVector      m_Scale;
```

```
};
```

The **ILTModel **interface, defined in the iltmodel.h header file, provides the **GetSocketTransform **and **GetNodeTransform **functions to retrieve the transformations for sockets and nodes, respectively.

The **ILTTransform **interface, defined in the ilttransform.h header file, includes functions to create and manipulate transformations in a variety of different ways.

## Transformation Hierarchy and Transformation Nodes

Each model has a transformation hierarchy. A transformation hierarchy is an organization of transformations, arranged in a tree-like fashion, that define the position and orientation of various aspects of the model. Each node in the hierarchy has a transformation associated with it. Nodes can have parent nodes and child nodes. There is no limit to the number of nodes in a transformation hierarchy. If presented graphically, a transformation hierarchy would appear similar to a directory tree. Artists create the transformation hierarchy in Max or Maya along with the model.

The artist sets the number of nodes in the transformation hierarchy. Engine code cannot change the number or the arrangement of nodes in the transformation hierarchy. The transformations at the nodes can be queried and modified in game code.

**HMODELNODE **is a handle to a transformation node in the model database transform hierarchy.

To retrieve a handle to a node, use **ILTModel::GetNode **, or to access the transform hierarchy iteratively use **ILTServer::GetNextModelNode **.

Since the transform hierarchy cannot be modified, there is no set function for transformation nodes. Nodes can only be set, or created, by the artist when creating the model.

To retrieve the name of a node, use the **ILTServer::GetModelNodeName **function.

The **LTransform **instance for a transformation node can be retrieved using the **ILTModel::GetNode **Transform function.

### Modifying Animations

LithTech provides the ability to take the individual transformation nodes, and alter them on the fly per-frame. This allows some very powerful effects, but also can become a serious performance hit.

| **Note: ** | This technique only works on the client. It is useful for some effects, but it is not a good method of actual animation. |
| --- | --- |

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\ModlAnim\Transfrm.md)2006, All Rights Reserved.
