#include <gtest/gtest.h>
#include <memory>
#include <future>
#include "pocketflow/async_batch_node.hpp"

using json = nlohmann::json;

// Test implementation of AsyncBatchNode
class TestAsyncBatchNode : public pocketflow::AsyncBatchNode {
public:
    TestAsyncBatchNode(int max_retries = 1, int wait = 0) : AsyncBatchNode(max_retries, wait) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [shared]() {
            return shared.value("items", json::array());
        });
    }
    
    std::future<json> exec_async(const json& item) override {
        return std::async(std::launch::async, [item]() {
            return json{{"processed", item}};
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_res, const json& exec_res) override {
        return std::async(std::launch::async, [exec_res]() {
            return json{{"action", "default"}};
        });
    }
};

class AsyncBatchNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        node = std::make_shared<TestAsyncBatchNode>();
        shared = json{{"items", json::array({"item1", "item2", "item3"})}};
    }
    
    std::shared_ptr<TestAsyncBatchNode> node;
    json shared;
};

TEST_F(AsyncBatchNodeTest, AsyncBatchProcessing) {
    auto future = node->run_async(shared);
    auto result = future.get();
    EXPECT_EQ(result["action"], "default");
}