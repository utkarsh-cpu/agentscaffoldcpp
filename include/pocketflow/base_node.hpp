#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <nlohmann/json.hpp>

namespace pocketflow {

using json = nlohmann::json;

// Forward declarations
class BaseNode;
class ConditionalTransition;

/**
 * BaseNode - Core abstraction with successor management and lifecycle methods
 * 
 * This is the fundamental building block of PocketFlow. All nodes inherit from BaseNode
 * and implement the prep->exec->post lifecycle pattern.
 */
class BaseNode : public std::enable_shared_from_this<BaseNode> {
public:
    BaseNode() = default;
    virtual ~BaseNode() = default;
    
    // Non-copyable but movable
    BaseNode(const BaseNode&) = delete;
    BaseNode& operator=(const BaseNode&) = delete;
    BaseNode(BaseNode&&) = default;
    BaseNode& operator=(BaseNode&&) = default;
    
    /**
     * Set parameters for this node
     * @param params JSON object containing node parameters
     */
    void set_params(const json& params) { params_ = params; }
    
    /**
     * Get current parameters
     * @return Current node parameters
     */
    const json& get_params() const { return params_; }
    
    /**
     * Add a successor node with an action
     * @param node The successor node
     * @param action The action that triggers this successor (default: "default")
     * @return The successor node for chaining
     */
    std::shared_ptr<BaseNode> next(std::shared_ptr<BaseNode> node, const std::string& action = "default") {
        successors_[action] = node;
        return node;
    }
    
    /**
     * Get successor for a given action
     * @param action The action to look up
     * @return The successor node, or nullptr if not found
     */
    std::shared_ptr<BaseNode> get_successor(const std::string& action) const {
        auto it = successors_.find(action);
        return (it != successors_.end()) ? it->second : nullptr;
    }
    
    // Lifecycle methods (virtual, can be overridden)
    
    /**
     * Prepare phase - extract and prepare data from shared state
     * @param shared The shared state object
     * @return Prepared data for exec phase
     */
    virtual json prep([[maybe_unused]] const json& shared) { return json{}; }
    
    /**
     * Execute phase - perform the main computation
     * @param prep_res Result from prep phase
     * @return Execution result
     */
    virtual json exec([[maybe_unused]] const json& prep_res) { return json{}; }
    
    /**
     * Post phase - process results and update shared state
     * @param shared The shared state object (mutable)
     * @param prep_res Result from prep phase
     * @param exec_res Result from exec phase
     * @return Action string for flow control
     */
    virtual json post([[maybe_unused]] const json& shared, 
                     [[maybe_unused]] const json& prep_res, 
                     [[maybe_unused]] const json& exec_res) { return json{}; }
    
    // Internal execution methods
    
    /**
     * Internal exec method - can be overridden for retry logic
     * @param prep_res Result from prep phase
     * @return Execution result
     */
    virtual json _exec(const json& prep_res) { return exec(prep_res); }
    
    /**
     * Internal run method - executes the full lifecycle
     * @param shared The shared state object (mutable)
     * @return Action string from post phase
     */
    virtual json _run(json& shared) {
        // Execute the full lifecycle: prep -> exec -> post
        json prep_res = prep(shared);
        json exec_res = _exec(prep_res);
        json post_res = post(shared, prep_res, exec_res);
        return post_res;
    }
    
    /**
     * Main execution method - runs the node and warns about successors
     * @param shared The shared state object (mutable)
     * @return Action string from post phase
     */
    json run(json& shared) {
        json action = _run(shared);
        
        // Warn if there are successors but this is being called directly
        if (!successors_.empty()) {
            // This is a warning that successors exist but won't be executed
            // In a real implementation, you might want to log this
        }
        
        return action;
    }
    
    // Operator overloading for fluent API
    
    /**
     * Sequential chaining operator: node1 >> node2
     * @param other The next node in sequence
     * @return The next node for further chaining
     */
    std::shared_ptr<BaseNode> operator>>(std::shared_ptr<BaseNode> other) {
        return next(std::move(other), "default");
    }
    
    /**
     * Conditional transition operator: node - "action"
     * @param action The action for conditional branching
     * @return ConditionalTransition object for >> operator
     */
    ConditionalTransition operator-(const std::string& action);
    
protected:
    json params_;  ///< Node parameters
    std::unordered_map<std::string, std::shared_ptr<BaseNode>> successors_;  ///< Successor nodes by action
};

/**
 * ConditionalTransition - Helper class for action-based connections
 * 
 * Enables syntax like: node - "action" >> target
 */
class ConditionalTransition {
public:
    ConditionalTransition(std::shared_ptr<BaseNode> src, std::string action)
        : src_(std::move(src)), action_(std::move(action)) {}
    
    /**
     * Complete the conditional transition: (node - "action") >> target
     * @param target The target node
     * @return The target node for further chaining
     */
    std::shared_ptr<BaseNode> operator>>(std::shared_ptr<BaseNode> target) {
        return src_->next(std::move(target), action_);
    }
    
private:
    std::shared_ptr<BaseNode> src_;
    std::string action_;
};

// Inline implementation of BaseNode::operator- after ConditionalTransition is defined
inline ConditionalTransition BaseNode::operator-(const std::string& action) {
    return ConditionalTransition(std::static_pointer_cast<BaseNode>(shared_from_this()), action);
}

// Global operators for shared_ptr<BaseNode> to enable fluent API with smart pointers

/**
 * Sequential chaining operator for shared_ptr: node1 >> node2
 * @param left The first node
 * @param right The second node
 * @return The second node for further chaining
 */
inline std::shared_ptr<BaseNode> operator>>(const std::shared_ptr<BaseNode>& left, std::shared_ptr<BaseNode> right) {
    return left->next(std::move(right), "default");
}

/**
 * ConditionalTransition for shared_ptr - Helper class for action-based connections
 * 
 * Enables syntax like: node - "action" >> target
 */
class SharedPtrConditionalTransition {
public:
    SharedPtrConditionalTransition(std::shared_ptr<BaseNode> src, std::string action)
        : src_(std::move(src)), action_(std::move(action)) {}
    
    /**
     * Complete the conditional transition: (node - "action") >> target
     * @param target The target node
     * @return The target node for further chaining
     */
    std::shared_ptr<BaseNode> operator>>(std::shared_ptr<BaseNode> target) {
        return src_->next(std::move(target), action_);
    }
    
private:
    std::shared_ptr<BaseNode> src_;
    std::string action_;
};

/**
 * Conditional transition operator for shared_ptr: node - "action"
 * @param node The source node
 * @param action The action for conditional branching
 * @return SharedPtrConditionalTransition object for >> operator
 */
inline SharedPtrConditionalTransition operator-(std::shared_ptr<BaseNode> node, std::string action) {
    return SharedPtrConditionalTransition(std::move(node), std::move(action));
}

} // namespace pocketflow