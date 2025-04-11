#pragma once

#include <chrono>
#include "core/types.hpp"

namespace flowdriver::testing {

/**
 * @brief Configuration for benchmark execution
 */
struct BenchmarkConfig {
    RequestConfig request;               // Request configuration to benchmark
    int concurrent_users{1};             // Number of concurrent users
    std::chrono::seconds duration{1};    // Duration of the benchmark
};

struct BenchmarkMetrics {
    std::size_t total_requests{0};
    std::size_t successful_requests{0};
    std::size_t failed_requests{0};
    double requests_per_second{0.0};
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
};

using BenchmarkResult = BenchmarkMetrics;

} // namespace flowdriver::testing 