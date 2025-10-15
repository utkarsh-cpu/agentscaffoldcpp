#pragma once

#include "base_node.hpp"

namespace pocketflow {

/**
 * Flow - Graph orchestrator that manages node execution and transitions
 * 
 * Flow manages the execution of a graph of nodes, handling action-based
 * transitions and parameter passing between nodes.
 */
class Flow : public virtual BaseNode {
public:
    /**
     * Constructor
     * @param start Optional start node for the flow
     */
    explicit Flow(std::shared_ptr<BaseNode> start = nullptr) 
        : start_node_(std::move(start)) {}
    
    /**
     * Set the start node for this flow
     * @param start_node The node to start execution from
     * @return The start node for chaining
     */
    std::shared_ptr<BaseNode> start(std::shared_ptr<BaseNode> start_node) {
        start_node_ = std::move(start_node);
        return start_node_;
    }
    
    /**
     * Get the start node
     * @return The current start node
     */
    std::shared_ptr<BaseNode> get_start_node() const {
        return start_node_;
    }
    
    /**
     * Get next node based on current node and action
     * @param curr Current node
     * @param action Action returned by current node
     * @return Next node to execute, or nullptr if flow should terminate
     */
    std::shared_ptr<BaseNode> get_next_node(const std::shared_ptr<BaseNode>& curr, const std::string& action) {
        if (!curr) {
            return nullptr;
        }
        
        // First try to get successor for the specific action
        auto next = curr->get_successor(action);
        if (next) {
            return next;
        }
        
        // If no specific action successor, try "default"
        if (action != "default") {
            next = curr->get_successor("default");
            if (next) {
                return next;
            }
        }
        
        // No successor found - flow terminates
        return nullptr;
    }
    
    /**
     * Main orchestration logic - traverses and executes the node graph
     * @param shared Shared state object (mutable)
     * @param params Optional parameters to combine with node parameters
     * @return Final result from orchestration
     */
    json _orch(json& shared, const json& params = json{}) {
        if (!start_node_) {
            return json{};
        }
        
        std::shared_ptr<BaseNode> current = start_node_;
        json last_action;
        
        while (current) {
            // Create a copy of the current node for execution
            // This ensures each execution is independent
            std::shared_ptr<BaseNode> node_copy = current;
            
            // Combine flow parameters with node parameters
            json combined_params = params_;
            if (!params.is_null()) {
                // Merge params into combined_params
                if (params.is_object()) {
                    combined_params.update(params);
                }
            }
            
            // Set parameters on the node copy
            if (!combined_params.is_null() && combined_params.is_object()) {
                node_copy->set_params(combined_params);
            }
            
            // Execute the current node
            json action = node_copy->_run(shared);
            
            // Convert action to string for navigation
            std::string action_str = "default";
            if (action.is_string()) {
                action_str = action.get<std::string>();
            } else if (!action.is_null()) {
                // If action is not a string but not null, convert to string
                action_str = action.dump();
            }
            
            last_action = action;
            
            // Get next node based on action
            std::shared_ptr<BaseNode> next_node = get_next_node(current, action_str);
            
            // Move to next node or terminate if no successor
            current = next_node;
        }
        
        return last_action;
    }
    
    /**
     * Override _run to use orchestration instead of direct execution
     * @param shared Shared state object (mutable)
     * @return Result from post phase
     */
    json _run(json& shared) override {
        // Execute the flow lifecycle: prep -> orchestration -> post
        json prep_res = prep(shared);
        json exec_res = _orch(shared, prep_res);
        json post_res = post(shared, prep_res, exec_res);
        return post_res;
    }
    
    /**
     * Default post implementation for flows
     * @param shared Shared state object
     * @param prep_res Result from prep phase
     * @param exec_res Result from exec phase (orchestration result)
     * @return The exec_res (orchestration result)
     */
    json post([[maybe_unused]] const json& shared, [[maybe_unused]] const json& prep_res, const json& exec_res) override {
        return exec_res;
    }

private:
    std::shared_ptr<BaseNode> start_node_;  ///< The starting node of the flow
};

} // namespace pocketflow