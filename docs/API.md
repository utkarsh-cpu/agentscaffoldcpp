# PocketFlow-CPP API Reference

## Table of Contents

1. [Core Classes](#core-classes)
2. [Node Lifecycle](#node-lifecycle)
3. [Graph Building](#graph-building)
4. [Async Processing](#async-processing)
5. [Batch Processing](#batch-processing)
6. [Error Handling](#error-handling)
7. [Shared State Management](#shared-state-management)
8. [Performance Considerations](#performance-considerations)

## Core Classes

### BaseNode

The fundamental building block of PocketFlow graphs. All other node types inherit from BaseNode.

```cpp
class BaseNode {
public:
    BaseNode() = default;
    virtual ~BaseNode() = default;
    
    // Parameter management
    void set_params(const json& params);
    
    // Graph building
    std::shared_ptr<BaseNode> next(std::shared_ptr<BaseNode> node, const std::string& action = "default");
    
    // Lifecycle methods (override in derived classes)
    virtual json prep(const json& shared) { return json{}; }
    virtual json exec(const json& prep_res) { return json{}; }
    virtual json post(const json& shared, const json& prep_res, const json& exec_res) { return json{}; }
    
    // Execution methods
    json run(json& shared);
    
    // Operator overloading for fluent API
    std::shared_ptr<BaseNode> operator>>(std::shared_ptr<BaseNode> other);
    ConditionalTransition operator-(const std::string& action);
    
protected:
    json params_;
    std::unordered_map<std::string, std::shared_ptr<BaseNode>> successors_;
    virtual json _exec(const json& prep_res) { return exec(prep_res); }
    json _run(json& shared);
};
```

#### Methods

- **`set_params(params)`**: Set node-specific parameters
- **`next(node, action)`**: Add a successor node with optional action
- **`prep(shared)`**: Override to prepare input data from shared state
- **`exec(prep_result)`**: Override to implement main computation logic
- **`post(shared, prep_result, exec_result)`**: Override to process results and update shared state
- **`run(shared)`**: Execute the node with given shared state
- **`operator>>(other)`**: Chain nodes sequentially
- **`operator-(action)`**: Create conditional transition

### Node

Extends BaseNode with retry logic and error handling.

```cpp
class Node : public BaseNode {
public:
    Node(int max_retries = 1, int wait = 0);
    
    // Fallback method for handling failures
    virtual json exec_fallback(const json& prep_res, const std::exception& exc);
    
protected:
    json _exec(const json& prep_res) override;
    
private:
    int max_retries_;
    int wait_;
    int cur_retry_ = 0;
};
```

#### Constructor Parameters

- **`max_retries`**: Maximum number of retry attempts (default: 1)
- **`wait`**: Wait time in milliseconds between retries (default: 0)

#### Methods

- **`exec_fallback(prep_res, exc)`**: Override to handle failures after all retries exhausted

### BatchNode

Processes arrays of items sequentially.

```cpp
class BatchNode : public Node {
public:
    BatchNode(int max_retries = 1, int wait = 0);
    
protected:
    json _exec(const json& items) override;
};
```

The `exec()` method in BatchNode processes individual items from the array. Override `exec()` to define per-item processing logic.

### Flow

Orchestrates graph execution and manages node transitions.

```cpp
class Flow : public BaseNode {
public:
    Flow(std::shared_ptr<BaseNode> start = nullptr);
    
    // Set start node
    std::shared_ptr<BaseNode> start(std::shared_ptr<BaseNode> start_node);
    
    // Get next node based on action
    std::shared_ptr<BaseNode> get_next_node(std::shared_ptr<BaseNode> curr, const std::string& action);
    
    // Main orchestration logic
    json _orch(json& shared, const json& params = json{});
    
    // Override _run to use orchestration
    json _run(json& shared) override;
    
private:
    std::shared_ptr<BaseNode> start_node_;
};
```

#### Methods

- **`start(start_node)`**: Set the starting node for the flow
- **`get_next_node(curr, action)`**: Get next node based on current node and action
- **`_orch(shared, params)`**: Internal orchestration method

### BatchFlow

Processes batches of parameters through flows.

```cpp
class BatchFlow : public Flow {
public:
    BatchFlow(std::shared_ptr<BaseNode> start = nullptr);
    
protected:
    json _run(json& shared) override;
};
```

The `prep()` method should return an array of parameter objects. Each parameter object will be processed through the flow.

## Async Processing

### AsyncNode

Base class for asynchronous node execution using std::future.

```cpp
class AsyncNode : public Node {
public:
    AsyncNode(int max_retries = 1, int wait = 0);
    
    // Async lifecycle methods
    virtual std::future<json> prep_async(const json& shared);
    virtual std::future<json> exec_async(const json& prep_res) = 0;
    virtual std::future<json> exec_fallback_async(const json& prep_res, const std::exception& exc);
    virtual std::future<json> post_async(const json& shared, const json& prep_res, const json& exec_res);
    
    // Async execution
    std::future<json> run_async(json& shared);
    std::future<json> _run_async(json& shared);
    
    // Override sync _run to throw error
    json _run(json& shared) override;
    
protected:
    std::future<json> _exec_async(const json& prep_res);
};
```

#### Methods

- **`prep_async(shared)`**: Override for async preparation
- **`exec_async(prep_res)`**: Override for async execution (pure virtual)
- **`exec_fallback_async(prep_res, exc)`**: Override for async error handling
- **`post_async(shared, prep_res, exec_res)`**: Override for async post-processing
- **`run_async(shared)`**: Execute node asynchronously
- **`_run_async(shared)`**: Internal async execution method

### AsyncBatchNode

Sequential asynchronous batch processing.

```cpp
class AsyncBatchNode : public AsyncNode, public BatchNode {
public:
    AsyncBatchNode(int max_retries = 1, int wait = 0);
    
protected:
    std::future<json> _exec_async(const json& items) override;
};
```

Processes array items sequentially using async execution.

### AsyncParallelBatchNode

Parallel asynchronous batch processing.

```cpp
class AsyncParallelBatchNode : public AsyncNode, public BatchNode {
public:
    AsyncParallelBatchNode(int max_retries = 1, int wait = 0);
    
protected:
    std::future<json> _exec_async(const json& items) override;
};
```

Processes array items in parallel using multiple threads.

### AsyncFlow

Asynchronous flow orchestration.

```cpp
class AsyncFlow : public Flow, public AsyncNode {
public:
    AsyncFlow(std::shared_ptr<BaseNode> start = nullptr);
    
    // Async orchestration
    std::future<json> _orch_async(json& shared, const json& params = json{});
    
    // Override async _run
    std::future<json> _run_async(json& shared) override;
};
```

### AsyncBatchFlow

Sequential asynchronous batch flow processing.

```cpp
class AsyncBatchFlow : public AsyncFlow, public BatchFlow {
public:
    AsyncBatchFlow(std::shared_ptr<BaseNode> start = nullptr);
    
protected:
    std::future<json> _run_async(json& shared) override;
};
```

### AsyncParallelBatchFlow

Parallel asynchronous batch flow processing.

```cpp
class AsyncParallelBatchFlow : public AsyncFlow, public BatchFlow {
public:
    AsyncParallelBatchFlow(std::shared_ptr<BaseNode> start = nullptr);
    
protected:
    std::future<json> _run_async(json& shared) override;
};
```

## Node Lifecycle

### Synchronous Lifecycle

1. **prep(shared)** → **exec(prep_result)** → **post(shared, prep_result, exec_result)**

### Asynchronous Lifecycle

1. **prep_async(shared)** → **exec_async(prep_result)** → **post_async(shared, prep_result, exec_result)**

### Lifecycle Method Details

#### prep() / prep_async()
- **Purpose**: Extract and prepare input data from shared state
- **Input**: `shared` - The shared state JSON object
- **Output**: JSON object containing prepared data for exec()
- **When to override**: When you need to extract specific data from shared state

#### exec() / exec_async()
- **Purpose**: Perform the main computation or processing
- **Input**: `prep_result` - Output from prep() method
- **Output**: JSON object containing execution results
- **When to override**: Always - this contains your main business logic

#### post() / post_async()
- **Purpose**: Process results and update shared state, determine next action
- **Input**: 
  - `shared` - The shared state JSON object
  - `prep_result` - Output from prep() method
  - `exec_result` - Output from exec() method
- **Output**: String or JSON indicating next action for flow control
- **When to override**: When you need to update shared state or control flow

## Graph Building

### Sequential Chaining

Use the `>>` operator to chain nodes sequentially:

```cpp
auto node1 = std::make_shared<Node1>();
auto node2 = std::make_shared<Node2>();
auto node3 = std::make_shared<Node3>();

// Chain nodes: node1 >> node2 >> node3
node1 >> node2 >> node3;

Flow flow(node1);
```

### Conditional Branching

Use the `-` operator with action strings for conditional transitions:

```cpp
auto decision = std::make_shared<DecisionNode>();
auto path_a = std::make_shared<PathANode>();
auto path_b = std::make_shared<PathBNode>();

// Conditional transitions
decision - "approve" >> path_a;
decision - "reject" >> path_b;

Flow flow(decision);
```

### ConditionalTransition Helper

The `operator-` returns a ConditionalTransition object:

```cpp
class ConditionalTransition {
public:
    ConditionalTransition(std::shared_ptr<BaseNode> src, const std::string& action);
    std::shared_ptr<BaseNode> operator>>(std::shared_ptr<BaseNode> target);
};
```

## Error Handling

### Retry Mechanism

Nodes support automatic retry with configurable parameters:

```cpp
// Node with 3 retries and 100ms wait between retries
auto node = std::make_shared<MyNode>(3, 100);
```

### Fallback Handling

Override `exec_fallback()` for graceful error handling:

```cpp
class RobustNode : public Node {
public:
    json exec_fallback(const json& prep_res, const std::exception& exc) override {
        std::cout << "Fallback: " << exc.what() << std::endl;
        return json{{"fallback_result", true}};
    }
};
```

### Exception Types

```cpp
class FlowException : public std::exception {
    // Base exception for flow-related errors
};

class NodeExecutionException : public FlowException {
    // Specific to node execution failures
};
```

## Shared State Management

### JSON-Based State

PocketFlow-CPP uses `nlohmann::json` for shared state, exactly matching Python's dict-based approach:

```cpp
json shared = {
    {"input_data", "some value"},
    {"results", json::array()},
    {"context", json::object()}
};

// Access and modify shared state
shared["new_key"] = "new_value";
auto value = shared["existing_key"];
```

### State Modification in Nodes

```cpp
json post(const json& shared, const json& prep_result, const json& exec_result) override {
    // Modify shared state (cast away const)
    const_cast<json&>(shared)["output"] = exec_result;
    const_cast<json&>(shared)["timestamp"] = get_current_timestamp();
    
    return json{};  // Return action for flow control
}
```

### Thread Safety

- Shared state is passed by reference and should be modified carefully in concurrent contexts
- AsyncParallelBatchNode and AsyncParallelBatchFlow handle thread safety internally
- For custom concurrent access, use appropriate synchronization mechanisms

## Performance Considerations

### Memory Management

- Use `std::shared_ptr` for node management
- Minimize JSON copying with move semantics where possible
- Consider object pooling for high-frequency operations

### Async Performance

- Use async nodes for I/O-bound operations
- Use parallel batch nodes for CPU-bound operations
- Consider thread pool size for parallel operations

### Optimization Tips

1. **Minimize JSON Serialization**: Keep data in native C++ types when possible
2. **Batch Operations**: Use batch nodes for processing multiple items
3. **Async for I/O**: Use async nodes for network calls, file I/O, etc.
4. **Parallel for CPU**: Use parallel batch nodes for CPU-intensive tasks
5. **Memory Pools**: Consider custom allocators for high-frequency scenarios

### Benchmarking

```cpp
auto start = std::chrono::high_resolution_clock::now();
flow.run(shared);
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "Execution time: " << duration.count() << "ms" << std::endl;
```

## Best Practices

### Node Design

1. **Single Responsibility**: Each node should have one clear purpose
2. **Stateless**: Nodes should be stateless; use shared state for data
3. **Error Handling**: Always consider failure cases and provide fallbacks
4. **Documentation**: Document expected input/output formats

### Flow Design

1. **Clear Entry Points**: Use descriptive start nodes
2. **Explicit Actions**: Use meaningful action strings for branching
3. **Error Paths**: Design flows with error handling paths
4. **Testing**: Test individual nodes and complete flows

### Performance

1. **Profile First**: Measure before optimizing
2. **Async for I/O**: Use async nodes for I/O-bound operations
3. **Batch for Scale**: Use batch processing for large datasets
4. **Memory Awareness**: Monitor memory usage in long-running flows

### Code Organization

```cpp
// Recommended project structure
include/
  pocketflow/
    pocketflow.hpp          // Main header
    base_node.hpp
    node.hpp
    flow.hpp
    // ... other headers
examples/
  basic_sequential.cpp
  agent_pattern.cpp
  // ... other examples
tests/
  test_nodes.cpp
  test_flows.cpp
  // ... test files
```