#pragma once

#include "performance_logger.h"


struct BenchmarkInfo {
  void setZero() { memset(this, 0, sizeof(BenchmarkInfo)); }
  bool isZero() { return frameTimeStart == 0; }

  uint32_t frameTimeStart;
  uint32_t frameTimeEnd;

  uint32_t latency;
};

using Benchmark = PerformanceLogger<BenchmarkInfo>;
