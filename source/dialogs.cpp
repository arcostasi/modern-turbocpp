#include "turboc/dialogs.h"
#include "turboc/commands.h"
#include "turboc/syntax.h"
#include <format>
#include <filesystem>
#include <fstream>
#include <cstring>

namespace TurboC {

// ============================================================================
// TDynamicText & TProgressBar Implementation
// ============================================================================

TDynamicText::TDynamicText(const TRect& bounds, const std::string& initialText, TColorAttr color) noexcept
    : TView(bounds), text(initialText), customColor(color) {
    growMode |= gfFixed;
}

void TDynamicText::setText(const std::string& newText) {
    if (text != newText) {
        text = newText;
        drawView();
    }
}

void TDynamicText::setColor(TColorAttr color) {
    if (customColor != color) {
        customColor = color;
        drawView();
    }
}

void TDynamicText::draw() {
    TDrawBuffer b;
    TColorAttr color = (customColor != 0) ? TColorAttr(customColor) : TColorAttr(getColor(1));
    b.moveChar(0, ' ', color, size.x);
    std::string display = text;
    if (static_cast<int>(display.length()) > size.x) {
        display = display.substr(0, std::max(0, size.x - 3)) + "...";
    }
    b.moveStr(0, display.c_str(), color);
    writeLine(0, 0, size.x, 1, b);
}

TProgressBar::TProgressBar(const TRect& bounds) noexcept : TView(bounds), percent(0) {
    growMode |= gfFixed;
}

void TProgressBar::setPercent(int p) {
    if (p < 0) p = 0;
    if (p > 100) p = 100;
    if (percent != p) {
        percent = p;
        drawView();
    }
}

void TProgressBar::draw() {
    TDrawBuffer b;
    TColorAttr frameColor = getColor(1);
    TColorAttr fillColor = 0x1E; // Yellow on blue
    TColorAttr emptyColor = 0x17; // Gray on blue

    b.moveChar(0, ' ', emptyColor, size.x);
    b.moveChar(0, '[', frameColor, 1);
    b.moveChar(size.x - 1, ']', frameColor, 1);

    int barWidth = size.x - 2;
    int fillWidth = (barWidth * percent) / 100;

    for (int i = 0; i < fillWidth; ++i) {
        b.moveChar(1 + i, 0xDB, fillColor, 1);
    }

    std::string pctStr = std::format(" {}% ", percent);
    int pos = (size.x - static_cast<int>(pctStr.length())) / 2;
    if (pos > 0) {
        b.moveStr(pos, pctStr.c_str(), 0x1F);
    }

    writeLine(0, 0, size.x, 1, b);
}

// ============================================================================
// TCompileDialog Implementation (Real-Time Async Execution)
// ============================================================================

static const char* getCompileModeTitle(CompileMode mode) {
    switch (mode) {
        case CompileMode::BuildExe: return "Compiling / Building Executable";
        case CompileMode::CompileObj: return "Compiling to Object File";
        case CompileMode::SyntaxCheck: return "Syntax Check (LLVM Clang)";
        case CompileMode::GenerateAssembly: return "Generating Assembly (Intel)";
    }
    return "Compiling";
}

static const char* getCompileModeRunningString(CompileMode mode) {
    switch (mode) {
        case CompileMode::BuildExe: return "Compiling & linking executable...";
        case CompileMode::CompileObj: return "Compiling source to OBJ...";
        case CompileMode::SyntaxCheck: return "Checking C++23 syntax...";
        case CompileMode::GenerateAssembly: return "Generating Intel assembly...";
    }
    return "Compiling...";
}

TCompileDialog::TCompileDialog(const std::string& sourceFile,
                               CompileMode mode,
                               const CompilerConfig& config,
                               const std::string& customOutPath) noexcept
    : TDialog(TRect(0, 0, 62, 17), getCompileModeTitle(mode)),
      TWindowInit(&TCompileDialog::initFrame),
      sourcePath(sourceFile),
      compileMode(mode),
      compilerConfig(config)
{
    options |= ofCentered;
    targetPath = LLVMCompiler::determineTargetPath(mode, sourceFile, config, customOutPath);

    std::filesystem::path src(sourceFile);
    std::filesystem::path tgt(targetPath);

    statusLabel = new TDynamicText(TRect(4, 2, 58, 3), std::format("Status:    {} [|]", getCompileModeRunningString(mode)), 0x1E);
    insert(statusLabel);

    mainFileLabel = new TDynamicText(TRect(4, 4, 58, 5), std::format("Main file: {:<32}", src.filename().string()));
    insert(mainFileLabel);

    targetLabel = new TDynamicText(TRect(4, 5, 58, 6), std::format("Target:    {:<32}", tgt.filename().string()));
    insert(targetLabel);

    elapsedLabel = new TDynamicText(TRect(4, 6, 58, 7), std::format("Elapsed:   0.0s  (Timeout: {}s)", config.processTimeoutSeconds));
    insert(elapsedLabel);

    diagnosticsLabel = new TDynamicText(TRect(4, 7, 58, 8), "Errors:    0            Warnings:  0");
    insert(diagnosticsLabel);

    insert(new TStaticText(TRect(4, 9, 58, 10), "Activity:"));

    activityLabel = new TDynamicText(TRect(4, 10, 58, 11), "Starting LLVM Clang process...", 0x1B);
    insert(activityLabel);

    cancelBtn = new TButton(TRect(23, 13, 39, 15), "~C~ancel", cmCancel, bfDefault);
    insert(cancelBtn);

    viewMsgsBtn = new TButton(TRect(12, 13, 30, 15), "~V~iew Messages", cmYes, bfDefault);
    viewMsgsBtn->hide();
    insert(viewMsgsBtn);

    okBtn = new TButton(TRect(32, 13, 46, 15), "O~K~", cmOK, bfNormal);
    okBtn->hide();
    insert(okBtn);

    std::string cmd = LLVMCompiler::buildCommandLine(mode, sourceFile, config, targetPath);
    process = std::make_unique<AsyncProcess>(cmd, config.processTimeoutSeconds);
    process->start();

    timerId = setTimer(50, 50);
}

TCompileDialog::~TCompileDialog() {
    if (timerId) {
        killTimer(timerId);
        timerId = nullptr;
    }
    if (process && process->isRunning()) {
        process->cancel();
    }
}

void TCompileDialog::handleEvent(TEvent& event) {
    if (event.what == evCommand) {
        if (event.message.command == cmCancel) {
            if (process && process->isRunning()) {
                process->cancel();
                if (statusLabel) {
                    statusLabel->setColor(0x4F);
                    statusLabel->setText("Status:    Canceled by user");
                }
                if (activityLabel) {
                    activityLabel->setText("Operation aborted by user.");
                }
                finishProcess();
                endModal(cmCancel);
                clearEvent(event);
                return;
            }
        }
    } else if (event.what == evBroadcast) {
        if (event.message.command == cmTimerExpired || event.message.command == cmPeriodicUpdate) {
            if (!hasFinished) {
                updateUI();
            }
        }
    }

    TDialog::handleEvent(event);
}

void TCompileDialog::updateUI() {
    if (!process) return;

    if (process->isRunning()) {
        spinnerIndex = (spinnerIndex + 1) % 4;
        static const char spinChars[] = {'|', '/', '-', '\\'};

        double el = process->getElapsedSeconds();
        int to = process->getTimeoutSeconds();
        double rem = (to > 0) ? std::max(0.0, to - el) : 0.0;

        elapsedLabel->setText(std::format("Elapsed:   {:.1f}s  (Timeout: {}s, Rem: {:.1f}s)", el, to, rem));
        statusLabel->setText(std::format("Status:    {} [{}]", getCompileModeRunningString(compileMode), spinChars[spinnerIndex]));

        std::string latest = process->getLatestLine();
        if (!latest.empty()) {
            activityLabel->setText(latest);
        }
    } else if (process->isFinished() && !hasFinished) {
        finishProcess();
    }
}

void TCompileDialog::finishProcess() {
    if (hasFinished) return;
    hasFinished = true;

    if (timerId) {
        killTimer(timerId);
        timerId = nullptr;
    }

    result.rawOutput = process->getFullOutput();
    result.exitCode = process->getExitCode();
    result.durationSeconds = process->getElapsedSeconds();
    result.outputExecutable = targetPath;
    result.diagnostics = LLVMCompiler::parseDiagnostics(result.rawOutput);

    result.errorCount = 0;
    result.warningCount = 0;
    result.noteCount = 0;
    for (const auto& d : result.diagnostics) {
        if (d.severity == DiagnosticSeverity::Error) result.errorCount++;
        else if (d.severity == DiagnosticSeverity::Warning) result.warningCount++;
        else if (d.severity == DiagnosticSeverity::Note) result.noteCount++;
    }

    if (process->isTimedOut()) {
        result.success = false;
        result.errorCount++;
        statusLabel->setColor(0x4F);
        statusLabel->setText(std::format("Status:    TIMEOUT! Exceeded {}s and was aborted", compilerConfig.processTimeoutSeconds));
        activityLabel->setText("Process was forcibly terminated to prevent system hang.");
    } else if (process->isCanceled()) {
        result.success = false;
        statusLabel->setColor(0x4F);
        statusLabel->setText("Status:    Canceled by user");
        activityLabel->setText("Compilation aborted by user.");
    } else if (result.exitCode == 0 && result.errorCount == 0) {
        result.success = true;
        statusLabel->setColor(0x1A);
        std::string modeDone = (compileMode == CompileMode::SyntaxCheck) ? "Syntax check clean!" :
                               (compileMode == CompileMode::GenerateAssembly) ? "Assembly generated!" :
                               (compileMode == CompileMode::CompileObj) ? "Object file compiled!" : "Build Successful!";
        statusLabel->setText(std::format("Status:    {} (in {:.2f}s)", modeDone, result.durationSeconds));

        if (std::filesystem::exists(targetPath)) {
            try {
                auto sz = std::filesystem::file_size(targetPath);
                activityLabel->setText(std::format("Target: {} ({:.1f} KB)", targetPath, sz / 1024.0));
            } catch (...) {
                activityLabel->setText(std::format("Target: {}", targetPath));
            }
        } else {
            activityLabel->setText("Operation completed successfully with no errors.");
        }
    } else {
        result.success = false;
        statusLabel->setColor(0x4E);
        statusLabel->setText(std::format("Status:    Errors occurred ({} errors, {} warnings)", result.errorCount, result.warningCount));
        if (!result.diagnostics.empty()) {
            activityLabel->setText(result.diagnostics[0].getFormattedText());
        } else if (!process->getLatestLine().empty()) {
            activityLabel->setText(process->getLatestLine());
        } else {
            activityLabel->setText("Build failed with non-zero exit code.");
        }
    }

    elapsedLabel->setText(std::format("Elapsed:   {:.2f} seconds total", result.durationSeconds));
    diagnosticsLabel->setText(std::format("Errors:    {:<12} Warnings:  {:<12}", result.errorCount, result.warningCount));

    cancelBtn->hide();
    if (result.errorCount > 0 || result.warningCount > 0) {
        viewMsgsBtn->show();
        okBtn->show();
        TRect rOk(32, 13, 46, 15);
        okBtn->locate(rOk);
        okBtn->makeDefault(False);
        TRect rView(12, 13, 30, 15);
        viewMsgsBtn->locate(rView);
        viewMsgsBtn->makeDefault(True);
        viewMsgsBtn->select();
    } else {
        okBtn->show();
        TRect rOk(22, 13, 38, 15);
        okBtn->locate(rOk);
        okBtn->makeDefault(True);
        okBtn->select();
    }
    drawView();
}

// ============================================================================
// TBuildAllDialog Implementation (Batch Async Execution)
// ============================================================================

TBuildAllDialog::TBuildAllDialog(const std::vector<std::string>& files,
                                 const CompilerConfig& config) noexcept
    : TDialog(TRect(0, 0, 64, 18), "Build All (Batch Compilation)"),
      TWindowInit(&TBuildAllDialog::initFrame),
      fileList(files),
      compilerConfig(config)
{
    options |= ofCentered;

    overallLabel = new TDynamicText(TRect(4, 2, 60, 3), std::format("Overall:   File 1 of {} (0%)", files.empty() ? 1 : files.size()));
    insert(overallLabel);

    progressBar = new TProgressBar(TRect(4, 4, 60, 5));
    insert(progressBar);

    currentFileLabel = new TDynamicText(TRect(4, 6, 60, 7), "Current:   Starting...");
    insert(currentFileLabel);

    statusLabel = new TDynamicText(TRect(4, 7, 60, 8), "Status:    Initializing batch... [|]", 0x1E);
    insert(statusLabel);

    elapsedLabel = new TDynamicText(TRect(4, 8, 60, 9), "Elapsed:   0.0s");
    insert(elapsedLabel);

    insert(new TStaticText(TRect(4, 10, 60, 11), "Activity:"));

    activityLabel = new TDynamicText(TRect(4, 11, 60, 12), "Preparing build tasks...", 0x1B);
    insert(activityLabel);

    cancelBtn = new TButton(TRect(24, 14, 40, 16), "~C~ancel", cmCancel, bfDefault);
    insert(cancelBtn);

    okBtn = new TButton(TRect(24, 14, 40, 16), "O~K~", cmOK, bfDefault);
    okBtn->hide();
    insert(okBtn);

    startNextFile();
    timerId = setTimer(50, 50);
}

TBuildAllDialog::~TBuildAllDialog() {
    if (timerId) {
        killTimer(timerId);
        timerId = nullptr;
    }
    if (currentProcess && currentProcess->isRunning()) {
        currentProcess->cancel();
    }
}

void TBuildAllDialog::startNextFile() {
    if (currentIndex >= fileList.size() || wasCanceled) {
        finishAll();
        return;
    }

    std::string currentPath = fileList[currentIndex];
    std::filesystem::path p(currentPath);

    int pct = (fileList.size() > 0) ? static_cast<int>((currentIndex * 100) / fileList.size()) : 0;
    overallLabel->setText(std::format("Overall:   File {} of {} ({}%)", currentIndex + 1, fileList.size(), pct));
    progressBar->setPercent(pct);

    currentFileLabel->setText(std::format("Current:   {:<36}", p.filename().string()));
    statusLabel->setText("Status:    Building with Clang... [|]");
    activityLabel->setText(std::format("Compiling {}", p.filename().string()));

    std::string targetExe = LLVMCompiler::determineTargetPath(CompileMode::BuildExe, currentPath, compilerConfig);
    std::string cmd = LLVMCompiler::buildCommandLine(CompileMode::BuildExe, currentPath, compilerConfig, targetExe);

    currentProcess = std::make_unique<AsyncProcess>(cmd, compilerConfig.processTimeoutSeconds);
    currentProcess->start();
}

void TBuildAllDialog::finishAll() {
    allDone = true;
    if (timerId) {
        killTimer(timerId);
        timerId = nullptr;
    }

    progressBar->setPercent(100);
    int total = static_cast<int>(fileList.size());
    overallLabel->setText(std::format("Overall:   {} file(s) processed (100%)", total));

    if (wasCanceled) {
        statusLabel->setColor(0x4F);
        statusLabel->setText("Status:    Batch canceled by user");
        activityLabel->setText(std::format("Halted. Succeeded: {}, Failed: {}, Skipped: {}",
                                           succeededCount, failedCount, total - succeededCount - failedCount));
    } else if (failedCount == 0) {
        statusLabel->setColor(0x1A);
        statusLabel->setText(std::format("Status:    Build All Successful! ({} files)", succeededCount));
        activityLabel->setText("All targets built successfully with no errors.");
    } else {
        statusLabel->setColor(0x4E);
        statusLabel->setText(std::format("Status:    Build All Completed with errors! ({} failed)", failedCount));
        activityLabel->setText(std::format("Done. Succeeded: {}, Failed: {}", succeededCount, failedCount));
    }

    cancelBtn->hide();
    okBtn->show();
    okBtn->makeDefault(True);
    okBtn->select();
    drawView();
}

void TBuildAllDialog::handleEvent(TEvent& event) {
    if (event.what == evCommand) {
        if (event.message.command == cmCancel) {
            if (!allDone) {
                wasCanceled = true;
                if (currentProcess && currentProcess->isRunning()) {
                    currentProcess->cancel();
                }
                finishAll();
                clearEvent(event);
                return;
            }
        }
    } else if (event.what == evBroadcast) {
        if (event.message.command == cmTimerExpired || event.message.command == cmPeriodicUpdate) {
            if (!allDone) {
                updateUI();
            }
        }
    }

    TDialog::handleEvent(event);
}

void TBuildAllDialog::updateUI() {
    if (allDone) return;

    if (currentProcess) {
        if (currentProcess->isRunning()) {
            spinnerIndex = (spinnerIndex + 1) % 4;
            static const char spinChars[] = {'|', '/', '-', '\\'};

            double el = currentProcess->getElapsedSeconds();
            int to = currentProcess->getTimeoutSeconds();
            double rem = (to > 0) ? std::max(0.0, to - el) : 0.0;

            elapsedLabel->setText(std::format("Elapsed:   {:.1f}s  (Timeout: {}s, Rem: {:.1f}s)", el, to, rem));
            statusLabel->setText(std::format("Status:    Building with Clang... [{}]", spinChars[spinnerIndex]));

            std::string line = currentProcess->getLatestLine();
            if (!line.empty()) {
                activityLabel->setText(line);
            }
        } else if (currentProcess->isFinished()) {
            if (currentProcess->getExitCode() == 0 && !currentProcess->isTimedOut() && !currentProcess->isCanceled()) {
                succeededCount++;
            } else {
                failedCount++;
            }
            currentIndex++;
            startNextFile();
        }
    }
}

// ============================================================================
// TFormatDialog Implementation (Clang-Format Async Execution)
// ============================================================================

TFormatDialog::TFormatDialog(const std::string& filePath, const CompilerConfig& config) noexcept
    : TDialog(TRect(0, 0, 56, 14), "Format with Clang-Format"),
      TWindowInit(&TFormatDialog::initFrame),
      filePath(filePath),
      compilerConfig(config)
{
    options |= ofCentered;
    std::filesystem::path p(filePath);

    fileLabel = new TDynamicText(TRect(4, 2, 52, 3), std::format("File:      {:<32}", p.filename().string()));
    insert(fileLabel);

    statusLabel = new TDynamicText(TRect(4, 4, 52, 5), "Status:    Running clang-format... [|]", 0x1E);
    insert(statusLabel);

    elapsedLabel = new TDynamicText(TRect(4, 5, 52, 6), "Elapsed:   0.0s");
    insert(elapsedLabel);

    insert(new TStaticText(TRect(4, 7, 52, 8), "Activity:"));

    activityLabel = new TDynamicText(TRect(4, 8, 52, 9), "Applying formatting rules...", 0x1B);
    insert(activityLabel);

    cancelBtn = new TButton(TRect(20, 10, 36, 12), "~C~ancel", cmCancel, bfDefault);
    insert(cancelBtn);

    okBtn = new TButton(TRect(20, 10, 36, 12), "O~K~", cmOK, bfDefault);
    okBtn->hide();
    insert(okBtn);

    std::string formatExe = config.getClangFormatPath();
    std::string cmd = std::format("\"{}\" -i \"{}\"", formatExe, filePath);
    process = std::make_unique<AsyncProcess>(cmd, config.processTimeoutSeconds);
    process->start();

    timerId = setTimer(50, 50);
}

TFormatDialog::~TFormatDialog() {
    if (timerId) {
        killTimer(timerId);
        timerId = nullptr;
    }
    if (process && process->isRunning()) {
        process->cancel();
    }
}

void TFormatDialog::finishFormat() {
    if (hasFinished) return;
    hasFinished = true;

    if (timerId) {
        killTimer(timerId);
        timerId = nullptr;
    }

    if (process->isTimedOut()) {
        success = false;
        statusLabel->setColor(0x4F);
        statusLabel->setText("Status:    TIMEOUT! Formatting took too long");
        activityLabel->setText("Operation was aborted to avoid freeze.");
    } else if (process->isCanceled()) {
        success = false;
        statusLabel->setColor(0x4F);
        statusLabel->setText("Status:    Canceled by user");
        activityLabel->setText("Formatting aborted.");
    } else if (process->getExitCode() == 0) {
        success = true;
        statusLabel->setColor(0x1A);
        statusLabel->setText(std::format("Status:    Formatted successfully! ({:.2f}s)", process->getElapsedSeconds()));
        activityLabel->setText("File code re-aligned to Clang style.");
    } else {
        success = false;
        statusLabel->setColor(0x4E);
        statusLabel->setText("Status:    Error running clang-format");
        activityLabel->setText("Verify clang-format.exe in LLVM bin path.");
    }

    elapsedLabel->setText(std::format("Elapsed:   {:.2f} seconds", process->getElapsedSeconds()));
    cancelBtn->hide();
    okBtn->show();
    okBtn->makeDefault(True);
    okBtn->select();
    drawView();
}

void TFormatDialog::handleEvent(TEvent& event) {
    if (event.what == evCommand) {
        if (event.message.command == cmCancel) {
            if (process && process->isRunning()) {
                process->cancel();
                finishFormat();
                clearEvent(event);
                return;
            }
        }
    } else if (event.what == evBroadcast) {
        if (event.message.command == cmTimerExpired || event.message.command == cmPeriodicUpdate) {
            if (!hasFinished) {
                updateUI();
            }
        }
    }

    TDialog::handleEvent(event);
}

void TFormatDialog::updateUI() {
    if (!process) return;

    if (process->isRunning()) {
        spinnerIndex = (spinnerIndex + 1) % 4;
        static const char spinChars[] = {'|', '/', '-', '\\'};
        elapsedLabel->setText(std::format("Elapsed:   {:.1f}s", process->getElapsedSeconds()));
        statusLabel->setText(std::format("Status:    Running clang-format... [{}]", spinChars[spinnerIndex]));
    } else if (process->isFinished() && !hasFinished) {
        finishFormat();
    }
}

// ============================================================================
// TOptionsDialog Implementation
// ============================================================================

TOptionsDialog::TOptionsDialog(CompilerConfig& config) noexcept
    : TDialog(TRect(0, 0, 68, 20), "Compiler & LLVM Settings"),
      TWindowInit(&TOptionsDialog::initFrame)
{
    options |= ofCentered;

    // LLVM Path
    llvmPathInput = new TInputLine(TRect(20, 2, 64, 3), 128);
    llvmPathInput->setData(const_cast<char*>(config.llvmBinPath.c_str()));
    insert(llvmPathInput);
    insert(new TLabel(TRect(3, 2, 19, 3), "~L~LVM Bin Path:", llvmPathInput));

    // C++ Standard
    standardInput = new TInputLine(TRect(20, 4, 40, 5), 32);
    standardInput->setData(const_cast<char*>(config.cppStandard.c_str()));
    insert(standardInput);
    insert(new TLabel(TRect(3, 4, 19, 5), "~S~tandard:", standardInput));

    // Optimization
    optInput = new TInputLine(TRect(50, 4, 64, 5), 16);
    optInput->setData(const_cast<char*>(config.optLevel.c_str()));
    insert(optInput);
    insert(new TLabel(TRect(44, 4, 49, 5), "~O~pt:", optInput));

    // Warnings
    warningsInput = new TInputLine(TRect(20, 6, 64, 7), 64);
    warningsInput->setData(const_cast<char*>(config.warningFlags.c_str()));
    insert(warningsInput);
    insert(new TLabel(TRect(3, 6, 19, 7), "~W~arning Flags:", warningsInput));

    // Extra Flags
    extraFlagsInput = new TInputLine(TRect(20, 8, 64, 9), 128);
    extraFlagsInput->setData(const_cast<char*>(config.extraFlags.c_str()));
    insert(extraFlagsInput);
    insert(new TLabel(TRect(3, 8, 19, 9), "~E~xtra Flags:", extraFlagsInput));

    // Output Directory
    outDirInput = new TInputLine(TRect(20, 10, 64, 11), 128);
    outDirInput->setData(const_cast<char*>(config.outDir.c_str()));
    insert(outDirInput);
    insert(new TLabel(TRect(3, 10, 19, 11), "Out ~D~irectory:", outDirInput));

    // Process Timeout
    timeoutInput = new TInputLine(TRect(20, 12, 34, 13), 16);
    std::string toStr = std::to_string(config.processTimeoutSeconds);
    timeoutInput->setData(const_cast<char*>(toStr.c_str()));
    insert(timeoutInput);
    insert(new TLabel(TRect(3, 12, 19, 13), "~T~imeout (sec):", timeoutInput));

    // Buttons
    insert(new TButton(TRect(20, 16, 34, 18), "O~K~", cmOK, bfDefault));
    insert(new TButton(TRect(36, 16, 50, 18), "Cancel", cmCancel, bfNormal));

    selectNext(False);
}

bool TOptionsDialog::getUpdatedConfig(CompilerConfig& config) {
    char buf[256];

    if (llvmPathInput) {
        llvmPathInput->getData(buf);
        config.llvmBinPath = buf;
    }
    if (standardInput) {
        standardInput->getData(buf);
        config.cppStandard = buf;
    }
    if (optInput) {
        optInput->getData(buf);
        config.optLevel = buf;
    }
    if (warningsInput) {
        warningsInput->getData(buf);
        config.warningFlags = buf;
    }
    if (extraFlagsInput) {
        extraFlagsInput->getData(buf);
        config.extraFlags = buf;
    }
    if (outDirInput) {
        outDirInput->getData(buf);
        config.outDir = buf;
    }
    if (timeoutInput) {
        timeoutInput->getData(buf);
        try { config.processTimeoutSeconds = std::stoi(buf); } catch (...) {}
    }

    return true;
}

// ============================================================================
// TGoToLineDialog Implementation
// ============================================================================

TGoToLineDialog::TGoToLineDialog(int currentLine) noexcept
    : TDialog(TRect(0, 0, 36, 10), "Go to Line"),
      TWindowInit(&TGoToLineDialog::initFrame)
{
    options |= ofCentered;

    lineInput = new TInputLine(TRect(18, 3, 30, 4), 10);
    std::string curStr = std::to_string(currentLine);
    lineInput->setData(const_cast<char*>(curStr.c_str()));
    insert(lineInput);
    insert(new TLabel(TRect(4, 3, 17, 4), "~L~ine number:", lineInput));

    insert(new TButton(TRect(6, 6, 18, 8), "O~K~", cmOK, bfDefault));
    insert(new TButton(TRect(20, 6, 32, 8), "Cancel", cmCancel, bfNormal));

    selectNext(False);
}

int TGoToLineDialog::getLineNumber() const {
    if (!lineInput) return 1;
    char buf[32];
    lineInput->getData(buf);
    try {
        return std::stoi(buf);
    } catch (...) {
        return 1;
    }
}

// ============================================================================
// TAboutDialog Implementation
// ============================================================================

TAboutDialog::TAboutDialog() noexcept
    : TDialog(TRect(0, 0, 62, 17), "About Modern Turbo C++"),
      TWindowInit(&TAboutDialog::initFrame)
{
    options |= ofCentered;

    const char* line1 = "MODERN TURBO C++ (C++23 EDITION)";
    const char* line2 = "Next-Gen TUI IDE for Windows 11 Terminal";
    const char* line3 = "Compiler Backend: LLVM Clang";
    const char* line4 = "User Interface: Turbo Vision 2.0 (Unicode & RGB)";
    const char* line5 = "Reverse Engineering & Architecture: REA (rea.tools)";
    const char* line6 = "AI Pair Programming: Antigravity & Gemini 3.8 Flash";
    const char* line7 = "Classic 1990s Spirit - Ultra-Modern C++23 Speed!";

    insert(new TStaticText(TRect(14, 2, 52, 3), line1));
    insert(new TStaticText(TRect(11, 4, 55, 5), line2));
    insert(new TStaticText(TRect(6, 6, 56, 7), line3));
    insert(new TStaticText(TRect(6, 7, 56, 8), line4));
    insert(new TStaticText(TRect(6, 8, 56, 9), line5));
    insert(new TStaticText(TRect(6, 9, 56, 10), line6));
    insert(new TStaticText(TRect(6, 11, 56, 12), line7));

    insert(new TButton(TRect(23, 13, 39, 15), "O~K~", cmOK, bfDefault));
}

// ============================================================================
// TProgramArgsDialog Implementation
// ============================================================================

TProgramArgsDialog::TProgramArgsDialog(const std::string& currentArgs) noexcept
    : TDialog(TRect(0, 0, 56, 11), "Program Arguments"),
      TWindowInit(&TProgramArgsDialog::initFrame)
{
    options |= ofCentered;

    argsInput = new TInputLine(TRect(3, 3, 52, 4), 128);
    argsInput->setData(const_cast<char*>(currentArgs.c_str()));
    insert(argsInput);
    insert(new TLabel(TRect(3, 2, 35, 3), "Command-line ~A~rguments:", argsInput));

    insert(new TButton(TRect(14, 6, 26, 8), "O~K~", cmOK, bfDefault));
    insert(new TButton(TRect(28, 6, 40, 8), "Cancel", cmCancel, bfNormal));

    selectNext(False);
}

std::string TProgramArgsDialog::getArguments() const {
    if (!argsInput) return "";
    char buf[256];
    argsInput->getData(buf);
    return std::string(buf);
}

// ============================================================================
// TEditorOptionsDialog Implementation
// ============================================================================

TEditorOptionsDialog::TEditorOptionsDialog(EditorConfig& config) noexcept
    : TDialog(TRect(0, 0, 56, 19), "Editor Settings"),
      TWindowInit(&TEditorOptionsDialog::initFrame),
      activeConfig(config)
{
    options |= ofCentered;

    tabSizeInput = new TInputLine(TRect(16, 2, 26, 3), 4);
    std::string tabStr = std::to_string(config.tabSize);
    tabSizeInput->setData(const_cast<char*>(tabStr.c_str()));
    insert(tabSizeInput);
    insert(new TLabel(TRect(3, 2, 15, 3), "~T~ab Size:", tabSizeInput));

    checkBoxes = new TCheckBoxes(TRect(3, 4, 52, 10),
        new TSItem("~A~uto indent mode",
        new TSItem("~S~yntax highlighting (C++23)",
        new TSItem("~L~ine numbers gutter",
        new TSItem("~V~erify syntax on save",
        new TSItem("~C~lose brackets/quotes automatically", nullptr)))))
    );

    ushort flags = 0;
    if (config.autoIndent) flags |= 0x01;
    if (config.syntaxHighlight) flags |= 0x02;
    if (config.showLineNumbers) flags |= 0x04;
    if (config.autoSyntaxCheckOnSave) flags |= 0x08;
    if (config.autoCloseDelimiters) flags |= 0x10;
    checkBoxes->setData(&flags);
    insert(checkBoxes);

    themeLabel = new TDynamicText(TRect(3, 11, 32, 12), std::format("Theme: {:<16}", config.colorTheme), 0x1E);
    insert(themeLabel);
    insert(new TButton(TRect(34, 10, 52, 12), "Color ~T~hemes...", cmColorThemes, bfNormal));

    insert(new TButton(TRect(14, 15, 26, 17), "O~K~", cmOK, bfDefault));
    insert(new TButton(TRect(28, 15, 40, 17), "Cancel", cmCancel, bfNormal));

    selectNext(False);
}

void TEditorOptionsDialog::handleEvent(TEvent& event) {
    if (event.what == evCommand && event.message.command == cmColorThemes) {
        int currentIdx = 0;
        if (activeConfig.colorTheme == "ModernDark") currentIdx = 1;
        else if (activeConfig.colorTheme == "RetroGreen") currentIdx = 2;
        else if (activeConfig.colorTheme == "RetroAmber") currentIdx = 3;
        else if (activeConfig.colorTheme == "BorlandPaper") currentIdx = 4;
        else if (activeConfig.colorTheme == "DarkViolet") currentIdx = 5;
        else if (activeConfig.colorTheme == "SlateTeal") currentIdx = 6;
        else if (activeConfig.colorTheme == "ObsidianBlue") currentIdx = 7;
        else currentIdx = 0;

        TThemeDialog* dlg = new TThemeDialog(currentIdx);
        if (TProgram::deskTop && TProgram::deskTop->execView(dlg) == cmOK) {
            int sel = dlg->getSelectedThemeIndex();
            switch (sel) {
                case 1: activeConfig.colorTheme = "ModernDark"; break;
                case 2: activeConfig.colorTheme = "RetroGreen"; break;
                case 3: activeConfig.colorTheme = "RetroAmber"; break;
                case 4: activeConfig.colorTheme = "BorlandPaper"; break;
                case 5: activeConfig.colorTheme = "DarkViolet"; break;
                case 6: activeConfig.colorTheme = "SlateTeal"; break;
                case 7: activeConfig.colorTheme = "ObsidianBlue"; break;
                case 0:
                default: activeConfig.colorTheme = "BorlandBlue"; break;
            }
            if (themeLabel) {
                themeLabel->setText(std::format("Theme: {:<16}", activeConfig.colorTheme));
                themeLabel->drawView();
            }
            SyntaxHighlighter::instance().setThemeByName(activeConfig.colorTheme);
        }
        TObject::destroy(dlg);
        clearEvent(event);
        return;
    }

    TDialog::handleEvent(event);
}

bool TEditorOptionsDialog::getUpdatedConfig(EditorConfig& config) {
    if (tabSizeInput) {
        char buf[16];
        tabSizeInput->getData(buf);
        try {
            int t = std::stoi(buf);
            if (t >= 1 && t <= 16) config.tabSize = t;
        } catch (...) {}
    }

    if (checkBoxes) {
        ushort flags = 0;
        checkBoxes->getData(&flags);
        config.autoIndent = (flags & 0x01) != 0;
        config.syntaxHighlight = (flags & 0x02) != 0;
        config.showLineNumbers = (flags & 0x04) != 0;
        config.autoSyntaxCheckOnSave = (flags & 0x08) != 0;
        config.autoCloseDelimiters = (flags & 0x10) != 0;
    }

    config.colorTheme = activeConfig.colorTheme;
    return true;
}

// ============================================================================
// TDirectoriesDialog Implementation
// ============================================================================

TDirectoriesDialog::TDirectoriesDialog(CompilerConfig& config) noexcept
    : TDialog(TRect(0, 0, 64, 16), "Directories"),
      TWindowInit(&TDirectoriesDialog::initFrame)
{
    options |= ofCentered;

    includeInput = new TInputLine(TRect(18, 2, 60, 3), 256);
    includeInput->setData(const_cast<char*>(config.includeDirs.c_str()));
    insert(includeInput);
    insert(new TLabel(TRect(3, 2, 17, 3), "~I~nclude Dirs:", includeInput));

    libInput = new TInputLine(TRect(18, 5, 60, 6), 256);
    libInput->setData(const_cast<char*>(config.libDirs.c_str()));
    insert(libInput);
    insert(new TLabel(TRect(3, 5, 17, 6), "~L~ibrary Dirs:", libInput));

    outDirInput = new TInputLine(TRect(18, 8, 60, 9), 256);
    outDirInput->setData(const_cast<char*>(config.outDir.c_str()));
    insert(outDirInput);
    insert(new TLabel(TRect(3, 8, 17, 9), "~O~utput Dir:", outDirInput));

    insert(new TButton(TRect(18, 12, 30, 14), "O~K~", cmOK, bfDefault));
    insert(new TButton(TRect(32, 12, 44, 14), "Cancel", cmCancel, bfNormal));

    selectNext(False);
}

bool TDirectoriesDialog::getUpdatedConfig(CompilerConfig& config) {
    char buf[512];
    if (includeInput) {
        includeInput->getData(buf);
        config.includeDirs = buf;
    }
    if (libInput) {
        libInput->getData(buf);
        config.libDirs = buf;
    }
    if (outDirInput) {
        outDirInput->getData(buf);
        config.outDir = buf;
    }
    return true;
}

// ============================================================================
// TCompilerInfoDialog Implementation
// ============================================================================

TCompilerInfoDialog::TCompilerInfoDialog(const std::string& infoText) noexcept
    : TDialog(TRect(0, 0, 72, 18), "Compiler & Toolchain Information"),
      TWindowInit(&TCompilerInfoDialog::initFrame)
{
    options |= ofCentered;

    // Display first 8 lines of version info
    std::istringstream iss(infoText);
    std::string line;
    int y = 2;
    while (std::getline(iss, line) && y < 13) {
        if (line.ends_with('\r')) line.pop_back();
        if (line.length() > 64) line = line.substr(0, 64);
        insert(new TStaticText(TRect(4, y, 68, y + 1), line.c_str()));
        y++;
    }

    insert(new TButton(TRect(28, 14, 44, 16), "O~K~", cmOK, bfDefault));
}

// ============================================================================
// THelpContentsDialog Implementation
// ============================================================================

THelpContentsDialog::THelpContentsDialog() noexcept
    : TDialog(TRect(0, 0, 72, 20), "Modern Turbo C++ Help"),
      TWindowInit(&THelpContentsDialog::initFrame)
{
    options |= ofCentered;

    const char* lines[] = {
        "WELCOME TO MODERN TURBO C++ (C++23 EDITION)",
        "",
        "1. EDITING: Full C++23 syntax highlighting, line numbers gutter,",
        "   bracket matching, and modern editor operations.",
        "2. SYNTAX CHECK (Ctrl+F7): Runs Clang in -fsyntax-only mode",
        "   to find typos, missing semicolons, and errors in milliseconds.",
        "3. MESSAGES (F11): Displays errors and warnings. Double-click or",
        "   press Enter on any message to jump directly to the code.",
        "4. BUILD (Alt+F9 / F9): Compiles and links standalone executables.",
        "5. RUN (Ctrl+F9): Runs the compiled binary in Windows 11 Terminal.",
        "6. CLANG-FORMAT (Ctrl+K): Formats your code according to standard.",
        "7. CONFIGURATION: Preserved in turboc.json."
    };

    for (int i = 0; i < 12; ++i) {
        insert(new TStaticText(TRect(4, 2 + i, 68, 3 + i), lines[i]));
    }

    insert(new TButton(TRect(28, 16, 44, 18), "O~K~", cmOK, bfDefault));
}

// ============================================================================
// TClangHelpDialog Implementation
// ============================================================================

TClangHelpDialog::TClangHelpDialog() noexcept
    : TDialog(TRect(0, 0, 72, 19), "LLVM Clang Quick Reference"),
      TWindowInit(&TClangHelpDialog::initFrame)
{
    options |= ofCentered;

    const char* lines[] = {
        "LLVM CLANG 23.1 COMMAND-LINE OPTIONS:",
        "",
        "  -std=c++23         Enables standard ISO C++2023 features",
        "  -fsyntax-only      Validates syntax without code generation",
        "  -O0, -O1, -O2, -O3 Optimizations (O2 recommended for Release)",
        "  -Wall -Wextra      Enables recommended diagnostic warnings",
        "  -Wpedantic         Enforces strict ISO standard compliance",
        "  -I<dir>            Adds directory to include search path",
        "  -L<dir>            Adds directory to library search path",
        "  -l<library>        Links with specified library (e.g. -luser32)",
        "  -fcolor-diagnostics Colors diagnostic messages in console"
    };

    for (int i = 0; i < 11; ++i) {
        insert(new TStaticText(TRect(4, 2 + i, 68, 3 + i), lines[i]));
    }

    insert(new TButton(TRect(28, 15, 44, 17), "O~K~", cmOK, bfDefault));
}

// ============================================================================
// TWindowListDialog Implementation
// ============================================================================

TWindowListDialog::TWindowListDialog(const std::vector<std::string>& windowNames, int currentIdx) noexcept
    : TDialog(TRect(0, 0, 54, 16), "Window List"),
      TWindowInit(&TWindowListDialog::initFrame)
{
    options |= ofCentered;

    TSItem* head = nullptr;
    TSItem** curr = &head;
    for (size_t i = 0; i < windowNames.size() && i < 6; ++i) {
        *curr = new TSItem(windowNames[i].c_str(), nullptr);
        curr = &((*curr)->next);
    }

    if (head) {
        radioButtons = new TRadioButtons(TRect(3, 3, 50, 3 + std::min<int>(6, (int)windowNames.size())), head);
        ushort sel = (ushort)currentIdx;
        radioButtons->setData(&sel);
        insert(radioButtons);
    } else {
        insert(new TStaticText(TRect(4, 3, 48, 4), "No open windows."));
    }

    insert(new TButton(TRect(12, 11, 24, 13), "~S~witch", cmOK, bfDefault));
    insert(new TButton(TRect(26, 11, 38, 13), "Cancel", cmCancel, bfNormal));

    selectNext(False);
}

int TWindowListDialog::getSelectedIndex() const {
    if (!radioButtons) return 0;
    ushort sel = 0;
    radioButtons->getData(&sel);
    return (int)sel;
}

// ============================================================================
// TThemeDialog Implementation
// ============================================================================

TThemeDialog::TThemeDialog(int currentThemeIdx) noexcept
    : TDialog(TRect(0, 0, 52, 18), "Color Themes"),
      TWindowInit(&TThemeDialog::initFrame)
{
    options |= ofCentered;
    radioButtons = new TRadioButtons(TRect(4, 2, 48, 11),
        new TSItem("~C~lassic Blue (Default)",
        new TSItem("~M~odern Dark / Monokai",
        new TSItem("Retro ~G~reen CRT",
        new TSItem("Retro ~A~mber CRT",
        new TSItem("Classic ~P~aper (White)",
        new TSItem("Dark ~V~iolet (Charcoal/Purple)",
        new TSItem("Slate ~T~eal (Nord/Cyan)",
        new TSItem("Obsidian ~B~lue (Cobalt/Gold)", nullptr))))))))
    );
    ushort sel = (ushort)currentThemeIdx;
    radioButtons->setData(&sel);
    insert(radioButtons);

    insert(new TButton(TRect(12, 13, 24, 15), "O~K~", cmOK, bfDefault));
    insert(new TButton(TRect(28, 13, 40, 15), "Cancel", cmCancel, bfNormal));
    selectNext(False);
}

int TThemeDialog::getSelectedThemeIndex() const {
    if (!radioButtons) return 0;
    ushort sel = 0;
    radioButtons->getData(&sel);
    return (int)sel;
}

// ============================================================================
// TSnippetsDialog Implementation
// ============================================================================

TSnippetsDialog::TSnippetsDialog() noexcept
    : TDialog(TRect(0, 0, 56, 16), "Insert C++23 Code Snippet"),
      TWindowInit(&TSnippetsDialog::initFrame)
{
    options |= ofCentered;
    radioButtons = new TRadioButtons(TRect(4, 2, 52, 9),
        new TSItem("C++23 ~H~ello World (std::print)",
        new TSItem("~R~anges & Views Pipeline",
        new TSItem("Modern ~C~lass (Rule of 5)",
        new TSItem("~T~emplate with Concept",
        new TSItem("Try-Catch ~E~xception Block",
        new TSItem("~P~ragma once Header Guard", nullptr))))))
    );
    ushort sel = 0;
    radioButtons->setData(&sel);
    insert(radioButtons);

    insert(new TButton(TRect(14, 11, 26, 13), "~I~nsert", cmOK, bfDefault));
    insert(new TButton(TRect(30, 11, 42, 13), "Cancel", cmCancel, bfNormal));
    selectNext(False);
}

std::string TSnippetsDialog::getSelectedSnippetText() const {
    ushort sel = 0;
    if (radioButtons) radioButtons->getData(&sel);

    switch (sel) {
        case 0:
            return
                "#include <iostream>\n"
                "#include <format>\n"
                "#include <print>\n\n"
                "int main(int argc, char* argv[]) {\n"
                "    std::println(\"Hello, Modern C++23 World!\");\n"
                "    return 0;\n"
                "}\n";
        case 1:
            return
                "#include <iostream>\n"
                "#include <vector>\n"
                "#include <ranges>\n\n"
                "void processPipeline() {\n"
                "    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};\n"
                "    auto result = numbers\n"
                "        | std::views::filter([](int n) { return n % 2 == 0; })\n"
                "        | std::views::transform([](int n) { return n * 10; });\n\n"
                "    for (int n : result) {\n"
                "        std::cout << n << \" \";\n"
                "    }\n"
                "    std::cout << \"\\n\";\n"
                "}\n";
        case 2:
            return
                "class ResourceHandle {\n"
                "public:\n"
                "    ResourceHandle() = default;\n"
                "    ~ResourceHandle() = default;\n\n"
                "    ResourceHandle(const ResourceHandle&) = default;\n"
                "    ResourceHandle& operator=(const ResourceHandle&) = default;\n\n"
                "    ResourceHandle(ResourceHandle&&) noexcept = default;\n"
                "    ResourceHandle& operator=(ResourceHandle&&) noexcept = default;\n\n"
                "private:\n"
                "    int id = 0;\n"
                "};\n";
        case 3:
            return
                "#include <concepts>\n"
                "#include <iostream>\n\n"
                "template <typename T>\n"
                "concept Numeric = std::integral<T> || std::floating_point<T>;\n\n"
                "template <Numeric T>\n"
                "T addNumbers(T a, T b) {\n"
                "    return a + b;\n"
                "}\n";
        case 4:
            return
                "try {\n"
                "    // Protected operations\n"
                "} catch (const std::exception& ex) {\n"
                "    std::cerr << \"Standard exception caught: \" << ex.what() << \"\\n\";\n"
                "} catch (...) {\n"
                "    std::cerr << \"Unknown exception caught.\\n\";\n"
                "}\n";
        case 5:
        default:
            return
                "#pragma once\n\n"
                "#include <string>\n"
                "#include <vector>\n"
                "#include <memory>\n\n"
                "namespace MyProject {\n\n"
                "} // namespace MyProject\n";
    }
}

// ============================================================================
// TFileStatsDialog Implementation
// ============================================================================

TFileStatsDialog::TFileStatsDialog(const std::string& fileName, const FileStats& stats) noexcept
    : TDialog(TRect(0, 0, 58, 17), "File Statistics"),
      TWindowInit(&TFileStatsDialog::initFrame)
{
    options |= ofCentered;

    std::filesystem::path p(fileName);
    std::string nameStr = p.filename().string();
    if (nameStr.empty()) nameStr = "NONAME.CPP";

    insert(new TStaticText(TRect(4, 2, 54, 3), std::format("File: {:<40}", nameStr).c_str()));
    insert(new TStaticText(TRect(4, 3, 54, 4), "--------------------------------------------------"));

    insert(new TStaticText(TRect(4, 4, 30, 5), std::format("Total Lines:       {:>8}", stats.totalLines).c_str()));
    insert(new TStaticText(TRect(30, 4, 54, 5), std::format("Code Lines:    {:>8}", stats.codeLines).c_str()));

    insert(new TStaticText(TRect(4, 5, 30, 6), std::format("Blank Lines:       {:>8}", stats.blankLines).c_str()));
    insert(new TStaticText(TRect(30, 5, 54, 6), std::format("Comment Lines: {:>8}", stats.commentLines).c_str()));

    insert(new TStaticText(TRect(4, 7, 30, 8), std::format("Total Characters:  {:>8}", stats.charCount).c_str()));
    insert(new TStaticText(TRect(30, 7, 54, 8), std::format("Without Spaces:{:>8}", stats.charNoSpaceCount).c_str()));

    insert(new TStaticText(TRect(4, 8, 30, 9), std::format("File Size (bytes): {:>8}", stats.fileSize).c_str()));
    insert(new TStaticText(TRect(30, 8, 54, 9), std::format("Functions:     {:>8}", stats.functionCount).c_str()));

    insert(new TButton(TRect(22, 12, 36, 14), "O~K~", cmOK, bfDefault));
}

// ============================================================================
// THexViewDialog Implementation
// ============================================================================

THexViewDialog::THexViewDialog(const std::string& filePath) noexcept
    : TDialog(TRect(0, 0, 78, 22), "Hex Viewer / Binary Inspector"),
      TWindowInit(&THexViewDialog::initFrame)
{
    options |= ofCentered;

    std::filesystem::path p(filePath);
    std::string title = std::format("File: {} ({})", p.filename().string(), std::filesystem::exists(p) ? std::format("{} bytes", std::filesystem::file_size(p)) : "0 bytes");
    insert(new TStaticText(TRect(3, 1, 75, 2), title.c_str()));
    insert(new TStaticText(TRect(3, 2, 75, 3), "Offset(h)  00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  Decoded Text"));
    insert(new TStaticText(TRect(3, 3, 75, 4), "---------  -----------------------  -----------------------  ----------------"));

    std::ifstream file(filePath, std::ios::binary);
    std::vector<unsigned char> data(256, 0);
    size_t bytesRead = 0;
    if (file.is_open()) {
        file.read(reinterpret_cast<char*>(data.data()), 256);
        bytesRead = file.gcount();
    }

    for (size_t row = 0; row < 12; ++row) {
        size_t offset = row * 16;
        if (offset >= bytesRead && bytesRead > 0) break;

        std::string hexPart1, hexPart2, asciiPart;
        for (size_t col = 0; col < 16; ++col) {
            size_t idx = offset + col;
            if (idx < bytesRead) {
                unsigned char byte = data[idx];
                std::string bStr = std::format("{:02X} ", byte);
                if (col < 8) hexPart1 += bStr;
                else hexPart2 += bStr;
                asciiPart += (byte >= 32 && byte <= 126) ? (char)byte : '.';
            } else {
                if (col < 8) hexPart1 += "   ";
                else hexPart2 += "   ";
                asciiPart += " ";
            }
        }

        std::string line = std::format("{:08X}   {} {} {}", offset, hexPart1, hexPart2, asciiPart);
        insert(new TStaticText(TRect(3, 4 + (int)row, 75, 5 + (int)row), line.c_str()));
    }

    insert(new TButton(TRect(32, 18, 46, 20), "~C~lose", cmOK, bfDefault));
}

} // namespace TurboC
