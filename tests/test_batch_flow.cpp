#include <gtest/gtest.h>
#include <memory>
#include "pocketflow/batch_flow.hpp"
#include "pocketflow/node.hpp"

using json = nlohmann::json;

// Test implementation for BatchFlow testing
class TestBatchFlowNode : public pocketflow::Node {
public:
    json prep(const json& shared) override {
        return shared.value("input", json{});
    }
    
    json exec(const json& prep_res) override {
        return json{{"result", "processed"}};
    }
    
    json post(const json& shared, const json& prep_res, const json& exec_res) override {
        // For BatchFlow, exec_res might be empty, so create a proper result
        json result = json::object();
        if (!exec_res.is_null() && exec_res.is_object()) {
            result = exec_res;
        }
        result["action"] = "default";
        return result;
    }
};

class BatchFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        node = std::make_shared<TestBatchFlowNode>();
        shared = json{{"input", "test_data"}};
    }
    
    std::shared_ptr<TestBatchFlowNode> node;
    json shared;
};

TEST_F(BatchFlowTest, BasicBatchFlowExecution) {
    pocketflow::BatchFlow flow(node);
    auto result = flow.run(shared);
    EXPECT_TRUE(result.is_object());
}