#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <functional>

class Timer {
public:
    Timer() : is_running(false), time_interval(0), is_repeating(false) {}

    void start(unsigned int interval_ms, std::function<void()> callback, bool repeat = false) {
        std::unique_lock<std::mutex> lock(timer_mutex);

        if (!is_running) {
            time_interval = interval_ms;
            is_repeating = repeat;
            is_running = true;
            timer_thread = std::thread(&Timer::run, this);
            callback_ = callback;
        }
    }

    void stop() {
        std::unique_lock<std::mutex> lock(timer_mutex);
        if (is_running) {
            is_running = false;
            lock.unlock();
            timer_thread.join();
        }
    }

    bool isRunning() const {
        return is_running;
    }

private:
    void run() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(time_interval));

            std::unique_lock<std::mutex> lock(timer_mutex);
            if (!is_running) {
                break;
            }

            // Perform the action associated with the timer (you can modify this part)
            callback_();
            if (!is_repeating) {
                is_running = false;
                break;
            }
        }
    }

    bool is_running;
    unsigned int time_interval;
    bool is_repeating;
    std::thread timer_thread;
    mutable std::mutex timer_mutex;
    std::function<void()> callback_;
};
void myTimeout()
{
  std::cout << "Timeout occured" << std::endl;
}
int main() {
    Timer timer;
    timer.start(1000, myTimeout, true); // Fires every 1000 milliseconds (1 second) and repeats

    // Let the timer run for 5 seconds
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    timer.stop();

    return 0;
}
