#include "vpu_core.h"
#include <iostream>
#include <string> // Required for std::string
#include <vector> // Required for std::vector in HAL calls
#include <memory> // Required for std::make_unique, std::make_shared

// Removed the local stub definition of VPU::Cortex from here.
// VPUCore will now use VPU::Cortex directly.

namespace VPU {

// --- Public API Implementation ---
VPU_Environment::VPU_Environment() : core(std::make_unique<VPUCore>()) {}
VPU_Environment::~VPU_Environment() {}
void VPU_Environment::execute(VPU_Task& task) { core->execute_task(task); }
void VPU_Environment::print_beliefs() { core->print_current_beliefs(); }


// --- VPUCore Implementation ---
VPUCore::VPUCore() {
    std::cout << "[VPU System] Core starting up..." << std::endl;

    // On initialization, all pillars are constructed and wired together.
    initialize_beliefs();
    initialize_hal();

    // Instantiate each pillar, providing shared access to necessary resources.
    pillar2_cortex_ = std::make_unique<VPU::Cortex>(); // Corrected instantiation
    pillar3_orchestrator_ = std::make_unique<Orchestrator>(hw_profile_);
    pillar4_cerebellum_ = std::make_unique<Cerebellum>(kernel_lib_);
    pillar5_feedback_ = std::make_unique<FeedbackLoop>(hw_profile_);

    std::cout << "[VPU System] All pillars are online. Ready." << std::endl;
}


// The full Perceive -> Decide -> Act -> Learn loop.
void VPUCore::execute_task(VPU_Task& task) {
    // 1. PERCEIVE: Use the Cortex to analyze the data.
    EnrichedExecutionContext context = pillar2_cortex_->analyze(task);
    // Direct use of RepresentationalFluxAnalyzer, profileOmni, or manual DataProfile creation is removed.

    // 2. DECIDE: Use the Orchestrator to select the best execution plan.
    ExecutionPlan plan = pillar3_orchestrator_->determine_optimal_path(context);

    // 3. ACT: Use the Cerebellum to execute the plan and record performance.
    ActualPerformanceRecord perf_record = pillar4_cerebellum_->execute(plan, task);

    // 4. LEARN: Use the Feedback Loop to compare prediction and reality.
    LearningContext learning_ctx;
    // Removed duplicate declaration: LearningContext learning_ctx;
    learning_ctx.path_name = plan.chosen_path_name;

    bool is_transform_focused = false;

    if (plan.chosen_path_name.find("FFT") != std::string::npos) {
        learning_ctx.transform_key = "TRANSFORM_TIME_TO_FREQ";
        // Potentially set main_operation_name = "ELEMENT_WISE_MULTIPLY" if its base cost is to be learned
        // operation_key might be less relevant here or specific to ELEMENT_WISE_MULTIPLY if it had one
        is_transform_focused = true;
    } else if (plan.chosen_path_name.find("JIT Compiled SAXPY") != std::string::npos) {
        learning_ctx.transform_key = "TRANSFORM_JIT_COMPILE_SAXPY";
        learning_ctx.main_operation_name = "EXECUTE_JIT_SAXPY";
        learning_ctx.operation_key = "lambda_SAXPY_generic"; // Sensitivity for the execution part
        is_transform_focused = true;
    }

    // If not primarily a transform-focused error path, identify main operation and its sensitivity.
    // This block will also execute for JIT paths to set keys for the execution part, if not already set,
    // but the JIT path above is more specific.
    // For pure direct paths, is_transform_focused will be false.
    if (!is_transform_focused || !learning_ctx.main_operation_name.empty()) { // Also process if main_op already set (e.g. JIT)
        if (task.task_type == "CONVOLUTION") {
            // This implies a direct convolution path was chosen if is_transform_focused is false
            if (!is_transform_focused) { // Only set if not an FFT path
                learning_ctx.main_operation_name = "CONV_DIRECT";
                learning_ctx.operation_key = "lambda_Conv_Amp"; // Correct sensitivity key for CONV_DIRECT
                // Or, could be "lambda_Conv_Freq" or a combination. For now, Amp is primary.
            }
        } else if (task.task_type == "GEMM") {
            for (const auto& step : plan.steps) {
                if (step.operation_name == "GEMM_NAIVE" || step.operation_name == "GEMM_FLUX_ADAPTIVE") {
                    learning_ctx.main_operation_name = step.operation_name;
                    break;
                }
            }
            learning_ctx.operation_key = "lambda_Sparsity";
        } else if (task.task_type == "SAXPY") {
            if (!is_transform_focused) { // Standard SAXPY path (JIT already handled)
                 learning_ctx.main_operation_name = "SAXPY_STANDARD";
                 learning_ctx.operation_key = "lambda_SAXPY_generic";
            }
            // If it was JIT, main_op and op_key are already set for EXECUTE_JIT_SAXPY
        }
    }

    pillar5_feedback_->learn_from_feedback(learning_ctx, plan.predicted_holistic_flux, perf_record);
}

void VPUCore::initialize_beliefs() {
    hw_profile_ = std::make_shared<HardwareProfile>();
    // Load initial beliefs (our "common sense" priors). These will be refined over time.
    hw_profile_->base_operational_costs = {
        {"CONV_DIRECT", 50000.0},
        {"ELEMENT_WISE_MULTIPLY", 10000.0}, // Part of the FFT path
        {"GEMM_NAIVE", 100000.0},
        {"GEMM_FLUX_ADAPTIVE", 300000.0},  // Higher base cost due to setup overhead
        {"SAXPY_STANDARD", 20000.0},
        {"EXECUTE_JIT_SAXPY", 5000.0}
    };
    hw_profile_->transform_costs = {
        {"TRANSFORM_TIME_TO_FREQ", 200000.0}, // Cost for one direction (e.g. R2C)
        {"FFT_FORWARD", 200000.0}, // Explicitly for FFT forward
        {"FFT_INVERSE", 200000.0},  // Explicitly for FFT inverse
        {"TRANSFORM_JIT_COMPILE_SAXPY", 75000.0} // Cost of JIT compilation itself
    };
    hw_profile_->flux_sensitivities = {
        // {"lambda_A", 100.0}, // Removed old generic lambda_A
        {"lambda_Conv_Amp", 100.0},      // Sensitivity of Convolution to amplitude flux
        {"lambda_Conv_Freq", 150.0},     // Sensitivity of Convolution to frequency flux
        {"lambda_Sparsity", 5000000.0},  // Sensitivity to density for GEMM
        {"lambda_SAXPY_generic", 10.0}   // Example sensitivity for SAXPY (remains)
    };
}

void VPUCore::initialize_hal() {
    kernel_lib_ = std::make_shared<HAL::KernelLibrary>();
    // These would be bound to actual data from VPU_Task in Cerebellum::execute
    // For now, they are parameterless lambdas as per HAL::GenericKernel type.
    // The actual data handling is conceptual in Cerebellum.
    // Dummy const vectors for input arguments where needed by HAL function signatures.
    const std::vector<double> const_dummy_double_in_vec;
    const std::vector<float> const_dummy_float_in_vec;

    (*kernel_lib_)["CONV_DIRECT"] = [](){
        std::cout << "    -> [HAL KERNEL STUB] CONV_DIRECT called." << std::endl;
        // HAL::cpu_conv_direct(params...);
    };
    (*kernel_lib_)["FFT_FORWARD"] = [const_dummy_double_in_vec](){
        std::vector<double> dummy_out_vec; // Mutable lvalue for output
        HAL::cpu_fft_forward(const_dummy_double_in_vec, dummy_out_vec);
    };
    (*kernel_lib_)["ELEMENT_WISE_MULTIPLY"] = [](){
        std::cout << "    -> [HAL KERNEL STUB] ELEMENT_WISE_MULTIPLY called." << std::endl;
        // HAL::cpu_element_wise_multiply(params...);
    };
    (*kernel_lib_)["FFT_INVERSE"] = [const_dummy_double_in_vec](){ // const_dummy_double_in_vec is for the first param
        std::vector<double> dummy_out_vec; // Mutable lvalue for output (second param)
        int N_dummy = const_dummy_double_in_vec.empty() ? 0 : (const_dummy_double_in_vec.size()/2 - 1) * 2; // Infer N if possible, or set default for stub
        if (N_dummy <= 0 && !const_dummy_double_in_vec.empty()) N_dummy = 10; // Default N if input not empty but N calc is weird for stub
        else if (const_dummy_double_in_vec.empty()) N_dummy = 0; // Handles empty input case for N

        HAL::cpu_fft_inverse(const_dummy_double_in_vec, dummy_out_vec, N_dummy);
        std::cout << "    -> [HAL KERNEL STUB CALL] FFT_INVERSE (via actual cpu_fft_inverse) called with N_dummy=" << N_dummy << std::endl;
    };
    (*kernel_lib_)["GEMM_NAIVE"] = [const_dummy_float_in_vec](){
        std::vector<float> dummy_out_vec; // Mutable lvalue for output
        HAL::cpu_gemm_naive(const_dummy_float_in_vec, const_dummy_float_in_vec, dummy_out_vec, 0,0,0);
    };
    (*kernel_lib_)["GEMM_FLUX_ADAPTIVE"] = [const_dummy_float_in_vec](){
        std::vector<float> dummy_out_vec; // Mutable lvalue for output
        HAL::cpu_gemm_flux_adaptive(const_dummy_float_in_vec, const_dummy_float_in_vec, dummy_out_vec, 0,0,0);
    };
    (*kernel_lib_)["SAXPY_STANDARD"] = [](){ // Removed [this] capture, not needed for stub
        // Conceptual: need to get actual data for task from VPU_Task passed to Cerebellum
        // For this stub, we just call the HAL function with dummy values.
        // HAL::cpu_saxpy expects float, const vector<float>&, vector<float>&
        std::vector<float> x_dummy, y_dummy;
        // x_dummy.resize(AppropriateSize); // In a real scenario, size might be known.
        // y_dummy.resize(AppropriateSize);
        HAL::cpu_saxpy(1.0f, x_dummy, y_dummy); // Example alpha = 1.0f
        std::cout << "    -> [HAL KERNEL STUB] SAXPY_STANDARD (via cpu_saxpy) called." << std::endl;
    };
    (*kernel_lib_)["EXECUTE_JIT_SAXPY"] = [](){
        std::cout << "    -> [HAL KERNEL STUB] Conceptually executing JIT-compiled SAXPY." << std::endl;
        // Actual execution would use a kernel compiled by FluxJITEngine passed via Cerebellum
    };

}


void VPUCore::print_current_beliefs() {
    std::cout << "\n===== VPU CURRENT BELIEFS =====\n";
    std::cout << "--- Base Operational Costs ---\n";
    for(const auto& [key, val] : hw_profile_->base_operational_costs) {
        std::cout << "  '" << key << "': " << val << "\n";
    }
    std::cout << "--- Transform Costs ---\n";
    for(const auto& [key, val] : hw_profile_->transform_costs) {
        std::cout << "  '" << key << "': " << val << "\n";
    }
    std::cout << "--- Flux Sensitivities ---\n";
    for(const auto& [key, val] : hw_profile_->flux_sensitivities) {
        std::cout << "  '" << key << "': " << val << "\n";
    }
     std::cout << "==============================\n" << std::endl;
}

} // namespace VPU
