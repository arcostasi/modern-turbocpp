#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>

#define Uses_TColorAttr
#define Uses_TDrawBuffer
#define Uses_TFrame
#include <tvision/tv.h>

namespace TurboC {

enum class TokenType {
    Default,
    Keyword,
    Type,
    Preprocessor,
    String,
    Character,
    Number,
    Comment,
    Operator,
    Function,
    Attribute,
    BracketMatch,
    Error,
    Warning
};

enum class ColorTheme {
    BorlandBlue,
    ClassicBlue = BorlandBlue,
    ModernDark,
    RetroGreen,
    RetroAmber,
    BorlandPaper,
    ClassicPaper = BorlandPaper,
    DarkViolet,
    SlateTeal,
    ObsidianBlue
};

struct Token {
    size_t start;
    size_t length;
    TokenType type;
};

class SyntaxHighlighter {
public:
    static SyntaxHighlighter& instance();

    // Sets active color theme
    void setTheme(ColorTheme aTheme) { currentTheme = aTheme; }
    void setThemeByName(std::string_view name);
    ColorTheme getTheme() const { return currentTheme; }
    std::string_view getThemeName() const;

    // Tokenizes a single line of C/C++ code. inMultiLineComment tracks state across lines.
    std::vector<Token> tokenizeLine(std::string_view line, bool& inMultiLineComment) const;

    // Maps a token type to Turbo Vision TColorAttr according to active theme
    TColorAttr getAttrForToken(TokenType type, bool isSelected = false, bool isError = false, bool isWarning = false) const;

    // Theme-aware attributes
    TColorAttr getBackgroundAttr(bool isError = false, bool isWarning = false) const;
    TColorAttr getGutterAttr(bool isActiveLine = false, bool isError = false, bool isWarning = false) const;
    TColorAttr getBracketMatchAttr() const;

private:
    SyntaxHighlighter();

    ColorTheme currentTheme = ColorTheme::BorlandBlue;
    std::unordered_set<std::string_view> keywords;
    std::unordered_set<std::string_view> types;
    std::unordered_set<std::string_view> preprocessorDirectives;
};

} // namespace TurboC
