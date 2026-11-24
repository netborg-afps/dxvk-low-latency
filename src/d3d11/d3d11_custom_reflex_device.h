#pragma once

#include "d3d11_interfaces.h"

namespace dxvk {

  class D3D11DXGIDevice;
  class D3D11Device;
  class NvApi_PacingAdapter;

  class D3D11CustomReflexDevice : public ID3DLowLatencyDevice {

  public:

    D3D11CustomReflexDevice(
            D3D11DXGIDevice*        pContainer,
            D3D11Device*            pDevice);

    ~D3D11CustomReflexDevice();

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID                        riid,
            void**                        ppvObject);

    BOOL STDMETHODCALLTYPE SupportsLowLatency();

    HRESULT STDMETHODCALLTYPE LatencySleep();

    HRESULT STDMETHODCALLTYPE SetLatencySleepMode(
            BOOL                          LowLatencyEnable,
            BOOL                          LowLatencyBoost,
            UINT32                        MinIntervalUs);

    HRESULT STDMETHODCALLTYPE SetLatencyMarker(
            UINT64                        FrameId,
            UINT32                        MarkerType);

    HRESULT STDMETHODCALLTYPE GetLatencyInfo(
            D3D_LOW_LATENCY_RESULTS*      pLowLatencyResults);

    void RegisterFramePacer(
            Rc<FramePacer>                pacer);

    void UnregisterFramePacer(
            Rc<FramePacer>                pacer);

  private:

    D3D11DXGIDevice*  m_container;
    D3D11Device*      m_device;

    NvApi_PacingAdapter* m_nvApi_pacingAdapter;

  };

}
