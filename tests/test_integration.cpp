#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <vector>
#include <random>
#include <iostream>
#include "../include/pocketflow/pocketflow.hpp"

using json = nlohmann::json;
using namespace pocketflow;

/**
 * Integration Test Suite for PocketFlow-CPP
 * 
 * This test suite covers:
 * 1. Complete workflows end-to-end
 * 2. Interoperability between sync and async components
 * 3. Performance benchmarking
 */

// Test helper nodes for integration testing
class TestDataLoader : public Node {
public:
    TestDataLoader(int delay_ms = 50) : Node(1, 0), delay_ms_(delay_ms) {}
    
    json prep(const json& shared) override {
        return json{{"source", shared.value("data_source", "test_data")}};
    }
    
    json exec(const json& prep_result) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_));
        return json{
            {"data", json::array({1, 2, 3, 4, 5})},
            {"metadata", {{"source", prep_result["source"]}, {"count", 5}}}
        };
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        const_cast<json&>(shared)["loaded_data"] = exec_result["data"];
        const_cast<json&>(shared)["load_metadata"] = exec_result["metadata"];
        return json{};
    }
    
private:
    int delay_ms_;
};

class TestDataProcessor : public Node {
public:
    TestDataProcessor(int delay_ms = 100) : Node(2, 50), delay_ms_(delay_ms) {}
    
    json prep(const json& shared) override {
        return json{
            {"data", shared["loaded_data"]},
            {"multiplier", shared.value("multiplier", 2)}
        };
    }
    
    json exec(const json& prep_result) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_));
        
        json data = prep_result["data"];
        int multiplier = prep_result["multiplier"];
        json processed = json::array();
        
        for (const auto& item : data) {
            processed.push_back(item.get<int>() * multiplier);
        }
        
        return json{{"processed_data", processed}};
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        const_cast<json&>(shared)["processed_data"] = exec_result["processed_data"];
        return json{};
    }
    
private:
    int delay_ms_;
};

class TestAsyncProcessor : public AsyncNode {
public:
    TestAsyncProcessor(int delay_ms = 200) : AsyncNode(1, 0), delay_ms_(delay_ms) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [shared]() {
            return json{{"input", shared.value("async_input", "default")}};
        });
    }
    
    std::future<json> exec_async(const json& prep_result) override {
        return std::async(std::launch::async, [this, prep_result]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_));
            std::string input = prep_result["input"];
            return json{{"async_result", "processed_" + input}};
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_result, const json& exec_result) override {
        return std::async(std::launch::async, [&shared, exec_result]() -> json {
            json& mutable_shared = const_cast<json&>(shared);
            mutable_shared["async_result"] = exec_result["async_result"];
            return json{};
        });
    }
    
private:
    int delay_ms_;
};

class TestBatchProcessor : public BatchNode {
public:
    TestBatchProcessor() : BatchNode(1, 0) {}
    
    json prep(const json& shared) override {
        return shared["batch_data"];
    }
    
    json exec(const json& item) override {
        // Process individual item
        return json{{"item_result", item.get<int>() * 10}};
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        const_cast<json&>(shared)["batch_results"] = exec_result;
        return json{};
    }
};

class TestDecisionNode : public Node {
public:
    TestDecisionNode() : Node(1, 0) {}
    
    json prep(const json& shared) override {
        return json{{"condition", shared.value("condition", "default")}};
    }
    
    json exec(const json& prep_result) override {
        std::string condition = prep_result["condition"];
        return json{{"decision", condition}};
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        const_cast<json&>(shared)["decision_made"] = exec_result["decision"];
        return exec_result["decision"];  // Return action for branching
    }
};

class TestActionNodeA : public Node {
public:
    TestActionNodeA() : Node(1, 0) {}
    
    json exec(const json& prep_result) override {
        return json{{"path", "A"}};
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        const_cast<json&>(shared)["path_taken"] = "A";
        return json{};
    }
};

class TestActionNodeB : public Node {
public:
    TestActionNodeB() : Node(1, 0) {}
    
    json exec(const json& prep_result) override {
        return json{{"path", "B"}};
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        const_cast<json&>(shared)["path_taken"] = "B";
        return json{};
    }
};

// Integration Test Class
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset shared state for each test
        shared_state = json::object();
    }
    
    json shared_state;
};

/**
 * Test 1: Complete Sequential Workflow End-to-End
 */
TEST_F(IntegrationTest, CompleteSequentialWorkflow) {
    // Setup shared state
    shared_state = {
        {"data_source", "integration_test"},
        {"multiplier", 3}
    };
    
    // Create nodes
    auto loader = std::make_shared<TestDataLoader>(30);
    auto processor = std::make_shared<TestDataProcessor>(50);
    
    // Build sequential flow
    loader >> processor;
    
    // Execute flow
    Flow workflow(loader);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    workflow.run(shared_state);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Verify results
    ASSERT_TRUE(shared_state.contains("loaded_data"));
    ASSERT_TRUE(shared_state.contains("processed_data"));
    
    json loaded = shared_state["loaded_data"];
    json processed = shared_state["processed_data"];
    
    EXPECT_EQ(loaded.size(), 5);
    EXPECT_EQ(processed.size(), 5);
    
    // Verify processing logic (multiply by 3)
    for (size_t i = 0; i < loaded.size(); ++i) {
        EXPECT_EQ(processed[i], loaded[i].get<int>() * 3);
    }
    
    // Verify metadata
    ASSERT_TRUE(shared_state.contains("load_metadata"));
    EXPECT_EQ(shared_state["load_metadata"]["source"], "integration_test");
    EXPECT_EQ(shared_state["load_metadata"]["count"], 5);
    
    // Performance check (should complete in reasonable time)
    EXPECT_LT(duration.count(), 500);  // Should complete in under 500ms (CI-friendly)
    
    std::cout << "Sequential workflow completed in " << duration.count() << "ms\n";
}

/**
 * Test 2: Conditional Branching Workflow
 */
TEST_F(IntegrationTest, ConditionalBranchingWorkflow) {
    // Test path A
    {
        shared_state = {{"condition", "path_a"}};
        
        auto decision = std::make_shared<TestDecisionNode>();
        auto action_a = std::make_shared<TestActionNodeA>();
        auto action_b = std::make_shared<TestActionNodeB>();
        
        // Setup conditional branching
        decision - "path_a" >> action_a;
        decision - "path_b" >> action_b;
        
        Flow workflow(decision);
        workflow.run(shared_state);
        
        EXPECT_EQ(shared_state["decision_made"], "path_a");
        EXPECT_EQ(shared_state["path_taken"], "A");
    }
    
    // Test path B
    {
        shared_state = {{"condition", "path_b"}};
        
        auto decision = std::make_shared<TestDecisionNode>();
        auto action_a = std::make_shared<TestActionNodeA>();
        auto action_b = std::make_shared<TestActionNodeB>();
        
        decision - "path_a" >> action_a;
        decision - "path_b" >> action_b;
        
        Flow workflow(decision);
        workflow.run(shared_state);
        
        EXPECT_EQ(shared_state["decision_made"], "path_b");
        EXPECT_EQ(shared_state["path_taken"], "B");
    }
}

/**
 * Test 3: Batch Processing Workflow
 */
TEST_F(IntegrationTest, BatchProcessingWorkflow) {
    shared_state = {
        {"batch_data", json::array({1, 2, 3, 4, 5})}
    };
    
    auto batch_processor = std::make_shared<TestBatchProcessor>();
    Flow workflow(batch_processor);
    
    workflow.run(shared_state);
    
    ASSERT_TRUE(shared_state.contains("batch_results"));
    json results = shared_state["batch_results"];
    
    EXPECT_EQ(results.size(), 5);
    
    // Verify each item was processed (multiplied by 10)
    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_EQ(results[i]["item_result"], (i + 1) * 10);
    }
}

/**
 * Test 4: Async Node Integration
 */
TEST_F(IntegrationTest, AsyncNodeIntegration) {
    shared_state = {
        {"async_input", "test_data"}
    };
    
    auto async_processor = std::make_shared<TestAsyncProcessor>(100);
    AsyncFlow async_workflow(async_processor);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto future = async_workflow.run_async(shared_state);
    
    // Verify we can do other work while waiting
    bool other_work_done = false;
    std::thread worker([&other_work_done]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        other_work_done = true;
    });
    
    future.get();  // Wait for async completion
    worker.join();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_TRUE(other_work_done);
    ASSERT_TRUE(shared_state.contains("async_result"));
    EXPECT_EQ(shared_state["async_result"], "processed_test_data");
    
    // Should complete in reasonable time but allow for async overhead
    EXPECT_LT(duration.count(), 1000);  // Allow more time for async operations in CI
    
    std::cout << "Async workflow completed in " << duration.count() << "ms\n";
}

/**
 * Test 5: Mixed Sync/Async Interoperability
 */
TEST_F(IntegrationTest, MixedSyncAsyncInteroperability) {
    shared_state = {
        {"data_source", "mixed_test"},
        {"multiplier", 2},
        {"async_input", "mixed_data"}
    };
    
    // Create mixed workflow: Sync -> Async -> Sync
    auto sync_loader = std::make_shared<TestDataLoader>(30);
    auto async_processor = std::make_shared<TestAsyncProcessor>(80);
    auto sync_finalizer = std::make_shared<TestDataProcessor>(40);
    
    // Build mixed flow (this tests interoperability)
    sync_loader >> async_processor >> sync_finalizer;
    
    // Use AsyncFlow to handle mixed sync/async nodes
    AsyncFlow mixed_workflow(sync_loader);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto future = mixed_workflow.run_async(shared_state);
    future.get();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Verify all stages completed
    ASSERT_TRUE(shared_state.contains("loaded_data"));
    ASSERT_TRUE(shared_state.contains("async_result"));
    ASSERT_TRUE(shared_state.contains("processed_data"));
    
    // Verify data flow
    EXPECT_EQ(shared_state["loaded_data"].size(), 5);
    EXPECT_EQ(shared_state["async_result"], "processed_mixed_data");
    EXPECT_EQ(shared_state["processed_data"].size(), 5);
    
    std::cout << "Mixed sync/async workflow completed in " << duration.count() << "ms\n";
}

/**
 * Test 6: Nested Flow Integration
 */
TEST_F(IntegrationTest, NestedFlowIntegration) {
    shared_state = {
        {"data_source", "nested_test"},
        {"multiplier", 4}
    };
    
    // Create sub-flow
    auto sub_loader = std::make_shared<TestDataLoader>(20);
    auto sub_processor = std::make_shared<TestDataProcessor>(30);
    sub_loader >> sub_processor;
    auto sub_flow = std::make_shared<Flow>(sub_loader);
    
    // Create main flow that includes sub-flow
    auto main_start = std::make_shared<TestDataLoader>(10);
    auto main_end = std::make_shared<TestDataProcessor>(20);
    
    main_start >> sub_flow >> main_end;
    
    Flow main_workflow(main_start);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    main_workflow.run(shared_state);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Verify nested execution
    ASSERT_TRUE(shared_state.contains("loaded_data"));
    ASSERT_TRUE(shared_state.contains("processed_data"));
    
    // Data should be processed multiple times through the nested flows
    json final_data = shared_state["processed_data"];
    EXPECT_EQ(final_data.size(), 5);
    
    std::cout << "Nested flow completed in " << duration.count() << "ms\n";
}

/**
 * Test 7: Error Handling and Recovery
 */
class FailingNode : public Node {
public:
    FailingNode(int fail_count = 2) : Node(3, 10), fail_count_(fail_count), attempt_count_(0) {}
    
    json exec(const json& prep_result) override {
        attempt_count_++;
        if (attempt_count_ <= fail_count_) {
            throw std::runtime_error("Simulated failure attempt " + std::to_string(attempt_count_));
        }
        return json{{"success", true}, {"attempts", attempt_count_}};
    }
    
    json exec_fallback(const json& prep_result, const std::exception& exc) override {
        return json{{"fallback", true}, {"error", exc.what()}, {"attempts", attempt_count_}};
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        const_cast<json&>(shared)["error_test_result"] = exec_result;
        return json{};
    }
    
private:
    int fail_count_;
    int attempt_count_;
};

TEST_F(IntegrationTest, ErrorHandlingAndRecovery) {
    // Test successful retry
    {
        shared_state = json::object();
        auto failing_node = std::make_shared<FailingNode>(2);  // Fail 2 times, succeed on 3rd
        Flow workflow(failing_node);
        
        workflow.run(shared_state);
        
        ASSERT_TRUE(shared_state.contains("error_test_result"));
        json result = shared_state["error_test_result"];
        
        EXPECT_TRUE(result["success"].get<bool>());
        EXPECT_EQ(result["attempts"], 3);
    }
    
    // Test fallback execution
    {
        shared_state = json::object();
        auto failing_node = std::make_shared<FailingNode>(5);  // Fail more than max retries
        Flow workflow(failing_node);
        
        workflow.run(shared_state);
        
        ASSERT_TRUE(shared_state.contains("error_test_result"));
        json result = shared_state["error_test_result"];
        
        EXPECT_TRUE(result["fallback"].get<bool>());
        EXPECT_EQ(result["attempts"], 3);  // Should stop at max retries
        EXPECT_TRUE(result.contains("error"));
    }
}

/**
 * Test 8: Performance Benchmarking
 */
class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Prepare test data
        test_data = json::array();
        for (int i = 0; i < 100; ++i) {
            test_data.push_back(i);
        }
    }
    
    json test_data;
};

TEST_F(PerformanceTest, SequentialVsParallelPerformance) {
    const int num_iterations = 100;  // Increased for more measurable timing
    
    // Sequential processing benchmark
    auto sequential_start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_iterations; ++i) {
        json shared = {{"batch_data", test_data}};
        auto batch_node = std::make_shared<TestBatchProcessor>();
        Flow sequential_flow(batch_node);
        sequential_flow.run(shared);
    }
    
    auto sequential_end = std::chrono::high_resolution_clock::now();
    auto sequential_duration = std::chrono::duration_cast<std::chrono::microseconds>(sequential_end - sequential_start);
    
    // Async processing benchmark (simulating parallel processing)
    auto async_start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_iterations; ++i) {
        futures.push_back(std::async(std::launch::async, [this]() {
            json shared = {{"async_input", "perf_test"}};
            auto async_node = std::make_shared<TestAsyncProcessor>(10);
            AsyncFlow async_flow(async_node);
            auto future = async_flow.run_async(shared);
            future.get();
        }));
    }
    
    // Wait for all async operations to complete
    for (auto& future : futures) {
        future.get();
    }
    
    auto async_end = std::chrono::high_resolution_clock::now();
    auto async_duration = std::chrono::duration_cast<std::chrono::microseconds>(async_end - async_start);
    
    // Performance analysis
    std::cout << "Performance Benchmark Results:\n";
    std::cout << "Sequential processing: " << sequential_duration.count() << "μs\n";
    std::cout << "Async processing: " << async_duration.count() << "μs\n";
    
    if (async_duration.count() > 0) {
        double speedup = static_cast<double>(sequential_duration.count()) / async_duration.count();
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x\n";
    }
    
    // Basic performance expectations - both should take some measurable time
    EXPECT_GT(sequential_duration.count(), 0);
    EXPECT_GT(async_duration.count(), 0);
    
    // Performance comparison - allow for significant variance in timing
    // This is a loose check since performance can vary based on system load
    if (sequential_duration.count() > 0 && async_duration.count() > 0) {
        // In CI environments, async can be much slower due to overhead
        // Just ensure both complete in reasonable time (within 100x of each other)
        EXPECT_LT(async_duration.count(), sequential_duration.count() * 100);
        EXPECT_LT(sequential_duration.count(), async_duration.count() * 100);
    }
}

/**
 * Test 9: Memory and Resource Management
 */
TEST_F(IntegrationTest, MemoryAndResourceManagement) {
    const int num_nodes = 100;
    const int num_iterations = 50;
    
    // Create a large workflow to test memory management
    std::vector<std::shared_ptr<Node>> nodes;
    
    for (int i = 0; i < num_nodes; ++i) {
        nodes.push_back(std::make_shared<TestDataProcessor>(1));  // Minimal delay
    }
    
    // Chain all nodes together
    for (size_t i = 0; i < nodes.size() - 1; ++i) {
        nodes[i] >> nodes[i + 1];
    }
    
    // Run multiple iterations to test for memory leaks
    for (int iter = 0; iter < num_iterations; ++iter) {
        json shared = {
            {"loaded_data", json::array({1, 2, 3})},
            {"multiplier", 1}
        };
        
        Flow large_workflow(nodes[0]);
        large_workflow.run(shared);
        
        // Verify the workflow still works correctly
        ASSERT_TRUE(shared.contains("processed_data"));
        EXPECT_EQ(shared["processed_data"].size(), 3);
    }
    
    // If we reach here without crashes or excessive memory usage, the test passes
    SUCCEED();
}

/**
 * Test 10: Thread Safety and Concurrent Execution
 */
TEST_F(IntegrationTest, ThreadSafetyAndConcurrentExecution) {
    const int num_threads = 10;
    const int iterations_per_thread = 20;
    
    std::vector<std::future<bool>> futures;
    std::atomic<int> success_count{0};
    
    // Launch multiple threads executing workflows concurrently
    for (int t = 0; t < num_threads; ++t) {
        futures.push_back(std::async(std::launch::async, [&, t]() -> bool {
            try {
                for (int i = 0; i < iterations_per_thread; ++i) {
                    json thread_shared = {
                        {"data_source", "thread_" + std::to_string(t)},
                        {"multiplier", t + 1},
                        {"iteration", i}
                    };
                    
                    auto loader = std::make_shared<TestDataLoader>(5);
                    auto processor = std::make_shared<TestDataProcessor>(10);
                    loader >> processor;
                    
                    Flow thread_workflow(loader);
                    thread_workflow.run(thread_shared);
                    
                    // Verify results
                    if (!thread_shared.contains("processed_data") || 
                        thread_shared["processed_data"].size() != 5) {
                        return false;
                    }
                }
                success_count++;
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Thread " << t << " failed: " << e.what() << "\n";
                return false;
            }
        }));
    }
    
    // Wait for all threads to complete
    bool all_successful = true;
    for (auto& future : futures) {
        if (!future.get()) {
            all_successful = false;
        }
    }
    
    EXPECT_TRUE(all_successful);
    EXPECT_EQ(success_count.load(), num_threads);
    
    std::cout << "Concurrent execution test: " << success_count.load() 
              << "/" << num_threads << " threads successful\n";
}

/**
 * Test 11: Complex Real-World Scenario
 */
TEST_F(IntegrationTest, ComplexRealWorldScenario) {
    // Simulate a complex LLM workflow: Data ingestion -> Processing -> Decision -> Action
    shared_state = {
        {"documents", json::array({"doc1.txt", "doc2.txt", "doc3.txt"})},
        {"processing_mode", "enhanced"},
        {"confidence_threshold", 0.8}
    };
    
    // Create a complex workflow with multiple paths
    auto data_loader = std::make_shared<TestDataLoader>(50);
    auto batch_processor = std::make_shared<TestBatchProcessor>();
    auto decision_node = std::make_shared<TestDecisionNode>();
    auto action_a = std::make_shared<TestActionNodeA>();
    auto action_b = std::make_shared<TestActionNodeB>();
    auto async_finalizer = std::make_shared<TestAsyncProcessor>(100);
    
    // Build complex workflow
    data_loader >> batch_processor >> decision_node;
    decision_node - "path_a" >> action_a >> async_finalizer;
    decision_node - "path_b" >> action_b >> async_finalizer;
    
    // Set up for path A execution
    shared_state["condition"] = "path_a";
    shared_state["batch_data"] = json::array({10, 20, 30});
    shared_state["async_input"] = "final_processing";
    
    AsyncFlow complex_workflow(data_loader);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto future = complex_workflow.run_async(shared_state);
    future.get();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Verify complete workflow execution
    ASSERT_TRUE(shared_state.contains("loaded_data"));
    ASSERT_TRUE(shared_state.contains("batch_results"));
    ASSERT_TRUE(shared_state.contains("decision_made"));
    ASSERT_TRUE(shared_state.contains("path_taken"));
    ASSERT_TRUE(shared_state.contains("async_result"));
    
    EXPECT_EQ(shared_state["decision_made"], "path_a");
    EXPECT_EQ(shared_state["path_taken"], "A");
    EXPECT_EQ(shared_state["async_result"], "processed_final_processing");
    
    std::cout << "Complex real-world scenario completed in " << duration.count() << "ms\n";
}

/**
 * Main test runner with performance reporting
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "=== PocketFlow-CPP Integration Test Suite ===\n";
    std::cout << "Testing complete workflows, interoperability, and performance\n\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    int result = RUN_ALL_TESTS();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n=== Integration Test Suite Completed ===\n";
    std::cout << "Total execution time: " << total_duration.count() << "ms\n";
    
    return result;
}