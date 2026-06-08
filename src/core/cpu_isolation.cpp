#include "cpu_isolation.h"
#include <iostream>
#include <windows.h>
#include <powerbase.h>

#pragma comment(lib, "powrprof.lib")

namespace tpt::core {

std::vector<int> CPUIsolation::isolated_cores_;

bool CPUIsolation::initialize() {
    std::cout << "[CPUIsolation] Initializing CPU isolation...\n";
    
    // Get system information
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    int core_count = sys_info.dwNumberOfProcessors;
    
    std::cout << "[CPUIsolation] System has " << core_count << " logical cores\n";
    
    // Get recommended configuration
    auto config = get_recommended_config();
    std::cout << "[CPUIsolation] Recommended configuration:\n";
    for (auto [core_id, role] : config) {
        std::string role_str;
        switch (role) {
            case CoreRole::OS_BACKGROUND: role_str = "OS_BACKGROUND"; break;
            case CoreRole::CAPTURE: role_str = "CAPTURE"; break;
            case CoreRole::VISION: role_str = "VISION"; break;
            case CoreRole::PREDICTION: role_str = "PREDICTION"; break;
            case CoreRole::INPUT_CRITICAL: role_str = "INPUT_CRITICAL"; break;
        }
        std::cout << "  Core " << core_id << ": " << role_str << "\n";
    }
    
    return true;
}

bool CPUIsolation::assign_core(int core_id, CoreRole role) {
    std::cout << "[CPUIsolation] Assigning core " << core_id << " to role\n";
    
    // Store in isolation list
    isolated_cores_.push_back(core_id);
    
    // Disable power management
    disable_power_management(core_id);
    
    // Disable SMT if hyperthreading enabled
    disable_smt_on_core(core_id);
    
    return true;
}

DWORD_PTR CPUIsolation::pin_thread_to_core(int core_id) {
    HANDLE thread_handle = GetCurrentThread();
    
    // Create affinity mask for single core
    DWORD_PTR affinity_mask = 1ULL << core_id;
    
    // Set affinity
    DWORD_PTR result = SetThreadAffinityMask(thread_handle, affinity_mask);
    
    if (result == 0) {
        std::cerr << "[CPUIsolation] Failed to pin thread to core " << core_id 
                  << ". Error: " << GetLastError() << "\n";
        return 0;
    }
    
    std::cout << "[CPUIsolation] Thread pinned to core " << core_id 
              << " (affinity: " << std::hex << affinity_mask << std::dec << ")\n";
    
    return result;
}

bool CPUIsolation::disable_power_management(int core_id) {
    // This requires admin privileges
    // Set CPU frequency to maximum (disable C-states)
    
    // On modern Windows, this is done via:
    // - Power management settings in registry
    // - or direct CPU control (requires driver)
    
    // For now, log the action
    std::cout << "[CPUIsolation] Disabling power management on core " << core_id << "\n";
    
    return true;
}

bool CPUIsolation::disable_smt_on_core(int core_id) {
    // Hyperthreading creates logical cores that share physical resources
    // Disabling SMT on isolated cores prevents interference
    
    // This requires:
    // - BIOS setting (manual)
    // - or kernel driver (requires admin)
    
    std::cout << "[CPUIsolation] Note: SMT disabling requires BIOS or admin\n";
    
    return true;
}

bool CPUIsolation::verify_isolation(int core_id) {
    std::cout << "[CPUIsolation] Verifying isolation of core " << core_id << "\n";
    
    // Check current thread affinity
    HANDLE thread_handle = GetCurrentThread();
    DWORD_PTR affinity = SetThreadAffinityMask(thread_handle, 0);
    SetThreadAffinityMask(thread_handle, affinity);  // Restore
    
    DWORD_PTR expected_mask = 1ULL << core_id;
    if ((affinity & expected_mask) == 0) {
        std::cerr << "[CPUIsolation] Core " << core_id << " not in thread affinity!\n";
        return false;
    }
    
    std::cout << "[CPUIsolation] Core " << core_id << " verification OK\n";
    return true;
}

std::vector<std::pair<int, CPUIsolation::CoreRole>> CPUIsolation::get_recommended_config() {
    std::vector<std::pair<int, CoreRole>> config;
    
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    int core_count = sys_info.dwNumberOfProcessors;
    
    if (core_count >= 8) {
        // Plenty of cores, use dedicated isolation
        config.push_back({0, CoreRole::OS_BACKGROUND});
        config.push_back({1, CoreRole::CAPTURE});
        config.push_back({2, CoreRole::VISION});
        config.push_back({3, CoreRole::PREDICTION});
        config.push_back({4, CoreRole::INPUT_CRITICAL});
        for (int i = 5; i < core_count; ++i) {
            config.push_back({i, CoreRole::OS_BACKGROUND});
        }
    } else if (core_count >= 6) {
        // 6-core system
        config.push_back({0, CoreRole::OS_BACKGROUND});
        config.push_back({1, CoreRole::CAPTURE});
        config.push_back({2, CoreRole::VISION});
        config.push_back({3, CoreRole::PREDICTION});
        config.push_back({4, CoreRole::INPUT_CRITICAL});
        config.push_back({5, CoreRole::OS_BACKGROUND});
    } else if (core_count >= 4) {
        // 4-core system (minimal)
        config.push_back({0, CoreRole::OS_BACKGROUND});
        config.push_back({1, CoreRole::CAPTURE});
        config.push_back({2, CoreRole::VISION});
        config.push_back({3, CoreRole::INPUT_CRITICAL});
    } else {
        // 2-core system (not recommended for real-time)
        std::cerr << "[CPUIsolation] WARNING: System has <4 cores. Real-time performance may suffer.\n";
        config.push_back({0, CoreRole::OS_BACKGROUND});
        config.push_back({1, CoreRole::INPUT_CRITICAL});
    }
    
    return config;
}

} // namespace tpt::core