# Lith Kernel TODO

Track Linux/POSIX kernel gaps introduced in recent platform shims/stubs.

## Linux/POSIX kernel stubs to implement
- Music driver: implement non-Windows backend or gate usage (currently disables music). `engine/runtime/kernel/src/sys/linux/musicdriver.cpp`
- dsys resource module load/unload: implement real resource module handling. `engine/runtime/kernel/src/sys/linux/linuxdsys.cpp`
- dsys error/message handling: implement `dsi_SetupMessage` non-SDL path and `dsi_DoErrorMessage`. `engine/runtime/kernel/src/sys/linux/linuxdsys.cpp`
- dsys renderer integration: implement `dsi_GetRenderModes`, `dsi_GetRenderMode`, `dsi_SetRenderMode`, `dsi_ShutdownRender`. `engine/runtime/kernel/src/sys/linux/linuxdsys.cpp`
- dsys input queue: implement key down/up tracking and clear methods. `engine/runtime/kernel/src/sys/linux/linuxdsys.cpp`
- dsys client defaults: implement `dsi_GetDefaultWorld`, `dsi_GetInstanceHandle`, `dsi_GetMainWindow`. `engine/runtime/kernel/src/sys/linux/linuxdsys.cpp`
- Input manager shim: replace Windows-header include with Linux implementation. `engine/runtime/kernel/src/sys/linux/input.h`
- Video manager shim: replace Windows-header include with Linux implementation. `engine/runtime/kernel/src/sys/linux/videomgr.h`

## Client hook regressions to fix
- Restore `cres_hinstance`/`cresl_hinstance` engine hook handling for cursor resources. `engine/runtime/client/src/clientde_impl_sys.cpp`
## Client hook removals (D3D path retired)
- `d3ddevice` engine hook removed with D3D9 path; no longer supported. `engine/runtime/client/src/clientde_impl_sys.cpp`
