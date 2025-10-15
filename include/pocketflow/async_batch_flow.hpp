#pragma once

#include "async_flow.hpp"
#include <utility>

namespace pocketflow {

/**
 * AsyncBatchFlow - Asynchronous batch processing flow
 * 
 * AsyncBatchFlow extends AsyncFlow to provide sequential asynchronous batch 
 * processing where each batch parameter set is processed through the flow 
 * orchestration asynchronously.
 */
class AsyncBatchFlow : public AsyncFlow {
public:
    /**
     * Constructor
     * @param start Optional start node for the flow
     */
    explicit AsyncBatchFlow(std::shared_ptr<BaseNode> start = nullptr) 
        : AsyncFlow(std::move(start)) {}

protected:
    /**
     * Override _run_async for sequential async batch parameter processing
     * Iterates over batch parameters and runs async orchestration for each set
     * @param shared Shared state object (mutable)
     * @return Future containing result from post phase
     */
    std::future<json> _run_async(json& shared) override {
        return std::async(std::launch::async, [this, &shared]() -> json {
            // Execute async prep phase to get batch parameters
            auto prep_future = this->prep_async(shared);
            json prep_res = prep_future.get();
            
            // If prep_res is an array, iterate over each batch parameter set
            if (prep_res.is_array()) {
                for (const auto& batch_params : prep_res) {
                    // Combine flow parameters with individual batch parameters
                    json combined_params = this->params_;
                    
                    // Merge batch_params into combined_params if it's an object
                    if (batch_params.is_object()) {
                        if (combined_params.is_null()) {
                            combined_params = json::object();
                        }
                        combined_params.update(batch_params);
                    }
                    
                    // Run async orchestration with combined parameters
                    auto orch_future = this->_orch_async(shared, combined_params);
                    orch_future.get();  // Wait for completion before processing next batch
                }
            }
            
            // Execute async post phase and return result
            auto post_future = this->post_async(shared, prep_res, json{});
            return post_future.get();
        });
    }
};

/**
 * AsyncParallelBatchFlow - Parallel asynchronous batch processing flow
 * 
 * AsyncParallelBatchFlow extends AsyncFlow to provide parallel asynchronous 
 * batch processing where all batch parameter sets are processed simultaneously 
 * through the flow orchestration.
 */
class AsyncParallelBatchFlow : public AsyncFlow {
public:
    /**
     * Constructor
     * @param start Optional start node for the flow
     */
    explicit AsyncParallelBatchFlow(std::shared_ptr<BaseNode> start = nullptr) 
        : AsyncFlow(std::move(start)) {}

protected:
    /**
     * Override _run_async for parallel async batch parameter processing
     * Launches all batch operations simultaneously and waits for completion
     * @param shared Shared state object (mutable)
     * @return Future containing result from post phase
     */
    std::future<json> _run_async(json& shared) override {
        return std::async(std::launch::async, [this, &shared]() -> json {
            // Execute async prep phase to get batch parameters
            auto prep_future = this->prep_async(shared);
            json prep_res = prep_future.get();
            
            // If prep_res is an array, launch all batch operations in parallel
            if (prep_res.is_array()) {
                std::vector<std::future<json>> futures;
                
                // Launch all batch operations simultaneously
                for (const auto& batch_params : prep_res) {
                    // Combine flow parameters with individual batch parameters
                    json combined_params = this->params_;
                    
                    // Merge batch_params into combined_params if it's an object
                    if (batch_params.is_object()) {
                        if (combined_params.is_null()) {
                            combined_params = json::object();
                        }
                        combined_params.update(batch_params);
                    }
                    
                    // Launch async orchestration with combined parameters
                    futures.push_back(this->_orch_async(shared, combined_params));
                }
                
                // Wait for all batch operations to complete
                for (auto& future : futures) {
                    future.get();
                }
            }
            
            // Execute async post phase and return result
            auto post_future = this->post_async(shared, prep_res, json{});
            return post_future.get();
        });
    }
};

} // namespace pocketflow