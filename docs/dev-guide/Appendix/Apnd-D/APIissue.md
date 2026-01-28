| ### Content Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Appendix D: API Issues

The following issues may cause some abnormal behavior and should be accounted for in your game code.

- [HMODELANIM ](#HMODELANIM)
- [LT_NO and LT_YES ](#LTNOandLTYES)

---

## HMODELANIM

The data type **HMODELANIM **is not a handle, but an index. So, zero is a perfectly valid value. In cases where the engine returns an **HMODELANIM **an invalid animation is represented by **((HMODELANIM)-1). **So, don't write code that looks like:

>
```
HMODELANIM hAnim;
```

```
...
```

```
if (hAnim)
```

```
{
```

```
// This code will NOT be evaluated when hAnim is set
// to the first animation specified in the model.
```

```
       ...
```

```
}
```

[Top ](#top)

---

## LT_NO and LT_YES

Beware of engine functions that use the LTRESULT values **LT_NO **and **LT_YES **. **LT_NO **is NOT defined as zero, and **LT_YES **is NOT defined as 1. In fact **LT_YES = 86 and LT_NO = 87 **. Make sure you write code to check for the appropriate return value and do not write code that does the following:

>
```
ILTModel* pLTModel;
```

```
...
```

```
//INCORRECT:
```

```
if (pLTModel->GetLooping(hObj, nTrackerID))
```

```
{
```

```
       // This code will ALWAYS be evaluated
```

```
       ...
```

```
}
```

```
//CORRECT:
```

```
if (LT_YES == pLTModel->GetLooping(hObj, nTrackerID))
```

```
{
```

```
       // This code will only be evaluated when the model is looping...
```

```
       ...
```

```
}
```

This may cause some bugs because the API looks like it is returning a Boolean value in many of these cases. Here are the API interfaces that use these return types:

- ILTModel::GetLooping
- ILTModel::GetPlaying
- ILTModel::IsCollisionObjectsEnabled
- ILTModel::IsUpdateCollisionObjectsEnabled
- ILTModel::IsTransformCacheEnabled
- CLTCursorInst::IsValid
- CLTCursor::IsCursorModeAvailable
- CLTCursor::IsValidCursor
- ILTPhysics::IsWorldObject
- ILTPhysics::IsWorldObject

**ILTPhysics::IsWorldObject **checks to see if the passed in object is the **MAIN **world object, not just any world object. A better function to use to determine if the passed in object is the main world object is the function **IsMainWorld **, defined in CommonUtilities.h.

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: Appendix\Apnd-D\APIissue.md)2006, All Rights Reserved.
