#include <gtest/gtest.h>
#include <memory>
#include "pocketflow/flow.hpp"
#include "pocketflow/node.hpp"

using json = nlohmann::json;

// Test implementation of Node for Flow testing
class TestFlowNode : public pocketflow::Node {
public:
    TestFlowNode(const std::string& name) : name_(name) {}
    
    json prep(const json& shared) override {
        return shared.value("input", json{});
    }
    
    json exec(const json& prep_res) override {
        return json{{"processed_by", name_}};
    }
    
    json post(const json& shared, const json& prep_res, const json& exec_res) override {
        // Pass through the exec result and add action
        json result = exec_res;
        result["action"] = "default";
        return result;
    }
    
private:
    std::string name_;
};

class FlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        node1 = std::make_shared<TestFlowNode>("node1");
        node2 = std::make_shared<TestFlowNode>("node2");
        shared = json{{"input", "test_data"}};
    }
    
    std::shared_ptr<TestFlowNode> node1;
    std::shared_ptr<TestFlowNode> node2;
    json shared;
};

TEST_F(FlowTest, BasicFlowExecution) {
    pocketflow::Flow flow(node1);
    auto result = flow.run(shared);
    EXPECT_TRUE(result.contains("processed_by"));
}

TEST_F(FlowTest, FlowChaining) {
    *node1 >> node2;
    pocketflow::Flow flow(node1);
    auto result = flow.run(shared);
    EXPECT_TRUE(result.contains("processed_by"));
}