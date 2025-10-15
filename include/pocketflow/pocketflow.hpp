#pragma once

/**
 * PocketFlow-CPP: A minimalist C++ LLM framework
 * 
 * This is the main header file that includes all PocketFlow components.
 * Include this file to use the PocketFlow framework in your project.
 */

#include "base_node.hpp"
#include "node.hpp"
#include "batch_node.hpp"
#include "flow.hpp"
#include "batch_flow.hpp"
#include "async_node.hpp"
#include "async_batch_node.hpp"
#include "async_flow.hpp"
#include "async_batch_flow.hpp"
#include "exceptions.hpp"

namespace pocketflow {
    // Version information
    constexpr int VERSION_MAJOR = 1;
    constexpr int VERSION_MINOR = 0;
    constexpr int VERSION_PATCH = 0;
    constexpr const char* VERSION_STRING = "1.0.0";
}