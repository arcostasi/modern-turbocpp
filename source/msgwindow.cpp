#include "turboc/msgwindow.h"
#include "turboc/commands.h"
#include <format>
#include <cstring>
#include <algorithm>

namespace TurboC {

TMessageListViewer::TMessageListViewer(const TRect& bounds,
                                       TScrollBar* aHScrollBar,
                                       TScrollBar* aVScrollBar) noexcept
    : TListViewer(bounds, 1, aHScrollBar, aVScrollBar)
{
    growMode = gfGrowHiX | gfGrowHiY;
}

void TMessageListViewer::setDiagnostics(const std::vector<Diagnostic>& diags) {
    diagnostics = diags;
    setRange(static_cast<short>(diagnostics.size()));
    if (!diagnostics.empty()) {
        focused = 0;
    } else {
        focused = 0;
    }
    drawView();
}

void TMessageListViewer::setFocusedIndex(int idx) {
    if (idx >= 0 && idx < static_cast<int>(diagnostics.size())) {
        focusItemNum(static_cast<short>(idx));
        drawView();
    }
}

void TMessageListViewer::getText(char* dest, short item, short maxLen) {
    if (item >= 0 && item < static_cast<short>(diagnostics.size())) {
        std::string txt = diagnostics[item].getFormattedText();
        std::strncpy(dest, txt.c_str(), maxLen - 1);
        dest[maxLen - 1] = '\0';
    } else {
        dest[0] = '\0';
    }
}

void TMessageListViewer::draw() {
    TDrawBuffer b;
    for (short i = 0; i < size.y; ++i) {
        short item = topItem + i;
        b.moveChar(0, ' ', TColorAttr(0x17), size.x);

        if (item < range && item < static_cast<short>(diagnostics.size())) {
            const auto& diag = diagnostics[item];
            std::string text = diag.getFormattedText();

            bool isFocused = (item == focused && (state & sfSelected));
            TColorAttr attr;

            if (isFocused) {
                attr = TColorAttr(0x70); // Inverse selection
            } else {
                switch (diag.severity) {
                    case DiagnosticSeverity::Error:
                        attr = TColorAttr(0x1C); // Bright Red on Blue
                        break;
                    case DiagnosticSeverity::Warning:
                        attr = TColorAttr(0x1E); // Yellow on Blue
                        break;
                    case DiagnosticSeverity::Note:
                        attr = TColorAttr(0x1B); // Light Cyan on Blue
                        break;
                }
            }

            b.moveStr(0, text, attr, size.x);
        } else if (diagnostics.empty() && i == 0) {
            b.moveStr(2, "No compile or syntax messages.", TColorAttr(0x13), size.x);
        }

        writeBuf(0, i, size.x, 1, b);
    }
}

void TMessageListViewer::handleEvent(TEvent& event) {
    TListViewer::handleEvent(event);

    if ((event.what == evMouseDown && (event.mouse.eventFlags & meDoubleClick)) ||
        (event.what == evKeyDown && event.keyDown.keyCode == kbEnter)) {
        if (!diagnostics.empty() && focused >= 0 && focused < static_cast<short>(diagnostics.size())) {
            message(owner, evCommand, cmJumpToDiagnostic, reinterpret_cast<void*>(static_cast<intptr_t>(focused)));
            clearEvent(event);
        }
    }
}

// ============================================================================
// TMessageWindow Implementation
// ============================================================================

TMessageWindow::TMessageWindow(const TRect& bounds) noexcept
    : TWindowInit(&TMessageWindow::initFrame),
      TWindow(bounds, "Messages", wnNoNumber)
{
    options |= ofTileable;

    TScrollBar* vScrollBar = new TScrollBar(TRect(size.x - 1, 1, size.x, size.y - 1));
    insert(vScrollBar);

    TScrollBar* hScrollBar = new TScrollBar(TRect(1, size.y - 1, size.x - 1, size.y));
    insert(hScrollBar);

    TRect r(1, 1, size.x - 1, size.y - 1);
    listViewer = new TMessageListViewer(r, hScrollBar, vScrollBar);
    insert(listViewer);
}

void TMessageWindow::setDiagnostics(const std::vector<Diagnostic>& diags) {
    if (listViewer) {
        listViewer->setDiagnostics(diags);
    }
    frame->drawView();
}

const std::vector<Diagnostic>& TMessageWindow::getDiagnostics() const {
    static std::vector<Diagnostic> empty;
    return listViewer ? listViewer->getDiagnostics() : empty;
}

bool TMessageWindow::selectNext() {
    if (!listViewer) return false;
    const auto& diags = listViewer->getDiagnostics();
    if (diags.empty()) return false;

    int nextIdx = listViewer->getFocusedIndex() + 1;
    if (nextIdx >= static_cast<int>(diags.size())) {
        nextIdx = 0; // Wrap around
    }
    listViewer->setFocusedIndex(nextIdx);
    message(owner, evCommand, cmJumpToDiagnostic, reinterpret_cast<void*>(static_cast<intptr_t>(nextIdx)));
    return true;
}

bool TMessageWindow::selectPrev() {
    if (!listViewer) return false;
    const auto& diags = listViewer->getDiagnostics();
    if (diags.empty()) return false;

    int prevIdx = listViewer->getFocusedIndex() - 1;
    if (prevIdx < 0) {
        prevIdx = static_cast<int>(diags.size()) - 1; // Wrap around
    }
    listViewer->setFocusedIndex(prevIdx);
    message(owner, evCommand, cmJumpToDiagnostic, reinterpret_cast<void*>(static_cast<intptr_t>(prevIdx)));
    return true;
}

const Diagnostic* TMessageWindow::getCurrentDiagnostic() const {
    if (!listViewer) return nullptr;
    const auto& diags = listViewer->getDiagnostics();
    int idx = listViewer->getFocusedIndex();
    if (idx >= 0 && idx < static_cast<int>(diags.size())) {
        return &diags[idx];
    }
    return nullptr;
}

const char* TMessageWindow::getTitle(short) {
    if (!listViewer) return " Messages ";

    int errors = 0;
    int warnings = 0;
    for (const auto& d : listViewer->getDiagnostics()) {
        if (d.severity == DiagnosticSeverity::Error) errors++;
        else if (d.severity == DiagnosticSeverity::Warning) warnings++;
    }

    std::snprintf(titleBuffer, sizeof(titleBuffer), " Messages (%d Errors, %d Warnings) ", errors, warnings);
    return titleBuffer;
}

void TMessageWindow::handleEvent(TEvent& event) {
    TWindow::handleEvent(event);
}

} // namespace TurboC
