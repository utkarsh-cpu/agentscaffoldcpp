#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include "../include/pocketflow/pocketflow.hpp"

using json = nlohmann::json;

/**
 * DataLoader - Loads input data into the shared state
 * Demonstrates the prep() method for data initialization
 */
class DataLoader : public pocketflow::Node {
public:
    DataLoader() : Node(1, 0) {}  // No retries needed for data loading
    
    json prep(const json& shared) override {
        // Extract configuration from shared state
        return json{
            {"source", shared.value("data_source", "default_input.txt")},
            {"format", shared.value("input_format", "json")}
        };
    }
    
    json exec(const json& prep_result) override {
        // Simulate loading data from a source
        std::string source = prep_result["source"];
        std::string format = prep_result["format"];
        
        std::cout << "Loading data from: " << source << " (format: " << format << ")\n";
        
        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Return loaded data
        return json{
            {"raw_data", json::array({
                json{{"id", 1}, {"name", "Alice"}, {"score", 85}},
                json{{"id", 2}, {"name", "Bob"}, {"score", 92}},
                json{{"id", 3}, {"name", "Charlie"}, {"score", 78}}
            })},
            {"metadata", {
                {"source", source},
                {"loaded_at", std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()},
                {"record_count", 3}
            }}
        };
    }
    
    json post(const json& shared, const json& /* prep_result */, const json& exec_result) override {
        // Update shared state with loaded data
        json& mutable_shared = const_cast<json&>(shared);
        mutable_shared["loaded_data"] = exec_result["raw_data"];
        mutable_shared["load_metadata"] = exec_result["metadata"];
        
        std::cout << "✓ Data loaded successfully: " << exec_result["metadata"]["record_count"] 
                  << " records\n";
        
        return json{};  // Return empty for "default" action
    }
};

/**
 * DataProcessor - Processes the loaded data
 * Demonstrates the exec() method for data transformation
 */
class DataProcessor : public pocketflow::Node {
public:
    DataProcessor() : Node(2, 100) {}  // Allow 2 retries with 100ms wait
    
    json prep(const json& shared) override {
        // Get data from shared state and processing parameters
        if (!shared.contains("loaded_data")) {
            throw std::runtime_error("DataProcessor: loaded_data not found in shared state");
        }
        return json{
            {"data", shared["loaded_data"]},
            {"processing_mode", shared.value("processing_mode", "standard")},
            {"threshold", shared.value("score_threshold", 80)}
        };
    }
    
    json exec(const json& prep_result) override {
        try {
            json data = prep_result["data"];
            std::string mode = prep_result["processing_mode"];
            int threshold = prep_result["threshold"];
            
            std::cout << "Processing data in " << mode << " mode (threshold: " << threshold << ")\n";
            
            // Simulate processing time
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            json processed_data = json::array();
            int high_performers = 0;
            
            for (const auto& record : data) {
                json processed_record = record;
                int score = record["score"];
                
                // Add computed fields
                processed_record["grade"] = (score >= 90) ? "A" : 
                                           (score >= 80) ? "B" : 
                                           (score >= 70) ? "C" : "D";
                processed_record["high_performer"] = (score >= threshold);
                
                if (score >= threshold) {
                    high_performers++;
                }
                
                processed_data.push_back(processed_record);
            }
            
            return json{
                {"processed_records", processed_data},
                {"statistics", {
                    {"total_records", data.size()},
                    {"high_performers", high_performers},
                    {"processing_mode", mode},
                    {"threshold_used", threshold}
                }}
            };
        } catch (const std::exception& e) {
            std::cout << "DataProcessor exec error: " << e.what() << "\n";
            throw;
        }
    }
    
    json post(const json& shared, const json& /* prep_result */, const json& exec_result) override {
        // Update shared state with processed data
        json& mutable_shared = const_cast<json&>(shared);
        mutable_shared["processed_data"] = exec_result["processed_records"];
        mutable_shared["processing_stats"] = exec_result["statistics"];
        
        int high_performers = exec_result["statistics"]["high_performers"];
        int total = exec_result["statistics"]["total_records"];
        
        std::cout << "✓ Data processed: " << high_performers << "/" << total 
                  << " high performers identified\n";
        
        return json{};  // Continue to next node
    }
};

/**
 * DataValidator - Validates the processed data
 * Demonstrates error handling and validation logic
 */
class DataValidator : public pocketflow::Node {
public:
    DataValidator() : Node(1, 0) {}
    
    json prep(const json& shared) override {
        return json{
            {"data", shared["processed_data"]},
            {"stats", shared["processing_stats"]},
            {"validation_rules", shared.value("validation_rules", json::object())}
        };
    }
    
    json exec(const json& prep_result) override {
        json data = prep_result["data"];
        
        std::cout << "Validating processed data...\n";
        
        // Simulate validation time
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        json validation_results = json::array();
        bool all_valid = true;
        
        for (const auto& record : data) {
            json validation = {
                {"id", record["id"]},
                {"valid", true},
                {"issues", json::array()}
            };
            
            // Validate required fields
            if (!record.contains("name") || record["name"].get<std::string>().empty()) {
                validation["valid"] = false;
                validation["issues"].push_back("Missing or empty name");
                all_valid = false;
            }
            
            if (!record.contains("score") || record["score"] < 0 || record["score"] > 100) {
                validation["valid"] = false;
                validation["issues"].push_back("Invalid score range");
                all_valid = false;
            }
            
            if (!record.contains("grade")) {
                validation["valid"] = false;
                validation["issues"].push_back("Missing grade assignment");
                all_valid = false;
            }
            
            validation_results.push_back(validation);
        }
        
        return json{
            {"validation_results", validation_results},
            {"all_valid", all_valid},
            {"total_validated", data.size()},
            {"validation_passed", all_valid}
        };
    }
    
    json post(const json& shared, const json& /* prep_result */, const json& exec_result) override {
        // Update shared state with validation results
        json& mutable_shared = const_cast<json&>(shared);
        mutable_shared["validation_results"] = exec_result["validation_results"];
        mutable_shared["data_valid"] = exec_result["all_valid"];
        
        bool all_valid = exec_result["all_valid"];
        int total = exec_result["total_validated"];
        
        if (all_valid) {
            std::cout << "✓ Validation passed: All " << total << " records are valid\n";
        } else {
            std::cout << "⚠ Validation issues found in some records\n";
        }
        
        return json{};  // Continue regardless of validation results
    }
};

/**
 * DataSaver - Saves the final results
 * Demonstrates the post() method for output operations
 */
class DataSaver : public pocketflow::Node {
public:
    DataSaver() : Node(2, 50) {}  // Allow retries for save operations
    
    json prep(const json& shared) override {
        return json{
            {"data", shared["processed_data"]},
            {"validation", shared["validation_results"]},
            {"stats", shared["processing_stats"]},
            {"output_path", shared.value("output_path", "output.json")},
            {"save_format", shared.value("output_format", "json")}
        };
    }
    
    json exec(const json& prep_result) override {
        std::string output_path = prep_result["output_path"];
        std::string format = prep_result["save_format"];
        
        std::cout << "Saving results to: " << output_path << " (format: " << format << ")\n";
        
        // Simulate save operation
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        
        // Create final output structure
        json output = {
            {"data", prep_result["data"]},
            {"validation", prep_result["validation"]},
            {"statistics", prep_result["stats"]},
            {"metadata", {
                {"saved_at", std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()},
                {"output_path", output_path},
                {"format", format}
            }}
        };
        
        return json{
            {"output_data", output},
            {"save_path", output_path},
            {"bytes_written", output.dump().length()}
        };
    }
    
    json post(const json& shared, const json& /* prep_result */, const json& exec_result) override {
        // Update shared state with final results
        json& mutable_shared = const_cast<json&>(shared);
        mutable_shared["final_output"] = exec_result["output_data"];
        mutable_shared["save_info"] = {
            {"path", exec_result["save_path"]},
            {"size_bytes", exec_result["bytes_written"]}
        };
        
        int bytes = exec_result["bytes_written"];
        std::string path = exec_result["save_path"];
        
        std::cout << "✓ Results saved successfully: " << bytes << " bytes written to " << path << "\n";
        
        return json{};  // End of pipeline
    }
};

/**
 * Main function demonstrating basic sequential flow
 */
int main() {
    std::cout << "=== PocketFlow-CPP Basic Sequential Flow Example ===\n";
    std::cout << "\n";
    
    // Initialize shared state with input parameters
    json shared = {
        {"data_source", "student_scores.csv"},
        {"input_format", "csv"},
        {"processing_mode", "enhanced"},
        {"score_threshold", 85},
        {"output_path", "processed_results.json"},
        {"output_format", "json"},
        {"validation_rules", {
            {"require_name", true},
            {"score_range", {0, 100}}
        }}
    };
    
    std::cout << "Initial shared state:\n";
    std::cout << shared.dump(2) << "\n";
    std::cout << "\n";
    
    // Create nodes
    auto loader = std::make_shared<DataLoader>();
    auto processor = std::make_shared<DataProcessor>();
    auto validator = std::make_shared<DataValidator>();
    auto saver = std::make_shared<DataSaver>();
    
    // Build sequential flow using >> operator
    std::cout << "Building flow: DataLoader >> DataProcessor >> DataValidator >> DataSaver\n";
    loader >> processor >> validator >> saver;
    
    // Create and execute flow
    pocketflow::Flow pipeline(loader);
    
    std::cout << "\n";
    std::cout << "--- Executing Pipeline ---\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        pipeline.run(shared);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\n";
        std::cout << "--- Pipeline Completed Successfully ---\n";
        std::cout << "Execution time: " << duration.count() << "ms\n";
        std::cout << "\n";
        
        // Display final results
        std::cout << "Final shared state:\n";
        std::cout << shared.dump(2) << "\n";
        
        // Show summary statistics
        if (shared.contains("processing_stats")) {
            json stats = shared["processing_stats"];
            std::cout << "\n";
            std::cout << "Processing Summary:\n";
            std::cout << "- Total records: " << stats["total_records"] << "\n";
            std::cout << "- High performers: " << stats["high_performers"] << "\n";
            std::cout << "- Processing mode: " << stats["processing_mode"] << "\n";
            std::cout << "- Threshold used: " << stats["threshold_used"] << "\n";
        }
        
        if (shared.contains("save_info")) {
            json save_info = shared["save_info"];
            std::cout << "- Output size: " << save_info["size_bytes"] << " bytes\n";
            std::cout << "- Saved to: " << save_info["path"] << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "\n";
        std::cout << "❌ Pipeline failed with error: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n";
    std::cout << "=== Example completed successfully ===\n";
    
    return 0;
}