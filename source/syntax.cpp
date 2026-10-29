#include "turboc/syntax.h"
#include <cctype>
#include <format>

namespace TurboC {

SyntaxHighlighter& SyntaxHighlighter::instance() {
    static SyntaxHighlighter inst;
    return inst;
}

SyntaxHighlighter::SyntaxHighlighter() {
    keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit",
        "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
        "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const",
        "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await",
        "co_return", "co_yield", "decltype", "default", "delete", "do", "double",
        "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
        "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
        "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
        "private", "protected", "public", "reflexpr", "register", "reinterpret_cast",
        "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
        "static_cast", "struct", "switch", "synchronized", "template", "this",
        "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
        "while", "xor", "xor_eq", "override", "final", "import", "module"
    };

    types = {
        "std", "string", "string_view", "vector", "map", "set", "unordered_map",
        "unordered_set", "queue", "stack", "deque", "list", "array", "span", "pair",
        "tuple", "optional", "variant", "any", "unique_ptr", "shared_ptr", "weak_ptr",
        "make_unique", "make_shared", "size_t", "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t", "ptrdiff_t",
        "cout", "cin", "cerr", "clog", "endl", "ostream", "istream", "stringstream",
        "ifstream", "ofstream", "fstream", "format", "print", "println", "ranges",
        "views", "filesystem", "chrono", "exception", "runtime_error", "out_of_range",
        "invalid_argument", "atomic", "mutex", "lock_guard", "unique_lock", "thread",
        "jthread", "future", "promise", "span"
    };

    preprocessorDirectives = {
        "#include", "#define", "#undef", "#if", "#ifdef", "#ifndef",
        "#elif", "#elifdef", "#elifndef", "#else", "#endif", "#pragma",
        "#line", "#error", "#warning"
    };
}

void SyntaxHighlighter::setThemeByName(std::string_view name) {
    if (name == "ModernDark") currentTheme = ColorTheme::ModernDark;
    else if (name == "RetroGreen") currentTheme = ColorTheme::RetroGreen;
    else if (name == "RetroAmber") currentTheme = ColorTheme::RetroAmber;
    else if (name == "BorlandPaper" || name == "ClassicPaper") currentTheme = ColorTheme::ClassicPaper;
    else if (name == "DarkViolet") currentTheme = ColorTheme::DarkViolet;
    else if (name == "SlateTeal") currentTheme = ColorTheme::SlateTeal;
    else if (name == "ObsidianBlue") currentTheme = ColorTheme::ObsidianBlue;
    else currentTheme = ColorTheme::ClassicBlue;
}

std::string_view SyntaxHighlighter::getThemeName() const {
    switch (currentTheme) {
        case ColorTheme::ModernDark: return "ModernDark";
        case ColorTheme::RetroGreen: return "RetroGreen";
        case ColorTheme::RetroAmber: return "RetroAmber";
        case ColorTheme::ClassicPaper: return "ClassicPaper";
        case ColorTheme::DarkViolet: return "DarkViolet";
        case ColorTheme::SlateTeal: return "SlateTeal";
        case ColorTheme::ObsidianBlue: return "ObsidianBlue";
        case ColorTheme::ClassicBlue:
        default: return "ClassicBlue";
    }
}

std::vector<Token> SyntaxHighlighter::tokenizeLine(std::string_view line, bool& inMultiLineComment) const {
    std::vector<Token> tokens;
    size_t i = 0;
    size_t len = line.length();

    // Check if continuing multi-line comment from previous line
    if (inMultiLineComment) {
        size_t endComment = line.find("*/", i);
        if (endComment == std::string_view::npos) {
            tokens.push_back({0, len, TokenType::Comment});
            return tokens;
        } else {
            tokens.push_back({0, endComment + 2, TokenType::Comment});
            i = endComment + 2;
            inMultiLineComment = false;
        }
    }

    while (i < len) {
        // Skip whitespace as Default tokens
        if (std::isspace(static_cast<unsigned char>(line[i]))) {
            size_t start = i;
            while (i < len && std::isspace(static_cast<unsigned char>(line[i]))) {
                i++;
            }
            tokens.push_back({start, i - start, TokenType::Default});
            continue;
        }

        // Single-line comment
        if (i + 1 < len && line[i] == '/' && line[i + 1] == '/') {
            tokens.push_back({i, len - i, TokenType::Comment});
            break;
        }

        // Multi-line comment start
        if (i + 1 < len && line[i] == '/' && line[i + 1] == '*') {
            size_t start = i;
            size_t end = line.find("*/", i + 2);
            if (end == std::string_view::npos) {
                inMultiLineComment = true;
                tokens.push_back({start, len - start, TokenType::Comment});
                break;
            } else {
                tokens.push_back({start, end + 2 - start, TokenType::Comment});
                i = end + 2;
                continue;
            }
        }

        // C++20/C++23 Attributes: [[nodiscard]], [[maybe_unused]], etc.
        if (line[i] == '[' && i + 1 < len && line[i + 1] == '[') {
            size_t start = i;
            size_t endAttr = line.find("]]", i + 2);
            if (endAttr != std::string_view::npos) {
                tokens.push_back({start, endAttr + 2 - start, TokenType::Attribute});
                i = endAttr + 2;
                continue;
            }
        }

        // Preprocessor directive (starts with #)
        if (line[i] == '#') {
            size_t start = i;
            while (i < len && (std::isalpha(static_cast<unsigned char>(line[i])) || line[i] == '#' || line[i] == '_')) {
                i++;
            }
            tokens.push_back({start, i - start, TokenType::Preprocessor});
            continue;
        }

        // C++ Raw string literal: R"(...)"
        if (line[i] == 'R' && i + 1 < len && line[i + 1] == '\"') {
            size_t start = i;
            size_t openParen = line.find('(', i + 2);
            if (openParen != std::string_view::npos) {
                std::string_view delim = line.substr(i + 2, openParen - (i + 2));
                std::string target = std::format("){}\"", delim);
                size_t closeParen = line.find(target, openParen + 1);
                if (closeParen != std::string_view::npos) {
                    i = closeParen + target.length();
                    tokens.push_back({start, i - start, TokenType::String});
                    continue;
                }
            }
        }

        // String literal
        if (line[i] == '\"') {
            size_t start = i++;
            while (i < len) {
                if (line[i] == '\\' && i + 1 < len) {
                    i += 2;
                } else if (line[i] == '\"') {
                    i++;
                    break;
                } else {
                    i++;
                }
            }
            tokens.push_back({start, i - start, TokenType::String});
            continue;
        }

        // Character literal
        if (line[i] == '\'') {
            size_t start = i++;
            while (i < len) {
                if (line[i] == '\\' && i + 1 < len) {
                    i += 2;
                } else if (line[i] == '\'') {
                    i++;
                    break;
                } else {
                    i++;
                }
            }
            tokens.push_back({start, i - start, TokenType::Character});
            continue;
        }

        // Number literal
        if (std::isdigit(static_cast<unsigned char>(line[i])) ||
            (line[i] == '.' && i + 1 < len && std::isdigit(static_cast<unsigned char>(line[i + 1])))) {
            size_t start = i;
            bool isHex = (line[i] == '0' && i + 1 < len && (line[i + 1] == 'x' || line[i + 1] == 'X'));
            bool isBin = (line[i] == '0' && i + 1 < len && (line[i + 1] == 'b' || line[i + 1] == 'B'));
            if (isHex || isBin) i += 2;

            while (i < len) {
                char c = line[i];
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '\'') {
                    i++;
                } else {
                    break;
                }
            }
            tokens.push_back({start, i - start, TokenType::Number});
            continue;
        }

        // Identifiers / Keywords / Types / Function calls (supports Unicode/accented identifiers in C23 & C++23)
        if (std::isalpha(static_cast<unsigned char>(line[i])) || line[i] == '_' || static_cast<unsigned char>(line[i]) >= 0x80) {
            size_t start = i;
            while (i < len && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_' || static_cast<unsigned char>(line[i]) >= 0x80)) {
                i++;
            }
            std::string_view word = line.substr(start, i - start);
            if (keywords.contains(word)) {
                tokens.push_back({start, i - start, TokenType::Keyword});
            } else if (types.contains(word)) {
                tokens.push_back({start, i - start, TokenType::Type});
            } else {
                // Check if followed by '(' -> function call or declaration
                size_t lookAhead = i;
                while (lookAhead < len && (line[lookAhead] == ' ' || line[lookAhead] == '\t')) {
                    lookAhead++;
                }
                if (lookAhead < len && line[lookAhead] == '(') {
                    tokens.push_back({start, i - start, TokenType::Function});
                } else {
                    tokens.push_back({start, i - start, TokenType::Default});
                }
            }
            continue;
        }

        // Operators & Punctuation
        char op = line[i];
        if (std::string_view("+-*/%=!<>^&|~?:;,.()[]{}").find(op) != std::string_view::npos) {
            tokens.push_back({i, 1, TokenType::Operator});
            i++;
            continue;
        }

        // Fallback default
        tokens.push_back({i, 1, TokenType::Default});
        i++;
    }

    return tokens;
}

TColorAttr SyntaxHighlighter::getAttrForToken(TokenType type, bool isSelected, bool isError, bool isWarning) const {
    if (isSelected) {
        switch (currentTheme) {
            case ColorTheme::DarkViolet:   return TColorAttr(0x5F); // Bright White on Magenta/Purple (Image 1)
            case ColorTheme::SlateTeal:    return TColorAttr(0x30); // Black on Bright Cyan/Teal (Image 2)
            case ColorTheme::ObsidianBlue: return TColorAttr(0x1F); // Bright White on Cobalt Blue (Image 3)
            case ColorTheme::RetroGreen:   return TColorAttr(0x20); // Black on Green
            case ColorTheme::RetroAmber:   return TColorAttr(0x60); // Black on Amber
            case ColorTheme::BorlandPaper: return TColorAttr(0x1F); // White on Blue
            case ColorTheme::ModernDark:
            case ColorTheme::BorlandBlue:
            default:                       return TColorAttr(0x70); // Inverse: Black on Gray/White
        }
    }
    if (isError) {
        return TColorAttr(0x4F); // White on Red
    }
    if (isWarning) {
        return TColorAttr(0x6E); // Yellow on Brown/Orange
    }
    if (type == TokenType::BracketMatch) {
        return getBracketMatchAttr();
    }

    switch (currentTheme) {
        case ColorTheme::ModernDark: {
            switch (type) {
                case TokenType::Keyword:      return TColorAttr(0x0D); // Light Magenta / Pink
                case TokenType::Type:         return TColorAttr(0x0B); // Light Cyan
                case TokenType::Preprocessor: return TColorAttr(0x0C); // Light Red
                case TokenType::String:
                case TokenType::Character:    return TColorAttr(0x0A); // Light Green
                case TokenType::Number:       return TColorAttr(0x0E); // Yellow
                case TokenType::Comment:      return TColorAttr(0x08); // Dark Gray
                case TokenType::Operator:     return TColorAttr(0x0F); // Bright White
                case TokenType::Function:     return TColorAttr(0x0E); // Bright Yellow / Gold
                case TokenType::Attribute:    return TColorAttr(0x0B); // Light Cyan
                case TokenType::Default:
                default:                      return TColorAttr(0x07); // Light Gray on Black
            }
        }
        case ColorTheme::RetroGreen: {
            switch (type) {
                case TokenType::Keyword:
                case TokenType::Type:
                case TokenType::Function:
                case TokenType::Number:
                case TokenType::String:
                case TokenType::Character:
                case TokenType::Attribute:    return TColorAttr(0x0A); // Bright Phosphor Green
                case TokenType::Comment:      return TColorAttr(0x08); // Dark Gray
                case TokenType::Preprocessor:
                case TokenType::Operator:
                case TokenType::Default:
                default:                      return TColorAttr(0x02); // Classic Green on Black
            }
        }
        case ColorTheme::RetroAmber: {
            switch (type) {
                case TokenType::Keyword:
                case TokenType::Type:
                case TokenType::Function:
                case TokenType::Number:
                case TokenType::String:
                case TokenType::Character:
                case TokenType::Attribute:    return TColorAttr(0x0E); // Bright Amber / Yellow
                case TokenType::Comment:      return TColorAttr(0x08); // Dark Gray
                case TokenType::Preprocessor:
                case TokenType::Operator:
                case TokenType::Default:
                default:                      return TColorAttr(0x06); // Amber / Brown on Black
            }
        }
        case ColorTheme::BorlandPaper: {
            switch (type) {
                case TokenType::Keyword:      return TColorAttr(0x71); // Blue on White
                case TokenType::Type:         return TColorAttr(0x73); // Teal on White
                case TokenType::Preprocessor: return TColorAttr(0x72); // Green on White
                case TokenType::String:
                case TokenType::Character:    return TColorAttr(0x72); // Green on White
                case TokenType::Number:       return TColorAttr(0x74); // Red on White
                case TokenType::Comment:      return TColorAttr(0x78); // Dark Gray on White
                case TokenType::Operator:     return TColorAttr(0x70); // Black on White
                case TokenType::Function:     return TColorAttr(0x71); // Blue on White
                case TokenType::Attribute:    return TColorAttr(0x75); // Magenta on White
                case TokenType::Default:
                default:                      return TColorAttr(0x70); // Black on White/Light Gray
            }
        }
        case ColorTheme::DarkViolet: { // Charcoal dark background with rich purple highlights & cyan/amber accents (Image 1)
            switch (type) {
                case TokenType::Keyword:      return TColorAttr(0x0D); // Light Magenta / Violet
                case TokenType::Type:         return TColorAttr(0x0B); // Light Cyan
                case TokenType::Preprocessor: return TColorAttr(0x0E); // Amber / Gold
                case TokenType::String:
                case TokenType::Character:    return TColorAttr(0x0A); // Light Green
                case TokenType::Number:       return TColorAttr(0x0B); // Light Cyan
                case TokenType::Comment:      return TColorAttr(0x08); // Dark Charcoal Gray
                case TokenType::Operator:     return TColorAttr(0x07); // Light Gray
                case TokenType::Function:     return TColorAttr(0x0F); // Crisp White
                case TokenType::Attribute:    return TColorAttr(0x0E); // Amber / Gold
                case TokenType::Default:
                default:                      return TColorAttr(0x07); // Light Slate Gray
            }
        }
        case ColorTheme::SlateTeal: { // Cool slate-navy background with bright cyan/teal highlights (Image 2)
            switch (type) {
                case TokenType::Keyword:      return TColorAttr(0x0D); // Pastel Rose / Pink
                case TokenType::Type:         return TColorAttr(0x0A); // Soft Green
                case TokenType::Preprocessor: return TColorAttr(0x0B); // Soft Cyan
                case TokenType::String:
                case TokenType::Character:    return TColorAttr(0x0A); // Soft Green
                case TokenType::Number:       return TColorAttr(0x0E); // Warm Peach / Amber
                case TokenType::Comment:      return TColorAttr(0x08); // Dark Slate Gray
                case TokenType::Operator:     return TColorAttr(0x07); // Silver
                case TokenType::Function:     return TColorAttr(0x0F); // Crisp White
                case TokenType::Attribute:    return TColorAttr(0x0C); // Light Salmon / Red
                case TokenType::Default:
                default:                      return TColorAttr(0x07); // Silver on Slate
            }
        }
        case ColorTheme::ObsidianBlue: { // Jet obsidian black with electric cobalt highlights & gold headers (Image 3)
            switch (type) {
                case TokenType::Keyword:      return TColorAttr(0x0E); // Bright Amber / Gold
                case TokenType::Type:         return TColorAttr(0x0B); // Light Cyan
                case TokenType::Preprocessor: return TColorAttr(0x0E); // Gold
                case TokenType::String:
                case TokenType::Character:    return TColorAttr(0x0A); // Light Green
                case TokenType::Number:       return TColorAttr(0x03); // Deep Cyan / Blue
                case TokenType::Comment:      return TColorAttr(0x08); // Dark Charcoal Gray
                case TokenType::Operator:     return TColorAttr(0x07); // Light Gray
                case TokenType::Function:     return TColorAttr(0x0F); // Crisp White
                case TokenType::Attribute:    return TColorAttr(0x0E); // Bright Amber / Gold
                case TokenType::Default:
                default:                      return TColorAttr(0x07); // White / Light Gray on Black
            }
        }
        case ColorTheme::BorlandBlue:
        default: {
            switch (type) {
                case TokenType::Keyword:      return TColorAttr(0x1F); // Bright White on Blue
                case TokenType::Type:         return TColorAttr(0x1B); // Light Cyan on Blue
                case TokenType::Preprocessor: return TColorAttr(0x1A); // Light Green on Blue
                case TokenType::String:
                case TokenType::Character:    return TColorAttr(0x1A); // Light Green on Blue
                case TokenType::Number:       return TColorAttr(0x1E); // Yellow on Blue
                case TokenType::Comment:      return TColorAttr(0x13); // Cyan on Blue
                case TokenType::Operator:     return TColorAttr(0x1E); // Yellow on Blue
                case TokenType::Function:     return TColorAttr(0x1F); // Bright White
                case TokenType::Attribute:    return TColorAttr(0x1D); // Light Magenta on Blue
                case TokenType::Default:
                default:                      return TColorAttr(0x17); // Light Gray on Blue
            }
        }
    }
}

TColorAttr SyntaxHighlighter::getBackgroundAttr(bool isError, bool isWarning) const {
    if (isError) return TColorAttr(0x4F);
    if (isWarning) return TColorAttr(0x6E);

    switch (currentTheme) {
        case ColorTheme::ModernDark:
        case ColorTheme::RetroGreen:
        case ColorTheme::RetroAmber:
        case ColorTheme::DarkViolet:
        case ColorTheme::SlateTeal:
        case ColorTheme::ObsidianBlue:
            return TColorAttr(0x07); // Deep dark background
        case ColorTheme::BorlandPaper:
            return TColorAttr(0x70); // Light gray / white background
        case ColorTheme::BorlandBlue:
        default:
            return TColorAttr(0x17); // Blue background
    }
}

TColorAttr SyntaxHighlighter::getGutterAttr(bool isActiveLine, bool isError, bool isWarning) const {
    if (isError) return TColorAttr(0x4F);   // White on Red
    if (isWarning) return TColorAttr(0x6E); // Yellow on Brown

    switch (currentTheme) {
        case ColorTheme::ModernDark:
            return isActiveLine ? TColorAttr(0x0F) : TColorAttr(0x08);
        case ColorTheme::RetroGreen:
            return isActiveLine ? TColorAttr(0x0A) : TColorAttr(0x08);
        case ColorTheme::RetroAmber:
            return isActiveLine ? TColorAttr(0x0E) : TColorAttr(0x08);
        case ColorTheme::DarkViolet:
            return isActiveLine ? TColorAttr(0x0D) : TColorAttr(0x08); // Violet on active line
        case ColorTheme::SlateTeal:
            return isActiveLine ? TColorAttr(0x0B) : TColorAttr(0x08); // Cyan/Teal on active line
        case ColorTheme::ObsidianBlue:
            return isActiveLine ? TColorAttr(0x0E) : TColorAttr(0x08); // Gold/Amber on active line
        case ColorTheme::BorlandPaper:
            return isActiveLine ? TColorAttr(0x7F) : TColorAttr(0x78);
        case ColorTheme::BorlandBlue:
        default:
            return isActiveLine ? TColorAttr(0x1F) : TColorAttr(0x13);
    }
}

TColorAttr SyntaxHighlighter::getBracketMatchAttr() const {
    switch (currentTheme) {
        case ColorTheme::ModernDark:
            return TColorAttr(0x2F); // Bright White on Green
        case ColorTheme::RetroGreen:
            return TColorAttr(0x20); // Black on Bright Green
        case ColorTheme::RetroAmber:
            return TColorAttr(0x60); // Black on Bright Amber
        case ColorTheme::DarkViolet:
            return TColorAttr(0x5F); // Bright White on Magenta/Purple
        case ColorTheme::SlateTeal:
            return TColorAttr(0x30); // Black on Bright Cyan/Teal
        case ColorTheme::ObsidianBlue:
            return TColorAttr(0x1E); // Bright Yellow on Blue
        case ColorTheme::BorlandPaper:
            return TColorAttr(0x1F); // Bright White on Blue
        case ColorTheme::BorlandBlue:
        default:
            return TColorAttr(0x3F); // Bright White on Cyan
    }
}

} // namespace TurboC
