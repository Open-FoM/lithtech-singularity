---
name: lithtech-game-mcp
description: Use when implementing or smoke-testing a change to the LithTech "singularity" engine (or a game built on it) and you want to verify it against the RUNNING game instead of guessing from static code. Drives and inspects the live game over the in-engine "ltjs" MCP control surface — launch the game, run console commands, read/set cvars, list and look up game objects, capture screenshots, and inject keyboard/mouse input. Triggers: "run the game", "run renderdemo", "test/verify this in-game", "does this render/work", "screenshot the game", "drive/move in the game", "spawn an object", "is the MCP up".
---

# Driving the LithTech game via the `ltjs` MCP

The engine ships an in-engine **agent control surface** exposed over MCP as the
`ltjs` server (`engine/runtime/agent/`). It lets you observe and drive the
*running* game; handlers run on the engine main thread, so they can safely touch
engine state. This is the fastest way to confirm an engine change actually
behaves — launch, change state, look, verify — rather than reasoning only from
source. The surface is always compiled in and gated at runtime by `--mcp-port`.

## 1. Build + launch a game with the MCP on

The repo ships **renderdemo** as a self-contained test game. From the repo root:

```bash
cmake --preset engine -DLTJS_BUILD_RENDERDEMO=ON
cmake --build --preset engine            # engine; agent surface is always built in
cmake --build build/engine --target renderdemo_cshell renderdemo_object renderdemo_cres renderdemo_sres
# launch renderdemo with the control surface listening:
./bin/singularity -workingdir game/renderdemo/bin -rez game/renderdemo/rez --mcp-port 27182
```

- The engine logs `[agent] control surface listening on 127.0.0.1:27182` once up.
- A game needs `-workingdir` + `-rez` to mount resources (otherwise `StartClient`
  fails — the surface still attaches in diagnostic mode; see Gotchas).
- **Requires a real GUI / window-server session** — launch from a normal terminal
  (the game opens a window); it will not run from a headless/sandboxed shell.
- It runs a frame loop, so run it in the background and stop it when done:
  `pkill -f bin/singularity`.

## 2. Call the tools

- **Via MCP (preferred):** the `ltjs` server in `.mcp.json` exposes every tool
  directly. One-time: `cd tools/mcp_bridge && npm install` (Node ≥ 18). Use
  `__list_tools` to see the live set (including any the game registered).
- **Raw (no MCP client):** newline-delimited JSON over TCP. One request per line
  `{"id","tool","params"}`, one reply `{"id","result"}` or `{"id","error"}`:
  ```bash
  echo '{"id":"1","tool":"ping","params":{}}' | nc 127.0.0.1 27182
  ```

## 3. Tools

| Tool | Params | Use |
|------|--------|-----|
| `ping` | — | Liveness check (`{pong:true}`). |
| `__list_tools` | — | List all tools + their schema (incl. game-registered ones). |
| `console_exec` | `command` | Run a client console command (output capture is best-effort). |
| `get_cvar` / `set_cvar` | `name` (and `value`) | Read / write a console variable. |
| `query_objects` | `source` (`client`\|`server`), `type`, `limit` | List live objects with pos/dims/radius/flags; `source:server` also gives name+class (host/single-player only). |
| `get_object` | `name` | Look up a named server object → type/class/pos/dims (host only). |
| `capture_screenshot` | `path` (optional) | Write a PNG of the current frame; returns its path + dimensions. Open it to *see* the game. |
| `send_key` | `key` (SDL name, e.g. `W`,`Up`,`Space`), `state` (`down`\|`up`\|`tap`) | Inject a key — hold movement with `down`/`up`, fire discrete actions with `tap`. |
| `send_mouse` | `dx`, `dy` | Relative mouse motion (turn the view). |

## 4. The loop: observe → act → observe

1. **Observe** baseline: `capture_screenshot` and/or `query_objects` / `get_cvar`.
2. **Act**: `console_exec`, `set_cvar`, `send_key` / `send_mouse`, or a game tool.
3. **Observe** again and compare.

Example — confirm movement input works (camera position should change):
```
query_objects {"type":"camera"}        # note the camera pos
send_key {"key":"W","state":"down"}    # hold "forward" (held keys sustain)
…wait ~1s…
send_key {"key":"W","state":"up"}
query_objects {"type":"camera"}        # pos should differ
```

## 5. Gotchas

- **`source:server` / `get_object` need an in-process server.** In host /
  single-player (e.g. renderdemo) the server runs in-process, so server objects
  carry names/classes — e.g. renderdemo exposes `GameStartPoint0`,
  `WorldProperties0`, `MainWorld`. A pure network client has no in-process server.
- `capture_screenshot` needs the renderer up; it fails cleanly otherwise.
- The surface **also attaches when the game fails to boot** (diagnostic mode):
  console/cvar tools return a clean "console not initialized" error (no crash),
  `query_objects` returns an empty set — useful for diagnosing boot failures.
- `send_key` `down`/`up` hold a key across frames; `tap` is a press+release.
- Tool calls are marshalled onto the main thread (drained once per frame).

## 6. Extending from game code

New tools register into the same surface and run on the main thread:
```cpp
#include "agent_registry.h"
using namespace ltjs::agent;
agent_AddTool({ .name = "spawn_thing", .description = "...",
                .params = {{"x","number","..."}},
                .handler = [](const AgentRequest& req) -> AgentResponse {
                  // touch engine state freely; return Ok(json) or Err("...")
                  return AgentResponse::Ok();
                } });
```
Architecture + full details: `engine/runtime/agent/README.md`.
