#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <assert.h>
#include <conio.h>

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


namespace {
  namespace chrono = std::chrono;

  HANDLE hStdout;

  std::mutex mutex;

  bool exit = false;

  // start time for this practice.
  chrono::system_clock::time_point start_time;
  chrono::system_clock::time_point last_beat_time;

  int count = 0;

  // 25 ms per tick.
  std::vector<double> interval_history;
  int interval_i = -1;
  constexpr int interval_size = 5;

  void Beat() {
    char ch = 0;

    while (!exit) {
      char ch = _getch();
      if (ch == 'c') {
        count = 0;
        mutex.lock();
        system("cls");
        mutex.unlock();
      } else if (ch == 'q') {
        exit = true;
      } else {
        count++;
      }

      // Start a new practice
      if (count == 1) {
        start_time = chrono::system_clock::now();
        interval_history.clear();
        last_beat_time = start_time;
      }

      // Duration since last beat
      if (count > 1) {
        auto now = chrono::system_clock::now();
        auto duration =
          chrono::duration_cast<chrono::duration<double, std::milli>>(
            now - last_beat_time)
          .count() / 25;
        if (interval_history.size() < interval_size) {
          interval_history.push_back(duration);
        } else {
          interval_i = (interval_i + 1) % interval_size;
          interval_history[interval_i] = duration;
        }
        last_beat_time = chrono::system_clock::now();
      }


    }
  }

  void Display() {
    chrono::system_clock::time_point last_print_time = chrono::system_clock::now() - chrono::seconds(2);
    std::string msg{};
    while (!exit) {
      auto now = chrono::system_clock::now();
      if (chrono::duration_cast<chrono::milliseconds>(now - last_print_time)
        .count() >= 400) {
        std::unique_lock<std::mutex> lk(mutex);
        SetConsoleCursorPosition(hStdout, { 20, 0 });
        const std::time_t now_time_t = chrono::system_clock::to_time_t(now);
        std::cout << std::put_time(std::localtime(&now_time_t),
          "%Y-%m-%d %H:%M:%S");

        // Display BPM
        SetConsoleCursorPosition(hStdout, { 20, 1 });
        int duration_from_start =
          chrono::duration_cast<chrono::milliseconds>(now - start_time).count();
        std::cout << "BPM (beats per minute): ";
        if (duration_from_start == 0 || count == 0) {
          std::cout << "N/A";
        } else {
          std::cout << (double)count * 1000 * 60 / duration_from_start;
        }

        // Display BPS
        SetConsoleCursorPosition(hStdout, { 20, 2 });
        std::cout << "BPM (beats per seconds): ";
        if (duration_from_start == 0 || count == 0) {
          std::cout << "N/A";
        } else {
          std::cout << (double)count * 1000 / duration_from_start;
        }

        // variation of the intervals
        lk.unlock();
        msg = "Variation for beats (25 ms tick): ";
        if (count == 0 || interval_history.empty()) {
          msg += "N/A";
        } else {
          auto& vec = interval_history;
          double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
          double mean = sum / vec.size();

          double sq_sum =
            std::inner_product(vec.begin(), vec.end(), vec.begin(), 0.0);
          double stdev = std::sqrt(sq_sum / vec.size() - mean * mean);
          msg += std::to_string(stdev) + std::string("     ");
        }
        lk.lock();
        SetConsoleCursorPosition(hStdout, { 20, 3 });
        std::cout << msg;
        msg.clear();

        last_print_time = now;
        SetConsoleCursorPosition(hStdout, COORD{ 0, 1 });
      }  // Print time and info

      mutex.lock();
      SetConsoleCursorPosition(hStdout, { 0, 0 });
      printf("%d\n", count);
      mutex.unlock();
      std::this_thread::sleep_for(chrono::milliseconds(50));
    }
  }
}  // namespace

int main() {
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

  // Hide cursor
  CONSOLE_CURSOR_INFO cursorInfo;
  GetConsoleCursorInfo(hStdout, &cursorInfo);
  cursorInfo.bVisible = false;  // set the cursor visibility
  SetConsoleCursorInfo(hStdout, &cursorInfo);

  std::thread beat_th(Beat);
  std::thread display_time_th(Display);

  display_time_th.join();
  beat_th.join();
}
