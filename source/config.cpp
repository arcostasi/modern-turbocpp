#include "turboc/config.h"
#include <fstream>
#include <sstream>
#include <format>
#include <iostream>

namespace TurboC {

std::string CompilerConfig::getClangxxPath() const {
    std::filesystem::path p(llvmBinPath);
    p /= cxxCompiler;
    return p.string();
}

std::string CompilerConfig::getClangPath() const {
    std::filesystem::path p(llvmBinPath);
    p /= cCompiler;
    return p.string();
}

std::string CompilerConfig::getClangFormatPath() const {
    std::filesystem::path p(llvmBinPath);
    p /= clangFormat;
    return p.string();
}

ConfigManager& ConfigManager::instance() {
    static ConfigManager mgr;
    return mgr;
}

ConfigManager::ConfigManager() {
    loadFromFile("turboc.json");
}

static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n\"");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n\"");
    return str.substr(start, end - start + 1);
}

static std::string unescapeJson(const std::string& str) {
    std::string res;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            if (str[i + 1] == '\\') {
                res += '\\';
                ++i;
            } else if (str[i + 1] == '\"') {
                res += '\"';
                ++i;
            } else if (str[i + 1] == 'n') {
                res += '\n';
                ++i;
            } else if (str[i + 1] == 't') {
                res += '\t';
                ++i;
            } else {
                res += str[i];
            }
        } else {
            res += str[i];
        }
    }
    return res;
}

bool ConfigManager::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        if (value.ends_with(',')) {
            value = trim(value.substr(0, value.length() - 1));
        }
        value = unescapeJson(value);

        if (key == "llvmBinPath") compiler.llvmBinPath = value;
        else if (key == "cxxCompiler") compiler.cxxCompiler = value;
        else if (key == "cCompiler") compiler.cCompiler = value;
        else if (key == "clangFormat") compiler.clangFormat = value;
        else if (key == "cppStandard") compiler.cppStandard = value;
        else if (key == "cStandard") compiler.cStandard = value;
        else if (key == "optLevel") compiler.optLevel = value;
        else if (key == "warningFlags") compiler.warningFlags = value;
        else if (key == "extraFlags") compiler.extraFlags = value;
        else if (key == "outDir") compiler.outDir = value;
        else if (key == "programArgs") compiler.programArgs = value;
        else if (key == "processTimeoutSeconds") {
            try { compiler.processTimeoutSeconds = std::stoi(value); } catch (...) {}
        }
        else if (key == "includeDirs") compiler.includeDirs = value;
        else if (key == "libDirs") compiler.libDirs = value;
        else if (key == "showLineNumbers") editor.showLineNumbers = (value == "true" || value == "1");
        else if (key == "autoIndent") editor.autoIndent = (value == "true" || value == "1");
        else if (key == "syntaxHighlight") editor.syntaxHighlight = (value == "true" || value == "1");
        else if (key == "tabSize") {
            try { editor.tabSize = std::stoi(value); } catch (...) {}
        }
        else if (key == "useTabs") editor.useTabs = (value == "true" || value == "1");
        else if (key == "autoSyntaxCheckOnSave") editor.autoSyntaxCheckOnSave = (value == "true" || value == "1");
        else if (key == "autoCloseDelimiters") editor.autoCloseDelimiters = (value == "true" || value == "1");
        else if (key == "colorTheme") editor.colorTheme = value;
    }
    return true;
}

static std::string escapeJson(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c == '\\') res += "\\\\";
        else if (c == '\"') res += "\\\"";
        else res += c;
    }
    return res;
}

bool ConfigManager::saveToFile(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"compiler\": {\n";
    file << std::format("    \"llvmBinPath\": \"{}\",\n", escapeJson(compiler.llvmBinPath));
    file << std::format("    \"cxxCompiler\": \"{}\",\n", escapeJson(compiler.cxxCompiler));
    file << std::format("    \"cCompiler\": \"{}\",\n", escapeJson(compiler.cCompiler));
    file << std::format("    \"clangFormat\": \"{}\",\n", escapeJson(compiler.clangFormat));
    file << std::format("    \"cppStandard\": \"{}\",\n", escapeJson(compiler.cppStandard));
    file << std::format("    \"cStandard\": \"{}\",\n", escapeJson(compiler.cStandard));
    file << std::format("    \"optLevel\": \"{}\",\n", escapeJson(compiler.optLevel));
    file << std::format("    \"warningFlags\": \"{}\",\n", escapeJson(compiler.warningFlags));
    file << std::format("    \"extraFlags\": \"{}\",\n", escapeJson(compiler.extraFlags));
    file << std::format("    \"outDir\": \"{}\",\n", escapeJson(compiler.outDir));
    file << std::format("    \"includeDirs\": \"{}\",\n", escapeJson(compiler.includeDirs));
    file << std::format("    \"libDirs\": \"{}\",\n", escapeJson(compiler.libDirs));
    file << std::format("    \"programArgs\": \"{}\",\n", escapeJson(compiler.programArgs));
    file << std::format("    \"processTimeoutSeconds\": {}\n", compiler.processTimeoutSeconds);
    file << "  },\n";
    file << "  \"editor\": {\n";
    file << std::format("    \"showLineNumbers\": {},\n", editor.showLineNumbers ? "true" : "false");
    file << std::format("    \"autoIndent\": {},\n", editor.autoIndent ? "true" : "false");
    file << std::format("    \"syntaxHighlight\": {},\n", editor.syntaxHighlight ? "true" : "false");
    file << std::format("    \"tabSize\": {},\n", editor.tabSize);
    file << std::format("    \"useTabs\": {},\n", editor.useTabs ? "true" : "false");
    file << std::format("    \"autoSyntaxCheckOnSave\": {},\n", editor.autoSyntaxCheckOnSave ? "true" : "false");
    file << std::format("    \"autoCloseDelimiters\": {},\n", editor.autoCloseDelimiters ? "true" : "false");
    file << std::format("    \"colorTheme\": \"{}\"\n", escapeJson(editor.colorTheme));
    file << "  }\n";
    file << "}\n";
    return true;
}

} // namespace TurboC
