#ifdef UI_QUICK_COMMANDS

#include "UITask.h"
#include <helpers/TxtDataHelpers.h>
#include "../MyMesh.h"
#include "target.h"
#include "UIList.h"

/* ---------------------------------------------------------------------------------
   "Invia comando": the configured commands, then "Config"
   --------------------------------------------------------------------------------- */

class QuickCmdScreen : public UIScreen {
  UITask* _task;
  int _sel, _top;

  // row -> slot index, or -1 for the "Config" row
  int slotForRow(int row, int& num_rows) {
    int slot = -1, n = 0;
    for (int i = 0; i < UI_QUICK_MAX_CMDS; i++) {
      if (_task->getQuickCmd(i)->text[0] == 0) continue;
      if (n == row) slot = i;
      n++;
    }
    num_rows = n + 1;
    return slot;
  }

public:
  QuickCmdScreen(UITask* task) : _task(task), _sel(0), _top(0) { }
  void reset() { _sel = _top = 0; }

  int render(DisplayDriver& display) override {
    int num_rows;
    slotForRow(0, num_rows);
    if (_sel >= num_rows) reset();

    drawListHeader(display, "Invia comando", NULL);
    int y = LIST_ROW_Y0;
    for (int row = _top; row < num_rows && row < _top + LIST_VISIBLE; row++, y += LIST_ROW_HEIGHT) {
      int slot = slotForRow(row, num_rows);
      if (slot < 0) {
        drawListRow(display, y, row == _sel, 2, "* Config");
      } else {
        QuickCmdSlot* q = _task->getQuickCmd(slot);
        char count[6];
        sprintf(count, "%d", q->n_dest);
        // leave room on the right for the recipients count
        drawListRow(display, y, row == _sel, 2, "");
        char filtered[UI_QUICK_TEXT_LEN];
        display.translateUTF8ToBlocks(filtered, q->text, sizeof(filtered));
        display.drawTextEllipsized(2, y, display.width() - 20, filtered);
        display.drawTextRightAlign(display.width() - 2, y, count);
      }
    }
    return 5000;
  }

  bool handleInput(char c) override {
    int num_rows;
    slotForRow(0, num_rows);
    switch (listInput(c, _sel, _top, num_rows)) {
      case LIST_MOVED: return true;
      case LIST_BACK:
        _task->gotoHomeScreen();
        return true;
      case LIST_SELECT: {
        int slot = slotForRow(_sel, num_rows);
        if (slot < 0) {
          _task->gotoQuickConfigScreen();
        } else {
          QuickCmdSlot* q = _task->getQuickCmd(slot);
          if (q->n_dest == 0) {
            _task->showAlert("Nessun destinatario", 1500);
          } else {
            int ok = _task->sendQuickCommand(slot);
            char tmp[24];
            sprintf(tmp, "Inviato a %d/%d", ok, q->n_dest);
            _task->showAlert(tmp, 1500);
          }
        }
        return true;
      }
      default:
        return false;
    }
  }
};

/* ---------------------------------------------------------------------------------
   "Config": all slots
   --------------------------------------------------------------------------------- */

class QuickConfigScreen : public UIScreen {
  UITask* _task;
  int _sel, _top;

public:
  QuickConfigScreen(UITask* task) : _task(task), _sel(0), _top(0) { }
  void reset() { _sel = _top = 0; }

  int render(DisplayDriver& display) override {
    char tmp[UI_QUICK_TEXT_LEN + 16];
    sprintf(tmp, "%d/%d", _task->getQuickCmdCount(), UI_QUICK_MAX_CMDS);
    drawListHeader(display, "Config comandi", tmp);

    int y = LIST_ROW_Y0;
    for (int i = _top; i < UI_QUICK_MAX_CMDS && i < _top + LIST_VISIBLE; i++, y += LIST_ROW_HEIGHT) {
      QuickCmdSlot* q = _task->getQuickCmd(i);
      if (q->text[0]) {
        sprintf(tmp, "%d. %s", i + 1, q->text);
      } else {
        sprintf(tmp, "%d. (vuoto)", i + 1);
      }
      drawListRow(display, y, i == _sel, 2, tmp);
    }
    return 5000;
  }

  bool handleInput(char c) override {
    switch (listInput(c, _sel, _top, UI_QUICK_MAX_CMDS)) {
      case LIST_MOVED: return true;
      case LIST_BACK:
        _task->gotoQuickCmdScreen(false);
        return true;
      case LIST_SELECT:
        _task->gotoQuickSlotScreen(_sel);
        return true;
      default:
        return false;
    }
  }
};

/* ---------------------------------------------------------------------------------
   One slot: text, recipients, send, delete
   --------------------------------------------------------------------------------- */

class QuickSlotScreen : public UIScreen {
  UITask* _task;
  int _slot, _sel, _top;
  enum { ITEM_TEXT, ITEM_DEST, ITEM_SEND, ITEM_DELETE, ITEM_COUNT };

public:
  QuickSlotScreen(UITask* task) : _task(task), _slot(0), _sel(0), _top(0) { }
  void open(int slot, bool reset) {
    _slot = slot;
    if (reset) _sel = _top = 0;
  }

  int render(DisplayDriver& display) override {
    QuickCmdSlot* q = _task->getQuickCmd(_slot);
    char tmp[UI_QUICK_TEXT_LEN + 16];
    sprintf(tmp, "Comando %d", _slot + 1);
    drawListHeader(display, tmp, NULL);

    int y = LIST_ROW_Y0;
    for (int i = _top; i < ITEM_COUNT && i < _top + LIST_VISIBLE; i++, y += LIST_ROW_HEIGHT) {
      switch (i) {
        case ITEM_TEXT:   sprintf(tmp, "Testo: %s", q->text[0] ? q->text : "-"); break;
        case ITEM_DEST:   sprintf(tmp, "Destinatari (%d)", q->n_dest); break;
        case ITEM_SEND:   strcpy(tmp, "Invia ora"); break;
        case ITEM_DELETE: strcpy(tmp, "Cancella comando"); break;
      }
      drawListRow(display, y, i == _sel, 2, tmp);
    }
    return 5000;
  }

  bool handleInput(char c) override {
    switch (listInput(c, _sel, _top, ITEM_COUNT)) {
      case LIST_MOVED: return true;
      case LIST_BACK:
        _task->gotoQuickConfigScreen(false);
        return true;
      case LIST_SELECT: {
        QuickCmdSlot* q = _task->getQuickCmd(_slot);
        if (_sel == ITEM_TEXT) {
          _task->gotoQuickTextScreen(_slot);
        } else if (_sel == ITEM_DEST) {
          _task->gotoQuickDestScreen(_slot);
        } else if (_sel == ITEM_SEND) {
          if (q->text[0] == 0) {
            _task->showAlert("Testo vuoto", 1500);
          } else if (q->n_dest == 0) {
            _task->showAlert("Nessun destinatario", 1500);
          } else {
            int ok = _task->sendQuickCommand(_slot);
            char tmp[24];
            sprintf(tmp, "Inviato a %d/%d", ok, q->n_dest);
            _task->showAlert(tmp, 1500);
          }
        } else if (_sel == ITEM_DELETE) {
          memset(q, 0, sizeof(*q));
          _task->saveQuickCmds();
          _task->showAlert("Cancellato", 1000);
          _task->gotoQuickConfigScreen(false);
        }
        return true;
      }
      default:
        return false;
    }
  }
};

/* ---------------------------------------------------------------------------------
   Recipients of a slot: companion contacts only, press to toggle
   --------------------------------------------------------------------------------- */

class QuickDestScreen : public UIScreen {
  UITask* _task;
  int _slot;
  uint16_t _idx[MAX_CONTACTS];
  int _count, _sel, _top;

public:
  QuickDestScreen(UITask* task) : _task(task), _slot(0), _count(0), _sel(0), _top(0) { }

  // rebuild the list of companion contacts, sorted by name
  void open(int slot) {
    _slot = slot;
    _count = _sel = _top = 0;
    const int name_key_len = 12;
    char (*keys)[name_key_len] = (char (*)[name_key_len]) malloc(MAX_CONTACTS * name_key_len);

    int start = the_mesh.getTotalContactSlots() - the_mesh.getNumContacts();  // skip anon slots
    ContactInfo c;
    for (int i = start; i < the_mesh.getTotalContactSlots() && _count < MAX_CONTACTS; i++) {
      if (!the_mesh.getContactByIdx(i, c)) continue;
      if (c.type != ADV_TYPE_CHAT || c.name[0] == 0) continue;

      // insertion sort by (lowercased) name prefix
      char key[name_key_len];
      int k = 0;
      for (; k < name_key_len - 1 && c.name[k]; k++) key[k] = tolower((unsigned char)c.name[k]);
      key[k] = 0;

      int pos = _count;
      if (keys) {
        while (pos > 0 && strcmp(keys[pos - 1], key) > 0) {
          _idx[pos] = _idx[pos - 1];
          memcpy(keys[pos], keys[pos - 1], name_key_len);
          pos--;
        }
        memcpy(keys[pos], key, name_key_len);
      }
      _idx[pos] = i;
      _count++;
    }
    if (keys) free(keys);
  }

  int render(DisplayDriver& display) override {
    char title[20], right[12];
    sprintf(title, "Dest. comando %d", _slot + 1);
    sprintf(right, "%d sel", _task->getQuickCmd(_slot)->n_dest);
    drawListHeader(display, title, right);

    if (_count == 0) {
      display.setColor(UIColor::secondary_txt);
      display.drawTextCentered(display.width() / 2, 30, "Nessun companion");
      return 5000;
    }

    int y = LIST_ROW_Y0;
    ContactInfo c;
    for (int i = _top; i < _count && i < _top + LIST_VISIBLE; i++, y += LIST_ROW_HEIGHT) {
      if (!the_mesh.getContactByIdx(_idx[i], c)) continue;
      drawListRow(display, y, i == _sel, 12, c.name);
      // checkbox, in the row's current colour
      display.drawRect(2, y, 7, 7);
      if (_task->isQuickRecipient(_slot, c.id.pub_key)) {
        display.fillRect(4, y + 2, 3, 3);
      }
    }
    return 5000;
  }

  bool handleInput(char c) override {
    switch (listInput(c, _sel, _top, _count)) {
      case LIST_MOVED: return true;
      case LIST_BACK:
        _task->gotoQuickSlotScreen(_slot, false);
        return true;
      case LIST_SELECT: {
        ContactInfo contact;
        if (_count > 0 && the_mesh.getContactByIdx(_idx[_sel], contact)) {
          if (_task->toggleQuickRecipient(_slot, contact.id.pub_key) < 0) {
            char tmp[24];
            sprintf(tmp, "Max %d destinatari", UI_QUICK_MAX_RECIPIENTS);
            _task->showAlert(tmp, 1500);
          }
        }
        return true;
      }
      default:
        return false;
    }
  }
};

/* ---------------------------------------------------------------------------------
   Text of a slot: on-screen keyboard driven by the joystick
     joystick = move, press = type the key, side button = cancel
     last row: Aa (upper/lower case), space, delete, OK (save)
   --------------------------------------------------------------------------------- */

#define KB_COLS        16
#define KB_CHAR_ROWS    3
#define KB_ROWS        (KB_CHAR_ROWS + 1)
#define KB_SPECIALS     4
#define KB_CELL_W       8
#define KB_ROW_Y0      12
#define KB_ROW_H       13

static const char* const kb_rows[KB_CHAR_ROWS] = {
  "abcdefghijklmnop",
  "qrstuvwxyz.,?!-:",
  "0123456789/@#+=*",
};
enum { KB_SHIFT, KB_SPACE, KB_DEL, KB_OK };
static const char* const kb_special_labels[KB_SPECIALS] = { "Aa", "spaz", "<-", "OK" };

class QuickTextScreen : public UIScreen {
  UITask* _task;
  int _slot;
  char _buf[UI_QUICK_TEXT_LEN];
  int _row, _col;
  bool _upper;

  char keyChar(int row, int col) const {
    char ch = kb_rows[row][col];
    return _upper ? toupper((unsigned char)ch) : ch;
  }

  void type(char ch) {
    int len = strlen(_buf);
    if (len >= UI_QUICK_TEXT_LEN - 1) {
      _task->showAlert("Testo pieno", 1000);
      return;
    }
    _buf[len] = ch;
    _buf[len + 1] = 0;
  }

public:
  QuickTextScreen(UITask* task) : _task(task), _slot(0), _row(0), _col(0), _upper(false) { _buf[0] = 0; }

  void open(int slot) {
    _slot = slot;
    StrHelper::strncpy(_buf, _task->getQuickCmd(slot)->text, sizeof(_buf));
    _row = _col = 0;
    _upper = false;
  }

  int render(DisplayDriver& display) override {
    display.setTextSize(1);

    // text field: show the tail of the text, followed by the cursor
    const int max_chars = display.width() / 6 - 2;
    int len = strlen(_buf);
    const char* tail = len > max_chars ? &_buf[len - max_chars] : _buf;
    display.setColor(UIColor::primary_txt);
    display.setCursor(0, 0);
    display.print(tail);
    display.print("_");
    display.drawRect(0, 9, display.width(), 1);

    for (int r = 0; r < KB_ROWS; r++) {
      int y = KB_ROW_Y0 + r * KB_ROW_H;
      if (r < KB_CHAR_ROWS) {
        for (int c = 0; c < KB_COLS; c++) {
          int x = c * KB_CELL_W;
          bool sel = (r == _row && c == _col);
          if (sel) {
            display.setColor(UIColor::primary_txt);   // inverted: light fill
            display.fillRect(x, y, KB_CELL_W, KB_ROW_H - 1);
            display.setColor(UIColor::window_bkg);    // dark text on it
          } else {
            display.setColor(UIColor::primary_txt);
          }
          char s[2] = { keyChar(r, c), 0 };
          display.setCursor(x + 1, y + 2);
          display.print(s);
        }
      } else {
        int w = display.width() / KB_SPECIALS;
        for (int c = 0; c < KB_SPECIALS; c++) {
          int x = c * w;
          bool sel = (r == _row && c == _col);
          if (sel) {
            display.setColor(UIColor::primary_txt);   // inverted: light fill
            display.fillRect(x, y, w, KB_ROW_H - 1);
            display.setColor(UIColor::window_bkg);    // dark text on it
          } else {
            display.setColor(UIColor::secondary_txt);
          }
          display.drawTextCentered(x + w / 2, y + 2, kb_special_labels[c]);
        }
      }
    }
    return 5000;
  }

  bool handleInput(char c) override {
    int width = (_row < KB_CHAR_ROWS) ? KB_COLS : KB_SPECIALS;
    switch (c) {
      case KEY_LEFT:
        _col = (_col + width - 1) % width;
        return true;
      case KEY_RIGHT:
        _col = (_col + 1) % width;
        return true;
      case KEY_UP:
      case KEY_DOWN: {
        int new_row = (c == KEY_UP) ? (_row + KB_ROWS - 1) % KB_ROWS : (_row + 1) % KB_ROWS;
        // keep roughly the same horizontal position between the char rows and the specials row
        if (_row < KB_CHAR_ROWS && new_row == KB_CHAR_ROWS) _col = _col * KB_SPECIALS / KB_COLS;
        else if (_row == KB_CHAR_ROWS && new_row < KB_CHAR_ROWS) _col = _col * KB_COLS / KB_SPECIALS + 1;
        _row = new_row;
        return true;
      }
      case KEY_CANCEL:
        _task->showAlert("Annullato", 800);
        _task->gotoQuickSlotScreen(_slot, false);
        return true;
      case KEY_ENTER:
        if (_row < KB_CHAR_ROWS) {
          type(keyChar(_row, _col));
        } else if (_col == KB_SHIFT) {
          _upper = !_upper;
        } else if (_col == KB_SPACE) {
          type(' ');
        } else if (_col == KB_DEL) {
          int len = strlen(_buf);
          if (len > 0) _buf[len - 1] = 0;
        } else if (_col == KB_OK) {
          QuickCmdSlot* q = _task->getQuickCmd(_slot);
          StrHelper::strncpy(q->text, _buf, sizeof(q->text));
          _task->saveQuickCmds();
          _task->showAlert("Salvato", 800);
          _task->gotoQuickSlotScreen(_slot, false);
        }
        return true;
    }
    return false;
  }
};

/* ---------------------------------------------------------------------------------
   UITask glue + persistence
   --------------------------------------------------------------------------------- */

void UITask::beginQuickCommands() {
  quick_cmd = new QuickCmdScreen(this);
  quick_config = new QuickConfigScreen(this);
  quick_slot = new QuickSlotScreen(this);
  quick_dest = new QuickDestScreen(this);
  quick_text = new QuickTextScreen(this);
}

void UITask::gotoQuickCmdScreen(bool reset) {
  if (reset) ((QuickCmdScreen *) quick_cmd)->reset();
  setCurrScreen(quick_cmd);
}

void UITask::gotoQuickConfigScreen(bool reset) {
  if (reset) ((QuickConfigScreen *) quick_config)->reset();
  setCurrScreen(quick_config);
}

void UITask::gotoQuickSlotScreen(int slot, bool reset) {
  ((QuickSlotScreen *) quick_slot)->open(slot, reset);
  setCurrScreen(quick_slot);
}

void UITask::gotoQuickDestScreen(int slot) {
  ((QuickDestScreen *) quick_dest)->open(slot);
  setCurrScreen(quick_dest);
}

void UITask::gotoQuickTextScreen(int slot) {
  ((QuickTextScreen *) quick_text)->open(slot);
  setCurrScreen(quick_text);
}

// file format: 1 byte version, 1 byte slot count, then the QuickCmdSlot array
static uint8_t qc_file_buf[2 + UI_QUICK_MAX_CMDS * sizeof(QuickCmdSlot)];

void UITask::loadQuickCmds() {
  if (_qc_loaded) return;
  _qc_loaded = true;
  memset(_qc, 0, sizeof(_qc));

  int len = the_mesh.getDataStore()->readSmallFile(UI_QUICK_CMDS_FILE, qc_file_buf, sizeof(qc_file_buf));
  if (len >= 2 && qc_file_buf[0] == UI_QUICK_CMDS_FILE_VER) {
    int n = qc_file_buf[1];
    if (n > UI_QUICK_MAX_CMDS) n = UI_QUICK_MAX_CMDS;
    if (len < 2 + n * (int)sizeof(QuickCmdSlot)) n = (len - 2) / sizeof(QuickCmdSlot);
    memcpy(_qc, &qc_file_buf[2], n * sizeof(QuickCmdSlot));
    for (int i = 0; i < n; i++) {   // sanitise
      _qc[i].text[UI_QUICK_TEXT_LEN - 1] = 0;
      if (_qc[i].n_dest > UI_QUICK_MAX_RECIPIENTS) _qc[i].n_dest = UI_QUICK_MAX_RECIPIENTS;
    }
  } else {   // first run: pre-load the default texts
    static const char* const defaults[] = { UI_QUICK_COMMAND_LIST };
    for (int i = 0; i < (int)(sizeof(defaults) / sizeof(defaults[0])) && i < UI_QUICK_MAX_CMDS; i++) {
      StrHelper::strncpy(_qc[i].text, defaults[i], sizeof(_qc[i].text));
    }
  }
}

void UITask::saveQuickCmds() {
  qc_file_buf[0] = UI_QUICK_CMDS_FILE_VER;
  qc_file_buf[1] = UI_QUICK_MAX_CMDS;
  memcpy(&qc_file_buf[2], _qc, sizeof(_qc));
  the_mesh.getDataStore()->writeSmallFile(UI_QUICK_CMDS_FILE, qc_file_buf, 2 + sizeof(_qc));
}

QuickCmdSlot* UITask::getQuickCmd(int slot) {
  loadQuickCmds();
  return &_qc[slot];
}

int UITask::getQuickCmdCount() {
  loadQuickCmds();
  int n = 0;
  for (int i = 0; i < UI_QUICK_MAX_CMDS; i++) {
    if (_qc[i].text[0]) n++;
  }
  return n;
}

bool UITask::isQuickRecipient(int slot, const uint8_t* pub_key) {
  QuickCmdSlot* q = getQuickCmd(slot);
  for (int i = 0; i < q->n_dest; i++) {
    if (memcmp(q->dest[i], pub_key, UI_QUICK_KEY_LEN) == 0) return true;
  }
  return false;
}

int UITask::toggleQuickRecipient(int slot, const uint8_t* pub_key) {
  QuickCmdSlot* q = getQuickCmd(slot);
  for (int i = 0; i < q->n_dest; i++) {
    if (memcmp(q->dest[i], pub_key, UI_QUICK_KEY_LEN) == 0) {   // remove
      q->n_dest--;
      if (i < q->n_dest) memmove(q->dest[i], q->dest[i + 1], (q->n_dest - i) * UI_QUICK_KEY_LEN);
      saveQuickCmds();
      return 0;
    }
  }
  if (q->n_dest >= UI_QUICK_MAX_RECIPIENTS) return -1;
  memcpy(q->dest[q->n_dest++], pub_key, UI_QUICK_KEY_LEN);
  saveQuickCmds();
  return 1;
}

int UITask::sendQuickCommand(int slot) {
  QuickCmdSlot* q = getQuickCmd(slot);
  if (q->text[0] == 0) return 0;
  int ok = 0;
  for (int i = 0; i < q->n_dest; i++) {
    if (the_mesh.sendTextFromUI(q->dest[i], UI_QUICK_KEY_LEN, q->text) != MSG_SEND_FAILED) ok++;
  }
  return ok;
}

void UITask::onUIMsgAck(const char* contact_name) {
  char name[18];
  if (_display != NULL) {
    _display->translateUTF8ToBlocks(name, contact_name, sizeof(name));
  } else {
    StrHelper::strncpy(name, contact_name, sizeof(name));
  }
  char tmp[24];
  sprintf(tmp, "ACK: %s", name);
  showAlert(tmp, 1200);
  notify(UIEventType::ack);
  _next_refresh = 0;
}

#endif  // UI_QUICK_COMMANDS
