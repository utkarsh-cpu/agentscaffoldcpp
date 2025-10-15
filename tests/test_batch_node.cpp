#include <gtest/gtest.h>
#include <memory>
#include "pocketflow/batch_node.hpp"

using json = nlohmann::json;

// Test implementation of BatchNode
class TestBatchNode : public pocketflow::BatchNode {
public:
    TestBatchNode(int max_retries = 1, int wait = 0) : BatchNode(max_retries, wait) {}
    
    json prep(const json& shared) override {
        return shared.value("items", json::array());
    }
    
    json exec(const json& item) override {
        return json{{"processed", item}};
    }
    
    json post(const json& shared, const json& prep_res, const json& exec_res) override {
        return json{{"action", "default"}};
    }
};

class BatchNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        node = std::make_shared<TestBatchNode>();
        shared = json{{"items", json::array({"item1", "item2", "item3"})}};
    }
    
    std::shared_ptr<TestBatchNode> node;
    json shared;
};

TEST_F(BatchNodeTest, ArrayProcessing) {
    auto result = node->run(shared);
    EXPECT_EQ(result["action"], "default");
}

TEST_F(BatchNodeTest, EmptyArrayHandling) {
    shared["items"] = json::array();
    auto result = node->run(shared);
    EXPECT_EQ(result["action"], "default");
}