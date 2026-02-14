#include "memory_monitor.h"
#include "utils.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/resource.h>

// OpenWRT/Linux-only implementation
long long MemoryMonitor::getCurrentMemoryUsage() {
    // Read current memory usage from /proc/self/status
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::istringstream iss(line.substr(6));
            long long mem_kb;
            iss >> mem_kb;
            return mem_kb * 1024; // Convert KB to bytes
        }
    }
    return -1;
}

long long MemoryMonitor::getPeakMemoryUsage() {
    // Get peak memory usage using getrusage
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024; // Convert KB to bytes on Linux
    }
    return -1;
}

std::string MemoryMonitor::formatBytes(long long bytes) {
    if (bytes < 0) return "N/A";
    
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 3) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

void MemoryMonitor::logMemoryUsage(const std::string& log_file, const std::string& context) {
    long long current = getCurrentMemoryUsage();
    long long peak = getPeakMemoryUsage();
    
    std::ostringstream msg;
    msg << "Memory Usage";
    if (!context.empty()) {
        msg << " [" << context << "]";
    }
    msg << " - Current: " << formatBytes(current) 
        << ", Peak: " << formatBytes(peak);
    
    write_log(log_file, msg.str());
}

// ============================================================================
// Memory Leak Detector Implementation
// ============================================================================

MemoryLeakDetector::MemoryLeakDetector() 
    : baseline_memory(0), last_check_memory(0), check_count(0), consecutive_growth_count(0) {
    baseline_memory = MemoryMonitor::getCurrentMemoryUsage();
    last_check_memory = baseline_memory;
}

void MemoryLeakDetector::resetBaseline() {
    baseline_memory = MemoryMonitor::getCurrentMemoryUsage();
    last_check_memory = baseline_memory;
    consecutive_growth_count = 0;
    check_count = 0;
}

long long MemoryLeakDetector::getCurrentGrowth() const {
    long long current = MemoryMonitor::getCurrentMemoryUsage();
    return current - baseline_memory;
}

bool MemoryLeakDetector::checkAndLog(const std::string& log_file, const std::string& context) {
    check_count++;
    
    long long current_memory = MemoryMonitor::getCurrentMemoryUsage();
    if (current_memory < 0) {
        write_log(log_file, "WARNING: Unable to read memory usage");
        return false;
    }
    
    long long growth_from_baseline = current_memory - baseline_memory;
    long long growth_from_last = current_memory - last_check_memory;
    
    // Track consecutive growth
    if (growth_from_last > 0) {
        consecutive_growth_count++;
    } else {
        consecutive_growth_count = 0;
    }
    
    bool leak_detected = false;
    std::ostringstream msg;
    
    // Check 1: Absolute growth threshold
    if (growth_from_baseline > LEAK_THRESHOLD_BYTES) {
        leak_detected = true;
        msg << "⚠️ MEMORY LEAK DETECTED [" << context << "] - "
            << "Memory grew by " << MemoryMonitor::formatBytes(growth_from_baseline)
            << " since baseline (Threshold: " << MemoryMonitor::formatBytes(LEAK_THRESHOLD_BYTES) << "). "
            << "Baseline: " << MemoryMonitor::formatBytes(baseline_memory)
            << ", Current: " << MemoryMonitor::formatBytes(current_memory)
            << ", Checks: " << check_count;
        
        std::cerr << msg.str() << std::endl;
        write_log(log_file, msg.str());
    }
    
    // Check 2: Consecutive growth pattern (slower leak)
    if (consecutive_growth_count >= CONSECUTIVE_GROWTH_THRESHOLD) {
        leak_detected = true;
        msg.str("");
        msg << "⚠️ MEMORY LEAK PATTERN DETECTED [" << context << "] - "
            << "Memory growing consistently for " << consecutive_growth_count << " consecutive checks. "
            << "Total growth: " << MemoryMonitor::formatBytes(growth_from_baseline)
            << ". Current: " << MemoryMonitor::formatBytes(current_memory);
        
        std::cerr << msg.str() << std::endl;
        write_log(log_file, msg.str());
        
        // Reset counter to avoid spam
        consecutive_growth_count = 0;
    }
    
    // Log normal memory status every check
    if (!leak_detected) {
        msg.str("");
        msg << "Memory Check [" << context << "] - "
            << "Current: " << MemoryMonitor::formatBytes(current_memory)
            << ", Growth: " << MemoryMonitor::formatBytes(growth_from_baseline)
            << " (" << (growth_from_baseline >= 0 ? "+" : "") 
            << MemoryMonitor::formatBytes(growth_from_last) << " from last check)"
            << ", Consecutive growth: " << consecutive_growth_count;
        write_log(log_file, msg.str());
    }
    
    last_check_memory = current_memory;
    return leak_detected;
}
