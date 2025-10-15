#include <gtest/gtest.h>
#include <memory>
#include <future>
#include "pocketflow/async_flow.hpp"
#include "pocketflow/async_node.hpp"

using json = nlohmann::json;

// Test implementation of AsyncNode for AsyncFlow testing
class TestAsyncFlowNode : public pocketflow::AsyncNode {
public:
    TestAsyncFlowNode(const std::string& name) : name_(name) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [shared]() {
            return shared.value("input", json{});
        });
    }
    
    std::future<json> exec_async(const json& prep_res) override {
        return std::async(std::launch::async, [this]() {
            return json{{"processed_by", name_}};
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_res, const json& exec_res) override {
        return std::async(std::launch::async, [exec_res]() {
            // Pass through the exec result and add action
            json result = exec_res;
            result["action"] = "default";
            return result;
        });
    }
    
private:
    std::string name_;
};

class AsyncFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        node1 = std::make_shared<TestAsyncFlowNode>("async_node1");
        shared = json{{"input", "test_data"}};
    }
    
    std::shared_ptr<TestAsyncFlowNode> node1;
    json shared;
};

TEST_F(AsyncFlowTest, BasicAsyncFlowExecution) {
    pocketflow::AsyncFlow flow(node1);
    auto future = flow.run_async(shared);
    auto result = future.get();
    EXPECT_TRUE(result.contains("processed_by"));
}