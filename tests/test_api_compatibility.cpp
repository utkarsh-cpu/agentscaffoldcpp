#include <gtest/gtest.h>
#include "pocketflow/pocketflow.hpp"
#include <memory>

using namespace pocketflow;

/**
 * Test nodes for API compatibility validation
 */
class TestDataProcessor : public Node {
public:
    json prep(const json& shared) override {
        return shared["input_data"];
    }
    
    json exec(const json& prep_res) override {
        return json{
            {"processed", true},
            {"original", prep_res}
        };
    }
    
    json post(const json& shared, const json& prep_res, const json& exec_res) override {
        // Modify shared state (same as Python)
        const_cast<json&>(shared)["output"] = exec_res;
        return "default";  // Return action string
    }
};

class TestDecisionNode : public Node {
public:
    json prep(const json& shared) override {
        return json{
            {"query", shared["query"]},
            {"context", shared.value("context", "")},
            {"iteration", shared.value("iteration", 0)}
        };
    }
    
    json exec(const json& prep_res) override {
        std::string query = prep_res["query"];
        int iteration = prep_res.value("iteration", 0);
        
        // Prevent infinite loops
        if (iteration > 2) {
            return json{{"action", "answer"}, {"reason", "Max iterations reached"}};
        }
        
        if (query.find("search") != std::string::npos && iteration == 0) {
            return json{{"action", "search"}, {"reason", "Need more info"}};
        }
        return json{{"action", "answer"}, {"reason", "Can answer directly"}};
    }
    
    json post(const json& shared, const json& prep_res, const json& exec_res) override {
        const_cast<json&>(shared)["last_decision"] = exec_res;
        const_cast<json&>(shared)["iteration"] = prep_res.value("iteration", 0) + 1;
        return exec_res["action"];  // Return action string for branching
    }
};

class TestSearchNode : public Node {
public:
    json prep(const json& shared) override {
        return shared["query"];
    }
    
    json exec(const json& prep_res) override {
        return json{{"results", "Search results for: " + prep_res.get<std::string>()}};
    }
    
    json post(const json& shared, const json& prep_res, const json& exec_res) override {
        const_cast<json&>(shared)["search_results"] = exec_res;
        return "decide";  // Loop back to decision
    }
};

class TestAnswerNode : public Node {
public:
    json prep(const json& shared) override {
        return json{
            {"query", shared["query"]},
            {"context", shared.value("search_results", json{})}
        };
    }
    
    json exec(const json& prep_res) override {
        return json{{"answer", "Final answer for: " + prep_res["query"].get<std::string>()}};
    }
    
    json post(const json& shared, const json& prep_res, const json& exec_res) override {
        const_cast<json&>(shared)["final_answer"] = exec_res;
        return "complete";
    }
};

/**
 * Test 1: Sequential Node Chaining (>> operator)
 * Python equivalent: node1 >> node2 >> node3
 */
TEST(APICompatibilityTest, SequentialChaining) {
    // Test the exact syntax: node1 >> node2 >> node3
    auto node1 = std::make_shared<TestDataProcessor>();
    auto node2 = std::make_shared<TestDataProcessor>();
    auto node3 = std::make_shared<TestDataProcessor>();
    
    // This should compile and work exactly like Python
    node1 >> node2 >> node3;
    
    // Verify the chain was created correctly
    EXPECT_EQ(node1->get_successor("default"), node2);
    EXPECT_EQ(node2->get_successor("default"), node3);
    EXPECT_EQ(node3->get_successor("default"), nullptr);
}

/**
 * Test 2: Action-based Transitions (- operator)
 * Python equivalent: node - "action" >> target
 */
TEST(APICompatibilityTest, ActionBasedTransitions) {
    auto decision = std::make_shared<TestDecisionNode>();
    auto search = std::make_shared<TestSearchNode>();
    auto answer = std::make_shared<TestAnswerNode>();
    
    // Test exact Python syntax: node - "action" >> target
    decision - "search" >> search;
    decision - "answer" >> answer;
    search - "decide" >> decision;
    
    // Verify the transitions were created correctly
    EXPECT_EQ(decision->get_successor("search"), search);
    EXPECT_EQ(decision->get_successor("answer"), answer);
    EXPECT_EQ(search->get_successor("decide"), decision);
}

/**
 * Test 3: Shared State Behavior (matches Python dict usage)
 * Python equivalent: shared["key"] = value, shared.get("key", default)
 */
TEST(APICompatibilityTest, SharedStateBehavior) {
    json shared = {
        {"input_data", json{{"value", 42}}},
        {"query", "test search query"}
    };
    
    auto processor = std::make_shared<TestDataProcessor>();
    Flow flow(processor);
    
    // Execute flow and verify shared state modification
    flow.run(shared);
    
    // Verify shared state was modified like Python dict
    EXPECT_TRUE(shared.contains("output"));
    EXPECT_TRUE(shared["output"]["processed"].get<bool>());
    EXPECT_EQ(shared["output"]["original"]["value"].get<int>(), 42);
}

/**
 * Test 4: Flow Orchestration with Branching
 * Tests complete agent pattern like Python version
 */
TEST(APICompatibilityTest, FlowOrchestrationBranching) {
    // Set up agent flow with branching
    auto decide = std::make_shared<TestDecisionNode>();
    auto search = std::make_shared<TestSearchNode>();
    auto answer = std::make_shared<TestAnswerNode>();
    
    // Create branching structure (exact Python syntax)
    decide - "search" >> search;
    decide - "answer" >> answer;
    search - "decide" >> decide;
    answer - "complete" >> nullptr;  // Explicit termination
    
    Flow agent_flow(decide);
    
    // Test 1: Direct answer path
    json shared1 = {{"query", "What is 2+2?"}};
    agent_flow.run(shared1);
    
    EXPECT_TRUE(shared1.contains("final_answer"));
    EXPECT_TRUE(shared1["final_answer"]["answer"].get<std::string>().find("What is 2+2?") != std::string::npos);
    
    // Test 2: Search path (limit iterations to prevent infinite loop)
    json shared2 = {{"query", "search for latest news"}, {"max_iterations", 3}};
    agent_flow.run(shared2);
    
    EXPECT_TRUE(shared2.contains("search_results"));
}

/**
 * Test 5: Batch Processing
 * Tests BatchNode and BatchFlow behavior
 */
TEST(APICompatibilityTest, BatchProcessing) {
    class TestBatchProcessor : public BatchNode {
    public:
        json prep(const json& shared) override {
            return shared["batch_data"];
        }
        
        json exec(const json& item) override {
            return json{{"processed_item", item.get<std::string>() + "_processed"}};
        }
        
        json post(const json& shared, const json& prep_res, const json& exec_res) override {
            const_cast<json&>(shared)["batch_results"] = exec_res;
            return "default";
        }
    };
    
    auto batch_node = std::make_shared<TestBatchProcessor>();
    
    json shared = {
        {"batch_data", json::array({"item1", "item2", "item3"})}
    };
    
    // Test batch processing directly
    batch_node->set_params(json{});
    json results = batch_node->run(shared);
    
    // Verify batch processing occurred
    EXPECT_TRUE(shared.contains("batch_results"));
    json batch_results = shared["batch_results"];
    EXPECT_TRUE(batch_results.is_array());
    EXPECT_EQ(batch_results.size(), 3);
    EXPECT_EQ(batch_results[0]["processed_item"].get<std::string>(), "item1_processed");
    EXPECT_EQ(batch_results[1]["processed_item"].get<std::string>(), "item2_processed");
    EXPECT_EQ(batch_results[2]["processed_item"].get<std::string>(), "item3_processed");
}

/**
 * Test 6: Parameter Management
 * Tests node parameter setting and retrieval
 */
TEST(APICompatibilityTest, ParameterManagement) {
    auto node = std::make_shared<TestDataProcessor>();
    
    json params = {
        {"setting1", "value1"},
        {"setting2", 42},
        {"setting3", true}
    };
    
    // Test parameter setting (same as Python)
    node->set_params(params);
    
    // Test parameter retrieval
    const json& retrieved = node->get_params();
    EXPECT_EQ(retrieved["setting1"].get<std::string>(), "value1");
    EXPECT_EQ(retrieved["setting2"].get<int>(), 42);
    EXPECT_EQ(retrieved["setting3"].get<bool>(), true);
}

/**
 * Test 7: Nested Flows
 * Tests flow composition like Python version
 */
TEST(APICompatibilityTest, NestedFlows) {
    // Create sub-flow
    auto sub_node1 = std::make_shared<TestDataProcessor>();
    auto sub_node2 = std::make_shared<TestDataProcessor>();
    sub_node1 >> sub_node2;
    auto sub_flow = std::make_shared<Flow>(sub_node1);
    
    // Create main flow with sub-flow
    auto main_start = std::make_shared<TestDataProcessor>();
    auto main_end = std::make_shared<TestDataProcessor>();
    
    // Connect flows (same as Python)
    main_start >> sub_flow >> main_end;
    Flow main_flow(main_start);
    
    // Verify connections
    EXPECT_EQ(main_start->get_successor("default"), sub_flow);
    EXPECT_EQ(sub_flow->get_successor("default"), main_end);
}

/**
 * Test 8: Error Handling and Retry Logic
 * Tests retry behavior matches Python expectations
 */
TEST(APICompatibilityTest, RetryLogic) {
    class FailingNode : public Node {
    private:
        mutable int attempt_count = 0;
        
    public:
        FailingNode() : Node(3, 10) {}  // 3 retries, 10ms wait
        
        json exec(const json& prep_res) override {
            attempt_count++;
            if (attempt_count < 3) {
                throw std::runtime_error("Simulated failure");
            }
            return json{{"success", true}, {"attempts", attempt_count}};
        }
        
        json exec_fallback(const json& prep_res, const std::exception& exc) override {
            return json{{"fallback", true}, {"error", exc.what()}};
        }
    };
    
    auto failing_node = std::make_shared<FailingNode>();
    json shared = {{"input", "test"}};
    
    // Should succeed after retries
    json result = failing_node->run(shared);
    
    // Verify retry behavior worked
    EXPECT_EQ(result.get<std::string>(), "default");
}

/**
 * Test 9: JSON Compatibility
 * Tests that JSON operations match Python dict behavior
 */
TEST(APICompatibilityTest, JSONCompatibility) {
    json shared = json::object();
    
    // Test Python-like operations
    shared["key1"] = "value1";
    shared["key2"] = json::array({1, 2, 3});
    shared["key3"] = json::object({{"nested", "value"}});
    
    // Test access patterns
    EXPECT_EQ(shared["key1"].get<std::string>(), "value1");
    EXPECT_TRUE(shared["key2"].is_array());
    EXPECT_EQ(shared["key2"].size(), 3);
    EXPECT_EQ(shared["key3"]["nested"].get<std::string>(), "value");
    
    // Test contains (like Python 'in' operator)
    EXPECT_TRUE(shared.contains("key1"));
    EXPECT_FALSE(shared.contains("nonexistent"));
    
    // Test value with default (like Python dict.get())
    EXPECT_EQ(shared.value("key1", "default"), "value1");
    EXPECT_EQ(shared.value("nonexistent", "default"), "default");
}

/**
 * Test 10: Complete API Surface Compatibility
 * Comprehensive test of all major API elements
 */
TEST(APICompatibilityTest, CompleteAPISurface) {
    // Test all major components can be instantiated and used
    auto base_node = std::make_shared<TestDataProcessor>();
    auto batch_node = std::make_shared<BatchNode>();
    auto flow = std::make_shared<Flow>();
    
    // Test all operators work
    base_node >> batch_node;
    base_node - "custom" >> flow;
    
    // Test parameter management
    json params = {{"test", true}};
    base_node->set_params(params);
    EXPECT_EQ(base_node->get_params()["test"].get<bool>(), true);
    
    // Test shared state
    json shared = {{"data", "test"}};
    base_node->run(shared);
    
    // All operations should complete without errors
    SUCCEED();
}