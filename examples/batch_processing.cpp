#include <iostream>
#include <memory>
#include <chrono>
#include <future>
#include <thread>
#include <random>
#include <vector>
#include <algorithm>
#include <set>
#include "../include/pocketflow/pocketflow.hpp"

using json = nlohmann::json;

/**
 * DataTransformBatch - Demonstrates BatchNode for sequential array processing
 * Processes arrays of data items sequentially
 */
class DataTransformBatch : public pocketflow::BatchNode {
private:
    std::mt19937 rng_;
    
public:
    DataTransformBatch() : BatchNode(1, 0), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    json prep(const json& shared) override {
        return json{
            {"data_items", shared["data_items"]},
            {"transform_type", shared.value("transform_type", "normalize")},
            {"batch_size", shared.value("batch_size", 10)}
        };
    }
    
    json exec(const json& data_item) override {
        // This method processes a single item from the array
        std::uniform_int_distribution<int> delay_dist(50, 150);
        int delay = delay_dist(rng_);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        
        // Simulate data transformation
        if (data_item.is_object() && data_item.contains("value")) {
            double value = data_item["value"];
            std::string id = data_item.value("id", "unknown");
            
            return json{
                {"id", id},
                {"original_value", value},
                {"transformed_value", value * 2.0 + 1.0},  // Simple transformation
                {"processing_time_ms", delay},
                {"status", "transformed"}
            };
        }
        
        return json{
            {"original", data_item},
            {"transformed", data_item},
            {"processing_time_ms", delay},
            {"status", "unchanged"}
        };
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        // exec_result is an array of transformed items
        const_cast<json&>(shared)["transformed_data"] = exec_result;
        
        int item_count = exec_result.size();
        int total_time = 0;
        
        for (const auto& item : exec_result) {
            if (item.contains("processing_time_ms")) {
                total_time += item["processing_time_ms"].get<int>();
            }
        }
        
        const_cast<json&>(shared)["batch_stats"] = {
            {"items_processed", item_count},
            {"total_processing_time_ms", total_time},
            {"average_time_per_item_ms", item_count > 0 ? total_time / item_count : 0}
        };
        
        std::cout << "📊 BatchNode processed " << item_count << " items in " << total_time << "ms" << std::endl;
        
        return json{};
    }
};

/**
 * AsyncDataTransformBatch - Demonstrates AsyncBatchNode for sequential async processing
 * Processes arrays asynchronously but sequentially
 */
class AsyncDataTransformBatch : public pocketflow::AsyncBatchNode {
private:
    std::mt19937 rng_;
    
public:
    AsyncDataTransformBatch() : AsyncBatchNode(1, 0), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [shared]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return shared["data_items"];
        });
    }
    
    std::future<json> exec_async(const json& data_item) override {
        return std::async(std::launch::async, [this, data_item]() {
            // Simulate async processing for each item
            std::uniform_int_distribution<int> delay_dist(100, 300);
            int delay = delay_dist(rng_);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            if (data_item.is_object() && data_item.contains("value")) {
                double value = data_item["value"];
                std::string id = data_item.value("id", "unknown");
                
                return json{
                    {"id", id},
                    {"original_value", value},
                    {"async_transformed_value", std::pow(value, 1.5)},  // Different transformation
                    {"processing_time_ms", delay},
                    {"status", "async_transformed"}
                };
            }
            
            return json{
                {"original", data_item},
                {"async_transformed", data_item},
                {"processing_time_ms", delay},
                {"status", "async_unchanged"}
            };
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_result, const json& exec_result) override {
        return std::async(std::launch::async, [shared, exec_result]() {
            const_cast<json&>(shared)["async_transformed_data"] = exec_result;
            
            int item_count = exec_result.size();
            int total_time = 0;
            
            for (const auto& item : exec_result) {
                if (item.contains("processing_time_ms")) {
                    total_time += item["processing_time_ms"].get<int>();
                }
            }
            
            const_cast<json&>(shared)["async_batch_stats"] = {
                {"items_processed", item_count},
                {"total_processing_time_ms", total_time},
                {"average_time_per_item_ms", item_count > 0 ? total_time / item_count : 0}
            };
            
            std::cout << "🚀 AsyncBatchNode processed " << item_count << " items in " << total_time << "ms" << std::endl;
            
            return json{};
        });
    }
};

/**
 * ParallelDataTransformBatch - Demonstrates AsyncParallelBatchNode
 * Processes arrays with true parallel execution
 */
class ParallelDataTransformBatch : public pocketflow::AsyncParallelBatchNode {
private:
    std::mt19937 rng_;
    
public:
    ParallelDataTransformBatch() : AsyncParallelBatchNode(1, 0), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [shared]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return shared["data_items"];
        });
    }
    
    std::future<json> exec_async(const json& data_item) override {
        return std::async(std::launch::async, [this, data_item]() {
            // Simulate parallel async processing
            std::uniform_int_distribution<int> delay_dist(200, 500);
            int delay = delay_dist(rng_);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            if (data_item.is_object() && data_item.contains("value")) {
                double value = data_item["value"];
                std::string id = data_item.value("id", "unknown");
                
                return json{
                    {"id", id},
                    {"original_value", value},
                    {"parallel_transformed_value", std::sin(value) * 100},  // Another transformation
                    {"processing_time_ms", delay},
                    {"status", "parallel_transformed"},
                    {"thread_id", std::hash<std::thread::id>{}(std::this_thread::get_id())}
                };
            }
            
            return json{
                {"original", data_item},
                {"parallel_transformed", data_item},
                {"processing_time_ms", delay},
                {"status", "parallel_unchanged"},
                {"thread_id", std::hash<std::thread::id>{}(std::this_thread::get_id())}
            };
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_result, const json& exec_result) override {
        return std::async(std::launch::async, [shared, exec_result]() {
            const_cast<json&>(shared)["parallel_transformed_data"] = exec_result;
            
            int item_count = exec_result.size();
            int total_time = 0;
            std::set<size_t> unique_threads;
            
            for (const auto& item : exec_result) {
                if (item.contains("processing_time_ms")) {
                    total_time += item["processing_time_ms"].get<int>();
                }
                if (item.contains("thread_id")) {
                    unique_threads.insert(item["thread_id"].get<size_t>());
                }
            }
            
            const_cast<json&>(shared)["parallel_batch_stats"] = {
                {"items_processed", item_count},
                {"total_processing_time_ms", total_time},
                {"average_time_per_item_ms", item_count > 0 ? total_time / item_count : 0},
                {"threads_used", unique_threads.size()}
            };
            
            std::cout << "⚡ AsyncParallelBatchNode processed " << item_count 
                      << " items using " << unique_threads.size() << " threads" << std::endl;
            
            return json{};
        });
    }
};

/**
 * BatchFlowExample - Demonstrates BatchFlow for processing batches through flows
 */
class BatchFlowExample : public pocketflow::BatchFlow {
public:
    BatchFlowExample(std::shared_ptr<pocketflow::BaseNode> start = nullptr) : BatchFlow(start) {}
    
    json prep(const json& shared) override {
        // Return array of batch parameters
        json batch_params = json::array();
        
        int num_batches = shared.value("num_batches", 3);
        for (int i = 0; i < num_batches; ++i) {
            batch_params.push_back(json{
                {"batch_id", i + 1},
                {"batch_size", shared.value("batch_size", 5)},
                {"processing_mode", shared.value("processing_mode", "standard")}
            });
        }
        
        return batch_params;
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        const_cast<json&>(shared)["batch_flow_completed"] = true;
        const_cast<json&>(shared)["batches_processed"] = prep_result.size();
        
        std::cout << "📦 BatchFlow completed processing " << prep_result.size() << " batches" << std::endl;
        
        return json{};
    }
};

/**
 * BatchProcessor - Individual batch processing node for use in BatchFlow
 */
class BatchProcessor : public pocketflow::Node {
public:
    BatchProcessor() : Node(1, 0) {}
    
    json prep(const json& shared) override {
        return json{
            {"batch_id", shared["batch_id"]},
            {"batch_size", shared["batch_size"]},
            {"mode", shared["processing_mode"]}
        };
    }
    
    json exec(const json& prep_result) override {
        int batch_id = prep_result["batch_id"];
        int batch_size = prep_result["batch_size"];
        std::string mode = prep_result["mode"];
        
        std::cout << "🔄 Processing batch " << batch_id << " (size: " << batch_size 
                  << ", mode: " << mode << ")" << std::endl;
        
        // Simulate batch processing
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        return json{
            {"batch_id", batch_id},
            {"items_processed", batch_size},
            {"processing_mode", mode},
            {"status", "completed"}
        };
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        int batch_id = exec_result["batch_id"];
        int items = exec_result["items_processed"];
        
        std::cout << "✅ Batch " << batch_id << " completed: " << items << " items processed" << std::endl;
        
        return json{};
    }
};

/**
 * Performance comparison function
 */
void run_batch_performance_comparison() {
    std::cout << "=== Batch Processing Performance Comparison ===" << std::endl;
    std::cout << std::endl;
    
    // Create test data
    json test_data = json::array();
    for (int i = 0; i < 10; ++i) {
        test_data.push_back(json{
            {"id", "item_" + std::to_string(i)},
            {"value", i * 10.5}
        });
    }
    
    // Test 1: Sequential BatchNode
    std::cout << "--- Sequential BatchNode ---" << std::endl;
    json shared1 = {{"data_items", test_data}};
    auto batch_node = std::make_shared<DataTransformBatch>();
    pocketflow::Flow batch_flow(batch_node);
    
    auto start1 = std::chrono::high_resolution_clock::now();
    batch_flow.run(shared1);
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    
    std::cout << "Sequential processing: " << duration1.count() << "ms" << std::endl;
    if (shared1.contains("batch_stats")) {
        json stats = shared1["batch_stats"];
        std::cout << "- Items processed: " << stats["items_processed"] << std::endl;
        std::cout << "- Average time per item: " << stats["average_time_per_item_ms"] << "ms" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 2: Async Sequential BatchNode
    std::cout << "--- Async Sequential BatchNode ---" << std::endl;
    json shared2 = {{"data_items", test_data}};
    auto async_batch_node = std::make_shared<AsyncDataTransformBatch>();
    pocketflow::AsyncFlow async_batch_flow(async_batch_node);
    
    auto start2 = std::chrono::high_resolution_clock::now();
    auto future2 = async_batch_flow.run_async(shared2);
    future2.get();
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
    std::cout << "Async sequential processing: " << duration2.count() << "ms" << std::endl;
    if (shared2.contains("async_batch_stats")) {
        json stats = shared2["async_batch_stats"];
        std::cout << "- Items processed: " << stats["items_processed"] << std::endl;
        std::cout << "- Average time per item: " << stats["average_time_per_item_ms"] << "ms" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 3: Parallel BatchNode
    std::cout << "--- Parallel BatchNode ---" << std::endl;
    json shared3 = {{"data_items", test_data}};
    auto parallel_batch_node = std::make_shared<ParallelDataTransformBatch>();
    pocketflow::AsyncFlow parallel_batch_flow(parallel_batch_node);
    
    auto start3 = std::chrono::high_resolution_clock::now();
    auto future3 = parallel_batch_flow.run_async(shared3);
    future3.get();
    auto end3 = std::chrono::high_resolution_clock::now();
    auto duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(end3 - start3);
    
    std::cout << "Parallel processing: " << duration3.count() << "ms" << std::endl;
    if (shared3.contains("parallel_batch_stats")) {
        json stats = shared3["parallel_batch_stats"];
        std::cout << "- Items processed: " << stats["items_processed"] << std::endl;
        std::cout << "- Average time per item: " << stats["average_time_per_item_ms"] << "ms" << std::endl;
        std::cout << "- Threads used: " << stats["threads_used"] << std::endl;
    }
    std::cout << std::endl;
    
    // Performance analysis
    std::cout << "Performance Analysis:" << std::endl;
    std::cout << "- Sequential: " << duration1.count() << "ms" << std::endl;
    std::cout << "- Async Sequential: " << duration2.count() << "ms" << std::endl;
    std::cout << "- Parallel: " << duration3.count() << "ms" << std::endl;
    
    double speedup_async = static_cast<double>(duration1.count()) / duration2.count();
    double speedup_parallel = static_cast<double>(duration1.count()) / duration3.count();
    
    std::cout << "- Async speedup: " << std::fixed << std::setprecision(2) << speedup_async << "x" << std::endl;
    std::cout << "- Parallel speedup: " << std::fixed << std::setprecision(2) << speedup_parallel << "x" << std::endl;
    std::cout << std::endl;
}

/**
 * Main function demonstrating batch processing capabilities
 */
int main() {
    std::cout << "=== PocketFlow-CPP Batch Processing Examples ===" << std::endl;
    std::cout << std::endl;
    
    // Example 1: Basic BatchNode
    std::cout << "--- Example 1: DataTransformBatch (BatchNode) ---" << std::endl;
    
    json test_data = json::array();
    for (int i = 0; i < 5; ++i) {
        test_data.push_back(json{
            {"id", "item_" + std::to_string(i)},
            {"value", (i + 1) * 5.0}
        });
    }
    
    json shared1 = {
        {"data_items", test_data},
        {"transform_type", "normalize"},
        {"batch_size", 5}
    };
    
    auto batch_transformer = std::make_shared<DataTransformBatch>();
    pocketflow::Flow batch_flow(batch_transformer);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        batch_flow.run(shared1);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "✅ BatchNode completed in " << duration.count() << "ms" << std::endl;
        
        if (shared1.contains("transformed_data")) {
            std::cout << "Transformed data sample:" << std::endl;
            json transformed = shared1["transformed_data"];
            for (size_t i = 0; i < std::min(transformed.size(), size_t(3)); ++i) {
                std::cout << "  " << transformed[i].dump() << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ BatchNode failed: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << std::endl;
    
    // Example 2: BatchFlow
    std::cout << "--- Example 2: BatchFlowExample (BatchFlow) ---" << std::endl;
    
    json shared2 = {
        {"num_batches", 4},
        {"batch_size", 8},
        {"processing_mode", "enhanced"}
    };
    
    auto batch_processor = std::make_shared<BatchProcessor>();
    auto batch_flow_example = std::make_shared<BatchFlowExample>(batch_processor);
    
    start_time = std::chrono::high_resolution_clock::now();
    
    try {
        batch_flow_example->run(shared2);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "✅ BatchFlow completed in " << duration.count() << "ms" << std::endl;
        
        if (shared2.contains("batches_processed")) {
            std::cout << "Batches processed: " << shared2["batches_processed"] << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ BatchFlow failed: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << std::endl;
    
    // Performance comparison
    run_batch_performance_comparison();
    
    std::cout << "=== Batch Processing Examples Completed ===" << std::endl;
    
    return 0;
}