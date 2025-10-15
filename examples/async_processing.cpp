#include <iostream>
#include <memory>
#include <chrono>
#include <future>
#include <thread>
#include <random>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "../include/pocketflow/pocketflow.hpp"

using json = nlohmann::json;

/**
 * AsyncSummarizeNode - Demonstrates async node implementation
 * Simulates asynchronous text summarization (like calling an LLM API)
 */
class AsyncSummarizeNode : public pocketflow::AsyncNode {
private:
    std::mt19937 rng_;
    
public:
    AsyncSummarizeNode() : AsyncNode(2, 100), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [this, shared]() {
            std::cout << "📖 AsyncSummarizeNode: Loading document..." << std::endl;
            
            // Simulate async file reading or API preparation
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            return json{
                {"doc_path", shared.value("doc_path", "document.txt")},
                {"max_length", shared.value("max_summary_length", 200)},
                {"style", shared.value("summary_style", "concise")}
            };
        });
    }
    
    std::future<json> exec_async(const json& prep_res) override {
        return std::async(std::launch::async, [this, prep_res]() {
            std::string doc_path = prep_res["doc_path"];
            int max_length = prep_res["max_length"];
            std::string style = prep_res["style"];
            
            std::cout << "🤖 AsyncSummarizeNode: Generating " << style << " summary for " << doc_path << std::endl;
            
            // Simulate variable async processing time (like LLM API call)
            std::uniform_int_distribution<int> delay_dist(300, 1000);
            int delay = delay_dist(rng_);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            // Simulate occasional API failures
            std::uniform_real_distribution<double> failure_dist(0.0, 1.0);
            if (failure_dist(rng_) < 0.15) {  // 15% chance of failure
                throw std::runtime_error("LLM API temporarily unavailable");
            }
            
            // Generate simulated summary
            std::string summary = "This is an AI-generated " + style + " summary of " + doc_path + 
                                ". The document contains important information about the topic. " +
                                "Key points have been extracted and condensed into this " + 
                                std::to_string(max_length) + "-character summary.";
            
            if (summary.length() > static_cast<size_t>(max_length)) {
                summary = summary.substr(0, max_length - 3) + "...";
            }
            
            return json{
                {"summary", summary},
                {"original_doc", doc_path},
                {"processing_time_ms", delay},
                {"summary_length", summary.length()},
                {"style", style}
            };
        });
    }
    
    std::future<json> exec_fallback_async(const json& prep_res, const std::exception& exc) override {
        return std::async(std::launch::async, [prep_res, &exc]() {
            std::cout << "⚠️  AsyncSummarizeNode fallback: " << exc.what() << std::endl;
            
            // Simulate fallback processing (e.g., using cached summary or simpler method)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            std::string doc_path = prep_res["doc_path"];
            std::string fallback_summary = "Fallback summary for " + doc_path + 
                                         ". Full processing unavailable, using cached or simplified summary.";
            
            return json{
                {"summary", fallback_summary},
                {"original_doc", doc_path},
                {"processing_time_ms", 50},
                {"summary_length", fallback_summary.length()},
                {"style", "fallback"},
                {"fallback_used", true}
            };
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_res, const json& exec_res) override {
        return std::async(std::launch::async, [shared, exec_res]() -> json {
            // Simulate async post-processing (e.g., saving results, user feedback)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            const_cast<json&>(shared)["summary"] = exec_res["summary"];
            const_cast<json&>(shared)["summary_metadata"] = {
                {"length", exec_res["summary_length"]},
                {"processing_time", exec_res["processing_time_ms"]},
                {"style", exec_res["style"]},
                {"fallback_used", exec_res.value("fallback_used", false)}
            };
            
            bool fallback = exec_res.value("fallback_used", false);
            int time_ms = exec_res["processing_time_ms"];
            
            if (fallback) {
                std::cout << "📝 Summary completed with fallback in " << time_ms << "ms" << std::endl;
            } else {
                std::cout << "📝 Summary completed successfully in " << time_ms << "ms" << std::endl;
            }
            
            return json("approve");  // Return action for flow control
        });
    }
};

/**
 * ParallelSummaries - Demonstrates AsyncParallelBatchNode
 * Processes multiple texts concurrently
 */
class ParallelSummaries : public pocketflow::AsyncParallelBatchNode {
private:
    std::mt19937 rng_;
    
public:
    ParallelSummaries() : AsyncParallelBatchNode(1, 0), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [shared]() {
            std::cout << "📚 ParallelSummaries: Preparing batch processing..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            return shared["texts"];  // Array of texts to process
        });
    }
    
    std::future<json> exec_async(const json& text_item) override {
        return std::async(std::launch::async, [this, text_item]() {
            std::string text = text_item.get<std::string>();
            
            // Simulate async processing for each text item
            std::uniform_int_distribution<int> delay_dist(200, 600);
            int delay = delay_dist(rng_);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            std::string summary = "Summary of: " + text.substr(0, 50);
            if (text.length() > 50) summary += "...";
            
            return json{
                {"original_text", text},
                {"summary", summary},
                {"processing_time_ms", delay},
                {"text_length", text.length()}
            };
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_res, const json& exec_res_list) override {
        return std::async(std::launch::async, [shared, exec_res_list]() -> json {
            std::cout << "📊 ParallelSummaries: Combining results..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Combine all summaries
            std::string combined_summary;
            int total_processing_time = 0;
            int total_texts = exec_res_list.size();
            
            for (const auto& result : exec_res_list) {
                combined_summary += result["summary"].get<std::string>() + "\n\n";
                total_processing_time += result["processing_time_ms"].get<int>();
            }
            
            const_cast<json&>(shared)["combined_summary"] = combined_summary;
            const_cast<json&>(shared)["batch_stats"] = {
                {"total_texts", total_texts},
                {"total_processing_time_ms", total_processing_time},
                {"average_processing_time_ms", total_processing_time / total_texts},
                {"combined_length", combined_summary.length()}
            };
            
            std::cout << "📊 Batch processing completed: " << total_texts << " texts in " 
                      << total_processing_time << "ms total" << std::endl;
            
            return json("default");
        });
    }
};

/**
 * FileProcessorFlow - Demonstrates AsyncParallelBatchFlow
 * Processes multiple files in parallel using sub-flows
 */
class FileProcessorFlow : public pocketflow::AsyncParallelBatchFlow {
public:
    FileProcessorFlow(std::shared_ptr<pocketflow::BaseNode> start = nullptr) 
        : AsyncParallelBatchFlow(start) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [shared]() {
            std::cout << "📁 FileProcessorFlow: Preparing file batch..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            json batch_params = json::array();
            
            // Create batch parameters for each file
            for (const auto& filename : shared["files"]) {
                batch_params.push_back(json{
                    {"filename", filename},
                    {"processing_mode", shared.value("processing_mode", "standard")},
                    {"output_format", shared.value("output_format", "json")}
                });
            }
            
            return batch_params;
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_res, const json& exec_res) override {
        return std::async(std::launch::async, [shared, prep_res]() {
            std::cout << "📁 FileProcessorFlow: All files processed successfully" << std::endl;
            
            const_cast<json&>(shared)["batch_completed"] = true;
            const_cast<json&>(shared)["files_processed"] = prep_res.size();
            
            return json{};
        });
    }
};

/**
 * LoadAndProcessFile - Sub-flow node for individual file processing
 * Used within FileProcessorFlow for each file
 */
class LoadAndProcessFile : public pocketflow::AsyncNode {
private:
    std::mt19937 rng_;
    
public:
    LoadAndProcessFile() : AsyncNode(1, 0), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    std::future<json> prep_async(const json& shared) override {
        return std::async(std::launch::async, [shared]() {
            // Get file-specific parameters from shared state
            return json{
                {"filename", shared["filename"]},
                {"mode", shared.value("processing_mode", "standard")},
                {"format", shared.value("output_format", "json")}
            };
        });
    }
    
    std::future<json> exec_async(const json& prep_res) override {
        return std::async(std::launch::async, [this, prep_res]() {
            std::string filename = prep_res["filename"];
            std::string mode = prep_res["mode"];
            
            std::cout << "📄 Processing file: " << filename << " (mode: " << mode << ")" << std::endl;
            
            // Simulate file loading and processing
            std::uniform_int_distribution<int> delay_dist(400, 1200);
            int delay = delay_dist(rng_);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
            // Simulate file content processing
            json result = {
                {"filename", filename},
                {"content_summary", "Processed content from " + filename},
                {"processing_mode", mode},
                {"processing_time_ms", delay},
                {"file_size_kb", 1024 + (delay / 10)},  // Simulate variable file sizes
                {"status", "completed"}
            };
            
            return result;
        });
    }
    
    std::future<json> post_async(const json& shared, const json& prep_res, const json& exec_res) override {
        return std::async(std::launch::async, [shared, exec_res]() {
            // Update shared state with file processing results
            std::string filename = exec_res["filename"];
            int processing_time = exec_res["processing_time_ms"];
            
            const_cast<json&>(shared)["file_result"] = exec_res;
            
            std::cout << "✅ File processed: " << filename << " in " << processing_time << "ms" << std::endl;
            
            return json{};
        });
    }
};

/**
 * Performance comparison helper
 */
void run_performance_comparison() {
    std::cout << "=== Performance Comparison: Sequential vs Parallel ===" << std::endl;
    std::cout << std::endl;
    
    // Test data
    json test_texts = json::array({
        "This is the first document that needs to be processed and summarized.",
        "Here is another document with different content that requires analysis.",
        "The third document contains various information that should be condensed.",
        "Document four has its own unique content and characteristics.",
        "Finally, the fifth document rounds out our test dataset."
    });
    
    // Sequential processing test
    std::cout << "--- Sequential Processing ---" << std::endl;
    auto start_sequential = std::chrono::high_resolution_clock::now();
    
    json sequential_shared = {{"texts", test_texts}};
    auto sequential_node = std::make_shared<ParallelSummaries>();
    
    // Force sequential by using regular AsyncBatchNode behavior
    pocketflow::AsyncFlow sequential_flow(std::dynamic_pointer_cast<pocketflow::BaseNode>(sequential_node));
    auto sequential_future = sequential_flow.run_async(sequential_shared);
    sequential_future.get();
    
    auto end_sequential = std::chrono::high_resolution_clock::now();
    auto sequential_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_sequential - start_sequential);
    
    std::cout << "Sequential processing completed in: " << sequential_duration.count() << "ms" << std::endl;
    std::cout << std::endl;
    
    // Parallel processing test
    std::cout << "--- Parallel Processing ---" << std::endl;
    auto start_parallel = std::chrono::high_resolution_clock::now();
    
    json parallel_shared = {{"texts", test_texts}};
    auto parallel_node = std::make_shared<ParallelSummaries>();
    pocketflow::AsyncFlow parallel_flow(std::dynamic_pointer_cast<pocketflow::BaseNode>(parallel_node));
    auto parallel_future = parallel_flow.run_async(parallel_shared);
    parallel_future.get();
    
    auto end_parallel = std::chrono::high_resolution_clock::now();
    auto parallel_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_parallel - start_parallel);
    
    std::cout << "Parallel processing completed in: " << parallel_duration.count() << "ms" << std::endl;
    std::cout << std::endl;
    
    // Performance analysis
    double speedup = static_cast<double>(sequential_duration.count()) / parallel_duration.count();
    std::cout << "Performance Analysis:" << std::endl;
    std::cout << "- Sequential time: " << sequential_duration.count() << "ms" << std::endl;
    std::cout << "- Parallel time: " << parallel_duration.count() << "ms" << std::endl;
    std::cout << "- Speedup: " << std::fixed << std::setprecision(2) << speedup << "x" << std::endl;
    std::cout << "- Efficiency: " << std::fixed << std::setprecision(1) << (speedup / test_texts.size()) * 100 << "%" << std::endl;
    std::cout << std::endl;
}

/**
 * Main function demonstrating async processing capabilities
 */
int main() {
    std::cout << "=== PocketFlow-CPP Async Processing Examples ===" << std::endl;
    std::cout << std::endl;
    
    // Example 1: Basic AsyncNode usage
    std::cout << "--- Example 1: AsyncSummarizeNode ---" << std::endl;
    
    json shared1 = {
        {"doc_path", "research_paper.pdf"},
        {"max_summary_length", 150},
        {"summary_style", "academic"}
    };
    
    auto async_summarizer = std::make_shared<AsyncSummarizeNode>();
    pocketflow::AsyncFlow async_flow(std::dynamic_pointer_cast<pocketflow::BaseNode>(async_summarizer));
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        auto future1 = async_flow.run_async(shared1);
        
        // Demonstrate that we can do other work while waiting
        std::cout << "⏳ Processing in background..." << std::endl;
        
        // Simulate doing other work
        for (int i = 0; i < 5; ++i) {
            std::cout << "   Doing other work... (" << (i + 1) << "/5)" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Wait for completion
        future1.get();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "✅ AsyncSummarizeNode completed in " << duration.count() << "ms" << std::endl;
        std::cout << "Summary: \"" << shared1["summary"] << "\"" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ AsyncSummarizeNode failed: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << std::endl;
    
    // Example 2: AsyncParallelBatchNode
    std::cout << "--- Example 2: ParallelSummaries (AsyncParallelBatchNode) ---" << std::endl;
    
    json shared2 = {
        {"texts", json::array({
            "First document about machine learning and artificial intelligence applications.",
            "Second document discussing climate change and environmental sustainability.",
            "Third document covering economic trends and market analysis.",
            "Fourth document exploring space exploration and astronomical discoveries."
        })}
    };
    
    auto parallel_summaries = std::make_shared<ParallelSummaries>();
    pocketflow::AsyncFlow parallel_flow(std::dynamic_pointer_cast<pocketflow::BaseNode>(parallel_summaries));
    
    start_time = std::chrono::high_resolution_clock::now();
    
    try {
        auto future2 = parallel_flow.run_async(shared2);
        future2.get();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "✅ ParallelSummaries completed in " << duration.count() << "ms" << std::endl;
        
        if (shared2.contains("batch_stats")) {
            json stats = shared2["batch_stats"];
            std::cout << "Batch Statistics:" << std::endl;
            std::cout << "- Total texts: " << stats["total_texts"] << std::endl;
            std::cout << "- Average processing time: " << stats["average_processing_time_ms"] << "ms" << std::endl;
            std::cout << "- Combined summary length: " << stats["combined_length"] << " characters" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ ParallelSummaries failed: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << std::endl;
    
    // Example 3: AsyncParallelBatchFlow with file processing
    std::cout << "--- Example 3: FileProcessorFlow (AsyncParallelBatchFlow) ---" << std::endl;
    
    json shared3 = {
        {"files", json::array({
            "document1.txt",
            "document2.pdf", 
            "document3.docx",
            "document4.md"
        })},
        {"processing_mode", "enhanced"},
        {"output_format", "json"}
    };
    
    // Create sub-flow for individual file processing
    auto file_processor = std::make_shared<LoadAndProcessFile>();
    auto sub_flow = std::make_shared<pocketflow::AsyncFlow>(file_processor);
    
    // Create batch flow that uses the sub-flow
    auto batch_file_flow = std::make_shared<FileProcessorFlow>(sub_flow);
    
    start_time = std::chrono::high_resolution_clock::now();
    
    try {
        auto future3 = batch_file_flow->run_async(shared3);
        future3.get();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "✅ FileProcessorFlow completed in " << duration.count() << "ms" << std::endl;
        
        if (shared3.contains("files_processed")) {
            std::cout << "Files processed: " << shared3["files_processed"] << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ FileProcessorFlow failed: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << std::endl;
    
    // Performance comparison
    run_performance_comparison();
    
    std::cout << "=== Async Processing Examples Completed ===" << std::endl;
    
    return 0;
}