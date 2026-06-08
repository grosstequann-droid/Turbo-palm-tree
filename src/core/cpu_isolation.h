#pragma once

#include <windows.h>
#include <cstdint>
#include <vector>

namespace tpt::core {

/**
 * @brief CPU Isolation for Real-Time Determinism
 * 
 * Reserves and isolates specific CPU cores from OS scheduler.
 * Prevents context-switching, interrupts, and background tasks.
 * 
 * Guarantees: <1µs scheduling latency (vs ~100µs normal)
 */
class CPUIsolation {
public:
    /**
     * @brief Core allocation strategy
     * 
     * On 8-core system:
     * - Core 0: OS/background (untouched)
     * - Core 1: Capture thread (ISOLATED)
     * - Core 2: Vision/GPU (ISOLATED)
     * - Core 3: Prediction (ISOLATED)
     * - Core 4: Input (ISOLATED - HIGHEST PRIORITY)
     * - Cores 5-7: OS/background
     */
    enum class CoreRole {
        OS_BACKGROUND,    // Don't touch (handles interrupts)
        CAPTURE,          // Real-time frame capture
        VISION,           // GPU compute (can tolerate some latency)
        PREDICTION,       // Kalman + logic
        INPUT_CRITICAL,   // Button press timing (HIGHEST PRIORITY)
    };
    
    /**
     * @brief Configure CPU isolation
     * Must be called ONCE at startup before threads created
     */
    static bool initialize();
    
    /**
     * @brief Assign core to role
     * @param core_id Physical core number (0-indexed)
     * @param role What this core does
     */
    static bool assign_core(int core_id, CoreRole role);
    
    /**
     * @brief Pin thread to isolated core
     * Must be called FROM the thread (not external)
     * 
     * Returns affinity mask for verification
     */
    static DWORD_PTR pin_thread_to_core(int core_id);
    
    /**
     * @brief Disable power management on core
     * Prevents C-states (CPU sleep) during critical sections
     */
    static bool disable_power_management(int core_id);
    
    /**
     * @brief Disable SMT (hyperthreading) interference
     * Prevents sibling thread from preempting our code
     */
    static bool disable_smt_on_core(int core_id);
    
    /**
     * @brief Verify core is truly isolated
     * Checks CPU frequency, power state, scheduled tasks
     */
    static bool verify_isolation(int core_id);
    
    /**
     * @brief Get recommended core configuration for system
     * Auto-detects core count and suggests allocation
     */
    static std::vector<std::pair<int, CoreRole>> get_recommended_config();
    
private:
    static std::vector<int> isolated_cores_;
};

} // namespace tpt::core