#pragma once

#include "async_node.hpp"
#include "batch_node.hpp"
#include <vector>

namespace pocketflow {

/**
 * AsyncBatchNode - Sequential async processing of arrays
 *
 * Processes array items sequentially using async execution.
 * Each item is processed asynchronously but waits for completion before
 * proceeding to the next item.
 */
class AsyncBatchNode : public AsyncNode, public BatchNode {
public:
  /**
   * Constructor
   * @param max_retries Maximum number of retry attempts per item (default: 1)
   * @param wait Wait time in milliseconds between retries (default: 0)
   */
  explicit AsyncBatchNode(int max_retries = 1, int wait = 0)
      : AsyncNode(max_retries, wait), BatchNode(max_retries, wait) {}

  // Resolve ambiguity by using AsyncNode's _exec (which throws error for sync
  // usage)
  using AsyncNode::_exec;

protected:
  /**
   * Internal async exec method for sequential batch processing
   * @param items JSON array of items to process
   * @return Future containing JSON array of results
   */
  std::future<json> _exec_async(const json &items) override {
    return std::async(std::launch::async, [this, items]() {
      json results = json::array();

      // Handle empty arrays gracefully
      if (items.is_array()) {
        // Process each item sequentially with async execution
        for (const auto &item : items) {
          // Use this->_exec_async to get async retry logic for each item
          auto future = this->AsyncNode::_exec_async(item);
          json item_result =
              future.get(); // Wait for completion before next item
          results.push_back(item_result);
        }
      } else {
        // Handle non-array inputs gracefully by treating as single item
        auto future = this->AsyncNode::_exec_async(items);
        json item_result = future.get();
        results.push_back(item_result);
      }

      return results;
    });
  }
};

/**
 * AsyncParallelBatchNode - Parallel async processing of arrays
 *
 * Processes all array items in parallel using async execution.
 * All items are launched simultaneously and results are collected
 * when all futures complete.
 */
class AsyncParallelBatchNode : public AsyncNode, public BatchNode {
public:
  /**
   * Constructor
   * @param max_retries Maximum number of retry attempts per item (default: 1)
   * @param wait Wait time in milliseconds between retries (default: 0)
   */
  explicit AsyncParallelBatchNode(int max_retries = 1, int wait = 0)
      : AsyncNode(max_retries, wait), BatchNode(max_retries, wait) {}

  // Resolve ambiguity by using AsyncNode's _exec (which throws error for sync
  // usage)
  using AsyncNode::_exec;

protected:
  /**
   * Internal async exec method for parallel batch processing
   * @param items JSON array of items to process
   * @return Future containing JSON array of results
   */
  std::future<json> _exec_async(const json &items) override {
    return std::async(std::launch::async, [this, items]() {
      json results = json::array();

      // Handle empty arrays gracefully
      if (items.is_array()) {
        std::vector<std::future<json>> futures;

        // Launch all async tasks simultaneously
        for (const auto &item : items) {
          // Use AsyncNode::_exec_async to get async retry logic for each item
          futures.push_back(this->AsyncNode::_exec_async(item));
        }

        // Collect results in order
        for (auto &future : futures) {
          json item_result = future.get();
          results.push_back(item_result);
        }
      } else {
        // Handle non-array inputs gracefully by treating as single item
        auto future = this->AsyncNode::_exec_async(items);
        json item_result = future.get();
        results.push_back(item_result);
      }

      return results;
    });
  }
};

} // namespace pocketflow