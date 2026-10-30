#include "util_sleep.h"
#include "util_string.h"

#include "./log/log.h"

using namespace std::chrono_literals;

namespace dxvk {

  Sleep Sleep::s_instance;


  Sleep::Sleep() {

  }


  Sleep::~Sleep() {

  }


  void Sleep::initialize() {
    std::lock_guard lock(m_mutex);

    if (m_initialized.load())
      return;

    initializePlatformSpecifics();
    m_sleepThreshold = m_onWine ? m_sleepGranularity : 4 * m_sleepGranularity;

    m_initialized.store(true);
  }


  void Sleep::initializePlatformSpecifics() {
#ifdef _WIN32
    HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");

    if (ntdll) {
      NtDelayExecution = reinterpret_cast<NtDelayExecutionProc>(
        ::GetProcAddress(ntdll, "NtDelayExecution"));
      auto NtQueryTimerResolution = reinterpret_cast<NtQueryTimerResolutionProc>(
        ::GetProcAddress(ntdll, "NtQueryTimerResolution"));
      auto NtSetTimerResolution = reinterpret_cast<NtSetTimerResolutionProc>(
        ::GetProcAddress(ntdll, "NtSetTimerResolution"));

      ULONG min, max, cur;

      // Wine's implementation of these functions is a stub as of 6.10, which is fine
      // since it uses select() in NtDelayExecution. This is only relevant for Windows.
      if (NtQueryTimerResolution && !NtQueryTimerResolution(&min, &max, &cur)) {
        m_sleepGranularity = TimerDuration(cur);

        if (NtSetTimerResolution && !NtSetTimerResolution(max, TRUE, &cur)) {
          Logger::info(str::format("Setting timer interval to ", (double(max) / 10.0), " us"));
          m_sleepGranularity = TimerDuration(max);
        }
      }

      // If wine_get_version exists, we are on Linux/Proton and profit from better timer resolution
      if (GetProcAddress(ntdll, "wine_get_version") != NULL) {
        m_onWine = true;
      } else {
        // Fallback: Proton might hide exports in some games
        // We can still query for Wine using the Registry
        HKEY hKey = NULL;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\WineBus", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
          m_onWine = true;
          RegCloseKey(hKey);
        }
      }

      if (m_onWine)
        m_sleepGranularity = TimerDuration(125us);
    } else {
      // Assume 1ms sleep granularity by default
      m_sleepGranularity = TimerDuration(1ms);
    }
#else
    // Assume 0.5ms sleep granularity by default
    m_sleepGranularity = TimerDuration(500us);
#endif
  }


  Sleep::TimePoint Sleep::sleep(TimePoint t0, TimerDuration duration) {
    if (duration <= TimerDuration::zero())
      return t0;

    // If necessary, initialize function pointers and some values
    if (!m_initialized.load())
      initialize();

    // Busy-wait for the last couple of milliseconds since sleeping
    // on Windows is highly inaccurate and inconsistent.
    // on Wine we get Linux timer precision, handle it differently
    TimerDuration sleepThreshold = m_sleepThreshold;

    if (!m_onWine && m_sleepGranularity != TimerDuration::zero())
      sleepThreshold += duration / 6;

    using std::chrono::microseconds;
    TimePoint target = t0 + duration;
    TimePoint coarseTarget = target - sleepThreshold;

    TimePoint now = high_resolution_clock::now();

    while (now + std::chrono::microseconds(50) < coarseTarget) {
      TimerDuration sleepDuration = std::chrono::duration_cast<TimerDuration>(coarseTarget - now);
      systemSleep(sleepDuration);
      now = high_resolution_clock::now();
    }

    // Busy-wait until we have slept long enough
    while (now < target) {
      pause();
      now = high_resolution_clock::now();
    }

    return now;
  }


  void Sleep::systemSleep(TimerDuration duration) {
#ifdef _WIN32
    if (NtDelayExecution) {
      LARGE_INTEGER ticks;
      ticks.QuadPart = -duration.count();

      NtDelayExecution(FALSE, &ticks);
    } else {
      std::this_thread::sleep_for(duration);
    }
#else
    std::this_thread::sleep_for(duration);
#endif
  }


  void Sleep::pause() {
    #if defined(DXVK_ARCH_X86)
    _mm_pause();
    #elif defined(DXVK_ARCH_ARM64)
    __asm__ __volatile__ ("yield");
    #else
    /* Do nothing (busy-loop). Please add more #elif above here if
    * your CPU architecture has a suitable pause/yield instruction */
    #endif
  }

}
