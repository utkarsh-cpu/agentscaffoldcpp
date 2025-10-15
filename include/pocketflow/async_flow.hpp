#pragma once

#include "flow.hpp"
#include "async_node.hpp"
#include <future>

namespace pocketflow {

/**
 * AsyncFlow - Asynchronous version of Flow with mixed sync/async node support
 * 
 * AsyncFlow extends Flow to provide asynchronous graph orchestration.
 * It can handle both synchronous and asynchronous nodes within the same flow.
 */
class AsyncFlow : public Flow {
public:
    /**
     * Constructor
     * @param start Optional start node for the flow
     */
    explicit AsyncFlow(std::shared_ptr<BaseNode> start = nullptr) 
        : Flow(std::move(start)) {}
    
    /**
     * Async orchestration logic - traverses and executes the node graph asynchronously
     * @param shared Shared state object (mutable)
     * @param params Optional parameters to combine with node parameters
     * @return Future containing final result from orchestration
     */
    std::future<json> _orch_async(json& shared, const json& params = json{}) {
        return std::async(std::launch::async, [this, &shared, params]() -> json {
            if (!this->get_start_node()) {
                return json{};
            }
            
            std::shared_ptr<BaseNode> current = this->get_start_node();
            json last_action;
            
            while (current) {
                // Create a copy of the current node for execution
                std::shared_ptr<BaseNode> node_copy = current;
                
                // Combine flow parameters with node parameters
                json combined_params = this->params_;
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
                
                // Execute the current node - handle both sync and async nodes
                json action;
                
                // Check if this is an AsyncNode
                auto async_node = std::dynamic_pointer_cast<AsyncNode>(node_copy);
                if (async_node) {
                    // Execute asynchronously
                    auto future = async_node->run_async(shared);
                    action = future.get();
                } else {
                    // Execute synchronously
                    action = node_copy->_run(shared);
                }
                
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
                std::shared_ptr<BaseNode> next_node = this->get_next_node(current, action_str);
                
                // Move to next node or terminate if no successor
                current = next_node;
            }
            
            return last_action;
        });
    }
    
    // Async lifecycle methods
    
    /**
     * Async prepare phase
     * @param shared The shared state object
     * @return Future containing prepared data for exec phase
     */
    virtual std::future<json> prep_async(const json& shared) {
        return std::async(std::launch::async, [this, shared]() -> json { 
            return prep(shared); 
        });
    }
    
    /**
     * Main async execution method
     * @param shared The shared state object (mutable)
     * @return Future containing action string from post phase
     */
    std::future<json> run_async(json& shared) {
        return _run_async(shared);
    }
    
    /**
     * Override _run_async to use async orchestration
     * @param shared Shared state object (mutable)
     * @return Future-containing result from post-phase
     */
    virtual std::future<json> _run_async(json& shared) {
        return std::async(std::launch::async, [this, &shared]() -> json {
            // Execute the async flow lifecycle: prep -> orchestration -> post
            auto prep_future = this->prep_async(shared);
            json prep_res = prep_future.get();
            
            auto orch_future = _orch_async(shared, prep_res);
            json exec_res = orch_future.get();
            
            auto post_future = this->post_async(shared, prep_res, exec_res);
            json post_res = post_future.get();
            
            return post_res;
        });
    }
    
    /**
     * Default async post implementation for flows
     * @param shared Shared state object
     * @param prep_res Result from prep phase
     * @param exec_res Result from exec phase (orchestration result)
     * @return Future containing the exec_res (orchestration result)
     */
    virtual std::future<json> post_async([[maybe_unused]] const json& shared, 
                                        [[maybe_unused]] const json& prep_res, 
                                        const json& exec_res) {
        return std::async(std::launch::async, [exec_res]() -> json {
            return exec_res;
        });
    }
};

} // namespace pocketflow