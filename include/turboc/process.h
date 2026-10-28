#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace TurboC {

class AsyncProcess {
public:
    AsyncProcess(const std::string& commandLine, int timeoutSeconds = 30);
    ~AsyncProcess();

    // Starts background execution in a worker thread
    bool start();

    // Requests immediate termination of the running process
    void cancel();

    // Status queries
    bool isRunning() const { return running.load(); }
    bool isFinished() const { return finished.load(); }
    bool isTimedOut() const { return timedOut.load(); }
    bool isCanceled() const { return canceled.load(); }

    int getExitCode() const { return exitCode.load(); }
    double getElapsedSeconds() const { return elapsedSecs.load(); }
    int getTimeoutSeconds() const { return timeoutSeconds; }
    const std::string& getCommandLine() const { return cmd; }

    std::string getLatestLine() const;
    std::string getFullOutput() const;

    // Synchronous execution helper with timeout
    static int executeSync(const std::string& cmd, std::string& output, int timeoutSeconds = 30, bool* outTimedOut = nullptr);

private:
    void run();

    std::string cmd;
    int timeoutSeconds;

    std::atomic<bool> running{false};
    std::atomic<bool> finished{false};
    std::atomic<bool> canceled{false};
    std::atomic<bool> timedOut{false};
    std::atomic<int> exitCode{-1};
    std::atomic<double> elapsedSecs{0.0};

    mutable std::mutex outputMutex;
    std::string fullOutput;
    std::string latestLine;

    std::thread workerThread;

#ifdef _WIN32
    std::atomic<HANDLE> hChildProc{NULL};
#endif
};

} // namespace TurboC
