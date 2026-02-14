#pragma once

#include <string>

// Memory monitoring utilities
class MemoryMonitor {
public:
    // Get current process memory usage in bytes
    static long long getCurrentMemoryUsage();
    
    // Get peak memory usage in bytes
    static long long getPeakMemoryUsage();
    
    // Format bytes to human-readable string (KB, MB, GB)
    static std::string formatBytes(long long bytes);
    
    // Log current memory usage to file
    static void logMemoryUsage(const std::string& log_file, const std::string& context = "");
};

// Memory leak detector - tracks memory growth over time
class MemoryLeakDetector {
public:
    MemoryLeakDetector();
    
    // Check for memory leaks and log if detected
    // Returns true if potential leak detected
    bool checkAndLog(const std::string& log_file, const std::string& context = "");
    
    // Reset baseline (useful after known memory-intensive operations)
    void resetBaseline();
    
    // Get statistics
    long long getBaselineMemory() const { return baseline_memory; }
    long long getCurrentGrowth() const;
    int getCheckCount() const { return check_count; }
    
private:
    long long baseline_memory;      // Initial memory usage
    long long last_check_memory;    // Memory at last check
    int check_count;                // Number of checks performed
    int consecutive_growth_count;   // Consecutive checks showing growth
    
    // Thresholds
    static const long long LEAK_THRESHOLD_BYTES = 5 * 1024 * 1024;  // 5 MB growth = potential leak
    static const int CONSECUTIVE_GROWTH_THRESHOLD = 5;               // 5 consecutive growths = leak pattern
};
