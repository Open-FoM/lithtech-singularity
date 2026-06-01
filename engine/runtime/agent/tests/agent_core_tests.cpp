// Unit tests for the engine-independent agent control core:
// AgentResponse factories, AgentRegistry, AgentCommandQueue, and the JSON wire
// protocol. No engine dependencies — builds and runs standalone.

#include <chrono>
#include <future>
#include <string>

#include <gtest/gtest.h>

#include "agent_protocol.h"
#include "agent_queue.h"
#include "agent_registry.h"
#include "agent_tool.h"

using namespace ltjs::agent;

namespace {

AgentTool MakeTool(std::string name, AgentToolFn fn) {
  AgentTool t;
  t.name = std::move(name);
  t.description = "test tool";
  t.handler = std::move(fn);
  return t;
}

AgentToolFn EchoHandler() {
  return [](const AgentRequest &req) { return AgentResponse::Ok(Json{{"echo", req.params}}); };
}

} // namespace

// ----------------------------- AgentResponse ------------------------------ //

TEST(AgentResponse, OkCarriesResultAndNoError) {
  auto r = AgentResponse::Ok(Json{{"value", 42}});
  EXPECT_TRUE(r.ok);
  EXPECT_TRUE(r.error.empty());
  EXPECT_EQ(r.result.at("value").get<int>(), 42);
}

TEST(AgentResponse, ErrCarriesMessageAndNotOk) {
  auto r = AgentResponse::Err("boom");
  EXPECT_FALSE(r.ok);
  EXPECT_EQ(r.error, "boom");
}

// ----------------------------- AgentRegistry ------------------------------ //

TEST(AgentRegistry, AddThenFind) {
  AgentRegistry reg;
  EXPECT_TRUE(reg.Add(MakeTool("ping", EchoHandler())));
  ASSERT_EQ(reg.Size(), 1u);
  const AgentTool *t = reg.Find("ping");
  ASSERT_NE(t, nullptr);
  EXPECT_EQ(t->name, "ping");
}

TEST(AgentRegistry, FindMissingReturnsNull) {
  AgentRegistry reg;
  EXPECT_EQ(reg.Find("nope"), nullptr);
}

TEST(AgentRegistry, DuplicateNameRejected) {
  AgentRegistry reg;
  EXPECT_TRUE(reg.Add(MakeTool("dup", EchoHandler())));
  EXPECT_FALSE(reg.Add(MakeTool("dup", EchoHandler())));
  EXPECT_EQ(reg.Size(), 1u);
}

TEST(AgentRegistry, ToolsPreservesRegistrationOrder) {
  AgentRegistry reg;
  reg.Add(MakeTool("a", EchoHandler()));
  reg.Add(MakeTool("b", EchoHandler()));
  reg.Add(MakeTool("c", EchoHandler()));
  auto tools = reg.Tools();
  ASSERT_EQ(tools.size(), 3u);
  EXPECT_EQ(tools[0].name, "a");
  EXPECT_EQ(tools[1].name, "b");
  EXPECT_EQ(tools[2].name, "c");
}

TEST(AgentRegistry, ClearEmpties) {
  AgentRegistry reg;
  reg.Add(MakeTool("a", EchoHandler()));
  reg.Clear();
  EXPECT_EQ(reg.Size(), 0u);
}

// ---------------------------- AgentCommandQueue --------------------------- //

TEST(AgentCommandQueue, PushThenDrainDispatchesToHandler) {
  AgentRegistry reg;
  reg.Add(MakeTool("echo", EchoHandler()));
  AgentCommandQueue queue(reg);

  auto fut = queue.Push("echo", Json{{"x", 1}});
  EXPECT_EQ(queue.PendingCount(), 1u);

  int dispatched = queue.DrainFrame(4);
  EXPECT_EQ(dispatched, 1);
  EXPECT_EQ(queue.PendingCount(), 0u);

  ASSERT_EQ(fut.wait_for(std::chrono::seconds(0)), std::future_status::ready);
  AgentResponse resp = fut.get();
  EXPECT_TRUE(resp.ok);
  EXPECT_EQ(resp.result.at("echo").at("x").get<int>(), 1);
}

TEST(AgentCommandQueue, UnknownToolYieldsErrorResponse) {
  AgentRegistry reg;
  AgentCommandQueue queue(reg);

  auto fut = queue.Push("missing", Json::object());
  queue.DrainFrame();

  AgentResponse resp = fut.get();
  EXPECT_FALSE(resp.ok);
  EXPECT_NE(resp.error.find("missing"), std::string::npos);
}

TEST(AgentCommandQueue, HandlerExceptionBecomesError) {
  AgentRegistry reg;
  reg.Add(MakeTool("throws", [](const AgentRequest &) -> AgentResponse { throw std::runtime_error("kaboom"); }));
  AgentCommandQueue queue(reg);

  auto fut = queue.Push("throws", Json::object());
  queue.DrainFrame();

  AgentResponse resp = fut.get();
  EXPECT_FALSE(resp.ok);
  EXPECT_NE(resp.error.find("kaboom"), std::string::npos);
}

TEST(AgentCommandQueue, DrainRespectsMaxPerFrame) {
  AgentRegistry reg;
  reg.Add(MakeTool("echo", EchoHandler()));
  AgentCommandQueue queue(reg);

  for (int i = 0; i < 5; ++i)
    queue.Push("echo", Json::object());
  EXPECT_EQ(queue.PendingCount(), 5u);

  EXPECT_EQ(queue.DrainFrame(2), 2);
  EXPECT_EQ(queue.PendingCount(), 3u);
  EXPECT_EQ(queue.DrainFrame(10), 3);
  EXPECT_EQ(queue.PendingCount(), 0u);
}

TEST(AgentCommandQueue, DrainEmptyReturnsZero) {
  AgentRegistry reg;
  AgentCommandQueue queue(reg);
  EXPECT_EQ(queue.DrainFrame(4), 0);
}

// ------------------------------ Wire protocol ----------------------------- //

TEST(WireProtocol, ParsesValidRequest) {
  std::string err;
  auto req = agent_ParseRequest(R"({"id":"7","tool":"console_exec","params":{"command":"version"}})", err);
  ASSERT_TRUE(req.has_value()) << err;
  EXPECT_EQ(req->id, "7");
  EXPECT_EQ(req->tool, "console_exec");
  EXPECT_EQ(req->params.at("command").get<std::string>(), "version");
}

TEST(WireProtocol, MissingParamsDefaultsToEmptyObject) {
  std::string err;
  auto req = agent_ParseRequest(R"({"id":"1","tool":"ping"})", err);
  ASSERT_TRUE(req.has_value()) << err;
  EXPECT_TRUE(req->params.is_object());
  EXPECT_TRUE(req->params.empty());
}

TEST(WireProtocol, InvalidJsonFails) {
  std::string err;
  auto req = agent_ParseRequest("{not json", err);
  EXPECT_FALSE(req.has_value());
  EXPECT_FALSE(err.empty());
}

TEST(WireProtocol, NonObjectTopLevelFails) {
  std::string err;
  auto req = agent_ParseRequest(R"(["a","b"])", err);
  EXPECT_FALSE(req.has_value());
}

TEST(WireProtocol, MissingToolFails) {
  std::string err;
  auto req = agent_ParseRequest(R"({"id":"1","params":{}})", err);
  EXPECT_FALSE(req.has_value());
}

TEST(WireProtocol, SerializesOkResponseWithResult) {
  auto wire = agent_SerializeResponse("9", AgentResponse::Ok(Json{{"pong", true}}));
  Json parsed = Json::parse(wire);
  EXPECT_EQ(parsed.at("id").get<std::string>(), "9");
  ASSERT_TRUE(parsed.contains("result"));
  EXPECT_TRUE(parsed.at("result").at("pong").get<bool>());
  EXPECT_FALSE(parsed.contains("error"));
}

TEST(WireProtocol, SerializesErrorResponseWithoutResult) {
  auto wire = agent_SerializeResponse("9", AgentResponse::Err("nope"));
  Json parsed = Json::parse(wire);
  EXPECT_EQ(parsed.at("id").get<std::string>(), "9");
  EXPECT_EQ(parsed.at("error").get<std::string>(), "nope");
  EXPECT_FALSE(parsed.contains("result"));
}

TEST(WireProtocol, SerializesToolListSchema) {
  AgentRegistry reg;
  AgentTool t;
  t.name = "set_cvar";
  t.description = "Set a console variable";
  t.params = {{"name", "string", "cvar name", true}, {"value", "string", "new value", true}};
  t.handler = EchoHandler();
  reg.Add(std::move(t));

  Json list = agent_SerializeToolList(reg.Tools());
  ASSERT_TRUE(list.is_array());
  ASSERT_EQ(list.size(), 1u);
  EXPECT_EQ(list[0].at("name").get<std::string>(), "set_cvar");
  EXPECT_EQ(list[0].at("description").get<std::string>(), "Set a console variable");
  ASSERT_TRUE(list[0].at("parameters").is_array());
  EXPECT_EQ(list[0].at("parameters").size(), 2u);
  EXPECT_EQ(list[0].at("parameters")[0].at("name").get<std::string>(), "name");
  EXPECT_EQ(list[0].at("parameters")[0].at("required").get<bool>(), true);
}
