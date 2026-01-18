# Mac Build TODO Matrix

This matrix inventories platform-specific areas that block a mac build and
tracks the intended cross-platform approach. Items marked "Decision needed"
require discussion before implementation.

Legend:
- Status: todo | in-progress | blocked | done
- Decision needed: yes | no

| Area | Files / Paths | Current State | Existing Alternative | Proposed Action | Decision needed | Status |
| --- | --- | --- | --- | --- | --- | --- |
| Build entry points | `CMakeLists.txt`, `engine/runtime/build/*` | mac uses new `engine/runtime/build/*` targets | Win CMake targets exist | Keep winbuild for Windows; use build/ for mac | no | done |
| Sys headers (kernel) | `engine/runtime/kernel/src/sys*.h` | Hard-include `sys/win/*` | `sys/linux/*` exists | Add platform-selecting includes (use linux on mac) | no | in-progress |
| Sys headers (shared) | `engine/runtime/shared/src/sys/*.h` | Hard-include win headers | `sys/linux/linuxddstructs.h`, `linuxstdlterror.h` | Add platform-selecting includes; add missing POSIX headers | no | in-progress |
| Sys headers (net/io) | `engine/runtime/kernel/net/src/sys*.h`, `engine/runtime/kernel/io/src/sysfile.h` | Hard-include win headers | `sys/linux/*` exists | Add platform-selecting includes; wire linux for mac | no | in-progress |
| Kernel dsys | `engine/runtime/kernel/src/sys/win/*` | Win-only implementation | `sys/linux/linuxdsys.*` is stubby | Implement mac/SDL dsys or improve linux dsys | yes (SDL vs native) | todo |
| Client sys layer | `engine/runtime/client/src/sys/win/*` | Win-only implementations (GDI, HWND) | None | Create mac/SDL versions or refactor to shared platform-neutral layer | yes | todo |
| ClientDE impl split | `engine/runtime/client/src/clientde_impl.cpp`, `engine/runtime/client/src/clientde_impl_sys.cpp` | Sys layer now platform-neutral but still split | N/A | Consider merging once legacy hooks removed; confirm desired structure | yes | todo |
| Console | `engine/runtime/client/src/sys/win/winconsole_impl.*` | Mixed Win32/SDL | SDL paths exist | Promote SDL paths to non-win, guard Win-only sections | yes | todo |
| Font/Glyph rendering | `customfontfilemgr.cpp`, `texturestringimage.cpp` | Uses Win32 GDI, AddFontResource | SDL_ttf (vendored), FreeType in repo | Replace with SDL_ttf-backed path | no | in-progress |
| Input | `engine/runtime/kernel/src/sys/win/input.cpp`, `sdl_input.cpp` | SDL input already in win path | SDL in repo | Move SDL input to platform-neutral build path | no (SDL) | todo |
| Cursor | `engine/runtime/kernel/src/sys/win/sdl_lt_cursor_impl.cpp` | SDL impl under win path | SDL in repo | Move to platform-neutral target | yes | todo |
| Sound | `engine/runtime/sound/src/sys/s_dx8` | Win-only | OpenAL (`s_oal`) | Use OpenAL on mac; exclude dx8 | no | todo |
| Video | `ltjs_ffmpeg_video_mgr_impl.*` in win sys | Win-located, likely portable | FFmpeg in repo | Move/guard for mac; confirm platform APIs used | yes | todo |
| Resources | `engine/runtime/build/ltmsg`, `.rc` files | Windows resources | None | Either stub or replace with text/JSON on mac | yes | todo |
| Renderstructs | `shared/src/sys/win/renderstruct.h` | Win-only | none | Create platform-neutral renderstruct header or alias | yes | todo |
| Perf counters | `sys/win/ddperfcountermanager.h` | Win-only | `sys/linux/linuxperfcountermanager.*` | Use linux impl on mac | yes | todo |
| Networking threads | `kernel/net/src/sys/win/win32_ltthread*` | Win-only | `sys/linux/linux_ltthread*` | Use linux impl on mac | yes | todo |
| Build source lists | `engine/runtime/build/lithtech/CMakeLists.txt` | Includes many `sys/win/*.cpp` | N/A | Split source lists: common + win + posix | yes | todo |
