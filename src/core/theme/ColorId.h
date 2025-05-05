#ifndef COLOR_ID_H
#define COLOR_ID_H


/**
 * @brief Enumeration of color identifiers used in the UI.
 *
 * These identifiers are used to access theme colors for UI components.
 */
enum class ColorId {
    MarginBackground,       ///< Background color of the margin area (line number container).
    LineBackground,         ///< Background color for the current (at cursor position) text lines.
    SelectedTextBackground, ///< Background color for selected text range.
    LineNumber,             ///< Color for the line numbers.
    InfoBarBackground,      ///< Background color of the info bar.
    EditorBackground,       ///< Background color of the editor area.
    PromptBackground,       ///< Background color of the command prompt.
    InfoBarText,            ///< Text color in the info bar.
    PromptText,             ///< Text color for static prompt messages.
    PromptInputText,        ///< Text color for user input in the prompt.
    Border,                 ///< Color used for borders (e.g. between components).
    CursorIndicator         ///< Color of the cursor indicator.
};


#endif //COLOR_ID_H
