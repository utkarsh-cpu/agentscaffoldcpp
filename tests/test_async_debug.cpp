#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <iostream>
#include "../include/pocketflow/pocketflow.hpp"

using json = nlohmann::json;
using namespace pocketflow;

class SimpleAsyncNode : public AsyncNode {
public:
    SimpleAsyncNode() : AsyncNode(1, 0) {}
    
    std::future<json> exec_async(const json& prep_result) override {
        return std::async(std::launch::async, [prep_result]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return json{{"result", "async_processed"}};
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_result, const json& exec_result) override {
        return std::async(std::launch::async, [&shared, exec_result]() -> json {
            std::cout << "Post async called with exec_result: " << exec_result.dump() << std::endl;
            // Modify shared state directly
            json& mutable_shared = const_cast<json&>(shared);
            mutable_shared["async_result"] = exec_result["result"];
            std::cout << "Shared state after modification: " << mutable_shared.dump() << std::endl;
            return json{};
        });
    }
};

TEST(AsyncDebugTest, SimpleAsyncExecution) {
    json shared = {{"input", "test"}};
    
    std::cout << "Initial shared state: " << shared.dump() << std::endl;
    
    auto async_node = std::make_shared<SimpleAsyncNode>();
    
    // Test direct async node execution
    auto future = async_node->run_async(shared);
    future.get();
    
    std::cout << "Final shared state: " << shared.dump() << std::endl;
    
    EXPECT_TRUE(shared.contains("async_result"));
    if (shared.contains("async_result")) {
        EXPECT_EQ(shared["async_result"], "async_processed");
    }
}

TEST(AsyncDebugTest, AsyncFlowExecution) {
    json shared = {{"input", "test"}};
    
    std::cout << "Initial shared state: " << shared.dump() << std::endl;
    
    auto async_node = std::make_shared<SimpleAsyncNode>();
    AsyncFlow async_flow(async_node);
    
    auto future = async_flow.run_async(shared);
    future.get();
    
    std::cout << "Final shared state: " << shared.dump() << std::endl;
    
    EXPECT_TRUE(shared.contains("async_result"));
    if (shared.contains("async_result")) {
        EXPECT_EQ(shared["async_result"], "async_processed");
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}