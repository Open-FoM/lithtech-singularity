#include "bdefs.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keyboard.h"

#include "agent_engine_tools.h"

#include "concommand.h"
#include "console.h"
#include "consolecommands.h"
#include "icommandlineargs.h"

#include "clientmgr.h"
#include "de_objects.h"
#include "iltserver.h"
#include "objectmgr.h"
#include "servermgr.h"

#include "diligent_screenshot.h"

#include "agent_registry.h"
#include "agent_tool.h"

// Command-line access via the engine module holder (same pattern as client.cpp).
// The generated holder symbols have internal linkage, so a second holder for the
// same interface in this TU does not clash with the one in client.cpp.
static ICommandLineArgs *g_agent_command_line_args = nullptr;
define_holder(ICommandLineArgs, g_agent_command_line_args);

// In-process server interface (single-player/host). Used to resolve object names
// and class names. Null in client-only network mode; callers must guard.
static ILTServer *g_agent_lt_server = nullptr;
define_holder(ILTServer, g_agent_lt_server);

namespace ltjs::agent {

namespace {

// Console output capture. The engine routes console text through the error-log
// callback; we install one for the duration of a single command. Single-threaded
// (runs on the main thread inside DrainFrame), but guarded against re-entrancy
// in case a command itself invokes console output handling.
thread_local std::vector<std::string> *tls_capture = nullptr;

void CaptureConsoleLine(const char *message) {
  if (tls_capture != nullptr && message != nullptr) {
    tls_capture->emplace_back(message);
  }
}

// True once the client console state has been initialized (cc_InitState). When a
// game fails to boot before that point the console hash tables are null and
// touching them would crash, so the console tools guard with this and return a
// clean error instead (relevant in the failed-boot diagnostic loop).
bool ConsoleReady() { return g_ClientConsoleState.m_VarHash != nullptr; }

AgentResponse ConsoleExec(const AgentRequest &request) {
  if (!ConsoleReady()) {
    return AgentResponse::Err("console not initialized (game has not finished booting)");
  }
  if (!request.params.contains("command") || !request.params.at("command").is_string()) {
    return AgentResponse::Err("console_exec requires a string 'command'");
  }
  const std::string command = request.params.at("command").get<std::string>();

  if (tls_capture != nullptr) {
    return AgentResponse::Err("console_exec is not re-entrant");
  }

  std::vector<std::string> captured;
  tls_capture = &captured;
  con_SetErrorLog(&CaptureConsoleLine);
  c_CommandHandler(command.c_str());
  con_SetErrorLog(nullptr);
  tls_capture = nullptr;

  Json result;
  result["command"] = command;
  result["output"] = captured; // best-effort: not all output flows through the error log
  return AgentResponse::Ok(std::move(result));
}

AgentResponse GetCvar(const AgentRequest &request) {
  if (!ConsoleReady()) {
    return AgentResponse::Err("console not initialized (game has not finished booting)");
  }
  if (!request.params.contains("name") || !request.params.at("name").is_string()) {
    return AgentResponse::Err("get_cvar requires a string 'name'");
  }
  const std::string name = request.params.at("name").get<std::string>();

  LTCommandVar *var = cc_FindConsoleVar(&g_ClientConsoleState, name.c_str());
  if (var == nullptr) {
    return AgentResponse::Err("no such console variable: " + name);
  }

  Json result;
  result["name"] = name;
  result["float"] = var->floatVal;
  result["value"] = var->pStringVal != nullptr ? std::string(var->pStringVal) : std::string();
  return AgentResponse::Ok(std::move(result));
}

AgentResponse SetCvar(const AgentRequest &request) {
  if (!ConsoleReady()) {
    return AgentResponse::Err("console not initialized (game has not finished booting)");
  }
  if (!request.params.contains("name") || !request.params.at("name").is_string()) {
    return AgentResponse::Err("set_cvar requires a string 'name'");
  }
  const std::string name = request.params.at("name").get<std::string>();

  std::string value;
  const Json provided = request.params.contains("value") ? request.params.at("value") : Json();
  if (provided.is_string()) {
    value = provided.get<std::string>();
  } else if (provided.is_number()) {
    value = provided.dump();
  } else if (provided.is_boolean()) {
    value = provided.get<bool>() ? "1" : "0";
  } else {
    return AgentResponse::Err("set_cvar requires 'value' as a string, number or boolean");
  }

  cc_SetConsoleVariable(&g_ClientConsoleState, name.c_str(), value.c_str());

  Json result;
  result["name"] = name;
  result["value"] = value;
  result["ok"] = true;
  return AgentResponse::Ok(std::move(result));
}

const char *ObjectTypeName(uint8 type) {
  switch (type) {
  case OT_NORMAL: return "normal";
  case OT_MODEL: return "model";
  case OT_WORLDMODEL: return "worldmodel";
  case OT_SPRITE: return "sprite";
  case OT_LIGHT: return "light";
  case OT_CAMERA: return "camera";
  case OT_PARTICLESYSTEM: return "particlesystem";
  case OT_POLYGRID: return "polygrid";
  case OT_LINESYSTEM: return "linesystem";
  case OT_CONTAINER: return "container";
  case OT_CANVAS: return "canvas";
  case OT_VOLUMEEFFECT: return "volumeeffect";
  default: return "unknown";
  }
}

// Maps a type-name string to an OT_ value, or -1 if unrecognized.
int ObjectTypeFromName(const std::string &name) {
  for (uint8 type = 0; type < NUM_OBJECTTYPES; ++type) {
    if (name == ObjectTypeName(type)) {
      return static_cast<int>(type);
    }
  }
  return -1;
}

// Builds the JSON description of an object. Adds name + class when the object is
// server-backed (sd != null) and the in-process server interface is available;
// client-only objects (e.g. renderdemo ModelAdd, id 65535) carry neither.
Json ObjectToJson(LTObject *object) {
  const LTVector &pos = object->GetPos();
  const LTVector &dims = object->GetDims();
  Json obj = Json{{"type", ObjectTypeName(object->m_ObjectType)},
                  {"id", object->m_ObjectID},
                  {"pos", {pos.x, pos.y, pos.z}},
                  {"dims", {dims.x, dims.y, dims.z}},
                  {"radius", object->GetRadius()},
                  {"flags", object->m_Flags}};

  if (object->sd != nullptr && g_agent_lt_server != nullptr) {
    auto handle = static_cast<HOBJECT>(object);
    char name_buf[256] = {0};
    if (g_agent_lt_server->GetObjectName(handle, name_buf, sizeof(name_buf)) == LT_OK && name_buf[0] != '\0') {
      obj["name"] = std::string(name_buf);
    }
    if (HCLASS hclass = g_agent_lt_server->GetObjectClass(handle)) {
      char class_buf[256] = {0};
      if (g_agent_lt_server->GetClassName(hclass, class_buf, sizeof(class_buf)) == LT_OK && class_buf[0] != '\0') {
        obj["class"] = std::string(class_buf);
      }
    }
  }
  return obj;
}

// Lists live objects with position/dims/flags (plus name/class for server
// objects). `source`: "client" (default; what the client renders) or "server"
// (the in-process server's game objects, which carry names/classes).
AgentResponse QueryObjects(const AgentRequest &request) {
  std::string source = "client";
  if (request.params.contains("source") && request.params.at("source").is_string()) {
    source = request.params.at("source").get<std::string>();
  }

  ObjectMgr *object_mgr = nullptr;
  if (source == "server") {
    if (g_pServerMgr == nullptr) {
      return AgentResponse::Err("server not running (source 'server' needs single-player/host)");
    }
    object_mgr = &g_pServerMgr->m_ObjectMgr;
  } else if (source == "client") {
    if (g_pClientMgr == nullptr) {
      return AgentResponse::Err("client manager not initialized");
    }
    object_mgr = &g_pClientMgr->m_ObjectMgr;
  } else {
    return AgentResponse::Err("source must be 'client' or 'server'");
  }

  int type_filter = -1; // -1 == all types
  if (request.params.contains("type") && request.params.at("type").is_string()) {
    const std::string type_name = request.params.at("type").get<std::string>();
    type_filter = ObjectTypeFromName(type_name);
    if (type_filter < 0) {
      return AgentResponse::Err("unknown object type: " + type_name);
    }
  }

  std::size_t max_objects = 256;
  if (request.params.contains("limit") && request.params.at("limit").is_number_integer()) {
    const long long requested = request.params.at("limit").get<long long>();
    if (requested > 0) {
      max_objects = static_cast<std::size_t>(requested > 4096 ? 4096 : requested);
    }
  }

  Json objects = Json::array();
  bool truncated = false;

  for (int type = 0; type < NUM_OBJECTTYPES && !truncated; ++type) {
    if (type_filter >= 0 && type != type_filter) {
      continue;
    }
    LTList &list = object_mgr->m_ObjectLists[type];
    for (LTLink *cur = list.m_Head.m_pNext; cur != &list.m_Head; cur = cur->m_pNext) {
      if (objects.size() >= max_objects) {
        truncated = true;
        break;
      }
      auto *object = static_cast<LTObject *>(cur->m_pData);
      if (object == nullptr) {
        continue;
      }
      objects.push_back(ObjectToJson(object));
    }
  }

  Json result;
  result["source"] = source;
  result["count"] = objects.size();
  result["truncated"] = truncated;
  result["objects"] = std::move(objects);
  return AgentResponse::Ok(std::move(result));
}

// Finds a named server object and returns its details. Object names live on the
// server's game objects, so this requires a running server (single-player/host).
AgentResponse GetObject(const AgentRequest &request) {
  if (!request.params.contains("name") || !request.params.at("name").is_string()) {
    return AgentResponse::Err("get_object requires a string 'name'");
  }
  const std::string name = request.params.at("name").get<std::string>();

  if (g_pServerMgr == nullptr || g_agent_lt_server == nullptr) {
    return AgentResponse::Err("object names require a running server (single-player/host)");
  }

  ObjectMgr &object_mgr = g_pServerMgr->m_ObjectMgr;
  for (int type = 0; type < NUM_OBJECTTYPES; ++type) {
    LTList &list = object_mgr.m_ObjectLists[type];
    for (LTLink *cur = list.m_Head.m_pNext; cur != &list.m_Head; cur = cur->m_pNext) {
      auto *object = static_cast<LTObject *>(cur->m_pData);
      if (object == nullptr || object->sd == nullptr) {
        continue;
      }
      char name_buf[256] = {0};
      if (g_agent_lt_server->GetObjectName(static_cast<HOBJECT>(object), name_buf, sizeof(name_buf)) == LT_OK &&
          name == name_buf) {
        return AgentResponse::Ok(ObjectToJson(object));
      }
    }
  }
  return AgentResponse::Err("no server object named: " + name);
}

// Captures the current backbuffer to a PNG the agent can open. Runs on the main
// thread, so the Diligent immediate context is safe to use here.
AgentResponse CaptureScreenshot(const AgentRequest &request) {
  std::string path;
  if (request.params.contains("path") && request.params.at("path").is_string()) {
    path = request.params.at("path").get<std::string>();
  } else {
    std::error_code ec;
    std::filesystem::path dir = std::filesystem::temp_directory_path(ec);
    if (ec) {
      dir = std::filesystem::path(".");
    }
    path = (dir / "ltjs_agent_screenshot.png").string();
  }

  int width = 0;
  int height = 0;
  if (!diligent_CaptureBackbufferToPng(path.c_str(), &width, &height)) {
    return AgentResponse::Err("screenshot capture failed (no swapchain or write error): " + path);
  }

  Json result;
  result["path"] = path;
  result["width"] = width;
  result["height"] = height;
  return AgentResponse::Ok(std::move(result));
}

// Injects a synthetic keyboard event through the real SDL pipeline (event ->
// binding -> action), so the game sees it exactly like physical input. Pushed
// here on the main thread; the engine picks it up on the next frame's poll.
AgentResponse SendKey(const AgentRequest &request) {
  if (!request.params.contains("key") || !request.params.at("key").is_string()) {
    return AgentResponse::Err("send_key requires a string 'key' (SDL key name, e.g. 'W','Up','Space')");
  }
  const std::string key_name = request.params.at("key").get<std::string>();

  std::string state = "tap";
  if (request.params.contains("state") && request.params.at("state").is_string()) {
    state = request.params.at("state").get<std::string>();
  }
  if (state != "down" && state != "up" && state != "tap") {
    return AgentResponse::Err("send_key 'state' must be 'down', 'up' or 'tap'");
  }

  SDL_Keycode keycode = SDL_GetKeyFromName(key_name.c_str());
  if (keycode == SDLK_UNKNOWN && key_name.size() == 1 && std::isalpha(static_cast<unsigned char>(key_name[0]))) {
    const std::string upper(1, static_cast<char>(std::toupper(static_cast<unsigned char>(key_name[0]))));
    keycode = SDL_GetKeyFromName(upper.c_str());
  }
  if (keycode == SDLK_UNKNOWN) {
    return AgentResponse::Err("unknown key name: " + key_name +
                              " (use SDL key names, e.g. 'W','Up','Left','Space','Return','1')");
  }

  const SDL_Scancode scancode = SDL_GetScancodeFromKey(keycode, nullptr);
  const auto push = [&](bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.key = keycode;
    event.key.mod = SDL_KMOD_NONE;
    event.key.down = down;
    event.key.repeat = false;
    SDL_PushEvent(&event);
  };

  if (state == "down") {
    push(true);
  } else if (state == "up") {
    push(false);
  } else {
    push(true);
    push(false);
  }

  Json result;
  result["key"] = key_name;
  result["state"] = state;
  result["keycode"] = static_cast<std::uint32_t>(keycode);
  return AgentResponse::Ok(std::move(result));
}

// Injects relative mouse motion (for turning the view).
AgentResponse SendMouse(const AgentRequest &request) {
  float dx = 0.0f;
  float dy = 0.0f;
  if (request.params.contains("dx") && request.params.at("dx").is_number()) {
    dx = request.params.at("dx").get<float>();
  }
  if (request.params.contains("dy") && request.params.at("dy").is_number()) {
    dy = request.params.at("dy").get<float>();
  }
  if (dx == 0.0f && dy == 0.0f) {
    return AgentResponse::Err("send_mouse requires a non-zero 'dx' and/or 'dy' (relative motion)");
  }

  SDL_Event event{};
  event.type = SDL_EVENT_MOUSE_MOTION;
  event.motion.xrel = dx;
  event.motion.yrel = dy;
  SDL_PushEvent(&event);

  Json result;
  result["dx"] = dx;
  result["dy"] = dy;
  return AgentResponse::Ok(std::move(result));
}

AgentTool MakeTool(std::string name, std::string description, std::vector<AgentParamDesc> params, AgentToolFn handler) {
  AgentTool tool;
  tool.name = std::move(name);
  tool.description = std::move(description);
  tool.params = std::move(params);
  tool.handler = std::move(handler);
  return tool;
}

// Returns a non-empty argument value for the given "--name" flag (matched via
// FindArgDash, which prepends one dash to the supplied name), or nullptr.
const char *FindDoubleDashArg(const char *name_without_leading_dashes) {
  if (g_agent_command_line_args == nullptr) {
    return nullptr;
  }
  const char *value = g_agent_command_line_args->FindArgDash(name_without_leading_dashes);
  return (value != nullptr && value[0] != '\0') ? value : nullptr;
}

} // namespace

void agent_RegisterEngineTools() {
  agent_AddTool(MakeTool("console_exec", "Run a client console command. Output capture is best-effort.",
                         {{"command", "string", "The console command line to execute", true}}, &ConsoleExec));

  agent_AddTool(MakeTool("get_cvar", "Read a console variable's string and float value.",
                         {{"name", "string", "Console variable name", true}}, &GetCvar));

  agent_AddTool(MakeTool("set_cvar", "Set a console variable.",
                         {{"name", "string", "Console variable name", true},
                          {"value", "string", "New value (string, number or boolean)", true}},
                         &SetCvar));

  agent_AddTool(
      MakeTool("query_objects",
               "List live objects with position/dims/flags (plus name/class for server objects). "
               "source: 'client' (default, rendered objects) or 'server' (game objects with names).",
               {{"source", "string", "'client' (default) or 'server'", false},
                {"type", "string", "Optional type filter: model, sprite, worldmodel, light, camera, etc.", false},
                {"limit", "integer", "Max objects to return (default 256, cap 4096)", false}},
               &QueryObjects));

  agent_AddTool(MakeTool("get_object",
                         "Find a named server object and return its type/class/pos/dims (single-player/host only).",
                         {{"name", "string", "Object name to look up", true}}, &GetObject));

  agent_AddTool(MakeTool(
      "capture_screenshot", "Capture the current rendered backbuffer to a PNG file; returns its path and dimensions.",
      {{"path", "string", "Optional output PNG path (defaults to a temp file)", false}}, &CaptureScreenshot));

  agent_AddTool(
      MakeTool("send_key",
               "Inject a keyboard event (SDL key name). state: down|up|tap (default tap). Use down/up to hold "
               "movement keys, tap for discrete actions.",
               {{"key", "string", "SDL key name, e.g. 'W','S','A','D','Up','Left','Space','Return','1'", true},
                {"state", "string", "'down', 'up', or 'tap' (default 'tap')", false}},
               &SendKey));

  agent_AddTool(MakeTool(
      "send_mouse", "Inject relative mouse motion (for turning the view).",
      {{"dx", "number", "Relative X motion in pixels", false}, {"dy", "number", "Relative Y motion in pixels", false}},
      &SendMouse));
}

bool agent_GetMcpServerConfig(AgentControlConfig &out) {
  // Command line: "--mcp-port <n>". FindArgDash("-mcp-port") forms "--mcp-port".
  const char *port_str = FindDoubleDashArg("-mcp-port");
  const char *token_str = FindDoubleDashArg("-mcp-token");

  // Env fallbacks.
  if (port_str == nullptr) {
    const char *env_port = std::getenv("LTJS_AGENT_PORT");
    if (env_port != nullptr && env_port[0] != '\0') {
      port_str = env_port;
    }
  }
  if (token_str == nullptr) {
    const char *env_token = std::getenv("LTJS_AGENT_TOKEN");
    if (env_token != nullptr && env_token[0] != '\0') {
      token_str = env_token;
    }
  }

  if (port_str == nullptr) {
    return false; // operator did not request the control surface
  }

  const int port = std::atoi(port_str);
  if (port <= 0 || port > 65535) {
    return false;
  }

  out.port = static_cast<std::uint16_t>(port);
  if (token_str != nullptr) {
    out.auth_token = token_str;
  }
  return true;
}

} // namespace ltjs::agent
