#include <gtest/gtest.h>
#include <memory>
#include <future>
#include "pocketflow/async_node.hpp"

using json = nlohmann::json;

// Test implementation of AsyncNode
class TestAsyncNode : public pocketflow::AsyncNode {
public:
    TestAsyncNode(int max_retries = 1, int wait = 0) : AsyncNode(max_retries, wait) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [shared]() {
            return shared.value("input", json{});
        });
    }
    
    std::future<json> exec_async(const json& prep_res) override {
        return std::async(std::launch::async, [prep_res]() {
            return json{{"result", "async_processed"}};
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_res, const json& exec_res) override {
        return std::async(std::launch::async, [exec_res]() {
            return json{{"action", "default"}};
        });
    }
};

class AsyncNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        node = std::make_shared<TestAsyncNode>();
        shared = json{{"input", "test_data"}};
    }
    
    std::shared_ptr<TestAsyncNode> node;
    json shared;
};

TEST_F(AsyncNodeTest, AsyncExecution) {
    auto future = node->run_async(shared);
    auto result = future.get();
    EXPECT_EQ(result["action"], "default");
}

TEST_F(AsyncNodeTest, SyncExecutionThrows) {
    EXPECT_THROW(node->run(shared), std::runtime_error);
}