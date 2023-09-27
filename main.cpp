#include <chrono>
#if defined(_WIN32) || defined(_WIN64)
#define _WINDOWS
#else
#define _MACOS
#endif

#ifdef _WINDOWS
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <conio.h>
#else
#include <ncurses.h>
#endif

#include <assert.h>

#include <algorithm>
#include <array>
#include <ctime>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <valarray>
#include <vector>

namespace chrono = std::chrono;
std::mutex mutex;

bool quit = false;

int g_count = 0;

template <typename T> class LoopList {
public:
  std::vector<T> data;
  int size = 5;
  // points to the data to be written
  int i = 0;

  // Actual elemtents being written.
  int actual_size = 0;

  // Returns all the valid data
  std::vector<T> ValidData() {
    std::vector<T> ret;
    if (actual_size < size) {
      for (int j = 0; j < i; j++) {
        ret.push_back(data[j]);
      }
    } else {
      ret = data;
    }
    return ret;
  }

  void Add(const T &val) {
    data[i] = val;
    i = (i + 1) % size;
    actual_size++;
    if (actual_size > size) {
      actual_size = size;
    }
  }

  void Clear() {
    data = std::vector<T>(size, T{});
    i = 0;
    actual_size = 0;
  }

  bool Empty() const { return !actual_size; }

  LoopList(int size) : size(size) { data = std::vector<T>(size, T{}); }
  LoopList() { data = std::vector<T>(size, T{}); }

  // Can only be called when list is not empty!
  T tail() const {
    if (i == 0) {
      return data[size - 1];
    }
    return data[i - 1];
  }

  // Can only be called when list is not empty!
  T head() const {
    if (actual_size < size) {
      return data[0];
    } else {
      return data[i];
    }
  }

  int Size() const { return actual_size; }
};

// 25 ms per tick.
LoopList<double> interval_history(5);

// Last 10 beats timestamp in ms.
LoopList<int64_t> last_beats(10);

/* -----------I/O------------------- */
char GetKeyboardInput() {
#ifdef _WINDOWS
  return _getch();
#else
  return getch();
#endif
}

#ifdef _WINDOWS
HANDLE hStdout;
void SetupWindowsDisplay() {
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
  // Hide cursor
  CONSOLE_CURSOR_INFO cursorInfo;
  GetConsoleCursorInfo(hStdout, &cursorInfo);
  cursorInfo.bVisible = false; // set the cursor visibility
  SetConsoleCursorInfo(hStdout, &cursorInfo);
}
#else
#endif

void PrintStr(int x, int y, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
#ifdef _WINDOWS
  SetConsoleCursorPosition(hStdout, {x, y});
  printf(fmt, args);
#else
  move(y, x);
  vwprintw(stdscr, fmt, args);
#endif
  va_end(args);
}

void Clear() {
  mutex.lock();
#ifdef _WINDOWS
  system("cls");
#else
  clear();
#endif
  mutex.unlock();
}

void Beat() {
  // start time for this practice.
  chrono::system_clock::time_point start_time;
  chrono::system_clock::time_point last_beat_time;

  while (!quit) {
    char ch = GetKeyboardInput();
    if (ch == 'c') {
      g_count = 0;
      Clear();
      last_beats.Clear();
      interval_history.Clear();
    } else if (ch == 'q') {
      quit = true;
    } else {
      g_count++;
    }

    // Start a new practice
    if (g_count == 1) {
      start_time = chrono::system_clock::now();
      last_beat_time = start_time;
    }

    if (g_count != 0) {
      int64_t unix_ms = chrono::duration_cast<chrono::milliseconds>(
                            chrono::system_clock::now().time_since_epoch())
                            .count();
      last_beats.Add(unix_ms);
    }

    // Duration since last beat
    if (g_count > 1) {
      auto now = chrono::system_clock::now();
      auto duration =
          chrono::duration_cast<chrono::duration<double, std::milli>>(
              now - last_beat_time)
              .count() /
          25;
      interval_history.Add(duration);
      last_beat_time = chrono::system_clock::now();
    }
  }
}

void Display() {
  chrono::system_clock::time_point last_print_time =
      chrono::system_clock::now() - chrono::seconds(2);
  std::string msg{};
  while (!quit) {
    auto now = chrono::system_clock::now();
    if (chrono::duration_cast<chrono::milliseconds>(now - last_print_time)
            .count() >= 400) {
      const std::time_t now_time_t = chrono::system_clock::to_time_t(now);
      std::stringstream ss;
      ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
      mutex.lock();
      PrintStr(20, 0, ss.str().c_str());
      mutex.unlock();

      // Display BPM
      int64_t unix_ms = chrono::duration_cast<chrono::milliseconds>(
                            chrono::system_clock::now().time_since_epoch())
                            .count();
      int duration_for_last_beats = unix_ms - last_beats.head();
      int size_for_last_beats = last_beats.Size();

      mutex.lock();
      if (last_beats.Empty()) {
        PrintStr(20, 1, "BPM (beats per minute): N/A");
      } else {
        PrintStr(20, 1, "BPM (beats per minute): %f",
                 (double)size_for_last_beats * 1000 * 60 /
                     duration_for_last_beats);
      }
      mutex.unlock();

      // Display BPS
      mutex.lock();
      if (last_beats.Empty()) {
        PrintStr(20, 2, "BPS (beats per seconds): N/A");
      } else {
        PrintStr(20, 2, "BPS (beats per seconds): %f",
                 (double)size_for_last_beats * 1000 / duration_for_last_beats);
      }
      mutex.unlock();

      // variation of the intervals
      msg = "Variation for beats (25 ms tick): ";
      if (g_count == 0 || interval_history.Empty()) {
        msg += "N/A";
      } else {
        std::vector<double> vec = interval_history.ValidData();
        double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
        double mean = sum / vec.size();

        double sq_sum =
            std::inner_product(vec.begin(), vec.end(), vec.begin(), 0.0);
        double stdev = std::sqrt(sq_sum / vec.size() - mean * mean);
        msg += std::to_string(stdev) + std::string("     ");
      }
      mutex.lock();
      PrintStr(20, 3, msg.c_str());
      mutex.unlock();
      msg.clear();

      last_print_time = now;
    } // Print time and info

    mutex.lock();
    PrintStr(0, 0, "%d", g_count);
    mutex.unlock();

    refresh();
    std::this_thread::sleep_for(chrono::milliseconds(50));
  }
}

int main() {
#ifdef _WINDOWS
  SetupWindowsDisplay();
#else
  initscr();
#endif

  std::thread beat_th(Beat);
  std::thread display_time_th(Display);

  display_time_th.join();
  beat_th.join();
  endwin();
}
