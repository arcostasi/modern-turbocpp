#pragma once

#include "turboc/compiler.h"
#include <vector>
#include <string>

#define Uses_TWindow
#define Uses_TFrame
#define Uses_TListViewer
#define Uses_TScrollBar
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#include <tvision/tv.h>

namespace TurboC {

class TMessageListViewer : public TListViewer {
public:
    TMessageListViewer(const TRect& bounds,
                       TScrollBar* aHScrollBar,
                       TScrollBar* aVScrollBar) noexcept;

    virtual void getText(char* dest, short item, short maxLen) override;
    virtual void handleEvent(TEvent& event) override;
    virtual void draw() override;

    void setDiagnostics(const std::vector<Diagnostic>& diags);
    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics; }

    int getFocusedIndex() const { return focused; }
    void setFocusedIndex(int idx);

private:
    std::vector<Diagnostic> diagnostics;
};

class TMessageWindow : public TWindow {
public:
    TMessageWindow(const TRect& bounds) noexcept;
    virtual ~TMessageWindow() = default;

    virtual void handleEvent(TEvent& event) override;
    virtual const char* getTitle(short) override;

    void setDiagnostics(const std::vector<Diagnostic>& diags);
    const std::vector<Diagnostic>& getDiagnostics() const;

    bool selectNext();
    bool selectPrev();
    const Diagnostic* getCurrentDiagnostic() const;

    TMessageListViewer* listViewer = nullptr;

private:
    char titleBuffer[128];
};

} // namespace TurboC
