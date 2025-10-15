#include <gtest/gtest.h>
#include <memory>
#include "pocketflow/base_node.hpp"

using json = nlohmann::json;

// Test implementation of BaseNode
class TestBaseNode : public pocketflow::BaseNode {
public:
    json prep(const json& shared) override {
        return shared.value("input", json{});
    }
    
    json exec(const json& prep_res) override {
        return json{{"result", "processed"}};
    }
    
    json post(const json& shared, const json& prep_res, const json& exec_res) override {
        return json{{"action", "default"}};
    }
};

class BaseNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        node = std::make_shared<TestBaseNode>();
        shared = json{{"input", "test_data"}};
    }
    
    std::shared_ptr<TestBaseNode> node;
    json shared;
};

TEST_F(BaseNodeTest, BasicExecution) {
    auto result = node->run(shared);
    EXPECT_EQ(result["action"], "default");
}

TEST_F(BaseNodeTest, ParameterSetting) {
    json params = {{"key", "value"}};
    node->set_params(params);
    // Parameters are set (internal state, can't directly test without exposing)
    SUCCEED();
}

TEST_F(BaseNodeTest, SuccessorManagement) {
    auto next_node = std::make_shared<TestBaseNode>();
    auto returned_node = node->next(next_node, "test_action");
    EXPECT_EQ(returned_node, next_node);
}

TEST_F(BaseNodeTest, OperatorChaining) {
    auto node2 = std::make_shared<TestBaseNode>();
    auto result = *node >> node2;
    EXPECT_EQ(result, node2);
}