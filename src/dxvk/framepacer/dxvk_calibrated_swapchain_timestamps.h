#pragma once

#include "../../util/util_time.h"
#include "../vulkan/vulkan_loader.h"
#include "../../util/thread.h"

namespace dxvk {

  class DxvkDevice;

  /*
   * Clock synchronization for Swapchain/CPU via the VK_KHR_calibrated_timestamps
   * device extension. The clocks need to get calibrated regularly, for example once every frame,
   * to account for clock drift.
   */

  class CalibratedSwapchainTimestamps {
  public:
    using time_point = high_resolution_clock::time_point;

    CalibratedSwapchainTimestamps( DxvkDevice* device );
    ~CalibratedSwapchainTimestamps() { }

    void registerSwapchain( VkSwapchainKHR swapchain ) {
      std::lock_guard<dxvk::mutex> lock(m_mutex);
      m_swapchain = swapchain;
      m_calibration = Calibration{};
    }

    void enable() { m_enabled = m_canEnable; }
    bool isEnabled() const { return m_enabled; }

    void calibrate( VkPresentStageFlagsEXT stageFlags, VkTimeDomainKHR timeDomain, uint64_t timeDomainId );
    time_point getHostTimestamp( uint64_t swapchainTimestamp );
    uint64_t getSwapchainTimestamp( time_point hostTimestamp );

  private:

    struct Calibration {
      using time_point = high_resolution_clock::time_point;
      uint64_t swapchainTimestamp = 0;
      uint64_t maxDeviation       = 0;
      time_point hostTimestamp    = time_point{};
    };

    DxvkDevice* m_device;

    dxvk::mutex m_mutex;
    VkSwapchainKHR m_swapchain;

    Calibration m_calibration;
    bool        m_enabled = false;

    const bool  m_canEnable;

  };


}
