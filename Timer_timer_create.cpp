#include <iostream>
#include <signal.h>
#include <time.h>
#include <mutex>
#include <thread>

class ThreadSafeTimer {
public:
    ThreadSafeTimer() : isRunning(false) {
        // Initialize mutex
        pthread_mutex_init(&timerMutex, nullptr);
    }

    ~ThreadSafeTimer() {
        stop();
        // Destroy the mutex
        pthread_mutex_destroy(&timerMutex);
    }

    void start(unsigned int intervalMilliseconds, void (*callback)(void)) {
        stop(); // Stop the timer if it's already running

        // Store the callback function
        this->callback = callback;

        // Create the timer
        struct sigevent sev;
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_value.sival_ptr = this;
        sev.sigev_notify_function = timerCallback;
        sev.sigev_notify_attributes = nullptr;

        if (timer_create(CLOCK_MONOTONIC, &sev, &timerID) == -1) {
            std::cerr << "Error creating timer.\n";
            return;
        }

        // Set the timer interval
        struct itimerspec its;
        its.it_interval.tv_sec = intervalMilliseconds / 1000;
        its.it_interval.tv_nsec = (intervalMilliseconds % 1000) * 1000000;
        its.it_value.tv_sec = its.it_interval.tv_sec;
        its.it_value.tv_nsec = its.it_interval.tv_nsec;

        // Start the timer
        pthread_mutex_lock(&timerMutex);
        isRunning = true;
        pthread_mutex_unlock(&timerMutex);
        if (timer_settime(timerID, 0, &its, nullptr) == -1) {
            std::cerr << "Error starting timer.\n";
            timer_delete(timerID);
            return;
        }
    }

    void stop() {
        pthread_mutex_lock(&timerMutex);
        if (isRunning) {
            // Stop the timer
            struct itimerspec its;
            its.it_interval.tv_sec = 0;
            its.it_interval.tv_nsec = 0;
            its.it_value.tv_sec = 0;
            its.it_value.tv_nsec = 0;

            timer_settime(timerID, 0, &its, nullptr);
            timer_delete(timerID);
            isRunning = false;
        }
        pthread_mutex_unlock(&timerMutex);
    }

private:
    timer_t timerID;
    void (*callback)(void);
    bool isRunning;
    pthread_mutex_t timerMutex;

    static void timerCallback(union sigval arg) {
        ThreadSafeTimer* timer = static_cast<ThreadSafeTimer*>(arg.sival_ptr);
        // Lock the mutex to ensure thread safety
        pthread_mutex_lock(&timer->timerMutex);
        if (timer->isRunning && timer->callback) {
            timer->callback();
        }
        pthread_mutex_unlock(&timer->timerMutex);
    }
};

// Example usage
void exampleCallback() {
    std::cout << "Timer callback executed!\n";
}

int main() {
    ThreadSafeTimer timer;
    timer.start(1000, exampleCallback); // Start the timer with a 1000ms (1 second) interval
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Sleep for 5 seconds
    timer.stop(); // Stop the timer

    return 0;
}
