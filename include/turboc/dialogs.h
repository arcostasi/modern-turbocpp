#pragma once

#include "turboc/compiler.h"
#include "turboc/config.h"
#include "turboc/process.h"
#include <memory>
#include <vector>

#define Uses_TDialog
#define Uses_TRect
#define Uses_TButton
#define Uses_TStaticText
#define Uses_TInputLine
#define Uses_TLabel
#define Uses_TRadioButtons
#define Uses_TCheckBoxes
#define Uses_TSItem
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TProgram
#define Uses_TDeskTop
#include <tvision/tv.h>

namespace TurboC {

// Dynamic text view with custom color and runtime update capability
class TDynamicText : public TView {
public:
    TDynamicText(const TRect& bounds, const std::string& initialText = "", TColorAttr color = 0) noexcept;

    void setText(const std::string& newText);
    void setColor(TColorAttr color);

    virtual void draw() override;

    std::string text;
    TColorAttr customColor = 0;
};

// Classic-style progress bar with percentage indicator
class TProgressBar : public TView {
public:
    TProgressBar(const TRect& bounds) noexcept;

    void setPercent(int p);

    virtual void draw() override;

    int percent = 0;
};

// Classic Turbo C "Compiling" modal dialog with real-time execution feedback & timeout
class TCompileDialog : public TDialog {
public:
    TCompileDialog(const std::string& sourceFile,
                   CompileMode mode,
                   const CompilerConfig& config,
                   const std::string& customOutPath = "") noexcept;
    virtual ~TCompileDialog();

    virtual void handleEvent(TEvent& event) override;

    const CompilationResult& getResult() const { return result; }

private:
    void updateUI();
    void finishProcess();

    std::string sourcePath;
    CompileMode compileMode;
    CompilerConfig compilerConfig;
    std::string targetPath;

    std::unique_ptr<AsyncProcess> process;
    CompilationResult result;

    TDynamicText* statusLabel = nullptr;
    TDynamicText* mainFileLabel = nullptr;
    TDynamicText* targetLabel = nullptr;
    TDynamicText* elapsedLabel = nullptr;
    TDynamicText* diagnosticsLabel = nullptr;
    TDynamicText* activityLabel = nullptr;

    TButton* cancelBtn = nullptr;
    TButton* viewMsgsBtn = nullptr;
    TButton* okBtn = nullptr;

    int spinnerIndex = 0;
    bool hasFinished = false;
    TTimerId timerId = nullptr;
};

// Batch Build All modal dialog with real-time progress bar and cancellation
class TBuildAllDialog : public TDialog {
public:
    TBuildAllDialog(const std::vector<std::string>& files,
                    const CompilerConfig& config) noexcept;
    virtual ~TBuildAllDialog();

    virtual void handleEvent(TEvent& event) override;

    int getCompiledCount() const { return succeededCount + failedCount; }
    int getSucceededCount() const { return succeededCount; }
    int getFailedCount() const { return failedCount; }

private:
    void startNextFile();
    void updateUI();
    void finishAll();

    std::vector<std::string> fileList;
    CompilerConfig compilerConfig;
    size_t currentIndex = 0;

    std::unique_ptr<AsyncProcess> currentProcess;

    int succeededCount = 0;
    int failedCount = 0;
    bool allDone = false;
    bool wasCanceled = false;
    int spinnerIndex = 0;
    TTimerId timerId = nullptr;

    TDynamicText* overallLabel = nullptr;
    TProgressBar* progressBar = nullptr;
    TDynamicText* currentFileLabel = nullptr;
    TDynamicText* statusLabel = nullptr;
    TDynamicText* elapsedLabel = nullptr;
    TDynamicText* activityLabel = nullptr;

    TButton* cancelBtn = nullptr;
    TButton* okBtn = nullptr;
};

// Modal progress dialog for formatting source files with clang-format
class TFormatDialog : public TDialog {
public:
    TFormatDialog(const std::string& filePath, const CompilerConfig& config) noexcept;
    virtual ~TFormatDialog();

    virtual void handleEvent(TEvent& event) override;

    bool isSuccess() const { return success; }

private:
    void updateUI();
    void finishFormat();

    std::string filePath;
    CompilerConfig compilerConfig;
    std::unique_ptr<AsyncProcess> process;
    bool success = false;
    bool hasFinished = false;
    int spinnerIndex = 0;
    TTimerId timerId = nullptr;

    TDynamicText* statusLabel = nullptr;
    TDynamicText* fileLabel = nullptr;
    TDynamicText* elapsedLabel = nullptr;
    TDynamicText* activityLabel = nullptr;

    TButton* cancelBtn = nullptr;
    TButton* okBtn = nullptr;
};

// Compiler & LLVM Configuration Dialog
class TOptionsDialog : public TDialog {
public:
    TOptionsDialog(CompilerConfig& config) noexcept;

    bool getUpdatedConfig(CompilerConfig& config);

private:
    TInputLine* llvmPathInput = nullptr;
    TInputLine* standardInput = nullptr;
    TInputLine* optInput = nullptr;
    TInputLine* warningsInput = nullptr;
    TInputLine* extraFlagsInput = nullptr;
    TInputLine* outDirInput = nullptr;
    TInputLine* timeoutInput = nullptr;
};

// Go to line number dialog
class TGoToLineDialog : public TDialog {
public:
    TGoToLineDialog(int currentLine) noexcept;

    int getLineNumber() const;

private:
    TInputLine* lineInput = nullptr;
};

// About Modern Turbo C++ dialog
class TAboutDialog : public TDialog {
public:
    TAboutDialog() noexcept;
};

// Program Arguments Dialog
class TProgramArgsDialog : public TDialog {
public:
    TProgramArgsDialog(const std::string& currentArgs) noexcept;
    std::string getArguments() const;

private:
    TInputLine* argsInput = nullptr;
};

// Editor Settings Dialog
class TEditorOptionsDialog : public TDialog {
public:
    TEditorOptionsDialog(EditorConfig& config) noexcept;
    virtual void handleEvent(TEvent& event) override;
    bool getUpdatedConfig(EditorConfig& config);

private:
    TInputLine* tabSizeInput = nullptr;
    TCheckBoxes* checkBoxes = nullptr;
    TDynamicText* themeLabel = nullptr;
    EditorConfig& activeConfig;
};

// Directories Dialog (Include & Lib paths)
class TDirectoriesDialog : public TDialog {
public:
    TDirectoriesDialog(CompilerConfig& config) noexcept;
    bool getUpdatedConfig(CompilerConfig& config);

private:
    TInputLine* includeInput = nullptr;
    TInputLine* libInput = nullptr;
    TInputLine* outDirInput = nullptr;
};

// Compiler Info Dialog
class TCompilerInfoDialog : public TDialog {
public:
    TCompilerInfoDialog(const std::string& infoText) noexcept;
};

// Help Contents Dialog
class THelpContentsDialog : public TDialog {
public:
    THelpContentsDialog() noexcept;
};

// Clang Reference Dialog
class TClangHelpDialog : public TDialog {
public:
    TClangHelpDialog() noexcept;
};

// Window List Dialog
class TWindowListDialog : public TDialog {
public:
    TWindowListDialog(const std::vector<std::string>& windowNames, int currentIdx) noexcept;
    int getSelectedIndex() const;

private:
    TRadioButtons* radioButtons = nullptr;
};

// Color Themes Dialog
class TThemeDialog : public TDialog {
public:
    TThemeDialog(int currentThemeIdx) noexcept;
    int getSelectedThemeIndex() const;

private:
    TRadioButtons* radioButtons = nullptr;
};

// C++23 Code Snippets Selection Dialog
class TSnippetsDialog : public TDialog {
public:
    TSnippetsDialog() noexcept;
    std::string getSelectedSnippetText() const;

private:
    TRadioButtons* radioButtons = nullptr;
};

struct FileStats {
    int totalLines = 0;
    int blankLines = 0;
    int commentLines = 0;
    int codeLines = 0;
    size_t charCount = 0;
    size_t charNoSpaceCount = 0;
    size_t fileSize = 0;
    int functionCount = 0;
};

// File Statistics Dialog
class TFileStatsDialog : public TDialog {
public:
    TFileStatsDialog(const std::string& fileName, const FileStats& stats) noexcept;
};

// Hex Viewer Dialog for Binaries and Source Files
class THexViewDialog : public TDialog {
public:
    THexViewDialog(const std::string& filePath) noexcept;
};

} // namespace TurboC
