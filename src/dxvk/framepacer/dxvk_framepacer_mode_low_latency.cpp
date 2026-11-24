#include "dxvk_framepacer_mode_low_latency.h"
#include <vulkan/vulkan_core.h>

namespace dxvk {

  int32_t LowLatencyMode::getLowLatencyOffset( const DxvkOptions& options ) {
    int32_t offset = options.lowLatencyOffset;

    offset = std::max( -1000, offset );
    offset = std::min(  1000, offset );
    return offset;
  }

}
