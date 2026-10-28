#include "turboc/editor.h"
#include "turboc/commands.h"
#include "turboc/config.h"
#include <format>
#include <algorithm>
#include <iostream>

namespace TurboC {

TTurboEditor::TTurboEditor(const TRect& bounds,
                           TScrollBar* aHScrollBar,
                           TScrollBar* aVScrollBar,
                           TIndicator* aIndicator,
                           TStringView aFileName) noexcept
    : TFileEditor(bounds, aHScrollBar, aVScrollBar, aIndicator, aFileName)
{
}

void TTurboEditor::setDiagnostics(const std::vector<Diagnostic>& diags) {
    activeDiagnostics = diags;
    errorLines.clear();
    warningLines.clear();

    std::filesystem::path myPath(fileName);
    std::string myName = myPath.filename().string();

    for (const auto& d : activeDiagnostics) {
        std::filesystem::path dPath(d.file);
        std::string dName = dPath.filename().string();

        if (dName == myName || d.file == fileName) {
            if (d.severity == DiagnosticSeverity::Error) {
                errorLines.insert(d.line);
            } else if (d.severity == DiagnosticSeverity::Warning) {
                warningLines.insert(d.line);
            }
        }
    }

    if (gutter) gutter->drawView();
    drawView();
}

bool TTurboEditor::hasErrorsOnLine(int line) const {
    return errorLines.contains(line);
}

bool TTurboEditor::hasWarningsOnLine(int line) const {
    return warningLines.contains(line);
}

int TTurboEditor::getCurrentLine() const {
    return curPos.y + 1;
}

int TTurboEditor::getCurrentCol() const {
    return curPos.x + 1;
}

int TTurboEditor::getTotalLines() const {
    return limit.y;
}

void TTurboEditor::goToLineAndCol(int line, int col) {
    if (line < 1) line = 1;
    if (col < 1) col = 1;

    uint p = 0;
    int curLine = 1;
    while (p < bufLen && curLine < line) {
        p = nextLine(p);
        curLine++;
    }

    uint lineStartP = p;
    uint lineEndP = lineEnd(p);
    uint targetP = lineStartP + (col - 1);
    if (targetP > lineEndP) targetP = lineEndP;

    setCurPtr(targetP, 0);
    trackCursor(True);

    if (gutter) gutter->drawView();
    drawView();
}

void TTurboEditor::updateBracketMatching() {
    matchingBracketPos = uint(-1);
    currentBracketPos = uint(-1);

    if (bufLen == 0) return;

    auto isBracketChar = [](char c) {
        return c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']';
    };

    uint checkPos = curPtr;
    char ch = 0;
    if (checkPos < bufLen && isBracketChar(bufChar(checkPos))) {
        ch = bufChar(checkPos);
    } else if (curPtr > 0 && isBracketChar(bufChar(curPtr - 1))) {
        checkPos = curPtr - 1;
        ch = bufChar(checkPos);
    } else {
        return;
    }

    currentBracketPos = checkPos;

    char matchCh = 0;
    int dir = 0;
    if (ch == '(') { matchCh = ')'; dir = 1; }
    else if (ch == ')') { matchCh = '('; dir = -1; }
    else if (ch == '{') { matchCh = '}'; dir = 1; }
    else if (ch == '}') { matchCh = '{'; dir = -1; }
    else if (ch == '[') { matchCh = ']'; dir = 1; }
    else if (ch == ']') { matchCh = '['; dir = -1; }

    int depth = 1;
    uint scan = checkPos + dir;

    while (dir > 0 ? (scan < bufLen) : (scan < bufLen && scan >= 0)) {
        if (dir < 0 && scan == uint(-1)) break;
        char c = bufChar(scan);
        if (c == ch) {
            depth++;
        } else if (c == matchCh) {
            depth--;
            if (depth == 0) {
                matchingBracketPos = scan;
                break;
            }
        }
        if (dir < 0 && scan == 0) break;
        scan += dir;
    }
}

void TTurboEditor::draw() {
    updateBracketMatching();
    if (drawLine != delta.y) {
        drawPtr = lineMove(drawPtr, delta.y - drawLine);
        drawLine = delta.y;
    }
    drawSyntaxLines(0, size.y, drawPtr);
    if (gutter) {
        gutter->drawView();
    }
}

void TTurboEditor::drawSyntaxLines(int y, int count, uint linePtr) {
    TDrawBuffer b;
    SyntaxHighlighter& hl = SyntaxHighlighter::instance();

    // Determine multi-line comment state at linePtr by scanning from 0
    bool inComment = false;
    uint scanPtr = 0;
    while (scanPtr < linePtr) {
        uint next = nextLine(scanPtr);
        uint end = lineEnd(scanPtr);
        std::string scanLine;
        for (uint i = scanPtr; i < end; ++i) {
            scanLine.push_back(bufChar(i));
        }
        hl.tokenizeLine(scanLine, inComment);
        scanPtr = next;
    }

    int startY = y;
    while (count-- > 0 && y < size.y) {
        int currentLineNum = delta.y + (y - startY) + 1;
        bool isErrorLine = hasErrorsOnLine(currentLineNum);
        bool isWarningLine = hasWarningsOnLine(currentLineNum);

        if (linePtr >= bufLen && y >= (limit.y - delta.y)) {
            // Empty space beyond end of file
            b.moveChar(0, ' ', hl.getBackgroundAttr(), size.x);
            writeBuf(0, y, size.x, 1, b);
            y++;
            continue;
        }

        uint end = lineEnd(linePtr);
        std::string lineStr;
        for (uint i = linePtr; i < end; ++i) {
            lineStr.push_back(bufChar(i));
        }

        std::vector<Token> tokens = hl.tokenizeLine(lineStr, inComment);

        // Render characters into draw buffer
        int screenX = 0;
        size_t tokenIdx = 0;
        size_t currentTokenEnd = tokens.empty() ? 0 : (tokens[0].start + tokens[0].length);

        for (size_t charIdx = 0; charIdx < lineStr.length(); ++charIdx) {
            while (tokenIdx < tokens.size() && charIdx >= currentTokenEnd) {
                tokenIdx++;
                if (tokenIdx < tokens.size()) {
                    currentTokenEnd = tokens[tokenIdx].start + tokens[tokenIdx].length;
                }
            }

            TokenType tokType = (tokenIdx < tokens.size()) ? tokens[tokenIdx].type : TokenType::Default;

            uint absPos = linePtr + charIdx;
            bool isSel = (selStart <= absPos && absPos < selEnd);
            TColorAttr attr;
            if (absPos == currentBracketPos || absPos == matchingBracketPos) {
                attr = hl.getBracketMatchAttr();
            } else {
                attr = hl.getAttrForToken(tokType, isSel, isErrorLine, isWarningLine);
            }

            char ch = lineStr[charIdx];
            if (ch == '\t') {
                int tabStop = 4;
                int spaces = tabStop - (screenX % tabStop);
                for (int sp = 0; sp < spaces; ++sp) {
                    int drawCol = screenX - delta.x;
                    if (drawCol >= 0 && drawCol < size.x) {
                        b.moveChar(drawCol, ' ', attr, 1);
                    }
                    screenX++;
                }
            } else {
                int drawCol = screenX - delta.x;
                if (drawCol >= 0 && drawCol < size.x) {
                    b.moveChar(drawCol, ch, attr, 1);
                }
                screenX++;
            }
        }

        // Fill remaining width of line with background attribute
        int remainingStart = screenX - delta.x;
        if (remainingStart < 0) remainingStart = 0;
        if (remainingStart < size.x) {
            TColorAttr bgAttr = hl.getBackgroundAttr(isErrorLine, isWarningLine);
            b.moveChar(remainingStart, ' ', bgAttr, size.x - remainingStart);
        }

        writeBuf(0, y, size.x, 1, b);
        linePtr = nextLine(linePtr);
        y++;
    }
}

void TTurboEditor::handleEvent(TEvent& event) {
    // Auto-closing delimiters
    if (event.what == evKeyDown && ConfigManager::instance().editor.autoCloseDelimiters) {
        char c = event.keyDown.charScan.charCode;
        if (c == '(' || c == '[' || c == '{' || c == '\"' || c == '\'') {
            if (hasSelection()) {
                char closeC = (c == '(' ? ')' : (c == '[' ? ']' : (c == '{' ? '}' : c)));
                uint sStart = selStart;
                uint sEnd = selEnd;
                std::string selectedText;
                for (uint i = sStart; i < sEnd; ++i) selectedText += bufChar(i);
                std::string wrapped = std::format("{}{}{}", c, selectedText, closeC);
                insertText(wrapped.data(), wrapped.length(), False);
                setCurPtr(sStart + wrapped.length(), 0);
                clearEvent(event);
                return;
            } else {
                if (c == '\"' && curPtr < bufLen && bufChar(curPtr) == '\"') {
                    setCurPtr(curPtr + 1, 0);
                    clearEvent(event);
                    return;
                } else if (c == '\'' && curPtr < bufLen && bufChar(curPtr) == '\'') {
                    setCurPtr(curPtr + 1, 0);
                    clearEvent(event);
                    return;
                }
                char pair[2] = {c, (c == '(' ? ')' : (c == '[' ? ']' : (c == '{' ? '}' : c)))};
                insertText(pair, 2, False);
                setCurPtr(curPtr - 1, 0);
                clearEvent(event);
                return;
            }
        } else if (c == ')' || c == ']' || c == '}') {
            if (!hasSelection() && curPtr < bufLen && bufChar(curPtr) == c) {
                setCurPtr(curPtr + 1, 0);
                clearEvent(event);
                return;
            }
        }
    }

    Boolean wasModified = modified;
    TFileEditor::handleEvent(event);

    if (modified != wasModified && owner) {
        message(owner, evBroadcast, cmUpdateTitle, 0);
    }

    // If editor line or view updated, make sure our custom draw runs
    if (updateFlags != 0) {
        updateFlags |= ufView;
        if (gutter) gutter->drawView();
    }
}

// ============================================================================
// TGutterView Implementation
// ============================================================================

TGutterView::TGutterView(const TRect& bounds, TTurboEditor* aEditor) noexcept
    : TView(bounds), editor(aEditor)
{
    eventMask |= evMouseDown;
}

void TGutterView::draw() {
    TDrawBuffer b;
    SyntaxHighlighter& hl = SyntaxHighlighter::instance();
    if (!editor) {
        b.moveChar(0, ' ', hl.getGutterAttr(), size.x);
        for (int y = 0; y < size.y; ++y) writeBuf(0, y, size.x, 1, b);
        return;
    }

    int totalLines = editor->getTotalLines();

    for (int y = 0; y < size.y; ++y) {
        int lineNum = editor->delta.y + y + 1;
        b.moveChar(0, ' ', hl.getGutterAttr(), size.x);

        if (lineNum <= totalLines) {
            bool isError = editor->hasErrorsOnLine(lineNum);
            bool isWarning = editor->hasWarningsOnLine(lineNum);
            bool isActive = (lineNum == editor->getCurrentLine());

            char marker = ' ';
            if (isError) marker = '!';
            else if (isWarning) marker = '?';

            TColorAttr attr = hl.getGutterAttr(isActive, isError, isWarning);
            std::string lineStr = std::format("{}{:>4}|", marker, lineNum);
            b.moveStr(0, lineStr, attr);
        } else {
            b.moveStr(0, "     |", hl.getGutterAttr());
        }

        writeBuf(0, y, size.x, 1, b);
    }
}

void TGutterView::handleEvent(TEvent& event) {
    TView::handleEvent(event);
    if (event.what == evMouseDown && editor) {
        int clickedLine = editor->delta.y + event.mouse.where.y + 1;
        editor->goToLineAndCol(clickedLine, 1);
        editor->select();
        clearEvent(event);
    }
}

// ============================================================================
// TTurboEditWindow Implementation
// ============================================================================

TTurboEditWindow::TTurboEditWindow(const TRect& bounds, TStringView aFileName, int aNumber) noexcept
    : TWindowInit(&TTurboEditWindow::initFrame),
      TWindow(bounds, 0, aNumber)
{
    options |= ofTileable;

    TScrollBar* hScrollBar = new TScrollBar(TRect(18, size.y - 1, size.x - 2, size.y));
    hScrollBar->hide();
    insert(hScrollBar);

    TScrollBar* vScrollBar = new TScrollBar(TRect(size.x - 1, 1, size.x, size.y - 1));
    vScrollBar->hide();
    insert(vScrollBar);

    TIndicator* indicator = new TIndicator(TRect(2, size.y - 1, 16, size.y));
    indicator->hide();
    insert(indicator);

    int gutterWidth = 6;
    gutter = new TGutterView(TRect(1, 1, 1 + gutterWidth, size.y - 1), nullptr);
    insert(gutter);

    TRect r(1 + gutterWidth, 1, size.x - 1, size.y - 1);
    editor = new TTurboEditor(r, hScrollBar, vScrollBar, indicator, aFileName);
    gutter->editor = editor;
    editor->setGutter(gutter);
    insert(editor);
}

void TTurboEditWindow::close() {
    if (editor && editor->isClipboard()) {
        hide();
    } else {
        TWindow::close();
    }
}

const char* TTurboEditWindow::getTitle(short) {
    if (!editor) return "NONAME00.CPP";

    std::string fname = editor->fileName;
    if (fname.empty()) fname = "NONAME00.CPP";
    std::filesystem::path p(fname);
    std::string base = p.filename().string();

    std::snprintf(titleBuffer, sizeof(titleBuffer), " %s%s ", base.c_str(), editor->modified ? " *" : "");
    return titleBuffer;
}

void TTurboEditWindow::handleEvent(TEvent& event) {
    TWindow::handleEvent(event);
    if (event.what == evBroadcast && event.message.command == cmUpdateTitle) {
        if (frame != nullptr) {
            frame->drawView();
        }
        clearEvent(event);
    }
    if (event.what == evCommand && event.message.command == TurboC::cmToggleGutter) {
        if (gutter) {
            if (gutter->state & sfVisible) {
                gutter->hide();
                TRect r = editor->getBounds();
                r.a.x = 1;
                editor->locate(r);
            } else {
                gutter->show();
                TRect r = editor->getBounds();
                r.a.x = 7;
                editor->locate(r);
            }
        }
        clearEvent(event);
    }
}

} // namespace TurboC
