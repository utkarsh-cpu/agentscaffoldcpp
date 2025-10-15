#include <iostream>
#include <memory>
#include <chrono>
#include <random>
#include "../include/pocketflow/pocketflow.hpp"

using json = nlohmann::json;

/**
 * DecisionNode - Makes decisions about how to handle queries
 * Demonstrates conditional logic and action-based flow control
 */
class DecisionNode : public pocketflow::Node {
private:
    std::mt19937 rng_;
    
public:
    DecisionNode() : Node(1, 0), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    json prep(const json& shared) override {
        return json{
            {"query", shared["query"]},
            {"context", shared.value("context", "")},
            {"conversation_history", shared.value("conversation_history", json::array())},
            {"confidence_threshold", shared.value("confidence_threshold", 0.7)}
        };
    }
    
    json exec(const json& prep_result) override {
        std::string query = prep_result["query"];
        std::string context = prep_result["context"];
        double threshold = prep_result["confidence_threshold"];
        
        std::cout << "🤔 DecisionNode analyzing query: \"" << query << "\"" << std::endl;
        
        // Simulate decision-making process
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Simple decision logic based on query content
        std::string action;
        std::string reason;
        double confidence;
        
        // Check for search-related keywords
        if (query.find("search") != std::string::npos || 
            query.find("find") != std::string::npos ||
            query.find("lookup") != std::string::npos ||
            query.find("what is") != std::string::npos ||
            query.find("who is") != std::string::npos) {
            
            action = "search";
            reason = "Query requires external information lookup";
            confidence = 0.85;
            
        } else if (query.find("calculate") != std::string::npos ||
                   query.find("compute") != std::string::npos ||
                   query.find("math") != std::string::npos ||
                   query.find("solve") != std::string::npos) {
            
            action = "calculate";
            reason = "Query requires mathematical computation";
            confidence = 0.9;
            
        } else if (context.length() > 50) {
            // If we have sufficient context, try to answer directly
            action = "answer";
            reason = "Sufficient context available for direct response";
            confidence = 0.75;
            
        } else {
            // Simulate uncertainty - sometimes need more info
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double random_confidence = dist(rng_);
            
            if (random_confidence > threshold) {
                action = "answer";
                reason = "Attempting direct response based on available knowledge";
                confidence = random_confidence;
            } else {
                action = "search";
                reason = "Insufficient confidence, need more information";
                confidence = random_confidence;
            }
        }
        
        return json{
            {"action", action},
            {"reason", reason},
            {"confidence", confidence},
            {"query_analysis", {
                {"length", query.length()},
                {"context_available", !context.empty()},
                {"keywords_detected", json::array()}
            }}
        };
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        // Update shared state with decision
        const_cast<json&>(shared)["last_decision"] = exec_result;
        const_cast<json&>(shared)["decision_count"] = shared.value("decision_count", 0) + 1;
        
        std::string action = exec_result["action"];
        std::string reason = exec_result["reason"];
        double confidence = exec_result["confidence"];
        
        std::cout << "📋 Decision: " << action << " (confidence: " << std::fixed << std::setprecision(2) 
                  << confidence << ")" << std::endl;
        std::cout << "   Reason: " << reason << std::endl;
        
        // Return the action for flow control
        return action;
    }
};

/**
 * SearchNode - Performs information search/retrieval
 * Demonstrates external data gathering simulation
 */
class SearchNode : public pocketflow::Node {
private:
    std::mt19937 rng_;
    
public:
    SearchNode() : Node(2, 100), rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {}
    
    json prep(const json& shared) override {
        return json{
            {"query", shared["query"]},
            {"search_type", shared.value("search_type", "web")},
            {"max_results", shared.value("max_results", 5)}
        };
    }
    
    json exec(const json& prep_result) override {
        std::string query = prep_result["query"];
        std::string search_type = prep_result["search_type"];
        int max_results = prep_result["max_results"];
        
        std::cout << "🔍 SearchNode performing " << search_type << " search for: \"" << query << "\"" << std::endl;
        
        // Simulate search operation with variable delay
        std::uniform_int_distribution<int> delay_dist(200, 800);
        int delay = delay_dist(rng_);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        
        // Simulate search results
        json search_results = json::array();
        std::uniform_int_distribution<int> result_count_dist(1, max_results);
        int num_results = result_count_dist(rng_);
        
        for (int i = 0; i < num_results; ++i) {
            search_results.push_back({
                {"title", "Search Result " + std::to_string(i + 1) + " for: " + query},
                {"url", "https://example.com/result" + std::to_string(i + 1)},
                {"snippet", "This is a relevant snippet for " + query + " from source " + std::to_string(i + 1)},
                {"relevance_score", 0.9 - (i * 0.1)},
                {"source", "Source " + std::to_string(i + 1)}
            });
        }
        
        // Simulate occasional search failures for demonstration
        std::uniform_real_distribution<double> failure_dist(0.0, 1.0);
        if (failure_dist(rng_) < 0.1) {  // 10% chance of failure
            throw std::runtime_error("Search service temporarily unavailable");
        }
        
        return json{
            {"results", search_results},
            {"query", query},
            {"search_type", search_type},
            {"total_results", num_results},
            {"search_time_ms", delay}
        };
    }
    
    json exec_fallback(const json& prep_result, const std::exception& exc) override {
        std::cout << "⚠️  Search failed: " << exc.what() << std::endl;
        std::cout << "   Using cached/default results..." << std::endl;
        
        // Return minimal fallback results
        return json{
            {"results", json::array({
                {
                    {"title", "Fallback result for: " + prep_result["query"].get<std::string>()},
                    {"snippet", "Limited information available due to search service issues"},
                    {"relevance_score", 0.3},
                    {"source", "Cache"}
                }
            })},
            {"query", prep_result["query"]},
            {"search_type", "fallback"},
            {"total_results", 1},
            {"search_time_ms", 0},
            {"fallback_used", true}
        };
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        // Update shared state with search results
        const_cast<json&>(shared)["search_results"] = exec_result["results"];
        const_cast<json&>(shared)["context"] = ""; // Build context from results
        
        // Aggregate search results into context
        std::string context;
        for (const auto& result : exec_result["results"]) {
            context += result["snippet"].get<std::string>() + " ";
        }
        const_cast<json&>(shared)["context"] = context;
        
        int num_results = exec_result["total_results"];
        int search_time = exec_result["search_time_ms"];
        bool fallback = exec_result.value("fallback_used", false);
        
        if (fallback) {
            std::cout << "🔍 Search completed with fallback: " << num_results << " results" << std::endl;
        } else {
            std::cout << "🔍 Search completed: " << num_results << " results in " << search_time << "ms" << std::endl;
        }
        
        // Return action to go back to decision node for re-evaluation
        return "decide";
    }
};

/**
 * CalculateNode - Performs mathematical computations
 * Demonstrates specialized processing nodes
 */
class CalculateNode : public pocketflow::Node {
public:
    CalculateNode() : Node(1, 0) {}
    
    json prep(const json& shared) override {
        return json{
            {"query", shared["query"]},
            {"context", shared.value("context", "")}
        };
    }
    
    json exec(const json& prep_result) override {
        std::string query = prep_result["query"];
        
        std::cout << "🧮 CalculateNode processing: \"" << query << "\"" << std::endl;
        
        // Simulate calculation processing
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        
        // Simple calculation logic (for demonstration)
        json calculation_result;
        
        if (query.find("2+2") != std::string::npos || query.find("2 + 2") != std::string::npos) {
            calculation_result = {
                {"expression", "2 + 2"},
                {"result", 4},
                {"type", "arithmetic"}
            };
        } else if (query.find("square root") != std::string::npos) {
            calculation_result = {
                {"expression", "sqrt(16)"},
                {"result", 4.0},
                {"type", "mathematical_function"}
            };
        } else {
            calculation_result = {
                {"expression", "complex_calculation"},
                {"result", 42},
                {"type", "general_computation"},
                {"note", "Simulated result for demonstration"}
            };
        }
        
        return json{
            {"calculation", calculation_result},
            {"query", query},
            {"processing_time_ms", 150}
        };
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        // Update shared state with calculation results
        const_cast<json&>(shared)["calculation_result"] = exec_result["calculation"];
        
        json calc = exec_result["calculation"];
        std::cout << "🧮 Calculation completed: " << calc["expression"] << " = " << calc["result"] << std::endl;
        
        // Go to answer node to formulate response
        return "answer";
    }
};

/**
 * AnswerNode - Formulates final responses
 * Demonstrates response generation and conversation management
 */
class AnswerNode : public pocketflow::Node {
public:
    AnswerNode() : Node(1, 0) {}
    
    json prep(const json& shared) override {
        return json{
            {"query", shared["query"]},
            {"context", shared.value("context", "")},
            {"search_results", shared.value("search_results", json::array())},
            {"calculation_result", shared.value("calculation_result", json::object())},
            {"conversation_history", shared.value("conversation_history", json::array())}
        };
    }
    
    json exec(const json& prep_result) override {
        std::string query = prep_result["query"];
        std::string context = prep_result["context"];
        
        std::cout << "💬 AnswerNode generating response for: \"" << query << "\"" << std::endl;
        
        // Simulate response generation
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::string response;
        std::string response_type;
        
        // Check if we have calculation results
        if (!prep_result["calculation_result"].empty()) {
            json calc = prep_result["calculation_result"];
            response = "The answer to your calculation is: " + calc["result"].dump() + 
                      " (Expression: " + calc["expression"].get<std::string>() + ")";
            response_type = "calculation_response";
            
        } else if (!prep_result["search_results"].empty()) {
            // Use search results to formulate response
            json results = prep_result["search_results"];
            response = "Based on my search, here's what I found: ";
            
            for (size_t i = 0; i < std::min(results.size(), size_t(2)); ++i) {
                response += results[i]["snippet"].get<std::string>();
                if (i < std::min(results.size(), size_t(2)) - 1) {
                    response += " ";
                }
            }
            response_type = "search_based_response";
            
        } else if (!context.empty()) {
            // Use available context
            response = "Based on the available information: " + context.substr(0, 200);
            if (context.length() > 200) response += "...";
            response_type = "context_based_response";
            
        } else {
            // General response
            response = "I understand your question about: " + query + 
                      ". However, I need more specific information to provide a detailed answer.";
            response_type = "general_response";
        }
        
        return json{
            {"response", response},
            {"response_type", response_type},
            {"query", query},
            {"confidence", 0.8},
            {"sources_used", !prep_result["search_results"].empty()}
        };
    }
    
    json post(const json& shared, const json& prep_result, const json& exec_result) override {
        // Update conversation history
        json conversation_entry = {
            {"query", exec_result["query"]},
            {"response", exec_result["response"]},
            {"response_type", exec_result["response_type"]},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()}
        };
        
        json history = shared.value("conversation_history", json::array());
        history.push_back(conversation_entry);
        const_cast<json&>(shared)["conversation_history"] = history;
        const_cast<json&>(shared)["final_response"] = exec_result["response"];
        
        std::string response = exec_result["response"];
        std::string type = exec_result["response_type"];
        
        std::cout << "💬 Response generated (" << type << "):" << std::endl;
        std::cout << "   \"" << response << "\"" << std::endl;
        
        return json{};  // End of conversation flow
    }
};

/**
 * Main function demonstrating agent pattern with conditional branching
 */
int main() {
    std::cout << "=== PocketFlow-CPP Agent Pattern Example ===" << std::endl;
    std::cout << std::endl;
    
    // Test queries to demonstrate different flow paths
    std::vector<std::string> test_queries = {
        "What is the capital of France?",
        "Calculate 2+2",
        "Search for information about machine learning",
        "Who is the current president?",
        "Compute the square root of 16",
        "Find recent news about artificial intelligence"
    };
    
    for (const auto& query : test_queries) {
        std::cout << "--- Processing Query: \"" << query << "\" ---" << std::endl;
        
        // Initialize shared state for this query
        json shared = {
            {"query", query},
            {"context", ""},
            {"confidence_threshold", 0.6},
            {"search_type", "web"},
            {"max_results", 3},
            {"conversation_history", json::array()}
        };
        
        // Create nodes
        auto decision = std::make_shared<DecisionNode>();
        auto search = std::make_shared<SearchNode>();
        auto calculate = std::make_shared<CalculateNode>();
        auto answer = std::make_shared<AnswerNode>();
        
        // Build agent flow with conditional branching
        // This demonstrates the key syntax: node - "action" >> target
        decision - "search" >> search;
        decision - "calculate" >> calculate;
        decision - "answer" >> answer;
        
        // Search results loop back to decision for re-evaluation
        search - "decide" >> decision;
        
        // Calculations go directly to answer
        calculate - "answer" >> answer;
        
        std::cout << "Flow structure:" << std::endl;
        std::cout << "  DecisionNode -\"search\"-> SearchNode -\"decide\"-> DecisionNode" << std::endl;
        std::cout << "  DecisionNode -\"calculate\"-> CalculateNode -\"answer\"-> AnswerNode" << std::endl;
        std::cout << "  DecisionNode -\"answer\"-> AnswerNode" << std::endl;
        std::cout << std::endl;
        
        // Create and execute agent flow
        pocketflow::Flow agent(decision);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            agent.run(shared);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            std::cout << std::endl;
            std::cout << "✅ Query processed successfully in " << duration.count() << "ms" << std::endl;
            
            // Show final response
            if (shared.contains("final_response")) {
                std::cout << "Final Response: \"" << shared["final_response"] << "\"" << std::endl;
            }
            
            // Show decision count (indicates how many decision cycles occurred)
            if (shared.contains("decision_count")) {
                std::cout << "Decision cycles: " << shared["decision_count"] << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "❌ Query processing failed: " << e.what() << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << "=== Agent Pattern Example Completed ===" << std::endl;
    std::cout << std::endl;
    
    // Demonstrate interactive mode
    std::cout << "--- Interactive Mode Demo ---" << std::endl;
    std::cout << "Enter a query (or 'quit' to exit): ";
    
    std::string user_query;
    while (std::getline(std::cin, user_query) && user_query != "quit") {
        if (user_query.empty()) {
            std::cout << "Enter a query (or 'quit' to exit): ";
            continue;
        }
        
        json shared = {
            {"query", user_query},
            {"context", ""},
            {"confidence_threshold", 0.6}
        };
        
        // Create fresh nodes for each query
        auto decision = std::make_shared<DecisionNode>();
        auto search = std::make_shared<SearchNode>();
        auto calculate = std::make_shared<CalculateNode>();
        auto answer = std::make_shared<AnswerNode>();
        
        // Set up flow
        decision - "search" >> search;
        decision - "calculate" >> calculate;
        decision - "answer" >> answer;
        search - "decide" >> decision;
        calculate - "answer" >> answer;
        
        pocketflow::Flow agent(decision);
        
        try {
            agent.run(shared);
            std::cout << "Response: " << shared.value("final_response", "No response generated") << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "Enter a query (or 'quit' to exit): ";
    }
    
    std::cout << "Goodbye!" << std::endl;
    
    return 0;
}