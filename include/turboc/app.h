#pragma once

#include "turboc/editor.h"
#include "turboc/msgwindow.h"
#include "turboc/compiler.h"
#include "turboc/config.h"

#define Uses_TApplication
#define Uses_TDeskTop
#define Uses_TRect
#define Uses_TFrame
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusLine
#define Uses_TStatusItem
#define Uses_TStatusDef
#define Uses_TKeys
#define Uses_TEvent
#define Uses_MsgBox
#include <tvision/tv.h>

namespace TurboC {

class TTurboCApp : public TApplication {
public:
    TTurboCApp(int argc, char** argv);
    virtual ~TTurboCApp();

    static TMenuBar* initMenuBar(TRect r);
    static TStatusLine* initStatusLine(TRect r);
    static TDeskTop* initDeskTop(TRect r);

    virtual void handleEvent(TEvent& event) override;
    virtual void idle() override;

    TTurboEditWindow* openEditor(const char* fileName, Boolean visible);
    TTurboEditWindow* getActiveEditor();

    void fileOpen();
    void fileNew();
    void fileSave();
    void fileSaveAs();
    void fileSaveAll();
    void changeDir();

    void runCompile();
    void runMake();
    void runSyntaxCheck();
    void runProgram();
    void toggleMessages();

    void jumpToDiagnostic(int index);
    void nextError();
    void prevError();
    void goToLine();
    void compilerOptions();
    void editorOptions();
    void directories();
    void saveOptions();
    void clangFormat();
    void aboutBox();
    void shortcutsHelp();
    void programArgs();
    void cleanArtifacts();
    void compileToObj();
    void buildAll();
    void compilerInfo();
    void closeAll();
    void windowList();
    void locateFunction();
    void showClipboard();
    void helpContents();
    void clangHelp();
    void refreshScreen();
    void dosShell();
    void toggleLineNumbers();
    void hexView();
    void insertSnippet();
    void fileStatistics();
    void viewAssembly();
    void colorThemes();

    TMessageWindow* messageWindow = nullptr;
    int windowCounter = 1;
};

} // namespace TurboC
