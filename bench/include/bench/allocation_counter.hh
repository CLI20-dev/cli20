#pragma once

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstddef>

namespace bench {

struct AllocationStats {
  std::size_t allocations{};
  std::size_t bytes{};
};

class AllocationCounter {
 public:
  AllocationCounter() {
    reset();
    enabled() = true;
  }

  AllocationCounter(const AllocationCounter&) = delete;
  auto operator=(const AllocationCounter&) -> AllocationCounter& = delete;

  ~AllocationCounter() { enabled() = false; }

  static auto reset() -> void {
    allocations().store(0, std::memory_order_relaxed);
    bytes().store(0, std::memory_order_relaxed);
  }

  static auto record(std::size_t n) -> void {
    if (!enabled()) return;
    allocations().fetch_add(1, std::memory_order_relaxed);
    bytes().fetch_add(n, std::memory_order_relaxed);
  }

  [[nodiscard]] static auto snapshot() -> AllocationStats {
    return {
        .allocations = allocations().load(std::memory_order_relaxed),
        .bytes = bytes().load(std::memory_order_relaxed),
    };
  }

 private:
  static auto enabled() -> bool& {
    static thread_local bool value = false;
    return value;
  }

  static auto allocations() -> std::atomic<std::size_t>& {
    static std::atomic<std::size_t> value{};
    return value;
  }

  static auto bytes() -> std::atomic<std::size_t>& {
    static std::atomic<std::size_t> value{};
    return value;
  }
};

inline auto set_allocation_counters(benchmark::State& state,
                                    std::size_t allocations, std::size_t bytes)
    -> void {
  state.counters["allocs"] = benchmark::Counter(
      static_cast<double>(allocations), benchmark::Counter::kAvgIterations);
  state.counters["bytes"] = benchmark::Counter(
      static_cast<double>(bytes), benchmark::Counter::kAvgIterations);
}

template <class Fn>
auto measure_allocations(Fn&& fn) -> AllocationStats {
  AllocationCounter counter;
  fn();
  return counter.snapshot();
}

}  // namespace bench
