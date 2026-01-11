#pragma once

#include "dxvk_framepacer_mode.h"
#include "dxvk_framepacer_mode_low_latency.h"
#include "../../util/util_flush.h"
#include "../../util/log/log.h"
#include "../../util/util_string.h"
#include <ctime>
#include <random>
#include <winuser.h>

namespace dxvk {

  /*
   * Switches between two modes with a button press. Can also be used for double blind testing.
   */

  class ABSwitchMode : public FramePacerMode {
    using time_point = high_resolution_clock::time_point;
  public:

    struct AB {
      std::atomic<FramePacerMode*> A             = { nullptr };
      std::atomic<FramePacerMode*> B             = { nullptr };
      std::atomic<FramePacerMode*> m_activeMode  = { nullptr };

      const char* getModeString( const FramePacerMode* mode ) const {
        if (mode == A) return "A";
        return "B";
      }
    };


    ABSwitchMode(LatencyMarkersStorage* storage, FrameSync* frameSync, const DxvkOptions& options, uint64_t firstFrameId)
    : FramePacerMode(AB_SWITCH, "ab-switch", storage, frameSync, firstFrameId) {

      // note, it currently doesn't make sense to compare vrr modes to non-vrr modes, because we first would need to support dynamic present modes (v-sync on/off)

      m_modeOne = std::make_unique<LowLatencyMode>(FramePacerMode::LOW_LATENCY, storage, frameSync, options, firstFrameId);
      m_modeTwo = std::make_unique<FramePacerMode>(FramePacerMode::MAX_FRAME_LATENCY, "max-frame-latency", storage, frameSync, firstFrameId);

//      m_modeOne = std::make_unique<LowLatencyMode>(FramePacerMode::LOW_LATENCY_VRR, storage, frameSync, options, firstFrameId, 340);

      m_randEngine = std::default_random_engine( m_randDevice() );
      m_uniformDist = std::uniform_real_distribution<>( 0.0, 1.0 );

      switchToRandomMode();

    }

    ~ABSwitchMode() {}


    void startFrame( uint64_t frameId ) override {

      auto now = high_resolution_clock::now();

      if ((GetAsyncKeyState('N') & 0x8000)) {// || (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
        if (std::chrono::duration_cast<std::chrono::milliseconds> (now - m_lastKeyRegistered).count() > 300) {
          switchModeToNextMode();
        }
        m_lastKeyRegistered = now;
      }

      if (GetAsyncKeyState('I') & 0x8000) {
        if (std::chrono::duration_cast<std::chrono::milliseconds> (now - m_lastKeyRegistered).count() > 300) {
          switchToRandomMode();
        }
        m_lastKeyRegistered = now;
      }

      m_curMode.load()->startFrame( frameId );

    }


    void switchToRandomMode() {

      float r = m_uniformDist(m_randEngine);
      Logger::info( str::format( "AB: switch to random mode: ", r ) );
      if (r > 0.5) {
        m_AB.A = m_modeOne.get();
        m_AB.B = m_modeTwo.get();
      } else {
        m_AB.A = m_modeTwo.get();
        m_AB.B = m_modeOne.get();
      }

      switchToMode( m_AB.A );

    }


    void switchModeToNextMode() {

      Logger::info( "AB: switch to next mode" );

      if (m_AB.m_activeMode == m_modeTwo.get()) {
        switchToMode( m_modeOne.get() );
      } else {
        switchToMode( m_modeTwo.get() );
      }

    }


    void switchToMode( FramePacerMode* mode ) {

      if (mode->m_mode == MAX_FRAME_LATENCY) {
        GpuFlushTracker::m_minPendingSubmissions = 2;
        GpuFlushTracker::m_minChunkCount = 3;
        m_frameSync->m_waitIsActive = false;
      } else {
        GpuFlushTracker::m_minPendingSubmissions = 1;
        GpuFlushTracker::m_minChunkCount = 1;
        m_frameSync->m_waitIsActive = true;
      }

      m_curMode.store( mode );
      m_AB.m_activeMode.store( mode );

    }


    // forward these messages, so we don't get freezing issues after the switch
    void notifyGpuReady( uint64_t frameId, time_point t ) override {
      m_modeOne->notifyGpuReady( frameId, t );
      m_modeTwo->notifyGpuReady( frameId, t );
    }

    void notifyQueueSubmit( uint64_t frameId, time_point t ) override {
      m_modeOne->notifyQueueSubmit( frameId, t );
      m_modeTwo->notifyQueueSubmit( frameId, t );
    }


    // also forward these; this might introduce very minor computation overhead (1-5 microseconds)
    // but the only thing relevant should be the management of the frame start
    void finishRender( uint64_t frameId ) override {
      m_modeOne->finishRender( frameId );
      m_modeTwo->finishRender( frameId );
    }

    void endFrame( uint64_t frameId ) override {
      m_modeOne->endFrame( frameId );
      m_modeTwo->endFrame( frameId );
    }

    virtual void setTargetFrameRate( double frameRate ) override {
      m_modeOne->setTargetFrameRate( frameRate );
      m_modeTwo->setTargetFrameRate( frameRate );
    }

    const AB& getABInfo() const
      { return m_AB; }

    FramePacerMode::Mode getCurMode() const
      { return m_curMode.load()->m_mode; }


  private:

    AB m_AB;
    std::atomic<FramePacerMode*> m_curMode;
    std::unique_ptr<FramePacerMode> m_modeOne;
    std::unique_ptr<FramePacerMode> m_modeTwo;

    time_point m_lastKeyRegistered;

    std::random_device m_randDevice;
    std::default_random_engine m_randEngine;
    std::uniform_real_distribution<> m_uniformDist;

  };

}
