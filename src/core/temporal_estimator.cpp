#include "temporal_estimator.h"
#include <iostream>
#include <numeric>
#include <algorithm>

namespace tpt::core {

TemporalEstimator::TemporalEstimator(const LatencyBudget& budget)
    : budget_(budget) {}

void TemporalEstimator::log_timing(
    uint64_t capture_time_ns,
    uint64_t vision_time_ns,
    uint64_t prediction_time_ns,
    uint64_t input_time_ns) {
    
    // Calculate actual latencies
    double capture_to_vision_ms = (vision_time_ns - capture_time_ns) / 1e6;
    double total_latency_ms = (input_time_ns - capture_time_ns) / 1e6;
    
    capture_to_vision_history_.push_back(capture_to_vision_ms);
    total_latency_history_.push_back(total_latency_ms);
    
    // Keep history bounded
    if (capture_to_vision_history_.size() > HISTORY_WINDOW) {
        capture_to_vision_history_.pop_front();
        total_latency_history_.pop_front();
    }
}

uint64_t TemporalEstimator::get_reality_time_at_console_ns() const {
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t current_time_ns = now.time_since_epoch().count();
    
    // Add average measured latency
    double avg_latency_ms = get_average_latency();
    uint64_t latency_ns = (uint64_t)(avg_latency_ms * 1e6);
    
    return current_time_ns + latency_ns;
}

double TemporalEstimator::predict_meter_at_console_time(
    double current_position,
    double velocity_per_sec) const {
    
    // How long until input reaches console?
    double avg_latency_ms = get_average_latency();
    double avg_latency_sec = avg_latency_ms / 1000.0;
    
    // Distance meter will travel
    double distance = velocity_per_sec * avg_latency_sec;
    
    // Future position
    return current_position + distance;
}

double TemporalEstimator::get_average_latency() const {
    if (total_latency_history_.empty()) {
        return budget_.total_ms();
    }
    
    double sum = 0.0;
    for (double latency : total_latency_history_) {
        sum += latency;
    }
    
    return sum / total_latency_history_.size();
}

} // namespace tpt::core