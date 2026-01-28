| ### Programming Guide | [ ![](../../Images/Logo-s.png) ](http://www.touchdownentertainment.com/jupiter.md) |
| --- | --- |

# SETUP_CLIENTSHELL

**SETUP_CLIENTSHELL **is a macro definition and can be found in the IClientShell.h header file. It performs two operations:

1. It uses the define_holder macro to acquire a pointer to Jupiter’s ILTClient interface instance. After using the SETUP_CLIENTSHELL macro you can access the ILTClient functions using the g_pLTClient global pointer.
2. It sets up required DLL function exports.


```
SETUP_CLIENTSHELL();
```

[Top ](#top)

---

Touchdown Entertainment, Inc. [Send feedback regarding this page. ](mailto:support@touchdownentertainment.com?subject=JupiterDevGuide Feedback: ProgGuid\CltShell\setup.md)2006, All Rights Reserved.
