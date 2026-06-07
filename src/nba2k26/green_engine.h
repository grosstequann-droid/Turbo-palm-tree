#pragma once

#include "vision/meter_detector.h"
#include "control/greens_predictor.h"
#include "nba2k26/calibrator.h"
#include "nba2k26/anti_cheat.h"
#include "core/worker_thread.h"
#include <memory>
#include <atomic>

namespace tpt::nba2k26 {

/**
 * @brief NBA 2K26 guaranteed green automation engine
 * 
 * Complete end-to-end system for automated perfect releases.
 * 
 * Features:
 * - Automatic calibration (learn green zone from 10 shots)
 * - Real-time meter detection and prediction
 * - Nanosecond-precision button timing
 * - Anti-cheat evasion (looks like human player)
 * - Performance monitoring and stats
 */
class GreenEngine {
public:
    enum class State {
        IDLE,
        CALIBRATING,
        CALIBRATED,
        RUNNING,
        PAUSED,
        ERROR
    };
    
    struct Config {
        bool auto_calibrate;           // Run calibration on startup
        bool anti_cheat_enabled;       // Enable evasion layer
        double target_perfect_rate;    // 0.80 = 80% perfect (human-like)
        double system_latency_ms;      // Measured console latency
        bool verbose_logging;          // Print debug info
    };
    
    struct Stats {
        uint64_t total_shots;
        uint64_t perfect_shots;
        uint64_t good_shots;
        uint64_t missed_shots;
        double perfect_percentage;
        double avg_timing_error_ms;
        double green_accuracy;
    };
    
    GreenEngine();
    ~GreenEngine();
    
    /**
     * @brief Initialize the engine
     * @param config Configuration parameters
     */
    void initialize(const Config& config);
    
    /**
     * @brief Start automation
     */
    void start();
    
    /**
     * @brief Stop automation
     */
    void stop();
    
    /**
     * @brief Pause without stopping threads
     */
    void pause();
    
    /**
     * @brief Resume from pause
     */
    void resume();
    
    /**
     * @brief Run calibration phase
     * Records 10 shots and determines green zone parameters
     */
    void run_calibration();
    
    /**
     * @brief Process frame during gameplay
     * Called from vision thread with each new frame
     */
    void process_frame(const cv::Mat& frame);
    
    /**
     * @brief Execute button press for a shot
     */
    void execute_shot();
    
    /**
     * @brief Get current state
     */
    State get_state() const { return state_; }
    
    /**
     * @brief Get performance statistics
     */
    Stats get_stats() const { return stats_; }
    
    /**
     * @brief Get current calibration
     */
    NBA2K26Calibrator::CalibrationResult get_calibration() const {
        return calibrator_.get_current_calibration();
    }
    
    /**
     * @brief Save calibration to file
     */
    void save_calibration(const std::string& filename) {
        calibrator_.save_to_file(filename);
    }
    
    /**
     * @brief Load calibration from file
     */
    void load_calibration(const std::string& filename) {
        calibrator_.load_from_file(filename);
        state_ = State::CALIBRATED;
    }
    
private:
    // Configuration
    Config config_;
    State state_;
    
    // Core components
    vision::RobustMeterDetector meter_detector_;
    control::GuaranteedGreenPredictor predictor_;
    NBA2K26Calibrator calibrator_;
    AntiCheatLayer anti_cheat_;
    
    // Worker thread
    std::unique_ptr<core::WorkerThread> processing_thread_;
    
    // Statistics
    Stats stats_;
    
    // Synchronization
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    
    // Processing loop
    void processing_loop();
    void update_stats(bool was_perfect);
};

} // namespace tpt::nba2k26
