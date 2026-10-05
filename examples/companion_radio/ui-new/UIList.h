#pragma once

// Simple scrolling list helpers shared by the extra UI menus (quick commands, device config)

#include <helpers/ui/DisplayDriver.h>
#include <helpers/ui/UIScreen.h>

#define LIST_ROW_Y0      14
#define LIST_ROW_HEIGHT  12
#define LIST_VISIBLE      4

static inline void drawListHeader(DisplayDriver& display, const char* title, const char* right) {
  display.setTextSize(1);
  display.setColor(UIColor::corp_blue);
  display.setCursor(0, 0);
  display.print(title);
  if (right) display.drawTextRightAlign(display.width() - 1, 0, right);
  display.drawRect(0, 11, display.width(), 1);  // horiz line
}

// draws a list row, inverted when selected. Leaves the colour set for any extra drawing on the row.
static inline void drawListRow(DisplayDriver& display, int y, bool selected, int text_x, const char* text) {
  if (selected) {
    display.setColor(UIColor::primary_txt);   // inverted: light fill
    display.fillRect(0, y - 2, display.width(), LIST_ROW_HEIGHT - 1);
    display.setColor(UIColor::window_bkg);    // dark text on it
  } else {
    display.setColor(UIColor::primary_txt);
  }
  char filtered[64];
  display.translateUTF8ToBlocks(filtered, text, sizeof(filtered));
  display.drawTextEllipsized(text_x, y, display.width() - text_x - 2, filtered);
}

static inline void moveSelection(int& sel, int& top, int count, int delta) {
  if (count <= 0) { sel = top = 0; return; }
  sel = (sel + delta + count) % count;
  if (sel < top) top = sel;
  if (sel >= top + LIST_VISIBLE) top = sel - LIST_VISIBLE + 1;
}

// up/down = move, right/press = select, left/side button = go back
enum ListAction { LIST_NONE, LIST_MOVED, LIST_SELECT, LIST_BACK };

static inline ListAction listInput(char c, int& sel, int& top, int count) {
  switch (c) {
    case KEY_UP:   case KEY_PREV: moveSelection(sel, top, count, -1); return LIST_MOVED;
    case KEY_DOWN: case KEY_NEXT: moveSelection(sel, top, count, 1);  return LIST_MOVED;
    case KEY_RIGHT: case KEY_ENTER: return LIST_SELECT;
    case KEY_LEFT:  case KEY_CANCEL: return LIST_BACK;
  }
  return LIST_NONE;
}
