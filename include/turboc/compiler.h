#pragma once

#include "turboc/config.h"
#include <string>
#include <vector>
#include <chrono>

namespace TurboC {

enum class DiagnosticSeverity {
    Error,
    Warning,
    Note
};

enum class CompileMode {
    BuildExe,
    CompileObj,
    SyntaxCheck,
    GenerateAssembly
};

struct Diagnostic {
    std::string file;
    int line = 1;
    int column = 1;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string message;
    std::string sourceExcerpt;

    std::string getFormattedText() const;
    std::string getSeverityString() const;
};

struct CompilationResult {
    bool success = false;
    int exitCode = 0;
    std::vector<Diagnostic> diagnostics;
    int errorCount = 0;
    int warningCount = 0;
    int noteCount = 0;
    std::string rawOutput;
    double durationSeconds = 0.0;
    std::string outputExecutable;
};

class LLVMCompiler {
public:
    static LLVMCompiler& instance();

    // Runs clang++ -fsyntax-only to detect errors and warnings without writing binary
    CompilationResult checkSyntax(const std::string& sourcePath, const CompilerConfig& config);

    // Compiles the source file to an object file (.obj)
    CompilationResult compileToObj(const std::string& sourcePath, const CompilerConfig& config, const std::string& outObjPath = "");

    // Compiles and links the source file into an executable
    CompilationResult compileAndLink(const std::string& sourcePath, const CompilerConfig& config, const std::string& outExePath = "");

    // Generates readable assembly from the source file with clang++ -S -masm=intel
    CompilationResult generateAssembly(const std::string& sourcePath, const CompilerConfig& config, const std::string& outAsmPath = "");

    // Returns formatted compiler version and environment info
    std::string getCompilerVersionInfo(const CompilerConfig& config);

    // Formats a file in-place using clang-format
    bool formatFile(const std::string& filePath, const CompilerConfig& config);

    // Runs an executable in interactive console mode
    int runExecutable(const std::string& exePath, const std::string& args);

    // Parses raw clang output into structured Diagnostic objects
    static std::vector<Diagnostic> parseDiagnostics(const std::string& output);

    // Builds the full clang command line for the specified compilation mode
    static std::string buildCommandLine(CompileMode mode,
                                       const std::string& sourcePath,
                                       const CompilerConfig& config,
                                       const std::string& customOutPath = "");

    // Determines default target path for a compilation mode
    static std::string determineTargetPath(CompileMode mode,
                                           const std::string& sourcePath,
                                           const CompilerConfig& config,
                                           const std::string& customOutPath = "");

private:
    LLVMCompiler() = default;

    // Executes a process and captures both stdout and stderr with timeout
    static int executeProcess(const std::string& cmd, std::string& output, int timeoutSeconds = 30);
};

} // namespace TurboC
