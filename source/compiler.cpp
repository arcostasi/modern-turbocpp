#include "turboc/compiler.h"
#include "turboc/process.h"
#include <regex>
#include <sstream>
#include <format>
#include <iostream>
#include <filesystem>

namespace TurboC {

LLVMCompiler& LLVMCompiler::instance() {
    static LLVMCompiler inst;
    return inst;
}

std::string Diagnostic::getSeverityString() const {
    switch (severity) {
        case DiagnosticSeverity::Error: return "Error";
        case DiagnosticSeverity::Warning: return "Warning";
        case DiagnosticSeverity::Note: return "Note";
    }
    return "Message";
}

std::string Diagnostic::getFormattedText() const {
    std::filesystem::path p(file);
    std::string fname = p.filename().string();
    return std::format("{:<7} {:<15} {:>4}:{:<3} {}", getSeverityString(), fname, line, column, message);
}

int LLVMCompiler::executeProcess(const std::string& cmd, std::string& output, int timeoutSeconds) {
    bool timedOut = false;
    int code = AsyncProcess::executeSync(cmd, output, timeoutSeconds, &timedOut);
    if (timedOut) {
        output += "\n[ERROR] Process exceeded timeout limit and was aborted.\n";
    }
    return code;
}

std::vector<Diagnostic> LLVMCompiler::parseDiagnostics(const std::string& output) {
    std::vector<Diagnostic> diagnostics;
    std::istringstream stream(output);
    std::string line;

    // Pattern 1: C:\path\file.cpp:12:5: error: message
    // Pattern 2: file.cpp:12:5: warning: message
    // Pattern 3: file.cpp(12,5): error: message (MSVC format if invoked with clang-cl)
    std::regex clangRegex(R"(^(?:([a-zA-Z]:[\\/][^:\r\n]+|[^:\r\n]+)):(\d+):(\d+):\s+(fatal error|error|warning|note):\s+(.*)$)");
    std::regex msvcRegex(R"(^(?:([a-zA-Z]:[\\/][^(\r\n]+|[^(\r\n]+))\s*\(\s*(\d+)(?:,\s*(\d+))?\s*\)\s*:\s+(fatal error|error|warning|note)\s*:\s*(.*)$)");

    std::smatch match;
    while (std::getline(stream, line)) {
        if (line.ends_with('\r')) {
            line.pop_back();
        }

        if (std::regex_match(line, match, clangRegex)) {
            Diagnostic diag;
            diag.file = match[1].str();
            diag.line = std::stoi(match[2].str());
            diag.column = std::stoi(match[3].str());

            std::string sev = match[4].str();
            if (sev == "error" || sev == "fatal error") {
                diag.severity = DiagnosticSeverity::Error;
            } else if (sev == "warning") {
                diag.severity = DiagnosticSeverity::Warning;
            } else {
                diag.severity = DiagnosticSeverity::Note;
            }
            diag.message = match[5].str();
            diagnostics.push_back(diag);
        } else if (std::regex_match(line, match, msvcRegex)) {
            Diagnostic diag;
            diag.file = match[1].str();
            diag.line = std::stoi(match[2].str());
            diag.column = match[3].matched ? std::stoi(match[3].str()) : 1;

            std::string sev = match[4].str();
            if (sev == "error" || sev == "fatal error") {
                diag.severity = DiagnosticSeverity::Error;
            } else if (sev == "warning") {
                diag.severity = DiagnosticSeverity::Warning;
            } else {
                diag.severity = DiagnosticSeverity::Note;
            }
            diag.message = match[5].str();
            diagnostics.push_back(diag);
        }
    }

    return diagnostics;
}

static std::string buildDirFlags(const CompilerConfig& config) {
    std::string flags;
    if (!config.includeDirs.empty()) {
        std::istringstream iss(config.includeDirs);
        std::string dir;
        while (std::getline(iss, dir, ';')) {
            if (!dir.empty()) flags += std::format(" -I\"{}\"", dir);
        }
    }
    if (!config.libDirs.empty()) {
        std::istringstream iss(config.libDirs);
        std::string dir;
        while (std::getline(iss, dir, ';')) {
            if (!dir.empty()) flags += std::format(" -L\"{}\"", dir);
        }
    }
    return flags;
}

std::string LLVMCompiler::determineTargetPath(CompileMode mode,
                                              const std::string& sourcePath,
                                              const CompilerConfig& config,
                                              const std::string& customOutPath) {
    if (!customOutPath.empty()) return customOutPath;

    std::filesystem::path src(sourcePath);
    std::filesystem::path outP = (config.outDir.empty() || config.outDir == ".") ? src.parent_path() : config.outDir;

    switch (mode) {
        case CompileMode::SyntaxCheck:
            return "(Syntax Check Only)";
        case CompileMode::CompileObj:
            outP /= src.stem();
            outP += ".obj";
            return outP.string();
        case CompileMode::BuildExe:
            outP /= src.stem();
            outP += ".exe";
            return outP.string();
        case CompileMode::GenerateAssembly:
            outP /= src.stem();
            outP += ".asm";
            return outP.string();
    }
    return "";
}

std::string LLVMCompiler::buildCommandLine(CompileMode mode,
                                           const std::string& sourcePath,
                                           const CompilerConfig& config,
                                           const std::string& customOutPath) {
    std::filesystem::path src(sourcePath);
    std::string ext = src.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    bool isC = (ext == ".c");

    std::string compilerExe = isC ? config.getClangPath() : config.getClangxxPath();
    std::string langStandard = isC ? (config.cStandard.empty() ? "c23" : config.cStandard)
                                   : (config.cppStandard.empty() ? "c++23" : config.cppStandard);
    std::string dirFlags = buildDirFlags(config);
    std::string target = determineTargetPath(mode, sourcePath, config, customOutPath);

    switch (mode) {
        case CompileMode::SyntaxCheck:
            return std::format(
                "\"{}\" -fsyntax-only -fno-color-diagnostics -std={} {} {} {} \"{}\"",
                compilerExe, langStandard, config.warningFlags, config.extraFlags, dirFlags, sourcePath
            );
        case CompileMode::CompileObj:
            return std::format(
                "\"{}\" -c -std={} {} {} {} {} \"{}\" -o \"{}\"",
                compilerExe, langStandard, config.optLevel, config.warningFlags, config.extraFlags, dirFlags, sourcePath, target
            );
        case CompileMode::BuildExe:
            return std::format(
                "\"{}\" -std={} {} {} {} {} \"{}\" -o \"{}\"",
                compilerExe, langStandard, config.optLevel, config.warningFlags, config.extraFlags, dirFlags, sourcePath, target
            );
        case CompileMode::GenerateAssembly:
            return std::format(
                "\"{}\" -S -masm=intel -std={} {} {} {} {} \"{}\" -o \"{}\"",
                compilerExe, langStandard, config.optLevel, config.warningFlags, config.extraFlags, dirFlags, sourcePath, target
            );
    }
    return "";
}

CompilationResult LLVMCompiler::checkSyntax(const std::string& sourcePath, const CompilerConfig& config) {
    auto startTime = std::chrono::high_resolution_clock::now();
    CompilationResult res;

    std::string cmd = buildCommandLine(CompileMode::SyntaxCheck, sourcePath, config);
    res.outputExecutable = "(Syntax Check)";
    res.exitCode = executeProcess(cmd, res.rawOutput, config.processTimeoutSeconds);
    res.diagnostics = parseDiagnostics(res.rawOutput);

    for (const auto& d : res.diagnostics) {
        if (d.severity == DiagnosticSeverity::Error) res.errorCount++;
        else if (d.severity == DiagnosticSeverity::Warning) res.warningCount++;
        else if (d.severity == DiagnosticSeverity::Note) res.noteCount++;
    }

    res.success = (res.exitCode == 0 && res.errorCount == 0);

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = endTime - startTime;
    res.durationSeconds = diff.count();

    return res;
}

CompilationResult LLVMCompiler::compileToObj(const std::string& sourcePath, const CompilerConfig& config, const std::string& outObjPath) {
    auto startTime = std::chrono::high_resolution_clock::now();
    CompilationResult res;

    std::string targetObj = determineTargetPath(CompileMode::CompileObj, sourcePath, config, outObjPath);
    res.outputExecutable = targetObj;

    std::string cmd = buildCommandLine(CompileMode::CompileObj, sourcePath, config, targetObj);
    res.exitCode = executeProcess(cmd, res.rawOutput, config.processTimeoutSeconds);
    res.diagnostics = parseDiagnostics(res.rawOutput);

    for (const auto& d : res.diagnostics) {
        if (d.severity == DiagnosticSeverity::Error) res.errorCount++;
        else if (d.severity == DiagnosticSeverity::Warning) res.warningCount++;
        else if (d.severity == DiagnosticSeverity::Note) res.noteCount++;
    }

    res.success = (res.exitCode == 0 && res.errorCount == 0 && std::filesystem::exists(targetObj));

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = endTime - startTime;
    res.durationSeconds = diff.count();

    return res;
}

CompilationResult LLVMCompiler::compileAndLink(const std::string& sourcePath, const CompilerConfig& config, const std::string& outExePath) {
    auto startTime = std::chrono::high_resolution_clock::now();
    CompilationResult res;

    std::string targetExe = determineTargetPath(CompileMode::BuildExe, sourcePath, config, outExePath);
    res.outputExecutable = targetExe;

    std::string cmd = buildCommandLine(CompileMode::BuildExe, sourcePath, config, targetExe);
    res.exitCode = executeProcess(cmd, res.rawOutput, config.processTimeoutSeconds);
    res.diagnostics = parseDiagnostics(res.rawOutput);

    for (const auto& d : res.diagnostics) {
        if (d.severity == DiagnosticSeverity::Error) res.errorCount++;
        else if (d.severity == DiagnosticSeverity::Warning) res.warningCount++;
        else if (d.severity == DiagnosticSeverity::Note) res.noteCount++;
    }

    res.success = (res.exitCode == 0 && res.errorCount == 0 && std::filesystem::exists(targetExe));

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = endTime - startTime;
    res.durationSeconds = diff.count();

    return res;
}

CompilationResult LLVMCompiler::generateAssembly(const std::string& sourcePath, const CompilerConfig& config, const std::string& outAsmPath) {
    auto startTime = std::chrono::high_resolution_clock::now();
    CompilationResult res;

    std::string targetAsm = determineTargetPath(CompileMode::GenerateAssembly, sourcePath, config, outAsmPath);
    res.outputExecutable = targetAsm;

    std::string cmd = buildCommandLine(CompileMode::GenerateAssembly, sourcePath, config, targetAsm);
    res.exitCode = executeProcess(cmd, res.rawOutput, config.processTimeoutSeconds);
    res.diagnostics = parseDiagnostics(res.rawOutput);

    for (const auto& d : res.diagnostics) {
        if (d.severity == DiagnosticSeverity::Error) res.errorCount++;
        else if (d.severity == DiagnosticSeverity::Warning) res.warningCount++;
        else if (d.severity == DiagnosticSeverity::Note) res.noteCount++;
    }

    res.success = (res.exitCode == 0 && res.errorCount == 0 && std::filesystem::exists(targetAsm));

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = endTime - startTime;
    res.durationSeconds = diff.count();

    return res;
}

std::string LLVMCompiler::getCompilerVersionInfo(const CompilerConfig& config) {
    std::string compilerExe = config.getClangxxPath();
    std::string output;
    int code = executeProcess(std::format("\"{}\" -v", compilerExe), output, 5);
    if (code != 0 || output.empty()) {
        executeProcess(std::format("\"{}\" --version", compilerExe), output, 5);
    }
    return output;
}

bool LLVMCompiler::formatFile(const std::string& filePath, const CompilerConfig& config) {
    std::string formatExe = config.getClangFormatPath();
    if (!std::filesystem::exists(formatExe)) return false;

    std::string cmd = std::format("\"{}\" -i \"{}\"", formatExe, filePath);
    std::string out;
    int code = executeProcess(cmd, out, config.processTimeoutSeconds);
    return code == 0;
}

int LLVMCompiler::runExecutable(const std::string& exePath, const std::string& args) {
    std::string cmd;
    if (!args.empty()) {
        cmd = std::format("\"{}\" {}", exePath, args);
    } else {
        cmd = std::format("\"{}\"", exePath);
    }

    std::cout << "\n=======================================================\n";
    std::cout << " Modern Turbo C++ Runner: " << exePath << "\n";
    std::cout << "=======================================================\n\n";

    int ret = system(cmd.c_str());

    std::cout << "\n\n=======================================================\n";
    std::cout << std::format(" Process exited with code {}\n", ret);
    std::cout << " Press Enter to return to Modern Turbo C++...";
    std::cout.flush();

    std::string line;
    std::getline(std::cin, line);

    return ret;
}

} // namespace TurboC
