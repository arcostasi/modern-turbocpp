#pragma once

#include <string>
#include <filesystem>

namespace TurboC {

struct CompilerConfig {
    std::string llvmBinPath = "C:\\LLVM\\bin";
    std::string cxxCompiler = "clang++.exe";
    std::string cCompiler = "clang.exe";
    std::string clangFormat = "clang-format.exe";
    std::string cppStandard = "c++23";       // c++23, c++20, c++17, c++14, c++11
    std::string cStandard = "c23";           // c23, c17, c11, c99
    std::string optLevel = "-O2";            // -O0, -O1, -O2, -O3, -Os
    std::string warningFlags = "-Wall -Wextra";
    std::string extraFlags = "-D_CRT_SECURE_NO_WARNINGS";
    std::string outDir = ".";
    std::string programArgs = "";
    std::string includeDirs = "";
    std::string libDirs = "";
    int processTimeoutSeconds = 30; // Timeout in seconds for compiler and external tools

    std::string getClangxxPath() const;
    std::string getClangPath() const;
    std::string getClangFormatPath() const;
};

struct EditorConfig {
    bool showLineNumbers = true;
    bool autoIndent = true;
    bool syntaxHighlight = true;
    int tabSize = 4;
    bool useTabs = false;
    bool autoSyntaxCheckOnSave = true;
    bool autoCloseDelimiters = true;
    std::string colorTheme = "ClassicBlue"; // ClassicBlue (default), ModernDark, RetroGreen, RetroAmber, ClassicPaper, DarkViolet, SlateTeal, ObsidianBlue
};

class ConfigManager {
public:
    static ConfigManager& instance();

    CompilerConfig compiler;
    EditorConfig editor;

    bool loadFromFile(const std::string& path = "turboc.json");
    bool saveToFile(const std::string& path = "turboc.json");

private:
    ConfigManager();
};

} // namespace TurboC
