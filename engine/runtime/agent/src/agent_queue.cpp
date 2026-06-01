#include "agent_queue.h"

#include <exception>
#include <utility>
#include <vector>

#include "agent_registry.h"

namespace ltjs::agent {

std::future<AgentResponse> AgentCommandQueue::Push(std::string tool_name, Json params) {
  std::lock_guard<std::mutex> lock(mutex_);
  Pending pending;
  pending.tool_name = std::move(tool_name);
  pending.params = std::move(params);
  std::future<AgentResponse> future = pending.promise.get_future();
  queue_.push_back(std::move(pending));
  return future;
}

int AgentCommandQueue::DrainFrame(int max_per_frame) {
  // Move a bounded batch out under the lock, then run handlers unlocked so a
  // slow handler never blocks transport threads from enqueuing more work.
  std::vector<Pending> batch;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty() && static_cast<int>(batch.size()) < max_per_frame) {
      batch.push_back(std::move(queue_.front()));
      queue_.pop_front();
    }
  }

  for (Pending &pending : batch) {
    AgentResponse response;
    const AgentTool *tool = registry_.Find(pending.tool_name);
    if (tool == nullptr) {
      response = AgentResponse::Err("unknown tool: " + pending.tool_name);
    } else {
      try {
        AgentRequest request{pending.tool_name, pending.params};
        response = tool->handler(request);
      } catch (const std::exception &e) {
        response = AgentResponse::Err(std::string("handler threw: ") + e.what());
      } catch (...) {
        response = AgentResponse::Err("handler threw: unknown exception");
      }
    }
    pending.promise.set_value(std::move(response));
  }

  return static_cast<int>(batch.size());
}

std::size_t AgentCommandQueue::PendingCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.size();
}

} // namespace ltjs::agent
