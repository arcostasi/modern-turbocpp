#pragma once

#include "turboc/compiler.h"
#include "turboc/syntax.h"
#include <unordered_set>
#include <vector>
#include <string>

#define Uses_TFileEditor
#define Uses_TEditor
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TRect
#define Uses_TScrollBar
#define Uses_TIndicator
#define Uses_TView
#define Uses_TWindow
#define Uses_TFrame
#include <tvision/tv.h>

namespace TurboC {

class TGutterView;
class TTurboEditWindow;

class TTurboEditor : public TFileEditor {
public:
    TTurboEditor(const TRect& bounds,
                 TScrollBar* aHScrollBar,
                 TScrollBar* aVScrollBar,
                 TIndicator* aIndicator,
                 TStringView aFileName) noexcept;

    virtual ~TTurboEditor() = default;

    virtual void draw() override;
    virtual void handleEvent(TEvent& event) override;

    void setGutter(TGutterView* aGutter) { gutter = aGutter; }
    void setDiagnostics(const std::vector<Diagnostic>& diags);

    void goToLineAndCol(int line, int col);
    int getCurrentLine() const;
    int getCurrentCol() const;
    int getTotalLines() const;

    bool hasErrorsOnLine(int line) const;
    bool hasWarningsOnLine(int line) const;

    const std::vector<Diagnostic>& getDiagnostics() const { return activeDiagnostics; }

    void updateBracketMatching();

private:
    void drawSyntaxLines(int y, int count, uint linePtr);

    TGutterView* gutter = nullptr;
    std::vector<Diagnostic> activeDiagnostics;
    std::unordered_set<int> errorLines;
    std::unordered_set<int> warningLines;
    bool syntaxHighlightEnabled = true;

    uint matchingBracketPos = uint(-1);
    uint currentBracketPos = uint(-1);
};

class TGutterView : public TView {
    friend class TTurboEditWindow;
public:
    TGutterView(const TRect& bounds, TTurboEditor* aEditor) noexcept;

    virtual void draw() override;
    virtual void handleEvent(TEvent& event) override;

    TTurboEditor* editor = nullptr;
};

class TTurboEditWindow : public TWindow {
public:
    TTurboEditWindow(const TRect& bounds, TStringView aFileName, int aNumber) noexcept;
    virtual ~TTurboEditWindow() = default;

    virtual void close() override;
    virtual const char* getTitle(short) override;
    virtual void handleEvent(TEvent& event) override;

    TTurboEditor* editor = nullptr;
    TGutterView* gutter = nullptr;

private:
    char titleBuffer[256];
};

} // namespace TurboC
