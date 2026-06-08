#pragma once

#include <chrono>
#include <deque>
#include <cmath>
#include <array>

namespace tpt::core {

/**
 * @brief Temporal aliasing correction
 * 
 * Problem: There's always a delay between when game generates frame
 * and when we process it. This causes us to "lag behind" reality.
 * 
 * Solution: Model the latency and work backwards.
 * 
 * Example:
 * - Frame captured at T=0
 * - Vision processed at T=8ms
 * - Prediction at T=9ms
 * - Press at T=10ms
 * - Console receives at T=15ms (total latency: 15ms)
 * 
 * But the meter has been moving the whole time!
 * If we predict at T=9ms where it WILL be at T=15ms, we're correct.
 */
class TemporalEstimator {
public:
    struct LatencyBudget {
        double capture_to_vision_ms;     // Typical: 3-5ms
        double vision_to_prediction_ms;  // Typical: 1-2ms
        double prediction_to_input_ms;   // Typical: 1ms
        double input_to_console_ms;      // Typical: 5-10ms
        
        double total_ms() const {
            return capture_to_vision_ms + vision_to_prediction_ms + 
                   prediction_to_input_ms + input_to_console_ms;
        }
    };
    
    explicit TemporalEstimator(const LatencyBudget& budget);
    
    /**
     * @brief Log timing measurement
     * 
     * Records actual timing of each stage to auto-calibrate.
     */
    void log_timing(
        uint64_t capture_time_ns,
        uint64_t vision_time_ns,
        uint64_t prediction_time_ns,
        uint64_t input_time_ns
    );
    
    /**
     * @brief Get predicted "current" reality time
     * 
     * Accounts for all latency. When you call this at prediction time,
     * it returns what time the console will receive the input.
     */
    uint64_t get_reality_time_at_console_ns() const;
    
    /**
     * @brief Estimate where meter will be when input arrives
     * 
     * Given current position and velocity, predict future position.
     */
    double predict_meter_at_console_time(
        double current_position,
        double velocity_per_sec
    ) const;
    
private:
    LatencyBudget budget_;
    
    // Rolling average of actual latencies
    std::deque<double> capture_to_vision_history_;
    std::deque<double> total_latency_history_;
    
    static constexpr int HISTORY_WINDOW = 100;
    
    double get_average_latency() const;
};

} // namespace tpt::core