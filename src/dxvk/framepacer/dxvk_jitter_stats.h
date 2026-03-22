#pragma once

#include <stdint.h>
#include <atomic>
#include <deque>

#include "../../util/util_time.h"


namespace dxvk {


  struct JitterEntry {

    using time_point = high_resolution_clock::time_point;

    time_point t;

    int32_t frametime;
    int32_t latency;
    int32_t renderThreadLatency;

  };


  struct JitterTotal {

    int64_t count                 = { 0 };

    int64_t frametime             = { 0 };
    int64_t latency               = { 0 };
    int64_t renderThreadLatency   = { 0 };

  };


  class JitterStats {
  public:

    void push( const JitterEntry&& entry ) {

      int16_t newIndex = (m_curIndex + 1) & 1;
      JitterTotal& oldTotal = m_jitter[m_curIndex];
      JitterTotal& newTotal = m_jitter[newIndex];

      m_queue.push_back( std::move(entry) );

      newTotal.count     = oldTotal.count + 1;
      newTotal.frametime = oldTotal.frametime + entry.frametime;
      newTotal.latency   = oldTotal.latency + entry.latency;
      newTotal.renderThreadLatency = oldTotal.renderThreadLatency + entry.renderThreadLatency;

      // remove old items from the queue
      while (!m_queue.empty() && m_queue.front().t
        < entry.t - std::chrono::milliseconds (m_duration) ) {

        JitterEntry& e = m_queue.front();
        --newTotal.count;
        newTotal.frametime -= e.frametime;
        newTotal.latency   -= e.latency;
        newTotal.renderThreadLatency -= e.renderThreadLatency;

        m_queue.pop_front();
      }

      m_curIndex.store(newIndex);

    }

    JitterTotal getJitterTotal() const
      { return m_jitter[m_curIndex.load()]; }

  private:

    inline static constexpr int32_t m_duration = 30000;
    std::deque<JitterEntry> m_queue;

    JitterTotal m_jitter[2] = { };
    std::atomic<int16_t> m_curIndex = { 0 };

  };


}
