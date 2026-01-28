| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# SETUP_SERVERSHELL

**SETUP_SERVERSHELL **is a macro definition and can be found in iservershell.h. It performs two operations:

1. It uses the define_holder macro to acquire a pointer to Jupiter’s ILTServer interface instance. After using the **SETUP_SERVERSHELL **macro, you can access the **ILTServer **functions using the **g_pLTServer **global pointer.
2. It sets up required DLL function exports.


```
SETUP_SERVERSHELL();
```

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\SrvShell\Setup.md)2006, All Rights Reserved.
