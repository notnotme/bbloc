#ifndef CURSOR_CONTEXT_H
#define CURSOR_CONTEXT_H

#include "cursor/Cursor.h"
#include "cursor/PromptCursor.h"
#include "highlight/HighLighter.h"


struct CursorContext {
    PromptCursor& prompt_cursor;    ///< The prompt cursor of the application.
    Cursor cursor;                  ///< The Cursor who is tied to this context.
    HighLighter highlighter;        ///< The highlighter used to highlight the text.
    int32_t scroll_x;               ///< Horizontal scroll value of cursor.
    int32_t scroll_y;               ///< Vertical scroll value of this cursor.
    bool follow_indicator;          ///< Indicate that the view should scroll to the indicator when rendering.
    bool wants_redraw;              ///< indicate that the view and parents or sibling should redraw due to state change.

    explicit CursorContext(PromptCursor &promptCursor, std::unique_ptr<TextBuffer> buffer)
        : prompt_cursor(promptCursor),
          cursor(std::move(buffer)),
          highlighter(cursor),
          scroll_x(0),
          scroll_y(0),
          follow_indicator(false),
          wants_redraw(true) {}
};


#endif //CURSOR_CONTEXT_H
