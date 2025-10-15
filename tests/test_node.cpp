#include <gtest/gtest.h>
#include <memory>
#include "pocketflow/node.hpp"

using json = nlohmann::json;

// Test implementation of Node
class TestNode : public pocketflow::Node {
public:
    TestNode(int max_retries = 1, int wait = 0) : Node(max_retries, wait) {}
    
    json prep(const json& shared) override {
        return shared.value("input", json{});
    }
    
    json exec(const json& prep_res) override {
        if (should_fail && retry_count < max_failures) {
            retry_count++;
            throw std::runtime_error("Test failure");
        }
        return json{{"result", "processed"}};
    }
    
    json post(const json& shared, const json& prep_res, const json& exec_res) override {
        // Pass through the exec result for testing
        json result = json{{"action", "default"}};
        if (exec_res.contains("result")) {
            result["result"] = exec_res["result"];
        }
        return result;
    }
    
    json exec_fallback(const json& prep_res, const std::exception& exc) override {
        return json{{"result", "fallback"}};
    }
    
    // Test control variables
    bool should_fail = false;
    int max_failures = 0;
    mutable int retry_count = 0;
};

class NodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        node = std::make_shared<TestNode>();
        shared = json{{"input", "test_data"}};
    }
    
    std::shared_ptr<TestNode> node;
    json shared;
};

TEST_F(NodeTest, SuccessfulExecution) {
    auto result = node->run(shared);
    EXPECT_EQ(result["action"], "default");
}

TEST_F(NodeTest, RetryMechanism) {
    auto retry_node = std::make_shared<TestNode>(3, 0);
    retry_node->should_fail = true;
    retry_node->max_failures = 2; // Fail twice, succeed on third try
    
    auto result = retry_node->run(shared);
    EXPECT_EQ(result["action"], "default");
    EXPECT_EQ(retry_node->retry_count, 2);
}

TEST_F(NodeTest, FallbackExecution) {
    auto fallback_node = std::make_shared<TestNode>(2, 0);
    fallback_node->should_fail = true;
    fallback_node->max_failures = 10; // Always fail
    
    auto result = fallback_node->run(shared);
    EXPECT_EQ(result["result"], "fallback");
    EXPECT_EQ(fallback_node->retry_count, 2); // Should have tried max_retries times
}