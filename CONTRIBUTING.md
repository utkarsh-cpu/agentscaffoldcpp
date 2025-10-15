# Contributing to PocketFlow-CPP

Thank you for your interest in contributing to PocketFlow-CPP! This document provides guidelines and information for contributors.

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [Getting Started](#getting-started)
3. [Development Setup](#development-setup)
4. [Contribution Guidelines](#contribution-guidelines)
5. [Code Style](#code-style)
6. [Testing](#testing)
7. [Documentation](#documentation)
8. [Pull Request Process](#pull-request-process)

## Code of Conduct

We are committed to providing a welcoming and inclusive environment for all contributors. Please be respectful and professional in all interactions.

## Getting Started

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16 or later
- Git
- nlohmann/json library

### Development Setup

1. **Fork and Clone**
   ```bash
   git clone https://github.com/your-username/pocketflow-cpp.git
   cd pocketflow-cpp
   ```

2. **Set up Build Environment**
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make
   ```

3. **Run Tests**
   ```bash
   make test
   # or
   ctest --verbose
   ```

4. **Run Examples**
   ```bash
   ./examples/basic_sequential
   ./examples/agent_pattern
   ./examples/async_processing
   ./examples/batch_processing
   ```

## Contribution Guidelines

### Types of Contributions

We welcome various types of contributions:

- 🐛 **Bug Fixes**: Fix issues in existing code
- ✨ **New Features**: Add new node types, utilities, or capabilities
- 📚 **Documentation**: Improve docs, examples, or tutorials
- 🧪 **Tests**: Add or improve test coverage
- 🔧 **Performance**: Optimize existing code
- 🎨 **Examples**: Create new example applications

### Before You Start

1. **Check Existing Issues**: Look for existing issues or discussions
2. **Create an Issue**: For significant changes, create an issue first to discuss
3. **Small Changes**: For small fixes, you can directly create a PR

### Contribution Areas

#### High Priority
- Performance optimizations
- Additional async node types
- Integration examples with popular libraries
- Comprehensive test coverage
- Documentation improvements

#### Medium Priority
- New utility nodes (logging, metrics, etc.)
- Additional batch processing patterns
- Error handling improvements
- Memory optimization

#### Low Priority
- Code style improvements
- Minor refactoring
- Additional examples

## Code Style

### C++ Style Guidelines

We follow modern C++ best practices:

#### Naming Conventions
```cpp
// Classes: PascalCase
class AsyncBatchNode;

// Methods and variables: snake_case
void run_async();
int max_retries_;

// Constants: UPPER_SNAKE_CASE
const int MAX_RETRY_COUNT = 10;

// Namespaces: lowercase
namespace pocketflow {
```

#### Header Structure
```cpp
#pragma once

#include <memory>
#include <future>
#include <nlohmann/json.hpp>

namespace pocketflow {

using json = nlohmann::json;

/**
 * Brief description of the class
 * 
 * Detailed description if needed.
 * Usage example:
 * ```cpp
 * auto node = std::make_shared<MyNode>();
 * ```
 */
class MyNode : public BaseNode {
public:
    // Public interface first
    MyNode(int param = 0);
    
    // Override virtual methods
    json exec(const json& prep_result) override;
    
private:
    // Private members last
    int param_;
};

} // namespace pocketflow
```

#### Code Formatting
- Use 4 spaces for indentation (no tabs)
- Maximum line length: 100 characters
- Always use braces for control structures
- Prefer `auto` for complex types
- Use `const` wherever possible

#### Modern C++ Features
```cpp
// Prefer smart pointers
auto node = std::make_shared<MyNode>();

// Use auto for complex types
auto future = node->run_async(shared);

// Range-based for loops
for (const auto& item : items) {
    process(item);
}

// Move semantics
return std::move(result);

// Uniform initialization
json result{{"key", "value"}};
```

### Documentation Style

#### Class Documentation
```cpp
/**
 * @brief Brief description of the class
 * 
 * Detailed description explaining:
 * - Purpose and use cases
 * - Key features
 * - Usage patterns
 * 
 * @example
 * ```cpp
 * auto node = std::make_shared<MyNode>(param);
 * node->set_params({{"key", "value"}});
 * auto result = node->run(shared);
 * ```
 */
class MyNode : public BaseNode {
```

#### Method Documentation
```cpp
/**
 * @brief Execute the main computation logic
 * 
 * @param prep_result JSON object from prep() method
 * @return JSON object containing execution results
 * @throws std::runtime_error if computation fails
 * 
 * This method performs the core business logic of the node.
 * It should be stateless and thread-safe.
 */
json exec(const json& prep_result) override;
```

## Testing

### Test Structure

Tests are organized in the `tests/` directory:

```
tests/
├── test_base_node.cpp
├── test_node.cpp
├── test_flow.cpp
├── test_async_nodes.cpp
├── test_batch_nodes.cpp
└── test_integration.cpp
```

### Writing Tests

Use Google Test framework:

```cpp
#include <gtest/gtest.h>
#include "pocketflow/pocketflow.hpp"

class TestNode : public pocketflow::Node {
public:
    json exec(const json& prep_result) override {
        return json{{"result", prep_result["input"].get<int>() * 2}};
    }
};

TEST(NodeTest, BasicExecution) {
    json shared = {{"input", 5}};
    auto node = std::make_shared<TestNode>();
    
    node->run(shared);
    
    EXPECT_TRUE(shared.contains("result"));
    EXPECT_EQ(shared["result"], 10);
}

TEST(NodeTest, ErrorHandling) {
    json shared = {{"invalid_input", "not_a_number"}};
    auto node = std::make_shared<TestNode>();
    
    EXPECT_THROW(node->run(shared), std::exception);
}
```

### Test Categories

#### Unit Tests
- Test individual node functionality
- Test error conditions
- Test edge cases

#### Integration Tests
- Test complete flows
- Test node interactions
- Test async behavior

#### Performance Tests
- Benchmark critical paths
- Compare sync vs async performance
- Memory usage tests

### Running Tests

```bash
# All tests
make test

# Specific test
./tests/test_nodes

# With verbose output
ctest --verbose

# With coverage (if configured)
make coverage
```

## Documentation

### Types of Documentation

1. **API Documentation**: In-code documentation (Doxygen style)
2. **User Guide**: README.md and API.md
3. **Examples**: Complete working examples
4. **Tutorials**: Step-by-step guides

### Documentation Standards

- All public APIs must be documented
- Examples should be complete and runnable
- Use clear, concise language
- Include code examples where helpful
- Keep documentation up-to-date with code changes

### Building Documentation

```bash
# Generate API docs (if Doxygen configured)
make docs

# Check documentation
# Ensure all examples compile and run
cd examples && make && ./basic_sequential
```

## Pull Request Process

### Before Submitting

1. **Code Quality**
   - [ ] Code follows style guidelines
   - [ ] All tests pass
   - [ ] New code has tests
   - [ ] Documentation is updated

2. **Functionality**
   - [ ] Feature works as intended
   - [ ] No breaking changes (or properly documented)
   - [ ] Performance impact considered

3. **Review Readiness**
   - [ ] Clear commit messages
   - [ ] PR description explains changes
   - [ ] Related issues referenced

### PR Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation update
- [ ] Performance improvement
- [ ] Other (please describe)

## Testing
- [ ] Unit tests added/updated
- [ ] Integration tests pass
- [ ] Examples still work
- [ ] Performance benchmarks (if applicable)

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] No breaking changes (or documented)

## Related Issues
Fixes #123
```

### Review Process

1. **Automated Checks**: CI/CD runs tests and checks
2. **Code Review**: Maintainers review code quality and design
3. **Testing**: Reviewers test functionality
4. **Approval**: At least one maintainer approval required
5. **Merge**: Squash and merge to main branch

### After Merge

- Update local repository
- Close related issues
- Update documentation if needed
- Consider writing a blog post for significant features

## Development Workflow

### Branch Naming

- `feature/description` - New features
- `fix/description` - Bug fixes
- `docs/description` - Documentation updates
- `perf/description` - Performance improvements

### Commit Messages

Follow conventional commit format:

```
type(scope): description

[optional body]

[optional footer]
```

Examples:
```
feat(async): add AsyncParallelBatchFlow class

Add new class for parallel batch processing with async flows.
Includes comprehensive tests and examples.

Closes #45
```

```
fix(node): handle empty JSON in exec method

Previously would throw exception on empty input.
Now returns empty result gracefully.
```

### Release Process

1. **Version Bump**: Update version numbers
2. **Changelog**: Update CHANGELOG.md
3. **Tag**: Create git tag
4. **Release**: Create GitHub release
5. **Documentation**: Update docs if needed

## Getting Help

### Communication Channels

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: General questions and ideas
- **Email**: maintainers@pocketflow-cpp.org

### Resources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [nlohmann/json Documentation](https://json.nlohmann.me/)
- [CMake Documentation](https://cmake.org/documentation/)

## Recognition

Contributors will be recognized in:
- CONTRIBUTORS.md file
- Release notes for significant contributions
- GitHub contributor graphs
- Special thanks in documentation

Thank you for contributing to PocketFlow-CPP! 🚀