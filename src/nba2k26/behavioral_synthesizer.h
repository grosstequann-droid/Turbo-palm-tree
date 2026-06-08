#pragma once

#include <random>
#include <cmath>
#include <array>
#include <deque>

namespace tpt::nba2k26 {

/**
 * @brief Human-like behavioral jitter synthesis
 * 
 * Uses Ornstein-Uhlenbeck stochastic process to generate
 * temporal correlation in randomness (just like humans).
 * 
 * Key: Real humans don't have uniformly random timing.
 * Their variance is autocorrelated (if they're early one time,
 * they tend to be early next time too, until they "reset").
 */
class BehavioralSynthesizer {
public:
    struct HumanModel {
        double timing_jitter_ms;           // ±5-10ms variance
        double hold_duration_mean_ms;      // 50-100ms
        double hold_duration_std_ms;       // 10-20ms
        double consecutive_perfect_rate;   // Fatigue factor
        double miss_probability;           // Miss ~15-25% of open shots
    };
    
    BehavioralSynthesizer();
    
    void set_human_model(const HumanModel& model) {
        human_model_ = model;
    }
    
    /**
     * @brief Get press timing jitter (autocorrelated)
     * Uses Ornstein-Uhlenbeck process for realistic variance
     */
    double get_timing_jitter_ms();
    
    /**
     * @brief Get realistic button hold duration
     */
    double get_hold_duration_ms();
    
    /**
     * @brief Should this shot be intentionally missed?
     * Tracks streaks to avoid perfect game detection
     */
    bool should_miss_intentionally();
    
    /**
     * @brief Log shot result for pattern analysis
     */
    void log_attempt_result(bool was_perfect);
    
    /**
     * @brief Get suspicion score (0.0=human, 1.0=bot)
     */
    double get_suspicion_score() const { return suspicion_score_; }
    
private:
    HumanModel human_model_;
    
    // Ornstein-Uhlenbeck process state
    // Generates temporally correlated noise
    double ou_state_;
    double ou_theta_;    // Mean reversion speed (~0.1-0.2)
    double ou_sigma_;    // Volatility (~0.5-1.0)
    
    // Recent results tracking
    std::deque<bool> recent_results_;
    static constexpr int RECENT_WINDOW = 50;
    
    int perfect_streak_;
    double suspicion_score_;
    
    std::mt19937 rng_;
    
    double ornstein_uhlenbeck_step();
};

} // namespace tpt::nba2k26