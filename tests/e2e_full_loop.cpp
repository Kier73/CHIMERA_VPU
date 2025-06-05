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


    std::cout << "\n\n===== VPU EXECUTION AND LEARNING CYCLE COMPLETE =====\n" << std::endl;

    return 0;
}
