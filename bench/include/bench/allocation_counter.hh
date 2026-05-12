#pragma once

#include <atomic>
#include <cstddef>

namespace bench {

struct AllocationStats {
  std::size_t allocations{};
  std::size_t bytes{};
};

class AllocationCounter {
 public:
  static auto reset() -> void {
    allocations().store(0, std::memory_order_relaxed);
    bytes().store(0, std::memory_order_relaxed);
  }

  static auto record(std::size_t n) -> void {
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
  static auto allocations() -> std::atomic<std::size_t>& {
    static std::atomic<std::size_t> value{};
    return value;
  }

  static auto bytes() -> std::atomic<std::size_t>& {
    static std::atomic<std::size_t> value{};
    return value;
  }
};

}  // namespace bench
