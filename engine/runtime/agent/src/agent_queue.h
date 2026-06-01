// ------------------------------------------------------------------ //
//  FILE    : agent_queue.h
//  PURPOSE : Marshal agent tool calls onto the engine main thread.
//
//  The engine runs a single-threaded main loop. Transport threads (the MCP TCP
//  server) must never touch engine state directly; they Push() a request and
//  await the returned future. Once per frame the main thread calls DrainFrame()
//  which dispatches queued requests through the registry and fulfils the
//  promises, unblocking the waiting transport threads.
// ------------------------------------------------------------------ //

#pragma once

#include <cstddef>
#include <deque>
#include <future>
#include <mutex>
#include <string>

#include "agent_tool.h"

namespace ltjs::agent {

class AgentRegistry;

/// Thread-safe hand-off of tool calls from transport threads to the main thread.
class AgentCommandQueue {
public:
  explicit AgentCommandQueue(AgentRegistry &registry) : registry_(registry) {}

  AgentCommandQueue(const AgentCommandQueue &) = delete;
  AgentCommandQueue &operator=(const AgentCommandQueue &) = delete;

  /// Enqueue a tool call. Non-blocking; safe to call from any thread. The
  /// returned future is fulfilled by a later DrainFrame() on the main thread.
  std::future<AgentResponse> Push(std::string tool_name, Json params);

  /// Dispatch up to `max_per_frame` queued requests on the calling (main)
  /// thread. Unknown tools and handler exceptions become error responses.
  /// Returns the number of requests dispatched. No lock is held while a handler
  /// runs.
  int DrainFrame(int max_per_frame = 4);

  std::size_t PendingCount() const;

private:
  struct Pending {
    std::string tool_name;
    Json params;
    std::promise<AgentResponse> promise;
  };

  AgentRegistry &registry_;
  mutable std::mutex mutex_;
  std::deque<Pending> queue_;
};

} // namespace ltjs::agent
