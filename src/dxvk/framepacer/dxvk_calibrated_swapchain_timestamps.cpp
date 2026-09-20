#include "dxvk_calibrated_swapchain_timestamps.h"
#include "../dxvk_device.h"

namespace dxvk {

  CalibratedSwapchainTimestamps::CalibratedSwapchainTimestamps( DxvkDevice* device )
  : m_device(device),
    m_canEnable(m_device->features().extPresentTiming.presentTiming && m_device->features().khrCalibratedTimestamps) {

    if (!m_device->features().khrCalibratedTimestamps)
      Logger::warn( "VK_KHR_calibrated_timestamps is not enabled while VK_EXT_present_timing is enabled, "
                    "possibly a Vulkan driver bug." );

    m_enabled = m_canEnable;

  }


  void CalibratedSwapchainTimestamps::calibrate( VkPresentStageFlagsEXT stageFlags, VkTimeDomainKHR timeDomain, uint64_t timeDomainId ) {

    std::lock_guard<dxvk::mutex> lock(m_mutex);
    if (!m_enabled || m_swapchain == VK_NULL_HANDLE)
      return;

    Calibration nextCalibration;

    // we are only interested in the device "now" timestamp via Vulkan
    // since getting the host timestamp directly proved to be more reliable
    // possibly because of how the GPU driver interacts with Wine
    VkSwapchainCalibratedTimestampInfoEXT ext{ VK_STRUCTURE_TYPE_SWAPCHAIN_CALIBRATED_TIMESTAMP_INFO_EXT };
    ext.swapchain = m_swapchain;
    ext.presentStage = stageFlags;
    ext.timeDomainId = timeDomainId;

    VkCalibratedTimestampInfoKHR calibratedTimestampInfo;
    calibratedTimestampInfo.sType = VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_KHR;
    calibratedTimestampInfo.pNext = &ext;
    calibratedTimestampInfo.timeDomain = timeDomain;

    VkResult res = m_device->vkd()->vkGetCalibratedTimestampsKHR(
      m_device->handle(), 1,
      &calibratedTimestampInfo,
      &nextCalibration.swapchainTimestamp,
      &nextCalibration.maxDeviation
    );

    nextCalibration.hostTimestamp = high_resolution_clock::now();

    if (unlikely(res != VK_SUCCESS)) {
      Logger::err( "Failed to calibrate swapchain timestamp" );
      return;
    }

    m_calibration = nextCalibration;

  }


  CalibratedSwapchainTimestamps::time_point CalibratedSwapchainTimestamps::getHostTimestamp( uint64_t swapchainTimestamp ) {

    std::lock_guard<dxvk::mutex> lock(m_mutex);
    if (unlikely(m_calibration.swapchainTimestamp == 0))
      return time_point{};

    int64_t deltaDeviceNanoseconds = swapchainTimestamp - m_calibration.swapchainTimestamp;
    return m_calibration.hostTimestamp + high_resolution_clock::nanoseconds( deltaDeviceNanoseconds );

  }

  uint64_t CalibratedSwapchainTimestamps::getSwapchainTimestamp( time_point hostTimestamp ) {

    std::lock_guard<dxvk::mutex> lock(m_mutex);
    if (unlikely(m_calibration.swapchainTimestamp == 0))
      return 0;

    int64_t deltaDeviceNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
      hostTimestamp - m_calibration.hostTimestamp).count();
    return m_calibration.swapchainTimestamp + deltaDeviceNanoseconds;

  }

}
