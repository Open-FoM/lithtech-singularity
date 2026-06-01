#!/usr/bin/env node
// ------------------------------------------------------------------ //
//  LTJS MCP bridge
//
//  A thin stdio MCP server that forwards tool calls to a running LTJS engine
//  over its localhost TCP control channel (newline-delimited JSON). The engine
//  owns the tool set; this bridge discovers it dynamically via __list_tools, so
//  tools registered from game code appear automatically with no changes here.
//
//  Usage:
//    node bridge.mjs [--host 127.0.0.1] [--port 27182]
//  Env overrides: LTJS_AGENT_HOST, LTJS_AGENT_PORT, LTJS_AGENT_TOKEN.
//
//  Register with Claude Code (.mcp.json):
//    { "mcpServers": { "ltjs": {
//        "command": "node",
//        "args": ["tools/mcp_bridge/src/bridge.mjs", "--port", "27182"] } } }
// ------------------------------------------------------------------ //

import net from "node:net";

import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
} from "@modelcontextprotocol/sdk/types.js";

function argValue(flag, fallback) {
  const i = process.argv.indexOf(flag);
  return i !== -1 && i + 1 < process.argv.length ? process.argv[i + 1] : fallback;
}

const HOST = argValue("--host", process.env.LTJS_AGENT_HOST || "127.0.0.1");
const PORT = parseInt(argValue("--port", process.env.LTJS_AGENT_PORT || "27182"), 10);
const TOKEN = process.env.LTJS_AGENT_TOKEN || "";

// ---- Engine TCP client: one line per request/response, correlated by id ---- //
class EngineClient {
  constructor(host, port, token) {
    this.host = host;
    this.port = port;
    this.token = token;
    this.socket = null;
    this.connecting = null;
    this.buffer = "";
    this.nextId = 1;
    this.pending = new Map();
  }

  connect() {
    if (this.socket && !this.socket.destroyed) return Promise.resolve();
    if (this.connecting) return this.connecting;

    this.connecting = new Promise((resolve, reject) => {
      const socket = net.createConnection({ host: this.host, port: this.port }, () => {
        if (this.token) socket.write(this.token + "\n");
        this.socket = socket;
        this.connecting = null;
        resolve();
      });
      socket.setEncoding("utf8");
      socket.on("data", (chunk) => this._onData(chunk));
      socket.on("error", (err) => {
        this.connecting = null;
        this._failAll(err);
        reject(err);
      });
      socket.on("close", () => {
        this.socket = null;
        this._failAll(new Error("engine connection closed"));
      });
    });
    return this.connecting;
  }

  _onData(chunk) {
    this.buffer += chunk;
    let nl;
    while ((nl = this.buffer.indexOf("\n")) !== -1) {
      const line = this.buffer.slice(0, nl);
      this.buffer = this.buffer.slice(nl + 1);
      if (!line.trim()) continue;
      let msg;
      try {
        msg = JSON.parse(line);
      } catch {
        continue;
      }
      const resolver = this.pending.get(msg.id);
      if (!resolver) continue;
      this.pending.delete(msg.id);
      resolver(msg);
    }
  }

  _failAll(err) {
    for (const [, resolver] of this.pending) resolver({ error: String(err.message || err) });
    this.pending.clear();
  }

  async call(tool, params) {
    await this.connect();
    const id = String(this.nextId++);
    const line = JSON.stringify({ id, tool, params: params || {} }) + "\n";
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(id);
        reject(new Error(`engine request '${tool}' timed out`));
      }, 10000);
      this.pending.set(id, (msg) => {
        clearTimeout(timer);
        resolve(msg);
      });
      this.socket.write(line);
    });
  }
}

const engine = new EngineClient(HOST, PORT, TOKEN);

// ---- Schema translation: engine tool descriptor -> MCP inputSchema ---------- //
function toInputSchema(params) {
  const properties = {};
  const required = [];
  for (const p of params || []) {
    const t = p.type === "integer" ? "integer"
      : p.type === "number" ? "number"
      : p.type === "boolean" ? "boolean"
      : "string";
    properties[p.name] = { type: t, description: p.description || "" };
    if (p.required) required.push(p.name);
  }
  return { type: "object", properties, required };
}

const server = new Server(
  { name: "ltjs-agent", version: "0.1.0" },
  { capabilities: { tools: {} } }
);

server.setRequestHandler(ListToolsRequestSchema, async () => {
  const reply = await engine.call("__list_tools", {});
  if (reply.error) throw new Error(reply.error);
  const tools = (reply.result || []).map((t) => ({
    name: t.name,
    description: t.description || "",
    inputSchema: toInputSchema(t.parameters),
  }));
  return { tools };
});

server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;
  const reply = await engine.call(name, args || {});
  if (reply.error) {
    return { content: [{ type: "text", text: `Error: ${reply.error}` }], isError: true };
  }
  return { content: [{ type: "text", text: JSON.stringify(reply.result, null, 2) }] };
});

const transport = new StdioServerTransport();
await server.connect(transport);
process.stderr.write(`[ltjs-mcp-bridge] forwarding to engine at ${HOST}:${PORT}\n`);
