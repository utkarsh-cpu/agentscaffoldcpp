#pragma once

#include <exception>
#include <string>

namespace pocketflow {

/**
 * Base exception class for PocketFlow framework
 */
class FlowException : public std::exception {
public:
    explicit FlowException(const std::string& message) : message_(message) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
protected:
    std::string message_;
};

/**
 * Exception thrown when node execution fails
 */
class NodeExecutionException : public FlowException {
public:
    NodeExecutionException(const std::string& node_name, const std::string& error)
        : FlowException("Node '" + node_name + "' failed: " + error) {}
};

/**
 * Exception thrown when flow orchestration encounters an error
 */
class FlowOrchestrationException : public FlowException {
public:
    explicit FlowOrchestrationException(const std::string& message)
        : FlowException("Flow orchestration error: " + message) {}
};

/**
 * Exception thrown when async operations are used incorrectly
 */
class AsyncOperationException : public FlowException {
public:
    explicit AsyncOperationException(const std::string& message)
        : FlowException("Async operation error: " + message) {}
};

/**
 * Exception thrown when retry limit is exceeded
 */
class RetryLimitExceededException : public FlowException {
public:
    RetryLimitExceededException(int max_retries, const std::string& last_error)
        : FlowException("Retry limit exceeded (" + std::to_string(max_retries) + 
                       " attempts). Last error: " + last_error) {}
};

} // namespace pocketflow