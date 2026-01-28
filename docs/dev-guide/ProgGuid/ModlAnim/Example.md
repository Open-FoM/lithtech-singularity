| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# Animation Code Example

The following code sample shows how to set up multi-tracked animation on an object.

>
```
HOBJECT hModelObject ;
```

```
HMODELANIM hPrimeAnim, hSecondAnim
```

```
HMODELWEIGHTSET hSet1, hSet2;
```

```
hPrimeAnim = g_pLTServer->GetAnimIndex(hModelObject, "aimup");
```

```
if (hPrimeAnim != INVALID_MODEL_ANIM)
```

```
{
```

```
                     //get the anim dims
```

```
if (g_pLTSCommon->GetModelAnimUserDims(hModelObject, &vDims, hPrimeAnim) == LT_OK)
```

```
                     {
```

```
                           //set the object dims
```

```
                           g_pLTServer->Physics()->SetDims(hModelObject, &vDims);
```

```
                     }

```

```
                     //initialize the upper half of the model
```

```
                     pModel->FindWeightSet(hModelObject, "upperbody", hSet1);
```

```
                     pModel->SetWeightSet(hModelObject, MAIN_TRACKER, hSet1);
```

```
 
```

```
                     pModel->SetCurAnim(hModelObject, MAIN_TRACKER,hPrimeAnim);
```

```
                     pModel->SetLooping(hModelObject, MAIN_TRACKER, LTTRUE);
```

```
              }

```

```
              //setup the second animation
```

```
              hSecondAnim = g_pLTServer->GetAnimIndex(hModelObject, "run");
```

```
              if (hSecondAnim != INVALID_MODEL_ANIM)
```

```
              {
```

```
                     //add another tracker and setup the lower body
```

```
                     pModel->AddTracker(hModelObject, SECONDARY_TRACKER);
```

```
 
```

```
                     pModel->FindWeightSet(hModelObject, "lowerbody", hSet2);
```

```
                     pModel->SetWeightSet(hModelObject, SECONDARY_TRACKER, hSet2);
```

```
                     pModel->SetCurAnim(hModelObject, SECONDARY_TRACKER, hSecondAnim);
```

```
                     pModel->SetLooping(hModelObject, SECONDARY_TRACKER, LTTRUE);
```

```
 
```

```
                     //set the animation rate
```

```
                     pModel->SetAnimRate(hModelObject, SECONDARY_TRACKER, 0.1f);
```

```
}     
```

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\ModlAnim\Example.md)2006, All Rights Reserved.
