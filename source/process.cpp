#include "turboc/process.h"
#include <vector>
#include <algorithm>

#define Uses_TProgram
#define Uses_TEventQueue
#include <tvision/tv.h>

namespace TurboC {

static inline void safeWakeUp() {
    if (TProgram::application != nullptr) {
        TEventQueue::wakeUp();
    }
}

AsyncProcess::AsyncProcess(const std::string& commandLine, int timeoutSeconds)
    : cmd(commandLine), timeoutSeconds(timeoutSeconds) {}

AsyncProcess::~AsyncProcess() {
    cancel();
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

bool AsyncProcess::start() {
    if (running.load()) return false;
    running = true;
    finished = false;
    canceled = false;
    timedOut = false;
    exitCode = -1;
    elapsedSecs = 0.0;

    {
        std::lock_guard<std::mutex> lock(outputMutex);
        fullOutput.clear();
        latestLine.clear();
    }

    workerThread = std::thread(&AsyncProcess::run, this);
    return true;
}

void AsyncProcess::cancel() {
    canceled = true;
#ifdef _WIN32
    HANDLE h = hChildProc.load();
    if (h != NULL && h != INVALID_HANDLE_VALUE) {
        TerminateProcess(h, 1);
    }
#endif
    safeWakeUp();
}

std::string AsyncProcess::getLatestLine() const {
    std::lock_guard<std::mutex> lock(outputMutex);
    return latestLine;
}

std::string AsyncProcess::getFullOutput() const {
    std::lock_guard<std::mutex> lock(outputMutex);
    return fullOutput;
}

void AsyncProcess::run() {
#ifdef _WIN32
    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
        running = false;
        finished = true;
        exitCode = -1;
        safeWakeUp();
        return;
    }
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);
    si.hStdError = hChildStd_OUT_Wr;
    si.hStdOutput = hChildStd_OUT_Wr;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    BOOL bSuccess = CreateProcessA(
        NULL,
        cmdBuffer.data(),
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    CloseHandle(hChildStd_OUT_Wr);

    if (!bSuccess) {
        CloseHandle(hChildStd_OUT_Rd);
        running = false;
        finished = true;
        exitCode = -1;
        safeWakeUp();
        return;
    }

    hChildProc = pi.hProcess;
    auto startTime = std::chrono::steady_clock::now();
    char buffer[2048];

    while (running.load() && !canceled.load()) {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - startTime).count();
        elapsedSecs = elapsed;

        if (timeoutSeconds > 0 && elapsed >= timeoutSeconds) {
            timedOut = true;
            TerminateProcess(pi.hProcess, 1);
            break;
        }

        DWORD bytesAvailable = 0;
        if (PeekNamedPipe(hChildStd_OUT_Rd, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
            DWORD toRead = std::min<DWORD>(bytesAvailable, sizeof(buffer) - 1);
            DWORD bytesRead = 0;
            if (ReadFile(hChildStd_OUT_Rd, buffer, toRead, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::lock_guard<std::mutex> lock(outputMutex);
                fullOutput.append(buffer, bytesRead);

                std::string piece(buffer, bytesRead);
                size_t lastNl = piece.find_last_not_of("\r\n");
                if (lastNl != std::string::npos) {
                    size_t prevNl = piece.find_last_of("\r\n", lastNl);
                    std::string line = (prevNl == std::string::npos) ? piece.substr(0, lastNl + 1) : piece.substr(prevNl + 1, lastNl - prevNl);
                    size_t first = line.find_first_not_of(" \t");
                    if (first != std::string::npos) {
                        latestLine = line.substr(first);
                    }
                }
            }
        }

        DWORD waitRes = WaitForSingleObject(pi.hProcess, 25);
        if (waitRes == WAIT_OBJECT_0) {
            break;
        }
    }

    // Read remaining bytes from pipe
    DWORD bytesAvailable = 0;
    while (PeekNamedPipe(hChildStd_OUT_Rd, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
        DWORD toRead = std::min<DWORD>(bytesAvailable, sizeof(buffer) - 1);
        DWORD bytesRead = 0;
        if (ReadFile(hChildStd_OUT_Rd, buffer, toRead, &bytesRead, NULL) && bytesRead > 0) {
            std::lock_guard<std::mutex> lock(outputMutex);
            fullOutput.append(buffer, bytesRead);
        }
    }

    if (canceled.load()) {
        TerminateProcess(pi.hProcess, 1);
    }

    DWORD dwExit = 0;
    GetExitCodeProcess(pi.hProcess, &dwExit);
    exitCode = static_cast<int>(dwExit);

    auto endTime = std::chrono::steady_clock::now();
    elapsedSecs = std::chrono::duration<double>(endTime - startTime).count();

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hChildStd_OUT_Rd);
    hChildProc = NULL;

    running = false;
    finished = true;

    safeWakeUp();
#else
    // Fallback for non-Windows if needed
    running = false;
    finished = true;
    exitCode = 0;
    safeWakeUp();
#endif
}

int AsyncProcess::executeSync(const std::string& cmd, std::string& output, int timeoutSeconds, bool* outTimedOut) {
    AsyncProcess proc(cmd, timeoutSeconds);
    if (!proc.start()) {
        output.clear();
        return -1;
    }

    while (proc.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    output = proc.getFullOutput();
    if (outTimedOut) {
        *outTimedOut = proc.isTimedOut();
    }
    return proc.getExitCode();
}

} // namespace TurboC
