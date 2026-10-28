#include "turboc/app.h"
#include "turboc/commands.h"
#include "turboc/dialogs.h"
#include <cstdarg>
#include <format>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cstdlib>

#define Uses_TFileDialog
#define Uses_TChDirDialog
#define Uses_TObject
#define Uses_THistory
#define Uses_TEditWindow
#define Uses_TWindow
#define Uses_TBackground

#include <tvision/tv.h>

namespace TurboC {

static ushort execDialog(TDialog* d, void* data) {
    TView* p = TProgram::application->validView(d);
    if (!p) return cmCancel;

    if (data) p->setData(data);
    ushort result = TProgram::deskTop->execView(p);
    if (result != cmCancel && data) p->getData(data);
    TObject::destroy(p);
    return result;
}

static TDialog* createFindDialog() {
    TDialog* d = new TDialog(TRect(0, 0, 42, 12), "Find");
    d->options |= ofCentered;

    TInputLine* control = new TInputLine(TRect(3, 3, 35, 4), 80);
    d->insert(control);
    d->insert(new TLabel(TRect(2, 2, 17, 3), "~T~ext to find", control));
    d->insert(new THistory(TRect(35, 3, 38, 4), control, 10));

    d->insert(new TCheckBoxes(TRect(3, 5, 38, 7),
        new TSItem("~C~ase sensitive",
        new TSItem("~W~hole words only", nullptr))));

    d->insert(new TButton(TRect(16, 9, 26, 11), "O~K~", cmOK, bfDefault));
    d->insert(new TButton(TRect(28, 9, 38, 11), "Cancel", cmCancel, bfNormal));

    d->selectNext(False);
    return d;
}

static TDialog* createReplaceDialog() {
    TDialog* d = new TDialog(TRect(0, 0, 44, 16), "Replace");
    d->options |= ofCentered;

    TInputLine* control = new TInputLine(TRect(3, 3, 37, 4), 80);
    d->insert(control);
    d->insert(new TLabel(TRect(2, 2, 17, 3), "~T~ext to find", control));
    d->insert(new THistory(TRect(37, 3, 40, 4), control, 10));

    control = new TInputLine(TRect(3, 6, 37, 7), 80);
    d->insert(control);
    d->insert(new TLabel(TRect(2, 5, 17, 6), "~N~ew text", control));
    d->insert(new THistory(TRect(37, 6, 40, 7), control, 11));

    d->insert(new TCheckBoxes(TRect(3, 8, 40, 12),
        new TSItem("~C~ase sensitive",
        new TSItem("~W~hole words only",
        new TSItem("~P~rompt on replace",
        new TSItem("~R~eplace all", nullptr))))));

    d->insert(new TButton(TRect(18, 13, 28, 15), "O~K~", cmOK, bfDefault));
    d->insert(new TButton(TRect(30, 13, 40, 15), "Cancel", cmCancel, bfNormal));

    d->selectNext(False);
    return d;
}

static ushort doEditDialog(int dialog, ...) {
    std::va_list ap;
    va_start(ap, dialog);

    ushort result = cmCancel;

    switch (dialog) {
        case edFind: {
            void* d = va_arg(ap, void*);
            result = execDialog(createFindDialog(), d);
            break;
        }
        case edReplace: {
            void* d = va_arg(ap, void*);
            result = execDialog(createReplaceDialog(), d);
            break;
        }
        case edSearchFailed: {
            result = messageBox("Search string not found.", mfError | mfOKButton);
            break;
        }
        case edReplacePrompt: {
            result = messageBox("Replace this occurrence?", mfConfirmation | mfYesNoCancel);
            break;
        }
        case edSaveAs: {
            char* fn = va_arg(ap, char*);
            char buffer[MAXPATH] = "";
            if (fn && *fn) {
                std::strncpy(buffer, fn, sizeof(buffer) - 1);
            } else {
                std::strncpy(buffer, "*.*", sizeof(buffer) - 1);
            }
            result = execDialog(new TFileDialog("*.*", "Save File As", "~N~ame", fdOKButton, 101), buffer);
            if (result != cmCancel && fn) {
                std::strncpy(fn, buffer, MAXPATH - 1);
                fn[MAXPATH - 1] = '\0';
            }
            break;
        }
        case edSaveModify: {
            char* fn = va_arg(ap, char*);
            std::string msg = std::format("{} has been modified. Save changes?", fn ? fn : "File");
            result = messageBox(msg.c_str(), mfConfirmation | mfYesNoCancel);
            break;
        }
        case edSaveUntitled: {
            result = messageBox("Save untitled file?", mfConfirmation | mfYesNoCancel);
            break;
        }
        case edReadError: {
            char* fn = va_arg(ap, char*);
            std::string msg = std::format("Error reading file {}.", fn ? fn : "");
            result = messageBox(msg.c_str(), mfError | mfOKButton);
            break;
        }
        case edWriteError: {
            char* fn = va_arg(ap, char*);
            std::string msg = std::format("Error writing file {}.", fn ? fn : "");
            result = messageBox(msg.c_str(), mfError | mfOKButton);
            break;
        }
        case edCreateError: {
            char* fn = va_arg(ap, char*);
            std::string msg = std::format("Error creating file {}.", fn ? fn : "");
            result = messageBox(msg.c_str(), mfError | mfOKButton);
            break;
        }
        case edOutOfMemory: {
            result = messageBox("Not enough memory for this operation.", mfError | mfOKButton);
            break;
        }
        default:
            result = cmCancel;
            break;
    }

    va_end(ap);
    return result;
}

TTurboCApp::TTurboCApp(int argc, char** argv)
    : TProgInit(TTurboCApp::initStatusLine,
                TTurboCApp::initMenuBar,
                TTurboCApp::initDeskTop),
      TApplication()
{
    TEditor::editorDialog = doEditDialog;
    SyntaxHighlighter::instance().setThemeByName(ConfigManager::instance().editor.colorTheme);

    // Enable standard editor commands
    TCommandSet ts;
    ts.enableCmd(cmSave);
    ts.enableCmd(cmSaveAs);
    ts.enableCmd(cmCut);
    ts.enableCmd(cmCopy);
    ts.enableCmd(cmPaste);
    ts.enableCmd(cmClear);
    ts.enableCmd(cmUndo);
    ts.enableCmd(cmFind);
    ts.enableCmd(cmReplace);
    ts.enableCmd(cmSearchAgain);
    disableCommands(ts);

    // Create system clipboard window (initially hidden)
    TRect clipRect(0, 0, 60, 16);
    TEditWindow* clipWindow = new TEditWindow(clipRect, "", wnNoNumber);
    clipWindow->hide();
    TEditor::clipboard = clipWindow->editor;
    deskTop->insert(clipWindow);

    // Create bottom Message Window initially docked or minimized
    TRect desk = deskTop->getExtent();
    TRect msgRect(0, desk.b.y - 7, desk.b.x, desk.b.y);
    messageWindow = new TMessageWindow(msgRect);
    messageWindow->hide();
    deskTop->insert(messageWindow);

    // Open command line files or open a default new file
    bool openedAny = false;
    for (int i = 1; i < argc; ++i) {
        if (argv[i] && argv[i][0] != '-') {
            openEditor(argv[i], True);
            openedAny = true;
        }
    }

    if (!openedAny) {
        fileNew();
    }
}

TTurboCApp::~TTurboCApp() {
    ConfigManager::instance().saveToFile("turboc.json");
}

TDeskTop* TTurboCApp::initDeskTop(TRect r) {
    r.a.y++;
    r.b.y--;
    return new TDeskTop(r);
}

TMenuBar* TTurboCApp::initMenuBar(TRect r) {
    r.b.y = r.a.y + 1;

    // System menu
    TSubMenu& subSystem = *new TSubMenu("~\xF0~", kbNoKey) +
        *new TMenuItem("~A~bout Modern Turbo C++...", cmAboutBox, kbNoKey, hcNoContext, "") +
        newLine() +
        *new TMenuItem("~R~efresh screen", cmRefreshScreen, kbNoKey, hcNoContext, "") +
        *new TMenuItem("~C~lear desktop", cmCloseAll, kbNoKey, hcNoContext, "");

    // File menu
    TSubMenu& subFile = *new TSubMenu("~F~ile", kbAltF) +
        *new TMenuItem("~N~ew", cmNew, kbCtrlN, hcNoContext, "Ctrl-N") +
        *new TMenuItem("~O~pen...", cmOpen, kbF3, hcNoContext, "F3") +
        *new TMenuItem("~S~ave", cmSave, kbF2, hcNoContext, "F2") +
        *new TMenuItem("Save ~a~s...", cmSaveAs, kbNoKey, hcNoContext, "") +
        *new TMenuItem("Save al~l~", cmSaveAllFiles, kbNoKey, hcNoContext, "") +
        newLine() +
        *new TMenuItem("~H~ex view...", cmHexView, kbNoKey, hcNoContext, "") +
        *new TMenuItem("~C~hange dir...", cmChangeDir, kbNoKey, hcNoContext, "") +
        *new TMenuItem("~D~OS shell", cmDosShell, kbNoKey, hcNoContext, "") +
        newLine() +
        *new TMenuItem("E~x~it", cmQuit, kbAltX, hcNoContext, "Alt-X");

    // Edit menu
    TSubMenu& subEdit = *new TSubMenu("~E~dit", kbAltE) +
        *new TMenuItem("~U~ndo", cmUndo, kbAltBack, hcNoContext, "Alt-BkSp") +
        newLine() +
        *new TMenuItem("Cu~t~", cmCut, kbShiftDel, hcNoContext, "Shift-Del") +
        *new TMenuItem("~C~opy", cmCopy, kbCtrlIns, hcNoContext, "Ctrl-Ins") +
        *new TMenuItem("~P~aste", cmPaste, kbShiftIns, hcNoContext, "Shift-Ins") +
        *new TMenuItem("~C~lear", cmClear, kbCtrlDel, hcNoContext, "Ctrl-Del") +
        *new TMenuItem("Select ~A~ll", cmSelectAll, kbCtrlA, hcNoContext, "Ctrl-A") +
        newLine() +
        *new TMenuItem("Insert ~s~nippet...", cmInsertSnippet, kbCtrlI, hcNoContext, "Ctrl-I") +
        *new TMenuItem("Show ~C~lipboard", cmShowClipboard, kbNoKey, hcNoContext, "") +
        *new TMenuItem("Format with Clang-~F~ormat", cmClangFormat, kbCtrlK, hcNoContext, "Ctrl-K");

    // Search menu
    TSubMenu& subSearch = *new TSubMenu("~S~earch", kbAltS) +
        *new TMenuItem("~F~ind...", cmFind, kbCtrlF, hcNoContext, "Ctrl-F") +
        *new TMenuItem("~R~eplace...", cmReplace, kbCtrlR, hcNoContext, "Ctrl-R") +
        *new TMenuItem("~S~earch again", cmSearchAgain, kbNoKey, hcNoContext, "") +
        newLine() +
        *new TMenuItem("~G~o to line...", cmGoToLine, kbAltG, hcNoContext, "Alt-G") +
        *new TMenuItem("~P~revious error", cmPrevError, kbAltF7, hcNoContext, "Alt-F7") +
        *new TMenuItem("~N~ext error", cmNextError, kbAltF8, hcNoContext, "Alt-F8") +
        *new TMenuItem("~L~ocate function...", cmLocateFunction, kbNoKey, hcNoContext, "") +
        *new TMenuItem("File ~s~tatistics...", cmFileStats, kbNoKey, hcNoContext, "");

    // Run menu
    TSubMenu& subRun = *new TSubMenu("~R~un", kbAltR) +
        *new TMenuItem("~R~un", cmRunProgram, kbCtrlF9, hcNoContext, "Ctrl-F9") +
        *new TMenuItem("Program ~a~rguments...", cmProgramArgs, kbNoKey, hcNoContext, "") +
        *new TMenuItem("C~l~ean artifacts", cmCleanArtifacts, kbNoKey, hcNoContext, "");

    // Compile menu
    TSubMenu& subCompile = *new TSubMenu("~C~ompile", kbAltC) +
        *new TMenuItem("Compile to ~O~BJ", cmCompileObj, kbAltF9, hcNoContext, "Alt-F9") +
        *new TMenuItem("~M~ake / Build EXE", cmMake, kbF9, hcNoContext, "F9") +
        *new TMenuItem("~S~yntax check", cmSyntaxCheck, kbCtrlF7, hcNoContext, "Ctrl-F7") +
        *new TMenuItem("~B~uild all", cmBuildAll, kbNoKey, hcNoContext, "") +
        newLine() +
        *new TMenuItem("~M~essages window", cmMessages, kbF11, hcNoContext, "F11") +
        *new TMenuItem("View ~A~ssembly", cmViewAssembly, kbAltF10, hcNoContext, "Alt-F10") +
        *new TMenuItem("Compiler ~I~nformation...", cmCompilerInfo, kbNoKey, hcNoContext, "");

    // Options menu
    TSubMenu& subOptions = *new TSubMenu("~O~ptions", kbAltO) +
        *new TMenuItem("~C~ompiler & LLVM...", cmCompilerOptions, kbNoKey, hcNoContext, "") +
        *new TMenuItem("~E~ditor settings...", cmEditorOptions, kbNoKey, hcNoContext, "") +
        *new TMenuItem("~D~irectories...", cmDirectories, kbNoKey, hcNoContext, "") +
        newLine() +
        *new TMenuItem("Color ~t~hemes...", cmColorThemes, kbShiftF10, hcNoContext, "Shift-F10") +
        *new TMenuItem("~T~oggle line numbers", cmToggleGutter, kbCtrlF8, hcNoContext, "Ctrl-F8") +
        *new TMenuItem("~S~ave options", cmSaveOptions, kbNoKey, hcNoContext, "");

    // Window menu
    TSubMenu& subWindow = *new TSubMenu("~W~indow", kbAltW) +
        *new TMenuItem("~S~ize/move", cmResize, kbCtrlF5, hcNoContext, "Ctrl-F5") +
        *new TMenuItem("~Z~oom", cmZoom, kbF5, hcNoContext, "F5") +
        *new TMenuItem("~T~ile", cmTile, kbNoKey, hcNoContext, "") +
        *new TMenuItem("C~a~scade", cmCascade, kbNoKey, hcNoContext, "") +
        *new TMenuItem("~N~ext", cmNext, kbF6, hcNoContext, "F6") +
        *new TMenuItem("~P~revious", cmPrev, kbShiftF6, hcNoContext, "Shift-F6") +
        *new TMenuItem("~C~lose", cmClose, kbAltF3, hcNoContext, "Alt-F3") +
        *new TMenuItem("Close ~a~ll", cmCloseAll, kbNoKey, hcNoContext, "") +
        newLine() +
        *new TMenuItem("~M~essage window", cmMessages, kbF11, hcNoContext, "F11") +
        *new TMenuItem("Window ~L~ist...", cmWindowList, kbAlt0, hcNoContext, "Alt-0");

    // Help menu
    TSubMenu& subHelp = *new TSubMenu("~H~elp", kbAltH) +
        *new TMenuItem("Help ~C~ontents", cmHelpContents, kbNoKey, hcNoContext, "") +
        *new TMenuItem("~K~eyboard shortcuts", cmShortcutsHelp, kbNoKey, hcNoContext, "") +
        *new TMenuItem("~L~LVM Clang Reference", cmClangHelp, kbNoKey, hcNoContext, "") +
        newLine() +
        *new TMenuItem("~A~bout Modern Turbo C++", cmAboutBox, kbNoKey, hcNoContext, "");

    return new TMenuBar(r,
        subSystem +
        subFile +
        subEdit +
        subSearch +
        subRun +
        subCompile +
        subOptions +
        subWindow +
        subHelp
    );
}

TStatusLine* TTurboCApp::initStatusLine(TRect r) {
    r.a.y = r.b.y - 1;
    return new TStatusLine(r,
        *new TStatusDef(0, 0xFFFF) +
            *new TStatusItem("~F1~ Help", kbF1, cmHelp) +
            *new TStatusItem("~F2~ Save", kbF2, cmSave) +
            *new TStatusItem("~F3~ Open", kbF3, cmOpen) +
            *new TStatusItem("~Alt-F9~ Compile", kbAltF9, cmCompile) +
            *new TStatusItem("~F9~ Make", kbF9, cmMake) +
            *new TStatusItem("~Ctrl-F7~ Syntax", kbCtrlF7, cmSyntaxCheck) +
            *new TStatusItem("~F11~ Msgs", kbF11, cmMessages) +
            *new TStatusItem("~Ctrl-F9~ Run", kbCtrlF9, cmRunProgram) +
            *new TStatusItem("~Alt-X~ Exit", kbAltX, cmQuit) +
            *new TStatusItem(nullptr, kbCtrlN, cmNew) +
            *new TStatusItem(nullptr, kbAltF3, cmClose) +
            *new TStatusItem(nullptr, kbAltF7, cmPrevError) +
            *new TStatusItem(nullptr, kbAltF8, cmNextError) +
            *new TStatusItem(nullptr, kbAltG, cmGoToLine)
    );
}

TTurboEditWindow* TTurboCApp::openEditor(const char* fileName, Boolean visible) {
    TRect r = deskTop->getExtent();
    // Leave room at bottom if message window is visible
    if (messageWindow && (messageWindow->state & sfVisible)) {
        r.b.y = messageWindow->origin.y;
    }

    TTurboEditWindow* p = new TTurboEditWindow(r, fileName, windowCounter++);
    if (!visible) p->hide();
    deskTop->insert(p);

    // If opening an existing file, run syntax check
    if (fileName && std::filesystem::exists(fileName)) {
        CompilationResult res = LLVMCompiler::instance().checkSyntax(fileName, ConfigManager::instance().compiler);
        p->editor->setDiagnostics(res.diagnostics);
        if (messageWindow && (!res.diagnostics.empty())) {
            messageWindow->setDiagnostics(res.diagnostics);
        }
    }

    return p;
}

TTurboEditWindow* TTurboCApp::getActiveEditor() {
    TView* v = deskTop->current;
    if (!v) return nullptr;
    TTurboEditWindow* w = dynamic_cast<TTurboEditWindow*>(v);
    return w;
}

void TTurboCApp::fileOpen() {
    char fileName[MAXPATH] = "*.*";
    if (execDialog(new TFileDialog("*.*", "Open File", "~N~ame", fdOpenButton, 100), fileName) != cmCancel) {
        openEditor(fileName, True);
    }
}

void TTurboCApp::fileNew() {
    std::string defaultName = std::format("NONAME{:02d}.CPP", windowCounter);
    openEditor(defaultName.c_str(), True);
}

void TTurboCApp::fileSave() {
    TTurboEditWindow* w = getActiveEditor();
    if (w && w->editor) {
        std::string fn = w->editor->fileName;
        if (fn.empty() || fn.starts_with("NONAME") || fn.starts_with("noname")) {
            fileSaveAs();
            return;
        }
        w->editor->save();
        if (w->frame) w->frame->drawView();
        if (ConfigManager::instance().editor.autoSyntaxCheckOnSave) {
            runSyntaxCheck();
        }
    }
}

void TTurboCApp::fileSaveAs() {
    TTurboEditWindow* w = getActiveEditor();
    if (w && w->editor) {
        if (w->editor->saveAs()) {
            if (w->frame) w->frame->drawView();
            if (ConfigManager::instance().editor.autoSyntaxCheckOnSave) {
                runSyntaxCheck();
            }
        }
    }
}

void TTurboCApp::fileSaveAll() {
    deskTop->forEach([](TView* v, void*) {
        TTurboEditWindow* w = dynamic_cast<TTurboEditWindow*>(v);
        if (w && w->editor && w->editor->modified) {
            std::string fn = w->editor->fileName;
            if (fn.empty() || fn.starts_with("NONAME") || fn.starts_with("noname")) {
                w->editor->saveAs();
            } else {
                w->editor->save();
            }
            if (w->frame) w->frame->drawView();
        }
    }, nullptr);
}

void TTurboCApp::dosShell() {
    suspend();
    std::cout << "\n=======================================================\n";
    std::cout << " Modern Turbo C++ - Command Prompt Shell\n";
    std::cout << " Type 'exit' and press Enter to return to Modern Turbo C++...\n";
    std::cout << "=======================================================\n\n";
    const char* comspec = std::getenv("COMSPEC");
    if (!comspec || !*comspec) comspec = "cmd.exe";
    std::system(comspec);
    resume();
    redraw();
}

void TTurboCApp::toggleLineNumbers() {
    auto& cfg = ConfigManager::instance().editor;
    cfg.showLineNumbers = !cfg.showLineNumbers;
    deskTop->forEach([](TView* v, void* ctx) {
        TTurboEditWindow* w = dynamic_cast<TTurboEditWindow*>(v);
        if (w && w->editor) {
            bool show = *static_cast<bool*>(ctx);
            if (w->gutter) {
                if (show) {
                    w->gutter->show();
                    TRect r = w->editor->getBounds();
                    r.a.x = 7;
                    w->editor->locate(r);
                } else {
                    w->gutter->hide();
                    TRect r = w->editor->getBounds();
                    r.a.x = 1;
                    w->editor->locate(r);
                }
            }
            w->editor->drawView();
        }
    }, &cfg.showLineNumbers);
}

void TTurboCApp::changeDir() {
    execDialog(new TChDirDialog(cdNormal, 0), nullptr);
}

void TTurboCApp::toggleMessages() {
    if (!messageWindow) return;
    if (messageWindow->state & sfVisible) {
        messageWindow->hide();
    } else {
        messageWindow->show();
        messageWindow->select();
    }
}

void TTurboCApp::runSyntaxCheck() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) {
        messageBox("No active editor window.", mfError | mfOKButton);
        return;
    }

    // Auto save before syntax check
    if (w->editor->modified) {
        w->editor->save();
    }

    std::string path = w->editor->fileName;
    if (path.empty() || !std::filesystem::exists(path)) {
        messageBox("Please save the file before running syntax check.", mfWarning | mfOKButton);
        return;
    }

    TCompileDialog* dlg = new TCompileDialog(path, CompileMode::SyntaxCheck, ConfigManager::instance().compiler);
    ushort dlgRes = deskTop->execView(dlg);
    CompilationResult res = dlg->getResult();
    TObject::destroy(dlg);

    w->editor->setDiagnostics(res.diagnostics);
    if (messageWindow) {
        messageWindow->setDiagnostics(res.diagnostics);
    }

    if (dlgRes == cmYes || res.errorCount > 0) {
        if (messageWindow && !(messageWindow->state & sfVisible)) {
            messageWindow->show();
            messageWindow->select();
        }
        if (res.errorCount > 0) {
            jumpToDiagnostic(0);
        }
    }
}

void TTurboCApp::runCompile() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) {
        messageBox("No active editor window.", mfError | mfOKButton);
        return;
    }

    if (w->editor->modified) {
        w->editor->save();
    }

    std::string path = w->editor->fileName;
    if (path.empty() || !std::filesystem::exists(path)) {
        messageBox("Please save the file before compiling.", mfWarning | mfOKButton);
        return;
    }

    TCompileDialog* dlg = new TCompileDialog(path, CompileMode::BuildExe, ConfigManager::instance().compiler);
    ushort dlgRes = deskTop->execView(dlg);
    CompilationResult res = dlg->getResult();
    TObject::destroy(dlg);

    w->editor->setDiagnostics(res.diagnostics);
    if (messageWindow) {
        messageWindow->setDiagnostics(res.diagnostics);
    }

    if (dlgRes == cmYes || res.errorCount > 0) {
        if (messageWindow && !(messageWindow->state & sfVisible)) {
            messageWindow->show();
            messageWindow->select();
        }
        if (res.errorCount > 0) {
            jumpToDiagnostic(0);
        }
    }
}

void TTurboCApp::runMake() {
    runCompile();
}

void TTurboCApp::runProgram() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) {
        messageBox("No active editor window.", mfError | mfOKButton);
        return;
    }

    if (w->editor->modified) {
        w->editor->save();
    }

    std::string path = w->editor->fileName;
    if (path.empty() || !std::filesystem::exists(path)) {
        messageBox("Please save the file before running.", mfWarning | mfOKButton);
        return;
    }

    // Compile first with real-time feedback
    TCompileDialog* dlg = new TCompileDialog(path, CompileMode::BuildExe, ConfigManager::instance().compiler);
    ushort dlgRes = deskTop->execView(dlg);
    CompilationResult res = dlg->getResult();
    TObject::destroy(dlg);

    w->editor->setDiagnostics(res.diagnostics);
    if (messageWindow) {
        messageWindow->setDiagnostics(res.diagnostics);
    }

    if (!res.success) {
        if (messageWindow && !(messageWindow->state & sfVisible)) {
            messageWindow->show();
            messageWindow->select();
        }
        if (res.errorCount > 0) {
            jumpToDiagnostic(0);
        }
        messageBox("Build failed. Fix compiler errors before running.", mfError | mfOKButton);
        return;
    }

    // Suspend Turbo Vision, execute program interactively in terminal, resume
    suspend();
    LLVMCompiler::instance().runExecutable(res.outputExecutable, ConfigManager::instance().compiler.programArgs);
    resume();
    redraw();
}

void TTurboCApp::jumpToDiagnostic(int index) {
    if (!messageWindow) return;
    const auto& diags = messageWindow->getDiagnostics();
    if (index < 0 || index >= static_cast<int>(diags.size())) return;

    const auto& diag = diags[index];

    // Find if editor is already open
    TTurboEditWindow* targetWin = nullptr;
    deskTop->forEach([](TView* v, void* ctx) {
        auto* pair = static_cast<std::pair<const Diagnostic*, TTurboEditWindow**>*>(ctx);
        TTurboEditWindow* w = dynamic_cast<TTurboEditWindow*>(v);
        if (w && w->editor) {
            std::filesystem::path p1(w->editor->fileName);
            std::filesystem::path p2(pair->first->file);
            if (p1.filename() == p2.filename() || w->editor->fileName == pair->first->file) {
                *pair->second = w;
            }
        }
    }, (void*)new std::pair<const Diagnostic*, TTurboEditWindow**>(&diag, &targetWin));

    if (!targetWin) {
        if (std::filesystem::exists(diag.file)) {
            targetWin = openEditor(diag.file.c_str(), True);
        }
    }

    if (targetWin && targetWin->editor) {
        targetWin->select();
        targetWin->editor->goToLineAndCol(diag.line, diag.column);
    }
}

void TTurboCApp::nextError() {
    if (messageWindow) {
        messageWindow->selectNext();
    }
}

void TTurboCApp::prevError() {
    if (messageWindow) {
        messageWindow->selectPrev();
    }
}

void TTurboCApp::goToLine() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) return;

    TGoToLineDialog* dlg = new TGoToLineDialog(w->editor->getCurrentLine());
    if (deskTop->execView(dlg) == cmOK) {
        int line = dlg->getLineNumber();
        w->editor->goToLineAndCol(line, 1);
    }
    TObject::destroy(dlg);
}

void TTurboCApp::compilerOptions() {
    auto& cfg = ConfigManager::instance().compiler;
    TOptionsDialog* dlg = new TOptionsDialog(cfg);
    if (deskTop->execView(dlg) == cmOK) {
        dlg->getUpdatedConfig(cfg);
        ConfigManager::instance().saveToFile("turboc.json");
    }
    TObject::destroy(dlg);
}

void TTurboCApp::programArgs() {
    auto& cfg = ConfigManager::instance().compiler;
    TProgramArgsDialog* dlg = new TProgramArgsDialog(cfg.programArgs);
    if (deskTop->execView(dlg) == cmOK) {
        cfg.programArgs = dlg->getArguments();
        ConfigManager::instance().saveToFile("turboc.json");
    }
    TObject::destroy(dlg);
}

void TTurboCApp::cleanArtifacts() {
    auto& cfg = ConfigManager::instance().compiler;
    std::filesystem::path p = cfg.outDir.empty() ? "." : cfg.outDir;
    int count = 0;
    try {
        if (std::filesystem::exists(p)) {
            for (const auto& entry : std::filesystem::directory_iterator(p)) {
                if (entry.is_regular_file()) {
                    std::string fname = entry.path().filename().string();
                    if (fname == "turboc.exe" || fname == "test_engine.exe") continue;

                    auto ext = entry.path().extension().string();
                    if (ext == ".obj" || ext == ".o" || ext == ".pdb" || ext == ".ilk" ||
                        (ext == ".exe" && fname != "turboc.exe" && fname != "test_engine.exe")) {
                        std::filesystem::remove(entry.path());
                        count++;
                    }
                }
            }
        }
    } catch (...) {}
    messageBox(std::format("Clean completed. Removed {} build artifact(s).", count).c_str(), mfInformation | mfOKButton);
}

void TTurboCApp::compileToObj() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) {
        messageBox("No active editor window.", mfError | mfOKButton);
        return;
    }
    if (w->editor->modified) {
        w->editor->save();
    }
    std::string path = w->editor->fileName;
    if (path.empty() || !std::filesystem::exists(path)) {
        messageBox("Please save the file before compiling.", mfWarning | mfOKButton);
        return;
    }

    TCompileDialog* dlg = new TCompileDialog(path, CompileMode::CompileObj, ConfigManager::instance().compiler);
    ushort dlgRes = deskTop->execView(dlg);
    CompilationResult res = dlg->getResult();
    TObject::destroy(dlg);

    w->editor->setDiagnostics(res.diagnostics);
    if (messageWindow) {
        messageWindow->setDiagnostics(res.diagnostics);
    }

    if (dlgRes == cmYes || res.errorCount > 0) {
        if (messageWindow && !(messageWindow->state & sfVisible)) {
            messageWindow->show();
            messageWindow->select();
        }
        if (res.errorCount > 0) {
            jumpToDiagnostic(0);
        }
    }
}

void TTurboCApp::buildAll() {
    std::vector<std::string> filesToBuild;
    deskTop->forEach([](TView* v, void* ctx) {
        TTurboEditWindow* w = dynamic_cast<TTurboEditWindow*>(v);
        if (w && w->editor) {
            auto* list = static_cast<std::vector<std::string>*>(ctx);
            std::string path = w->editor->fileName;
            if (!path.empty() && !path.starts_with("NONAME") && !path.starts_with("noname") && std::filesystem::exists(path)) {
                if (w->editor->modified) w->editor->save();
                list->push_back(path);
            }
        }
    }, &filesToBuild);

    if (filesToBuild.empty()) {
        messageBox("No saved source files open in desktop to build.", mfInformation | mfOKButton);
        return;
    }

    TBuildAllDialog* dlg = new TBuildAllDialog(filesToBuild, ConfigManager::instance().compiler);
    deskTop->execView(dlg);
    TObject::destroy(dlg);
}

void TTurboCApp::compilerInfo() {
    std::string info = LLVMCompiler::instance().getCompilerVersionInfo(ConfigManager::instance().compiler);
    TCompilerInfoDialog* dlg = new TCompilerInfoDialog(info);
    deskTop->execView(dlg);
    TObject::destroy(dlg);
}

void TTurboCApp::editorOptions() {
    auto& cfg = ConfigManager::instance().editor;
    TEditorOptionsDialog* dlg = new TEditorOptionsDialog(cfg);
    if (deskTop->execView(dlg) == cmOK) {
        dlg->getUpdatedConfig(cfg);
        SyntaxHighlighter::instance().setThemeByName(cfg.colorTheme);
        ConfigManager::instance().saveToFile("turboc.json");

        // Apply new editor settings across all open editor windows
        deskTop->forEach([](TView* v, void*) {
            TTurboEditWindow* w = dynamic_cast<TTurboEditWindow*>(v);
            if (w && w->editor) {
                w->editor->autoIndent = ConfigManager::instance().editor.autoIndent;
                if (w->gutter) {
                    if (ConfigManager::instance().editor.showLineNumbers) {
                        w->gutter->show();
                        TRect r = w->editor->getBounds();
                        r.a.x = 7;
                        w->editor->locate(r);
                    } else {
                        w->gutter->hide();
                        TRect r = w->editor->getBounds();
                        r.a.x = 1;
                        w->editor->locate(r);
                    }
                    w->gutter->drawView();
                }
                w->editor->drawView();
                if (w->frame) w->frame->drawView();
            }
        }, nullptr);
        redraw();
    }
    TObject::destroy(dlg);
}

void TTurboCApp::directories() {
    auto& cfg = ConfigManager::instance().compiler;
    TDirectoriesDialog* dlg = new TDirectoriesDialog(cfg);
    if (deskTop->execView(dlg) == cmOK) {
        dlg->getUpdatedConfig(cfg);
        ConfigManager::instance().saveToFile("turboc.json");
    }
    TObject::destroy(dlg);
}

void TTurboCApp::saveOptions() {
    bool ok = ConfigManager::instance().saveToFile("turboc.json");
    if (ok) {
        messageBox("Configuration options saved successfully to turboc.json.", mfInformation | mfOKButton);
    } else {
        messageBox("Error: Could not save turboc.json.", mfError | mfOKButton);
    }
}

void TTurboCApp::closeAll() {
    deskTop->forEach([](TView* v, void*) {
        TTurboEditWindow* w = dynamic_cast<TTurboEditWindow*>(v);
        if (w) {
            w->close();
        }
    }, nullptr);
}

void TTurboCApp::windowList() {
    std::vector<std::string> names;
    std::vector<TTurboEditWindow*> windows;
    int currentIdx = 0;
    int idx = 0;
    auto ctx = std::make_tuple(&names, &windows, &currentIdx, &idx);

    deskTop->forEach([](TView* v, void* c) {
        auto* data = static_cast<decltype(&ctx)>(c);
        TTurboEditWindow* w = dynamic_cast<TTurboEditWindow*>(v);
        if (w) {
            std::get<0>(*data)->push_back(w->getTitle(0));
            std::get<1>(*data)->push_back(w);
            if (v == TProgram::deskTop->current) {
                *std::get<2>(*data) = *std::get<3>(*data);
            }
            (*std::get<3>(*data))++;
        }
    }, &ctx);

    if (names.empty()) {
        messageBox("No open windows to display.", mfInformation | mfOKButton);
        return;
    }

    TWindowListDialog* dlg = new TWindowListDialog(names, currentIdx);
    if (deskTop->execView(dlg) == cmOK) {
        int sel = dlg->getSelectedIndex();
        if (sel >= 0 && sel < static_cast<int>(windows.size())) {
            windows[sel]->select();
        }
    }
    TObject::destroy(dlg);
}

void TTurboCApp::locateFunction() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) {
        messageBox("No active editor window.", mfError | mfOKButton);
        return;
    }

    uint p = 0;
    int line = 1;
    std::vector<std::pair<int, std::string>> functions;

    while (p < w->editor->bufLen && functions.size() < 100) {
        uint end = w->editor->lineEnd(p);
        std::string s;
        for (uint i = p; i < end; ++i) s += w->editor->bufChar(i);

        size_t openParen = s.find('(');
        size_t closeParen = s.find(')');
        if (openParen != std::string::npos && closeParen != std::string::npos && closeParen > openParen) {
            size_t firstNonSpace = s.find_first_not_of(" \t");
            if (firstNonSpace != std::string::npos && s[firstNonSpace] != '/' && s[firstNonSpace] != '#') {
                std::string prefix = s.substr(firstNonSpace, openParen - firstNonSpace);
                while (!prefix.empty() && (prefix.back() == ' ' || prefix.back() == '\t')) prefix.pop_back();

                if (prefix != "if" && prefix != "while" && prefix != "for" &&
                    prefix != "switch" && prefix != "catch" && !prefix.starts_with("else if")) {
                    std::string fnName = s.substr(firstNonSpace, std::min<size_t>(40, s.length() - firstNonSpace));
                    functions.push_back({line, std::format("{:>4}: {}", line, fnName)});
                }
            }
        }
        p = w->editor->nextLine(p);
        line++;
    }

    if (functions.empty()) {
        messageBox("No functions or signatures detected in current file.", mfInformation | mfOKButton);
        return;
    }

    std::vector<std::string> fnNames;
    for (const auto& fn : functions) fnNames.push_back(fn.second);

    TWindowListDialog* dlg = new TWindowListDialog(fnNames, 0);
    if (deskTop->execView(dlg) == cmOK) {
        int sel = dlg->getSelectedIndex();
        if (sel >= 0 && sel < static_cast<int>(functions.size())) {
            w->editor->goToLineAndCol(functions[sel].first, 1);
        }
    }
    TObject::destroy(dlg);
}

void TTurboCApp::showClipboard() {
    if (TEditor::clipboard && TEditor::clipboard->owner) {
        TEditor::clipboard->owner->show();
        TEditor::clipboard->owner->select();
    } else {
        messageBox("Clipboard window is empty or uninitialized.", mfInformation | mfOKButton);
    }
}

void TTurboCApp::helpContents() {
    THelpContentsDialog* dlg = new THelpContentsDialog();
    deskTop->execView(dlg);
    TObject::destroy(dlg);
}

void TTurboCApp::clangHelp() {
    TClangHelpDialog* dlg = new TClangHelpDialog();
    deskTop->execView(dlg);
    TObject::destroy(dlg);
}

void TTurboCApp::refreshScreen() {
    redraw();
}

void TTurboCApp::clangFormat() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) return;

    if (w->editor->modified) {
        w->editor->save();
    }

    std::string path = w->editor->fileName;
    if (path.empty() || !std::filesystem::exists(path)) return;

    TFormatDialog* dlg = new TFormatDialog(path, ConfigManager::instance().compiler);
    deskTop->execView(dlg);
    bool ok = dlg->isSuccess();
    TObject::destroy(dlg);

    if (ok) {
        w->editor->loadFile();
        w->editor->drawView();
    }
}

void TTurboCApp::aboutBox() {
    TAboutDialog* dlg = new TAboutDialog();
    deskTop->execView(dlg);
    TObject::destroy(dlg);
}

void TTurboCApp::hexView() {
    char fileName[MAXPATH] = "*.*";
    TTurboEditWindow* w = getActiveEditor();
    if (w && w->editor && w->editor->fileName[0] != '\0' && std::filesystem::exists(w->editor->fileName)) {
        std::strncpy(fileName, w->editor->fileName, sizeof(fileName) - 1);
    }

    if (execDialog(new TFileDialog("*.*", "Hex View File", "~N~ame", fdOpenButton, 100), fileName) != cmCancel) {
        if (std::filesystem::exists(fileName)) {
            THexViewDialog* dlg = new THexViewDialog(fileName);
            deskTop->execView(dlg);
            TObject::destroy(dlg);
        } else {
            messageBox("File does not exist.", mfError | mfOKButton);
        }
    }
}

void TTurboCApp::insertSnippet() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) {
        messageBox("No active editor window.", mfError | mfOKButton);
        return;
    }

    TSnippetsDialog* dlg = new TSnippetsDialog();
    if (deskTop->execView(dlg) == cmOK) {
        std::string snippet = dlg->getSelectedSnippetText();
        w->editor->insertText(snippet.data(), snippet.length(), False);
        w->editor->drawView();
    }
    TObject::destroy(dlg);
}

void TTurboCApp::fileStatistics() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) {
        messageBox("No active editor window.", mfError | mfOKButton);
        return;
    }

    FileStats stats;
    stats.fileSize = w->editor->bufLen;
    stats.totalLines = w->editor->getTotalLines();

    uint p = 0;
    bool inComment = false;
    while (p < w->editor->bufLen) {
        uint end = w->editor->lineEnd(p);
        std::string line;
        for (uint i = p; i < end; ++i) {
            char c = w->editor->bufChar(i);
            line += c;
            stats.charCount++;
            if (!std::isspace(static_cast<unsigned char>(c))) {
                stats.charNoSpaceCount++;
            }
        }

        std::string_view lineView = line;
        size_t firstNonSpace = lineView.find_first_not_of(" \t\r\n");
        if (firstNonSpace == std::string_view::npos) {
            stats.blankLines++;
        } else {
            std::string_view trimmed = lineView.substr(firstNonSpace);
            if (trimmed.starts_with("//")) {
                stats.commentLines++;
            } else if (trimmed.starts_with("/*")) {
                stats.commentLines++;
                if (trimmed.find("*/") == std::string_view::npos) inComment = true;
            } else if (inComment) {
                stats.commentLines++;
                if (trimmed.find("*/") != std::string_view::npos) inComment = false;
            } else {
                stats.codeLines++;
                size_t openP = trimmed.find('(');
                if (openP != std::string_view::npos && trimmed.find(')') != std::string_view::npos) {
                    std::string prefix = std::string(trimmed.substr(0, openP));
                    while (!prefix.empty() && std::isspace(static_cast<unsigned char>(prefix.back()))) prefix.pop_back();
                    if (prefix != "if" && prefix != "while" && prefix != "for" && prefix != "switch" && prefix != "catch") {
                        stats.functionCount++;
                    }
                }
            }
        }
        p = w->editor->nextLine(p);
    }

    TFileStatsDialog* dlg = new TFileStatsDialog(w->editor->fileName, stats);
    deskTop->execView(dlg);
    TObject::destroy(dlg);
}

void TTurboCApp::viewAssembly() {
    TTurboEditWindow* w = getActiveEditor();
    if (!w || !w->editor) {
        messageBox("No active editor window.", mfError | mfOKButton);
        return;
    }

    if (w->editor->modified) {
        w->editor->save();
    }

    std::string path = w->editor->fileName;
    if (path.empty() || !std::filesystem::exists(path)) {
        messageBox("Please save the file before viewing assembly.", mfWarning | mfOKButton);
        return;
    }

    std::filesystem::path src(path);
    std::filesystem::path asmPath = src.parent_path() / (src.stem().string() + ".asm");

    TCompileDialog* dlg = new TCompileDialog(path, CompileMode::GenerateAssembly, ConfigManager::instance().compiler, asmPath.string());
    ushort dlgRes = deskTop->execView(dlg);
    CompilationResult res = dlg->getResult();
    TObject::destroy(dlg);

    if (res.success && std::filesystem::exists(asmPath)) {
        openEditor(asmPath.string().c_str(), True);
    } else if (dlgRes == cmYes || res.errorCount > 0) {
        w->editor->setDiagnostics(res.diagnostics);
        if (messageWindow) {
            messageWindow->setDiagnostics(res.diagnostics);
            messageWindow->show();
            messageWindow->select();
        }
        if (res.errorCount > 0) {
            jumpToDiagnostic(0);
        }
    }
}

void TTurboCApp::colorThemes() {
    auto& cfg = ConfigManager::instance().editor;
    int currentIdx = 0;
    if (cfg.colorTheme == "ModernDark") currentIdx = 1;
    else if (cfg.colorTheme == "RetroGreen") currentIdx = 2;
    else if (cfg.colorTheme == "RetroAmber") currentIdx = 3;
    else if (cfg.colorTheme == "BorlandPaper") currentIdx = 4;
    else if (cfg.colorTheme == "DarkViolet") currentIdx = 5;
    else if (cfg.colorTheme == "SlateTeal") currentIdx = 6;
    else if (cfg.colorTheme == "ObsidianBlue") currentIdx = 7;
    else currentIdx = 0;

    TThemeDialog* dlg = new TThemeDialog(currentIdx);
    if (deskTop->execView(dlg) == cmOK) {
        int sel = dlg->getSelectedThemeIndex();
        switch (sel) {
            case 1: cfg.colorTheme = "ModernDark"; break;
            case 2: cfg.colorTheme = "RetroGreen"; break;
            case 3: cfg.colorTheme = "RetroAmber"; break;
            case 4: cfg.colorTheme = "BorlandPaper"; break;
            case 5: cfg.colorTheme = "DarkViolet"; break;
            case 6: cfg.colorTheme = "SlateTeal"; break;
            case 7: cfg.colorTheme = "ObsidianBlue"; break;
            case 0:
            default: cfg.colorTheme = "BorlandBlue"; break;
        }
        SyntaxHighlighter::instance().setThemeByName(cfg.colorTheme);
        ConfigManager::instance().saveToFile("turboc.json");

        // Redraw all windows
        deskTop->forEach([](TView* v, void*) {
            TTurboEditWindow* ew = dynamic_cast<TTurboEditWindow*>(v);
            if (ew && ew->editor) {
                ew->editor->drawView();
                if (ew->gutter) ew->gutter->drawView();
                if (ew->frame) ew->frame->drawView();
            }
        }, nullptr);
        redraw();
    }
    TObject::destroy(dlg);
}

void TTurboCApp::shortcutsHelp() {
    const char* helpText =
        "Modern Turbo C++ Keybindings:\n\n"
        "  F1          Help contents\n"
        "  F2          Save file (Save As if untitled)\n"
        "  F3          Open file dialog\n"
        "  F5          Zoom / Restore active window\n"
        "  F6          Next window\n"
        "  Shift-F6    Previous window\n"
        "  F9          Make / Build executable\n"
        "  F11         Toggle Messages window\n"
        "  Alt-F3      Close active window\n"
        "  Alt-F7      Previous error diagnostic\n"
        "  Alt-F8      Next error diagnostic\n"
        "  Alt-F9      Compile to .OBJ\n"
        "  Alt-F10     View Clang-generated x86_64 assembly\n"
        "  Shift-F10   Open Color Themes selector\n"
        "  Alt-G       Go to line number\n"
        "  Ctrl-F7     LLVM Clang syntax check\n"
        "  Ctrl-F8     Toggle line numbers gutter\n"
        "  Ctrl-F9     Run compiled executable\n"
        "  Ctrl-I      Insert C++23 code snippet\n"
        "  Ctrl-K      Format with Clang-Format\n"
        "  Alt-0       Window list\n"
        "  Alt-X       Exit Modern Turbo C++";

    messageBox(helpText, mfInformation | mfOKButton);
}

void TTurboCApp::handleEvent(TEvent& event) {
    TApplication::handleEvent(event);

    if (event.what == evCommand) {
        switch (event.message.command) {
            // Edit commands routed to active editor
            case cmUndo:
            case cmCut:
            case cmCopy:
            case cmPaste:
            case cmClear:
            case cmSelectAll:
            case cmFind:
            case cmReplace:
            case cmSearchAgain: {
                TTurboEditWindow* w = getActiveEditor();
                if (w && w->editor) {
                    w->editor->handleEvent(event);
                }
                break;
            }

            case cmOpen:
                fileOpen();
                break;
            case cmNew:
                fileNew();
                break;
            case cmSave:
                fileSave();
                break;
            case cmSaveAs:
                fileSaveAs();
                break;
            case cmSaveAllFiles:
                fileSaveAll();
                break;
            case cmHexView:
                hexView();
                break;
            case cmInsertSnippet:
                insertSnippet();
                break;
            case cmFileStats:
                fileStatistics();
                break;
            case cmViewAssembly:
                viewAssembly();
                break;
            case cmColorThemes:
                colorThemes();
                break;
            case cmChangeDir:
                changeDir();
                break;
            case cmDosShell:
                dosShell();
                break;
            case cmCompile:
            case cmCompileObj:
                compileToObj();
                break;
            case cmMake:
                runMake();
                break;
            case cmBuildAll:
                buildAll();
                break;
            case cmSyntaxCheck:
                runSyntaxCheck();
                break;
            case cmRunProgram:
                runProgram();
                break;
            case cmProgramArgs:
                programArgs();
                break;
            case cmCleanArtifacts:
                cleanArtifacts();
                break;
            case cmMessages:
                toggleMessages();
                break;
            case cmCompilerInfo:
                compilerInfo();
                break;
            case cmJumpToDiagnostic: {
                int idx = static_cast<int>(reinterpret_cast<intptr_t>(event.message.infoPtr));
                jumpToDiagnostic(idx);
                break;
            }
            case cmNextError:
                nextError();
                break;
            case cmPrevError:
                prevError();
                break;
            case cmGoToLine:
                goToLine();
                break;
            case cmLocateFunction:
                locateFunction();
                break;
            case cmShowClipboard:
                showClipboard();
                break;
            case cmCompilerOptions:
                compilerOptions();
                break;
            case cmEditorOptions:
                editorOptions();
                break;
            case cmDirectories:
                directories();
                break;
            case cmToggleGutter:
                toggleLineNumbers();
                break;
            case cmSaveOptions:
                saveOptions();
                break;
            case cmResize:
                if (deskTop && deskTop->current) {
                    deskTop->current->handleEvent(event);
                }
                break;
            case cmZoom:
                if (deskTop && deskTop->current) {
                    TWindow* w = dynamic_cast<TWindow*>(deskTop->current);
                    if (w) w->zoom();
                }
                break;
            case cmTile:
                tile();
                break;
            case cmCascade:
                cascade();
                break;
            case cmNext:
                if (deskTop) {
                    deskTop->selectNext(False);
                }
                break;
            case cmPrev:
                if (deskTop && deskTop->current && deskTop->background) {
                    deskTop->current->putInFrontOf(deskTop->background);
                }
                break;
            case cmClose:
                if (deskTop && deskTop->current) {
                    TWindow* w = dynamic_cast<TWindow*>(deskTop->current);
                    if (w) w->close();
                }
                break;
            case cmCloseAll:
                closeAll();
                break;
            case cmWindowList:
                windowList();
                break;
            case cmClangFormat:
                clangFormat();
                break;
            case cmAboutBox:
                aboutBox();
                break;
            case cmHelp:
            case cmHelpContents:
                helpContents();
                break;
            case cmClangHelp:
                clangHelp();
                break;
            case cmRefreshScreen:
                refreshScreen();
                break;
            case cmShortcutsHelp:
                shortcutsHelp();
                break;
            default:
                return;
        }
        clearEvent(event);
    }
}

void TTurboCApp::idle() {
    TApplication::idle();
    message(deskTop, evBroadcast, cmPeriodicUpdate, nullptr);
}

} // namespace TurboC
