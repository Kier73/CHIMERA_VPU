#include "vpu.h" // The only header a user needs
#include <iostream>
#include <vector>
#include <string> // Required for VPU_Task task_type

// This demonstration shows the full Perceive->Decide->Act->Learn cognitive cycle.
// We will run two jobs. Job 1 will trigger a learning event. Job 2 will show
// how the VPU's decision is more accurate because of what it learned in Job 1.

int main() {
    std::cout << "===== VPU BOOTSTRAP SEQUENCE STARTING =====\n" << std::endl;
    // 1. Initialize the VPU Environment
    // This creates the VPUCore and all 5 pillars.
    VPU::VPU_Environment vpu; // Corrected: Added VPU namespace
    vpu.print_beliefs();

    // 2. Define Data Scenarios
    std::vector<double> spiky_signal = {0, 0, 100, -100, 0, 0, 100, -100, 0, 0}; // High Amplitude Flux
    std::vector<double> smooth_signal = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // Low Amplitude Flux

    //---------------------------------------------------------------------------------
    // JOB 1: A task with spiky data. The VPU will make a prediction, but we will
    //        simulate the actual hardware being unexpectedly slow, forcing a lesson.
    //---------------------------------------------------------------------------------
    std::cout << "\n\n======== RUNNING JOB 1: CONVOLUTION ON SPIKY DATA (Simulating Slow Hardware) ========\n" << std::endl;
    VPU::VPU_Task task1 = {"CONVOLUTION", spiky_signal.data(), nullptr, nullptr, spiky_signal.size()};

    // --- MANUALLY SIMULATE REALITY ---
    // The Cerebellum measures actual performance. Here, we can override the simple
    // timers to inject a "Flux Quark". Let's say the FFT transform was much more
    // expensive than our initial belief. We can do this by modifying the HAL to be slower.
    // For this demo, the learning loop itself is what matters. We will just note the effect.
    // In a real test, we might have a way to tell the mock HAL to behave differently for this run.
    // For example, by setting an environment variable or a global flag that HAL::cpu_fft_forward checks.
    // This is currently not implemented in the HAL stubs, so the "observed flux" will be based on
    // the conceptual std::chrono timer in Cerebellum, which will be very fast for stubbed HAL calls.
    // To make learning happen, Pillar5_Feedback.cpp has a QUARK_THRESHOLD.
    // If predicted and observed are too close, no learning happens.
    // The current VPUCore::initialize_beliefs() sets TRANSFORM_TIME_TO_FREQ to 200000.0.
    // The Orchestrator will predict a cost based on this for the FFT path.
    // The Cerebellum's timer on stubbed functions will be near zero.
    // This will cause (0 - 200000)/200000 = -1.0 deviation (-100%), triggering learning.

    vpu.execute(task1);

    std::cout << "\n\n>>>>> VPU BELIEFS AFTER JOB 1 <<<<<";
    vpu.print_beliefs();

    // ANALYSIS OF JOB 1:
    // - Cortex will find a high Amplitude Flux in `spiky_signal`.
    // - Orchestrator, seeing high dynamic cost for the "Direct" path (e.g. lambda_A * AF),
    //   might choose the "FFT" path if its total predicted flux (transform_cost + op_cost_fft_multiply) is lower.
    // - Cerebellum will execute. Since HAL calls are stubs, observed flux will be near zero.
    // - FeedbackLoop will detect a large negative deviation (observed much lower than predicted),
    //   assume the `TRANSFORM_TIME_TO_FREQ` (or `FFT_FORWARD`) cost was overestimated, and DECREASE its belief.


    //---------------------------------------------------------------------------------
    // JOB 2: The exact same task is run again.
    //---------------------------------------------------------------------------------
    std::cout << "\n\n======== RUNNING JOB 2: CONVOLUTION ON SPIKY DATA (NOW WITH UPDATED BELIEFS) ========\n" << std::endl;
    VPU::VPU_Task task2 = {"CONVOLUTION", spiky_signal.data(), nullptr, nullptr, spiky_signal.size()};

    // This time, the prediction for TRANSFORM_TIME_TO_FREQ will be much lower (due to learning in Job 1).
    // The observed flux will still be near zero.
    // The deviation should be smaller. If it's within QUARK_THRESHOLD, beliefs stabilize.
    vpu.execute(task2);

    std::cout << "\n\n>>>>> FINAL VPU BELIEFS AFTER JOB 2 <<<<<";
    vpu.print_beliefs();

    // ANALYSIS OF JOB 2:
    // - Cortex perceives the same high flux.
    // - Orchestrator now consults the UPDATED beliefs. The cost for the FFT path (specifically the transform part) is now lower.
    //   The decision will likely still be FFT path. The predicted flux will be lower than Job 1's prediction.
    // - Cerebellum executes. Observed flux is still near zero.
    // - FeedbackLoop calculates a deviation. If the new prediction is close enough to zero,
    //   the deviation might be small enough to be within QUARK_THRESHOLD, and beliefs stabilize.


    std::cout << "\n\n======== RUNNING JOB 3: CONVOLUTION ON SMOOTH DATA ========\n" << std::endl;
    VPU::VPU_Task task3 = {"CONVOLUTION", smooth_signal.data(), nullptr, nullptr, smooth_signal.size()};

    vpu.execute(task3);

    // ANALYSIS OF JOB 3:
    // - Cortex will perceive a very LOW amplitude flux for `smooth_signal`.
    // - Orchestrator will calculate the dynamic cost for the "Direct" path (lambda_A * AF) as being very low.
    // - Even with the reduced FFT transform cost (from Job 1 & 2's learning), the "Direct" path's total flux
    //   (base_op_cost_direct + dynamic_cost_direct) might now be lower than the FFT path's total flux.
    // - If so, Orchestrator will choose the "Direct" path.
    // - Observed flux will be near zero. Prediction for direct path is base_direct + low_dynamic.
    // - Learning may or may not occur depending on how close this prediction is to zero.

    //---------------------------------------------------------------------------------
    // JOB 4: A SAXPY task to test JIT compilation path.
    //---------------------------------------------------------------------------------
    std::vector<float> saxpy_x = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // 50% sparse
    std::vector<float> saxpy_y_orig = {10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f, 10.0f};
    std::vector<float> saxpy_y_result = saxpy_y_orig; // VPU will modify this (hopefully)
    // float saxpy_a = 2.0f; // Actual 'a' for this task.
    // Note: The JIT engine currently uses a fixed 'a=1.0f'.
    // The standard SAXPY HAL stub in VPUCore also uses a fixed 'a=1.0f'.
    // This test will proceed with task data for x and y, but the underlying fixed 'a' in current stubs should be remembered.

    std::cout << "\n\n======== RUNNING JOB 4: SAXPY TASK (Testing JIT Path) ========\n" << std::endl;
    // For SAXPY, VPU_Task needs 'a' (scalar), 'x' (vector), 'y' (vector to be modified).
    // Current VPU_Task: data_in_a (for x), data_in_b (not used by current SAXPY), data_out (for y).
    // 'a' is not directly supported by VPU_Task. The JIT engine and SAXPY_STANDARD stub use a fixed 'a'.
    // We will pass saxpy_x as data_in_a and saxpy_y_result as data_out.
    // Important: VPU_Task expects data_in_a to be const void*. Cortex expects const double* for profiling.
    //            SAXPY HAL and JIT kernels expect float*. This is a type mismatch.
    //            For this test to pass through Cortex, data_in_a should point to doubles or Cortex needs to handle floats.
    //            Let's change saxpy_x to vector<double> for Cortex compatibility for now.
    //            The JIT SAXPY part internally casts to float*, which is unsafe but part of current design.
    std::vector<double> saxpy_x_double(saxpy_x.begin(), saxpy_x.end());


    VPU::VPU_Task task4 = {"SAXPY", saxpy_x_double.data(), nullptr, saxpy_y_result.data(), saxpy_x_double.size()};

    std::cout << "Initial saxpy_y_result[0]: " << saxpy_y_result[0] << std::endl;
    vpu.execute(task4);
    std::cout << "Modified saxpy_y_result[0] after VPU execute: " << saxpy_y_result[0] << std::endl;

    std::cout << "\n\n>>>>> VPU BELIEFS AFTER JOB 4 <<<<<";
    vpu.print_beliefs();

    // ANALYSIS OF JOB 4:
    // - Cortex will profile saxpy_x_double. Its amplitude/frequency flux might be used by SAXPY's generic sensitivity.
    //   The JIT engine in Pillar4 receives VPU_Task and casts data_in_a to float* to analyze sparsity of x.
    //   Sparsity of saxpy_x is 0.5. Current JIT in Pillar4 uses >0.5 for sparse kernel, so it should pick DENSE JIT path.
    // - Orchestrator will choose between "Standard SAXPY" and "JIT Compiled SAXPY".
    //   Costs: SAXPY_STANDARD (base 20000),
    //          JIT path: TRANSFORM_JIT_COMPILE_SAXPY (transform 75000) + EXECUTE_JIT_SAXPY (base 5000).
    //   Dynamic costs also apply (lambda_SAXPY_generic for both, EXECUTE_JIT_SAXPY's is halved).
    //   For small N (like 10 here), JIT overhead (75000) is very high, so Standard SAXPY should be chosen.
    // - Observe which path is chosen and how beliefs related to SAXPY are updated.
    // - If JIT DENSE path were chosen: y_result[0] would be input y[0] (10.0) + fixed_a (1.0) * x[0] (1.0) + 2.0 = 13.0.
    // - If JIT SPARSE path were chosen: y_result[0] would be input y[0] (10.0) + fixed_a (1.0) * x[0] (1.0) + 1.0 = 12.0.
    // - If Standard SAXPY path chosen: The HAL stub for SAXPY_STANDARD in VPUCore calls HAL::cpu_saxpy with its own dummy data and a=1.0f.
    //   The HAL::cpu_saxpy itself would modify its y parameter. But this is a dummy vector local to the lambda.
    //   So, saxpy_y_result passed to task4 will NOT be modified by the SAXPY_STANDARD path due to current stubbing.

    std::cout << "\n\n===== VPU EXECUTION AND LEARNING CYCLE COMPLETE =====\n" << std::endl;


    // --- Test Pillar 3 LLM Path Generation Toggle ---
    std::cout << "\n\n===== TESTING PILLAR 3 LLM PATH TOGGLE =====\n" << std::endl;
    VPU::VPU_Environment vpu_for_p3_test;
    VPU::VPUCore* core_p3 = vpu_for_p3_test.get_core_for_testing();
    VPU::Orchestrator* orchestrator_p3 = core_p3->get_orchestrator_for_testing();

    if (orchestrator_p3) {
        std::cout << "\n--- Test Case: Pillar 3 LLM Path Generation ENABLED ---\n" << std::endl;
        orchestrator_p3->set_llm_path_generation(true);
        VPU::VPU_Task p3_test_task = {"CONVOLUTION", spiky_signal.data(), nullptr, nullptr, spiky_signal.size()};
        vpu_for_p3_test.execute(p3_test_task);
        std::cout << "VERIFICATION: Check logs for:\n"
                  << "1. '[Pillar 3] Orchestrator: Using LLM for path generation.'\n"
                  << "2. '[Pillar 3] Orchestrator: LLM path generation called with context for task type: CONVOLUTION'\n"
                  << "3. '[Pillar 3] Orchestrator: LLM returned no paths, falling back to traditional method.'\n"
                  << "4. A valid plan (e.g., 'Time Domain (Direct)' or 'Frequency Domain (FFT)') was chosen and executed.\n" << std::endl;

        std::cout << "\n--- Test Case: Pillar 3 LLM Path Generation DISABLED ---\n" << std::endl;
        orchestrator_p3->set_llm_path_generation(false);
        vpu_for_p3_test.execute(p3_test_task); // Execute same task
        std::cout << "VERIFICATION: Check logs to ensure NO LLM path messages appear, and a traditional plan is chosen.\n" << std::endl;
    } else {
        std::cerr << "TEST ERROR: Could not get Orchestrator for Pillar 3 LLM toggle test." << std::endl;
    }

    // TODO: Add other tests here

    // --- Test Pillar 4 LLM JIT Generation Toggle ---
    std::cout << "\n\n===== TESTING PILLAR 4 LLM JIT TOGGLE =====\n" << std::endl;
    VPU::VPU_Environment vpu_for_p4_test;
    VPU::VPUCore* core_p4 = vpu_for_p4_test.get_core_for_testing();
    VPU::Cerebellum* cerebellum_p4 = core_p4->get_cerebellum_for_testing();

    if (cerebellum_p4) {
        VPU::FluxJITEngine* jit_engine_p4 = cerebellum_p4->get_jit_engine_for_testing();
        if (jit_engine_p4) {
            // Prepare SAXPY task data (using existing data from Job 4)
            std::vector<double> p4_saxpy_x_double(saxpy_x.begin(), saxpy_x.end()); // saxpy_x defined in main
            std::vector<float> p4_saxpy_y_result = saxpy_y_orig; // saxpy_y_orig defined in main
            VPU::VPU_Task p4_saxpy_task = {"SAXPY", p4_saxpy_x_double.data(), nullptr, p4_saxpy_y_result.data(), p4_saxpy_x_double.size()};

            std::cout << "\n--- Test Case: Pillar 4 LLM JIT Generation ENABLED ---\n" << std::endl;
            jit_engine_p4->set_llm_jit_generation(true);
            vpu_for_p4_test.execute(p4_saxpy_task);
            std::cout << "VERIFICATION: Check logs for:\n"
                      << "1. '[JIT Engine] Attempting LLM-based JIT generation...'\n"
                      << "2. '[JIT Engine] LLM JIT kernel generation called for task: SAXPY'\n"
                      << "3. '[JIT Engine] LLM JIT generation failed or not applicable, falling back to traditional JIT.'\n"
                      << "4. A traditional JIT path (e.g., 'Data is dense. Providing 'DENSE SAXPY' logic.') was taken.\n" << std::endl;

            // Reset y_result for next run if necessary, though current stubs might not modify it meaningfully
            p4_saxpy_y_result = saxpy_y_orig;

            std::cout << "\n--- Test Case: Pillar 4 LLM JIT Generation DISABLED ---\n" << std::endl;
            jit_engine_p4->set_llm_jit_generation(false);
            vpu_for_p4_test.execute(p4_saxpy_task);
            std::cout << "VERIFICATION: Check logs to ensure NO LLM JIT messages appear, and a traditional JIT path is taken.\n" << std::endl;

        } else {
            std::cerr << "TEST ERROR: Could not get FluxJITEngine for Pillar 4 LLM JIT toggle test." << std::endl;
        }
    } else {
        std::cerr << "TEST ERROR: Could not get Cerebellum for Pillar 4 LLM JIT toggle test." << std::endl;
    }

    // --- Test Pillar 2 & 3 IoT Data Influence ---
    std::cout << "\n\n===== TESTING PILLAR 2 & 3 IoT DATA INFLUENCE =====\n" << std::endl;
    VPU::VPU_Environment vpu_for_iot_test;
    VPU::VPUCore* core_iot = vpu_for_iot_test.get_core_for_testing();
    // VPUCore does not currently have a public getter for Cortex. Add one if needed, or make Cortex methods public.
    // For now, we assume Cortex applies default dummy values, and we'll verify Pillar3's reaction.
    // To properly test override, we'd need: VPU::Cortex* cortex_iot = core_iot->get_cortex_for_testing();

    std::cout << "\n--- Test Case: IoT Influence - Default Values ---\n" << std::endl;
    // Using spiky_signal and CONVOLUTION task type from earlier in main
    VPU::VPU_Task iot_test_task_default = {"CONVOLUTION", spiky_signal.data(), nullptr, nullptr, spiky_signal.size()};
    vpu_for_iot_test.execute(iot_test_task_default);
    std::cout << "VERIFICATION: Check Pillar 3 logs for simulate_flux_cost. It should show IoT values like:\n"
              << "  Power=" << 75.5 << "W, Temp=" << 65.2 << "C, etc. (default values from Pillar2_Cortex.cpp)\n"
              << "  And the 'Adjustments:' log part should show minimal or no penalties from these defaults.\n" << std::endl;

    // To test specific IoT values, we would need to call the new set_next_iot_profile_override method.
    // This requires getting Cortex from VPUCore. Let's assume get_cortex_for_testing() is added to VPUCore.
    VPU::Cortex* cortex_iot = nullptr;
    if (core_iot) { // core_iot is VPUCore*
         cortex_iot = core_iot->get_cortex_for_testing();
    }
    // The following tests are conceptual until get_cortex_for_testing() is available.
    // For now, they serve as placeholders for what would be tested.
    // To make this runnable without adding get_cortex_for_testing now, these specific tests will be commented out.

    // Un-commenting the tests now that the getter should be available.
    if (cortex_iot) {
        std::cout << "\n--- Test Case: IoT Influence - High Temperature Override ---\n" << std::endl;
        VPU::DataProfile high_temp_profile; // Default values are 0 or 1.0
        high_temp_profile.temperature_celsius = 90.0;
        // Keep other IoT values default or set them explicitly if desired
        high_temp_profile.power_draw_watts = 75.5; // Default-like
        high_temp_profile.network_latency_ms = 15.3; // Default-like
        high_temp_profile.network_bandwidth_mbps = 980.0; // Default-like
        high_temp_profile.io_throughput_mbps = 250.0; // Default-like
        high_temp_profile.data_quality_score = 0.95; // Default-like

        cortex_iot->set_next_iot_profile_override(high_temp_profile);
        VPU::VPU_Task iot_test_task_high_temp = {"CONVOLUTION", spiky_signal.data(), nullptr, nullptr, spiky_signal.size()};
        vpu_for_iot_test.execute(iot_test_task_high_temp);
        std::cout << "VERIFICATION: Check Pillar 3 logs for simulate_flux_cost. It should show Temp=" << 90.0 << "C.\n"
                  << "  The 'Adjustments:' log should include 'TempHigh(90.000000C * 1.5)'.\n" << std::endl;

        std::cout << "\n--- Test Case: IoT Influence - Low Data Quality Override ---\n" << std::endl;
        VPU::DataProfile low_quality_profile;
        low_quality_profile.data_quality_score = 0.5;
        // Keep other IoT values default or set them explicitly
        low_quality_profile.temperature_celsius = 65.2; // Default-like
        low_quality_profile.power_draw_watts = 75.5;    // Default-like
        low_quality_profile.network_latency_ms = 15.3;  // Default-like
        low_quality_profile.network_bandwidth_mbps = 980.0; // Default-like
        low_quality_profile.io_throughput_mbps = 250.0; // Default-like


        cortex_iot->set_next_iot_profile_override(low_quality_profile);
        VPU::VPU_Task iot_test_task_low_quality = {"CONVOLUTION", spiky_signal.data(), nullptr, nullptr, spiky_signal.size()};
        vpu_for_iot_test.execute(iot_test_task_low_quality);
        std::cout << "VERIFICATION: Check Pillar 3 logs for simulate_flux_cost. It should show DataQuality=" << 0.5 << " score.\n"
                  << "  The 'Adjustments:' log should include 'DataQuality(0.500000 score / ...)'.\n" << std::endl;
    } else {
        std::cerr << "TEST ERROR: Could not get Cortex for IoT override tests." << std::endl;
    }
    // For now, the first test (default values) is the only one that will effectively run for IoT.
    // The VERIFICATION lines for conceptual tests will still print.


    // --- Test Pillar 5 Proactive Experimentation ---
    std::cout << "\n\n===== TESTING PILLAR 5 PROACTIVE EXPERIMENTATION =====\n" << std::endl;
    VPU::VPU_Environment vpu_for_p5_test;
    VPU::VPUCore* core_p5 = vpu_for_p5_test.get_core_for_testing();
    VPU::FeedbackLoop* feedback_loop_p5 = core_p5->get_feedback_loop_for_testing();

    if (feedback_loop_p5) {
        // Using spiky_signal and CONVOLUTION task type from earlier in main
        // CONVOLUTION has at least two paths: "Time Domain (Direct)" and "Frequency Domain (FFT)"
        VPU::VPU_Task p5_test_task = {"CONVOLUTION", spiky_signal.data(), nullptr, nullptr, spiky_signal.size()};

        std::cout << "\n--- Test Case: Pillar 5 Force Exploration (Rate = 1.0) ---\n" << std::endl;
        feedback_loop_p5->force_exploration_rate_for_testing(1.0); // Always explore
        vpu_for_p5_test.execute(p5_test_task);
        std::cout << "VERIFICATION: Check logs for:\n"
                  << "1. '[Pillar 5] FeedbackLoop: Decision to EXPLORE...'\n"
                  << "2. '[VPUCore] EXPLORATION: Chose suboptimal plan...'\n"
                  << "3. LearningContext path name in Pillar 5 logs includes '(Exploratory)'.\n" << std::endl;

        // Reset exploration rate for next test (or re-init VPU_Environment)
        // feedback_loop_p5->force_exploration_rate_for_testing(0.1); // Reset to default or previous
        // For simplicity, let's re-init, though it's heavier
        VPU::VPU_Environment vpu_for_p5_test_no_explore;
        core_p5 = vpu_for_p5_test_no_explore.get_core_for_testing(); // Re-assign core_p5
        feedback_loop_p5 = core_p5->get_feedback_loop_for_testing(); // Re-assign feedback_loop_p5

        if (feedback_loop_p5) {
             std::cout << "\n--- Test Case: Pillar 5 Force NO Exploration (Rate = 0.0) ---\n" << std::endl;
            feedback_loop_p5->force_exploration_rate_for_testing(0.0); // Never explore
            vpu_for_p5_test_no_explore.execute(p5_test_task);
            std::cout << "VERIFICATION: Check logs for:\n"
                      << "1. NO '[Pillar 5] FeedbackLoop: Decision to EXPLORE...' message.\n"
                      << "2. NO '[VPUCore] EXPLORATION: Chose suboptimal plan...' message.\n"
                      << "3. '[VPUCore] Chose optimal plan...' message appears.\n"
                      << "4. LearningContext path name in Pillar 5 logs DOES NOT include '(Exploratory)'.\n" << std::endl;
        } else {
             std::cerr << "TEST ERROR: Could not get FeedbackLoop for Pillar 5 NO exploration test after re-init." << std::endl;
        }

    } else {
        std::cerr << "TEST ERROR: Could not get FeedbackLoop for Pillar 5 exploration tests." << std::endl;
    }

    // --- Test Pillar 6 Task Graph Orchestration (Conceptual Fusion) ---
    // This test requires careful setup to ensure a pattern is recorded multiple times.
    // For simplicity, we'll directly use Pillar 6 methods if VPUCore execution is too complex to guarantee.
    std::cout << "\n\n===== TESTING PILLAR 6 TASK GRAPH ORCHESTRATION =====\n" << std::endl;
    VPU::VPU_Environment vpu_for_p6_test;
    VPU::VPUCore* core_p6 = vpu_for_p6_test.get_core_for_testing();
    VPU::TaskGraphOrchestrator* tgo_p6 = core_p6->get_task_graph_orchestrator_for_testing();
    VPU::HardwareProfile* hw_profile_p6 = core_p6->get_hardware_profile_for_testing();
    VPU::HAL::KernelLibrary* kernel_lib_p6 = core_p6->get_kernel_library_for_testing();

    if (tgo_p6 && hw_profile_p6 && kernel_lib_p6) {
        // Manually construct plans to simulate history, as full task execution to get specific
        // multi-step plans repeatedly is complex for this test.
        // Assume fusion_candidate_threshold_ is 3 (default in new code is 10, analysis_interval_ is 5)
        // We can call analyze_and_fuse_patterns directly for testing.
        // Or, set fusion_candidate_threshold_ to a low value for the test instance.
        // Let's try to use record_executed_plan and trigger analysis.
        // The default analysis_interval is 5, threshold is 10. Let's make threshold 2, interval 3 for test.
        // This would require modifying TaskGraphOrchestrator constructor or adding a test helper.
        // For now, let's assume we can call analyze_and_fuse_patterns directly.

        VPU::ExecutionPlan plan1 = {"TestPlanWithFusionTarget", 0.0, {
            {"GEMM_NAIVE", "in_a", "tmp1"},
            {"SAXPY_STANDARD", "tmp1", "out1"} // Potential ADDBIAS could be SAXPY if it adds to existing buffer
        }};
        VPU::ExecutionPlan plan2 = {"AnotherPlan", 0.0, { {"CONV_DIRECT", "in", "out"} }};
        VPU::ExecutionPlan plan3 = {"TestPlanWithFusionTarget2", 0.0, { // Slightly different plan name
            {"GEMM_NAIVE", "in_b", "tmp2"},
            {"SAXPY_STANDARD", "tmp2", "out2"}
        }};
         VPU::ExecutionPlan plan4 = {"TestPlanWithFusionTarget3", 0.0, {
            {"GEMM_NAIVE", "in_c", "tmp3"},
            {"SAXPY_STANDARD", "tmp3", "out3"}
        }};


        std::cout << "\n--- Test Case: Pillar 6 Fusion ---" << std::endl;

        // Configure Pillar 6 for this test
        tgo_p6->set_fusion_candidate_threshold_for_testing(2); // Fuse if pattern appears >= 2 times
        tgo_p6->set_analysis_interval_for_testing(3);      // Analyze every 3 recorded plans
        tgo_p6->reset_task_execution_counter_for_testing(); // Ensure clean state for this test

        // Record plans:
        // Plan 1: GEMM_NAIVE -> SAXPY_STANDARD
        // Plan 2: CONV_DIRECT
        // Plan 3: GEMM_NAIVE -> SAXPY_STANDARD (Analysis should trigger here)
        // Plan 4: GEMM_NAIVE -> SAXPY_STANDARD (Pattern count becomes 3, already fused or re-verified)

        std::cout << "Recording plan1 (GEMM_NAIVE -> SAXPY_STANDARD)" << std::endl;
        tgo_p6->record_executed_plan(plan1); // task_execution_counter_ = 1. Sequence count = 1.
        std::cout << "Recording plan2 (CONV_DIRECT)" << std::endl;
        tgo_p6->record_executed_plan(plan2); // task_execution_counter_ = 2.
        std::cout << "Recording plan3 (GEMM_NAIVE -> SAXPY_STANDARD)" << std::endl;
        tgo_p6->record_executed_plan(plan3); // task_execution_counter_ = 3. Analysis IS triggered. Sequence count = 2. Fusion should occur.

        std::cout << "Recording plan4 (GEMM_NAIVE -> SAXPY_STANDARD) - to check re-fusion logic or if analysis is periodic" << std::endl;
        tgo_p6->record_executed_plan(plan1); // task_execution_counter_ = 4. Sequence count = 3.
                                             // If analysis was only on %3==0, it won't run again here.
                                             // If it runs, it should see kernel already exists.

        // The analyze_and_fuse_patterns() should have been called automatically by record_executed_plan on the 3rd call.
        // If we need a second analysis run (e.g. to test "already fused" logic after more plans):
        // tgo_p6->record_executed_plan(plan_another_trigger_analysis_1);
        // tgo_p6->record_executed_plan(plan_another_trigger_analysis_2); // This would be the 6th plan total.

        std::string fused_name = "FUSED_GEMM_NAIVE_SAXPY_STANDARD";
        std::cout << "VERIFICATION: Check logs for:\n"
                << "1. Sequence <GEMM_NAIVE, SAXPY_STANDARD> appeared 2 times (when fusion first triggered).\n"
                << "2. Attempting fusion for <GEMM_NAIVE, SAXPY_STANDARD> (first time).\n"
                << "3. Conceptually added new fused kernel '" << fused_name << "' to KernelLibrary (first time).\n"
                << "4. Added estimated cost for '" << fused_name << "' to HardwareProfile (first time).\n"
                << "5. On subsequent analysis (if any), log for 'Fused kernel ... already exists' for this pair.\n"<< std::endl;

        // Verify directly (if possible)
        if (kernel_lib_p6->count(fused_name)) {
            std::cout << "VERIFIED: Fused kernel '" << fused_name << "' exists in KernelLibrary." << std::endl;
        } else {
            std::cerr << "VERIFICATION FAILED: Fused kernel '" << fused_name << "' NOT FOUND in KernelLibrary." << std::endl;
        }
        if (hw_profile_p6->base_operational_costs.count(fused_name)) {
            std::cout << "VERIFIED: Fused kernel '" << fused_name << "' has an entry in HardwareProfile base_operational_costs." << std::endl;
        } else {
            std::cerr << "VERIFICATION FAILED: Fused kernel '" << fused_name << "' NOT FOUND in HardwareProfile base_operational_costs." << std::endl;
        }

    } else {
        std::cerr << "TEST ERROR: Could not get TaskGraphOrchestrator or its dependencies for Pillar 6 tests." << std::endl;
    }


    return 0;
}
