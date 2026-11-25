  #pragma once

  #include "../../util/util_time.h"
  #include <stdint.h>
  #include <atomic>

namespace dxvk {

  // somehow games (for example UE5 games) are calling NvAPI_setSleepMode
  // in strange ways, for example the game and the UI thread send different values.
  // at least that's what we observed in dx12 titles
  // so we have to filter what the game actually wants
  // hopefully x, 0, x, 0, x, 0 pattern is the only wrong pattern

  class NvApi_FpsLimitFilter {
      using time_point = dxvk::high_resolution_clock::time_point;
    public:

      void push( uint32_t us ) {

        if (us == 0) {
          auto t = m_lastNonNull.load(std::memory_order_acquire);
          int64_t dur = std::chrono::duration_cast<std::chrono::milliseconds>(
          dxvk::high_resolution_clock::now() - t).count();
          if (dur > 20)
            m_minInterval.store( 0, std::memory_order_release );
        }

        us = std::min( us, (uint32_t) 33333 );

        m_lastNonNull.store( dxvk::high_resolution_clock::now(), std::memory_order_release );
        m_minInterval.store( us, std::memory_order_release );

      }

      uint32_t getMinInterval() {
        return m_minInterval.load( std::memory_order_acquire );
      }

    private:

      std::atomic<uint32_t> m_minInterval;
      std::atomic<time_point> m_lastNonNull;
  };

};
