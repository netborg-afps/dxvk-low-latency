#include "d3d11_custom_reflex_device.h"
#include "d3d11_device.h"
#include "../dxvk/framepacer/dxvk_nvapi_pacing_adapter.h"

namespace dxvk {

  D3D11CustomReflexDevice::D3D11CustomReflexDevice(
          D3D11DXGIDevice*        pContainer,
          D3D11Device*            pDevice)
  : m_container(pContainer), m_device(pDevice) {
    auto dxvkDevice = pDevice->GetDXVKDevice();

    m_nvApi_pacingAdapter = new NvApi_PacingAdapter();
  }


  D3D11CustomReflexDevice::~D3D11CustomReflexDevice() {
    delete m_nvApi_pacingAdapter;
  }


  ULONG STDMETHODCALLTYPE D3D11CustomReflexDevice::AddRef() {
    return m_container->AddRef();
  }


  ULONG STDMETHODCALLTYPE D3D11CustomReflexDevice::Release() {
    return m_container->Release();
  }


  HRESULT STDMETHODCALLTYPE D3D11CustomReflexDevice::QueryInterface(
          REFIID                        riid,
          void**                        ppvObject) {
    return m_container->QueryInterface(riid, ppvObject);
  }


  BOOL STDMETHODCALLTYPE D3D11CustomReflexDevice::SupportsLowLatency() {
    return true;
  }


  HRESULT STDMETHODCALLTYPE D3D11CustomReflexDevice::LatencySleep() {
    m_nvApi_pacingAdapter->sleepAndBeginFrame();

    return S_OK;
  }


  HRESULT STDMETHODCALLTYPE D3D11CustomReflexDevice::SetLatencySleepMode(
          BOOL                          LowLatencyEnable,
          BOOL                          LowLatencyBoost,
          UINT32                        MinIntervalUs) {
    _INFO( str::format("SetLatencySleepMode() - enable:", LowLatencyEnable, ", minIntervalUs: ", MinIntervalUs) );
    m_nvApi_pacingAdapter->setLatencySleepMode(LowLatencyEnable, LowLatencyBoost, MinIntervalUs);
    return S_OK;
  }


  HRESULT STDMETHODCALLTYPE D3D11CustomReflexDevice::SetLatencyMarker(
          UINT64                        FrameId,
          UINT32                        MarkerType) {
    m_nvApi_pacingAdapter->setLatencyMarker(FrameId, VkLatencyMarkerNV(MarkerType));
    return S_OK;
  }


  HRESULT STDMETHODCALLTYPE D3D11CustomReflexDevice::GetLatencyInfo(
          D3D_LOW_LATENCY_RESULTS*      pLowLatencyResults) {
    return E_NOTIMPL;
  }


  void D3D11CustomReflexDevice::RegisterFramePacer(
          Rc<FramePacer>                pacer) {
    m_nvApi_pacingAdapter->registerPacer(pacer);
  }


  void D3D11CustomReflexDevice::UnregisterFramePacer(
          Rc<FramePacer>                pacer) {
    m_nvApi_pacingAdapter->unregisterPacer(pacer);
  }

}
