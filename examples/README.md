# PocketFlow-CPP Examples

This directory contains example implementations demonstrating various features and patterns of the PocketFlow-CPP framework.

## Examples Overview

### 1. basic_sequential.cpp
Demonstrates the fundamental concepts of PocketFlow:
- Basic node implementation with prep/exec/post lifecycle
- Sequential node chaining using the `>>` operator
- Shared state management with JSON
- Simple data processing pipeline

**Key Concepts:**
- Node lifecycle (prep → exec → post)
- Sequential flow execution
- Shared state updates

### 2. agent_pattern.cpp
Shows how to implement an agent pattern with conditional branching:
- Decision-making nodes with conditional logic
- Action-based flow control using `node - "action" >> target` syntax
- Dynamic flow paths based on execution results
- Loop-back patterns for iterative processing

**Key Concepts:**
- Conditional transitions
- Action-based routing
- Agent decision loops
- Dynamic flow control

### 3. async_processing.cpp
Demonstrates asynchronous execution capabilities:
- AsyncNode implementation with std::future
- Async lifecycle methods (prep_async, exec_async, post_async)
- Concurrent execution patterns
- Error handling in async contexts

**Key Concepts:**
- Asynchronous node execution
- Future-based programming
- Concurrent processing
- Async error handling

### 4. batch_processing.cpp
Shows batch and parallel processing features:
- BatchNode for sequential array processing
- AsyncParallelBatchNode for concurrent processing
- Performance comparisons between sequential and parallel execution
- Batch parameter handling

**Key Concepts:**
- Batch processing patterns
- Parallel execution
- Performance optimization
- Array data handling

## Building and Running Examples

### Prerequisites
- C++17 compatible compiler
- CMake 3.16 or later
- nlohmann/json library

### Build Instructions

From the project root directory:

```bash
mkdir build && cd build
cmake ..
make

# Run individual examples
./examples/basic_sequential
./examples/agent_pattern
./examples/async_processing
./examples/batch_processing
```

### Alternative: Build only examples

```bash
cd examples
mkdir build && cd build
cmake ..
make
```

## Example Usage Patterns

Each example is self-contained and demonstrates specific patterns:

1. **Start with basic_sequential** to understand core concepts
2. **Move to agent_pattern** to learn conditional flows
3. **Explore async_processing** for concurrent execution
4. **Study batch_processing** for performance optimization

## Extending Examples

These examples serve as templates for your own implementations:

- Copy an example as a starting point
- Modify the node implementations for your use case
- Add your own business logic to the exec() methods
- Experiment with different flow patterns

## Common Patterns Demonstrated

### Data Processing Pipeline
```cpp
auto loader = std::make_shared<DataLoader>();
auto processor = std::make_shared<DataProcessor>();
auto validator = std::make_shared<DataValidator>();
auto saver = std::make_shared<DataSaver>();

loader >> processor >> validator >> saver;
Flow pipeline(loader);
```

### Agent Decision Loop
```cpp
auto decision = std::make_shared<DecisionNode>();
auto search = std::make_shared<SearchNode>();
auto answer = std::make_shared<AnswerNode>();

decision - "search" >> search;
decision - "answer" >> answer;
search - "decide" >> decision;

Flow agent(decision);
```

### Parallel Processing
```cpp
auto parallel_processor = std::make_shared<AsyncParallelBatchNode>();
AsyncFlow parallel_flow(parallel_processor);

json shared = {{"items", json::array({"item1", "item2", "item3"})}};
auto future = parallel_flow.run_async(shared);
```

## Performance Notes

The examples include timing measurements to demonstrate:
- Sequential vs parallel processing performance
- Async vs sync execution overhead
- Memory usage patterns
- Scalability characteristics

## Troubleshooting

### Common Issues

1. **Compilation Errors**: Ensure C++17 support and nlohmann/json availability
2. **Runtime Errors**: Check JSON structure matches expected format
3. **Performance Issues**: Consider async/parallel alternatives for I/O or CPU-bound tasks

### Debug Tips

- Use JSON dump() method to inspect shared state
- Add logging to lifecycle methods to trace execution
- Use debugger to step through flow orchestration
- Check exception messages for detailed error information

## Next Steps

After exploring these examples:
1. Implement your own node types for specific use cases
2. Experiment with complex flow patterns
3. Integrate with external libraries (HTTP clients, databases, etc.)
4. Build production applications using PocketFlow patterns