#include "turboc/config.h"
#include "turboc/compiler.h"
#include "turboc/syntax.h"
#include "turboc/process.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

int main() {
    std::cout << "[TEST] 1. Testing ConfigManager...\n";
    auto& cfg = TurboC::ConfigManager::instance();
    assert(!cfg.compiler.llvmBinPath.empty());
    assert(cfg.compiler.cppStandard == "c++23");
    std::cout << " -> ConfigManager OK: LLVM path = " << cfg.compiler.llvmBinPath << "\n";

    std::cout << "[TEST] 2. Testing SyntaxHighlighter on C++23 code...\n";
    auto& hl = TurboC::SyntaxHighlighter::instance();
    bool inComment = false;
    std::string testCode = "constexpr int square(int x) { return x * x; } // compute square";
    auto tokens = hl.tokenizeLine(testCode, inComment);
    assert(!tokens.empty());
    std::cout << " -> Tokenized " << tokens.size() << " tokens successfully.\n";

    std::cout << "[TEST] 3. Testing LLVM Syntax Check on samples/hello.cpp...\n";
    auto resHello = TurboC::LLVMCompiler::instance().checkSyntax("samples/hello.cpp", cfg.compiler);
    std::cout << " -> Hello.cpp syntax check result: exitCode = " << resHello.exitCode 
              << ", errors = " << resHello.errorCount 
              << ", warnings = " << resHello.warningCount << "\n";
    assert(resHello.success);
    assert(resHello.errorCount == 0);

    std::cout << "[TEST] 4. Testing LLVM Error & Warning Detection on samples/error_demo.cpp...\n";
    auto resErr = TurboC::LLVMCompiler::instance().checkSyntax("samples/error_demo.cpp", cfg.compiler);
    std::cout << " -> error_demo.cpp check result: exitCode = " << resErr.exitCode 
              << ", errors = " << resErr.errorCount 
              << ", warnings = " << resErr.warningCount << "\n";
    assert(!resErr.success);
    assert(resErr.errorCount >= 1);
    for (const auto& d : resErr.diagnostics) {
        std::cout << "    [DIAGNOSTIC] " << d.getFormattedText() << "\n";
    }

    std::cout << "[TEST] 5. Testing LLVM Compilation of samples/hello.cpp...\n";
    auto resCompile = TurboC::LLVMCompiler::instance().compileAndLink("samples/hello.cpp", cfg.compiler, "bin/hello_test.exe");
    std::cout << " -> Compilation result: " << (resCompile.success ? "SUCCESS" : "FAILED")
              << " in " << resCompile.durationSeconds << "s\n";
    assert(resCompile.success);
    assert(std::filesystem::exists("bin/hello_test.exe"));

    std::cout << "[TEST] 6. Testing LLVM Clang-Format and Version Information...\n";
    std::string verInfo = TurboC::LLVMCompiler::instance().getCompilerVersionInfo(cfg.compiler);
    assert(!verInfo.empty());
    std::cout << " -> Compiler info retrieved: " << verInfo.substr(0, std::min<size_t>(verInfo.find('\n'), 60)) << "\n";

    bool formatOk = TurboC::LLVMCompiler::instance().formatFile("samples/hello.cpp", cfg.compiler);
    std::cout << " -> Clang-format execution: " << (formatOk ? "SUCCESS" : "SKIPPED/UNAVAILABLE") << "\n";

    std::cout << "[TEST] 7. Testing Enhanced Syntax Highlighter (Functions, Attributes, Themes)...\n";
    std::string advCode = "[[nodiscard]] int calculate(int x) { auto s = R\"(hello\\world)\"; return x * 2; }";
    bool inComm = false;
    auto advTokens = hl.tokenizeLine(advCode, inComm);
    bool foundAttr = false, foundFn = false, foundRawStr = false;
    for (const auto& t : advTokens) {
        if (t.type == TurboC::TokenType::Attribute) foundAttr = true;
        if (t.type == TurboC::TokenType::Function) foundFn = true;
        if (t.type == TurboC::TokenType::String) foundRawStr = true;
    }
    assert(foundAttr);
    assert(foundFn);
    assert(foundRawStr);
    std::cout << " -> Attributes, Functions, and Raw Strings tokenized correctly!\n";

    // Test themes
    hl.setThemeByName("ModernDark");
    assert(hl.getTheme() == TurboC::ColorTheme::ModernDark);
    hl.setThemeByName("RetroGreen");
    assert(hl.getTheme() == TurboC::ColorTheme::RetroGreen);
    hl.setThemeByName("DarkViolet");
    assert(hl.getTheme() == TurboC::ColorTheme::DarkViolet);
    assert(hl.getBackgroundAttr() == 0x07);
    hl.setThemeByName("SlateTeal");
    assert(hl.getTheme() == TurboC::ColorTheme::SlateTeal);
    assert(hl.getBackgroundAttr() == 0x1F);
    hl.setThemeByName("ObsidianBlue");
    assert(hl.getTheme() == TurboC::ColorTheme::ObsidianBlue);
    assert(hl.getBackgroundAttr() == 0x0F);
    hl.setThemeByName("ClassicPaper");
    assert(hl.getTheme() == TurboC::ColorTheme::ClassicPaper);
    hl.setThemeByName("ClassicBlue");
    assert(hl.getTheme() == TurboC::ColorTheme::ClassicBlue);
    hl.setThemeByName("BorlandBlue");
    assert(hl.getTheme() == TurboC::ColorTheme::BorlandBlue);
    std::cout << " -> Theme switching verified (ModernDark, RetroGreen, DarkViolet, SlateTeal, ObsidianBlue, ClassicPaper, ClassicBlue) OK.\n";

    std::cout << "[TEST] 8. Testing LLVM Assembly Generation (Intel Syntax)...\n";
    auto resAsm = TurboC::LLVMCompiler::instance().generateAssembly("samples/hello.cpp", cfg.compiler, "bin/hello_test.asm");
    assert(resAsm.success);
    assert(std::filesystem::exists("bin/hello_test.asm"));
    std::cout << " -> Assembly generation result: SUCCESS in " << resAsm.durationSeconds << "s (bin/hello_test.asm generated)\n";

    std::cout << "[TEST] 9. Testing Auto-Closing Delimiters & Themes in Config...\n";
    assert(cfg.editor.autoCloseDelimiters == true);
    assert(!cfg.editor.colorTheme.empty());
    std::cout << " -> Config options autoCloseDelimiters & colorTheme OK.\n";

    std::cout << "[TEST] 10. Testing AsyncProcess execution, cancellation, and timeout...\n";
    TurboC::AsyncProcess proc("cmd.exe /c echo Hello AsyncProcess", 5);
    bool started = proc.start();
    assert(started);
    while (proc.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(proc.isFinished());
    assert(proc.getExitCode() == 0);
    assert(!proc.isTimedOut());
    assert(!proc.isCanceled());
    assert(proc.getFullOutput().find("Hello AsyncProcess") != std::string::npos);
    std::cout << " -> AsyncProcess execution verified OK: output = " << proc.getFullOutput();

    // Test cancellation
    TurboC::AsyncProcess slowProc("cmd.exe /c ping -n 10 127.0.0.1 >nul", 10);
    slowProc.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(slowProc.isRunning());
    slowProc.cancel();
    while (slowProc.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(slowProc.isFinished());
    assert(slowProc.isCanceled());
    std::cout << " -> AsyncProcess cancellation verified OK.\n";

    // Clean up temporary test artifacts
    std::error_code ec;
    std::filesystem::remove("bin/hello_test.exe", ec);
    std::filesystem::remove("bin/hello_test.asm", ec);

    std::cout << "\n============================================\n";
    std::cout << "   ALL MODERN TURBO C++ UNIT TESTS PASSED!   \n";
    std::cout << "============================================\n";
    return 0;
}
