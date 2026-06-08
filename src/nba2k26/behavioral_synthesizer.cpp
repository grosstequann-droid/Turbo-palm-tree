#include "behavioral_synthesizer.h"
#include <iostream>
#include <algorithm>

namespace tpt::nba2k26 {

BehavioralSynthesizer::BehavioralSynthesizer()
    : human_model_{8.0, 75.0, 15.0, 0.9, 0.20},
      ou_state_(0.0),
      ou_theta_(0.15),
      ou_sigma_(0.8),
      perfect_streak_(0),
      suspicion_score_(0.0),
      rng_(std::random_device{}()) {}

double BehavioralSynthesizer::get_timing_jitter_ms() {
    // Ornstein-Uhlenbeck step for temporally correlated noise
    double jitter = ornstein_uhlenbeck_step();
    
    // Clamp to realistic range
    return std::clamp(jitter, -human_model_.timing_jitter_ms, human_model_.timing_jitter_ms);
}

double BehavioralSynthesizer::get_hold_duration_ms() {
    std::normal_distribution<double> hold_dist(
        human_model_.hold_duration_mean_ms,
        human_model_.hold_duration_std_ms
    );
    
    double duration = hold_dist(rng_);
    return std::clamp(duration, 30.0, 150.0);  // Realistic range
}

bool BehavioralSynthesizer::should_miss_intentionally() {
    // Humans miss more as streak gets longer (fatigue)
    double miss_chance = human_model_.miss_probability;
    
    if (perfect_streak_ > 15) {
        // Getting tiring, more likely to miss
        miss_chance *= (1.0 + (perfect_streak_ - 15) * 0.05);
    }
    
    std::uniform_real_distribution<double> rand_dist(0.0, 1.0);
    return rand_dist(rng_) < miss_chance;
}

void BehavioralSynthesizer::log_attempt_result(bool was_perfect) {
    recent_results_.push_back(was_perfect);
    
    if (recent_results_.size() > RECENT_WINDOW) {
        recent_results_.pop_front();
    }
    
    if (was_perfect) {
        perfect_streak_++;
    } else {
        perfect_streak_ = 0;
    }
    
    // Calculate suspicion score
    int perfect_count = 0;
    for (bool p : recent_results_) {
        if (p) perfect_count++;
    }
    
    double perfect_rate = (double)perfect_count / recent_results_.size();
    
    // Red flags
    double flags = 0.0;
    
    // Flag 1: Perfect rate too high (humans are 70-90%)
    if (perfect_rate > 0.95) {
        flags += 0.3;
    } else if (perfect_rate > 0.92) {
        flags += 0.15;
    }
    
    // Flag 2: Too long perfect streak (humans get fatigued)
    if (perfect_streak_ > 25) {
        flags += 0.3;
    } else if (perfect_streak_ > 18) {
        flags += 0.15;
    }
    
    // Blend with previous score (temporal filter)
    suspicion_score_ = suspicion_score_ * 0.7 + flags * 0.3;
    
    if (suspicion_score_ > 0.5) {
        std::cout << "[BehavioralSynthesizer] WARNING: Suspicion score rising (" 
                  << suspicion_score_ << "). Increase randomization.\n";
    }
}

double BehavioralSynthesizer::ornstein_uhlenbeck_step() {
    // Ornstein-Uhlenbeck: dX = theta * (0 - X) * dt + sigma * dW
    // Generates autocorrelated noise with mean reversion
    
    std::normal_distribution<double> white_noise(0.0, 1.0);
    double dW = white_noise(rng_);
    
    // Update state
    double dt = 0.016666;  // 60fps frame
    ou_state_ += ou_theta_ * (0.0 - ou_state_) * dt + ou_sigma_ * std::sqrt(dt) * dW;
    
    return ou_state_;
}

} // namespace tpt::nba2k26