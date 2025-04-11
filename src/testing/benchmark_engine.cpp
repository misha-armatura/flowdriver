#include "testing/benchmark_engine.hpp"
#include "core/error.hpp"
#include <thread>
#include <vector>
#include <atomic>

namespace flowdriver::testing {

BenchmarkEngine::BenchmarkEngine(ProtocolHandler* handler)
    : m_handler(handler)
{
}

BenchmarkEngine::~BenchmarkEngine() {
    stop();
}

BenchmarkResult BenchmarkEngine::run(const BenchmarkConfig& config) {
    validateConfig(config);
    
    BenchmarkResult result;
    result.start_time = std::chrono::system_clock::now();
    
    std::vector<std::thread> threads;
    std::atomic<size_t> success_count{0};
    std::atomic<size_t> error_count{0};
    
    is_running_ = true;
    
    // Create worker threads
    for (int i = 0; i < config.concurrent_users; ++i) {
        threads.emplace_back([this, &config, &success_count, &error_count]() {
            while (is_running_) {
                try {
                    auto req_result = m_handler->execute(config.request);
                    if (req_result.status_code >= 200 && req_result.status_code < 300) {
                        ++success_count;
                    } else {
                        ++error_count;
                    }
                } catch (const Error& e) {
                    ++error_count;
                }
            }
        });
    }
    
    std::this_thread::sleep_for(config.duration);
    stop();
    
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    result.end_time = std::chrono::system_clock::now();
    result.total_requests = success_count + error_count;
    result.successful_requests = success_count;
    result.failed_requests = error_count;
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        result.end_time - result.start_time).count();
    
    result.requests_per_second = static_cast<double>(result.total_requests) / duration;
    
    return result;
}

std::future<BenchmarkResult> BenchmarkEngine::runAsync(const BenchmarkConfig& config) {
    return std::async(std::launch::async, [this, config]() {
        return run(config);
    });
}

void BenchmarkEngine::stop() {
    is_running_ = false;
}

void BenchmarkEngine::validateConfig(const BenchmarkConfig& config) {
    if (config.concurrent_users <= 0) {
        throw Error(ErrorCode::INVALID_CONFIG, "Concurrent users must be greater than 0");
    }
    
    if (config.duration <= std::chrono::seconds(0)) {
        throw Error(ErrorCode::INVALID_CONFIG, "Duration must be greater than 0");
    }
    
    if (config.request.url.empty()) {
        throw Error(ErrorCode::INVALID_CONFIG, "Request URL cannot be empty");
    }
}

} // namespace flowdriver::testing 