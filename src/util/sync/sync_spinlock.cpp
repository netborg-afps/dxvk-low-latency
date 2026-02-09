#include "sync_spinlock.h"

#include "../../dxvk/hud/dxvk_hud_item.h"

namespace dxvk::sync {

  void Spinlock::lock() {
    auto t0 = dxvk::high_resolution_clock::now();
    spin(200, [this] { return try_lock(); });
    auto t1 = dxvk::high_resolution_clock::now();
    uint64_t dur = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    if (dur > dxvk::hud::HudDebugStallsItem::print_threshold
     && dur > dxvk::hud::HudDebugStallsItem::m_us) {
      dxvk::hud::HudDebugStallsItem::m_name = m_name;
      dxvk::hud::HudDebugStallsItem::m_us.store ( dur );

      auto& s = dxvk::hud::HudDebugStallsItem::m_startTime;
      uint64_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - s).count();
      dxvk::hud::HudDebugStallsItem::m_timestamp_ms = duration;
    }
  }

}

