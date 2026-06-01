# Agent Control Surface (`ltjs_lib_agent`)

An integral part of the engine that lets a coding agent (e.g. Claude Code)
**drive and inspect a running game over MCP** — for implementing and smoke-testing
engine/game features. It is **always compiled in**; whether it *runs* is a runtime
decision — pass `--mcp-port <n>` (no flag → the server never starts).

## Architecture

```
Claude Code ──MCP/stdio──▶ tools/mcp_bridge (Node) ──TCP JSON──▶ engine (singularity)
                                                                  transport thread
                                                                  → AgentCommandQueue
                                                                  → main thread DrainFrame
                                                                  → AgentTool handler
```

- **Engine-free core** (`agent_tool.h`, `agent_registry`, `agent_queue`, `agent_protocol`,
  `agent_tcp_server`, `agent_control`): tool registry + a main-thread command queue + a
  localhost TCP server speaking newline-delimited JSON. Fully unit-tested (`tests/`).
- **Main-thread safety**: the TCP server runs on its own thread and only ever enqueues.
  Handlers run inside `agent_control_PumpFrame()`, called once per frame from
  `CClientMgr::Update` — so they may safely touch engine state.
- **Engine-coupled glue** lives in the client module (`client/src/agent_engine_tools.cpp`):
  the console/cvar tools (reach client globals) and the `--mcp-port` config resolver
  (reads the command line via `ICommandLineArgs`).
- **The sidecar** (`tools/mcp_bridge`) translates MCP↔TCP and discovers tools dynamically
  via `__list_tools`, so game-registered tools appear with no sidecar changes.

## Runtime gating

The server starts only when a port is requested, resolved in this order:

1. `--mcp-port <n>` command-line flag (optionally `--mcp-token <t>` for auth)
2. `LTJS_AGENT_PORT` / `LTJS_AGENT_TOKEN` environment variables

No port → the server never binds and `PumpFrame()` is a cheap per-frame no-op.

## Built-in tools

| Tool | Purpose |
|------|---------|
| `ping` | Liveness check. |
| `__list_tools` | Enumerate all registered tools + parameter schema. |
| `console_exec` | Run a client console command (output capture best-effort). |
| `get_cvar` / `set_cvar` | Read / write a console variable. |

Planned next increment: `query_objects` / `get_object` (object introspection) and
`capture_screenshot` (Diligent backbuffer readback — the `diligent_MakeScreenShot` stub is
the entry point).

## Extending from game code

```cpp
#include "agent_registry.h"
using namespace ltjs::agent;

AgentTool t;
t.name = "spawn_player_at";
t.description = "Spawn a Player at a world position.";
t.params = {{"x","number","X"},{"y","number","Y"},{"z","number","Z"}};
t.handler = [](const AgentRequest& req) -> AgentResponse {
  // runs on the main thread — engine calls are safe here
  return AgentResponse::Ok();
};
agent_AddTool(std::move(t));   // register before the server starts
```

## Build & run

```bash
cmake --preset engine && cmake --build --preset engine
# run with the control surface enabled:
./bin/singularity --mcp-port 27182        # (plus the game's usual args)
# it logs: [agent] control surface listening on 127.0.0.1:27182
# smoke test the raw channel:
echo '{"id":"1","tool":"ping","params":{}}' | nc 127.0.0.1 27182

# wire up the MCP bridge:
cd tools/mcp_bridge && npm install
# then add to .mcp.json:
#   { "mcpServers": { "ltjs": {
#       "command": "node", "args": ["tools/mcp_bridge/src/bridge.mjs","--port","27182"] } } }
```

## Tests

```bash
# standalone (no engine/Diligent build), reusing a populated googletest tree:
cmake -S engine/runtime/agent/tests -B build/agent-tests -G Ninja \
  -DLTJS_GTEST_SRC="$PWD/build/dedit2-tests/_deps/googletest-src"
cmake --build build/agent-tests && ctest --test-dir build/agent-tests --output-on-failure
# or, in the main build: configure with -DLTJS_BUILD_TESTS=ON (builds ltjs_agent_core_tests too)
```
