#pragma once

// Modern Turbo C++ Command Codes
namespace TurboC {

const int
    // Core IDE commands
    cmCompile          = 2001,  // Alt+F9: Compile current file
    cmMake             = 2002,  // F9: Make / Build executable
    cmSyntaxCheck      = 2003,  // Ctrl+F7: LLVM Clang syntax check
    cmRunProgram       = 2004,  // Ctrl+F9: Run compiled program
    cmMessages         = 2005,  // F11: Toggle message window
    cmNextError        = 2006,  // Alt+F8: Jump to next diagnostic error/warning
    cmPrevError        = 2007,  // Alt+F7: Jump to previous diagnostic error/warning
    cmGoToLine         = 2008,  // Alt+G: Go to line dialog
    cmCompilerOptions  = 2009,  // Options -> Compiler & LLVM settings
    cmEditorOptions    = 2010,  // Options -> Editor settings
    cmClangFormat      = 2011,  // Ctrl+K: Reformat file with clang-format
    cmToggleGutter     = 2012,  // Ctrl+F8: Toggle line numbers gutter
    cmJumpToDiagnostic = 2013,  // Emitted when double-clicking / pressing Enter on an error in Message Window
    cmProgramArgs      = 2014,  // Run -> Program arguments
    cmAboutBox         = 2015,  // System -> About
    cmShortcutsHelp    = 2016,  // Help -> Keyboard shortcuts
    cmSaveAllFiles     = 2017,  // File -> Save all
    cmCloseAll         = 2018,  // Window -> Close all
    cmWindowList       = 2019,  // Window -> Window list (Alt+0)
    cmCompileObj       = 2020,  // Compile -> Compile to OBJ
    cmBuildAll         = 2021,  // Compile -> Build all
    cmCompilerInfo     = 2022,  // Compile -> Information
    cmDirectories      = 2023,  // Options -> Directories
    cmSaveOptions      = 2024,  // Options -> Save options
    cmShowClipboard    = 2025,  // Edit -> Show clipboard
    cmLocateFunction   = 2026,  // Search -> Locate function/symbol
    cmCleanArtifacts   = 2027,  // Run -> Clean build artifacts
    cmHelpContents     = 2028,  // Help -> Contents
    cmClangHelp        = 2029,  // Help -> LLVM Clang Reference
    cmRefreshScreen    = 2030,  // System -> Refresh screen
    cmHexView          = 2031,  // File -> Hex view
    cmInsertSnippet    = 2032,  // Edit -> Insert snippet (Ctrl+I)
    cmFileStats        = 2033,  // Search -> File statistics
    cmViewAssembly     = 2034,  // Compile -> View Assembly (Alt+F10)
    cmColorThemes      = 2035,  // Options -> Color themes
    cmPeriodicUpdate   = 2036;  // Broadcast emitted by idle() for responsive real-time modal updates

} // namespace TurboC
