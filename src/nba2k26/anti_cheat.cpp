#include "anti_cheat.h"
#include <iostream>
#include <algorithm>

namespace tpt::nba2k26 {

AntiCheatLayer::AntiCheatLayer()
    : target_profile_{0.80, 4.0, 2.0, false},
      current_stats_{0.0, 0.0, 0.0, false},
      rng_(std::random_device{}()),
      evasion_confidence_(0.5) {
    
    timing_noise_ = std::normal_distribution<double>(0.0, 3.0);  // 3ms sigma
}

bool AntiCheatLayer::should_intentionally_miss() {
    // Calculate how many perfects we've had recently
    int perfect_count = 0;
    for (bool is_perfect : recent_perfect_shots_) {
        if (is_perfect) perfect_count++;
    }
    
    double recent_perfect_rate = (double)perfect_count / std::max(1, (int)recent_perfect_shots_.size());
    
    // If we're above target, miss this one
    if (recent_perfect_rate > target_profile_.perfect_percentage) {
        std::cout << "[AntiCheat] Intentional miss (rate: " << (recent_perfect_rate * 100) 
                  << "% > target: " << (target_profile_.perfect_percentage * 100) << "%)\n";
        return true;
    }
    
    return false;
}

int64_t AntiCheatLayer::add_timing_variance(int64_t base_press_time_ns) {
    // Add human-like reaction variance
    double jitter_ms = timing_noise_(rng_);
    
    // Clamp to realistic range (±5ms max)
    jitter_ms = std::clamp(jitter_ms, -5.0, 5.0);
    
    int64_t jitter_ns = (int64_t)(jitter_ms * 1e6);
    
    return base_press_time_ns + jitter_ns;
}

void AntiCheatLayer::add_position_error(double& target_position) {
    // Add small positioning error (±1-3 pixels)
    std::normal_distribution<double> position_noise(0.0, 1.5);
    double error = position_noise(rng_);
    
    // Convert pixels to normalized position (assuming 400px meter)
    double normalized_error = error / 400.0;
    
    target_position += normalized_error;
    target_position = std::clamp(target_position, 0.0, 1.0);
}

void AntiCheatLayer::log_shot_result(bool was_perfect, double timing_error_ms) {
    recent_perfect_shots_.push_back(was_perfect);
    recent_timing_errors_.push_back(timing_error_ms);
    
    // Keep window size bounded
    if (recent_perfect_shots_.size() > WINDOW_SIZE) {
        recent_perfect_shots_.pop_front();
        recent_timing_errors_.pop_front();
    }
    
    // Update stats
    int perfect_count = 0;
    for (bool p : recent_perfect_shots_) {
        if (p) perfect_count++;
    }
    current_stats_.perfect_percentage = (double)perfect_count / recent_perfect_shots_.size();
    
    // Calculate timing variance
    double mean_error = 0.0;
    for (double err : recent_timing_errors_) {
        mean_error += err;
    }
    mean_error /= recent_timing_errors_.size();
    
    double variance = 0.0;
    for (double err : recent_timing_errors_) {
        variance += (err - mean_error) * (err - mean_error);
    }
    current_stats_.timing_variance_ms = std::sqrt(variance / recent_timing_errors_.size());
    
    // Check for suspicious patterns
    if (looks_suspiciously_perfect()) {
        std::cout << "[AntiCheat] SUSPICIOUS PATTERN DETECTED! Increasing evasion...\n";
        evasion_confidence_ *= 1.1;  // Increase evasion level
    } else {
        // Confidence increases if pattern looks natural
        evasion_confidence_ = std::min(1.0, evasion_confidence_ + 0.01);
    }
}

bool AntiCheatLayer::looks_suspiciously_perfect() {
    // Check for red flags that indicate bot behavior
    
    // Flag 1: Too many perfects in a row
    int perfect_streak = 0;
    int max_streak = 0;
    for (bool is_perfect : recent_perfect_shots_) {
        if (is_perfect) {
            perfect_streak++;
            max_streak = std::max(max_streak, perfect_streak);
        } else {
            perfect_streak = 0;
        }
    }
    
    if (max_streak > PERFECT_STREAK_THRESHOLD) {
        std::cout << "[AntiCheat] Flag: Too many perfects in streak (" << max_streak << ")\n";
        return true;
    }
    
    // Flag 2: Too consistent timing (no variance)
    if (current_stats_.timing_variance_ms < TIMING_CONSISTENCY_THRESHOLD) {
        std::cout << "[AntiCheat] Flag: Timing too consistent (" 
                  << current_stats_.timing_variance_ms << "ms variance)\n";
        return true;
    }
    
    // Flag 3: Perfect rate way above human level (>95%)
    if (current_stats_.perfect_percentage > 0.95) {
        std::cout << "[AntiCheat] Flag: Perfect rate too high (" 
                  << (current_stats_.perfect_percentage * 100) << "%)\n";
        return true;
    }
    
    return false;
}

void AntiCheatLayer::update_evasion_level() {
    // Continuously adjust evasion based on current pattern
    
    if (looks_suspiciously_perfect()) {
        evasion_confidence_ *= 0.9;  // Decrease confidence, increase evasion
    } else {
        evasion_confidence_ = std::min(1.0, evasion_confidence_ + 0.02);
    }
}

double AntiCheatLayer::calculate_timing_variance() {
    if (recent_timing_errors_.empty()) {
        return target_profile_.timing_variance_ms;
    }
    
    double sum = 0.0;
    for (double err : recent_timing_errors_) {
        sum += err;
    }
    return sum / recent_timing_errors_.size();
}

} // namespace tpt::nba2k26
