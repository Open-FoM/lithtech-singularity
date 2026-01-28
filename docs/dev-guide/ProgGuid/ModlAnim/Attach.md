| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Attachments and Sockets

An attachment is another simulation object connected to a model. Any object can be an attachment. For instance, a sword or gun model can be attached to the hand of a character model.

The point at which an attachment joins a model is called a socket. Sockets are added, edited, and named using the ModelEdit tool.

To programmatically attach something to a model, use the **ILTServer::CreateAttachment **function:

>
```
LTRESULT (*CreateAttachment)(
```

```
       HOBJECT hParent,
```

```
       HOBJECT hChild,
```

```
       char *pSocketName,
```

```
       LTVector *pOffset,
```

```
       LTRotation *pRotationOffset,
```

```
       HATTACHMENT *pAttachment);
```

The **CreateAttachment **function accepts several parameters. The **hParent **and **hChild **parameters identify the model and the object to use as the attachment, respectively. The **pSocketName **parameter identifies the socket (previously created and named in ModelEdit) at which to join the attachment to the model. The **pOffset **and **pRotationOffset **parameters modify the position and rotation of the attachment relative to the socket. The **pAttachment **parameter returns a handle to the newly created attachment.

The **ILTServer::RemoveAttachment **function removes a given attachment. When deleting an object that is attached to model the attachment should be removed first. If the child object is deleted without removing the attachment first, the results are undefined and game code should handle the error.

If a parent model is deleted, LithTech automatically removes all attachments for that model. However, the child objects are not automatically deleted. That is, if the parent model is deleted, the child objects still exist and are rendered.

If the parent model is removed using the **RemoveAttachment **function, the child objects are not automatically deleted.

The **ILTServer::FindAttachment **function returns an attachment handle given a model and an object.

You can also retrieve the position and rotation of any socket at any given point during an animation by calling the **ILTModel::GetSocketTransform **function. This is useful for locating a socket on the end of a gun to know exactly where to create the end of a laser beam, or where to make a rocket appear from the muzzle of a rocket launcher.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\ModlAnim\Attach.md)2006, All Rights Reserved.
