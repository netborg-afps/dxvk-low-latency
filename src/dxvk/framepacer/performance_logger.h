#pragma once

#include <fstream>
#include <vector>
#include <queue>
#include <stdint.h>
#include "../../util/thread.h"


template <typename T>
class PerformanceLogger {
  public:

    PerformanceLogger( const char* filename )
    : m_filename(filename) {
      m_thread = dxvk::thread( [this]() { loop(); } );
    }

    ~PerformanceLogger() {

      { std::lock_guard<dxvk::mutex> lock(m_mutex);
        T item;
        item.setZero();
        m_queue.push(item);
        m_cond.notify_one();
      }
      m_thread.join();

    }


    void loop () {

      std::ofstream file;

      if (!file.is_open()) {
        file.open(m_filename, std::ios::out | std::ios::trunc | std::ios::binary);
      }

      while (true) {

        T item;
        { std::unique_lock<dxvk::mutex> lock( m_mutex );
          m_cond.wait( lock, [this] { return !m_queue.empty(); } );

          item = std::move(m_queue.front());
          m_queue.pop();
        }

        if (item.isZero()) {
          break;
        }

        file.write( reinterpret_cast<char*>(&item), sizeof(T) );
        file.flush();

      }

      if (file.is_open())
        file.close();

    }

    void pushFrame( const T& item ) {

      std::lock_guard<dxvk::mutex> lock(m_mutex);
      m_queue.push(item);
      m_cond.notify_one();

    }

  private:

    dxvk::condition_variable m_cond;
    dxvk::mutex m_mutex;
    std::queue< T > m_queue;

    const char* m_filename;
    dxvk::thread m_thread;

};

