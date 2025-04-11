#pragma once

#include <memory>
#include <future>
#include "core/protocol_handler.hpp"
#include "testing/benchmark_config.hpp"

namespace flowdriver::testing {

class BenchmarkEngine {
public:
    explicit BenchmarkEngine(ProtocolHandler* handler);
    ~BenchmarkEngine();

    BenchmarkResult run(const BenchmarkConfig& config);
    std::future<BenchmarkResult> runAsync(const BenchmarkConfig& config);
    void stop();

private:
    void validateConfig(const BenchmarkConfig& config);

    ProtocolHandler* handler_;
    std::atomic<bool> is_running_{false};
};

} // namespace flowdriver::testing 