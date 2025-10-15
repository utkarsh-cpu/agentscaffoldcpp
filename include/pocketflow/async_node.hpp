#pragma once

#include "node.hpp"
#include <chrono>
#include <future>
#include <thread>

namespace pocketflow {

/**
 * AsyncNode - Asynchronous version of Node with std::future support
 *
 * AsyncNode provides asynchronous execution capabilities using std::future.
 * It includes async versions of all lifecycle methods and retry logic.
 */
class AsyncNode : public virtual Node {
public:
  /**
   * Constructor
   * @param max_retries Maximum number of retry attempts (default: 1)
   * @param wait_ms Wait time in milliseconds between retries (default: 0)
   */
  explicit AsyncNode(int max_retries = 1, int wait_ms = 0)
      : Node(max_retries, wait_ms) {}

  // Async lifecycle methods

  /**
   * Async prepare phase
   * @param shared The shared state object
   * @return Future containing prepared data for exec phase
   */
  virtual std::future<json> prep_async(const json &shared) {
    return std::async(std::launch::async,
                      [this, shared]() { return prep(shared); });
  }

  /**
   * Async execute phase - pure virtual, must be implemented
   * @param prep_res Result from prep phase
   * @return Future containing execution result
   */
  virtual std::future<json> exec_async(const json &prep_res) = 0;

  /**
   * Async fallback method for error handling
   * @param prep_res Result from prep phase
   * @param exc The exception that caused the failure
   * @return Future containing fallback result
   */
  virtual std::future<json> exec_fallback_async(const json &prep_res,
                                                const std::exception &exc) {
    return std::async(std::launch::async, [this, prep_res, &exc]() {
      return exec_fallback(prep_res, exc);
    });
  }

  /**
   * Async post phase
   * @param shared The shared state object
   * @param prep_res Result from prep phase
   * @param exec_res Result from exec phase
   * @return Future containing action string for flow control
   */
  virtual std::future<json> post_async(const json &shared, const json &prep_res,
                                       const json &exec_res) {
    return std::async(std::launch::async, [this, shared, prep_res, exec_res]() {
      return post(shared, prep_res, exec_res);
    });
  }

  // Async execution methods

  /**
   * Internal async exec method with retry logic
   * @param prep_res Result from prep phase
   * @return Future containing execution result
   */
  virtual std::future<json> _exec_async(const json &prep_res) {
    return std::async(std::launch::async, [this, prep_res]() {
      int cur_retry = 0;

      while (cur_retry < max_retries_) {
        try {
          // Attempt async execution
          auto future = exec_async(prep_res);
          json result = future.get();
          return result;
        } catch (const std::exception &e) {
          cur_retry++;

          // If this was the last retry attempt, call async fallback
          if (cur_retry >= max_retries_) {
            auto fallback_future = exec_fallback_async(prep_res, e);
            return fallback_future.get();
          }

          // Wait before retrying (exponential backoff)
          if (wait_ > 0) {
            // Exponential backoff: wait * (2^(cur_retry-1))
            int backoff_time = wait_ * (1 << (cur_retry - 1));
            std::this_thread::sleep_for(
                std::chrono::milliseconds(backoff_time));
          }
        }
      }

      // This should never be reached due to the logic above, but just in case
      throw std::runtime_error("Unexpected state in async retry logic");
    });
  }

  /**
   * Main async execution method
   * @param shared The shared state object (mutable)
   * @return Future containing action string from post phase
   */
  std::future<json> run_async(json &shared) { return _run_async(shared); }

  /**
   * Internal async run method - executes the full async lifecycle
   * @param shared The shared state object (mutable)
   * @return Future containing action string from post phase
   */
  virtual std::future<json> _run_async(json &shared) {
    return std::async(std::launch::async, [this, &shared]() {
      // Execute async lifecycle: prep -> exec -> post
      auto prep_future = prep_async(shared);
      json prep_res = prep_future.get();

      auto exec_future = _exec_async(prep_res);
      json exec_res = exec_future.get();

      auto post_future = post_async(shared, prep_res, exec_res);
      json action = post_future.get();

      return action;
    });
  }

  /**
   * Override sync _run to throw error - AsyncNode should use async methods
   * @param shared The shared state object
   * @throws std::runtime_error Always throws, directing user to use run_async
   */
  json _run([[maybe_unused]] json &shared) override {
    throw std::runtime_error("AsyncNode requires async execution - use "
                             "run_async() instead of run()");
  }
};

} // namespace pocketflow