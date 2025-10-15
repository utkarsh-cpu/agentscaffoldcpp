#pragma once

#include "node.hpp"

namespace pocketflow {

/**
 * BatchNode - Processes arrays of items sequentially
 * 
 * This class extends Node to handle batch processing of JSON arrays.
 * Each item in the array is processed individually through the exec method.
 */
class BatchNode : public virtual Node {
public:
    /**
     * Constructor
     * @param max_retries Maximum number of retry attempts per item (default: 1)
     * @param wait_ms Wait time in milliseconds between retries (default: 0)
     */
    explicit BatchNode(int max_retries = 1, int wait_ms = 0) 
        : Node(max_retries, wait_ms) {}

protected:
    /**
     * Internal exec method for batch processing
     * Processes each item in the array through Node::_exec
     * @param items JSON array of items to process
     * @return JSON array of results
     */
    json _exec(const json& items) override {
        json results = json::array();
        
        // Handle empty arrays gracefully
        if (items.is_array()) {
            // Process each item in the array sequentially
            for (const auto& item : items) {
                // Use Node::_exec to get retry logic for each item
                json item_result = Node::_exec(item);
                results.push_back(item_result);
            }
        } else {
            // Handle non-array inputs gracefully by treating as single item
            json item_result = Node::_exec(items);
            results.push_back(item_result);
        }
        
        return results;
    }
};

} // namespace pocketflow