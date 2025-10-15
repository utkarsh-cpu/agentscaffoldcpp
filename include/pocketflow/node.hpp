#pragma once

#include "base_node.hpp"
#include <exception>
#include <thread>
#include <chrono>

namespace pocketflow {

/**
 * Node - Extends BaseNode with retry logic and error handling
 * 
 * This class adds retry capabilities and graceful error handling to the base node functionality.
 * It implements exponential backoff and provides a fallback mechanism for handling failures.
 */
class Node : public BaseNode {
public:
    /**
     * Constructor with retry configuration
     * @param max_retries Maximum number of retry attempts (default: 1, meaning no retries)
     * @param wait_ms Initial wait time in milliseconds between retries (default: 0)
     */
    explicit Node(int max_retries = 1, int wait_ms = 0) 
        : max_retries_(max_retries), wait_(wait_ms), cur_retry_(0) {}
    
    virtual ~Node() = default;
    
    /**
     * Fallback method for handling failures after all retries are exhausted
     * @param prep_res Result from prep phase
     * @param exc The exception that caused the failure
     * @return Fallback result or re-throws the exception
     */
    virtual json exec_fallback([[maybe_unused]] const json& prep_res, const std::exception& exc) {
        // Default behavior: re-throw the exception
        throw exc;
    }
    
protected:
    /**
     * Internal exec method with retry logic and exponential backoff
     * @param prep_res Result from prep phase
     * @return Execution result
     * @throws std::exception if all retries are exhausted and exec_fallback also fails
     */
    json _exec(const json& prep_res) override {
        cur_retry_ = 0;
        
        while (cur_retry_ < max_retries_) {
            try {
                // Attempt execution
                json result = exec(prep_res);
                cur_retry_ = 0;  // Reset retry counter on success
                return result;
            } catch (const std::exception& e) {
                cur_retry_++;
                
                // If this was the last retry attempt, call fallback
                if (cur_retry_ >= max_retries_) {
                    return exec_fallback(prep_res, e);
                }
                
                // Wait before retrying (exponential backoff)
                if (wait_ > 0) {
                    // Exponential backoff: wait * (2^(cur_retry-1))
                    int backoff_time = wait_ * (1 << (cur_retry_ - 1));
                    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_time));
                }
            }
        }
        
        // This should never be reached due to the logic above, but just in case
        throw std::runtime_error("Unexpected state in retry logic");
    }
    
protected:
    int max_retries_;  ///< Maximum number of retry attempts
    int wait_;         ///< Base wait time in milliseconds
    int cur_retry_;    ///< Current retry attempt counter
};

} // namespace pocketflow