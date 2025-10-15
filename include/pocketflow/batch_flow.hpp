#pragma once

#include "flow.hpp"

namespace pocketflow {

/**
 * BatchFlow - Processes batches of parameters through a flow
 *
 * BatchFlow extends Flow to handle batch processing where the prep phase
 * returns an array of parameter sets, and each set is processed through
 * the flow orchestration.
 */
class BatchFlow : public virtual Flow {
public:
  /**
   * Constructor
   * @param start Optional start node for the flow
   */
  explicit BatchFlow(std::shared_ptr<BaseNode> start = nullptr) : Flow(start) {}

protected:
  /**
   * Override _run for batch parameter processing
   * Iterates over batch parameters and runs orchestration for each set
   * @param shared Shared state object (mutable)
   * @return Result from post phase
   */
  json _run(json &shared) override {
    // Execute prep phase to get batch parameters
    json prep_res = prep(shared);

    // If prep_res is an array, iterate over each batch parameter set
    if (prep_res.is_array()) {
      for (const auto &batch_params : prep_res) {
        // Combine flow parameters with individual batch parameters
        json combined_params = params_;

        // Merge batch_params into combined_params if it's an object
        if (batch_params.is_object()) {
          if (combined_params.is_null()) {
            combined_params = json::object();
          }
          combined_params.update(batch_params);
        }

        // Run orchestration with combined parameters
        _orch(shared, combined_params);
      }
    }

    // Execute post phase and return result
    return post(shared, prep_res, json::object());
  }
};

} // namespace pocketflow