#include "system.h"
#include <iostream>

namespace tpt::app {

System::System()
    : capture_(std::make_unique<pipeline::CaptureThread>()),
      vision_(std::make_unique<pipeline::VisionThread>(&capture_->get_buffer())),
      prediction_(std::make_unique<pipeline::PredictionThread>(vision_.get())),
      io_(std::make_unique<pipeline::IOThread>(prediction_.get())) {
    std::cout << "[System] Constructor called\n";
}

System::~System() {
    shutdown();
    std::cout << "[System] Destructor called\n";
}

void System::initialize() {
    std::cout << "\n[System] Starting pipeline threads...\n";
    std::cout << "  - Thread 1: Capture (Core 0, RT priority)\n";
    std::cout << "  - Thread 2: Vision (Core 1, RT priority)\n";
    std::cout << "  - Thread 3: Prediction (Core 2, RT priority)\n";
    std::cout << "  - Thread 4: I/O (Core 3, RT priority)\n\n";

    capture_->start();
    vision_->start();
    prediction_->start();
    io_->start();

    std::cout << "[System] Pipeline initialized and running\n";
}

void System::shutdown() {
    std::cout << "\n[System] Stopping pipeline threads...\n";

    // Stop in reverse order (I/O → Prediction → Vision → Capture)
    io_->stop();
    prediction_->stop();
    vision_->stop();
    capture_->stop();

    std::cout << "[System] All threads stopped\n";
}

void System::print_metrics() const {
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "  System Metrics\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";

    std::cout << "\nCapture Thread:\n";
    std::cout << "  Frames captured: " << capture_->get_frames_captured() << "\n";
    std::cout << "  Avg latency: " << capture_->get_avg_capture_latency_us() << " µs\n";

    std::cout << "\nVision Thread:\n";
    std::cout << "  Frames processed: " << vision_->get_frames_processed() << "\n";
    std::cout << "  Avg latency: " << vision_->get_avg_vision_latency_us() << " µs\n";

    std::cout << "\nI/O Thread:\n";
    std::cout << "  Inputs sent: " << io_->get_inputs_sent() << "\n";
    std::cout << "  Avg jitter: " << io_->get_avg_input_jitter_us() << " µs\n";

    std::cout << "\n═══════════════════════════════════════════════════════════\n";
}

} // namespace tpt::app
