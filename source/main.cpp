#include "turboc/app.h"
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleTitleA("Modern Turbo C++ [LLVM C++23]");
    // Enable UTF-8 console output
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try {
        TurboC::TTurboCApp app(argc, argv);
        app.run();
        app.shutDown();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred." << std::endl;
        return 1;
    }

    return 0;
}
